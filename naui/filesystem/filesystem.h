#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils/list.h"

#ifndef NAUI_PATH_MAX
#if defined(_WIN32) || defined(_WIN64)
#	define NAUI_PATH_MAX 1024
#else
#	define NAUI_PATH_MAX 4096
#endif
#endif

typedef struct Naui_Path
{
	char data[NAUI_PATH_MAX];
} Naui_Path;

#define NAUI_PATH(str) (Naui_Path){ .data = (str) }

typedef uint8_t Naui_Dir;
enum
{
	NAUI_DIR_HOME,
	NAUI_DIR_BIN,
	NAUI_DIR_WORKING,
	NAUI_DIR_APPDATA,
	NAUI_DIR_DOWNLOADS
};

typedef uint8_t Naui_FileMode;
enum
{
	NAUI_FILE_READ,
	NAUI_FILE_WRITE,
	NAUI_FILE_APPEND
};

#define NAUI_FILE_HANDLE_SIZE 64
typedef struct Naui_FileHandle
{
	unsigned char _opaque[NAUI_FILE_HANDLE_SIZE];
} Naui_FileHandle;

#define NAUI_FILE_HANDLE_INIT { {0} }

typedef struct Naui_DirEntry
{
	Naui_Path path;
	bool is_directory;
	size_t size;
} Naui_DirEntry;

/* Open a file. Returns true on success. */
bool naui_file_open(Naui_FileHandle* handle, Naui_Path* path, Naui_FileMode mode);

/* Read/write up to `size` bytes.
 * Returns bytes actually transferred. */
size_t naui_file_read(const Naui_FileHandle* handle, void* restrict buffer, size_t size);
size_t naui_file_write(const Naui_FileHandle* handle, const void* restrict buffer, size_t size);

/* Seek within a file. `origin`: SEEK_SET / SEEK_CUR / SEEK_END.
 * Returns false on invalid handle or seek failure. */
bool naui_file_seek(Naui_FileHandle* handle, long offset, int origin);

/* Returns false if the handle was never opened or has been closed. */
bool naui_file_is_valid(const Naui_FileHandle* handle);

void naui_file_close(Naui_FileHandle* handle);

/* Returns file size in bytes, or 0 on error. */
size_t naui_file_size(Naui_Path* path);

/* Read entire file into a heap buffer.
 * Sets *out_size to bytes read (excludes null terminator). NULL on failure. */
char* naui_file_read_all(Naui_Path* path, size_t* out_size);

/* Write `size` bytes from `data` to path, creating or truncating the file. */
bool naui_file_write_all(Naui_Path* path, const void* data, size_t size);

/* Delete or rename a file. */
bool naui_file_delete(Naui_Path* path);
bool naui_file_rename(Naui_Path* old_path, Naui_Path* new_path);

/* Hide or unhide a file. */
bool naui_file_hide(Naui_Path* path, bool hidden);
bool naui_file_is_hidden(Naui_Path* path);

/* Returns a pointer INTO path.data at the final component. */
const char* naui_file_filename(Naui_Path* path);

/* Returns the filename without its extension, as a NauiPath.
 * Returns an empty NauiPath if there is no stem. */
Naui_Path naui_file_stem(Naui_Path* path);

/* Returns the extension including the dot, as a NauiPath.
 * Returns an empty NauiPath if there is no extension. */
Naui_Path naui_file_extension(Naui_Path* path);

bool naui_directory_create(Naui_Path* path);
bool naui_directory_remove(Naui_Path* path);
bool naui_directory_remove_all(Naui_Path* path);
bool naui_directory_rename(Naui_Path* old_path, Naui_Path* new_path);

/* Returns a NauiPath for a well-known directory.
 * Returns an empty NauiPath on failure. */
Naui_Path naui_directory_get(Naui_Dir directory);

/* List directory entries.
 * Returns a Naui_List(Naui_DirEntry). Call naui_directory_filter_free() when done. */
Naui_List(Naui_DirEntry) naui_directory_filter(Naui_Path* path, const char* filter, const char** extensions, int ext_count);

/* Frees the list returned by naui_directory_filter. */
void naui_directory_filter_free(Naui_List(Naui_DirEntry) list);

bool naui_path_set_current(Naui_Path* current_directory);

/* Returns true if the path exists on the filesystem. */
bool naui_path_exists(Naui_Path* path);

/* Returns the parent directory. */
Naui_Path naui_path_parent(Naui_Path* path);

/* Join two path components, inserting a separator as needed. */
Naui_Path naui_path_join(Naui_Path* a, Naui_Path* b);

/* Resolve `.` and `..` components without hitting the filesystem. */
Naui_Path naui_path_normalize(Naui_Path* path);

/* Returns the absolute path by prepending the current working directory */
Naui_Path naui_path_absolute(Naui_Path* path);

/* The path must exist. Returns an empty NauiPath on failure. */
Naui_Path naui_path_canonical(Naui_Path* path);

/* Resolves as much of the path as possible, then normalizes the rest. */
Naui_Path naui_path_weakly_canonical(Naui_Path* path);

bool naui_path_lock(Naui_Path* path);
bool naui_path_is_locked(Naui_Path* path);
void naui_path_unlock(Naui_Path* path);