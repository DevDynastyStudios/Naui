#pragma once

#include "base.h"

#include <cstdint>
#include <cstddef>

struct NauiFreeNode
{
    size_t size;
    NauiFreeNode* next;
};

struct NauiFreeList
{
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
    NauiFreeNode* free_list;
};

NAUI_API void naui_create_free_list(NauiFreeList &heap, size_t size);
NAUI_API void naui_destroy_free_list(NauiFreeList &heap);
NAUI_API void* naui_free_list_alloc(NauiFreeList &heap, size_t size);
NAUI_API void naui_free_list_free(NauiFreeList &heap, void* ptr, size_t size);