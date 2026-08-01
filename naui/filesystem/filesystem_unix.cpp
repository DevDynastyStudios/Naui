#if !defined(_WIN32) && !defined(_WIN64)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#define NAUI_LOCK_MAX 32
typedef struct
{
	char* path;
	int fd;
} Naui_LockEntry;

static Naui_LockEntry s_locks[NAUI_LOCK_MAX];
static int s_lock_count = 0;

static NAUI_THREAD_LOCAL char s_path_scratch[NAUI_PATH_MAX];
#define NAUI_PATH_MAX_PARTS 4096
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

static bool is_separator(char c)
{
	return c == '/';
}

typedef struct { FILE* fp; } Naui_FileInternal;

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

static void filter_recursive_impl(const char* path, const char* filter, const char** extensions, int ext_count, Naui_List(Naui_DirEntry)* list)
{
	DIR* dir = opendir(path);
	if (!dir)
		return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		int written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s/%s", path, entry->d_name);
		if (written < 0)
			continue;

		size_t child_len = (size_t)written < NAUI_PATH_MAX ? (size_t)written : NAUI_PATH_MAX - 1;

		struct stat st;
		if (stat(s_path_scratch, &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode))
		{
			Naui_DirEntry de;
			de.path = path_alloc(s_path_scratch, child_len);
			de.is_directory = true;
			de.size = 0;
			naui_list_push(*list, de);
			filter_recursive_impl(de.path.data, filter, extensions, ext_count, list);
		}
		else
		{
			if (!match_filter(entry->d_name, filter))
				continue;

			if (!match_extensions(entry->d_name, extensions, ext_count))
				continue;

			Naui_DirEntry de;
			de.path = path_alloc(s_path_scratch, child_len);
			de.is_directory = false;
			de.size = (size_t)st.st_size;
			naui_list_push(*list, de);
		}
	}

	closedir(dir);
}

static bool resolve_lock_target(const char* path, char* out, size_t out_capacity)
{
	struct stat st;
	int written;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		written = snprintf(out, out_capacity, "%s/.lock", path);
	else
		written = snprintf(out, out_capacity, "%s", path);

	return written > 0 && (size_t)written < out_capacity;
}

bool naui_file_open(Naui_FileHandle* handle, const Naui_Path path, Naui_FileMode mode)
{
	if (!handle || !path.data)
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
	fi->fp = fopen(path.data, m);
	return fi->fp != NULL;
}

size_t naui_file_read(const Naui_FileHandle* handle, void* buffer, size_t size)
{
	if (!handle || !buffer)
		return 0;

	Naui_FileInternal* fi = file_internal(handle);
	if (!fi->fp)
		return 0;

	return fread(buffer, 1, size, fi->fp);
}

size_t naui_file_write(const Naui_FileHandle* handle, const void* buffer, size_t size)
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

size_t naui_file_size(const Naui_Path path)
{
	if (!path.data)
		return 0;

	struct stat st;
	if (stat(path.data, &st) != 0)
		return 0;

	return (size_t)st.st_size;
}

char* naui_file_read_all(const Naui_Path path, size_t* out_size)
{
	if (!path.data)
		return NULL;

	FILE* fp = fopen(path.data, "rb");
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

bool naui_file_write_all(const Naui_Path path, const void* data, size_t size)
{
	if (!data || !path.data)
		return false;

	FILE* fp = fopen(path.data, "wb");
	if (!fp)
		return false;

	size_t written = fwrite(data, 1, size, fp);
	fclose(fp);
	return written == size;
}

bool naui_file_delete(const Naui_Path path)
{
	if (!path.data)
		return false;

	return remove(path.data) == 0;
}

bool naui_file_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	if (!old_path.data || !new_path.data)
		return false;

	return rename(old_path.data, new_path.data) == 0;
}

