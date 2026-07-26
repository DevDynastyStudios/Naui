#define _CRT_SECURE_NO_WARNINGS
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

#include "filesystem.h"

#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct
{
	HANDLE h;
} Naui_FileInternal;

#define NAUI_LOCK_MAX 32
typedef struct
{
	char* path;
	HANDLE handle;
} Naui_LockEntry;

static Naui_LockEntry s_locks[NAUI_LOCK_MAX];
static int s_lock_count = 0;

#define NAUI_WIN_SHELL_PATH_MAX 512
#define NAUI_PATH_MAX_PARTS 4096

static NAUI_THREAD_LOCAL char s_path_scratch[NAUI_PATH_MAX];
static NAUI_THREAD_LOCAL char s_extended_scratch[2][NAUI_PATH_MAX];
static NAUI_THREAD_LOCAL wchar_t s_wide_scratch[2][NAUI_PATH_MAX];

static NAUI_THREAD_LOCAL const char* s_normalize_parts[NAUI_PATH_MAX_PARTS];
static NAUI_THREAD_LOCAL size_t s_normalize_part_lens[NAUI_PATH_MAX_PARTS];

static Naui_Path path_alloc(const char* data, size_t length)
{
	if (!data || length == 0)
		return naui_path_empty();

	char* buf = (char*)malloc(length + 1);
	if (!buf)
		return naui_path_empty();

	memcpy(buf, data, length);
	buf[length] = '\0';

	Naui_Path result;
	result.data = buf;
	result.length = length;
	return result;
}

static Naui_Path path_view(const char* s)
{
	if (!s || s[0] == '\0')
		return naui_path_empty();

	Naui_Path p;
	p.data = s;
	p.length = strlen(s);
	return p;
}

static Naui_Path path_view_len(const char* data, size_t length)
{
	if (!data || length == 0)
		return naui_path_empty();

	Naui_Path p;
	p.data = data;
	p.length = length;
	return p;
}

void naui_path_free_(int count, ...)
{
	va_list args;
	va_start(args, count);
	for (int i = 0; i < count; ++i)
	{
		Naui_Path p = va_arg(args, Naui_Path);
		free((void*)p.data);
	}
	va_end(args);
}

static bool to_utf8_into(const wchar_t* src, char* dst, int dst_capacity)
{
	return WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dst_capacity, NULL, NULL) > 0;
}

static bool is_separator(char c)
{
	return c == '/' || c == '\\';
}

static Naui_FileInternal* file_internal(const Naui_FileHandle* handle)
{
	return (Naui_FileInternal*)(void*)handle->_opaque;
}

static bool match_filter(const char* name, const char* filter)
{
	if (!filter || filter[0] == '\0')
		return true;

	const char* star = strchr(filter, '*');
	if (!star)
		return _stricmp(name, filter) == 0;

	size_t prefix_len = (size_t)(star - filter);
	if (_strnicmp(name, filter, prefix_len) != 0)
		return false;

	const char* suffix = star + 1;
	size_t suffix_len = strlen(suffix);
	size_t name_len = strlen(name);
	if (suffix_len > name_len - prefix_len)
		return false;

	return _stricmp(name + name_len - suffix_len, suffix) == 0;
}

static bool match_extensions(const char* name, const char** exts, int ext_count)
{
	if (ext_count <= 0 || !exts)
		return true;

	const char* dot = strrchr(name, '.');
	if (!dot)
		return false;

	for (int i = 0; i < ext_count; ++i)
	{
		if (exts[i] && _stricmp(dot, exts[i]) == 0)
			return true;
	}

	return false;
}

