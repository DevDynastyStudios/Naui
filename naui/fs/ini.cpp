#include "ini.h"
#include <fstream>
#include <cctype>

#define NAUI_INI_GLOBAL_SECTION "Global"

template<typename T>
static inline std::string naui_join_array(const std::vector<T>& values)
{
    std::string joined;
    for (size_t i = 0; i < values.size(); ++i) 
	{
        if (i > 0) 
			joined += ", ";

        joined += std::to_string(values[i]);
    }

    return joined;
}

template<>
inline std::string naui_join_array<std::string>(const std::vector<std::string>& values)
{
    std::string joined;
    for (size_t i = 0; i < values.size(); ++i) 
	{
        if (i > 0) 
			joined += ", ";

        joined += values[i];
    }

    return joined;
}

template<typename T>
static inline std::string naui_join_matrix(const std::vector<std::vector<T>>& matrix)
{
    std::string joined;
    for (size_t r = 0; r < matrix.size(); ++r) 
	{
        if (r > 0) 
			joined += ";";

        for (size_t c = 0; c < matrix[r].size(); ++c) 
		{
            if (c > 0) 
				joined += ",";

            joined += std::to_string(matrix[r][c]);
        }
    }

    return joined;
}

static inline std::string naui_trim_string(const std::string& s) 
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) 
        return "";

    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static inline std::vector<std::string> naui_split(const std::string& s, char delim)
{
    std::vector<std::string> parts;
    size_t start = 0, end;
    while ((end = s.find(delim, start)) != std::string::npos) 
	{
        parts.push_back(naui_trim_string(s.substr(start, end - start)));
        start = end + 1;
    }

    if (start < s.size())
        parts.push_back(naui_trim_string(s.substr(start)));

    return parts;
}

static inline std::string naui_strip_inline_comment(const std::string& s) 
{
    for (size_t i = 0; i < s.size(); ++i) {
        if ((s[i] == ';' || s[i] == '#') && (i == 0 || isspace((unsigned char)s[i-1]))) {
            return s.substr(0, i);
        }
    }
    return s;
}

static inline std::string naui_unquote(const std::string& s) 
{
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) 
        return s.substr(1, s.size() - 2);

    return s;
}

// Strip UTF-8 BOM if present
static inline void naui_strip_bom(std::string& line) 
{
    if (line.size() >= 3 && 
        (unsigned char)line[0] == 0xEF && 
        (unsigned char)line[1] == 0xBB && 
        (unsigned char)line[2] == 0xBF) 
    {
        line.erase(0, 3);
    }
}

static inline int naui_parse_int(const std::string& s, int def) 
{
    if (s.empty()) 
		return def;

    bool neg = false;
    size_t i = 0;
    if (s[0] == '-') 
	{ 
		neg = true; 
		i = 1; 
	}

    int val = 0;
    for (; i < s.size(); ++i) 
	{
        if (!isdigit((unsigned char)s[i])) 
			return def;

        val = val * 10 + (s[i] - '0');
    }

    return neg ? -val : val;
}

static inline float naui_parse_float(const std::string& s, float def) 
{
    if (s.empty()) 
		return def;

    char* end = nullptr;
    float val = strtof(s.c_str(), &end);
    if (end == s.c_str()) 
		return def;

    return val;
}

bool naui_read_ini(const std::string& filename, NauiIniData& data) 
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) 
        return false;

    std::string line, section = NAUI_INI_GLOBAL_SECTION;
    bool firstLine = true;

    while (std::getline(file, line)) 
    {
        if (firstLine) {
            naui_strip_bom(line);
            firstLine = false;
        }

        line = naui_strip_inline_comment(line);
        line = naui_trim_string(line);
        if (line.empty()) 
            continue;

        if (line.front() == '[' && line.back() == ']') {
            section = naui_trim_string(line.substr(1, line.size() - 2));
            if (section.empty())
                section = NAUI_INI_GLOBAL_SECTION;
        } 
        else 
        {
            size_t eq = line.find('=');
            if (eq == std::string::npos) 
                continue;

            std::string key = naui_trim_string(line.substr(0, eq));
            std::string value = naui_trim_string(line.substr(eq + 1));
            value = naui_unquote(value);
            data[section][key] = value;
        }
    }

    return true;
}

