#include "ini.h"
#include <fstream>
#include <sstream>

#pragma region Helpers
NauiIniSection naui_ini_get_section(const NauiIni& data, std::string section)
{
	for(const NauiIniSection& sec : data.sections)
	{
		if(sec.is_comment() || sec.name != section)
			continue;

		return sec;
	}

	return {};
}

NauiIniNode naui_ini_get_key_pair(const NauiIniSection& section, std::string key)
{
	for(const NauiIniNode& node : section.nodes)
	{
		if(node.key != key)
			continue;

		return node;
	}

	return {};
}

bool naui_ini_has_section(const NauiIni& data, const std::string& section) 
{
    NauiIniSection result = naui_ini_get_section(data, section);
    if (result.name.empty())
        return false;
		
    if (result.is_comment())
        return false;

    return true;
}

bool naui_ini_has_key(const NauiIniSection& section, const std::string& key) 
{
	if(section.is_comment() || section.is_empty())
		return false;

	NauiIniNode node = naui_ini_get_key_pair(section, key);
    return node.key == key;
}

bool naui_ini_has_key(const NauiIni& data, const std::string& section, const std::string& key) 
{
	NauiIniSection sec = naui_ini_get_section(data, section);
	return naui_ini_has_key(sec, key);
}

#pragma endregion

#pragma region Setters
void naui_ini_set_string_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::string>& values) 
{
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) 
			joined += ",";

        joined += values[i];
    }

    data[section].nodes.push_back({key, joined, {}});
}

void naui_ini_set_int_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<int>& values) 
{
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) 
			joined += ",";

        joined += std::to_string(values[i]);
    }

    data[section].nodes.push_back({key, joined, {}});
}

void naui_ini_set_float_array(NauiIni& data, const std::string& section, const std::string& key, const std::vector<float>& values) 
{
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) 
			joined += ",";

        joined += std::to_string(values[i]);
    }

    data[section].nodes.push_back({key, joined, {}});
}

void naui_ini_set_int_matrix(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix) 
{
    std::string joined;
    for (std::size_t r = 0; r < matrix.size(); ++r) {
        if (r > 0) 
			joined += ";";

        for (std::size_t c = 0; c < matrix[r].size(); ++c) {
            if (c > 0) 
				joined += ",";

            joined += std::to_string(matrix[r][c]);
        }
    }
	
    data[section].nodes.push_back({key, joined, {}});
}

void naui_ini_set_float_matrix(NauiIni& data, const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix) 
{
    std::string joined;
    for (std::size_t r = 0; r < matrix.size(); ++r) {
        if (r > 0) 
			joined += ";";

        for (std::size_t c = 0; c < matrix[r].size(); ++c) {
            if (c > 0) 
				joined += ",";

            joined += std::to_string(matrix[r][c]);
        }
    }

    data[section].nodes.push_back({key, joined, {}});
}

#pragma endregion

#pragma region Getters
std::string naui_ini_get_string(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def) 
{
    for (const NauiIniSection& sec : data.sections) {
		if(sec.is_comment() || sec.name != section)
			continue;

        for (const NauiIniNode& node : sec.nodes) {
            if (node.key == key)
                return node.value;
        }
    }

    return def;
}

int naui_ini_get_int(const NauiIni& data, const std::string& section, const std::string& key, int def) 
{
    std::string val = naui_ini_get_string(data, section, key, "");
    if (val.empty()) 
		return def;

    try { 
		return std::stoi(val); 
	} catch (...) { 
		return def; 
	}
}

float naui_ini_get_float(const NauiIni& data, const std::string& section, const std::string& key, float def) 
{
    std::string val = naui_ini_get_string(data, section, key, "");
    if (val.empty()) 
		return def;

    try { 
		return std::stof(val);
	} catch (...) { 
		return def; 
	}
}

bool naui_ini_get_bool(const NauiIni& data, const std::string& section, const std::string& key, bool def) 
{
    std::string val = naui_ini_get_string(data, section, key, "");
    if (val.empty()) 
		return def;

    if (val == "true" || val == "1") 
		return true;

    if (val == "false" || val == "0") 
		return false;

    return def;
}