static const wchar_t* prepare_os_path_slot(const Naui_Path path, int slot)
{
	if (!path.data)
		return NULL;

	wchar_t* wide = s_wide_scratch[slot];
	char* ext = s_extended_scratch[slot];
	int n = MultiByteToWideChar(CP_UTF8, 0, path.data, -1, wide, NAUI_PATH_MAX);
	if (n > 0 && n < MAX_PATH - 12)
		return wide;

	Naui_Path abs = naui_path_absolute(path);
	Naui_Path norm = naui_path_normalize(abs);
	NAUI_PATH_FREE(abs);
	if (norm.length == 0)
	{
		NAUI_PATH_FREE(norm);
		return NULL;
	}

	bool is_unc = norm.length > 1 && norm.data[0] == '\\' && norm.data[1] == '\\';
	int written = is_unc ? snprintf(ext, NAUI_PATH_MAX, "\\\\?\\UNC\\%s", norm.data + 2) : snprintf(ext, NAUI_PATH_MAX, "\\\\?\\%s", norm.data);
	NAUI_PATH_FREE(norm);

	if (written < 0 || (size_t)written >= NAUI_PATH_MAX)
		return NULL;

	int wn = MultiByteToWideChar(CP_UTF8, 0, ext, -1, wide, NAUI_PATH_MAX);
	return wn > 0 ? wide : NULL;
}

static const wchar_t* prepare_os_path(const Naui_Path path)
{
	return prepare_os_path_slot(path, 0);
}

static void filter_recursive_impl_w(const char* path, const char* filter, const char** extensions, int ext_count, Naui_List(Naui_DirEntry)* list)
{
	const wchar_t* wprepared = prepare_os_path(naui_path_from_cstr(path));
	if (!wprepared)
		return;

	size_t wprepared_len = wcslen(wprepared);
	wchar_t* wsearch = (wchar_t*)malloc((wprepared_len + 3) * sizeof(wchar_t));
	if (!wsearch)
		return;

	_snwprintf(wsearch, wprepared_len + 3, L"%s\\*", wprepared);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wsearch, &fd);
	free(wsearch);
	if (h == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		char name_u8[NAUI_WIN_SHELL_PATH_MAX];
		if (!to_utf8_into(fd.cFileName, name_u8, NAUI_WIN_SHELL_PATH_MAX))
			continue;

		int written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s\\%s", path, name_u8);
		if (written < 0)
			continue;

		size_t child_len = (size_t)written < NAUI_PATH_MAX ? (size_t)written : NAUI_PATH_MAX - 1;
		bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (is_dir)
		{
			Naui_DirEntry de;
			de.path = path_alloc(s_path_scratch, child_len);
			de.is_directory = true;
			de.size = 0;
			naui_list_push(*list, de);
			filter_recursive_impl_w(de.path.data, filter, extensions, ext_count, list);
		}
		else
		{
			if (!match_filter(name_u8, filter))
				continue;

			if (!match_extensions(name_u8, extensions, ext_count))
				continue;

			ULARGE_INTEGER size;
			size.HighPart = fd.nFileSizeHigh;
			size.LowPart = fd.nFileSizeLow;

			Naui_DirEntry de;
			de.path = path_alloc(s_path_scratch, child_len);
			de.is_directory = false;
			de.size = (size_t)size.QuadPart;
			naui_list_push(*list, de);
		}

	} while (FindNextFileW(h, &fd));

	FindClose(h);
}

static bool resolve_lock_target(const Naui_Path path, char* out, size_t out_capacity)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	DWORD attrs = GetFileAttributesW(wpath);
	int written;
	if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
		written = snprintf(out, out_capacity, "%s\\.lock", path.data);
	else
		written = snprintf(out, out_capacity, "%s", path.data);

	return written > 0 && (size_t)written < out_capacity;
}

