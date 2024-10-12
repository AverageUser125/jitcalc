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



	std::string input = "(x-5)^(0.5)";


	// std::getline(std::cin, input);
	Parser parser(input);
	ExpressionNode* tree = parser.parserParseExpression(Precedence::MIN);
	parser.parserDebugDumpTree(tree, 0);

	JITCompiler jit;
	auto func = jit.compile(tree);

	for (double i = 1; i < 6; i++) {
		std::cout << func(i) << '\n';
	}
	
}