#pragma once

// probably will only focus on windows but we will see
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define PLATFORM_WIN 1
#define PLATFORM_LINUX 0
#define PLATFORM_MAC 0
#elif defined(__linux__)
#define PLATFORM_WIN 0
#define PLATFORM_LINUX 1
#define PLATFORM_MAC 0
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_WIN 0
#define PLATFORM_LINUX 0
#define PLATFORM_MAC 1
#else
#define PLATFORM_WIN 0
#define PLATFORM_LINUX 0
#define PLATFORM_MAC 0
#endif

[[noreturn]] inline void unreachable() {
	// Uses compiler specific extensions if possible.
	// Even if no extension is used, undefined behavior is still raised by
	// an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
	__assume(false);
#else // GCC, Clang
	__builtin_unreachable();
#endif
}

#if defined(__cplusplus)					// Check if compiling in C++
#if defined(__GNUC__) || defined(__clang__) // For GCC/Clang
#define RESTRICT __restrict__
#elif defined(_MSC_VER) // For MSVC
#define RESTRICT __restrict
#else
#define RESTRICT // Fallback if unknown C++ compiler
#endif
#else // For C
#define RESTRICT restrict
#endif