bool naui_file_open(Naui_FileHandle* handle, const Naui_Path path, Naui_FileMode mode)
{
	if (!handle)
		return false;

	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
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

	HANDLE h = CreateFileW(wpath, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	if (mode == NAUI_FILE_APPEND)
		SetFilePointer(h, 0, NULL, FILE_END);

	file_internal(handle)->h = h;
	return true;
}

size_t naui_file_read(const Naui_FileHandle* handle, void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD got = 0;
	ReadFile(h, buffer, (DWORD)size, &got, NULL);
	return (size_t)got;
}

size_t naui_file_write(const Naui_FileHandle* handle, const void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD written = 0;
	WriteFile(h, buffer, (DWORD)size, &written, NULL);
	return (size_t)written;
}

bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	HANDLE h = file_internal(handle)->h;
	if (!h || h == INVALID_HANDLE_VALUE)
		return false;

	DWORD method;
	switch (origin)
	{
		case SEEK_SET:
			method = FILE_BEGIN;
			break;

		case SEEK_CUR:
			method = FILE_CURRENT;
			break;

		case SEEK_END:
			method = FILE_END;
			break;

		default:
			return false;
	}

	return SetFilePointer(h, offset, NULL, method) != INVALID_SET_FILE_POINTER;
}

bool naui_file_is_valid(const Naui_FileHandle* handle)
{
	if (!handle)
		return false;

	HANDLE h = file_internal(handle)->h;
	return h && h != INVALID_HANDLE_VALUE;
}

void naui_file_close(Naui_FileHandle* handle)
{
	if (!handle)
		return;

	Naui_FileInternal* fi = file_internal(handle);
	if (fi->h && fi->h != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fi->h);
		fi->h = NULL;
	}
}

size_t naui_file_size(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return 0;

	WIN32_FILE_ATTRIBUTE_DATA info;
	if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &info))
		return 0;

	ULARGE_INTEGER size;
	size.HighPart = info.nFileSizeHigh;
	size.LowPart = info.nFileSizeLow;
	return (size_t)size.QuadPart;
}

char* naui_file_read_all(const Naui_Path path, size_t* out_size)
{
	Naui_FileHandle handle = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&handle, path, NAUI_FILE_READ))
		return NULL;

	HANDLE h = file_internal(&handle)->h;
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(h, &file_size))
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t size = (size_t)file_size.QuadPart;
	char* buf = (char*)malloc(size + 1);
	if (!buf)
	{
		naui_file_close(&handle);
		return NULL;
	}

	size_t total = 0;
	DWORD chunk;
	while (total < size)
	{
		DWORD to_read = (DWORD)((size - total) > 0xFFFFFFFFu ? 0xFFFFFFFFu : size - total);
		if (!ReadFile(h, buf + total, to_read, &chunk, NULL) || chunk == 0)
			break;

		total += chunk;
	}

	naui_file_close(&handle);
	buf[total] = '\0';
	if (out_size)
		*out_size = total;

	return buf;
}

bool naui_file_write_all(const Naui_Path path, const void* data, size_t size)
{
	if (!data)
		return false;

	Naui_FileHandle h = NAUI_FILE_HANDLE_INIT;
	if (!naui_file_open(&h, path, NAUI_FILE_WRITE))
		return false;

	size_t written = naui_file_write(&h, data, size);
	naui_file_close(&h);
	return written == size;
}

bool naui_file_delete(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	return DeleteFileW(wpath) != 0;
}

bool naui_file_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	const wchar_t* wold = prepare_os_path_slot(old_path, 0);
	const wchar_t* wnew = prepare_os_path_slot(new_path, 1);
	if (!wold || !wnew)
		return false;

	return MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING) != 0;
}

