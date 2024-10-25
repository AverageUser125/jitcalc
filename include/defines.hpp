#pragma once

// probably will only focus on windows but we will see
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(_WIN64) && !defined(__CYGWIN__)
#define PLATFORM_WIN 1
#define PLATFORM_LINUX 0
#define PLATFORM_MAC 0
#elif defined(__linux__) || defined(linux) || defined(__linux)
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

// Compiler detection
#if defined(__GNUC__)
#define COMPILER_CLANG 0
#define COMPILER_MSVC 0
#define COMPILER_GCC 1
#elif defined(__clang__)
#define COMPILER_CLANG 1
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#elif defined(_MSC_VER)
#define COMPILER_CLANG 0
#define COMPILER_MSVC 1
#define COMPILER_GCC 0
#elif defined(__INTEL_COMPILER)
#define COMPILER_CLANG 0
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#else
#define COMPILER_CLANG 0
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#endif

// Architecture detection
#if defined(_M_IX86) || defined(__i386__) // x86
#define ARCH_X86 1
#define ARCH_X86_64 0
#define ARCH_ARM 0
#define ARCH_ARM64 0
#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) // x86_64
#define ARCH_X86 0
#define ARCH_X86_64 1
#define ARCH_ARM 0
#define ARCH_ARM64 0
#elif defined(__arm__) // ARM
#define ARCH_X86 0
#define ARCH_X86_64 0
#define ARCH_ARM 1
#define ARCH_ARM64 0
#elif defined(__aarch64__) // ARM64
#define ARCH_X86 0
#define ARCH_X86_64 0
#define ARCH_ARM 0
#define ARCH_ARM64 1
#else
#define ARCH_X86 0
#define ARCH_X86_64 0
#define ARCH_ARM 0
#define ARCH_ARM64 0
#endif

[[noreturn]] inline void unreachable() {
	// Uses compiler specific extensions if possible.
	// Even if no extension is used, undefined behavior is still raised by
	// an empty function body and the noreturn attribute.
#if COMPILER_MSVC && !COMPILER_CLANG // MSVC
	__assume(false);
#else // GCC, Clang
	__builtin_unreachable();
#endif
}

#if defined(__cplusplus)		   // Check if compiling in C++
#if COMPILER_GCC || COMPILER_CLANG // For GCC/Clang
#define RESTRICT __restrict__
#elif COMPILER_MSVC // For MSVC
#define RESTRICT __restrict
#else
#define RESTRICT // Fallback if unknown C++ compiler
#endif
#else // For C
#define RESTRICT restrict
#endif

#if COMPILER_MSVC
#define ALWAYS_INLINE __forceinline
#elif COMPILER_GCC || COMPILER_CLANG
#define ALWAYS_INLINE __attribute__((always_inline))
#endif

#define KB(x) ((unsigned long long)1024 * x)
#define MB(x) ((unsigned long long)1024 * KB(x))
#define GB(x) ((unsigned long long)1024 * MB(x))

#if COMPILER_MSVC
#define DEBUG_BREAK() __debugbreak()
#elif COMPILER_CLANG && __has_builtin(__builtin_debugtrap)
#define DEBUG_BREAK() __builtin_debugtrap()
#elif PLATFORM_MAC && ARCH_ARM64
#define DEBUG_BREAK() __builtin_trap()
#elif ARCH_X86 || ARCH_X86_64
#define DEBUG_BREAK() __asm__ volatile("int $0x03")
#elif ARCH_ARM
#define DEBUG_BREAK() __asm__ volatile(".inst 0xe7f001f0")
#elif ARCH_ARM64
#define DEBUG_BREAK() __asm__ volatile(".inst 0xd4200000")
#else
#include <signal.h>
#ifdef SIGTRAP
#define DEBUG_BREAK() raise(SIGTRAP)
#else
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#endif
