#pragma once
#include "arena.h"

template <typename T> class ArenaAllocator {
  public:
	using value_type = T;

	// Default constructor using the global static Arena
	ArenaAllocator() noexcept {
		snapshot = arena_snapshot(&global_arena());
	};

	~ArenaAllocator() noexcept {
		arena_rewind(&global_arena(), snapshot);
	}
	// Rebind allocator to another type
	template <typename U> struct rebind {
		using other = ArenaAllocator<U>;
	};

	// Allocate memory for n objects of type T using the static Arena
	[[nodiscard]] T* allocate(std::size_t n) {
		void* ptr = arena_alloc(&global_arena(), n * sizeof(T));
		if (!ptr) {
			throw std::bad_alloc();
		}
		return static_cast<T*>(ptr);
	}

	// Deallocate memory (no-op for this allocator)
	void deallocate(T* p, std::size_t n) noexcept {
		// No-op, memory is managed by the global arena
	}

	// Equality comparison for allocators (always true as the arena is global)
	bool operator==(const ArenaAllocator& other) const noexcept {
		return true; // All ArenaAllocator instances use the same global arena
	}

	bool operator!=(const ArenaAllocator& other) const noexcept {
		return !(*this == other);
	}

	static ArenaSnapshot getSnapshot() {
		return arena_snapshot(&global_arena());
	}

	static void restoreSnapshot(const ArenaSnapshot snap) {
		arena_rewind(&global_arena(), snap);
	}

	static void reset() {
		arena_reset(&global_arena());
	}

  private:
	ArenaSnapshot snapshot;
	// Static function to access the global arena
	static Arena& global_arena() {
		static Arena arena;
		static bool initialized = false;

		if (!initialized) {
			arena_init(&arena);
			initialized = true;
		}

		return arena;
	}
};