Naui_Path naui_file_hide(const Naui_Path path, bool hidden)
{
	Naui_Path result = path_view_len(path.data, path.length);
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return result;

	DWORD attrs = GetFileAttributesW(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return result;

	attrs = hidden ? attrs | FILE_ATTRIBUTE_HIDDEN : attrs & ~FILE_ATTRIBUTE_HIDDEN;
	SetFileAttributesW(wpath, attrs);
	return result;
}

bool naui_file_is_hidden(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	DWORD attrs = GetFileAttributesW(wpath);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
}

Naui_StringView naui_file_filename(const Naui_Path path)
{
	if (!path.data)
		return (Naui_StringView){ NULL, 0 };

	const char* last = NULL;
	for (const char* p = path.data; *p; ++p)
	{
		if (is_separator(*p))
			last = p;
	}

	const char* start = last ? last + 1 : path.data;
	size_t len = path.length - (size_t)(start - path.data);
	return (Naui_StringView){ start, len };
}

Naui_StringView naui_file_stem(const Naui_Path path)
{
	Naui_StringView filename = naui_file_filename(path);
	if (!filename.data)
		return (Naui_StringView){ NULL, 0 };

	const char* dot = strrchr(filename.data, '.');
	if (!dot || dot == filename.data)
		return (Naui_StringView){ filename.data, filename.len };

	size_t len = (size_t)(dot - filename.data);
	return (Naui_StringView){ filename.data, len };
}

Naui_StringView naui_file_extension(const Naui_Path path)
{
	Naui_StringView filename = naui_file_filename(path);
	if (!filename.data)
		return (Naui_StringView){ NULL, 0 };

	const char* dot = strrchr(filename.data, '.');
	if (!dot || dot == filename.data)
		return (Naui_StringView){ NULL, 0 };

	size_t len = filename.len - (size_t)(dot - filename.data);
	return (Naui_StringView){ dot, len };
}

bool naui_directory_create(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	return CreateDirectoryW(wpath, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool naui_directory_remove(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	return RemoveDirectoryW(wpath) != 0;
}

static bool remove_all_recursive(const wchar_t* wpath)
{
	size_t wpath_len = wcslen(wpath);
	wchar_t* search = (wchar_t*)malloc((wpath_len + 3) * sizeof(wchar_t));
	if (!search)
		return false;

	_snwprintf(search, wpath_len + 3, L"%s\\*", wpath);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(search, &fd);
	free(search);

	if (h == INVALID_HANDLE_VALUE)
		return RemoveDirectoryW(wpath) != 0;

	bool ok = true;
	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		size_t name_len = wcslen(fd.cFileName);
		wchar_t* child = (wchar_t*)malloc((wpath_len + 1 + name_len + 1) * sizeof(wchar_t));
		if (!child)
		{
			ok = false;
			continue;
		}

		_snwprintf(child, wpath_len + 1 + name_len + 1, L"%s\\%s", wpath, fd.cFileName);
		ok &= (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? remove_all_recursive(child) : (DeleteFileW(child) != 0);
		free(child);
	} while (FindNextFileW(h, &fd));

	FindClose(h);
	return ok & (RemoveDirectoryW(wpath) != 0);
}

bool naui_directory_remove_all(const Naui_Path path)
{
	if (path.length == 0)
		return false;

	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	return remove_all_recursive(wpath);
}

bool naui_directory_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	return naui_file_rename(old_path, new_path);
}

static char s_working[NAUI_PATH_MAX] = {0};
Naui_Path naui_directory_get(Naui_Dir directory)
{
	static char s_home[NAUI_WIN_SHELL_PATH_MAX] = {0};
	static char s_bin[NAUI_WIN_SHELL_PATH_MAX] = {0};
	static char s_appdata[NAUI_WIN_SHELL_PATH_MAX] = {0};
	static char s_downloads[NAUI_WIN_SHELL_PATH_MAX] = {0};
	static char s_temp[NAUI_WIN_SHELL_PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (!s_home[0])
			{
				wchar_t w[NAUI_WIN_SHELL_PATH_MAX];
				if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, w)))
					to_utf8_into(w, s_home, NAUI_WIN_SHELL_PATH_MAX);
			}

			return path_view(s_home);
		}
		case NAUI_DIR_BIN:
		{
			if (!s_bin[0])
			{
				wchar_t w[NAUI_WIN_SHELL_PATH_MAX];
				if (GetModuleFileNameW(NULL, w, NAUI_WIN_SHELL_PATH_MAX))
				{
					wchar_t* last = wcsrchr(w, L'\\');
					if (last)
						*last = L'\0';

					to_utf8_into(w, s_bin, NAUI_WIN_SHELL_PATH_MAX);
				}
			}

			return path_view(s_bin);
		}
		case NAUI_DIR_WORKING:
		{
			if (!s_working[0] && GetCurrentDirectoryW(NAUI_PATH_MAX, s_wide_scratch[0]))
				to_utf8_into(s_wide_scratch[0], s_working, NAUI_PATH_MAX);

			return path_alloc(s_working, strlen(s_working));
		}
		case NAUI_DIR_APPDATA:
		{
			if (!s_appdata[0])
			{
				wchar_t w[NAUI_WIN_SHELL_PATH_MAX];
				if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, w)))
					to_utf8_into(w, s_appdata, NAUI_WIN_SHELL_PATH_MAX);
			}

			return path_view(s_appdata);
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (!s_downloads[0])
			{
				Naui_Path home = naui_directory_get(NAUI_DIR_HOME);
				if (home.length)
					snprintf(s_downloads, NAUI_WIN_SHELL_PATH_MAX, "%s\\Downloads", home.data);
			}

			return path_view(s_downloads);
		}
		case NAUI_DIR_TEMP:
		{
			if (!s_temp[0])
			{
				wchar_t w[NAUI_WIN_SHELL_PATH_MAX];
				DWORD len = GetTempPathW(NAUI_WIN_SHELL_PATH_MAX, w);
				if (len > 0 && len < NAUI_WIN_SHELL_PATH_MAX)
				{
					if (len > 1 && (w[len - 1] == L'\\' || w[len - 1] == L'/'))
						w[len - 1] = L'\0';

					to_utf8_into(w, s_temp, NAUI_WIN_SHELL_PATH_MAX);
				}
			}

			return path_view(s_temp);
		}
	}

	return naui_path_empty();
}

