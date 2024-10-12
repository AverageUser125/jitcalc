#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>
#include <llvm/Support/TargetSelect.h>

void main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	std::string input = "1+1";
	// std::getline(std::cin, input);
	Parser parser(input);
	ExpressionNode* tree = parser.parserParseExpression(Precedence::MIN);
	parser.parserDebugDumpTree(tree, 0);
	double answer = evaluate(tree);
	std::cout << answer;
	
}