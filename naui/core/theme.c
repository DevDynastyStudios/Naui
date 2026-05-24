#include "themes.h"
#include "utils/map.h"

typedef struct
{
    char *key;
    float value;
}
Naui_ThemeFloatEntry;

typedef struct
{
    char *key;
    Naui_Color value;
}
Naui_ThemeColorEntry;

typedef struct
{
    Naui_Map(Naui_ThemeColorEntry) color_map;
    Naui_Map(Naui_ThemeFloatEntry) float_map;
}
Naui_ThemeData;
static Naui_ThemeData data;

void naui_load_theme(const char *file_name)
{
    // Fuck we need jsons CHIMPCHI HURRY TF UP

    // Anyway here are some hardcoded values for now :3
    naui_strmap_put(data.color_map, "naui_panel_bg_col", naui_color_rgba(255, 100, 100, 255));
    naui_strmap_put(data.color_map, "naui_panel_title_bg_col", naui_color_rgba(255, 150, 150, 255));
    naui_strmap_put(data.color_map, "naui_panel_title_text", naui_color_rgba(255, 255, 255, 255));
    naui_strmap_put(data.color_map, "naui_panel_text", naui_color_rgba(255, 255, 255, 255));
    naui_strmap_put(data.color_map, "naui_panel_text_disabled", naui_color_rgba(150, 150, 150, 255));
}

Naui_Color naui_theme_color(const char *name)
{
    return naui_strmap_get(data.color_map, name);
}

float naui_theme_float(const char *name)
{
    return naui_strmap_get(data.float_map, name);
}