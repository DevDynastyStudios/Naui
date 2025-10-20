#pragma once
#include "../base.h"
#include "../fs/ini.h"
#include <string>
#include <filesystem>
#include <optional>

#define INI_EXTENSION ".ini"
#define IMMUTABLE "Immutable"
#define LAYOUT_FOLDER_PATH "layouts/"

NAUI_API bool naui_save_layout(const char* filename);
NAUI_API std::optional<NauiIniData> naui_load_layout(const char* filename);
NAUI_API bool naui_delete_layout(const char* filename);
NAUI_API bool naui_layout_has_section(const char* section, const char* key, const NauiIniData* data = nullptr);
NAUI_API bool naui_layout_is_immutable(const NauiIniData* data = nullptr);
NAUI_API std::vector<std::string> naui_layout_list(std::filesystem::path path);