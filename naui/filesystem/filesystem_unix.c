#if !defined(_WIN32) && !defined(_WIN64)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "filesystem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

static Naui_Path path_empty(void)
{
	Naui_Path p;
	p.data[0] = '\0';
	return p;
}

static Naui_Path path_from(const char* s)
{
	Naui_Path p;
	if (s)
		snprintf(p.data, NAUI_PATH_MAX, "%s", s);
	else
		p.data[0] = '\0';

	return p;
}

static bool is_separator(char c)
{
	return c == '/';
}

typedef struct { FILE* fp; } Naui_FileInternal;

static Naui_FileInternal* file_internal(const Naui_FileHandle* handle)
{
	return (Naui_FileInternal*)(void*)handle->_opaque;
}

#define NAUI_LOCK_MAX 256
static struct
{
	char path[NAUI_PATH_MAX];
} s_locks[NAUI_LOCK_MAX];

static int s_lock_count = 0;

bool naui_file_open(Naui_FileHandle* handle, Naui_Path* path, Naui_FileMode mode)
{
	if (!handle)
		return false;

	const char* m;
	switch (mode)
	{
		case NAUI_FILE_READ:
			m = "rb";
			break;

		case NAUI_FILE_WRITE:
			m = "wb";
			break;

		case NAUI_FILE_APPEND:
			m = "ab";
			break;

		default:
			return false;
	}

	Naui_FileInternal* fi = file_internal(handle);
	fi->fp = fopen(path->data, m);
	return fi->fp != NULL;
}

size_t naui_file_read(const Naui_FileHandle* handle, void* restrict buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	Naui_FileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return 0;

	return fread(buffer, 1, size, fi->fp);
}

size_t naui_file_write(const Naui_FileHandle* handle, const void* restrict buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	Naui_FileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return 0;

	return fwrite(buffer, 1, size, fi->fp);
}

bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin)
{
	if (!handle)
		return false;

	Naui_FileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return false;

	return fseek(fi->fp, offset, origin) == 0;
}

bool naui_file_is_valid(const Naui_FileHandle* handle)
{
	if (!handle)
		return false;

	return file_internal(handle)->fp != NULL;
}

void naui_file_close(Naui_FileHandle* handle)
{
	if (!handle)
		return;

	Naui_FileInternal* fi = file_internal(handle);
	if (fi->fp)
	{
		fclose(fi->fp);
		fi->fp = NULL;
	}
}

size_t naui_file_size(Naui_Path* path)
{
	struct stat st;
	if (stat(path->data, &st) != 0)
		return 0;

	return (size_t)st.st_size;
}

char* naui_file_read_all(Naui_Path* path, size_t* out_size)
{
	FILE* fp = fopen(path->data, "rb");
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

	size_t got = fread(buf, 1, (size_t)len, fp);
	fclose(fp);
	buf[got] = '\0';

	if (out_size)
		*out_size = got;

	return buf;
}

bool naui_file_write_all(Naui_Path* path, const void* data, size_t size)
{
	if (!data)
		return false;

	FILE* fp = fopen(path->data, "wb");
	if (!fp)
		return false;

	size_t written = fwrite(data, 1, size, fp);
	fclose(fp);
	return written == size;
}

bool naui_file_delete(Naui_Path* path)
{
	return remove(path->data) == 0;
}

bool naui_file_rename(Naui_Path* old_path, Naui_Path* new_path)
{
	return rename(old_path->data, new_path->data) == 0;
}

bool naui_file_hide(Naui_Path* path, bool hidden)
{
	const char* filename = naui_file_filename(path);
	bool currently_hidden = (filename[0] == '.');
	if (currently_hidden == hidden)
		return true;

	Naui_Path parent = naui_path_parent(path);
	Naui_Path renamed;

	const char* expr = hidden ? "%s/.%s" : "%s/%s";
	snprintf(renamed.data, NAUI_PATH_MAX, expr, parent.data, filename + 1);
	return rename(path->data, renamed.data) == 0;
}

bool naui_file_is_hidden(Naui_Path* path)
{
	const char* filename = naui_file_filename(path);
	return filename[0] == '.';
}

const char* naui_file_filename(Naui_Path* path)
{
	const char* data = path->data;
	const char* last = NULL;
	for (const char* p = data; *p; ++p)
	{
		if (is_separator(*p))
			last = p;
	}

	return last ? last + 1 : data;
}

