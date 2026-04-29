#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "filesystem.h"

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int utf8_to_wide(const char* src, wchar_t* dst, int dst_len)
{
	return MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_len);
}

static int wide_to_utf8(const wchar_t* src, char* dst, int dst_len)
{
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dst_len, NULL, NULL);
}

typedef struct
{
	HANDLE h;
	NauiFileMode mode;
} NauiFileInternal;

//_Static_assert(sizeof(NauiFileInternal) <= NAUI_FILE_HANDLE_SIZE, "NAUI_FILE_HANDLE_SIZE too small for NauiFileInternal on Win32");

static NauiFileInternal* file_internal(const NauiFileHandle* handle)
{
	return (NauiFileInternal*)(void*)handle->_opaque;
}

#define NAUI_LOCK_MAX 256

static struct { char path[MAX_PATH]; } s_locks[NAUI_LOCK_MAX];
static int s_lock_count = 0;

void naui_filesystem_free(void* ptr)
{
	free(ptr);
}

bool naui_file_open(NauiFileHandle* handle, const char* path, NauiFileMode mode)
{
	if (!handle || !path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	DWORD access, creation;
	switch (mode)
	{
		case NAUI_FILE_READ:
			access = GENERIC_READ;
			creation = OPEN_EXISTING;
			break;
		case NAUI_FILE_WRITE:
			access = GENERIC_WRITE;
			creation = CREATE_ALWAYS;
			break;
		case NAUI_FILE_APPEND:
			access = FILE_APPEND_DATA;
			creation = OPEN_ALWAYS;
			break;
		default:
			return false;
	}

	HANDLE h = CreateFileW(
		wpath, access, FILE_SHARE_READ, NULL,
		creation, FILE_ATTRIBUTE_NORMAL, NULL
	);

	if (h == INVALID_HANDLE_VALUE)
		return false;

	NauiFileInternal* fi = file_internal(handle);
	fi->h = h;
	fi->mode = mode;

	if (mode == NAUI_FILE_APPEND)
		SetFilePointer(h, 0, NULL, FILE_END);

	return true;
}

size_t naui_file_read(const NauiFileHandle* handle, void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->h || fi->h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD read = 0;
	ReadFile(fi->h, buffer, (DWORD)size, &read, NULL);
	return (size_t)read;
}

size_t naui_file_write(const NauiFileHandle* handle, const void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->h || fi->h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD written = 0;
	WriteFile(fi->h, buffer, (DWORD)size, &written, NULL);
	return (size_t)written;
}

size_t naui_file_size(const char* path)
{
	if (!path)
		return 0;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return 0;

	WIN32_FILE_ATTRIBUTE_DATA info;
	if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &info))
		return 0;

	ULARGE_INTEGER size;
	size.HighPart = info.nFileSizeHigh;
	size.LowPart = info.nFileSizeLow;
	return (size_t)size.QuadPart;
}

char* naui_file_read_all(const char* path, size_t* out_size)
{
	if (!path)
		return NULL;

	NauiFileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_READ))
		return NULL;

	NauiFileInternal* fi = file_internal(&handle);
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(fi->h, &file_size))
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t sz = (size_t)file_size.QuadPart;
	char* buf = (char*)malloc(sz + 1);
	if (!buf)
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t total = 0;
	DWORD chunk;
	while (total < sz)
	{
		DWORD to_read = (DWORD)((sz - total) > 0xFFFFFFFFu ? 0xFFFFFFFFu : (sz - total));
		if (!ReadFile(fi->h, buf + total, to_read, &chunk, NULL) || chunk == 0)
			break;

		total += chunk;
	}

	naui_file_close(&handle);
	buf[total] = '\0';
	if (out_size)
		*out_size = total;

	return buf;
}

bool naui_file_write_all(const char* path, const void* data, size_t size)
{
	if (!path || !data)
		return false;

	NauiFileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_WRITE))
		return false;

	size_t written = naui_file_write(&handle, data, size);
	naui_file_close(&handle);
	return written == size;
}

bool naui_file_is_valid(const NauiFileHandle* handle)
{
	if (!handle)
		return false;

	NauiFileInternal* fi = file_internal(handle);
	return fi->h != NULL && fi->h != INVALID_HANDLE_VALUE;
}

bool naui_file_delete(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	return DeleteFileW(wpath) != 0;
}

