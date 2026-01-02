/*
 * Arena Allocator - Fast, leak-free memory management
 * 
 * Usage:
 *   Arena* arena = arena_new();
 *   void* ptr = arena_alloc(arena, size);
 *   // ... use memory ...
 *   arena_free(arena);  // Free everything at once
 */

#ifndef WYN_ARENA_H
#define WYN_ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

// Create new arena
Arena* arena_new(void);

// Allocate from arena (never fails, exits on OOM)
void* arena_alloc(Arena* arena, size_t size);

// Allocate and zero memory
void* arena_calloc(Arena* arena, size_t count, size_t size);

// Free entire arena
void arena_free(Arena* arena);

// Get total allocated bytes
size_t arena_total_allocated(Arena* arena);

#endif // WYN_ARENA_H
