#include "theme.h"
#include "utils/map.h"

typedef struct { char *key; float value; } Naui_ThemeFloatEntry;
typedef struct { char *key; Naui_Vec2 value; } Naui_ThemeVec2Entry;
typedef struct { char *key; Naui_Color value; } Naui_ThemeColorEntry;

typedef struct
{
    Naui_Map(Naui_ThemeColorEntry) color_map;
    Naui_Map(Naui_ThemeFloatEntry) float_map;
    Naui_Map(Naui_ThemeVec2Entry) vec2_map;
}
Naui_ThemeData;
static Naui_ThemeData data;

void naui_load_theme(const char *file_name)
{
    // Fuck we need jsons CHIMPCHI HURRY TF UP

    // Anyway here are some hardcoded values for now :3
    naui_strmap_put(data.color_map, "naui_panel_border_color",         naui_color_rgba( 55,  55,  60, 255));
    naui_strmap_put(data.color_map, "naui_panel_body_bg_color",        naui_color_rgba( 37,  37,  40, 255));
    naui_strmap_put(data.color_map, "naui_panel_tab_bg_color",         naui_color_rgba( 37,  37,  40, 255));
    naui_strmap_put(data.color_map, "naui_panel_title_bg_color",       naui_color_rgba( 50,  50,  55, 255));
    naui_strmap_put(data.color_map, "naui_panel_title_text_color",     naui_color_rgba(220, 220, 225, 255));
    naui_strmap_put(data.color_map, "naui_panel_text_color",           naui_color_rgba(185, 185, 190, 255));
    naui_strmap_put(data.color_map, "naui_panel_text_disabled_color",  naui_color_rgba( 90,  90,  95, 255));

    Naui_Vec2 padding = { 6.0f, 6.0f };
    naui_strmap_put(data.vec2_map, "naui_panel_title_padding", padding);
    naui_strmap_put(data.vec2_map, "naui_panel_body_padding", padding);

    naui_strmap_put(data.float_map, "naui_panel_font_size", 20.0f);
    naui_strmap_put(data.float_map, "naui_panel_border_width", 1.0f);

    naui_strmap_put(data.float_map, "naui_panel_rounding", 8.0f);
}

Naui_Color naui_theme_color(const char *name)
{
    return naui_strmap_get(data.color_map, name);
}

float naui_theme_float(const char *name)
{
    return naui_strmap_get(data.float_map, name);
}

Naui_Vec2 naui_theme_vec2(const char *name)
{
    return naui_strmap_get(data.vec2_map, name);
}