#pragma endregion

#pragma region Array Getters

std::vector<std::string> naui_ini_get_string_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def) 
{
    std::string val = naui_ini_get_string(data, section, key, def);
    std::vector<std::string> out;
    std::size_t start = 0;
    std::size_t pos;
    while ((pos = val.find(',', start)) != std::string::npos) {
        out.push_back(val.substr(start, pos - start));
        start = pos + 1;
    }

    if (!val.empty())
        out.push_back(val.substr(start));

    return out;
}

std::vector<int> naui_ini_get_int_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def) 
{
    std::vector<std::string> vals = naui_ini_get_string_array(data, section, key, def);
    std::vector<int> out;
    for (const std::string& s : vals) {
        try { 
			out.push_back(std::stoi(s)); 
		} catch (...) {}
    }

    return out;
}

std::vector<float> naui_ini_get_float_array(const NauiIni& data, const std::string& section, const std::string& key, const std::string& def) 
{
    std::vector<std::string> vals = naui_ini_get_string_array(data, section, key, def);
    std::vector<float> out;
    for (const std::string& s : vals) {
        try { 
			out.push_back(std::stof(s)); 
		} catch (...) {}
    }

    return out;
}

#pragma endregion

#pragma region Matrix Getters

std::vector<std::vector<int>> naui_ini_get_int_matrix(const NauiIni& data, const std::string& section, const std::string& key) 
{
    std::string val = naui_ini_get_string(data, section, key, "");
    std::vector<std::vector<int>> out;

    std::size_t rowStart = 0;
    std::size_t rowPos;

    while ((rowPos = val.find(';', rowStart)) != std::string::npos) {
        std::string rowStr = val.substr(rowStart, rowPos - rowStart);
        rowStart = rowPos + 1;

        std::vector<int> row;
        std::size_t colStart = 0;
        std::size_t colPos;

        while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
            row.push_back(std::stoi(rowStr.substr(colStart, colPos - colStart)));
            colStart = colPos + 1;
        }

        if (!rowStr.empty())
            row.push_back(std::stoi(rowStr.substr(colStart)));

        out.push_back(row);
    }

    if (!val.empty() && rowStart < val.size()) {
        std::string rowStr = val.substr(rowStart);
        std::vector<int> row;
        std::size_t colStart = 0;
        std::size_t colPos;

        while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
            row.push_back(std::stoi(rowStr.substr(colStart, colPos - colStart)));
            colStart = colPos + 1;
        }

        if (colStart < rowStr.size())
            row.push_back(std::stoi(rowStr.substr(colStart)));

        out.push_back(row);
    }

    return out;
}

std::vector<std::vector<float>> naui_ini_get_float_matrix(const NauiIni& data, const std::string& section, const std::string& key) 
{
    std::string val = naui_ini_get_string(data, section, key, "");
    std::vector<std::vector<float>> out;

    std::size_t rowStart = 0;
    std::size_t rowPos;

    while ((rowPos = val.find(';', rowStart)) != std::string::npos) {
        std::string rowStr = val.substr(rowStart, rowPos - rowStart);
        rowStart = rowPos + 1;

        std::vector<float> row;
        std::size_t colStart = 0;
        std::size_t colPos;

        while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
            row.push_back(std::stof(rowStr.substr(colStart, colPos - colStart)));
            colStart = colPos + 1;
        }

        if (!rowStr.empty())
            row.push_back(std::stof(rowStr.substr(colStart)));

        out.push_back(row);
    }

    if (!val.empty() && rowStart < val.size()) {
        std::string rowStr = val.substr(rowStart);
        std::vector<float> row;
        std::size_t colStart = 0;
        std::size_t colPos;

        while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
            row.push_back(std::stof(rowStr.substr(colStart, colPos - colStart)));
            colStart = colPos + 1;
        }

        if (colStart < rowStr.size())
            row.push_back(std::stof(rowStr.substr(colStart)));

        out.push_back(row);
    }

    return out;
}

