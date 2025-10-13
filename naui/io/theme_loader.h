#pragma once

#include "base.h"
#include <imgui.h>

NAUI_API void naui_load_theme_from_json(const char* path);
NAUI_API ImColor naui_get_theme_color(const char* name);