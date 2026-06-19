#include "renderer.h"

#include <magma/mgapp.h>
#include <magma/mgfx.h>

#include "math/math.h"

#include "shaders/base.glsl.h"

#include <stb/stb_truetype.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define NAUI_RENDERER_MAX_GEOMETRY (1 << 14)
#define NAUI_RENDERER_CORNER_SEGMENTS 8

#define NAUI_FONT_ATLAS_SIZE_SMALL  1024
#define NAUI_FONT_ATLAS_SIZE_LARGE  4096
#define NAUI_FONT_MAX_SLOTS         4
#define NAUI_FONT_FIRST_CHAR        32
#define NAUI_FONT_CHAR_COUNT        96
#define NAUI_FONT_SIZE_SMALL        32.0f
#define NAUI_FONT_SIZE_LARGE        256.0f
#define NAUI_FONT_SIZE_THRESHOLD    32.0f

#define NAUI_CLIP_STACK_MAX 32

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct
{
    Naui_Vec2 position;
    Naui_Vec2 uv;
    uint32_t color;
    int32_t texture_id;
}
Naui_BatchVertex;

typedef struct
{
    Naui_BatchVertex vertices[4];
}
Naui_BatchGeometry;

typedef struct
{
    float x0, y0, x1, y1;
}
Naui_ClipRect;

typedef struct
{
    Naui_BatchGeometry geometry_vertices[NAUI_RENDERER_MAX_GEOMETRY];
    uint32_t geometry_indices[NAUI_RENDERER_MAX_GEOMETRY * 6];
    uint32_t geometry_count;
    uint32_t geometry_offset;

    mgfx_pipeline base_pipeline;
    mgfx_buffer batch_vb, batch_ib;

    mgfx_sampler image_sampler;
    mgfx_image image_atlas;

    mgfx_sampler        font_sampler;
    mgfx_image          font_atlases_small[NAUI_FONT_MAX_SLOTS];
    mgfx_image          font_atlases_large[NAUI_FONT_MAX_SLOTS];
    stbtt_bakedchar     font_chars_small[NAUI_FONT_MAX_SLOTS][NAUI_FONT_CHAR_COUNT];
    stbtt_bakedchar     font_chars_large[NAUI_FONT_MAX_SLOTS][NAUI_FONT_CHAR_COUNT];
    uint8_t             font_loaded[NAUI_FONT_MAX_SLOTS];
    int8_t              font_current_index;

    Naui_ClipRect clip_stack[NAUI_CLIP_STACK_MAX];
    int32_t       clip_stack_depth;

    int32_t width, height;
}
Naui_RendererData;

static Naui_RendererData *rdata;
static float s_cos[NAUI_RENDERER_CORNER_SEGMENTS + 1];
static float s_sin[NAUI_RENDERER_CORNER_SEGMENTS + 1];

static inline float naui_min(float a, float b)
{
    return a < b ? a : b;
}

static inline uint32_t naui_pack_color(Naui_Color c)
{
    return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.r;
}

static inline Naui_Color naui_lerp_color(Naui_Color a, Naui_Color b, float t)
{
    return (Naui_Color){
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
        (uint8_t)(a.a + (b.a - a.a) * t),
    };
}

typedef struct
{
    float dx, dy, mn, range;
}
Naui_GradientAxis;

static Naui_GradientAxis naui_gradient_axis(Naui_Vec2 tl, Naui_Vec2 br, float angle_deg)
{
    float rad = angle_deg;
    float dx = cosf(rad), dy = sinf(rad);

    float pts[4][2] = {
        { tl.x, tl.y },
        { br.x, tl.y },
        { br.x, br.y },
        { tl.x, br.y },
    };

    float mn = 1e30f, mx = -1e30f;
    for (int i = 0; i < 4; i++)
    {
        float d = pts[i][0] * dx + pts[i][1] * dy;
        if (d < mn) mn = d;
        if (d > mx) mx = d;
    }

    float range = mx - mn;
    if (range < 1e-6f) range = 1e-6f;

    return (Naui_GradientAxis){ dx, dy, mn, range };
}

static inline uint32_t naui_gradient_color_at(const Naui_Gradient *g, const Naui_GradientAxis *axis, float px, float py)
{
    float t = (px * axis->dx + py * axis->dy - axis->mn) / axis->range;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return naui_pack_color(naui_lerp_color(g->color1, g->color2, t));
}

static inline Naui_ClipRect naui_current_clip(void)
{
    if (rdata->clip_stack_depth == 0)
        return (Naui_ClipRect){ 0, 0, (float)rdata->width, (float)rdata->height };
    return rdata->clip_stack[rdata->clip_stack_depth - 1];
}

static void naui_apply_scissor(void)
{
    Naui_ClipRect c = naui_current_clip();
    mgfx_scissor((int32_t)c.x0, (int32_t)c.y0, (uint32_t)(c.x1 - c.x0), (uint32_t)(c.y1 - c.y0));
}

static void naui_renderer_flush(void)
{
    if (rdata->geometry_count == rdata->geometry_offset) return;

    uint32_t count  = rdata->geometry_count - rdata->geometry_offset;
    size_t   vb_off = rdata->geometry_offset * sizeof(Naui_BatchGeometry);
    size_t   ib_off = rdata->geometry_offset * 6 * sizeof(uint32_t);

    mgfx_update_buffer(rdata->batch_vb, vb_off, count * sizeof(Naui_BatchGeometry), rdata->geometry_vertices + rdata->geometry_offset);
    mgfx_update_buffer(rdata->batch_ib, ib_off, count * 6 * sizeof(uint32_t), rdata->geometry_indices + rdata->geometry_offset * 6);

    mgfx_draw_indexed(count * 6, rdata->geometry_offset * 6, 0);

    rdata->geometry_offset = rdata->geometry_count;
}

