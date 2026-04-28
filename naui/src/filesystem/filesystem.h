#pragma once
#include <stdbool.h>
#include "utils/list.h"

typedef enum NauiDir
{
	NAUI_DIR_HOME,
	NAUI_DIR_BIN,
	NAUI_DIR_WORKING,
	NAUI_DIR_APPDATA,
	NAUI_DIR_DOWNLOADS
} NauiDir;

typedef enum NauiFileMode
{
	NAUI_FILE_READ,
	NAUI_FILE_WRITE,
	NAUI_FILE_APPEND
} NauiFileMode;

typedef struct NauiDirEntry
{
	char* path; /* heap-allocated, owned by the list */
	bool is_directory;
	size_t size;
} NauiDirEntry;

/*
 * NauiFileHandle is stack-allocatable. Use NAUI_FILE_HANDLE_INIT to zero it.
 * Internal layout is opaque — do not read fields directly.
 *
 * On all platforms this wraps a FILE* in a buffer large enough to hold
 * the platform handle without an extra heap allocation.
 */
#define NAUI_FILE_HANDLE_SIZE 64
typedef struct NauiFileHandle
{
	unsigned char _opaque[NAUI_FILE_HANDLE_SIZE];
} NauiFileHandle;

#define NAUI_FILE_HANDLE_INIT { {0} }

/*
 * Open a file. Returns true on success.
 * `handle` must be a pointer to a NauiFileHandle.
 */
bool naui_file_open(NauiFileHandle* handle, const char* path, NauiFileMode mode);


//Read/write up to `size` bytes. Returns bytes actually transferred.
size_t naui_file_read(const NauiFileHandle* handle, void* buffer, size_t size);
size_t naui_file_write(const NauiFileHandle* handle, const void* buffer, size_t size);

//Returns file size in bytes.
// 0 on Error.
size_t naui_file_size(const char* path);

/*
 * Read entire file into a heap buffer. Caller must call naui_fs_free().
 * Sets *out_size to the number of bytes read (excluding null terminator).
 * Returns NULL on failure.
 */
char* naui_file_read_all(const char* path, size_t* out_size);


// Write `size` bytes from `data` to `path`, creating or truncating the file.
bool naui_file_write_all(const char* path, const void* data, size_t size);


// Returns false if the handle was never successfully opened or has been closed.
bool naui_file_is_valid(const NauiFileHandle* handle);

bool naui_file_delete(const char* path);
bool naui_file_rename(const char* old_path, const char* new_path);

/*
 * Seek within a file. `origin` uses standard SEEK_SET / SEEK_CUR / SEEK_END.
 * Returns false on failure (invalid handle, or platform seek error).
 */
bool naui_file_seek(NauiFileHandle* handle, long offset, int origin);
void naui_file_close(NauiFileHandle* handle);


// Returns a pointer INTO `path` at the final component.
const char* naui_path_filename(const char* path);

/*
 * Returns a heap-allocated copy of the extension including the dot, or NULL if none.
 * Caller must call naui_fs_free().
 */
char* naui_path_get_extension(const char* path);

/*
 * Returns a heap-allocated string containing the parent directory.
 * Caller must call naui_fs_free().
 */
char* naui_path_parent(const char* path);
bool  naui_path_exists(const char* path);

bool naui_directory_create(const char* path);
bool naui_directory_remove(const char* path);
bool naui_directory_remove_all(const char* path);
bool naui_directory_rename(const char* old_path, const char* new_path);

/*
 * Returns a pointer into a static internal buffer.
 * Valid for the lifetime of the program.
 */
const char* naui_directory_get(NauiDir directory);

/*
 * List directory entries. 
 *   `filter`	 - optional glob/substring match on filename (NULL = all).
 *   `extensions` - optional array of extensions to include, e.g. ".txt".
 *   `ext_count`  - number of entries in `extensions` (0 = all).
 *
 * Returns a Naui_List.
 * Call naui_directory_filter_free() when done.
 */
Naui_List(NauiDirEntry) naui_directory_filter(const char* path, const char* filter, const char** extensions, int ext_count);

/*
 * Frees all path strings inside the list, then frees the list itself.
 * Always use this instead of naui_list_free for NauiDirEntry lists.
 */
void naui_directory_filter_free(Naui_List(NauiDirEntry) list);

bool naui_path_lock(const char* path);
bool naui_is_locked(const char* path);
void naui_path_unlock(const char* path);

bool naui_hide_file(const char* path, bool is_hidden);
bool naui_is_hidden(const char* path);

/*
 * Free any heap pointer returned by this API.
 * Equivalent to free(), provided as a single release point in case the allocator is ever swapped out.
 */
void naui_filesystem_free(void* ptr);