Naui_List(Naui_DirEntry) naui_directory_filter(const Naui_Path path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if (path.length == 0)
		return list;

	const wchar_t* wprepared = prepare_os_path(path);
	if (!wprepared)
		return list;

	size_t wprepared_len = wcslen(wprepared);
	wchar_t* wsearch = (wchar_t*)malloc((wprepared_len + 3) * sizeof(wchar_t));
	if (!wsearch)
		return list;

	_snwprintf(wsearch, wprepared_len + 3, L"%s\\*", wprepared);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wsearch, &fd);
	free(wsearch);

	if (h == INVALID_HANDLE_VALUE)
		return list;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		char name_u8[NAUI_WIN_SHELL_PATH_MAX];
		if (!to_utf8_into(fd.cFileName, name_u8, NAUI_WIN_SHELL_PATH_MAX))
			continue;

		if (!match_filter(name_u8, filter))
			continue;

		if (!match_extensions(name_u8, extensions, ext_count))
			continue;

		int written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s\\%s", path.data, name_u8);
		if (written < 0)
			continue;

		size_t full_len = (size_t)written < NAUI_PATH_MAX ? (size_t)written : NAUI_PATH_MAX - 1;
		ULARGE_INTEGER size;
		size.HighPart = fd.nFileSizeHigh;
		size.LowPart = fd.nFileSizeLow;

		Naui_DirEntry de;
		de.path = path_alloc(s_path_scratch, full_len);
		de.is_directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		de.size = (size_t)size.QuadPart;
		naui_list_push(list, de);
	} while (FindNextFileW(h, &fd));

	FindClose(h);
	return list;
}

Naui_List(Naui_DirEntry) naui_directory_filter_recursive(const Naui_Path path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;
	if (path.length == 0)
		return list;

	filter_recursive_impl_w(path.data, filter, extensions, ext_count, &list);
	return list;
}

void naui_directory_filter_free(Naui_List(Naui_DirEntry) list)
{
	if (!list)
		return;

	for (ptrdiff_t i = 0; i < naui_list_len(list); ++i)
	{
		NAUI_PATH_FREE(list[i].path);
	}

	naui_list_free(list);
}