static void naui_push_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, uint32_t color, int32_t texture_id)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, texture_id };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, texture_id };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, texture_id };
    g->vertices[3] = (Naui_BatchVertex){ {d.x, d.y}, {0, 0}, color, texture_id };

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

    rdata->geometry_count++;
}

static void naui_push_gradient_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d,
    uint32_t ca, uint32_t cb, uint32_t cc, uint32_t cd, int32_t texture_id)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, ca, texture_id };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, cb, texture_id };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, cc, texture_id };
    g->vertices[3] = (Naui_BatchVertex){ {d.x, d.y}, {0, 0}, cd, texture_id };

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

    rdata->geometry_count++;
}

static void naui_push_textured_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, Naui_Vec4 texture_area, uint32_t color, int32_t texture_id)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {texture_area.x,                  texture_area.y},                  color, texture_id };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {texture_area.x + texture_area.z, texture_area.y},                  color, texture_id };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {texture_area.x + texture_area.z, texture_area.y + texture_area.w}, color, texture_id };
    g->vertices[3] = (Naui_BatchVertex){ {d.x, d.y}, {texture_area.x,                  texture_area.y + texture_area.w}, color, texture_id };

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

    rdata->geometry_count++;
}

static void naui_push_rect(Naui_Vec2 tl, Naui_Vec2 br, uint32_t color, int32_t texture_id)
{
    naui_push_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        color, texture_id
    );
}

static void naui_push_gradient_rect(Naui_Vec2 tl, Naui_Vec2 br,
    const Naui_Gradient *g, const Naui_GradientAxis *axis, int32_t texture_id)
{
    naui_push_gradient_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        naui_gradient_color_at(g, axis, tl.x, tl.y),
        naui_gradient_color_at(g, axis, br.x, tl.y),
        naui_gradient_color_at(g, axis, br.x, br.y),
        naui_gradient_color_at(g, axis, tl.x, br.y),
        texture_id
    );
}

static void naui_push_textured_rect(Naui_Vec2 tl, Naui_Vec2 br, Naui_Vec4 uv, uint32_t color, int32_t texture_id)
{
    naui_push_textured_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        uv,
        color, texture_id
    );
}

static void naui_push_triangle(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, uint32_t color)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, -1 };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, -1 };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, -1 };
    g->vertices[3] = g->vertices[2];

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2;

    rdata->geometry_count++;
}

static void naui_push_gradient_triangle(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c,
    uint32_t ca, uint32_t cb, uint32_t cc)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, ca, -1 };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, cb, -1 };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, cc, -1 };
    g->vertices[3] = g->vertices[2];

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2;

    rdata->geometry_count++;
}

static void naui_push_corner_fan(Naui_Vec2 center, float r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        Naui_Vec2 p0 = { center.x + ax * s_cos[s]     * r, center.y + ay * s_sin[s]     * r };
        Naui_Vec2 p1 = { center.x + ax * s_cos[s + 1] * r, center.y + ay * s_sin[s + 1] * r };
        naui_push_triangle(center, p0, p1, color);
    }
}

static void naui_push_gradient_corner_fan(Naui_Vec2 center, float r, float ax, float ay,
    const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        Naui_Vec2 p0 = { center.x + ax * s_cos[s]     * r, center.y + ay * s_sin[s]     * r };
        Naui_Vec2 p1 = { center.x + ax * s_cos[s + 1] * r, center.y + ay * s_sin[s + 1] * r };
        naui_push_gradient_triangle(
            center, p0, p1,
            naui_gradient_color_at(g, axis, center.x, center.y),
            naui_gradient_color_at(g, axis, p0.x, p0.y),
            naui_gradient_color_at(g, axis, p1.x, p1.y)
        );
    }
}

static void naui_push_corner_ring(Naui_Vec2 center, float inner_r, float outer_r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx = s_cos[s],     cy = s_sin[s];
        float nx = s_cos[s + 1], ny = s_sin[s + 1];

        Naui_Vec2 o0 = { center.x + ax * cx * outer_r, center.y + ay * cy * outer_r };
        Naui_Vec2 o1 = { center.x + ax * nx * outer_r, center.y + ay * ny * outer_r };
        Naui_Vec2 i0 = { center.x + ax * cx * inner_r, center.y + ay * cy * inner_r };
        Naui_Vec2 i1 = { center.x + ax * nx * inner_r, center.y + ay * ny * inner_r };

        naui_push_quad4(o0, o1, i1, i0, color, -1);
    }
}

void naui_renderer_build_atlas(uint32_t width, uint32_t height, void *data)
{
    rdata->image_atlas = mgfx_create_image(&(mgfx_image_create_info){
        .type = MGFX_IMAGE_TYPE_2D,
        .format = MGFX_FORMAT_RGBA8_UNORM,
        .usage = MGFX_IMAGE_USAGE_COLOR_ATTACHMENT,
        .width = width,
        .height = height,
        .data = data,
    });
}

