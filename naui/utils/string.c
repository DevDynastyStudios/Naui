#include "string.h"

#include <string.h>
#include <assert.h>
#include <ctype.h>

bool naui_sv_valid(Naui_StringView s) {
    return (s.len == 0 || !s.data) ? false : true;
}

Naui_StringView naui_sv_from_cstr(char *s) {
    return (Naui_StringView){ .data = s, .len = strlen(s) };
}

char* naui_sv_clone_to_cstr(Naui_Arena *a, Naui_StringView s) {
    char *cstr = naui_arena_alloc(a, s.len + 1);
    memcpy(cstr, s.data, s.len);
    cstr[s.len] = '\0';
    return cstr;
}

Naui_StringView naui_cstr_clone_to_str(Naui_Arena *a, char *s) {
    const size_t len = strlen(s);
    return (Naui_StringView){ .data = naui_arena_alloc(a, len), .len = len };
}

Naui_StringView naui_sv_clone(Naui_Arena *a, Naui_StringView s) {
    return (Naui_StringView){ .data = naui_arena_alloc(a, s.len), .len = s.len };
}

Naui_StringView naui_sv_to_lower_temp(Naui_StringView s) {
    if (!naui_sv_valid(s)) return (Naui_StringView){0};
    Naui_StringView r = naui_sv_clone(naui_arena_frame(), s);
    for (size_t i = 0; i < s.len; i++)
        r.data[i] = (char)tolower((int)s.data[i]);
    return r;
}

Naui_StringView naui_sv_to_upper_temp(Naui_StringView s) {
    if (!naui_sv_valid(s)) return (Naui_StringView){0};
    Naui_StringView r = naui_sv_clone(naui_arena_frame(), s);
    for (size_t i = 0; i < s.len; i++)
        r.data[i] = (char)toupper((int)s.data[i]);
    return r;
}

static inline bool str_cmp_case_sensitive(Naui_StringView str1, Naui_StringView str2) {
    if (str1.data && str2.data && str1.len != str2.len) return false;
    else return memcmp(str1.data, str2.data, str1.len) == 0;
}

static inline bool str_cmp_case_insensitive(Naui_StringView str1, Naui_StringView str2) {
    if (str1.data && str2.data && str1.len != str2.len) return false;
    for (size_t i = 0; i < str1.len; i++)
        if (tolower((int)str1.data[i]) != tolower(str2.data[i])) return false;
    return true;
}

bool naui_sv_cmp(Naui_StringView str1, Naui_StringView str2, bool case_sensitive) {
    return case_sensitive
        ? str_cmp_case_sensitive(str1, str2)
        : str_cmp_case_insensitive(str1, str2);
}

bool naui_sv_starts_with(Naui_StringView s, Naui_StringView prefix) {
    if (!naui_sv_valid(s) || !naui_sv_valid(prefix)) return false;
    if (prefix.len > s.len) return false;
    return memcmp(s.data, prefix.data, prefix.len) == 0;
}

bool naui_sv_ends_with(Naui_StringView s, Naui_StringView suffix) {
    if (!naui_sv_valid(s) || !naui_sv_valid(suffix)) return false;
    if (suffix.len > s.len) return false;
    return memcmp(s.data + s.len - suffix.len, suffix.data, suffix.len) == 0;
}

Naui_StringView naui_sv_find(Naui_StringView s, Naui_StringView needle) {
    if (!naui_sv_valid(s) || !naui_sv_valid(needle)) return (Naui_StringView){0};
    for (; (size_t)s.data < s.len; s.data++) {
        const char *p = s.data, *q = needle.data;
        while (*p && *q && *p == *q) { p++; q++; }
        if (!*q) return (Naui_StringView){ .data = s.data, .len = needle.len };
    }
    return (Naui_StringView){0};
}

Naui_StringView naui_sv_find_char(Naui_StringView s, char c) {
    if (!naui_sv_valid(s)) return (Naui_StringView){0};
    for (size_t i = 0; i < s.len; i++)
        if (s.data[i] == c) return (Naui_StringView){ .data = s.data + i, .len = s.len - i };
    return (Naui_StringView){0};
}

// TODO(doomguy)
//bool naui_sv_to_int(Naui_StringView s, int* out);
//bool naui_sv_to_uint(Naui_StringView s, unsigned int* out);
//bool naui_sv_to_int64(Naui_StringView s, int64_t* out);
bool naui_sv_to_uint64(Naui_StringView s, uint64_t* out) {
    if (!naui_sv_valid(s)) return false;
    uint64_t result = 0;
    for (size_t i = 0; i < s.len && isdigit(s.data[i]); i++) {
        result = result * 10 + (uint64_t)s.data[i] - '0';
    }
    *out = result;
    return true;
}
//bool naui_sv_to_float(Naui_StringView s, float* out);
//bool naui_sv_to_double(Naui_StringView s, double* out);

Naui_StringView naui_sv_trim(Naui_StringView s) {
    return naui_sv_rtrim(naui_sv_ltrim(s));
}

Naui_StringView naui_sv_ltrim(Naui_StringView s) {
    size_t i = 0;
    while (i < s.len && isspace(s.data[i])) i++;
    return (Naui_StringView){ s.data + i, s.len - i };
}

Naui_StringView naui_sv_rtrim(Naui_StringView s) {
    size_t i = 0;
    while (i < s.len && isspace(s.data[s.len - 1 - i])) i++;
    return (Naui_StringView){ s.data, s.len - i };
}

Naui_StringView naui_sv_substring(Naui_StringView src, size_t start, size_t length) {
    if (start > src.len || length > src.len - start) return (Naui_StringView){0};
    size_t sv_start = (size_t)src.data + start;
    return (Naui_StringView){ .data = (char*)sv_start, .len = length };
}

Naui_StringView naui_sv_replace(Naui_StringView src, Naui_StringView find, Naui_StringView replace) {
    // TODO(doomguy)
}

bool naui_sv_split_by_delim(Naui_StringView src, Naui_StringView *s1, Naui_StringView *s2, char delim) {
    if (!naui_sv_valid(src)) return false;

    size_t i = 0;
    while (i < src.len && src.data[i] != delim) i++;

    if (i < src.len) {
        *s1 = (Naui_StringView){ .data = src.data, .len = i };
        *s2 = (Naui_StringView){ .data = src.data + i + 1, .len = src.len - i - 1 };
    } else return false;
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
