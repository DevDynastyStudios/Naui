#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *ptr;
    size_t pos;
    size_t size;
}
Naui_Arena;

void naui_create_arena(Naui_Arena *arena, size_t size);
void naui_destroy_arena(Naui_Arena *arena);
void naui_arena_reset(Naui_Arena *arena);
void *naui_arena_alloc(Naui_Arena *arena, size_t size);