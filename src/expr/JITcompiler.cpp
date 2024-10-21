#include "JITcompiler.hpp"
#include "parser.hpp"
#include <cmath>
#include <iostream>

#undef NDEBUG
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>   // For file writing support
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/LegacyPassManager.h> // For legacy pass manager

JITCompiler::JITCompiler() {
	// Initialize the LLVM context, module, and builder
	context = std::make_unique<llvm::LLVMContext>();
	module = std::make_unique<llvm::Module>("calculator_module", *context);
	builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

calcFunction JITCompiler::compile(ExpressionNode* expr) {
	assert(module != nullptr);

	// Create a function with a double parameter for the variable
	auto funcType = llvm::FunctionType::get(builder->getDoubleTy(), {builder->getDoubleTy()}, false);
	auto func = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "evaluate", module.get());
	llvm::BasicBlock* block = llvm::BasicBlock::Create(*context, "entry", func);
	builder->SetInsertPoint(block);

	// Set the function's argument as the variable
	llvm::Value* variable = func->getArg(0);

	// Generate code for the expression and return the result
	llvm::Value* result = generateCode(expr, variable);
	builder->CreateRet(result);

	// Create an optimizer pass manager
	llvm::legacy::FunctionPassManager fpm(module.get());

	// The only optimization that does something, I just don't know what exactly
	fpm.add((llvm::Pass*)llvm::createTailCallEliminationPass());
	fpm.add((llvm::Pass*)llvm::createEarlyCSEPass());

	// Run the optimizer on the function
	fpm.doInitialization();
	fpm.run(*func);
	fpm.doFinalization();

	// Create the JIT execution engine
	llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::move(module)).create();
	if (!engine) {
		std::cerr << "Failed to create execution engine." << std::endl;
		return nullptr;
	}

	// Finalize the JIT object
	engine->finalizeObject();

	// Return the pointer to the compiled function
	return (double (*)(double))engine->getPointerToFunction(func);
}


llvm::Value* JITCompiler::generateCode(ExpressionNode* expr, llvm::Value* variable) {
	switch (expr->type) {
	case NodeType::Number:
		return llvm::ConstantFP::get(*context, llvm::APFloat(expr->number));
	case NodeType::Positive:
		return generateCode(expr->unary.operand, variable);
	case NodeType::Negative: {
		llvm::Value* operand = generateCode(expr->unary.operand, variable);
		return builder->CreateFNeg(operand);
	}
	case NodeType::Add: {
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);
		return builder->CreateFAdd(left, right, "addtmp");
	}
	case NodeType::Sub: {
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);
		return builder->CreateFSub(left, right, "subtmp");
	}
	case NodeType::Mul: {
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);
		return builder->CreateFMul(left, right, "multmp");
	}
	case NodeType::Div: {
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);
		return builder->CreateFDiv(left, right, "divtmp");
	}
	case NodeType::Pow: {
		createExternalFunction("pow");
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);

		// Check if both left and right are constants
		if (llvm::ConstantFP* leftConst = llvm::dyn_cast<llvm::ConstantFP>(left)) {
			if (llvm::ConstantFP* rightConst = llvm::dyn_cast<llvm::ConstantFP>(right)) {
				// If both are constants, calculate the result and return it as a constant
				double result =
					std::pow(leftConst->getValueAPF().convertToDouble(), rightConst->getValueAPF().convertToDouble());
				return llvm::ConstantFP::get(*context, llvm::APFloat(result));
			}
		}

		// Check if the right operand is another power expression
		if (expr->binary.left->type == NodeType::Pow) {
			// Get the base and inner exponent from the left power expression
			ExpressionNode* innerPow = expr->binary.left; // This is the left Pow
			llvm::Value* innerBase = generateCode(innerPow->binary.left, variable);
			llvm::Value* innerExponent = generateCode(innerPow->binary.right, variable);

			// Combine the inner exponent with the right exponent
			llvm::Value* outerExponent = right; // Use the exponent from the current Pow
			llvm::Value* newExponent = builder->CreateFMul(innerExponent, outerExponent, "exponentProduct");

			return builder->CreateCall(createdFunctions.at("pow"), {innerBase, newExponent}, "powtmp");
		}
		return builder->CreateCall(createdFunctions.at("pow"), {left, right}, "powtmp");
	}
	case NodeType::Variable: {
		return variable; // Return the variable (the function's argument)
	}
	case NodeType::Function: {
		createExternalFunction(expr->function.name);
		llvm::Value* argValue = generateCode(expr->function.argument, variable);
		return builder->CreateCall(createdFunctions.at(expr->function.name), {argValue}, "funccalltmp");
	}
	case NodeType::Error: {
		assert(0 && "ERROR WAS FOUND!, YOU PROBABLY FORGOT TO CHECK FOR IT");
		break;
	}
	default:
		return nullptr;
		break;
	}
	// llvm_unreachable();
	unreachable();
}


void JITCompiler::createExternalFunction(const std::string_view name) {
	// Check if the function has already been created
	if (createdFunctions.find(name) == createdFunctions.end()) {
		auto funcType = llvm::FunctionType::get(builder->getDoubleTy(), {builder->getDoubleTy()}, false);
		llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module.get());
		func->addFnAttr(llvm::Attribute::ReadNone);
		func->addFnAttr(llvm::Attribute::NoUnwind);
		func->addFnAttr(llvm::Attribute::AlwaysInline);
		createdFunctions[name] = func; // Mark this function as created
	}
}