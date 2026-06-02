#include "arena.h"

#include <stdlib.h>

void naui_create_arena(Naui_Arena *arena, size_t size)
{
    arena->ptr = (uint8_t*)malloc(size);
    arena->size = size;
    arena->pos = 0;
}

void naui_destroy_arena(Naui_Arena *arena)
{
    free(arena->ptr);
}

void naui_arena_reset(Naui_Arena *arena)
{
    arena->pos = 0;
}

void *naui_arena_alloc(Naui_Arena *arena, size_t size)
{
    if (arena->pos + size > arena->size)
        return NULL;

    void *p = arena->ptr + arena->pos;
    arena->pos += size;
    return p;
}