Naui_Path naui_file_stem(Naui_Path* path)
{
	const char* filename = naui_file_filename(path);
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return path_from(filename);

	Naui_Path result;
	size_t len = (size_t)(dot - filename);
	if (len >= NAUI_PATH_MAX)
		len = NAUI_PATH_MAX - 1;

	memcpy(result.data, filename, len);
	result.data[len] = '\0';
	return result;
}

Naui_Path naui_file_extension(Naui_Path* path)
{
	const char* filename = naui_file_filename(path);
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return path_empty();

	return path_from(dot);
}

bool naui_directory_create(Naui_Path* path)
{
	return mkdir(path->data, 0755) == 0 || errno == EEXIST;
}

bool naui_directory_remove(Naui_Path* path)
{
	return rmdir(path->data) == 0;
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

		char child[NAUI_PATH_MAX];
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

bool naui_directory_remove_all(Naui_Path* path)
{
	if(path->data[0] == '\0')
		return;

	return remove_all_recursive(path->data);
}

bool naui_directory_rename(Naui_Path* old_path, Naui_Path* new_path)
{
	return naui_file_rename(old_path, new_path);
}

static char s_working[NAUI_PATH_MAX] = {0};	// Used in `naui_directory_get` and `naui_path_set_current`
Naui_Path naui_directory_get(Naui_Dir directory)
{
	static char s_home[NAUI_PATH_MAX] = {0};
	static char s_bin[NAUI_PATH_MAX] = {0};
	static char s_appdata[NAUI_PATH_MAX] = {0};
	static char s_downloads[NAUI_PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (s_home[0])
				return path_from(s_home);

			const char* h = getenv("HOME");
			if (h)
				snprintf(s_home, sizeof(s_home), "%s", h);

			return path_from(s_home);
		}
		case NAUI_DIR_BIN:
		{
			if (s_bin[0])
				return path_from(s_bin);

#ifdef __APPLE__
			uint32_t size = sizeof(s_bin);
			if (_NSGetExecutablePath(s_bin, &size) == 0)
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
			return path_from(s_bin);
		}
		case NAUI_DIR_WORKING:
		{
			if (s_working[0])
				return path_from(s_working);

			getcwd(s_working, sizeof(s_working));
			return path_from(s_working);
		}
		case NAUI_DIR_APPDATA:
		{
			if (s_appdata[0])
				return path_from(s_appdata);

#ifdef __APPLE__
			const char* h = getenv("HOME");
			if (h)
				snprintf(s_appdata, sizeof(s_appdata), "%s/Library/Application Support", h);
#else
			const char* xdg = getenv("XDG_DATA_HOME");
			if (xdg)
				snprintf(s_appdata, sizeof(s_appdata), "%s", xdg);
			else
			{
				const char* h = getenv("HOME");
				if (h)
					snprintf(s_appdata, sizeof(s_appdata), "%s/.local/share", h);
			}
#endif
			return path_from(s_appdata);
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (s_downloads[0])
				return path_from(s_downloads);

			const char* h = getenv("HOME");
			if (h)
				snprintf(s_downloads, sizeof(s_downloads), "%s/Downloads", h);

			return path_from(s_downloads);
		}
	}

	return path_empty();
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

	const char* suffix = star + 1;
	size_t suffix_len = strlen(suffix);
	size_t name_len = strlen(name);
	if (suffix_len > name_len - prefix_len)
		return false;

	return strcmp(name + name_len - suffix_len, suffix) == 0;
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
		if (exts[i] && strcmp(dot, exts[i]) == 0)
			return true;
	}

	return false;
}

Naui_List(Naui_DirEntry) naui_directory_filter(Naui_Path* path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if(path->data[0] == '\0')
		return list;

	DIR* dir = opendir(path->data);
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

		char child[NAUI_PATH_MAX];
		snprintf(child, sizeof(child), "%s/%s", path->data, entry->d_name);

		struct stat st;
		if (stat(child, &st) != 0)
			continue;

		Naui_DirEntry de;
		de.path = path_from(child);
		de.is_directory = S_ISDIR(st.st_mode);
		de.size = (size_t)st.st_size;
		naui_list_push(list, de);
	}

	closedir(dir);
	return list;
}

void naui_directory_filter_free(Naui_List(Naui_DirEntry) list)
{
	if (list)
		naui_list_free(list);
}

bool naui_path_set_current(Naui_Path* path)
{
	if(path->data[0] == '\0')
		return false;

	if(chdir(path->data) != 0)
		return false;

	memcpy(s_working, path->data, NAUI_PATH_MAX);
	return true;
}

