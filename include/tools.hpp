#pragma once

#include "defines.hpp"
#include <iostream>

#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)
#if PLATFORM_WIN
enum class ConsoleColor {
	BLUE = 9,
	GREEN = 10,
	RED = 12,
	YELLOW = 14,
};
#else
enum class ConsoleColor {
	BLUE = 34,
	GREEN = 32,
	RED = 31,
	YELLOW = 33,
};
#endif
void assertFuncProduction(const char* expression, const char* file_name, const unsigned int line_number,
								 const char* comment = "---");
void assertFuncInternal(const char* expression, const char* file_name, const unsigned int line_number,
							   const char* comment = "---");

void setConsoleColor(ConsoleColor color);
void resetConsoleColor();

#if PRODUCTION_BUILD == 0

#define permaAssert(expression)                                                                                        \
	(void)((!!(expression)) || (assertFuncInternal(#expression, __FILENAME__, (unsigned)(__LINE__)), 0))

#define permaAssertComment(expression, comment)                                                                        \
	(void)((!!(expression)) || (assertFuncInternal(#expression, __FILENAME__, (unsigned)(__LINE__), comment), 1))

#define debugAssert(expression)                                                                                        \
	(void)((!!(expression)) || (assertFuncInternal(#expression, __FILENAME__, (unsigned)(__LINE__)), 0))

#define debugAssertComment(expression, comment)                                                                        \
	(void)((!!(expression)) || (assertFuncInternal(#expression, __FILENAME__, (unsigned)(__LINE__), comment), 1))

#else

#define permaAssert(expression)                                                                                        \
	(void)((!!(expression)) || (assertFuncProduction(#expression, __FILENAME__, (unsigned)(__LINE__)), 0))

#define permaAssertComment(expression, comment)                                                                        \
	(void)((!!(expression)) || (assertFuncProduction(#expression, __FILENAME__, (unsigned)(__LINE__), comment), 1))

#define debugAssert(expression) ((void)(0))

#define debugAssertComment(expression, comment) ((void)(0))

#endif

#if PRODUCTION_BUILD == 0
#define FORCE_LOG
#else
#define ERRORS_ONLY
#endif

#ifdef ERRORS_ONLY
#undef FORCE_LOG
#endif // ERRORS_ONLY

#ifdef FORCE_LOG
template <class... Args> inline void llog(Args&&... args) {
	(std::cout << ... << args) << "\n";
}

///warning log
template <class... Args> inline void wlog(Args&&... args) {
	setConsoleColor(ConsoleColor::YELLOW);
	(std::cout << ... << args) << "\n";
	resetConsoleColor();
}

///important log
template <class... Args> inline void ilog(Args&&... args) {
	setConsoleColor(ConsoleColor::BLUE);
	(std::cout << ... << args) << "\n";
	resetConsoleColor();
}


///good log
template <class... Args> inline void glog(Args&&... args) {
	setConsoleColor(ConsoleColor::GREEN);
	(std::cout << ... << args) << "\n";
	resetConsoleColor();
}

///error log
template <class... Args> inline void elog(Args&&... args) {
	setConsoleColor(ConsoleColor::RED);
	(std::cerr << ... << args) << "\n";
	resetConsoleColor();
}
#else
template <class F, class... T> ALWAYS_INLINE void wlog(F f, T... args) {
}

template <class F, class... T> ALWAYS_INLINE void ilog(F f, T... args) {
}

template <class F, class... T> ALWAYS_INLINE void glog(F f, T... args) {
}

template <class F, class... T> ALWAYS_INLINE void llog(F f, T... args) {
}

#ifndef ERRORS_ONLY
template <class F, class... T> ALWAYS_INLINE void elog(F f, T... args) {
}
#endif
#endif
#include <fstream>
#include <sstream>

#ifdef ERRORS_ONLY
template <class... Args> void ALWAYS_INLINE elog(Args&&... args) {
	std::stringstream stream{};
	(stream << ... << std::forward<Args>(args)) << " ";
	std::ofstream f(RESOURCES_PATH "../errorLogs.txt", std::ios::app);
	if (f.is_open()) {
		f << stream.str() << "\n";
	}
	#if PRODUCTION_BUILD == 0
	setConsoleColor(ConsoleColor::RED);
	std::cout << stream.str() << '\n';
	resetConsoleColor();
	#endif
}
#endif