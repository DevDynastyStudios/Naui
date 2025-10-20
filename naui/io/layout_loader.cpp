#include "layout_loader.h"
#include "../fs/fs.h"
#include "../panel_manager.h"
#include "../util/defer.h"
#include <filesystem>
#include <imgui.h>

static NauiIniData current_layout;

 void naui_save_layout_deferred(const char* filename)
 {
 	naui_defer(naui_save_layout, filename);
 }

bool naui_save_layout(const char* filename)
{
    ImGui::SaveIniSettingsToDisk(filename);
    NauiIniData data;

	if(!naui_read_ini(filename, data))
		return false;

    for (uint32_t i = 0; i < naui_get_panel_count(); ++i) 
	{
        NauiPanelInstance& panel = naui_get_panel(i);
        if (!panel.is_open)
			continue;

        std::string section = std::string("Window][") + panel.title; 

        data[section]["DefaultSize"] =
            std::to_string((int)panel.default_size.x) + "," +
            std::to_string((int)panel.default_size.y);

        data[section]["MinSize"] =
            std::to_string((int)panel.min_size.x) + "," +
            std::to_string((int)panel.min_size.y);

        data[section]["MaxSize"] =
            std::to_string((int)panel.max_size.x) + "," +
            std::to_string((int)panel.max_size.y);
    }

	return naui_write_ini(filename, data);
}

std::optional<NauiIniData> naui_load_layout(const char* filename)
{
	NauiIniData data;
    if (!naui_read_ini(filename, data))
        return std::nullopt;

    ImGui::LoadIniSettingsFromDisk(filename);

    for (auto& [section, kv] : data) 
	{
        if (section.rfind("Window][", 0) == 0) {
            std::string title = section.substr(8);

            NauiPanelInstance& panel = naui_create_panel(title.c_str(), title.c_str());

            auto defSize = naui_ini_get_int_array(data, section, "DefaultSize");
            if (defSize.size() == 2)
                panel.default_size = ImVec2((float)defSize[0], (float)defSize[1]);

            auto minSize = naui_ini_get_int_array(data, section, "MinSize");
            if (minSize.size() == 2)
                panel.min_size = ImVec2((float)minSize[0], (float)minSize[1]);

            auto maxSize = naui_ini_get_int_array(data, section, "MaxSize");
            if (maxSize.size() == 2)
                panel.max_size = ImVec2((float)maxSize[0], (float)maxSize[1]);
        }
    }

	current_layout = data;
    return data;
}

bool naui_delete_layout(const char* filename)
{
    const auto& files = naui_fs_filter(LAYOUT_FOLDER_PATH, filename, {INI_EXTENSION});
    if (files.empty())
        return false;

    std::filesystem::path path = files[0].path();

    NauiIniData data;
    if (!naui_read_ini(path.string(), data) || naui_layout_is_immutable(&data))
		return false;

    std::error_code ec;
    bool removed = std::filesystem::remove(path, ec);
    if (ec) 
        return false;

    return removed;
}

bool naui_layout_has_section(const char* section, const char* key, const NauiIniData* data)
{
    const NauiIniData* src = data ? data : &current_layout;
    if (!src)
		return false;

    auto it = src->find(section);
    if (it == src->end()) 
		return false;

    if (key)
        return it->second.find(key) != it->second.end();

    return true;
}

bool naui_layout_is_immutable(const NauiIniData* data)
{
    const NauiIniData* src = data ? data : &current_layout;
    if (!src) 
		return false;

    return src->find(IMMUTABLE) != src->end();
}

std::vector<std::string> naui_layout_list(std::filesystem::path path)
{
	std::vector<std::string> layouts;
	if(!naui_fs_exists(path))
		return layouts;

	std::vector<std::filesystem::directory_entry> entries = naui_fs_filter(path, "", {INI_EXTENSION});
	for(std::filesystem::directory_entry& entry : entries)
	{
		if(!entry.is_regular_file())
			continue;

		layouts.push_back(entry.path().string());
	}

	return layouts;
}