bool naui_path_set_current(const Naui_Path path)
{
	if (path.length == 0 || !path.data)
		return false;

	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	if (!SetCurrentDirectoryW(wpath))
		return false;

	size_t len = path.length < NAUI_PATH_MAX - 1 ? path.length : NAUI_PATH_MAX - 1;
	memcpy(s_working, path.data, len);
	s_working[len] = '\0';
	return true;
}

bool naui_path_exists(const Naui_Path path)
{
	const wchar_t* wpath = prepare_os_path(path);
	if (!wpath)
		return false;

	return GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES;
}

Naui_Path naui_path_from_cstr(const char* str)
{
	return path_view(str);
}

Naui_Path naui_path_copy(const Naui_Path path)
{
	return path_alloc(path.data, path.length);
}

Naui_Path naui_path_parent(const Naui_Path path)
{
	size_t len = path.length;
	while (len > 1 && is_separator(path.data[len - 1]))
		--len;

	const char* last = NULL;
	for (size_t i = len; i > 0; --i)
	{
		if (!is_separator(path.data[i - 1]))
			continue;

		last = path.data + i - 1;
		break;
	}

	if (!last)
		return path_alloc(".", 1);

	if (last == path.data)
		return path_alloc("/", 1);

	size_t parent_len = (size_t)(last - path.data);
	if (parent_len == 2 && path.data[1] == ':')
		parent_len = 3;

	return path_alloc(path.data, parent_len);
}

Naui_Path naui_path_join(const Naui_Path a, const Naui_Path b)
{
	const char* parts[] = { a.data ? a.data : "", b.data ? b.data : "", NULL };
	return naui_path_join_parts(parts);
}

Naui_Path naui_path_join_parts(const char** parts)
{
	if (!parts)
		return naui_path_empty();

	size_t out_len = 0;
	for (int i = 0; parts[i] != NULL; ++i)
	{
		const char* part = parts[i];
		if (!part)
			continue;

		size_t part_len = strlen(part);
		if (part_len == 0)
			continue;

		size_t start = 0;
		if (out_len > 0)
		{
			while (start < part_len && is_separator(part[start]))
			{
				++start;
			}
		}

		size_t end = part_len;
		while (end > start && is_separator(part[end - 1]))
		{
			--end;
		}

		if (start >= end)
			continue;

		if (out_len > 0 && !is_separator(s_path_scratch[out_len - 1]))
		{
			if (out_len + 1 >= NAUI_PATH_MAX)
				break;

			s_path_scratch[out_len++] = '\\';
		}

		size_t copy_len = end - start;
		if (out_len + copy_len >= NAUI_PATH_MAX)
			copy_len = NAUI_PATH_MAX - 1 - out_len;

		memcpy(s_path_scratch + out_len, part + start, copy_len);
		out_len += copy_len;
		if (out_len >= NAUI_PATH_MAX - 1)
			break;
	}

	s_path_scratch[out_len] = '\0';
	return path_alloc(s_path_scratch, out_len);
}

