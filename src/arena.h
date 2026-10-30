#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct Arena {
  uint8_t* buffer = nullptr;
  size_t capacity = 0;
  size_t offset = 0;
};

inline Arena arena_create(size_t capacity) {
  Arena arena = {};
  arena.buffer = static_cast<uint8_t*>(malloc(capacity));
  if (!arena.buffer) {
    abort();
  }
  arena.capacity = capacity;
  arena.offset = 0;
  return arena;
}

inline void arena_destroy(Arena* arena) {
  if (arena && arena->buffer) {
    free(arena->buffer);
    arena->buffer = nullptr;
    arena->capacity = 0;
    arena->offset = 0;
  }
}

inline void arena_reset(Arena* arena) {
  if (arena) {
    arena->offset = 0;
  }
}

inline void* arena_alloc(Arena* arena, size_t bytes, size_t alignment = alignof(max_align_t)) {
  if (!arena || !arena->buffer) {
    return nullptr;
  }

  uintptr_t current = reinterpret_cast<uintptr_t>(arena->buffer + arena->offset);
  uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
  size_t new_offset = (aligned - reinterpret_cast<uintptr_t>(arena->buffer)) + bytes;

  if (new_offset > arena->capacity) {
    return nullptr;
  }

  arena->offset = new_offset;
  return reinterpret_cast<void*>(aligned);
}

template <typename T>
inline T* arena_alloc_array(Arena* arena, size_t count) {
  return static_cast<T*>(arena_alloc(arena, sizeof(T) * count, alignof(T)));
}
