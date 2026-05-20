#pragma once

#include "base.h"
#include "utils/map.h"
#include "renderer/renderer.h"

typedef struct
{
    char *key;
    Naui_Image value;
}
Naui_ImageHashEntry;

NAUI_API Naui_Map(Naui_ImageHashEntry) naui_asset_manager_load_images(const char *const images_path);
NAUI_API void naui_asset_manager_free(void);
NAUI_API const Naui_Image *naui_get_image(const char *const name);
