#pragma once

#include <memory>
using calcFunction = double (*)(double);

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
	llvm::Function* powFunction;

	std::unique_ptr<llvm::LLVMContext> context;
	std::unique_ptr<llvm::IRBuilder<>> builder;
	std::unique_ptr<llvm::Module> module;
};