#pragma once

#include "utils/map.h"
#include "renderer/renderer.h"

typedef struct
{
    char *key;
    Naui_Image value;
}
Naui_ImageHashEntry;

Naui_StrMap(Naui_ImageHashEntry) naui_asset_manager_load_images(char *const images_path);
void naui_asset_manager_free(void);
Naui_Image naui_get_image(char *const name);
