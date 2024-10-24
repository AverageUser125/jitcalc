#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

using calcFunction = double (*)(double);

// Forward declarations of LLVM types

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <tools.hpp>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
struct ExpressionNode;

#include <memory>  // For std::shared_ptr
#include <utility> // For std::move

class CompiledFunction {
  public:

	CompiledFunction(calcFunction fn, std::unique_ptr<llvm::orc::LLJIT> jit) : lljit(std::move(jit)), function(fn) {
	}

	CompiledFunction() : lljit(nullptr), function(nullptr) {
	}

	// deleted because LLJIT cannot be copied
	CompiledFunction(const CompiledFunction& other) = delete;
	CompiledFunction& operator=(const CompiledFunction& other) = delete;

	// Move constructor
	CompiledFunction(CompiledFunction&& other) noexcept : lljit(std::move(other.lljit)), function(other.function) {
		other.function = nullptr;
	}


	// Move assignment operator
	CompiledFunction& operator=(CompiledFunction&& other) noexcept {
		if (this != &other) {
			// needs to check if lljit is already destructed before doing this
			lljit = std::move(other.lljit);
			function = other.function;
			other.function = nullptr;
		}
		return *this;
	}

	// Assignment operator for a function
	CompiledFunction& operator=(calcFunction func) {
		if (this->function != func) {
			manualDestructor();
			function = func;
		}
		return *this;
	}

	// Equality operators
	bool operator==(calcFunction func) const {
		return function == func;
	}

	bool operator!=(calcFunction func) const {
		return function != func;
	}

	// Execute the compiled function
	double operator()(double arg) const {
		permaAssert(function != nullptr);
		return function(arg);
	}

	// Manual destructor for resource cleanup
	void manualDestructor() {
		lljit.release();
		function = nullptr;
	}

  private:
	std::unique_ptr<llvm::orc::LLJIT> lljit; // Unique ownership of LLJIT
	calcFunction function = nullptr;		 // Pointer to the function
};

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
	llvm::Value* variable = nullptr;
	
	std::unique_ptr<llvm::Module> module;
	std::unordered_map<std::string_view, llvm::Function*> createdFunctions;
};