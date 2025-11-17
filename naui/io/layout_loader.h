#pragma once
#include "../base.h"
#include "../fs/ini.h"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#define INI_EXTENSION        ".ini"
#define IMMUTABLE            "Immutable"
#define LAYOUT_FOLDER_PATH   "Layouts"

NAUI_API std::filesystem::path naui_get_layout_path();

NAUI_API void naui_save_layout_deferred(std::string filename);
NAUI_API void naui_load_layout_deferred(std::string filename);
NAUI_API bool naui_save_layout(std::string filename);
NAUI_API std::optional<NauiIni> naui_load_layout(std::string filename);
NAUI_API bool naui_delete_layout(std::string filename);

NAUI_API bool naui_layout_has_section(const char* section, const char* key = nullptr, const NauiIni* data = nullptr);
NAUI_API bool naui_layout_is_immutable(const NauiIni* data = nullptr);
NAUI_API std::vector<std::string> naui_layout_list();