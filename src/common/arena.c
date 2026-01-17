#include "arena.h"

#include <stdlib.h>
#include <string.h>

#define ARENA_MIN_CHUNK 4096

void arena_init(Arena *arena) {
    arena->head = NULL;
}

void arena_free(Arena *arena) {
    ArenaChunk *chunk = arena->head;
    while (chunk) {
        ArenaChunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    arena->head = NULL;
}

static ArenaChunk *arena_new_chunk(size_t size) {
    size_t cap = size < ARENA_MIN_CHUNK ? ARENA_MIN_CHUNK : size;
    ArenaChunk *chunk = (ArenaChunk *)malloc(sizeof(ArenaChunk) + cap);
    if (!chunk) {
        return NULL;
    }
    chunk->next = NULL;
    chunk->used = 0;
    chunk->cap = cap;
    return chunk;
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned = (size + 7u) & ~7u;
    if (!arena->head || arena->head->used + aligned > arena->head->cap) {
        ArenaChunk *chunk = arena_new_chunk(aligned);
        if (!chunk) {
            return NULL;
        }
        chunk->next = arena->head;
        arena->head = chunk;
    }
    void *ptr = arena->head->data + arena->head->used;
    arena->head->used += aligned;
    memset(ptr, 0, size);
    return ptr;
}
