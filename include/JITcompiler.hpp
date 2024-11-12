#pragma once

#include <memory>
#include <utility>
#include <unordered_map>
#include <tools.hpp>
#include <llvm/IR/IRBuilder.h>
struct ExpressionNode;
class string_view;

// Forward declarations of LLVM types
namespace llvm
{
class Value;
class Function;
class FunctionType;
class Module;
namespace orc
{
class ThreadSafeModule;
}
}

class CompiledFunction;


class JITCompiler {
  public:
	JITCompiler();
	CompiledFunction compile(ExpressionNode* expr);


  private:
	llvm::Value* generateCode(ExpressionNode* expr);
	void createExternalFunction(const std::string_view name);

	llvm::orc::ThreadSafeModule createModule(ExpressionNode* expr);
	
	llvm::IRBuilder<>* builderPtr = nullptr;
	llvm::LLVMContext* contextPtr = nullptr;
	llvm::Module* modulePtr = nullptr;
	llvm::Value* variable = nullptr;
	llvm::FunctionType* funcType = nullptr;
	
	std::unordered_map<std::string_view, llvm::Function*> createdFunctions;
};