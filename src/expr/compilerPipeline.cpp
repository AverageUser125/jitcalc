#include "compilerPipeline.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "arenaAllocator.hpp"
#include "JITcompiler.hpp"
#include <vector>

std::optional<CompiledFunction> compileFromSource(const std::string& sourceCode) {
	// the tokens have a string_view to a member string of the lexer
	// therfore you cannot call the destructor on the lexer before the parser has finished
	Lexer lexer(sourceCode);
	std::optional<std::vector<Token, ArenaAllocator<Token>>> tokenArrayOpt = lexer.lexerLexAllTokens();
	// lexer.lexerDebugPrintArray(*tokenArrayOpt);

	if (!tokenArrayOpt.has_value()) {
		return std::nullopt;
	}

	Parser parser(*tokenArrayOpt);
	// lifetime of tree pointer is the same as the parser object lifetime
	ExpressionNode* tree = parser.parserParseExpression();
	// parser.parserDebugDumpTree(tree);
	if (parser.hasError) {
		return std::nullopt;
	}

	JITCompiler jit;
	return jit.compile(tree);
}

#pragma region compiled function code

CompiledFunction::CompiledFunction(calcFunction fn, std::unique_ptr<llvm::orc::LLJIT> jit)
	: lljit(std::move(jit)), function(fn) {
}

CompiledFunction::CompiledFunction() : lljit(nullptr), function(nullptr) {
}

// Move constructor
CompiledFunction::CompiledFunction(CompiledFunction&& other) noexcept
	: lljit(std::move(other.lljit)), function(other.function) {
	other.function = nullptr;
}

// Move assignment operator
CompiledFunction& CompiledFunction::operator=(CompiledFunction&& other) noexcept {
	if (this != &other) {
		// Check if lljit is already destructed before assignment
		lljit = std::move(other.lljit);
		function = other.function;
		other.function = nullptr;
	}
	return *this;
}

// Assignment operator for a function
CompiledFunction& CompiledFunction::operator=(calcFunction func) {
	if (this->function != func) {
		manualDestructor();
		function = func;
	}
	return *this;
}

// Equality operators
bool CompiledFunction::operator==(calcFunction func) const {
	return function == func;
}

bool CompiledFunction::operator!=(calcFunction func) const {
	return function != func;
}

// Execute the compiled function
double CompiledFunction::operator()(double arg) const {
	assert(function != nullptr); // Check if function is valid
	return function(arg);
}

// Manual destructor for resource cleanup
void CompiledFunction::manualDestructor() {
	if (lljit.get() != nullptr) {
		lljit.get()->~LLJIT();
		lljit.release();
	}
	function = nullptr;
}

#pragma endregion