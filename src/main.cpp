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

	
	if (!guiInit()) {
		return EXIT_FAILURE;
	}
	guiLoop();
	guiCleanup();
	
	/*
	
	return EXIT_SUCCESS;
	*/
}