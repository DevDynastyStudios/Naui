#pragma once

#include "platform/platform.h"

NAUI_API void naui_asset_manager_initialize(const char *assets_directory = "Assets");
NAUI_API void naui_asset_manager_shutdown(void);

NAUI_API NauiImage &naui_get_image(const char *path);