#include <llvm/Support/TargetSelect.h>
#include "arenaAllocator.hpp"
#include "mainGui.hpp"

Arena global_arena;

int main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	arena_init(&global_arena);
	int returnCode = guiLoop();

	// The OS frees the memory when the program ends already
	// arena_free(&global_arena);

	return returnCode;
}