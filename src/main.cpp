#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>
#include <llvm/Support/TargetSelect.h>
#include "JITcompiler.hpp"
#include "mainGui.hpp"


int main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	/*
	if (!guiInit()) {
		return EXIT_FAILURE;
	}
	guiLoop();
	guiCleanup();
	*/
	
	// x ^ (0.5) / (x ^ 2) causes crash (negative x)
	// x ^ (0.5) / (2 ) doesn't crash
	std::string input = "x * x";

	// std::getline(std::cin, input);
	Parser parser(input);
	ExpressionNode* tree = parser.parserParseExpression(Precedence::MIN);
	parser.parserDebugDumpTree(tree, 0);

	JITCompiler jit;
	auto func = jit.compile(tree);


	for (double i = -1; i <= 2; i++) {
		std::cout << func(i) << '\n';
	}
	

	return EXIT_SUCCESS;
}