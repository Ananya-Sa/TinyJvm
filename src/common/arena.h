#ifndef TINYJVM_ARENA_H
#define TINYJVM_ARENA_H

#include <stddef.h>

typedef struct ArenaChunk {
    struct ArenaChunk *next;
    size_t used;
    size_t cap;
    unsigned char data[1];
} ArenaChunk;

typedef struct {
    ArenaChunk *head;
} Arena;

void arena_init(Arena *arena);
void arena_free(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);

#endif
