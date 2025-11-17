#pragma once

#include "../base.h"
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

enum class NauiFileMode
{
    Read,
    Write,
    Append
};

struct NauiFile
{
	void* handle = nullptr;      // FILE* or platform-specific file handle
    std::filesystem::path path;  // Optional: record which file this refers to (helps debugging)
    NauiFileMode mode = NauiFileMode::Read;
};

struct NauiDirEntry
{
    std::filesystem::path path;
    bool is_directory;
    uint64_t size;
};

NAUI_API std::vector<std::filesystem::directory_entry> naui_fs_filter(const std::filesystem::path& path, std::string_view name_filter = {}, const std::vector<std::string_view>& allowed_extensions = {});
NAUI_API NauiFile naui_fs_open(const std::filesystem::path& path, NauiFileMode mode);
NAUI_API void naui_fs_close(NauiFile& file);

NAUI_API size_t naui_fs_read(NauiFile& file, void* buffer, size_t size);
NAUI_API size_t naui_fs_write(NauiFile& file, const void* buffer, size_t size);

NAUI_API std::string naui_fs_read_text(const std::filesystem::path& path);

NAUI_API bool naui_fs_write_text(const std::filesystem::path& path, const std::string& text);

NAUI_API void naui_fs_normalize_path(char* path);
NAUI_API const char* naui_path_get_parent(const char* path);
NAUI_API const char* naui_fs_get_executable_path();
NAUI_API const char* naui_fs_get_bin_directory();
NAUI_API const char* naui_fs_get_workspace_path();
NAUI_API const char* naui_fs_get_working_directory();
NAUI_API void        naui_fs_set_workspace_path(const char* path);