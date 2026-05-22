#include "string.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

int naui_str_to_lower(const char* src, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	size_t i = 0;
	for (; src[i] && i < dest_size - 1; ++i)
	{
		dest[i] = (char)tolower((unsigned char)src[i]);
	}

	dest[i] = '\0';
	return (int)i;
}

int naui_str_to_upper(const char* src, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	size_t i = 0;
	for (; src[i] && i < dest_size - 1; ++i)
	{
		dest[i] = (char)toupper((unsigned char)src[i]);
	}

	dest[i] = '\0';
	return (int)i;
}

bool naui_str_starts_with(const char* s, const char* prefix)
{
	if (!s || !prefix)
		return false;

	while (*prefix)
	{
		if (*s++ != *prefix++)
			return false;
	}

	return true;
}

bool naui_str_ends_with(const char* s, const char* suffix)
{
	if (!s || !suffix)
		return false;

	size_t s_len = strlen(s);
	size_t suffix_len = strlen(suffix);
	if (suffix_len > s_len)
		return false;

	return memcmp(s + s_len - suffix_len, suffix, suffix_len) == 0;
}

const char* naui_str_find(const char* s, const char* needle)
{
	if (!s || !needle)
		return NULL;

	return strstr(s, needle);
}

const char* naui_str_find_char(const char* s, char c)
{
	if (!s)
		return NULL;

	return strchr(s, c);
}

bool naui_str_to_int(const char* s, int* out)
{
	if (!s || !out || s[0] == '\0')
		return false;

	char* end;
	long v = strtol(s, &end, 10);
	if (end == s || *end != '\0')
		return false;

	if (v < INT_MIN || v > INT_MAX)
		return false;

	*out = (int)v;
	return true;
}

bool naui_str_to_uint(const char* s, unsigned int* out)
{
	if (!s || !out || s[0] == '\0' || s[0] == '-')
		return false;

	char* end;
	unsigned long v = strtoul(s, &end, 10);
	if (end == s || *end != '\0')
		return false;

	if (v > UINT_MAX)
		return false;

	*out = (unsigned int)v;
	return true;
}

bool naui_str_to_int64(const char* s, int64_t* out)
{
	if (!s || !out || s[0] == '\0')
		return false;

	char* end;
	int64_t v = (int64_t)strtoll(s, &end, 10);

	if (end == s || *end != '\0')
		return false;

	*out = v;
	return true;
}

bool naui_str_to_uint64(const char* s, uint64_t* out)
{
	if (!s || !out || s[0] == '\0' || s[0] == '-')
		return false;

	char* end;
	uint64_t v = (uint64_t)strtoull(s, &end, 10);
	if (end == s || *end != '\0')
		return false;

	*out = v;
	return true;
}

bool naui_str_to_float(const char* s, float* out)
{
	if (!s || !out || s[0] == '\0')
		return false;

	char* end;
	float v = strtof(s, &end);
	if (end == s || *end != '\0')
		return false;

	*out = v;
	return true;
}

bool naui_str_to_double(const char* s, double* out)
{
	if (!s || !out || s[0] == '\0')
		return false;

	char* end;
	double v = strtod(s, &end);
	if (end == s || *end != '\0')
		return false;

	*out = v;
	return true;
}

int naui_str_ltrim(const char* src, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	while (*src && isspace((unsigned char)*src))
	{
		++src;
	}

	size_t i = 0;
	for (; src[i] && i < dest_size - 1; ++i)
	{
		dest[i] = src[i];
	}

	dest[i] = '\0';
	return (int)i;
}

int naui_str_rtrim(const char* src, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	size_t len = strlen(src);
	while (len > 0 && isspace((unsigned char)src[len - 1]))
	{
		--len;
	}

	if (len >= dest_size)
		len = dest_size - 1;

	memcpy(dest, src, len);
	dest[len] = '\0';
	return (int)len;
}

int naui_str_trim(const char* src, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	while (*src && isspace((unsigned char)*src))
	{
		++src;
	}

	size_t len = strlen(src);
	while (len > 0 && isspace((unsigned char)src[len - 1]))
	{
		--len;
	}

	if (len >= dest_size)
		len = dest_size - 1;

	memcpy(dest, src, len);
	dest[len] = '\0';
	return (int)len;
}

int naui_str_substring(const char* src, size_t start, size_t length, char* dest, size_t dest_size)
{
	if (!src || !dest || dest_size == 0)
		return -1;

	size_t src_len = strlen(src);
	if (start >= src_len)
	{
		dest[0] = '\0';
		return 0;
	}

	size_t available = src_len - start;
	size_t copy_len = length < available ? length : available;
	if (copy_len >= dest_size)
		copy_len = dest_size - 1;

	memcpy(dest, src + start, copy_len);
	dest[copy_len] = '\0';
	return (int)copy_len;
}

int naui_str_replace(const char* src, const char* find, const char* replace, char* dest, size_t dest_size)
{
	if (!src || !find || !replace || !dest || dest_size == 0)
		return -1;

	size_t find_len = strlen(find);
	size_t replace_len = strlen(replace);
	size_t written = 0;

	if (find_len == 0)
	{
		size_t src_len = strlen(src);
		size_t copy = src_len < dest_size - 1 ? src_len : dest_size - 1;
		memcpy(dest, src, copy);
		dest[copy] = '\0';
		return (int)copy;
	}

	const char* cursor = src;
	while (*cursor && written < dest_size - 1)
	{
		const char* match = strstr(cursor, find);
		if (!match)
		{
			while (*cursor && written < dest_size - 1)
			{
				dest[written++] = *cursor++;
			}

			break;
		}

		while (cursor < match && written < dest_size - 1)
		{
			dest[written++] = *cursor++;
		}

		for (size_t i = 0; i < replace_len && written < dest_size - 1; ++i)
		{
			dest[written++] = replace[i];
		}

		cursor += find_len;
	}

	dest[written] = '\0';
	return (int)written;
}

bool naui_str_split_once(const char* src, char delimiter, char* left, size_t left_size, char* right, size_t right_size)
{
	if (!src || !left || left_size == 0 || !right || right_size == 0)
		return false;

	const char* sep = strchr(src, delimiter);
	if (!sep)
	{
		size_t len = strlen(src);
		size_t copy = len < left_size - 1 ? len : left_size - 1;
		memcpy(left, src, copy);
		left[copy] = '\0';
		right[0] = '\0';
		return false;
	}

	size_t left_len = (size_t)(sep - src);
	size_t right_len = strlen(sep + 1);
	size_t left_copy = left_len < left_size - 1 ? left_len : left_size - 1;
	size_t right_copy = right_len < right_size - 1 ? right_len : right_size - 1;

	memcpy(left, src, left_copy);
	memcpy(right, sep + 1, right_copy);
	left[left_copy] = '\0';
	right[right_copy] = '\0';
	return true;
}