#include "arenaAllocator.hpp"
#include "mainGui.hpp"
#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/ManagedStatic.h"
Arena global_arena;

int main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	arena_init(&global_arena);
	int returnCode = guiLoop();

	// The OS frees the memory when the program ends already
	// arena_free(&global_arena);

	llvm::llvm_shutdown();
	return returnCode;
}