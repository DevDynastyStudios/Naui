#pragma once

#include <leaf/leaf.h>

typedef struct
{
    float x, y;
}
Naui_Vec2;
typedef struct
{
    float x, y, z, w;
}
Naui_Vec4;

typedef struct
{
    uint8_t r, g, b, a;
}
Naui_Color;

typedef struct
{
    uint32_t width, height;
    float texture_area[4];
}
Naui_Image;

void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color);
void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width);
void naui_fill_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding);
void naui_draw_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding);
void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width);
void naui_draw_image(Naui_Image image, Naui_Vec2 position, Naui_Vec2 scale);
