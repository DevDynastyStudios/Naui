#include "ini.h"
#include <fstream>

static std::string naui_trim_string(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool naui_read_ini(const std::string &filename, NauiIniData &data)
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line, section;
    while (getline(file, line))
    {
        line = naui_trim_string(line);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        if (line.front() == '[' && line.back() == ']')
        {
            section = naui_trim_string(line.substr(1, line.size() - 2));
        }
        else
        {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = naui_trim_string(line.substr(0, eq));
            std::string value = naui_trim_string(line.substr(eq + 1));
            data[section][key] = value;
        }
    }
    return true;
}

bool naui_write_ini(const std::string &filename, const NauiIniData &data)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    for (const auto &sec : data)
    {
        if (!sec.first.empty())
            file << "[" << sec.first << "]\n";
        for (const auto &kv : sec.second)
            file << kv.first << " = " << kv.second << "\n";
        file << "\n";
    }
    return true;
}