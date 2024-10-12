#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>

void main() {
	std::string input;
	std::getline(std::cin, input);
	Parser parser(input);
	ExpressionNode& tree = parser.parserParseExpression(Precedence::MIN);
	double answer = evaluate(&tree);
	std::cout << answer;
}