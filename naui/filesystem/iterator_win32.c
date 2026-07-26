#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#	define UNICODE
#endif
#ifndef _UNICODE
#	define _UNICODE
#endif

#include "iterator.h"
#include "utils/string.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define NAUI_WIN32_NAME_MAX MAX_PATH

typedef struct
{
	HANDLE find_handle;
	WIN32_FIND_DATAW find_data;
	bool first;
} Naui_DirIteratorInternal;

static Naui_DirIteratorInternal* iterator_internal(Naui_DirIterator* it)
{
	return (Naui_DirIteratorInternal*)(void*)it->_handle;
}

static bool to_wide(const char* src, wchar_t* dst, int dst_count)
{
	return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_count) > 0;
}

static bool to_utf8(const wchar_t* src, char* dst, int dst_count)
{
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dst_count, NULL, NULL) > 0;
}

static bool iterator_match_filter(const char* name, const char* filter, bool case_sensitive)
{
	if (!filter || filter[0] == '\0')
		return true;

	size_t filter_len = strlen(filter);
	return naui_cstr_strncmp(name, filter, filter_len, case_sensitive) == 0;
}

static bool iterator_match_extensions(const char* name, const char** exts, bool case_sensitive)
{
	if (!exts)
		return true;

	const char* dot = strrchr(name, '.');
	if (!dot)
		return false;

	size_t index = 0;
	while (exts[index] != NULL)
	{
		if (exts[index] && naui_cstr_strcmp(dot, exts[index], case_sensitive) == 0)
			return true;

		index++;
	}

	return false;
}

/* Ensures _path_buf can hold at least `needed` bytes (including NUL).
 * Only ever grows - never shrunk or reallocated away, so it amortizes
 * to zero allocations once the largest entry name has been seen once.
 * Returns false on allocation failure (existing buffer/entry untouched). */
static bool iterator_reserve_path_buf(Naui_DirIterator* it, size_t needed)
{
	if (needed <= it->_path_buf_cap)
		return true;

	size_t new_cap = it->_path_buf_cap ? it->_path_buf_cap * 2 : 256;
	if (new_cap < needed)
		new_cap = needed;

	char* grown = (char*)realloc(it->_path_buf, new_cap);
	if (!grown)
		return false;

	it->_path_buf = grown;
	it->_path_buf_cap = new_cap;
	return true;
}

static bool iterator_fill_entry(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	/* Scratch only - cFileName is capped at MAX_PATH wchars by Win32 itself,
	 * so a UTF-8 buffer of up to 4x that is always enough (worst case 4
	 * bytes/codepoint), independent of NAUI_PATH_MAX. */
	char name_u8[NAUI_WIN32_NAME_MAX * 4];
	if (!to_utf8(internal->find_data.cFileName, name_u8, sizeof(name_u8)))
		return false;

	if (strcmp(name_u8, ".") == 0 || strcmp(name_u8, "..") == 0)
		return false;

	const char* filter = it->_filter ? it->_filter : NULL;
	if (!iterator_match_filter(name_u8, filter, it->case_sensitive))
		return false;

	if (!iterator_match_extensions(name_u8, it->_extensions, it->case_sensitive))
		return false;

	size_t root_len = strlen(it->_root);
	size_t name_len = strlen(name_u8);
	/* root + '\\' + name + '\0' */
	size_t child_len = root_len + 1 + name_len + 1;
	if (!iterator_reserve_path_buf(it, child_len))
		return false;

	snprintf(it->_path_buf, child_len, "%s\\%s", it->_root, name_u8);

	/* Non-owning view into _path_buf - valid until the next advance
	 * or close(), per the iterator.h contract. */
	it->entry.path.data = it->_path_buf;
	it->entry.path.length = child_len - 1;
	it->entry.is_directory = (internal->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	ULARGE_INTEGER sz;
	sz.HighPart = internal->find_data.nFileSizeHigh;
	sz.LowPart = internal->find_data.nFileSizeLow;
	it->entry.size = (size_t)sz.QuadPart;
	return true;
}

static void iterator_advance(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	it->is_valid = false;
	it->entry.path = naui_path_empty();

	if (internal->find_handle == INVALID_HANDLE_VALUE)
		return;

	if (internal->first)
	{
		internal->first = false;
		if (iterator_fill_entry(it))
		{
			it->is_valid = true;
			return;
		}
	}

	while (FindNextFileW(internal->find_handle, &internal->find_data))
	{
		if (iterator_fill_entry(it))
		{
			it->is_valid = true;
			return;
		}
	}
}

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive)
{
	Naui_DirIterator it;
	memset(&it, 0, sizeof(it));
	it.entry.path = naui_path_empty();
	Naui_DirIteratorInternal* internal = iterator_internal(&it);
	internal->find_handle = INVALID_HANDLE_VALUE;
	internal->first = true;
	size_t path_len = strlen(path.data);
	size_t wpath_count = path_len + 1;
	size_t wsearch_count = path_len + 3;
	wchar_t* wpath = (wchar_t*)malloc(wpath_count * sizeof(wchar_t));
	wchar_t* wsearch = (wchar_t*)malloc(wsearch_count * sizeof(wchar_t));
	if (!wpath || !wsearch)
	{
		free(wpath);
		free(wsearch);
		return it;
	}

	if (!to_wide(path.data, wpath, (int)wpath_count))
	{
		free(wpath);
		free(wsearch);
		return it;
	}

	_snwprintf(wsearch, wsearch_count, L"%s\\*", wpath);
	free(wpath);

	internal->find_handle = FindFirstFileW(wsearch, &internal->find_data);
	free(wsearch);

	if (internal->find_handle == INVALID_HANDLE_VALUE)
		return it;

	it._root = (char*)malloc(path_len + 1);
	if (!it._root)
	{
		FindClose(internal->find_handle);
		internal->find_handle = INVALID_HANDLE_VALUE;
		return it;
	}
	memcpy(it._root, path.data, path_len + 1);

	if (filter && filter[0])
	{
		size_t filter_len = strlen(filter);
		it._filter = (char*)malloc(filter_len + 1);
		if (it._filter)
			memcpy(it._filter, filter, filter_len + 1);
	}

	it._extensions = extensions;
	it.case_sensitive = case_sensitive;
	iterator_advance(&it);
	return it;
}

void naui_dir_iterator_next(Naui_DirIterator* it)
{
	iterator_advance(it);
}

bool naui_dir_iterator_valid(const Naui_DirIterator* it)
{
	return it->is_valid;
}

void naui_dir_iterator_close(Naui_DirIterator* it)
{
	Naui_DirIteratorInternal* internal = iterator_internal(it);
	if (internal->find_handle != INVALID_HANDLE_VALUE)
	{
		FindClose(internal->find_handle);
		internal->find_handle = INVALID_HANDLE_VALUE;
	}

	it->entry.path = naui_path_empty();

	free(it->_root);
	it->_root = NULL;

	free(it->_filter);
	it->_filter = NULL;

	free(it->_path_buf);
	it->_path_buf = NULL;
	it->_path_buf_cap = 0;
	it->is_valid = false;
}

#endif