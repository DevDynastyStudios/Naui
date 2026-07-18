// TODO(doomguy): implement sving builders
// TODO(doomguy): make allocators so you don't have to rely on arenas

#pragma once

#include "base.h"
#include "utils/arena.h"

// this is convenient for normal C sving literals
#define NAUI_STR(cstr) (Naui_StringView){ .data = (cstr), .len = sizeof((cstr)) - 1 }

#define NAUI_STR_FMT "%.*s"
#define NAUI_STR_EXPAND(s) (s).len, (s).data

typedef struct {
    char *data;
    size_t len;
} Naui_StringView;

typedef char* Naui_StringBuilder; // dynamic array

bool naui_sv_valid(Naui_StringView s);

Naui_StringView naui_sv_from_cstr(char *s);
Naui_StringView naui_sv_clone(Naui_Arena *a, Naui_StringView s);
char* naui_sv_clone_to_cstr(Naui_Arena *a, Naui_StringView s);
Naui_StringView naui_cstr_clone_to_str(Naui_Arena *a, char *s);

Naui_StringView naui_sv_to_lower_temp(Naui_StringView s);
Naui_StringView naui_sv_to_upper_temp(Naui_StringView s);

bool naui_sv_cmp(Naui_StringView str1, Naui_StringView str2, bool case_sensitive);

bool naui_sv_starts_with(Naui_StringView s, Naui_StringView prefix);
bool naui_sv_ends_with(Naui_StringView s, Naui_StringView suffix);
Naui_StringView naui_sv_find(Naui_StringView s, Naui_StringView needle);
Naui_StringView naui_sv_find_char(Naui_StringView s, char c);

bool naui_sv_to_int(Naui_StringView s, int* out);
bool naui_sv_to_uint(Naui_StringView s, unsigned int* out);
bool naui_sv_to_int64(Naui_StringView s, int64_t* out);
bool naui_sv_to_uint64(Naui_StringView s, uint64_t* out);
bool naui_sv_to_float(Naui_StringView s, float* out);
bool naui_sv_to_double(Naui_StringView s, double* out);

Naui_StringView naui_sv_trim(Naui_StringView s);
Naui_StringView naui_sv_ltrim(Naui_StringView s);
Naui_StringView naui_sv_rtrim(Naui_StringView s);

Naui_StringView naui_sv_substring(Naui_StringView src, size_t start, size_t length);
Naui_StringView naui_sv_replace(Naui_StringView src, Naui_StringView find, Naui_StringView replace);

bool naui_sv_split_by_delim(Naui_StringView src, Naui_StringView *s1, Naui_StringView *s2, char delim);

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive);
int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive);
