#pragma once

#include <stdbool.h>
#include <stdint.h>

int naui_str_to_lower(const char* src, char* dest, size_t dest_size);
int naui_str_to_upper(const char* src, char* dest, size_t dest_size);

bool naui_str_starts_with(const char* s, const char* prefix);
bool naui_str_ends_with(const char* s, const char* suffix);
const char* naui_str_find(const char* s, const char* needle);
const char* naui_str_find_char(const char* s, char c);

bool naui_str_to_int(const char* s, int* out);
bool naui_str_to_uint(const char* s, unsigned int* out);
bool naui_str_to_int64(const char* s, int64_t* out);
bool naui_str_to_uint64(const char* s, uint64_t* out);
bool naui_str_to_float(const char* s, float* out);
bool naui_str_to_double(const char* s, double* out);

int naui_str_trim(const char* src, char* dest, size_t dest_size);
int naui_str_ltrim(const char* src, char* dest, size_t dest_size);
int naui_str_rtrim(const char* src, char* dest, size_t dest_size);

int naui_str_substring(const char* src, size_t start, size_t length, char* dest, size_t dest_size);
int naui_str_replace(const char* src, const char* find, const char* replace, char* dest, size_t dest_size);

bool naui_str_split_once(const char* src, char delimiter, char* left, size_t left_size, char* right, size_t right_size);