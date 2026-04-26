#include "list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CAP_MULT 1.5

static void *grow(void *arr, size_t elem_size)
{
    Naui_ListHeader *header = naui_list_get_header(arr);
    header->cap = (size_t)(header->cap * CAP_MULT);
    header = realloc(header, sizeof(*header) + elem_size * header->cap);
    return header + 1;
}

void *naui_list__init(size_t elem_size, size_t cap)
{
    Naui_ListHeader *header = calloc(1, sizeof(*header) + elem_size * cap);
    header->cap = cap;
    return header + 1;
}

void *naui_list__push(void *arr, size_t elem_size)
{
    if (!arr)
        arr = naui_list__init(elem_size, NAUI_LIST_DEFAULT_CAP);

    Naui_ListHeader *header = naui_list_get_header(arr);
    if (header->len >= header->cap - 1)
        arr = grow(arr, elem_size);

    naui_list_get_header(arr)->len++;
    return arr;
}

void *naui_list__push_at(void *arr, size_t elem_size, size_t index)
{
    if (!arr)
        arr = naui_list__init(elem_size, NAUI_LIST_DEFAULT_CAP);

    Naui_ListHeader *header = naui_list_get_header(arr);
    if (header->len >= header->cap - 1)
        arr = grow(arr, elem_size);

    header = naui_list_get_header(arr);
    assert(index <= header->len);
    memmove((char *)arr + (index + 1) * elem_size,
            (char *)arr + index       * elem_size,
            (header->len - index) * elem_size);
    header->len++;
    return arr;
}

void naui_list__pop(void *arr, size_t elem_size)
{
    Naui_ListHeader *header = naui_list_get_header(arr);
    if (header->len > 0) {
        memset((char *)arr + (header->len - 1) * elem_size, 0, elem_size);
        header->len--;
    }
}

void naui_list__remove(void *arr, size_t elem_size, size_t index)
{
    Naui_ListHeader *header = naui_list_get_header(arr);
    assert(index < header->len);
    memmove((char *)arr + index       * elem_size,
            (char *)arr + (index + 1) * elem_size,
            (header->len - index - 1) * elem_size);
    header->len--;
}

void naui_list__uremove(void *arr, size_t elem_size, size_t index)
{
    Naui_ListHeader *header = naui_list_get_header(arr);
    assert(index < header->len);
    if (index != header->len - 1)
        memmove((char *)arr + index * elem_size,
                (char *)arr + (header->len - 1) * elem_size,
                elem_size);
    header->len--;
}

void *naui_list__reserve(void *arr, size_t elem_size, size_t len)
{
    if (!arr)
        return naui_list__init(elem_size, len);

    Naui_ListHeader *header = naui_list_get_header(arr);
    if (len <= header->cap)
        return arr;

    header->cap = len;
    header = realloc(header, sizeof(*header) + elem_size * len);
    return header + 1;
}