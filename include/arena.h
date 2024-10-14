// Copyright 2022 Alexey Kutepov <reximkut@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef ARENA_H_
#define ARENA_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef ARENA_NOSTDIO
#include <stdarg.h>
#include <stdio.h>
#endif // ARENA_NOSTDIO

#ifndef ARENA_ASSERT
#if PRODUCTION_BUILD == 0
#undef NDEBUG
#endif
#include <assert.h>
#define ARENA_ASSERT assert
#endif

#define ARENA_BACKEND_LIBC_MALLOC 0
#define ARENA_BACKEND_LINUX_MMAP 1
#define ARENA_BACKEND_WIN32_VIRTUALALLOC 2
#define ARENA_BACKEND_WASM_HEAPBASE 3

#include "defines.hpp"

#ifndef ARENA_BACKEND
#if PLATFORM_WIN
#define ARENA_BACKEND ARENA_BACKEND_WIN32_VIRTUALALLOC
#elif PLATFORM_LINUX
#define ARENA_BACKEND ARENA_BACKEND_LINUX_MMAP
#endif
#endif // ARENA_BACKEND

struct Region {
	Region* next;
	size_t count;
	size_t capacity;
	uintptr_t data[];
};

struct Arena;
void arena_init(Arena* a);

struct Arena {
	Region *begin, *end;

	Arena() {
		arena_init(this);
	}
};

#define REGION_DEFAULT_CAPACITY (8 * 1024)

Region* new_region(size_t capacity);
void free_region(Region* r);

// TODO: snapshot/rewind capability for the arena
// - Snapshot should be combination of a->end and a->end->count.
// - Rewinding should be restoring a->end and a->end->count from the snapshot and
// setting count-s of all the Region-s after the remembered a->end to 0.
void* arena_alloc(Arena* a, size_t size_bytes);
void* arena_realloc(Arena* a, void* oldptr, size_t oldsz, size_t newsz);
char* arena_strdup(Arena* a, const char* cstr);
void* arena_memdup(Arena* a, void* data, size_t size);
#ifndef ARENA_NOSTDIO
char* arena_sprintf(Arena* a, const char* format, ...);
#endif // ARENA_NOSTDIO

void arena_reset(Arena* a);
void arena_free(Arena* a);

#define ARENA_DA_INIT_CAP 256

#ifdef __cplusplus
#define cast_ptr(ptr) (decltype(ptr))
#else
#define cast_ptr(...)
#endif

#define arena_da_append(a, da, item)                                                                                   \
	do {                                                                                                               \
		if ((da)->count >= (da)->capacity) {                                                                           \
			size_t new_capacity = (da)->capacity == 0 ? ARENA_DA_INIT_CAP : (da)->capacity * 2;                        \
			(da)->items = cast_ptr((da)->items) arena_realloc((a), (da)->items, (da)->capacity * sizeof(*(da)->items), \
															  new_capacity * sizeof(*(da)->items));                    \
			(da)->capacity = new_capacity;                                                                             \
		}                                                                                                              \
                                                                                                                       \
		(da)->items[(da)->count++] = (item);                                                                           \
	} while (0)

#endif // ARENA_H_
