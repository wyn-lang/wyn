/*
 * Arena Allocator Implementation
 * 
 * Simple bump allocator with 64KB blocks
 * Fast allocation, no individual frees, bulk deallocation
 */

#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ARENA_BLOCK_SIZE (64 * 1024)  // 64KB blocks

typedef struct ArenaBlock {
    struct ArenaBlock* next;
    size_t used;
    size_t size;
    char data[];
} ArenaBlock;

struct Arena {
    ArenaBlock* current;
    ArenaBlock* first;
    size_t total_allocated;
};

Arena* arena_new(void) {
    Arena* arena = malloc(sizeof(Arena));
    if (!arena) {
        fprintf(stderr, "Fatal: Out of memory\n");
        exit(1);
    }
    
    arena->current = NULL;
    arena->first = NULL;
    arena->total_allocated = 0;
    
    return arena;
}

static ArenaBlock* arena_block_new(size_t min_size) {
    size_t block_size = min_size > ARENA_BLOCK_SIZE ? min_size : ARENA_BLOCK_SIZE;
    ArenaBlock* block = malloc(sizeof(ArenaBlock) + block_size);
    if (!block) {
        fprintf(stderr, "Fatal: Out of memory\n");
        exit(1);
    }
    
    block->next = NULL;
    block->used = 0;
    block->size = block_size;
    
    return block;
}

void* arena_alloc(Arena* arena, size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    // Need new block?
    if (!arena->current || arena->current->used + size > arena->current->size) {
        ArenaBlock* new_block = arena_block_new(size);
        
        if (!arena->first) {
            arena->first = new_block;
        }
        
        if (arena->current) {
            arena->current->next = new_block;
        }
        
        arena->current = new_block;
    }
    
    void* ptr = arena->current->data + arena->current->used;
    arena->current->used += size;
    arena->total_allocated += size;
    
    return ptr;
}

void* arena_calloc(Arena* arena, size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = arena_alloc(arena, total);
    memset(ptr, 0, total);
    return ptr;
}

void arena_free(Arena* arena) {
    ArenaBlock* block = arena->first;
    while (block) {
        ArenaBlock* next = block->next;
        free(block);
        block = next;
    }
    free(arena);
}

size_t arena_total_allocated(Arena* arena) {
    return arena->total_allocated;
}