bool naui_file_rename(const char* old_path, const char* new_path)
{
	if (!old_path || !new_path)
		return false;

	wchar_t wold[MAX_PATH];
	wchar_t wnew[MAX_PATH];
	if (!utf8_to_wide(old_path, wold, MAX_PATH))
		return false;

	if (!utf8_to_wide(new_path, wnew, MAX_PATH))
		return false;

	return MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING) != 0;
}

bool naui_file_seek(NauiFileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->h || fi->h == INVALID_HANDLE_VALUE)
		return false;

	DWORD method;
	switch (origin)
	{
		case SEEK_SET: method = FILE_BEGIN; break;
		case SEEK_CUR: method = FILE_CURRENT; break;
		case SEEK_END: method = FILE_END; break;
		default: return false;
	}

	DWORD result = SetFilePointer(fi->h, offset, NULL, method);
	return result != INVALID_SET_FILE_POINTER;
}

void naui_file_close(NauiFileHandle* handle)
{
	if (!handle)
		return;

	NauiFileInternal* fi = file_internal(handle);
	if (fi->h && fi->h != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fi->h);
		fi->h = NULL;
	}
}

const char* naui_path_filename(const char* path)
{
	if (!path)
		return NULL;

	const char* last_fwd = strrchr(path, '/');
	const char* last_back = strrchr(path, '\\');
	const char* last = (last_fwd > last_back) ? last_fwd : last_back;
	return last ? last + 1 : path;
}

char* naui_path_get_extension(const char* path)
{
	if (!path)
		return NULL;

	const char* filename = naui_path_filename(path);
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return NULL;

	return _strdup(dot);
}

char* naui_path_parent(const char* path)
{
	if (!path)
		return NULL;

	size_t len = strlen(path);
	if (len == 0)
		return _strdup(".");

	while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'))
	{
		--len;
	}

	const char* last = NULL;
	for (size_t i = len; i > 0; --i)
	{
		if (path[i - 1] == '/' || path[i - 1] == '\\')
		{
			last = path + i - 1;
			break;
		}
	}

	if (!last)
		return _strdup(".");

	if(last == path)
		return _strdup("/");

	size_t parent_len = (size_t)(last - path);
	if (parent_len == 2 && path[1] == ':') parent_len = 3;

	char* result = (char*)malloc(parent_len + 1);
	if (!result)
		return NULL;

	memcpy(result, path, parent_len);
	result[parent_len] = '\0';
	return result;
}

bool naui_path_exists(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	return GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES;
}

bool naui_directory_create(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	return CreateDirectoryW(wpath, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool naui_directory_remove(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	return RemoveDirectoryW(wpath) != 0;
}

static bool remove_all_recursive_w(wchar_t* wpath)
{
	wchar_t search[MAX_PATH];
	swprintf(search, MAX_PATH, L"%s\\*", wpath);

	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(search, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return RemoveDirectoryW(wpath) != 0;

	bool ok = true;
	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		wchar_t child[MAX_PATH];
		swprintf(child, MAX_PATH, L"%s\\%s", wpath, fd.cFileName);

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ok &= remove_all_recursive_w(child);
		else
			ok &= (DeleteFileW(child) != 0);

	} while (FindNextFileW(h, &fd));

	FindClose(h);
	ok &= (RemoveDirectoryW(wpath) != 0);
	return ok;
}

bool naui_directory_remove_all(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	return remove_all_recursive_w(wpath);
}

bool naui_directory_rename(const char* old_path, const char* new_path)
{
	return naui_file_rename(old_path, new_path);
}

const char* naui_directory_get(NauiDir directory)
{
	static char s_home[MAX_PATH] = {0};
	static char s_bin[MAX_PATH] = {0};
	static char s_working[MAX_PATH] = {0};
	static char s_appdata[MAX_PATH] = {0};
	static char s_downloads[MAX_PATH] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (s_home[0])
				return s_home;

			wchar_t wbuf[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, wbuf)))
				wide_to_utf8(wbuf, s_home, MAX_PATH);

			return s_home;
		}
		case NAUI_DIR_BIN:
		{
			if (s_bin[0])
				return s_bin;

			wchar_t wbuf[MAX_PATH];
			if (GetModuleFileNameW(NULL, wbuf, MAX_PATH))
			{
				wchar_t* last = wcsrchr(wbuf, L'\\');
				if (last)
					*last = L'\0';

				wide_to_utf8(wbuf, s_bin, MAX_PATH);
			}

			return s_bin;
		}
		case NAUI_DIR_WORKING:
		{
			if (s_working[0])
				return s_working;

			wchar_t wbuf[MAX_PATH];
			if (GetCurrentDirectoryW(MAX_PATH, wbuf))
				wide_to_utf8(wbuf, s_working, MAX_PATH);

			return s_working;
		}
		case NAUI_DIR_APPDATA:
		{
			if (s_appdata[0])
				return s_appdata;

			wchar_t wbuf[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wbuf)))
				wide_to_utf8(wbuf, s_appdata, MAX_PATH);

			return s_appdata;
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (s_downloads[0])
				return s_downloads;

			const char* home = naui_directory_get(NAUI_DIR_HOME);
			if (home[0])
				snprintf(s_downloads, sizeof(s_downloads), "%s\\Downloads", home);

			return s_downloads;
		}
	}

	return NULL;
}