void naui_renderer_initialize(void)
{
    rdata = calloc(1, sizeof(Naui_RendererData));

    mgfx_init(&(mgfx_init_info){
        .handle = mg_app_handle(),
        .width = mg_app_width(),
        .height = mg_app_height(),
        .vsync = true
    });

    for (int i = 0; i <= NAUI_RENDERER_CORNER_SEGMENTS; i++)
    {
        float a = (float)(M_PI * 0.5) * i / NAUI_RENDERER_CORNER_SEGMENTS;
        s_cos[i] = cosf(a);
        s_sin[i] = sinf(a);
    }

    rdata->batch_vb = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_VERTEX,
        .access = MGFX_ACCESS_CPU,
        .size   = sizeof(rdata->geometry_vertices)
    });

    rdata->batch_ib = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_INDEX,
        .access = MGFX_ACCESS_CPU,
        .size   = sizeof(rdata->geometry_indices)
    });

    rdata->base_pipeline = mgfx_create_pipeline(&(mgfx_pipeline_create_info){
        .shader = get_base_shader(mgfx_get_shader_lang()),
        .vertex_attributes = {
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_UBYTE4N,
            MGFX_VERTEX_FORMAT_INT
        },
        .color_blend = {
            .blend_enabled = true,
            .src_color_blend_factor = MGFX_BLEND_FACTOR_SRC_ALPHA,
            .dst_color_blend_factor = MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = MGFX_BLEND_OP_ADD,
            .src_alpha_blend_factor = MGFX_BLEND_FACTOR_ONE,
            .dst_alpha_blend_factor = MGFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = MGFX_BLEND_OP_ADD
        }
    });

    rdata->image_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .mag_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    rdata->font_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .mag_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    rdata->width = mg_app_width();
    rdata->height = mg_app_height();
}

void naui_renderer_shutdown(void)
{
    mgfx_destroy_sampler(rdata->image_sampler);
    for (int i = 0; i < NAUI_FONT_MAX_SLOTS; i++)
    {
        if (rdata->font_loaded[i])
        {
            mgfx_destroy_image(rdata->font_atlases_small[i]);
            mgfx_destroy_image(rdata->font_atlases_large[i]);
        }
    }
    mgfx_destroy_sampler(rdata->font_sampler);
    mgfx_destroy_buffer(rdata->batch_vb);
    mgfx_destroy_buffer(rdata->batch_ib);
    mgfx_destroy_pipeline(rdata->base_pipeline);
    mgfx_shutdown();
    free(rdata);
}

void naui_renderer_resize(int32_t width, int32_t height)
{
    mgfx_resize(width, height);
    rdata->width = width;
    rdata->height = height;
}

void naui_renderer_begin(void)
{
    rdata->geometry_count = 0;
    rdata->geometry_offset = 0;
    rdata->clip_stack_depth = 0;
    rdata->font_current_index = -1;

    mgfx_begin();
    mgfx_bind_pass(&(mgfx_pass_info){});
    mgfx_bind_pipeline(rdata->base_pipeline);
    mgfx_bind_vertex_buffer(rdata->batch_vb);
    mgfx_bind_index_buffer(rdata->batch_ib, MGFX_INDEX_TYPE_UINT32);
    mgfx_bind_image(rdata->image_atlas, rdata->image_sampler, 0);

    struct { Naui_Vec2 u_resolution; } ub_data;
    ub_data.u_resolution = (Naui_Vec2){(float)rdata->width, (float)rdata->height};
    mgfx_bind_uniforms(0, sizeof(ub_data), &ub_data);
}

void naui_renderer_end(void)
{
    naui_renderer_flush();
    mgfx_end();
}

void naui_push_clip_rect(Naui_Vec2 position, Naui_Vec2 size)
{
    if (rdata->clip_stack_depth >= NAUI_CLIP_STACK_MAX) return;

    naui_renderer_flush();

    float x0 = position.x, y0 = position.y;
    float x1 = x0 + size.x, y1 = y0 + size.y;

    if (rdata->clip_stack_depth > 0)
    {
        Naui_ClipRect *p = &rdata->clip_stack[rdata->clip_stack_depth - 1];
        if (x0 < p->x0) x0 = p->x0;
        if (y0 < p->y0) y0 = p->y0;
        if (x1 > p->x1) x1 = p->x1;
        if (y1 > p->y1) y1 = p->y1;
        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;
    }

    rdata->clip_stack[rdata->clip_stack_depth++] = (Naui_ClipRect){ x0, y0, x1, y1 };
    naui_apply_scissor();
}

void naui_pop_clip_rect(void)
{
    if (rdata->clip_stack_depth == 0) return;

    naui_renderer_flush();
    rdata->clip_stack_depth--;
    naui_apply_scissor();
}

static mgfx_image naui_bake_font_atlas(uint8_t *ttf_buf, float bake_size, int atlas_size, stbtt_bakedchar *chars_out)
{
    uint8_t *alpha = calloc(atlas_size * atlas_size, 1);

    stbtt_BakeFontBitmap(ttf_buf, 0, bake_size,
        alpha, atlas_size, atlas_size,
        NAUI_FONT_FIRST_CHAR, NAUI_FONT_CHAR_COUNT, chars_out);

    uint32_t *rgba = malloc(atlas_size * atlas_size * 4);
    for (int p = 0; p < atlas_size * atlas_size; p++)
    {
        uint8_t v = alpha[p];
        rgba[p] = ((uint32_t)v << 24) | ((uint32_t)v << 16) | ((uint32_t)v << 8) | v;
    }
    free(alpha);

    mgfx_image img = mgfx_create_image(&(mgfx_image_create_info){
        .type   = MGFX_IMAGE_TYPE_2D,
        .format = MGFX_FORMAT_RGBA8_UNORM,
        .usage  = MGFX_IMAGE_USAGE_COLOR_ATTACHMENT,
        .width  = (uint32_t)atlas_size,
        .height = (uint32_t)atlas_size,
        .data   = rgba,
    });
    free(rgba);
    return img;
}

