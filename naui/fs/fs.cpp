#include "fs.h"
#include <fstream>
#include <cstdio>

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

std::vector<NauiDirEntry> naui_fs_list(const std::filesystem::path& path)
{
    std::vector<NauiDirEntry> entries;
    if (!std::filesystem::exists(path)) 
		return entries;

    for (auto& p : std::filesystem::directory_iterator(path))
    {
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