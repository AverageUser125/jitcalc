#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>
#include <llvm/Support/TargetSelect.h>
#include "JITcompiler.hpp"

void main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	
	std::string input = "1 + x";
	// std::getline(std::cin, input);
	Parser parser(input);
	ExpressionNode* tree = parser.parserParseExpression(Precedence::MIN);
	parser.parserDebugDumpTree(tree, 0);

	// JITCompiler jit;
	// std::cout << jit.compileAndRun(tree);

	double answer = evaluate(tree, 5);
	std::cout << answer;
	
}