Naui_Path naui_path_normalize(const Naui_Path path)
{
	if (path.length == 0)
		return path_alloc(".", 1);

	size_t copy_len = path.length < NAUI_PATH_MAX - 1 ? path.length : NAUI_PATH_MAX - 1;
	memcpy(s_path_scratch, path.data, copy_len);
	s_path_scratch[copy_len] = '\0';

	for (char* p = s_path_scratch; *p; ++p)
	{
		if (*p == '/')
			*p = '\\';
	}

	bool is_unc = (s_path_scratch[0] == '\\' && s_path_scratch[1] == '\\');
	bool is_unix_abs = !is_unc && (s_path_scratch[0] == '\\');
	bool is_drive_abs = !is_unc && (s_path_scratch[1] == ':' && s_path_scratch[2] == '\\');

	int n = 0;
	char* scan_start = s_path_scratch + (is_unc ? 2 : 0);
	char* tok = strtok(scan_start, "\\");

	if (is_drive_abs && tok)
	{
		s_normalize_parts[n] = tok;
		s_normalize_part_lens[n] = strlen(tok);
		++n;
		tok = strtok(NULL, "\\");
	}

	int root_depth = is_drive_abs ? 1 : (is_unc ? 2 : 0);
	while (tok && n < NAUI_PATH_MAX_PARTS)
	{
		if (strcmp(tok, ".") == 0)
			;
		else if (strcmp(tok, "..") == 0)
		{
			if (n > root_depth)
				--n;
		}
		else
		{
			s_normalize_parts[n] = tok;
			s_normalize_part_lens[n] = strlen(tok);
			++n;
		}

		tok = strtok(NULL, "\\");
	}

	if (n == 0)
		return path_alloc(is_unix_abs ? "\\" : ".", 1);

	size_t total;
	if (is_unc)
		total = 2 + s_normalize_part_lens[0];
	else if (is_unix_abs)
		total = 1 + s_normalize_part_lens[0];
	else
		total = s_normalize_part_lens[0];

	for (int i = 1; i < n; ++i)
		total += 1 + s_normalize_part_lens[i];

	char* out = (char*)malloc(total + 2);
	if (!out)
		return naui_path_empty();

	size_t w = 0;
	if (is_unc)
	{
		out[w++] = '\\';
		out[w++] = '\\';
	}
	else if (is_unix_abs)
	{
		out[w++] = '\\';
	}

	memcpy(out + w, s_normalize_parts[0], s_normalize_part_lens[0]);
	w += s_normalize_part_lens[0];
	for (int i = 1; i < n; ++i)
	{
		out[w++] = '\\';
		memcpy(out + w, s_normalize_parts[i], s_normalize_part_lens[i]);
		w += s_normalize_part_lens[i];
	}

	if (w == 2 && out[1] == ':')
		out[w++] = '\\';

	out[w] = '\0';
	Naui_Path result;
	result.data = out;
	result.length = w;
	return result;
}

Naui_Path naui_path_absolute(const Naui_Path path)
{
	bool is_abs = path.length > 0 && ((path.length > 1 && path.data[1] == ':') || path.data[0] == '/' || path.data[0] == '\\');
	if (is_abs)
		return path_view_len(path.data, path.length);

	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	Naui_Path result = naui_path_join(cwd, path);
	NAUI_PATH_FREE(cwd);
	return result;
}

Naui_Path naui_path_canonical(const Naui_Path path)
{
	if (path.length == 0)
		return naui_path_empty();

	const wchar_t* winput = prepare_os_path_slot(path, 0);
	if (!winput)
		return naui_path_empty();

	DWORD needed = GetFullPathNameW(winput, (DWORD)NAUI_PATH_MAX, s_wide_scratch[1], NULL);
	if (needed == 0 || needed >= NAUI_PATH_MAX)
		return naui_path_empty();

	if (!to_utf8_into(s_wide_scratch[1], s_path_scratch, NAUI_PATH_MAX))
		return naui_path_empty();

	Naui_Path resolved_utf8 = naui_path_from_cstr(s_path_scratch);
	const wchar_t* wverify = prepare_os_path_slot(resolved_utf8, 1);
	if (!wverify || GetFileAttributesW(wverify) == INVALID_FILE_ATTRIBUTES)
		return naui_path_empty();

	return path_alloc(s_path_scratch, strlen(s_path_scratch));
}

