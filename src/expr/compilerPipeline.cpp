#include "compilerPipeline.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "arenaAllocator.hpp"
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