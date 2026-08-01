typedef struct {
    char *data;
    size_t length;
} Naui_String;

typedef Naui_List<char> Naui_StringBuilder;

#define naui_string_lit(cstr) (Naui_String){ (char*)(cstr), sizeof(cstr) - 1 } // Pretty damn convenient
#define naui_string_spread(s) (int)(s).len, (s).data
#define naui_string_fmt "%.*s"
NAUI_API bool naui_string_is_valid(Naui_String string);
NAUI_API Naui_String naui_string_from_cstring(char *s);
NAUI_API bool naui_string_eq(Naui_String a, Naui_String b, bool case_sensitive);
NAUI_API bool naui_string_contains(Naui_String string, Naui_String substring);
NAUI_API bool naui_string_starts_with(Naui_String string, Naui_String substring);
NAUI_API bool naui_string_ends_with(Naui_String string, Naui_String substring);
NAUI_API Naui_String naui_string_substring(Naui_String string, size_t start, size_t count);
NAUI_API Naui_String naui_string_find(Naui_String haystack, Naui_String needle);
NAUI_API char *naui_string_find_char(Naui_String haystack, char needle);
NAUI_API Naui_String naui_string_trim_left(Naui_String string);
NAUI_API Naui_String naui_string_trim_right(Naui_String string);
NAUI_API Naui_String naui_string_trim(Naui_String string);
// String functions that involve allocations
NAUI_API Naui_String naui_string_clone(Naui_Arena *arena, Naui_String string);
NAUI_API Naui_String naui_string_clone_from_cstring(Naui_Arena *arena, char *s);
NAUI_API Naui_String naui_string_clone_from_bytes(Naui_Arena *arena, char *s, size_t len);
NAUI_API char *naui_string_clone_to_cstring(Naui_Arena *arena, Naui_String string);
NAUI_API Naui_String naui_string_to_lower(Naui_Arena *arena, Naui_String string);
NAUI_API Naui_String naui_string_to_upper(Naui_Arena *arena, Naui_String string);
NAUI_API Naui_String naui_string_concat(Naui_Arena *arena, Naui_String a, Naui_String b);
NAUI_API Naui_String naui_string_replace(Naui_Arena *arena, Naui_String string, Naui_String find, Naui_String replace);
/* TODO(doomguy)
NAUI_API naui_string_slice naui_string_split(Naui_Arena *arena, Naui_String string, Naui_String seperator);
NAUI_API naui_string_slice naui_string_split_lines(Naui_Arena *arena, Naui_String string);
NAUI_API naui_string_slice naui_string_split_char(Naui_Arena *arena, Naui_String string, char seperator);
*/

NAUI_API Naui_StringBuilder naui_sb_create(void);
NAUI_API void naui_sb_destroy(Naui_StringBuilder sb);
NAUI_API Naui_String naui_sb_to_string(Naui_StringBuilder sb);
NAUI_API void naui_sb_append_string(Naui_StringBuilder sb, Naui_String string);
// TODO(doomguy): use variadic arguments for sb append functions

// functions needed by iterator_win32 and iterator_unix
int naui_cstr_strcmp(const char *str1, const char *str2, bool case_sensitive);
int naui_cstr_strncmp(const char *str1, const char *str2, size_t len, bool case_sensitive);
