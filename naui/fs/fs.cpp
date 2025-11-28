#include "fs.h"
#include "../base.h"
#include "../platform/platform.h"
#include <fstream>
#include <cstdio>

inline int naui_fopen(FILE** f, const char* filename, const char* mode)
{
#if defined(NAUI_COMPILER_MSVC)
	return fopen_s(f, filename, mode);
#else
	*f = fopen(filename, mode);
	return (*f ? 0 : errno);
#endif
}

inline int naui_strcpy(char* dest, size_t destsz, const char* src)
{
#if defined(NAUI_COMPILER_MSVC)
	return strcpy_s(dest, destsz, src);
#else
	if(!dest || !src)
		return EINVAL;

	strncpy(dest, src, destsz - 1);
	return 0;
#endif
}

inline int naui_strncpy(char* dest, size_t destsz, const char* src, size_t count)
{
#if defined(NAUI_COMPILER_MSVC)
	return strncpy_s(dest, destsz, src, count);
#else
	if(!dest || !src)
		return EINVAL;

	strncpy(dest, src, count);
	return 0;
#endif
}

inline char* naui_getenv(const char* name)
{
#if defined(NAUI_COMPILER_MSVC)
	char* buffer = nullptr;
	size_t len = 0;
	if(_dupenv_s(&buffer, &len, name) == 0 && buffer)
		return buffer;

	return nullptr;
#else
	return getenv(name);
#endif
}

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

void naui_fs_remove_extension(std::string& filename)
{
	size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos)
        filename = filename.substr(0, dotPos);
}

std::vector<std::filesystem::directory_entry> naui_fs_filter(const std::filesystem::path& path, std::string_view name_filter, const std::vector<std::string_view>& allowed_extensions)
{
    std::vector<std::filesystem::directory_entry> results;

    if (!std::filesystem::exists(path))
        return results;

    // Normalize the name filter: strip right-most extension if present
    std::string filter_base{name_filter};
    size_t dotPos = filter_base.find_last_of('.');
    if (dotPos != std::string::npos)
        filter_base = filter_base.substr(0, dotPos);

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;

        std::string filename = entry.path().stem().string();
        if (!filter_base.empty() && filename != filter_base)
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
	// NOTE(smoke): should we use naui_fopen for this?
#if defined(NAUI_COMPILER_MSVC)
	fopen_s(f, path.string().c_str(), m);
#else
	f = fopen(path.string().c_str(), m);
#endif
    //fopen_s(&f, path.string().c_str(), m);
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

static char g_bin_directory[NAUI_PATH_MAX] = {0};
static char g_workspace_path[NAUI_PATH_MAX] = {0};

void naui_fs_normalize_path(char* path)
{
	if(!path)
		return;

	for(char* p = path; *p; ++p)
	{
		if(*p == '\\')
			*p = '/';
	}
}

std::string naui_fs_normalize_path(const std::filesystem::path& path)
{
    std::string s = path.string();
    for (char& c : s) {
        if (c == '\\') c = '/';
    }

    return s;
}

std::string naui_path_get_parent(const std::string path)
{
	if(path.empty())
		return {};

	std::string parent = path;
	size_t pos = parent.find_last_of('/');

#if defined(NAUI_PLATFORM_WINDOWS)
	size_t back = parent.find_last_of('\\');
	if(back != std::string::npos && (pos == std::string::npos || back > pos))
		pos = back;
#endif

	if(pos != std::string::npos)
		parent.erase(pos);

	return parent;
}

std::string naui_fs_get_executable_path()
{
	return naui_get_executable_path();
}

std::string naui_fs_get_working_path()
{
	return naui_get_working_directory();
}

std::string naui_fs_get_home_directory(void)
{
    static char home[NAUI_PATH_MAX];
    size_t len = 0;
	char* env = naui_getenv(NAUI_ENV_USER);
	if(!env)
		env = naui_getenv(NAUI_ENV_HOME);

    if (!env)
        return "";

    naui_strncpy(home, sizeof(home), env, NAUI_TRUNCATE);
    return std::string(home);
}

std::string naui_fs_get_bin_directory()
{
    std::string exe_path = naui_get_executable_path();
    return naui_path_get_parent(exe_path);
}

std::string naui_fs_get_workspace_path()
{
	if(g_workspace_path[0] == '\0')
	{
		naui_strcpy(g_workspace_path, NAUI_PATH_MAX, naui_get_working_directory().c_str());
		g_workspace_path[sizeof(g_workspace_path) - 1] = '\0';
	}

	return g_workspace_path;
}

void naui_fs_set_workspace_path(const char* path)
{
	if(!path || !*path)
	{
		g_workspace_path[0] = '\0';
		return;
	}

	naui_strcpy(g_workspace_path, NAUI_PATH_MAX, path);
	g_workspace_path[sizeof(g_workspace_path) - 1] = '\0';
	naui_fs_normalize_path(g_workspace_path);
}
