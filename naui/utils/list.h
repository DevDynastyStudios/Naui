static constexpr size_t naui_list_default_capacity = 64;

template <class T>
struct Naui_List {
    T *items;
    size_t length, capacity;

    T &operator[](size_t index) { assert(index < length); return items[index]; }
};
