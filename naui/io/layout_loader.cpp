#include "layout_loader.h"
#include "../fs/fs.h"
#include "../panel_manager.h"
#include "../util/defer.h"

#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <optional>
#include <vector>
#include <string>

namespace fs = std::filesystem;
static NauiIni current_layout;
std::vector<NauiLayoutInfo> layout_cache;

fs::path naui_get_layout_path()
{
    fs::path bin_dir = naui_fs_get_bin_directory();
    fs::path layouts_dir = bin_dir / LAYOUT_FOLDER_PATH;

    if (!fs::exists(layouts_dir)) {
        std::error_code ec;
        fs::create_directories(layouts_dir, ec);
        if (ec) {
            std::cerr << "Failed to create Layouts directory: " << ec.message() << "\n";
        }
    }

	return layouts_dir;
}

void naui_layout_save_deferred(const std::string& filename)
{
    naui_defer([&](const std::string& f){ naui_layout_save(f); }, filename);
}

void naui_layout_load_deferred(const std::string& filename)
{
    naui_defer([&](const std::string& f){ naui_layout_load(f); }, filename);
}

bool naui_layout_save(const std::string& filename)
{
	NauiIni data;
	return naui_layout_save(filename, data);
}

bool naui_layout_save(const std::string& filename, NauiIni& data)
{
    fs::path file_path = naui_get_layout_path() / filename;
	if (file_path.extension() != INI_EXTENSION)
    	file_path.replace_extension(INI_EXTENSION);

	std::cout << "Saving layout: " << file_path << "\n";
    ImGui::SaveIniSettingsToDisk(file_path.string().c_str());

    if (!naui_ini_read(file_path.string().c_str(), data))
        return false;

	for (uint32_t i = 0; i < naui_get_panel_count(); ++i)
    {
        NauiPanelInstance& panel = naui_get_panel(i);
        if (!panel.is_open)
            continue;
    }

	bool result = naui_ini_write(file_path.string().c_str(), data);
	naui_layout_refresh_cache();
    return result;
}

std::optional<NauiIni> naui_layout_load(const std::string& filename)
{
	NauiIni data;
	return naui_layout_load(filename, data);
}

std::optional<NauiIni> naui_layout_load(const std::string& filename, NauiIni& data)
{
    fs::path file_path = naui_get_layout_path() / (filename + INI_EXTENSION);

    if (!naui_ini_read(file_path.string().c_str(), data))
        return std::nullopt;

	ImGui::LoadIniSettingsFromDisk(file_path.string().c_str());

	std::cout << "Loaded layout: " << file_path << "\n";
    current_layout = data;
    return data;
}

bool naui_layout_delete(const std::string& filename)
{
	fs::path dir = naui_get_layout_path();
    const std::vector<fs::directory_entry>& files = naui_fs_filter(dir.string().c_str(), filename, { INI_EXTENSION });

    if (files.empty())
        return false;

	fs::path path = files[0].path();
    NauiIni data;

    if (!naui_ini_read(path.string().c_str(), data) || naui_layout_is_immutable(&data))
        return false;

    std::error_code ec;
    bool removed = fs::remove(path, ec);
    if (ec)
        return false;

	std::cout << "Deleted layout: " << path << "\n";
	naui_layout_refresh_cache();
    return removed;
}

bool naui_layout_has_section(const char* section, const char* key, const NauiIni* data)
{
    const NauiIni* src = data ? data : &current_layout;
    if (!src)
        return false;

	return src->has_section(section);
}

bool naui_layout_is_immutable(const NauiIni* data)
{
    const NauiIni* src = data ? data : &current_layout;
    if (!src)
        return false;

	return naui_ini_has_section(*src, IMMUTABLE);
}

// Returns the path of all files in the layout directory
std::vector<fs::path> naui_layout_paths()
{
    std::vector<fs::path> layouts;
    fs::path layout_dir = naui_get_layout_path();

    if (!fs::exists(layout_dir))
        return layouts;

    std::vector<fs::directory_entry> entries =
        naui_fs_filter(layout_dir.string().c_str(), "", { INI_EXTENSION });

    for (auto& entry : entries)
    {
        if (!entry.is_regular_file())
            continue;

        layouts.push_back(entry.path());
    }

    return layouts;
}

// Returns the path as a string of all files in the layout directory
std::vector<std::string> naui_layout_paths_str()
{
    std::vector<std::string> result;
    std::vector<fs::path> paths = naui_layout_paths();

    for (const fs::path& p : paths)
        result.push_back(p.string());

    return result;
}

// Takes a path and returns the file name at the end
std::string naui_layout_filename(const std::string& full_path)
{
    return fs::path(full_path).filename().stem().string();
}

// Refreshes the cached std::vector<LayoutInfo> layout_cache
void naui_layout_refresh_cache()
{
    layout_cache.clear();
    std::vector<fs::path> paths = naui_layout_paths();

    for (const fs::path& p : paths)
    {
        NauiIni data;
        bool immutable = false;
        if (naui_ini_read(p, data))
            immutable = naui_layout_is_immutable(&data);

        layout_cache.push_back({
            .filename = p.filename().stem().string(),
            .file_path = p,
            .immutable = immutable
        });
    }
}