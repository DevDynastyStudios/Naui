#pragma once

#include "base.h"
#include <cstddef>

struct NauiArena
{
    unsigned char *buffer;
    size_t size;
    size_t pos;
};

NAUI_API void naui_create_arena     (NauiArena &arena, size_t size);
NAUI_API void naui_destroy_arena    (NauiArena &arena);

NAUI_API void* naui_arena_alloc     (NauiArena &arena, size_t size);
NAUI_API void naui_arena_reset      (NauiArena &arena);