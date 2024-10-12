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

double JITCompiler::compileAndRun(ExpressionNode* expr) {
	// Create a function for evaluating the expression
	auto funcType = llvm::FunctionType::get(builder->getDoubleTy(), false);
	auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "evaluate", module.get());
	llvm::BasicBlock* block = llvm::BasicBlock::Create(*context, "entry", func);
	builder->SetInsertPoint(block);

	// Generate code for the expression and return the result
	llvm::Value* result = generateCode(expr);
	builder->CreateRet(result);

	// Now that the module is fully constructed, create the JIT execution engine
	llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::move(module)).create();
	if (!engine) {
		std::cerr << "Failed to create execution engine." << std::endl;
		return 0.0;
	}

	// Finalize the JIT object
	engine->finalizeObject();

	// Get the pointer to the JIT compiled function and execute it
	auto evalFunc = (double (*)())engine->getPointerToFunction(func);
	if (!evalFunc) {
		std::cerr << "Failed to get function pointer." << std::endl;
		return 0.0;
	}

	return evalFunc();
}

llvm::Value* JITCompiler::generateCode(ExpressionNode* expr) {
	switch (expr->type) {
	case NodeType::Number:
		return llvm::ConstantFP::get(*context, llvm::APFloat(expr->number));
	case NodeType::Positive:
		return generateCode(expr->unary.operand);
	case NodeType::Negative: {
		llvm::Value* operand = generateCode(expr->unary.operand);
		return builder->CreateFNeg(operand);
	}
	case NodeType::Add: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builder->CreateFAdd(left, right, "addtmp");
	}
	case NodeType::Sub: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builder->CreateFSub(left, right, "subtmp");
	}
	case NodeType::Mul: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builder->CreateFMul(left, right, "multmp");
	}
	case NodeType::Div: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builder->CreateFDiv(left, right, "divtmp");
	}
	case NodeType::Pow: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builder->CreateCall(getPowFunction(), {left, right}, "powtmp");
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