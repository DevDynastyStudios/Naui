#include "fs.h"
#include <fstream>
#include <cstdio>

std::string to_lower(std::string_view str)
{
    std::string result;
    result.reserve(str.size());

    for (char c : str)
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    return result;
}

inline bool contains_filter(std::string_view name, std::string_view filter)
{
    if (filter.empty())
        return true;

    auto to_lower = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };

    return std::search(
        name.begin(), name.end(),
        filter.begin(), filter.end(),
        [&](char a, char b) { return to_lower(a) == to_lower(b); }) != name.end();
}

bool matches_extension(const std::filesystem::path& path, const std::vector<std::string_view>& allowed_exts)
{
    if (allowed_exts.empty())
        return true;

    const std::string ext = to_lower(path.extension().string());
    for (const auto& allowed : allowed_exts)
        if (ext == to_lower(allowed))
            return true;

    return false;
}

bool naui_fs_exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

bool naui_fs_is_directory(const std::filesystem::path& path)
{
    return std::filesystem::is_directory(path);
}

bool naui_fs_create_directory(const std::filesystem::path& path)
{
    return std::filesystem::create_directories(path);
}

std::vector<std::filesystem::directory_entry> naui_fs_filter(const std::filesystem::path& path, std::string_view name_filter, const std::vector<std::string_view>& allowed_extensions)
{
    std::vector<std::filesystem::directory_entry> results;

    if (!std::filesystem::exists(path))
        return results;

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;

        const auto& filename = entry.path().stem().string();

        if (!contains_filter(filename, name_filter))
            continue;

        if (!matches_extension(entry.path(), allowed_extensions))
            continue;

        results.push_back(entry);
    }

    return results;
}

NauiFile naui_fs_open(const std::filesystem::path& path, NauiFileMode mode)
{
    const char* m = nullptr;
    switch (mode)
    {
        case NauiFileMode::Read:   m = "rb"; break;
        case NauiFileMode::Write:  m = "wb"; break;
        case NauiFileMode::Append: m = "ab"; break;
    }

    FILE* f = nullptr;
    fopen_s(&f, path.string().c_str(), m);
    return { f };
}

void naui_fs_close(NauiFile& file)
{
    if (file.handle)
    {
        fclose((FILE*)file.handle);
        file.handle = nullptr;
    }
}

size_t naui_fs_read(NauiFile& file, void* buffer, size_t size)
{
    if (!file.handle) 
		return 0;

    return fread(buffer, 1, size, (FILE*)file.handle);
}

size_t naui_fs_write(NauiFile& file, const void* buffer, size_t size)
{
    if (!file.handle) 
		return 0;

    return fwrite(buffer, 1, size, (FILE*)file.handle);
}

std::vector<NauiDirEntry> naui_fs_list(const std::filesystem::path& path, std::string_view name_filter, std::vector<std::string_view> allowed_extensions)
{
    std::vector<NauiDirEntry> entries;

    if (!std::filesystem::exists(path))
        return entries;

    std::string name_filter_lower = to_lower(name_filter);

    for (const auto& p : std::filesystem::directory_iterator(path))
    {
        const std::string filename = p.path().filename().string();

        if (!name_filter_lower.empty() &&
            to_lower(filename).find(name_filter_lower) == std::string::npos)
            continue;

        if (!matches_extension(p.path(), allowed_extensions))
            continue;

        NauiDirEntry e;
        e.path = p.path();
        e.is_directory = p.is_directory();
        e.size = e.is_directory ? 0 : std::filesystem::file_size(p.path());

        entries.push_back(std::move(e));
    }

    return entries;
}


std::string naui_fs_read_text(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs) 
		return {};

    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

bool naui_fs_write_text(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream ofs(path, std::ios::out | std::ios::binary);
    if (!ofs) 
		return false;

    ofs.write(text.data(), text.size());
    return true;
}