#pragma once

#include <memory>
#include <unordered_map>
#include <string_view>

using calcFunction = double (*)(double);

// Forward declarations of LLVM types

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <tools.hpp>
struct ExpressionNode;

class CompiledFunction {
  public:

	CompiledFunction(llvm::ExecutionEngine* eng, llvm::Module* md,  calcFunction fn)
		: module(md), engine(eng), function(fn) {
	}

	CompiledFunction() : module(nullptr), engine(nullptr), function(nullptr) {
	}

	void manualDestructor() {
		if (engine) {
			engine->removeModule(module);
			// engine->runStaticConstructorsDestructors(true);
			delete engine;
		}
		engine = nullptr;
		function = nullptr;
	}

	// Function to execute the compiled function
	double operator()(double arg) const {
		permaAssert(function != nullptr);
		return function(arg);
	}

	CompiledFunction& operator=(const CompiledFunction& other) {
		if (this != &other) {
			manualDestructor();

			module = other.module;
			engine = other.engine;
			function = other.function;
		}
		return *this;
	}

	CompiledFunction& operator=(const calcFunction& func) {
		if (this->function != func) {
			manualDestructor();
			function = func;
		}
		return *this;
	}

	bool operator==(const calcFunction& func) const {
		return function == func;
	}

	bool operator!=(const calcFunction& func) const {
		return function != func;
	}

  private:
	// unique_ptr doesn't work, so manual removal is needed
	llvm::Module* module = nullptr;
	llvm::ExecutionEngine* engine = nullptr;
	calcFunction function = nullptr;
};

class JITCompiler {
  public:
	JITCompiler();
	CompiledFunction compile(ExpressionNode* expr);


  private:
	llvm::Value* generateCode(ExpressionNode* expr, llvm::Value* variable);
	void createExternalFunction(const std::string_view name);

	std::unique_ptr<llvm::LLVMContext> context;
	std::unique_ptr<llvm::IRBuilder<>> builder;
	std::unique_ptr<llvm::Module> module;
	std::unordered_map<std::string_view, llvm::Function*> createdFunctions;
};