Naui_Path naui_file_hide(const Naui_Path path, bool hidden)
{
	Naui_String filename = naui_file_filename(path);
	bool currently_hidden = (filename.length > 0 && filename.data[0] == '.');
	if (currently_hidden == hidden || filename.length == 0)
		return path_view_len(path.data, path.length);

	Naui_Path parent = naui_path_parent(path);

	int written;
	if (hidden)
		written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s/.%.*s", parent.data, (int)filename.length, filename.data);
	else
		written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s/%.*s", parent.data, (int)filename.length - 1, filename.data + 1);

	NAUI_PATH_FREE(parent);

	if (written < 0)
		return path_view_len(path.data, path.length);

	size_t renamed_len = (size_t)written < NAUI_PATH_MAX ? (size_t)written : NAUI_PATH_MAX - 1;

	if (rename(path.data, s_path_scratch) != 0)
		return path_view_len(path.data, path.length);

	return path_alloc(s_path_scratch, renamed_len);
}

bool naui_file_is_hidden(const Naui_Path path)
{
	Naui_String filename = naui_file_filename(path);
	return filename.length > 0 && filename.data[0] == '.';
}

Naui_String naui_file_filename(const Naui_Path path)
{
	if (!path.data)
		return (Naui_String){ NULL, 0 };

	const char* last = NULL;
	for (const char* p = path.data; *p; ++p)
	{
		if (is_separator(*p))
			last = p;
	}

	const char* start = last ? last + 1 : path.data;
	size_t len = path.length - (size_t)(start - path.data);
	return (Naui_String){ (char*)start, len };
}

Naui_String naui_file_stem(const Naui_Path path)
{
	Naui_String filename = naui_file_filename(path);
	if (!filename.data)
		return (Naui_String){ NULL, 0 };

	const char* dot = strrchr(filename.data, '.');
	if (!dot || dot == filename.data)
		return (Naui_String){ filename.data, filename.length };

	size_t len = (size_t)(dot - filename.data);
	return (Naui_String){ filename.data, len };
}

Naui_String naui_file_extension(const Naui_Path path)
{
	Naui_String filename = naui_file_filename(path);
	if (!filename.data)
		return (Naui_String){ NULL, 0 };

	const char* dot = strrchr(filename.data, '.');
	if (!dot || dot == filename.data)
		return (Naui_String){ NULL, 0 };

	size_t len = filename.length - (size_t)(dot - filename.data);
	return (Naui_String){ (char*)dot, len };
}

bool naui_directory_create(const Naui_Path path)
{
	if (!path.data)
		return false;

	return mkdir(path.data, 0755) == 0 || errno == EEXIST;
}

bool naui_directory_remove(const Naui_Path path)
{
	if (!path.data)
		return false;

	return rmdir(path.data) == 0;
}

static bool remove_all_recursive(const char* path)
{
	DIR* dir = opendir(path);
	if (!dir)
		return remove(path) == 0;

	size_t path_len = strlen(path);
	struct dirent* entry;
	bool ok = true;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		size_t name_len = strlen(entry->d_name);
		char* child = (char*)malloc(path_len + 1 + name_len + 1);
		if (!child)
		{
			ok = false;
			continue;
		}

		snprintf(child, path_len + 1 + name_len + 1, "%s/%s", path, entry->d_name);

		struct stat st;
		ok &= (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) ? remove_all_recursive(child) : (remove(child) == 0);
		free(child);
	}

	closedir(dir);
	ok &= (rmdir(path) == 0);
	return ok;
}

bool naui_directory_remove_all(const Naui_Path path)
{
	if (path.length == 0)
		return false;

	return remove_all_recursive(path.data);
}

Naui_List(Naui_DirEntry) naui_directory_filter_recursive(const Naui_Path path, const char* filter, const char** extensions, int ext_count)
{
	Naui_List(Naui_DirEntry) list = NULL;

	if (path.length == 0)
		return list;

	filter_recursive_impl(path.data, filter, extensions, ext_count, &list);
	return list;
}

bool naui_directory_rename(const Naui_Path old_path, const Naui_Path new_path)
{
	return naui_file_rename(old_path, new_path);
}