void naui_set_font(uint8_t index, const char *file_path)
{
    if (index >= NAUI_FONT_MAX_SLOTS) return;

    FILE *f = fopen(file_path, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *ttf_buf = malloc(size);
    fread(ttf_buf, 1, size, f);
    fclose(f);

    if (rdata->font_loaded[index])
    {
        mgfx_destroy_image(rdata->font_atlases_small[index]);
        mgfx_destroy_image(rdata->font_atlases_large[index]);
    }

    rdata->font_atlases_small[index] = naui_bake_font_atlas(ttf_buf, NAUI_FONT_SIZE_SMALL, NAUI_FONT_ATLAS_SIZE_SMALL, rdata->font_chars_small[index]);
    rdata->font_atlases_large[index] = naui_bake_font_atlas(ttf_buf, NAUI_FONT_SIZE_LARGE, NAUI_FONT_ATLAS_SIZE_LARGE, rdata->font_chars_large[index]);

    free(ttf_buf);
    rdata->font_loaded[index] = 1;
}

Naui_Vec2 naui_measure_text(const char *text, uint32_t length, float font_size, uint8_t font_index)
{
    if (!text || length == 0) return (Naui_Vec2){0};
    if (font_index >= NAUI_FONT_MAX_SLOTS) return (Naui_Vec2){0};
    if (!rdata->font_loaded[font_index]) return (Naui_Vec2){0};

    int use_large = (font_size > NAUI_FONT_SIZE_THRESHOLD);
    float bake_size = use_large ? NAUI_FONT_SIZE_LARGE : NAUI_FONT_SIZE_SMALL;
    stbtt_bakedchar *chars = use_large ? rdata->font_chars_large[font_index] : rdata->font_chars_small[font_index];

    float scale = font_size / bake_size;
    float x = 0.0f;
    float max_x = 0.0f;
    float y_min = 1e30f;
    float y_max = -1e30f;
    int has_glyph = 0;

    for (uint32_t i = 0; i < length; i++)
    {
        int cp = (unsigned char)text[i];
        if (cp < NAUI_FONT_FIRST_CHAR || cp >= NAUI_FONT_FIRST_CHAR + NAUI_FONT_CHAR_COUNT)
        {
            x += font_size * 0.25f;
            continue;
        }

        stbtt_bakedchar *bc = &chars[cp - NAUI_FONT_FIRST_CHAR];

        float glyph_top    = bc->yoff * scale;
        float glyph_bottom = glyph_top + (bc->y1 - bc->y0) * scale;

        if (glyph_top    < y_min) y_min = glyph_top;
        if (glyph_bottom > y_max) y_max = glyph_bottom;

        x += bc->xadvance * scale;
        if (x > max_x) max_x = x;
        has_glyph = 1;
    }

    if (!has_glyph) return (Naui_Vec2){0};

    return (Naui_Vec2){ .x = max_x, .y = y_max - y_min };
}

void naui_draw_text(Naui_Vec2 position, const char *text, float size, uint8_t font_index, Naui_Color color)
{
    if (font_index >= NAUI_FONT_MAX_SLOTS) return;
    if (!rdata->font_loaded[font_index]) return;

    int use_large = (size > NAUI_FONT_SIZE_THRESHOLD);
    float bake_size = use_large ? NAUI_FONT_SIZE_LARGE : NAUI_FONT_SIZE_SMALL;
    stbtt_bakedchar *chars = use_large ? rdata->font_chars_large[font_index] : rdata->font_chars_small[font_index];

    int8_t wanted = (int8_t)(font_index * 2 + use_large);
    if (rdata->font_current_index != wanted)
    {
        naui_renderer_flush();
        mgfx_bind_image(
            use_large ? rdata->font_atlases_large[font_index] : rdata->font_atlases_small[font_index],
            rdata->font_sampler, 1);
        rdata->font_current_index = wanted;
    }

    float scale = size / bake_size;

    float ascent = 0.0f;
    for (const char *ch = text; *ch; ch++)
    {
        int cp = (unsigned char)*ch;
        if (cp < NAUI_FONT_FIRST_CHAR || cp >= NAUI_FONT_FIRST_CHAR + NAUI_FONT_CHAR_COUNT)
            continue;
        stbtt_bakedchar *bc = &chars[cp - NAUI_FONT_FIRST_CHAR];
        float top = bc->yoff * scale;
        if (top < ascent) ascent = top;
    }

    float x = position.x;
    float y = position.y - ascent;
    uint32_t c = naui_pack_color(color);
    float atlas_sz = (float)(use_large ? NAUI_FONT_ATLAS_SIZE_LARGE : NAUI_FONT_ATLAS_SIZE_SMALL);

    for (const char *ch = text; *ch; ch++)
    {
        int cp = (unsigned char)*ch;
        if (cp < NAUI_FONT_FIRST_CHAR || cp >= NAUI_FONT_FIRST_CHAR + NAUI_FONT_CHAR_COUNT)
        {
            x += size * 0.25f;
            continue;
        }

        stbtt_bakedchar *bc = &chars[cp - NAUI_FONT_FIRST_CHAR];

        float gx0 = x + bc->xoff * scale;
        float gy0 = y + bc->yoff * scale;
        float gx1 = gx0 + (bc->x1 - bc->x0) * scale;
        float gy1 = gy0 + (bc->y1 - bc->y0) * scale;
        float gu0 = bc->x0 / atlas_sz, gv0 = bc->y0 / atlas_sz;
        float gu1 = bc->x1 / atlas_sz, gv1 = bc->y1 / atlas_sz;

        uint32_t i = rdata->geometry_count;
        Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

        g->vertices[0] = (Naui_BatchVertex){ {gx0, gy0}, {gu0, gv0}, c, 1 };
        g->vertices[1] = (Naui_BatchVertex){ {gx1, gy0}, {gu1, gv0}, c, 1 };
        g->vertices[2] = (Naui_BatchVertex){ {gx1, gy1}, {gu1, gv1}, c, 1 };
        g->vertices[3] = (Naui_BatchVertex){ {gx0, gy1}, {gu0, gv1}, c, 1 };

        uint32_t base = i * 4;
        uint32_t *idx = &rdata->geometry_indices[i * 6];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

        rdata->geometry_count++;
        x += bc->xadvance * scale;
    }
}

void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    uint32_t c = naui_pack_color(color);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_rect(position, (Naui_Vec2){x1, y1}, c, -1);
        return;
    }

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    float x0l = x0 + (tl ? r : 0), x0r = x0 + (bl ? r : 0);
    float x1l = x1 - (tr ? r : 0), x1r = x1 - (br ? r : 0);
    float y0t = y0 + (tl ? r : 0), y0b = y0 + (tr ? r : 0);
    float y1t = y1 - (bl ? r : 0), y1b = y1 - (br ? r : 0);

    naui_push_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, c, -1);
    naui_push_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, c, -1);
    naui_push_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, c, -1);

    if (tl) naui_push_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, c);
    if (tr) naui_push_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, c);
    if (br) naui_push_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, c);
    if (bl) naui_push_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, c);
}

