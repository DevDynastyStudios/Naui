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