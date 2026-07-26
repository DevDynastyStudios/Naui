#if !defined(_WIN32) && !defined(_WIN64)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "iterator.h"
#include "utils/string.h"

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>

typedef struct
{
	DIR* dir;
} Naui_DirIterInternal;

static Naui_DirIterInternal* iterator_internal(Naui_DirIterator* it)
{
	return (Naui_DirIterInternal*)(void*)it->_handle;
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

static void iterator_advance(Naui_DirIterator* it)
{
	Naui_DirIterInternal* internal = iterator_internal(it);
	it->is_valid = false;
	it->entry.path = naui_path_empty();

	struct dirent* entry;
	while ((entry = readdir(internal->dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		const char* filter = it->_filter ? it->_filter : NULL;
		if (!iterator_match_filter(entry->d_name, filter, it->case_sensitive))
			continue;

		if (!iterator_match_extensions(entry->d_name, it->_extensions, it->case_sensitive))
			continue;

		size_t root_len = strlen(it->_root);
		size_t name_len = strlen(entry->d_name);
		size_t child_len = root_len + 1 + name_len + 1;
		if (!iterator_reserve_path_buf(it, child_len))
			continue;

		snprintf(it->_path_buf, child_len, "%s/%s", it->_root, entry->d_name);
		struct stat st;
		if (stat(it->_path_buf, &st) != 0)
			continue;

		it->entry.path.data = it->_path_buf;
		it->entry.path.length = child_len - 1;
		it->entry.is_directory = S_ISDIR(st.st_mode);
		it->entry.size = (size_t)st.st_size;
		it->is_valid = true;
		return;
	}
}

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive)
{
	Naui_DirIterator it;
	memset(&it, 0, sizeof(it));
	it.entry.path = naui_path_empty();

	Naui_DirIterInternal* internal = iterator_internal(&it);
	internal->dir = opendir(path.data);
	if (!internal->dir)
		return it;

	size_t root_len = strlen(path.data);
	it._root = (char*)malloc(root_len + 1);
	if (!it._root)
	{
		closedir(internal->dir);
		internal->dir = NULL;
		return it;
	}
	memcpy(it._root, path.data, root_len + 1);

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
	Naui_DirIterInternal* internal = iterator_internal(it);
	if (internal->dir)
	{
		closedir(internal->dir);
		internal->dir = NULL;
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