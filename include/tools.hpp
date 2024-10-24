#ifndef TOOLS_H_INCLUDE
#define TOOLS_H_INCLUDE


#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "defines.hpp"

#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)

#if PLATFORM_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

inline void assertFuncProduction(const char* expression, const char* file_name, unsigned const line_number,
								 const char* comment = "---") {

	char c[1024] = {};

	sprintf(c,
			"Assertion failed\n\n"
			"File:\n"
			"%s\n\n"
			"Line:\n"
			"%u\n\n"
			"Expresion:\n"
			"%s\n\n"
			"Comment:\n"
			"%s"
			"\n\nPlease report this error to the developer.",
			file_name, line_number, expression, comment);

	int const action = MessageBoxA(0, c, "Platform Layer", MB_TASKMODAL | MB_ICONHAND | MB_OK | MB_SETFOREGROUND);

	switch (action) {
	case IDOK: // Abort the program:
	{
		raise(SIGABRT);

		// We won't usually get here, but it's possible that a user-registered
		// abort handler returns, so exit the program immediately.  Note that
		// even though we are "aborting," we do not call abort() because we do
		// not want to invoke Watson (the user has already had an opportunity
		// to debug the error and chose not to).
		_exit(3);
	}
	default: {
		_exit(3);
	}
	}
}

inline void assertFuncInternal(const char* expression, const char* file_name, unsigned const line_number,
							   const char* comment = "---") {

	char c[1024] = {};

	sprintf(c,
			"Assertion failed\n\n"
			"File:\n"
			"%s\n\n"
			"Line:\n"
			"%u\n\n"
			"Expresion:\n"
			"%s\n\n"
			"Comment:\n"
			"%s"
			"\n\nPress retry to debug.",
			file_name, line_number, expression, comment);

	int const action =
		MessageBoxA(0, c, "Platform Layer", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

	switch (action) {
	case IDABORT: // Abort the program:
	{
		raise(SIGABRT);

		// We won't usually get here, but it's possible that a user-registered
		// abort handler returns, so exit the program immediately.  Note that
		// even though we are "aborting," we do not call abort() because we do
		// not want to invoke Watson (the user has already had an opportunity
		// to debug the error and chose not to).
		_exit(3);
	}
	case IDRETRY: // Break into the debugger then return control to caller
	{
		__debugbreak();
		return;
	}
	case IDIGNORE: // Return control to caller
	{
		return;
	}
	default: // This should not happen; treat as fatal error:
	{
		abort();
	}
	}
}

#else //linux or others

inline void assertFuncProduction(const char* expression, const char* file_name, unsigned const line_number,
								 const char* comment = "---") {

	raise(SIGABRT);
}

inline void assertFuncInternal(const char* expression, const char* file_name, unsigned const line_number,
							   const char* comment = "---") {

	raise(SIGABRT);
}

#endif

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
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
#endif
}

template <class F, class... T> inline void wlog(F f, T... args) {
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
#endif
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
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
#endif
}

template <class F, class... T> inline void ilog(F f, T... args) {
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
#endif

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
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
#endif
}

template <class F, class... T> inline void glog(F f, T... args) {
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
#endif

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
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
#endif
}

template <class F, class... T> inline void elog(F f, T... args) {
#if PLATFORM_WIN
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
#endif

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
	std::stringstream stream;

	stream << std::forward<F>(f) << " ";

	elog(std::move(stream), args...);
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