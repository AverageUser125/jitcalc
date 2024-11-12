#pragma once
#include <string>
#include <optional>
#include <memory>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

using calcFunction = double (*)(double);

class CompiledFunction {
  public:
	// Constructors
	CompiledFunction(calcFunction fn, std::unique_ptr<llvm::orc::LLJIT> jit);
	CompiledFunction();

	// Deleted copy constructor and assignment operator
	CompiledFunction(const CompiledFunction& other) = delete;
	CompiledFunction& operator=(const CompiledFunction& other) = delete;

	// Move constructor and move assignment operator
	CompiledFunction(CompiledFunction&& other) noexcept;
	CompiledFunction& operator=(CompiledFunction&& other) noexcept;

	// Assignment operator for a function
	CompiledFunction& operator=(calcFunction func);

	// Equality operators
	bool operator==(calcFunction func) const;
	bool operator!=(calcFunction func) const;

	// Execute the compiled function
	double operator()(double arg) const;

	// Manual destructor for resource cleanup
	void manualDestructor();

  private:
	std::unique_ptr<llvm::orc::LLJIT> lljit; // Unique ownership of LLJIT
	calcFunction function = nullptr;		 // Pointer to the function
};


std::optional<CompiledFunction> compileFromSource(const std::string& sourceCode);
