#pragma once

#include "../base.h"
#include <filesystem>
#include <cstdint>

struct NauiArchive;
struct NauiArchiveEntry
{
    std::filesystem::path path;
    uint64_t size;
    bool is_directory;
};

enum class NauiArchiveMode
{
    Read,
    Write
};

NAUI_API NauiArchive* naui_archive_open(const std::filesystem::path& file, NauiArchiveMode mode);
NAUI_API void naui_archive_close(NauiArchive* archive);
NAUI_API bool naui_archive_add_file(NauiArchive* archive, const std::filesystem::path& source, const std::filesystem::path& dest_in_archive);
NAUI_API bool naui_archive_extract_file(NauiArchive* archive, const std::filesystem::path& entry, const std::filesystem::path& dest);
NAUI_API bool naui_archive_create(const std::filesystem::path& folder, const std::filesystem::path& archivePath);
NAUI_API bool naui_archive_extract(const std::filesystem::path& archivePath, const std::filesystem::path& outputFolder);
NAUI_API std::vector<NauiArchiveEntry> naui_archive_list(NauiArchive* archive);