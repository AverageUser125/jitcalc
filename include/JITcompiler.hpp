#pragma once

#include <memory>

// Forward declarations of LLVM types
namespace llvm
{
struct LLVMContext;
struct Module;
struct ExecutionEngine;
struct Value;
struct Function;
} // namespace llvm

struct ExpressionNode;
#include <llvm/IR/IRBuilder.h>

class JITCompiler {
  public:
	JITCompiler();
	double compileAndRun(ExpressionNode* expr);

  private:
	llvm::Value* generateCode(ExpressionNode* expr);
	llvm::Function* getPowFunction();

	std::unique_ptr<llvm::LLVMContext> context;
	std::unique_ptr<llvm::Module> module;
	std::unique_ptr<llvm::IRBuilder<>> builder;
	llvm::ExecutionEngine* engine;
};

int NOTMAIN();