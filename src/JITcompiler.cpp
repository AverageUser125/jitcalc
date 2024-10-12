#include "JITCompiler.hpp"
#include "parser.hpp"
#include <cmath>
#include <iostream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

JITCompiler::JITCompiler() {
	context = std::make_unique<llvm::LLVMContext>();
	module = std::make_unique<llvm::Module>("expr_module", *context);
	builder = std::make_unique<llvm::IRBuilder<>>(*context);
	engine = llvm::EngineBuilder(std::move(module)).create();
}

double JITCompiler::compileAndRun(ExpressionNode* expr) {
	auto funcType = llvm::FunctionType::get(builder->getDoubleTy(), false);
	auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "evaluate", module.get());
	llvm::BasicBlock* block = llvm::BasicBlock::Create(*context, "entry", func);
	builder->SetInsertPoint(block);

	llvm::Value* result = generateCode(expr);
	builder->CreateRet(result);

	engine->finalizeObject();

	// Create a pointer to the JIT compiled function
	auto evalFunc = (double (*)())engine->getPointerToFunction(func);
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
	}
	return nullptr;
}

llvm::Function* JITCompiler::getPowFunction() {
	auto funcType =
		llvm::FunctionType::get(builder->getDoubleTy(), {builder->getDoubleTy(), builder->getDoubleTy()}, false);
	auto powFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "pow", module.get());
	return powFunc;
}