/*#include "layout_manager.h"
#include "panel_manager.h"
#include <imgui.h>
#include <fstream>
#include <filesystem>
#include <ini.h>
#include <deque>
#include <string>
#include <vector>

#define LAYOUT_IDENTIFIER     "NauiPanels"
#define IMMUTABLE_IDENTIFIER  "Immutable"
#define INI_EXTENSION         ".ini"

// -----------------------------------------------------------------------------
// Immediate operations (existing API)
// -----------------------------------------------------------------------------

// Yes, I know about the flag bug with already existing ini files
void naui_save_layout(const char* name) {
    std::string filename = std::string(name) + INI_EXTENSION;

    // Save ImGui window/dock state
    ImGui::SaveIniSettingsToDisk(filename.c_str());

    // Append our panel visibility section
    std::ofstream out(filename, std::ios::app);
    out << "\n[" << LAYOUT_IDENTIFIER << "]\n";

    // for (auto& panel : panels()) {
    //     if (panel.window_flags & ImGuiWindowFlags_NoSavedSettings)
    //         continue;

    //     out << panel.title << (panel.is_visible ? "=1\n" : "=0\n");
    // }
}

bool naui_load_layout(const char* name) {
    std::string filename = std::string(name) + INI_EXTENSION;
    if (!std::filesystem::exists(filename))
        return false;

    // Load ImGui window/dock state
    ImGui::LoadIniSettingsFromDisk(filename.c_str());

    // Apply our panel visibility section
    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    file.read(ini);

    auto& section = ini[LAYOUT_IDENTIFIER];
    // for (NauiPanel& panel : panels()) {
    //     if (!section.has(panel.title)) {
    //         panel.is_visible = false;
    //         continue;
    //     }
    //     panel.is_visible = (section[panel.title] == "1");
    // }

    return true;
}

bool naui_remove_layout(const char* name) {
    //if (naui_layout_has_header(name, IMMUTABLE_IDENTIFIER))
        //return false;

    std::string filename = std::string(name) + INI_EXTENSION;
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
        return true;
    }
    return false;
}

bool naui_layout_has_header(const char* name, const char* header) {
    std::string filename = std::string(name) + INI_EXTENSION;
    if (!std::filesystem::exists(filename))
        return false;

    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    if (!file.read(ini))
        return false;

    return ini.has(header);
}

bool naui_layout_is_immutable(const char* name) {
    return naui_layout_has_header(name, IMMUTABLE_IDENTIFIER);
}

std::vector<std::string> naui_list_layouts(const char* directory) {
    std::vector<std::string> layouts;
    namespace fs = std::filesystem;

    if (!fs::exists(directory))
        return layouts;

    for (auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file())
            continue;

        auto path = entry.path();
        if (path.extension() == INI_EXTENSION)
            layouts.push_back(path.stem().string());
    }
    return layouts;
}

// -----------------------------------------------------------------------------
// Deferred queue (new API)
// -----------------------------------------------------------------------------

enum class NauiLayoutOpType {
    Save,
    Load
};

struct NauiLayoutOp {
    NauiLayoutOpType type;
    std::string name;
};

static std::deque<NauiLayoutOp> g_layout_ops;

// Queue a save request (name is base path without .ini)
void naui_layout_request_save(const char* name) {
    g_layout_ops.push_back({NauiLayoutOpType::Save, name});
}

// Queue a load request (name is base path without .ini)
void naui_layout_request_load(const char* name) {
    g_layout_ops.push_back({NauiLayoutOpType::Load, name});
}

// Optional helper: clear pending requests
void naui_layout_clear_requests() {
    g_layout_ops.clear();
}

void naui_layout_process_requests() {
    while (!g_layout_ops.empty()) {
        const NauiLayoutOp op = g_layout_ops.front();
        g_layout_ops.pop_front();

        switch (op.type) {
            case NauiLayoutOpType::Save:
                naui_save_layout(op.name.c_str());
                break;
            case NauiLayoutOpType::Load:
                naui_load_layout(op.name.c_str());
                break;
        }
    }
}*/