template <class T>
void naui_list__alloc_if_null(Naui_List<T> *list) {
    if (!list->items) {
        list->items = (T*)calloc(1, sizeof(T) * naui_list_default_capacity);
        assert(list->items);
        list->capacity = naui_list_default_capacity;
    }
}

template <class T>
void naui_list_push(Naui_List<T> *list, T value) {
    naui_list__alloc_if_null(list);
    naui_list_reserve(list, list->length + 1);
    list->items[list->length++] = value;
}

template <class T>
void naui_list_push_at(Naui_List<T> *list, T value, size_t index) {
    assert(index >= 0 && index < list->length);
    naui_list__alloc_if_null(list);
    naui_list_reserve(list, list->length + 1);
    memmove(&list->items[index + 1], &list->items[index], (list->length - index) * sizeof(T));
    list->items[index] = value;
    list->length++;
}

template <class T>
T naui_list_pop(Naui_List<T> *list) {
    assert(list->length > 0);
    return list->items[--list->length];
}

template <class T>
void naui_list_remove(Naui_List<T> *list, size_t index) {
    assert(index >= 0 && index < list->length);
    memmove(&list->items[index], &list->items[index + 1], (list->length - index) * sizeof(T));
    list->length--;
}

template <class T>
void naui_list_uremove(Naui_List<T> *list, size_t index) {
    assert(index >= 0 && index < list->length);
    if (index != list->length - 1)
        memmove(&list->items[index], &list->items[list->length - 1], sizeof(T));
    list->length--;
}

template <class T>
void naui_list_reserve(Naui_List<T> *list, size_t capacity) {
    if (list->capacity < capacity)
        return;
    list->capacity = list->capacity;
    list->items = (T*)realloc(list->items, sizeof(T) * list->capacity);
}

template <class T>
void naui_list_clear(Naui_List<T> *list) {
    list->length = 0;
}

template <class T>
void naui_list_free(Naui_List<T> *list) {
    assert(list->items);
    free(list->items);
    list->items = NULL;
}
