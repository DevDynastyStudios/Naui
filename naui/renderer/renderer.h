#pragma once

#include "base.h"
#include "math/vec2.h"
#include "math/vec4.h"

typedef struct
{
    uint8_t r, g, b, a;
}
Naui_Color;

typedef struct
{
    Naui_Color color1, color2;
    float angle;
}
Naui_Gradient;

typedef struct
{
    uint32_t width, height;
    float texture_area[4];
}
Naui_Image;

static inline Naui_Color naui_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (Naui_Color){ r, g, b, a };
}

NAUI_API void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding);
NAUI_API void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding);
NAUI_API void naui_fill_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float rounding);
NAUI_API void naui_draw_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float line_width, float rounding);

NAUI_API void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width);

NAUI_API void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding);
NAUI_API void naui_draw_gradient_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient tint, float rounding);

NAUI_API void naui_set_font(uint8_t index, const char *file_path);
NAUI_API void naui_draw_text(Naui_Vec2 position, const char *text, float size, uint8_t font_index, Naui_Color color);
NAUI_API Naui_Vec2 naui_measure_text(const char *text, uint32_t length, float font_size, uint8_t font_index);

NAUI_API void naui_push_clip_rect(Naui_Vec2 position, Naui_Vec2 size);
NAUI_API void naui_pop_clip_rect(void);
