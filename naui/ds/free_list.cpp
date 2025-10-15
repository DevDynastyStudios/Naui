#include "free_list.h"
#include <cstdlib>

void naui_create_free_list(NauiFreeList &heap, size_t size)
{
    heap.buffer = (uint8_t*)calloc(1, size);
    heap.capacity = size;
    heap.offset = 0;
    heap.free_list = nullptr;
}

void naui_destroy_free_list(NauiFreeList &heap)
{
    free((void*)heap.buffer);
}

void* naui_free_list_alloc(NauiFreeList &heap, size_t size)
{
    NauiFreeNode** prev = &heap.free_list;
    NauiFreeNode* node = heap.free_list;

    while (node)
    {
        if (node->size >= size)
        {
            *prev = node->next;
            return (void*)node;
        }
        prev = &node->next;
        node = node->next;
    }

    if (heap.offset + size > heap.capacity)
        return nullptr;

    void* ptr = heap.buffer + heap.offset;
    heap.offset += size;
    return ptr;
}

void naui_free_list_free(NauiFreeList &heap, void* ptr, size_t size)
{
    NauiFreeNode* node = (NauiFreeNode*)ptr;
    node->size = size;
    node->next = heap.free_list;
    heap.free_list = node;
}