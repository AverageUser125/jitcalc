#include "JITCompiler.hpp"
#include "parser.hpp"
#include <cmath>
#include <iostream>


JITCompiler::JITCompiler() {
	// Initialize the LLVM context, module, and builder
	context = std::make_unique<llvm::LLVMContext>();
	module = std::make_unique<llvm::Module>("calculator_module", *context);
	builder = std::make_unique<llvm::IRBuilder<>>(*context);
}


double (*JITCompiler::compile(ExpressionNode* expr))(double) {
	// Create a function with a double parameter for the variable
	auto funcType = llvm::FunctionType::get(builder->getDoubleTy(), {builder->getDoubleTy()}, false);
	auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "evaluate", module.get());
	llvm::BasicBlock* block = llvm::BasicBlock::Create(*context, "entry", func);
	builder->SetInsertPoint(block);

	// Set the function's argument as the variable
	llvm::Value* variable = func->getArg(0);

	// Generate code for the expression and return the result
	llvm::Value* result = generateCode(expr, variable);
	builder->CreateRet(result);

	// Now that the module is fully constructed, create the JIT execution engine
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
		llvm::Value* left = generateCode(expr->binary.left, variable);
		llvm::Value* right = generateCode(expr->binary.right, variable);
		return builder->CreateCall(getPowFunction(), {left, right}, "powtmp");
	}
	case NodeType::Variable: {
		// Return the variable (the function's argument)
		return variable;
	}
	default:
		return nullptr;
	}
}

llvm::Function* JITCompiler::getPowFunction() {
	// Define the pow function for exponentiation
	auto funcType =
		llvm::FunctionType::get(builder->getDoubleTy(), {builder->getDoubleTy(), builder->getDoubleTy()}, false);
	auto powFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "pow", module.get());
	return powFunc;
}