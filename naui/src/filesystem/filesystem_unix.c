#if !defined(_WIN32) && !defined(_WIN64)

#include "filesystem.h"

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <limits.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

#ifdef __APPLE__
#  include <mach-o/dyld.h>
#endif

typedef struct
{
	FILE* fp;
} NauiFileInternal;

//_Static_assert(sizeof(NauiFileInternal) <= NAUI_FILE_HANDLE_SIZE, "NAUI_FILE_HANDLE_SIZE too small for NauiFileInternal");

static NauiFileInternal* file_internal(const NauiFileHandle* h)
{
	return (NauiFileInternal*)(void*)h->_opaque;
}

#define NAUI_LOCK_MAX 256

static struct
{
	char path[PATH_MAX];
} s_locks[NAUI_LOCK_MAX];

static int s_lock_count = 0;

void naui_filesystem_free(void* ptr)
{
	free(ptr);
}

bool naui_file_open(NauiFileHandle* handle, const char* path, NauiFileMode mode)
{
	if (!handle || !path)
		return false;

	const char* m;
	switch (mode)
	{
		case NAUI_FILE_READ: m = "rb";
		break;

		case NAUI_FILE_WRITE: m = "wb";
		break;

		case NAUI_FILE_APPEND: m = "ab";
		break;

		default: return false;
	}

	NauiFileInternal* fi = file_internal(handle);
	fi->fp = fopen(path, m);
	return fi->fp != NULL;
}

size_t naui_file_read(const NauiFileHandle* handle, void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return 0;

	return fread(buffer, 1, size, fi->fp);
}

size_t naui_file_write(const NauiFileHandle* handle, const void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return 0;

	return fwrite(buffer, 1, size, fi->fp);
}

size_t naui_file_size(const char* path)
{
	if (!path)
		return 0;

	struct stat st;
	if (stat(path, &st) != 0)
		return 0;

	return (size_t)st.st_size;
}

