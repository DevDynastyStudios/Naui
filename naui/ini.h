#pragma once

#include "base.h"

#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::string>> NauiIniData;

NAUI_API bool naui_read_ini(const std::string &filename, NauiIniData &data);
NAUI_API bool naui_write_ini(const std::string &filename, const NauiIniData &data);