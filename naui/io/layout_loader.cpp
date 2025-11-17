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

fs::path naui_get_layout_path()
{
    fs::path bin_dir = naui_fs_get_bin_directory(); // assume returns std::string or fs::path-compatible
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

void naui_save_layout_deferred(std::string filename)
{
	naui_defer(naui_save_layout, filename);
}

void naui_load_layout_deferred(std::string filename)
{
	naui_defer(naui_load_layout, filename);
}

bool naui_save_layout(std::string filename)
{
    fs::path file_path = naui_get_layout_path() / filename;
	if (file_path.extension() != INI_EXTENSION)
    	file_path.replace_extension(INI_EXTENSION);

	std::cout << "Saving layout to: " << file_path << "\n";
    ImGui::SaveIniSettingsToDisk(file_path.string().c_str());
	NauiIni data;
    if (!naui_ini_read(file_path.string().c_str(), data))
        return false;

	for (uint32_t i = 0; i < naui_get_panel_count(); ++i)
    {
        NauiPanelInstance& panel = naui_get_panel(i);
        if (!panel.is_open)
            continue;

		// std::string section = std::string("Window][") + panel.title;
	    // data[section]["DefaultSize"] = std::to_string((int)panel.default_size.x) + "," + std::to_string((int)panel.default_size.y);
        // data[section]["MinSize"]     = std::to_string((int)panel.min_size.x) + "," + std::to_string((int)panel.min_size.y);
        // data[section]["MaxSize"]     = std::to_string((int)panel.max_size.x) + "," + std::to_string((int)panel.max_size.y);
    }

    return naui_ini_write(file_path.string().c_str(), data);
}

std::optional<NauiIni> naui_load_layout(std::string filename)
{
	std::cout << filename << "\n";
    fs::path file_path = naui_get_layout_path() / filename;
    NauiIni data;
    if (!naui_ini_read(file_path.string().c_str(), data))
        return std::nullopt;

	ImGui::LoadIniSettingsFromDisk(file_path.string().c_str());
    // for (auto& [section, kv] : data)
    // {
    //     if (section.rfind("Window][", 0) == 0)
    //     {
    //         std::string title = section.substr(8);
    //         NauiPanelInstance& panel = naui_create_panel(title.c_str(), title.c_str());
    //         auto defSize = naui_ini_get_int_array(data, section, "DefaultSize");
    //         if (defSize.size() == 2)
    //             panel.default_size = ImVec2((float)defSize[0], (float)defSize[1]);
    //         auto minSize = naui_ini_get_int_array(data, section, "MinSize");
    //         if (minSize.size() == 2)
    //             panel.min_size = ImVec2((float)minSize[0], (float)minSize[1]);
    //         auto maxSize = naui_ini_get_int_array(data, section, "MaxSize");
    //         if (maxSize.size() == 2)
    //             panel.max_size = ImVec2((float)maxSize[0], (float)maxSize[1]);
    //     }
    // }

    current_layout = data;
    return data;
}

bool naui_delete_layout(std::string filename)
{
	fs::path dir = naui_get_layout_path();
    const auto& files = naui_fs_filter(dir.string().c_str(), filename, { INI_EXTENSION });

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

std::vector<std::string> naui_layout_list()
{
	std::vector<std::string> layouts;
	fs::path layout_dir = naui_get_layout_path();
	if (!fs::exists(layout_dir))
    	return layouts;

    std::vector<fs::directory_entry> entries =
        naui_fs_filter(layout_dir.string().c_str(), "", { INI_EXTENSION });

    for (auto& entry : entries)
    {
        if (!entry.is_regular_file())
            continue;

        layouts.push_back(entry.path().filename().string());
    }

    return layouts;
}