bool naui_write_ini(const std::string& filename, const NauiIniData& data) 
{
    std::ofstream file(filename);
    if (!file.is_open()) 
        return false;

    auto global = data.find(NAUI_INI_GLOBAL_SECTION);
    if (global != data.end()) 
	{
        for (const auto& kv : global->second)
            file << kv.first << " = " << kv.second << "\n";

        file << "\n";
    }

    for (const auto& sec : data) 
    {
        if (sec.first == NAUI_INI_GLOBAL_SECTION) 
            continue;

        file << "[" << sec.first << "]\n";
        for (const auto& kv : sec.second)
            file << kv.first << " = " << kv.second << "\n";

        file << "\n";
    }

    return true;
}

void naui_ini_set_string_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::string>& values)
{
    data[section][key] = naui_join_array(values);
}

void naui_ini_set_int_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<int>& values)
{
    data[section][key] = naui_join_array(values);
}

void naui_ini_set_float_array(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<float>& values)
{
    data[section][key] = naui_join_array(values);
}

void naui_ini_set_int_matrix(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix)
{
    data[section][key] = naui_join_matrix(matrix);
}

void naui_ini_set_float_matrix(NauiIniData& data, const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix)
{
    data[section][key] = naui_join_matrix(matrix);
}

bool naui_ini_has_section(const NauiIniData& data, const std::string& section) 
{
    return data.find(section) != data.end();
}

bool naui_ini_has_key(const NauiIniData& data, const std::string& section, const std::string& key) 
{
    auto it = data.find(section);
    if (it == data.end()) 
        return false;

    return it->second.find(key) != it->second.end();
}

std::string naui_ini_get_string(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def) 
{
    auto it = data.find(section);
    if (it == data.end()) 
        return def;

    auto kv = it->second.find(key);
    return (kv != it->second.end()) ? kv->second : def;
}

int naui_ini_get_int(const NauiIniData& data, const std::string& section, const std::string& key, int def) 
{
    return naui_parse_int(naui_ini_get_string(data, section, key, std::to_string(def)), def);
}

float naui_ini_get_float(const NauiIniData& data, const std::string& section, const std::string& key, float def) 
{
    return naui_parse_float(naui_ini_get_string(data, section, key, ""), def);
}

bool naui_ini_get_bool(const NauiIniData& data, const std::string& section, const std::string& key, bool def) 
{
    std::string v = naui_ini_get_string(data, section, key, def ? "true" : "false");
    for (auto& c : v) 
        c = (char)tolower(c);

    if (v == "1" || v == "true" || v == "yes" || v == "on") 
        return true;

    if (v == "0" || v == "false" || v == "no" || v == "off") 
        return false;

    return def;
}

std::vector<std::string> naui_ini_get_string_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def)
{
    std::string raw = naui_ini_get_string(data, section, key, def);
    std::vector<std::string> result;
    size_t start = 0, end;
    while ((end = raw.find(',', start)) != std::string::npos) 
    {
        result.push_back(naui_trim_string(raw.substr(start, end - start)));
        start = end + 1;
    }

    if (start < raw.size())
        result.push_back(naui_trim_string(raw.substr(start)));

    return result;
}

std::vector<int> naui_ini_get_int_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def)
{
    std::string raw = naui_ini_get_string(data, section, key, def);
    std::vector<int> result;
    for (auto& part : naui_split(raw, ','))
        result.push_back(naui_parse_int(part, 0));

    return result;
}

std::vector<float> naui_ini_get_float_array(const NauiIniData& data, const std::string& section, const std::string& key, const std::string& def)
{
    std::string raw = naui_ini_get_string(data, section, key, def);
    std::vector<float> result;
    for (auto& part : naui_split(raw, ','))
        result.push_back(naui_parse_float(part, 0.0f));

    return result;
}

std::vector<std::vector<int>> naui_ini_get_int_matrix(const NauiIniData& data, const std::string& section, const std::string& key)
{
    std::vector<std::vector<int>> matrix;
    std::string raw = naui_ini_get_string(data, section, key, "");
    for (auto& row : naui_split(raw, ';'))
        matrix.push_back(naui_ini_get_int_array({{"", {{"", row}}}}, "", ""));

    return matrix;
}

std::vector<std::vector<float>> naui_ini_get_float_matrix(const NauiIniData& data, const std::string& section, const std::string& key)
{
    std::vector<std::vector<float>> matrix;
    std::string raw = naui_ini_get_string(data, section, key, "");
    for (auto& row : naui_split(raw, ';'))
        matrix.push_back(naui_ini_get_float_array({{"", {{"", row}}}}, "", ""));

    return matrix;
}