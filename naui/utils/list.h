#pragma once

#include <stddef.h>

typedef struct
{
    size_t len, cap;
}
Naui_ListHeader;

#define NAUI_LIST_DEFAULT_CAP 64

void *naui_list__init(size_t elem_size, size_t cap);
void *naui_list__push(void *arr, size_t elem_size);
void *naui_list__push_at(void *arr, size_t elem_size, size_t index);
void  naui_list__pop(void *arr, size_t elem_size);
void  naui_list__remove(void *arr, size_t elem_size, size_t index);
void  naui_list__uremove(void *arr, size_t elem_size, size_t index);
void *naui_list__reserve(void *arr, size_t elem_size, size_t len);
void  naui_list__free(void *arr);

#define Naui_List(type) type *

#define naui_list_get_header(arr) ((Naui_ListHeader *)(arr) - 1)
#define naui_list_len(arr)        (naui_list_get_header(arr)->len)

#define naui_list_push(arr, data) do { \
    arr = naui_list__push(arr, sizeof(*arr)); \
    arr[naui_list_get_header(arr)->len - 1] = (data); \
} while (0)

#define naui_list_push_at(arr, data, index) do { \
    arr = naui_list__push_at(arr, sizeof(*arr), (index)); \
    arr[(index)] = (data); \
} while (0)

#define naui_list_pop(arr)              naui_list__pop(arr, sizeof(*arr))
#define naui_list_remove(arr, index)    naui_list__remove(arr, sizeof(*arr), (index))
#define naui_list_uremove(arr, index)   naui_list__uremove(arr, sizeof(*arr), (index))

#define naui_list_reserve(arr, len) do { \
    arr = naui_list__reserve(arr, sizeof(*arr), (len)); \
} while (0)

#define naui_list_free(arr) naui_list__free(arr)