Naui_Path naui_path_weakly_canonical(const Naui_Path path)
{
	Naui_Path existing = path;
	Naui_Path existing_owned = naui_path_empty();
	Naui_Path tail = naui_path_empty();

	while (existing.length != 0 && !naui_path_exists(existing))
	{
		Naui_Path parent = naui_path_parent(existing);
		Naui_StringView segment = naui_file_filename(existing);
		size_t seg_copy = segment.len < NAUI_PATH_MAX - 1 ? segment.len : NAUI_PATH_MAX - 1;
		memcpy(s_path_scratch, segment.data, seg_copy);
		size_t w = seg_copy;

		if (tail.length != 0 && w < NAUI_PATH_MAX - 1)
		{
			s_path_scratch[w++] = '\\';
			size_t tail_room = (NAUI_PATH_MAX - 1) - w;
			size_t tail_copy = tail.length < tail_room ? tail.length : tail_room;
			memcpy(s_path_scratch + w, tail.data, tail_copy);
			w += tail_copy;
		}

		s_path_scratch[w] = '\0';
		Naui_Path new_tail = path_alloc(s_path_scratch, w);
		NAUI_PATH_FREE(tail);
		tail = new_tail;
		NAUI_PATH_FREE(existing_owned);
		existing_owned = parent;
		existing = existing_owned;
	}

	Naui_Path base = (existing.length != 0) ? naui_path_canonical(existing) : path_alloc(".", 1);
	NAUI_PATH_FREE(existing_owned);
	if (tail.length == 0)
		return base;

	Naui_Path joined = naui_path_join(base, tail);
	NAUI_PATH_FREE(base, tail);
	Naui_Path result = naui_path_normalize(joined);
	NAUI_PATH_FREE(joined);
	return result;
}

bool naui_path_lock(const Naui_Path path)
{
	if (path.length == 0 || s_lock_count >= NAUI_LOCK_MAX)
		return false;

	if (!resolve_lock_target(path, s_path_scratch, NAUI_PATH_MAX))
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, s_path_scratch) == 0)
			return false;
	}

	const char* dot_lock = strstr(s_path_scratch, "\\.lock");
	if (dot_lock && *(dot_lock + 6) == '\0')
	{
		size_t parent_len = (size_t)(dot_lock - s_path_scratch);
		Naui_Path parent = path_alloc(s_path_scratch, parent_len);
		const wchar_t* wparent = prepare_os_path(parent);
		if (wparent)
			CreateDirectoryW(wparent, NULL);

		NAUI_PATH_FREE(parent);
	}

	const wchar_t* wtarget = prepare_os_path(naui_path_from_cstr(s_path_scratch));
	if (!wtarget)
		return false;

	HANDLE h = CreateFileW(
		wtarget,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL
	);

	if (h == INVALID_HANDLE_VALUE)
		return false;

	OVERLAPPED overlap = {0};
	if (!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXWORD, MAXWORD, &overlap))
	{
		CloseHandle(h);
		return false;
	}

	char* stored_path = (char*)malloc(strlen(s_path_scratch) + 1);
	if (!stored_path)
	{
		CloseHandle(h);
		return false;
	}

	strcpy(stored_path, s_path_scratch);
	s_locks[s_lock_count].path = stored_path;
	s_locks[s_lock_count].handle = h;
	++s_lock_count;
	return true;
}

bool naui_path_is_locked(const Naui_Path path)
{
	if (path.length == 0)
		return false;

	if (!resolve_lock_target(path, s_path_scratch, NAUI_PATH_MAX))
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, s_path_scratch) == 0)
			return true;
	}

	const wchar_t* wtarget = prepare_os_path(naui_path_from_cstr(s_path_scratch));
	if (!wtarget)
		return false;

	if (GetFileAttributesW(wtarget) == INVALID_FILE_ATTRIBUTES)
		return false;

	HANDLE h = CreateFileW(
		wtarget,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_HIDDEN,
		NULL
	);

	if (h == INVALID_HANDLE_VALUE)
		return true;

	CloseHandle(h);
	return false;
}

void naui_path_unlock(const Naui_Path path)
{
	if (path.length == 0)
		return;

	if (!resolve_lock_target(path, s_path_scratch, NAUI_PATH_MAX))
		return;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (_stricmp(s_locks[i].path, s_path_scratch) != 0)
			continue;

		UnlockFile(s_locks[i].handle, 0, 0, MAXWORD, MAXWORD);
		CloseHandle(s_locks[i].handle);
		free(s_locks[i].path);
		s_locks[i] = s_locks[--s_lock_count];
		return;
	}
}

#endif