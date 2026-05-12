#pragma once

#include "leaf.h"

typedef struct
{
    float x, y;
}
Leaf_Vec2;

typedef struct
{
    uint8_t r, g, b, a;
}
Leaf_Color;

typedef uint8_t Leaf_ColorFillType;
enum
{
    LEAF_SOLID_COLOR_FILL = 0,
    LEAF_GRADIENT_LINEAR_COLOR_FILL,
    LEAF_GRADIENT_DOT_COLOR_FILL
};

typedef struct
{
    Leaf_Color color1;
    Leaf_Color color2;
    float angle;
    Leaf_ColorFillType type;
}
Leaf_ColorFill;

void naui_renderer_initialize(const char *default_font_path);
void naui_renderer_shutdown(void);

// outlines (borders) only
void naui_draw_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_Color color, float line_width);
void naui_draw_gradient_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_ColorFill fill, float line_width);
void naui_draw_round_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_Color color, float line_width, float rounding);
void naui_draw_round_gradient_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_ColorFill fill, float line_width, float rounding);

// fills
void naui_fill_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_Color color);
void naui_fill_gradient_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_ColorFill fill);
void naui_fill_round_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_Color color, float rounding);
void naui_fill_round_gradient_rect(Leaf_Vec2 position, Leaf_Vec2 scale, Leaf_ColorFill fill, float rounding);

// iamges
void naui_draw_image(Leaf_Vec2 position, Leaf_Vec2 scale, Naui_Image *image, Leaf_Color tint);
void naui_draw_round_image(Leaf_Vec2 position, Leaf_Vec2 scale, Naui_Image *image, Leaf_Color tint, float rounding);
void naui_draw_gradient_image(Leaf_Vec2 position, Leaf_Vec2 scale, Naui_Image *image, Leaf_Color tint);
void naui_draw_round_gradient_image(Leaf_Vec2 position, Leaf_Vec2 scale, Naui_Image *image, Leaf_ColorFill tint, float rounding);

// lines
void naui_draw_line(Leaf_Vec2 point1, Leaf_Vec2 point2, Leaf_Color color, float line_width);
void naui_draw_line_list(Leaf_Vec2 *points, uint32_t point_count, Leaf_Color color, float line_width);

// text
void naui_draw_text(Leaf_Vec2 position, float font_size, Leaf_Color color);