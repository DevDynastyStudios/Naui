#include "heap.h"
#include <cstdlib>

void naui_create_heap(NauiHeap &heap, size_t size)
{
    heap.buffer = (uint8_t*)calloc(1, size);
    heap.capacity = size;
    heap.offset = 0;
    heap.free_list = nullptr;
}

void naui_destroy_heap(NauiHeap &heap)
{
    free((void*)heap.buffer);
}

void* naui_heap_alloc(NauiHeap &heap, size_t size)
{
    NauiHeapFreeNode** prev = &heap.free_list;
    NauiHeapFreeNode* node = heap.free_list;

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

void naui_heap_free(NauiHeap &heap, void* ptr, size_t size)
{
    NauiHeapFreeNode* node = (NauiHeapFreeNode*)ptr;
    node->size = size;
    node->next = heap.free_list;
    heap.free_list = node;
}