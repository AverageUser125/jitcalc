#include <llvm/Support/TargetSelect.h>
#include "mainGui.hpp"

int main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();


	int returnCode = guiLoop();

	return returnCode;
}