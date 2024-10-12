#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>

void main() {
	std::string input = "1+1";
	// std::getline(std::cin, input);
	Lexer lexer(input);
	
	Token tk;
	do {
		tk = lexer.lexerNextToken();
		lexer.lexerDebugPrintToken(tk);
	} while (tk.type != TokenType::EOF && tk.type != TokenType::Error);

	/*
	Parser parser(input);
	ExpressionNode& tree = parser.parserParseExpression(Precedence::MIN);
	parser.parserDebugDumpTree(&tree, 0);
	double answer = evaluate(&tree);
	std::cout << answer;
	*/
}