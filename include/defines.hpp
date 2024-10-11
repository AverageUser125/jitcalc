#pragma once

// probably will only focus on windows but we will see
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define PLATFORM_WIN 1
#define PLATFORM_LINUX 0
#elif defined(__linux__)
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX 1
#else
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX 0
#endif