bool naui_path_exists(Naui_Path* path)
{
	struct stat st;
	return stat(path->data, &st) == 0;
}

Naui_Path naui_path_parent(Naui_Path* path)
{
	size_t len = strlen(path->data);
	while (len > 1 && is_separator(path->data[len - 1]))
	{
		--len;
	}

	const char* last = NULL;
	for (size_t i = len; i > 0; --i)
	{
		if (!is_separator(path->data[i - 1]))
			continue;

		last = path->data + i - 1;
		break;
	}

	if (!last)
		return path_from(".");

	if (last == path->data)
		return path_from("/");

	size_t parent_len = (size_t)(last - path->data);
	Naui_Path result;
	memcpy(result.data, path->data, parent_len);
	result.data[parent_len] = '\0';
	return result;
}

Naui_Path naui_path_join(Naui_Path* a, Naui_Path* b)
{
	if (b->data[0] == '/')
		return *b;

	Naui_Path result;
	size_t a_len = strlen(a->data);

	if (a_len == 0)
		return *b;

	const char* expr = is_separator(a->data[a_len - 1]) ? "%s%s" : "%s/%s"
	snprintf(result.data, NAUI_PATH_MAX, expr, a->data, b->data);
	return result;
}

Naui_Path naui_path_normalize(Naui_Path* path)
{
	char buf[NAUI_PATH_MAX];
	snprintf(buf, sizeof(buf), "%s", path->data);

	bool is_absolute = (buf[0] == '/');
	const char* parts[NAUI_PATH_MAX >> 1];
	int n = 0;
	char* tok = strtok(buf, "/");

	while (tok)
	{
		if (strcmp(tok, ".") == 0)
			;
		else if (strcmp(tok, "..") == 0)
		{
			if (n > 0)
				--n;
		}
		else
			parts[n++] = tok;

		tok = strtok(NULL, "/");
	}

	if (n == 0)
		return path_from(is_absolute ? "/" : ".");

	Naui_Path result;
	result.data[0] = '\0';

	const char* expr = is_absolute ? "/%s" : "%s";
	snprintf(result.data, NAUI_PATH_MAX, expr, parts[0]);

	for (int i = 1; i < n; ++i)
	{
		strncat(result.data, "/", NAUI_PATH_MAX - strlen(result.data) - 1), strncat(result.data, parts[i], NAUI_PATH_MAX - strlen(result.data) - 1);
	}

	return result;
}

Naui_Path naui_path_absolute(Naui_Path* path)
{
	if (path->data[0] == '/')
		return *path;

	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	return naui_path_join(&cwd, path);
}

Naui_Path naui_path_canonical(Naui_Path* path)
{
	char resolved[NAUI_PATH_MAX];
	if (!realpath(path->data, resolved))
		return path_empty();

	return path_from(resolved);
}

Naui_Path naui_path_weakly_canonical(Naui_Path* path)
{
	Naui_Path existing = *path;
	Naui_Path tail = path_empty();

	while (existing.data[0] != '\0' && !naui_path_exists(&existing))
	{
		Naui_Path parent = naui_path_parent(&existing);
		Naui_Path segment = path_from(naui_file_filename(&existing));

		if (tail.data[0] != '\0')
		{
			Naui_Path new_tail;
			snprintf(new_tail.data, NAUI_PATH_MAX, "%s/%s", segment.data, tail.data);
			tail = new_tail;
		}
		else
			tail = segment;

		existing = parent;
	}

	Naui_Path base = (existing.data[0] != '\0') ? naui_path_canonical(&existing) : path_from(".");
	if (tail.data[0] == '\0')
		return base;

	Naui_Path joined = naui_path_join(&base, &tail);
	return naui_path_normalize(&joined);
}

bool naui_path_lock(Naui_Path* path)
{
	if(path->data[0] == '\0' || s_lock_count >= NAUI_LOCK_MAX)
		return false;

	if (naui_path_is_locked(path))
		return false;

	snprintf(s_locks[s_lock_count].path, NAUI_PATH_MAX, "%s", path->data);
	++s_lock_count;
	return true;
}

bool naui_path_is_locked(Naui_Path* path)
{
	if(path->data[0] == '\0')
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, path->data) == 0)
			return true;
	}

	return false;
}

void naui_path_unlock(Naui_Path* path)
{
	if(path->data[0] == '\0')
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, path->data) != 0)
			continue;

		s_locks[i] = s_locks[--s_lock_count];
		return;
	}
}

#endif