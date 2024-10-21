#pragma once
#include <memory>
#include "arena.h"

extern Arena global_arena;

template <typename T> class ArenaAllocator {
  public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	// Default constructor using the global static Arena
	ArenaAllocator() noexcept {};

	~ArenaAllocator() noexcept {
	}

	// Rebind allocator to another type
	template <typename U> struct rebind {
		using other = ArenaAllocator<U>;
	};

	[[nodiscard]] T* allocate(size_type n) {
		void* ptr = arena_alloc(&global_arena, n * sizeof(T));
		if (!ptr) {
			throw std::bad_alloc();
		}
		return static_cast<T*>(ptr);
	}

	void deallocate(T* p, size_type n) noexcept {
		// No-op, memory is managed by the global arena
	}

	// Equality comparison for allocators (always true as the arena is global)
	bool operator==(const ArenaAllocator& other) const noexcept {
		return true;
	}

	bool operator!=(const ArenaAllocator& other) const noexcept {
		return !(*this == other);
	}
};