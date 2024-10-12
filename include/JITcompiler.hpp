#pragma once

#include <memory>

// Forward declarations of LLVM types
/*
namespace llvm
{
struct LLVMContext;
struct Module;
struct ExecutionEngine;
struct Value;
struct Function;
} // namespace llvm
*/
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
struct ExpressionNode;

class JITCompiler {
  public:
	JITCompiler();
	double (*compile(ExpressionNode* expr))(double);


  private:
	llvm::Value* generateCode(ExpressionNode* expr, llvm::Value* variable);
	llvm::Function* getPowFunction();

	std::unique_ptr<llvm::LLVMContext> context;
	std::unique_ptr<llvm::IRBuilder<>> builder;
	std::unique_ptr<llvm::Module> module;
};

int NOTMAIN();