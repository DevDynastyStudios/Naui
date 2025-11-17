#pragma once
#include "../base.h"
#include <string>
#include <vector>
#include <filesystem>

#define NAUI_GLOBAL_SECTION "NAUI_GLOBAL"

struct NauiIniNode 
{
	std::string key;
	std::string value;
	std::vector<NauiIniNode> children;
};

struct NauiIniSection 
{
    std::string name;
    std::vector<NauiIniNode> nodes;

    bool is_comment() const {
        if (name.empty()) 
			return false;

        static const char markers[] = { ';', '#' };
        for (const auto& m : markers) {
            if (name.rfind(m, 0) == 0) {
                return true;
            }
        }

        return false;
    }

	bool has_key(const char* key) const {
		for(int i = 0; i < nodes.size(); i++) {
			if(nodes[i].key == key)
				return true;
		}

		return false;
	}

	bool is_empty() const {
		return (name.empty() && nodes.empty());
	}

};

struct NauiIni
{
	std::vector<NauiIniSection> sections;
	uint8_t index_space = 0;

	NauiIniSection& operator[](const std::string& sectionName) {
        for (auto& sec : sections) {
            if (!sec.is_comment() && sec.name == sectionName)
                return sec;
        }

        sections.push_back({sectionName, {}});
        return sections.back();
    }

	bool has_section(const char* section) const {
		for(int i = 0; i < sections.size(); i++) {
			if(sections[i].name == section)
				return true;
		}

		return false;
	}
};

NAUI_API bool naui_ini_read(const std::filesystem::path& filename, NauiIni& data);
NAUI_API bool naui_ini_write(const std::filesystem::path& filename, NauiIni& data, int indent_space = 2);

NAUI_API bool naui_ini_has_section(const NauiIni& data, const std::string& section);
NAUI_API bool naui_ini_has_key(const NauiIni& data, const std::string& section, const std::string& key);

NAUI_API void naui_ini_set_string_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::string>& values);
NAUI_API void naui_ini_set_int_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<int>& values);
NAUI_API void naui_ini_set_float_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<float>& values);
NAUI_API void naui_ini_set_int_matrix(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix);
NAUI_API void naui_ini_set_float_matrix(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix);

NAUI_API std::string naui_ini_get_string(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API int naui_ini_get_int(const NauiIni& data, const std::string& section, const std::string& key, int def);
NAUI_API float naui_ini_get_float(const NauiIni& data, const std::string& section, const std::string& key, float def);
NAUI_API bool naui_ini_get_bool(const NauiIni& data, const std::string& section, const std::string& key, bool def);

NAUI_API std::vector<std::string> naui_ini_get_string_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<int> naui_ini_get_int_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<float> naui_ini_get_float_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def = "");
NAUI_API std::vector<std::vector<int>> naui_ini_get_int_matrix(const NauiIni& data, const std::string& section, const std::string& key);
NAUI_API std::vector<std::vector<float>> naui_ini_get_float_matrix(const NauiIni& data, const std::string& section, const std::string& key);