#pragma endregion

#pragma region Parser

static void naui_ini_write_node(std::ofstream& out, const NauiIniNode& node, int indent_space, int depth) {
	out << std::string(indent_space * depth, ' ') << node.key << "=" << node.value << "\n";
	for(const NauiIniNode& child : node.children) {
		naui_ini_write_node(out, child, indent_space, depth + 1);
	}
}

bool naui_ini_write(const std::filesystem::path& path, NauiIni& ini, int indent_space)
{
	std::ofstream out(path);
    if (!out.is_open())
        return false;

    for (const NauiIniSection& sec : ini.sections) {
		if(sec.is_empty()) {
			out << "\n";
			continue;
		}

        if (sec.is_comment()) {
            out << sec.name << "\n";
            continue;
        }

		const char* section = NAUI_GLOBAL_SECTION;
		if(sec.nodes.size() > 0 && sec.name != "") {
			section = sec.name.c_str();
		}

		out << "[" << section << "]\n";

        for (const NauiIniNode& node : sec.nodes) {
            naui_ini_write_node(out, node, indent_space, 0);
        }

		out << "\n";
    }

    return true;
}

static std::size_t naui_ini_count_indent(const std::string& line) {
    std::size_t count = 0;
    while (count < line.size() && (line[count] == ' ' || line[count] == '\t'))
        ++count;

    return count;
}

bool naui_ini_read(const std::filesystem::path& filename, NauiIni& data) {
    std::ifstream in(filename);
    if (!in.is_open())
        return false;

    data.sections.clear();
    NauiIniSection* currentSection = nullptr;

    struct Frame {
        NauiIniNode* node;
        std::size_t indent;
    };

    std::vector<Frame> stack;
    std::string line;
    while (std::getline(in, line)) {
        std::size_t end = line.find_last_not_of(" \t\r\n");
        if (end == std::string::npos) 
			continue;

        line = line.substr(0, end + 1);
        if (line.empty()) 
			continue;

        if (line[0] == ';' || line[0] == '#') {
            data.sections.push_back({line, {}});
            currentSection = nullptr;
            stack.clear();
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            std::string sectionName = line.substr(1, line.size() - 2);
            data.sections.push_back({sectionName, {}});
            currentSection = &data.sections.back();
            stack.clear();
            continue;
        }

        std::size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) 
			continue;

        std::size_t indent = naui_ini_count_indent(line);
        std::string key = line.substr(indent, eqPos - indent);
        std::string value = line.substr(eqPos + 1);

        std::size_t keyStart = key.find_first_not_of(" \t");
        std::size_t keyEnd   = key.find_last_not_of(" \t");
        if (keyStart != std::string::npos)
            key = key.substr(keyStart, keyEnd - keyStart + 1);

        std::size_t valStart = value.find_first_not_of(" \t");
        std::size_t valEnd   = value.find_last_not_of(" \t");
        if (valStart != std::string::npos)
            value = value.substr(valStart, valEnd - valStart + 1);

        if (currentSection == nullptr) {
            data.sections.push_back({"", {}});
            currentSection = &data.sections.back();
            stack.clear();
        }

        NauiIniNode newNode{key, value, {}};

        if (stack.empty()) {
            currentSection->nodes.push_back(newNode);
            stack.push_back({&currentSection->nodes.back(), indent});
        } else if (indent > stack.back().indent) {
            stack.back().node->children.push_back(newNode);
            stack.push_back({&stack.back().node->children.back(), indent});
        } else {
            while (!stack.empty() && indent <= stack.back().indent) {
                stack.pop_back();
            }

            if (stack.empty()) {
                currentSection->nodes.push_back(newNode);
                stack.push_back({&currentSection->nodes.back(), indent});
            } else {
                stack.back().node->children.push_back(newNode);
                stack.push_back({&stack.back().node->children.back(), indent});
            }
        }
    }

    return true;
}

#pragma endregion