void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    uint32_t c = naui_pack_color(color);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_rect((Naui_Vec2){x0-lw, y0-lw}, (Naui_Vec2){x1+lw, y0    }, c, -1);
        naui_push_rect((Naui_Vec2){x0-lw, y1    }, (Naui_Vec2){x1+lw, y1+lw}, c, -1);
        naui_push_rect((Naui_Vec2){x0-lw, y0    }, (Naui_Vec2){x0,    y1    }, c, -1);
        naui_push_rect((Naui_Vec2){x1,    y0    }, (Naui_Vec2){x1+lw, y1    }, c, -1);
        return;
    }

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float outer_r = r + lw;

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    naui_push_rect((Naui_Vec2){x0 + (tl ? r : -lw), y0-lw}, (Naui_Vec2){x1 - (tr ? r : -lw), y0}, c, -1);
    naui_push_rect((Naui_Vec2){x0 + (bl ? r : -lw), y1   }, (Naui_Vec2){x1 - (br ? r : -lw), y1+lw}, c, -1);
    naui_push_rect((Naui_Vec2){x0-lw, y0 + (tl ? r : 0)}, (Naui_Vec2){x0, y1 - (bl ? r : 0)}, c, -1);
    naui_push_rect((Naui_Vec2){x1,    y0 + (tr ? r : 0)}, (Naui_Vec2){x1+lw, y1 - (br ? r : 0)}, c, -1);

    if (tl) naui_push_corner_ring((Naui_Vec2){x0+r, y0+r}, r, outer_r, -1, -1, c);
    if (tr) naui_push_corner_ring((Naui_Vec2){x1-r, y0+r}, r, outer_r, +1, -1, c);
    if (br) naui_push_corner_ring((Naui_Vec2){x1-r, y1-r}, r, outer_r, +1, +1, c);
    if (bl) naui_push_corner_ring((Naui_Vec2){x0+r, y1-r}, r, outer_r, -1, +1, c);
}

void naui_fill_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, gradient.angle);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_gradient_rect((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, &gradient, &axis, -1);
        return;
    }

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    naui_push_gradient_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, &gradient, &axis, -1);
    naui_push_gradient_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, &gradient, &axis, -1);
    naui_push_gradient_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, &gradient, &axis, -1);

    if (tl) naui_push_gradient_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, &gradient, &axis);
    if (tr) naui_push_gradient_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, &gradient, &axis);
    if (br) naui_push_gradient_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, &gradient, &axis);
    if (bl) naui_push_gradient_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, &gradient, &axis);
}