char* naui_file_read_all(const char* path, size_t* out_size)
{
	if (!path)
		return NULL;

	FILE* fp = fopen(path, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	rewind(fp);

	if (len < 0)
	{
		fclose(fp);
		return NULL;
	}

	char* buf = (char*)malloc((size_t)len + 1);
	if (!buf)
	{
		fclose(fp);
		return NULL;
	}

	size_t read = fread(buf, 1, (size_t)len, fp);
	fclose(fp);

	buf[read] = '\0';
	if (out_size)
		*out_size = read;

	return buf;
}

bool naui_file_write_all(const char* path, const void* data, size_t size)
{
	if (!path || !data)
		return false;

	FILE* fp = fopen(path, "wb");
	if (!fp)
		return false;

	size_t written = fwrite(data, 1, size, fp);
	fclose(fp);
	return written == size;
}

bool naui_file_is_valid(const NauiFileHandle* handle)
{
	if (!handle)
		return false;

	return file_internal(handle)->fp != NULL;
}

bool naui_file_delete(const char* path)
{
	if (!path)
		return false;

	return remove(path) == 0;
}

bool naui_file_rename(const char* old_path, const char* new_path)
{
	if (!old_path || !new_path)
		return false;

	return rename(old_path, new_path) == 0;
}

bool naui_file_seek(NauiFileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	NauiFileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return false;

	return fseek(fi->fp, offset, origin) == 0;
}

void naui_file_close(NauiFileHandle* handle)
{
	if (!handle)
		return;

	NauiFileInternal* fi = file_internal(handle);
	if (fi->fp)
	{
		fclose(fi->fp);
		fi->fp = NULL;
	}
}

const char* naui_path_filename(const char* path)
{
	if (!path)
		return NULL;

	const char* last = strrchr(path, '/');
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

	return strdup(dot);
}

char* naui_path_parent(const char* path)
{
	if (!path)
		return NULL;

	size_t len = strlen(path);
	if (len == 0)
		return strdup(".");

	while (len > 1 && path[len - 1] == '/')
	{
		--len;
	}

	const char* last = NULL;
	for (size_t i = len; i > 0; --i)
	{
		if (path[i - 1] == '/')
		{
			last = path + i - 1;
			break;
		}
	}

	if (!last)
		return strdup(".");

	if (last == path)
		return strdup("/");

	size_t parent_len = (size_t)(last - path);
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

	struct stat st;
	return stat(path, &st) == 0;
}

bool naui_directory_create(const char* path)
{
	if (!path)
		return false;

	return mkdir(path, 0755) == 0 || errno == EEXIST;
}

bool naui_directory_remove(const char* path)
{
	if (!path)
		return false;

	return rmdir(path) == 0;
}

static bool remove_all_recursive(const char* path)
{
	DIR* dir = opendir(path);
	if (!dir)
		return remove(path) == 0;

	struct dirent* entry;
	bool ok = true;

	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char child[PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

		struct stat st;
		if (stat(child, &st) == 0 && S_ISDIR(st.st_mode))
			ok &= remove_all_recursive(child);
		else
			ok &= (remove(child) == 0);
	}

	closedir(dir);
	ok &= (rmdir(path) == 0);
	return ok;
}

bool naui_directory_remove_all(const char* path)
{
	if (!path)
		return false;

	return remove_all_recursive(path);
}

bool naui_directory_rename(const char* old_path, const char* new_path)
{
	if (!old_path || !new_path)
		return false;

	return rename(old_path, new_path) == 0;
}

const char* naui_directory_get(NauiDir directory)
{
	static char s_home[PATH_MAX] = {0};
	static char s_bin[PATH_MAX] = {0};
	static char s_working[PATH_MAX] = {0};
	static char s_appdata[PATH_MAX] = {0};
	static char s_downloads[PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (s_home[0])
				return s_home;

			const char* home = getenv("HOME");
			if (home)
				snprintf(s_home, sizeof(s_home), "%s", home);

			return s_home;
		}
		case NAUI_DIR_BIN:
		{
			if (s_bin[0])
				return s_bin;

#ifdef __APPLE__
			uint32_t size = sizeof(s_bin);
			if (_NSGetExecutablePath(s_bin, &size) != 0)
				s_bin[0] = '\0';
			else
			{
				char* last = strrchr(s_bin, '/');
				if (last)
					*last = '\0';
			}
#else
			ssize_t n = readlink("/proc/self/exe", s_bin, sizeof(s_bin) - 1);
			if (n > 0)
			{
				s_bin[n] = '\0';
				char* last = strrchr(s_bin, '/');
				if (last)
					*last = '\0';
			}
#endif
			return s_bin;
		}
		case NAUI_DIR_WORKING:
		{
			if (s_working[0])
				return s_working;

			if (!getcwd(s_working, sizeof(s_working)))
				s_working[0] = '\0';

			return s_working;
		}
		case NAUI_DIR_APPDATA:
		{
			if (s_appdata[0])
				return s_appdata;

#ifdef __APPLE__
			const char* home = getenv("HOME");
			if (home)
				snprintf(s_appdata, sizeof(s_appdata), "%s/Library/Application Support", home);
#else
			const char* xdg = getenv("XDG_DATA_HOME");
			if (xdg)
				snprintf(s_appdata, sizeof(s_appdata), "%s", xdg);
			else
			{
				const char* home = getenv("HOME");
				if (home)
					snprintf(s_appdata, sizeof(s_appdata), "%s/.local/share", home);
			}
#endif
			return s_appdata;
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (s_downloads[0])
				return s_downloads;

			const char* home = getenv("HOME");
			if (home)
				snprintf(s_downloads, sizeof(s_downloads), "%s/Downloads", home);

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
	if (strncmp(name, filter, prefix_len) != 0)
		return false;

	name += prefix_len;
	filter = star + 1;

	size_t suffix_len = strlen(filter);
	size_t name_len = strlen(name);
	if (suffix_len > name_len)
		return false;

	return strcmp(name + name_len - suffix_len, filter) == 0;
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
		if (extensions[i] && strcmp(dot, extensions[i]) == 0)
			return true;
	}

	return false;
}

Naui_List(NauiDirEntry) naui_directory_filter(const char* path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(NauiDirEntry) list = naui_list__init(sizeof(NauiDirEntry), NAUI_LIST_DEFAULT_CAP);
	if (!path)
		return list;

	DIR* dir = opendir(path);
	if (!dir)
		return list;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (!match_filter(entry->d_name, filter))
			continue;

		if (!match_extensions(entry->d_name, extensions, ext_count))
			continue;

		char child[PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

		struct stat st;
		if (stat(child, &st) != 0)
			continue;

		NauiDirEntry de;
		de.path = strdup(child);
		de.is_directory = S_ISDIR(st.st_mode);
		de.size = (size_t)st.st_size;
		naui_list_push(list, de);
	}

	closedir(dir);
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
		if (strcmp(s_locks[i].path, path) == 0)
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
		if (strcmp(s_locks[i].path, path) == 0)
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

	const char* filename = naui_path_filename(path);
	bool currently_hidden = (filename[0] == '.');

	if (currently_hidden == is_hidden)
		return true;

	char* parent = naui_path_parent(path);
	if (!parent)
		return false;

	char renamed[PATH_MAX];
	if (is_hidden)
		snprintf(renamed, sizeof(renamed), "%s/.%s", parent, filename);
	else
		snprintf(renamed, sizeof(renamed), "%s/%s", parent, filename + 1);

	free(parent);
	return rename(path, renamed) == 0;
}

bool naui_is_hidden(const char* path)
{
	if (!path)
		return false;

	const char* filename = naui_path_filename(path);
	return filename && filename[0] == '.';
}

#endif