static char s_working[NAUI_PATH_MAX] = {0};
Naui_Path naui_directory_get(const Naui_Dir directory)
{
	static char s_home[NAUI_PATH_MAX] = {0};
	static char s_bin[NAUI_PATH_MAX] = {0};
	static char s_appdata[NAUI_PATH_MAX] = {0};
	static char s_downloads[NAUI_PATH_MAX] = {0};
	static char s_temp[NAUI_PATH_MAX] = {0};

	switch (directory)
	{
		case NAUI_DIR_HOME:
		{
			if (!s_home[0])
			{
				const char* h = getenv("HOME");
				if (h)
					snprintf(s_home, sizeof(s_home), "%s", h);
			}

			return path_view(s_home);
		}
		case NAUI_DIR_BIN:
		{
			if (!s_bin[0])
			{
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
			}

			return path_view(s_bin);
		}
		case NAUI_DIR_WORKING:
		{
			if (!s_working[0])
			{
				if (!getcwd(s_working, sizeof(s_working)))
					s_working[0] = '\0';
			}

			return path_alloc(s_working, strlen(s_working));
		}
		case NAUI_DIR_APPDATA:
		{
			if (!s_appdata[0])
			{
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
			}

			return path_view(s_appdata);
		}
		case NAUI_DIR_DOWNLOADS:
		{
			if (!s_downloads[0])
			{
				const char* h = getenv("HOME");
				if (h)
					snprintf(s_downloads, sizeof(s_downloads), "%s/Downloads", h);
			}

			return path_view(s_downloads);
		}
		case NAUI_DIR_TEMP:
		{
			if (!s_temp[0])
			{
				const char* tmp = getenv("TMPDIR");
				if (!tmp || !tmp[0])
					tmp = "/tmp";

				snprintf(s_temp, sizeof(s_temp), "%s", tmp);
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

	DIR* dir = opendir(path.data);
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

		int written = snprintf(s_path_scratch, NAUI_PATH_MAX, "%s/%s", path.data, entry->d_name);
		if (written < 0)
			continue;

		size_t full_len = (size_t)written < NAUI_PATH_MAX ? (size_t)written : NAUI_PATH_MAX - 1;

		struct stat st;
		if (stat(s_path_scratch, &st) != 0)
			continue;

		Naui_DirEntry de;
		de.path = path_alloc(s_path_scratch, full_len);
		de.is_directory = S_ISDIR(st.st_mode);
		de.size = (size_t)st.st_size;
		naui_list_push(list, de);
	}

	closedir(dir);
	return list;
}

void naui_directory_filter_free(Naui_List(Naui_DirEntry) list)
{
	if (!list)
		return;

	for (ptrdiff_t i = 0; i < naui_list_len(list); ++i)
		NAUI_PATH_FREE(list[i].path);

	naui_list_free(list);
}

bool naui_path_set_current(const Naui_Path path)
{
	if (path.length == 0 || !path.data)
		return false;

	if (chdir(path.data) != 0)
		return false;

	size_t len = path.length < NAUI_PATH_MAX - 1 ? path.length : NAUI_PATH_MAX - 1;
	memcpy(s_working, path.data, len);
	s_working[len] = '\0';
	return true;
}

bool naui_path_exists(const Naui_Path path)
{
	if (!path.data)
		return false;

	struct stat st;
	return stat(path.data, &st) == 0;
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
				++start;
		}

		size_t end = part_len;
		while (end > start && is_separator(part[end - 1]))
			--end;

		if (start >= end)
			continue;

		if (out_len > 0 && !is_separator(s_path_scratch[out_len - 1]))
		{
			if (out_len + 1 >= NAUI_PATH_MAX)
				break;

			s_path_scratch[out_len++] = '/';
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

	bool is_absolute = (s_path_scratch[0] == '/');
	int n = 0;
	char* tok = strtok(s_path_scratch, "/");

	while (tok && n < NAUI_PATH_MAX_PARTS)
	{
		if (strcmp(tok, ".") == 0)
			;
		else if (strcmp(tok, "..") == 0)
		{
			if (n > 0)
				--n;
		}
		else
		{
			s_normalize_parts[n] = tok;
			s_normalize_part_lens[n] = strlen(tok);
			++n;
		}

		tok = strtok(NULL, "/");
	}

	if (n == 0)
		return path_alloc(is_absolute ? "/" : ".", 1);

	size_t total = (is_absolute ? 1 : 0) + s_normalize_part_lens[0];
	for (int i = 1; i < n; ++i)
		total += 1 + s_normalize_part_lens[i];

	char* out = (char*)malloc(total + 1);
	if (!out)
		return naui_path_empty();

	size_t w = 0;
	if (is_absolute)
		out[w++] = '/';

	memcpy(out + w, s_normalize_parts[0], s_normalize_part_lens[0]);
	w += s_normalize_part_lens[0];

	for (int i = 1; i < n; ++i)
	{
		out[w++] = '/';
		memcpy(out + w, s_normalize_parts[i], s_normalize_part_lens[i]);
		w += s_normalize_part_lens[i];
	}

	out[w] = '\0';

	Naui_Path result;
	result.data = out;
	result.length = w;
	return result;
}

Naui_Path naui_path_absolute(const Naui_Path path)
{
	if (path.length > 0 && path.data[0] == '/')
		return path_view_len(path.data, path.length);

	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	Naui_Path result = naui_path_join(cwd, path);
	NAUI_PATH_FREE(cwd);
	return result;
}

Naui_Path naui_path_canonical(const Naui_Path path)
{
	if (!path.data)
		return naui_path_empty();

	char* resolved = realpath(path.data, NULL);
	if (!resolved)
		return naui_path_empty();

	Naui_Path result = path_alloc(resolved, strlen(resolved));
	free(resolved);
	return result;
}

Naui_Path naui_path_weakly_canonical(const Naui_Path path)
{
	Naui_Path existing = path;
	Naui_Path existing_owned = naui_path_empty();
	Naui_Path tail = naui_path_empty();

	while (existing.length != 0 && !naui_path_exists(existing))
	{
		Naui_Path parent = naui_path_parent(existing);
		Naui_String segment = naui_file_filename(existing);

		size_t seg_copy = segment.length < NAUI_PATH_MAX - 1 ? segment.length : NAUI_PATH_MAX - 1;
		memcpy(s_path_scratch, segment.data, seg_copy);
		size_t w = seg_copy;

		if (tail.length != 0 && w < NAUI_PATH_MAX - 1)
		{
			s_path_scratch[w++] = '/';
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

	if (!resolve_lock_target(path.data, s_path_scratch, NAUI_PATH_MAX))
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, s_path_scratch) == 0)
			return false;
	}

	const char* filename = s_path_scratch + strlen(s_path_scratch);
	while (filename > s_path_scratch && *filename != '/')
		--filename;

	if (strcmp(filename, "/.lock") == 0)
	{
		size_t parent_len = (size_t)(filename - s_path_scratch);
		if (parent_len > 0)
		{
			Naui_Path parent = path_alloc(s_path_scratch, parent_len);
			if (parent.data)
				mkdir(parent.data, 0755);
				
			NAUI_PATH_FREE(parent);
		}
	}

	int fd = open(s_path_scratch, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
	if (fd == -1)
		return false;

	if (flock(fd, LOCK_EX | LOCK_NB) != 0)
	{
		close(fd);
		return false;
	}

	char* stored_path = (char*)malloc(strlen(s_path_scratch) + 1);
	if (!stored_path)
	{
		close(fd);
		return false;
	}

	strcpy(stored_path, s_path_scratch);
	s_locks[s_lock_count].path = stored_path;
	s_locks[s_lock_count].fd = fd;
	++s_lock_count;
	return true;
}

bool naui_path_is_locked(const Naui_Path path)
{
	if (path.length == 0)
		return false;

	if (!resolve_lock_target(path.data, s_path_scratch, NAUI_PATH_MAX))
		return false;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, s_path_scratch) == 0)
			return true;
	}

	struct stat st;
	if (stat(s_path_scratch, &st) != 0)
		return false;

	int fd = open(s_path_scratch, O_RDWR | O_CLOEXEC);
	if (fd == -1)
		return false;

	bool locked = (flock(fd, LOCK_EX | LOCK_NB) != 0);
	if (!locked)
		flock(fd, LOCK_UN);

	close(fd);
	return locked;
}

void naui_path_unlock(const Naui_Path path)
{
	if (path.length == 0)
		return;

	if (!resolve_lock_target(path.data, s_path_scratch, NAUI_PATH_MAX))
		return;

	for (int i = 0; i < s_lock_count; ++i)
	{
		if (strcmp(s_locks[i].path, s_path_scratch) != 0)
			continue;

		flock(s_locks[i].fd, LOCK_UN);
		close(s_locks[i].fd);
		free(s_locks[i].path);
		s_locks[i] = s_locks[--s_lock_count];
		return;
	}
}

#endif
