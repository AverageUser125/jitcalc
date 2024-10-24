#ifndef TOOLS_H_INCLUDE
#define TOOLS_H_INCLUDE


#include "defines.hpp"
#include <iosfwd>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string.h>

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

#else

#define permaAssert(expression)                                                                                        \
	(void)((!!(expression)) || (assertFuncProduction(#expression, __FILENAME__, (unsigned)(__LINE__)), 0))

#define permaAssertComment(expression, comment)                                                                        \
	(void)((!!(expression)) || (assertFuncProduction(#expression, __FILENAME__, (unsigned)(__LINE__), comment), 1))
#endif


#if PRODUCTION_BUILD == 0
#define FORCE_LOG
#endif

#ifdef ERRORS_ONLY
#undef FORCE_LOG
#endif // ERRORS_ONLY

#ifdef FORCE_LOG
inline void llog() {
	std::cout << "\n";
}

template <class F, class... T> inline void llog(F f, T... args) {
	std::cout << f << " ";
	llog(args...);
}
#else
template <class F, class... T> inline void llog(F f, T... args) {
}
#endif

///warning log
#ifdef FORCE_LOG
inline void wlog() {
	std::cout << "\n";
	resetConsoleColor();
}

template <class F, class... T> inline void wlog(F f, T... args) {
	setConsoleColor(ConsoleColor::YELLOW);
	std::cout << f << " ";
	wlog(args...);
}
#else
template <class F, class... T> inline void wlog(F f, T... args) {
}
#endif

///important log
#ifdef FORCE_LOG
inline void ilog() {
	std::cout << "\n";
	resetConsoleColor();
}

template <class F, class... T> inline void ilog(F f, T... args) {
	setConsoleColor(ConsoleColor::BLUE);

	std::cout << f << " ";
	ilog(args...);
}
#else
template <class F, class... T> inline void ilog(F f, T... args) {
}
#endif

///good log
#ifdef FORCE_LOG
inline void glog() {
	std::cout << "\n";
	resetConsoleColor();
}

template <class F, class... T> inline void glog(F f, T... args) {
	setConsoleColor(ConsoleColor::GREEN);

	std::cout << f << " ";
	glog(args...);
}
#else
template <class F, class... T> inline void glog(F f, T... args) {
}
#endif

///error log
#ifdef FORCE_LOG
inline void elog() {
	std::cout << "\n";
	resetConsoleColor();
}

template <class F, class... T> inline void elog(F f, T... args) {
	setConsoleColor(ConsoleColor::RED);

	std::cout << f << " ";
	elog(args...);
}
#else

#ifdef ERRORS_ONLY

inline void elog(std::stringstream&& stream) {
#if PLATFORM_WIN
	MessageBoxA(0, stream.str().c_str(), "error", MB_ICONERROR);
#endif
}

template <class F, class... T> inline void elog(std::stringstream&& stream, F&& f, T&&... args) {
	stream << std::forward<F>(f) << " ";

	elog(std::move(stream), args...);
}

template <class F, class... T> inline void elog(F&& f, T&&... args) {
	std::stringstream stream;

	stream << std::forward<F>(f) << " ";

	elog(std::move(stream), args...);
}


#else
template <class F, class... T> inline void elog(F f, T... args) {
	// TODO: fix this
	/**std::stringstream stream;

	stream << std::forward<F>(f) << " ";

	elog(std::move(stream), args...);
	*/
}

template <class F, class... T> inline void elog(std::stringstream&& stream, F&& f, T&&... args) {
	stream << std::forward<F>(f) << " ";

	elog(std::move(stream), args...);
}

#include <fstream>

inline void elog(std::stringstream&& stream) {
	std::ofstream f(RESOURCES_PATH "../errorLogs.txt", std::ios::app);

	f << stream.str() << "\n";

	f.close();
}

#endif

#endif


#endif