void naui_draw_gradient_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient gradient, float line_width, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, gradient.angle);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_gradient_rect((Naui_Vec2){x0-lw, y0-lw}, (Naui_Vec2){x1+lw, y0    }, &gradient, &axis, -1);
        naui_push_gradient_rect((Naui_Vec2){x0-lw, y1    }, (Naui_Vec2){x1+lw, y1+lw}, &gradient, &axis, -1);
        naui_push_gradient_rect((Naui_Vec2){x0-lw, y0    }, (Naui_Vec2){x0,    y1    }, &gradient, &axis, -1);
        naui_push_gradient_rect((Naui_Vec2){x1,    y0    }, (Naui_Vec2){x1+lw, y1    }, &gradient, &axis, -1);
        return;
    }

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float outer_r = r + lw;

    naui_push_gradient_rect((Naui_Vec2){x0 + (tl ? r : -lw), y0-lw}, (Naui_Vec2){x1 - (tr ? r : -lw), y0    }, &gradient, &axis, -1);
    naui_push_gradient_rect((Naui_Vec2){x0 + (bl ? r : -lw), y1   }, (Naui_Vec2){x1 - (br ? r : -lw), y1+lw }, &gradient, &axis, -1);
    naui_push_gradient_rect((Naui_Vec2){x0-lw, y0 + (tl ? r : 0)}, (Naui_Vec2){x0, y1 - (bl ? r : 0)}, &gradient, &axis, -1);
    naui_push_gradient_rect((Naui_Vec2){x1,    y0 + (tr ? r : 0)}, (Naui_Vec2){x1+lw, y1 - (br ? r : 0)}, &gradient, &axis, -1);

    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx = s_cos[s],     cy = s_sin[s];
        float nx = s_cos[s + 1], ny = s_sin[s + 1];

        float ax_vals[4] = { -1, +1, +1, -1 };
        float ay_vals[4] = { -1, -1, +1, +1 };
        Naui_Vec2 centers[4] = {
            { x0+r, y0+r },
            { x1-r, y0+r },
            { x1-r, y1-r },
            { x0+r, y1-r },
        };
        int corner_flags[4] = { tl, tr, br, bl };

        for (int corner = 0; corner < 4; corner++)
        {
            if (!corner_flags[corner]) continue;

            float ax = ax_vals[corner], ay = ay_vals[corner];
            Naui_Vec2 center = centers[corner];

            Naui_Vec2 o0 = { center.x + ax * cx * outer_r, center.y + ay * cy * outer_r };
            Naui_Vec2 o1 = { center.x + ax * nx * outer_r, center.y + ay * ny * outer_r };
            Naui_Vec2 i0 = { center.x + ax * cx * r,       center.y + ay * cy * r       };
            Naui_Vec2 i1 = { center.x + ax * nx * r,       center.y + ay * ny * r       };

            naui_push_gradient_quad4(o0, o1, i1, i0,
                naui_gradient_color_at(&gradient, &axis, o0.x, o0.y),
                naui_gradient_color_at(&gradient, &axis, o1.x, o1.y),
                naui_gradient_color_at(&gradient, &axis, i1.x, i1.y),
                naui_gradient_color_at(&gradient, &axis, i0.x, i0.y),
                -1);
        }
    }
}

void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float inv_len = line_width * 0.5f / sqrtf(dx * dx + dy * dy);
    if (inv_len > 1e10f) return;

    float nx = -dy * inv_len;
    float ny =  dx * inv_len;

    naui_push_quad4(
        (Naui_Vec2){ a.x + nx, a.y + ny },
        (Naui_Vec2){ b.x + nx, b.y + ny },
        (Naui_Vec2){ b.x - nx, b.y - ny },
        (Naui_Vec2){ a.x - nx, a.y - ny },
        naui_pack_color(color), -1
    );
}

static inline Naui_Vec4 uv_for_sub(Naui_Vec4 uv, Naui_Vec2 img_tl, Naui_Vec2 img_size, float sx0, float sy0, float sx1, float sy1)
{
    float u0 = uv.x + ((sx0 - img_tl.x) / img_size.x) * uv.z;
    float v0 = uv.y + ((sy0 - img_tl.y) / img_size.y) * uv.w;
    float u1 = ((sx1 - sx0) / img_size.x) * uv.z;
    float v1 = ((sy1 - sy0) / img_size.y) * uv.w;
    return (Naui_Vec4){ u0, v0, u1, v1 };
}

static void naui_push_textured_corner_fan(
    Naui_Vec2 center, float r, float ax, float ay,
    Naui_Vec4 uv_rect,
    Naui_Vec2 img_tl, Naui_Vec2 img_size,
    uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx0 = ax * s_cos[s],     cy0 = ay * s_sin[s];
        float cx1 = ax * s_cos[s + 1], cy1 = ay * s_sin[s + 1];

        Naui_Vec2 p0 = { center.x + cx0 * r, center.y + cy0 * r };
        Naui_Vec2 p1 = { center.x + cx1 * r, center.y + cy1 * r };

        float u_c  = uv_rect.x + ((center.x - img_tl.x) / img_size.x) * uv_rect.z;
        float v_c  = uv_rect.y + ((center.y - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p0 = uv_rect.x + ((p0.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p0 = uv_rect.y + ((p0.y     - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p1 = uv_rect.x + ((p1.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p1 = uv_rect.y + ((p1.y     - img_tl.y) / img_size.y) * uv_rect.w;

        uint32_t i = rdata->geometry_count;
        Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

        g->vertices[0] = (Naui_BatchVertex){ {center.x, center.y}, {u_c,  v_c},  color, 0 };
        g->vertices[1] = (Naui_BatchVertex){ {p0.x,     p0.y    }, {u_p0, v_p0}, color, 0 };
        g->vertices[2] = (Naui_BatchVertex){ {p1.x,     p1.y    }, {u_p1, v_p1}, color, 0 };
        g->vertices[3] = g->vertices[2];

        uint32_t base = i * 4;
        uint32_t *idx = &rdata->geometry_indices[i * 6];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2;

        rdata->geometry_count++;
    }
}

void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;

    const float s0 = image->texture_area[0], t0 = image->texture_area[1];
    const float s1 = image->texture_area[2], t1 = image->texture_area[3];
    Naui_Vec4 uv = { s0, t0, s1, t1 };
    uint32_t c = naui_pack_color(tint);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_textured_rect(position, (Naui_Vec2){x1, y1}, uv, c, 0);
        return;
    }

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    Naui_Vec2 img_tl   = { x0, y0 };
    Naui_Vec2 img_size = { scale.x, scale.y };

    naui_push_textured_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, uv_for_sub(uv, img_tl, img_size, x0+r, y0, x1-r, y1), c, 0);
    naui_push_textured_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x0, y0 + (tl ? r : 0), x0+r, y1 - (bl ? r : 0)), c, 0);
    naui_push_textured_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x1-r, y0 + (tr ? r : 0), x1, y1 - (br ? r : 0)), c, 0);

    if (tl) naui_push_textured_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, uv, img_tl, img_size, c);
    if (tr) naui_push_textured_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, uv, img_tl, img_size, c);
    if (br) naui_push_textured_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, uv, img_tl, img_size, c);
    if (bl) naui_push_textured_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, uv, img_tl, img_size, c);
}

