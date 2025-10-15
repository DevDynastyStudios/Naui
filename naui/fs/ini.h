#pragma once
#include "../base.h"
#include <string>
#include <vector>
#include <unordered_map>

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::string>> NauiIniData;

NAUI_API bool naui_read_ini(const std::string& filename, NauiIniData& data);
NAUI_API bool naui_write_ini(const std::string& filename, const NauiIniData& data);

NAUI_API bool naui_ini_has_section(const NauiIniData& data, const std::string& section);
NAUI_API bool naui_ini_has_key(const NauiIniData& data, const std::string& section, const std::string& key);

NAUI_API void naui_ini_set_string_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::string>& values);
NAUI_API void naui_ini_set_int_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<int>& values);
NAUI_API void naui_ini_set_float_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<float>& values);
NAUI_API void naui_ini_set_int_matrix(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix);
NAUI_API void naui_ini_set_float_matrix(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix);

NAUI_API std::string naui_ini_get_string(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API int naui_ini_get_int(const NauiIniData& data, const std::string& section, const std::string& key, int def);
NAUI_API float naui_ini_get_float(const NauiIniData& data, const std::string& section, const std::string& key, float def);
NAUI_API bool naui_ini_get_bool(const NauiIniData& data, const std::string& section, const std::string& key, bool def);

NAUI_API std::vector<std::string> naui_ini_get_string_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<int> naui_ini_get_int_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<float> naui_ini_get_float_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<std::vector<int>> naui_ini_get_int_matrix(const NauiIniData& data, const std::string& section, const std::string& key);
NAUI_API std::vector<std::vector<float>> naui_ini_get_float_matrix(const NauiIniData& data, const std::string& section, const std::string& key);