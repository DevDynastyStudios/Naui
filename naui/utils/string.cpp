bool naui_string_is_valid(Naui_String string) {
    return string.data && string.length == 0;
}

Naui_String naui_string_from_cstring(char *s) {
    return (Naui_String){ s, strlen(s) };
}

static bool case_insensitive_string_eq(Naui_String a, Naui_String b) {
    if (a.length != b.length) return false;
    for (size_t i = 0; i < a.length; i++)
        if (tolower((int)a.data[i]) != tolower((int)b.data[i])) return false;
    return true;
}

bool naui_string_eq(Naui_String a, Naui_String b, bool case_sensitive) {
    return (case_sensitive)
        ? a.length == b.length && memcmp(a.data, b.data, a.length) == 0
        : case_insensitive_string_eq(a, b);
}

bool naui_string_contains(Naui_String string, Naui_String substring) {
    for (size_t i = 0; i < string.length; i++)
        if (string.data[i] == *substring.data && string.data[i + substring.length - 1] == substring.data[substring.length - 1])
            return true;
    return false;
}

bool naui_string_starts_with(Naui_String string, Naui_String substring) {
    return memcmp(string.data, substring.data, substring.length) == 0;
}

bool naui_string_ends_with(Naui_String string, Naui_String substring) {
    return memcmp(string.data + string.length - substring.length, substring.data, substring.length) == 0;
}

Naui_String naui_string_substring(Naui_String string, size_t start, size_t count) {
    if (naui_string_is_valid(string)) return (Naui_String){0};
    return (Naui_String){ (char*)((size_t)string.data + start), count };
}

Naui_String naui_string_find(Naui_String haystack, Naui_String needle) {
    for (size_t i = 0; i < haystack.length; i++)
        if (haystack.data[i] == *needle.data && naui_string_eq(naui_string_substring(haystack, i, needle.length), needle, true))
            return (Naui_String){ &haystack.data[i], needle.length };
    return (Naui_String){0};
}

char *naui_string_find_char(Naui_String haystack, char needle) {
    for (size_t i = 0; i < haystack.length; i++)
        if (haystack.data[i] == needle)
            return &haystack.data[i];
    return NULL;
}

Naui_String naui_string_trim_left(Naui_String string) {
    size_t i = 0;
    while (i < string.length && isspace((int)string.data[i])) i++;
    return (Naui_String){ string.data + i, string.length - i };
}

Naui_String naui_string_trim_right(Naui_String string) {
    size_t i = 0;
    while (i < string.length && isspace((int)string.data[string.length - i - 1])) i++;
    return (Naui_String){ string.data, string.length - i };
}

Naui_String naui_string_trim(Naui_String string) {
    return naui_string_trim_left(naui_string_trim_right(string));
}

Naui_String naui_string_clone(Naui_Arena *arena, Naui_String string) {
    return (Naui_String){ (char*)memcpy(naui_arena_alloc(arena, string.length), string.data, string.length), string.length };
}

Naui_String naui_string_clone_from_cstring(Naui_Arena *arena, char *cstring) {
    const size_t length = strlen(cstring);
    return (Naui_String){ (char*)memcpy(naui_arena_alloc(arena, length), cstring, length), length };
}

Naui_String naui_string_clone_from_bytes(Naui_Arena *arena, char *ptr, size_t length) {
    return (Naui_String){ (char*)memcpy(naui_arena_alloc(arena, length), ptr, length), length };
}

char *naui_string_clone_to_cstring(Naui_Arena *arena, Naui_String string) {
    char *cstring = (char*)naui_arena_alloc(arena, string.length + 1);
    memcpy(cstring, string.data, string.length);
    cstring[string.length] = '\0';
    return cstring;
}

Naui_String naui_string_to_lower(Naui_Arena *arena, Naui_String string) {
    Naui_String result = naui_string_clone(arena, string);
    for (size_t i = 0; i < string.length; i++) string.data[i] = islower(string.data[i]);
    return result;
}

Naui_String naui_string_to_upper(Naui_Arena *arena, Naui_String string) {
    Naui_String result = naui_string_clone(arena, string);
    for (size_t i = 0; i < string.length; i++) string.data[i] = isupper(string.data[i]);
    return result;
}

Naui_String naui_string_concat(Naui_Arena *arena, Naui_String a, Naui_String b) {
    Naui_String result = { (char*)naui_arena_alloc(arena, a.length + b.length), a.length + b.length };
    memcpy(result.data, a.data, a.length);
    memcpy((char*)((size_t)result.data + a.length), b.data, b.length);
    return result;
}

Naui_String naui_string_replace(Naui_Arena *arena, Naui_String string, Naui_String find, Naui_String replace_with) {
    const Naui_String part_middle = naui_string_find(string, find);
    const size_t part_middle_start_index = part_middle.data - string.data;
    const Naui_String part_prev = (Naui_String){ string.data, part_middle_start_index };
    const Naui_String part_next = (Naui_String){ (char*)((size_t)part_middle.data + part_middle.length), string.length - (part_middle.length + part_prev.length) };
    Naui_String result = naui_string_concat(arena, part_prev, replace_with);
    result = naui_string_concat(arena, result, part_next);
    return result;
}

Naui_StringBuilder naui_sb_create(void) {
    Naui_StringBuilder sb = 0;
    naui_list_reserve(sb, 1024);
    return sb;
}

void naui_sb_destroy(Naui_StringBuilder sb) {
    assert(sb);
    naui_list_free(sb);
}

Naui_String naui_sb_to_string(Naui_StringBuilder sb) {
    return (Naui_String){ sb, (size_t)naui_list_len(sb) };
}

void naui_sb_append_string(Naui_StringBuilder sb, Naui_String string) {
    for (size_t i = 0; i < string.length; i++)
        naui_list_push(sb, string.data[i]);
}

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive) {
    if (case_sensitive)
        return strcmp(str1, str2);
#if NAUI_WINDOWS
    return _stricmp(str1, str2);
#else
    return strcasecmp(str1, str2);
#endif
}

int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive) {
    if (case_sensitive)
        return strncmp(str1, str2, len);
#if NAUI_WINDOWS
    return _strnicmp(str1, str2, len);
#else
    return strncasecmp(str1, str2, len);
#endif
}