static void naui_push_textured_gradient_corner_fan(
    Naui_Vec2 center, float r, float ax, float ay,
    Naui_Vec4 uv_rect,
    Naui_Vec2 img_tl, Naui_Vec2 img_size,
    const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx0 = ax * s_cos[s],     cy0 = ay * s_sin[s];
        float cx1 = ax * s_cos[s + 1], cy1 = ay * s_sin[s + 1];

        Naui_Vec2 p0 = { center.x + cx0 * r, center.y + cy0 * r };
        Naui_Vec2 p1 = { center.x + cx1 * r, center.y + cy1 * r };

        float u_c  = uv_rect.x + ((center.x - img_tl.x) / img_size.x) * uv_rect.z;
        float v_c  = uv_rect.y + ((center.y - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p0 = uv_rect.x + ((p0.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p0 = uv_rect.y + ((p0.y     - img_tl.y) / img_size.y) * uv_rect.w;
        float u_p1 = uv_rect.x + ((p1.x     - img_tl.x) / img_size.x) * uv_rect.z;
        float v_p1 = uv_rect.y + ((p1.y     - img_tl.y) / img_size.y) * uv_rect.w;

        uint32_t i = rdata->geometry_count;
        Naui_BatchGeometry *geo = &rdata->geometry_vertices[i];

        geo->vertices[0] = (Naui_BatchVertex){ {center.x, center.y}, {u_c,  v_c },  naui_gradient_color_at(g, axis, center.x, center.y), 0 };
        geo->vertices[1] = (Naui_BatchVertex){ {p0.x,     p0.y    }, {u_p0, v_p0},  naui_gradient_color_at(g, axis, p0.x,     p0.y    ), 0 };
        geo->vertices[2] = (Naui_BatchVertex){ {p1.x,     p1.y    }, {u_p1, v_p1},  naui_gradient_color_at(g, axis, p1.x,     p1.y    ), 0 };
        geo->vertices[3] = geo->vertices[2];

        uint32_t base = i * 4;
        uint32_t *idx = &rdata->geometry_indices[i * 6];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2;

        rdata->geometry_count++;
    }
}

static void naui_push_textured_gradient_rect(Naui_Vec2 tl, Naui_Vec2 br,
    Naui_Vec4 texture_area, const Naui_Gradient *g, const Naui_GradientAxis *axis)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *geo = &rdata->geometry_vertices[i];

    geo->vertices[0] = (Naui_BatchVertex){ {tl.x, tl.y}, {texture_area.x,                   texture_area.y                  }, naui_gradient_color_at(g, axis, tl.x, tl.y), 0 };
    geo->vertices[1] = (Naui_BatchVertex){ {br.x, tl.y}, {texture_area.x + texture_area.z,  texture_area.y                  }, naui_gradient_color_at(g, axis, br.x, tl.y), 0 };
    geo->vertices[2] = (Naui_BatchVertex){ {br.x, br.y}, {texture_area.x + texture_area.z,  texture_area.y + texture_area.w }, naui_gradient_color_at(g, axis, br.x, br.y), 0 };
    geo->vertices[3] = (Naui_BatchVertex){ {tl.x, br.y}, {texture_area.x,                   texture_area.y + texture_area.w }, naui_gradient_color_at(g, axis, tl.x, br.y), 0 };

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

    rdata->geometry_count++;
}

void naui_draw_gradient_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Gradient tint, float rounding, Naui_CornerFlags flags)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;

    const float s0 = image->texture_area[0], t0 = image->texture_area[1];
    const float s1 = image->texture_area[2], t1 = image->texture_area[3];
    Naui_Vec4 uv = { s0, t0, s1, t1 };

    Naui_GradientAxis axis = naui_gradient_axis((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, tint.angle);

    if (rounding <= 0.0f || flags == NAUI_CORNER_NONE)
    {
        naui_push_textured_gradient_rect((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, uv, &tint, &axis);
        return;
    }

    int tl = (flags & NAUI_CORNER_TL) != 0;
    int tr = (flags & NAUI_CORNER_TR) != 0;
    int br = (flags & NAUI_CORNER_BR) != 0;
    int bl = (flags & NAUI_CORNER_BL) != 0;

    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    Naui_Vec2 img_tl   = { x0, y0 };
    Naui_Vec2 img_size = { scale.x, scale.y };

    naui_push_textured_gradient_rect((Naui_Vec2){x0+r, y0}, (Naui_Vec2){x1-r, y1}, uv_for_sub(uv, img_tl, img_size, x0+r, y0, x1-r, y1), &tint, &axis);
    naui_push_textured_gradient_rect((Naui_Vec2){x0, y0 + (tl ? r : 0)}, (Naui_Vec2){x0+r, y1 - (bl ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x0, y0 + (tl ? r : 0), x0+r, y1 - (bl ? r : 0)), &tint, &axis);
    naui_push_textured_gradient_rect((Naui_Vec2){x1-r, y0 + (tr ? r : 0)}, (Naui_Vec2){x1, y1 - (br ? r : 0)}, uv_for_sub(uv, img_tl, img_size, x1-r, y0 + (tr ? r : 0), x1, y1 - (br ? r : 0)), &tint, &axis);

    if (tl) naui_push_textured_gradient_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, uv, img_tl, img_size, &tint, &axis);
    if (tr) naui_push_textured_gradient_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, uv, img_tl, img_size, &tint, &axis);
    if (br) naui_push_textured_gradient_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, uv, img_tl, img_size, &tint, &axis);
    if (bl) naui_push_textured_gradient_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, uv, img_tl, img_size, &tint, &axis);
}

