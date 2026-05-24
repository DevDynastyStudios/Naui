#pragma once

#include "base.h"
#include "renderer/renderer.h"

#include <leaf/leaf.h>

NAUI_API void       naui_load_theme         (const char *file_name);

NAUI_API Naui_Color naui_theme_color        (const char *name);
NAUI_API float      naui_theme_float        (const char *name);

static inline Leaf_Color naui_theme_leaf_color(const char *name)
{
    const Naui_Color color = naui_theme_color(name);
    return (Leaf_Color) { color.r, color.g, color.b, color.a };
}