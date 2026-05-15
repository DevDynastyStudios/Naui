#pragma once

#include "utils/map.h"
#include "renderer/renderer.h"

typedef struct
{
    char *key;
    Naui_Image value;
}
Naui_ImageHashEntry;

Naui_Map(Naui_ImageHashEntry) naui_asset_manager_load_images(const char *const images_path);
void naui_asset_manager_free(void);
const Naui_Image *naui_get_image(const char *const name);