void naui_draw_shadow(Naui_Vec2 position, Naui_Vec2 scale, float blur_radius, Naui_Color color, float rounding, Naui_CornerFlags corners)
{
    float r  = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);
    float br = blur_radius;

    float x0 = position.x, y0 = position.y;
    float x1 = position.x + scale.x, y1 = position.y + scale.y;

    uint32_t c_full = naui_pack_color(color);
    uint32_t c_zero = naui_pack_color((Naui_Color){ color.r, color.g, color.b, 0 });

    int tl = (corners & NAUI_CORNER_TL) != 0;
    int tr = (corners & NAUI_CORNER_TR) != 0;
    int br_flag = (corners & NAUI_CORNER_BR) != 0;
    int bl = (corners & NAUI_CORNER_BL) != 0;

    float cx_tl = x0 + (tl ? r : 0), cy_tl = y0 + (tl ? r : 0);
    float cx_tr = x1 - (tr ? r : 0), cy_tr = y0 + (tr ? r : 0);
    float cx_br = x1 - (br_flag ? r : 0), cy_br = y1 - (br_flag ? r : 0);
    float cx_bl = x0 + (bl ? r : 0), cy_bl = y1 - (bl ? r : 0);

    naui_push_gradient_quad4(
        (Naui_Vec2){cx_tl, y0 - br}, (Naui_Vec2){cx_tr, y0 - br},
        (Naui_Vec2){cx_tr, y0     }, (Naui_Vec2){cx_tl, y0     },
        c_zero, c_zero, c_full, c_full, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){cx_bl, y1     }, (Naui_Vec2){cx_br, y1     },
        (Naui_Vec2){cx_br, y1 + br}, (Naui_Vec2){cx_bl, y1 + br},
        c_full, c_full, c_zero, c_zero, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){x0 - br, cy_tl}, (Naui_Vec2){x0, cy_tl},
        (Naui_Vec2){x0, cy_bl}, (Naui_Vec2){x0 - br, cy_bl},
        c_zero, c_full, c_full, c_zero, -1);

    naui_push_gradient_quad4(
        (Naui_Vec2){x1,      cy_tr}, (Naui_Vec2){x1 + br, cy_tr},
        (Naui_Vec2){x1 + br, cy_br}, (Naui_Vec2){x1,      cy_br},
        c_full, c_zero, c_zero, c_full, -1);

    naui_push_rect((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, c_full, -1);

    struct {
        float cx, cy;
        float ax, ay;
        int enabled;
    } fan[4] = {
        { cx_tl, cy_tl, -1, -1, tl      },
        { cx_tr, cy_tr, +1, -1, tr      },
        { cx_br, cy_br, +1, +1, br_flag },
        { cx_bl, cy_bl, -1, +1, bl      },
    };

    for (int f = 0; f < 4; f++)
    {
        if (!fan[f].enabled)
        {
            float cx = fan[f].cx, cy = fan[f].cy;
            float ax = fan[f].ax, ay = fan[f].ay;

            naui_push_gradient_quad4(
                (Naui_Vec2){cx, cy },
                (Naui_Vec2){cx + ax*br, cy },
                (Naui_Vec2){cx + ax*br, cy + ay*br},
                (Naui_Vec2){cx, cy + ay*br},
                c_full, c_zero, c_zero, c_zero, -1);
            continue;
        }

        float cx = fan[f].cx, cy = fan[f].cy;
        float ax = fan[f].ax, ay = fan[f].ay;

        for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
        {
            float c0 = s_cos[s], s0 = s_sin[s];
            float c1 = s_cos[s + 1], s1 = s_sin[s + 1];

            Naui_Vec2 i0 = { cx + ax * c0 * r, cy + ay * s0 * r };
            Naui_Vec2 i1 = { cx + ax * c1 * r, cy + ay * s1 * r };

            Naui_Vec2 o0 = { cx + ax * c0 * (r + br), cy + ay * s0 * (r + br) };
            Naui_Vec2 o1 = { cx + ax * c1 * (r + br), cy + ay * s1 * (r + br) };

            naui_push_gradient_quad4(i0, i1, o1, o0,
                c_full, c_full, c_zero, c_zero, -1);
        }
    }
}