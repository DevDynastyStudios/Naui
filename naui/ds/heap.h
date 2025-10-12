#pragma once

#include <base.h>

#include <cstdint>
#include <cstddef>

struct NauiHeapFreeNode
{
    size_t size;
    NauiHeapFreeNode* next;
};

struct NauiHeap
{
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
    NauiHeapFreeNode* free_list;
};

NAUI_API void naui_create_heap(NauiHeap &heap, size_t size);
NAUI_API void naui_destroy_heap(NauiHeap &heap);
NAUI_API void* naui_heap_alloc(NauiHeap &heap, size_t size);
NAUI_API void naui_heap_free(NauiHeap &heap, void* ptr, size_t size);