static bool match_filter(const char* name, const char* filter)
{
	if (!filter || filter[0] == '\0')
		return true;

	const char* star = strchr(filter, '*');
	if (!star)
		return strcmp(name, filter) == 0;

	size_t prefix_len = (size_t)(star - filter);
	if (_strnicmp(name, filter, prefix_len) != 0)
		return false;

	name += prefix_len;
	filter = star + 1;

	size_t suffix_len = strlen(filter);
	size_t name_len = strlen(name);
	if (suffix_len > name_len)
		return false;

	return _stricmp(name + name_len - suffix_len, filter) == 0;
}

static bool match_extensions(const char* name, const char** extensions, int ext_count)
{
	if (ext_count <= 0 || !extensions)
		return true;

	const char* dot = strrchr(name, '.');
	if (!dot)
		return false;

	for (int i = 0; i < ext_count; ++i)
	{
		if (extensions[i] && _stricmp(dot, extensions[i]) == 0)
			return true;
	}

	return false;
}

Naui_List(NauiDirEntry) naui_directory_filter(const char* path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(NauiDirEntry) list = NULL;
	naui_list_reserve(list, 1);

	if (!path)
		return list;

	wchar_t wsearch[MAX_PATH];
	{
		wchar_t wpath[MAX_PATH];
		if (!utf8_to_wide(path, wpath, MAX_PATH))
			return list;

		swprintf(wsearch, MAX_PATH, L"%s\\*", wpath);
	}

	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wsearch, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return list;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		char name_u8[MAX_PATH];
		wide_to_utf8(fd.cFileName, name_u8, MAX_PATH);

		if (!match_filter(name_u8, filter))
			continue;

		if (!match_extensions(name_u8, extensions, ext_count))
			continue;

		char full[MAX_PATH];
		snprintf(full, sizeof(full), "%s\\%s", path, name_u8);
		bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		ULARGE_INTEGER sz;
		sz.HighPart = fd.nFileSizeHigh;
		sz.LowPart = fd.nFileSizeLow;

		NauiDirEntry de;
		de.path = _strdup(full);
		de.is_directory = is_dir;
		de.size = (size_t)sz.QuadPart;

		naui_list_push(list, de);

	} while (FindNextFileW(h, &fd));

	FindClose(h);
	return list;
}

void naui_directory_filter_free(Naui_List(NauiDirEntry) list)
{
	if (!list)
		return;

	size_t len = naui_list_len(list);
	for (size_t i = 0; i < len; ++i)
	{
		free(list[i].path);
	}

	naui_list_free(list);
}

bool naui_path_lock(const char* path)
{
	if (!path || s_lock_count >= NAUI_LOCK_MAX)
		return false;

	if (naui_is_locked(path))
		return false;

	snprintf(s_locks[s_lock_count].path, sizeof(s_locks[s_lock_count].path), "%s", path);
	++s_lock_count;
	return true;
}

bool naui_is_locked(const char* path)
{
	if (!path)
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, path) == 0)
			return true;
	}

	return false;
}

void naui_path_unlock(const char* path)
{
	if (!path)
		return;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, path) == 0)
		{
			s_locks[i] = s_locks[--s_lock_count];
			return;
		}
	}
}

bool naui_hide_file(const char* path, bool is_hidden)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	DWORD attrs = GetFileAttributes(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (is_hidden)
		attrs |= FILE_ATTRIBUTE_HIDDEN;
	else
		attrs &= ~FILE_ATTRIBUTE_HIDDEN;

	return SetFileAttributesW(wpath, attrs) != 0;
}

bool naui_is_hidden(const char* path)
{
	if (!path)
		return false;

	wchar_t wpath[MAX_PATH];
	if (!utf8_to_wide(path, wpath, MAX_PATH))
		return false;

	DWORD attrs = GetFileAttributes(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;
		
	return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
}

#endif