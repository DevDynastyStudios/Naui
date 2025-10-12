#pragma once

#include <base.h>

#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::string>> IniData;

NAUI_API bool naui_read_ini(const std::string &filename, IniData &data);
NAUI_API bool naui_write_ini(const std::string &filename, const IniData &data);