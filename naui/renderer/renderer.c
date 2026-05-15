#include "renderer.h"

#include <magma/mgapp.h>
#include <magma/mgfx.h>
#include "shaders/base.glsl.h"

#include <stdlib.h>
#include <math.h>

#define NAUI_RENDERER_MAX_GEOMETRY (1 << 14)
#define NAUI_RENDERER_CORNER_SEGMENTS 16

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
    Naui_BatchGeometry geometry_vertices[NAUI_RENDERER_MAX_GEOMETRY];
    uint32_t geometry_indices[NAUI_RENDERER_MAX_GEOMETRY * 6];
    uint32_t geometry_count;

    mgfx_pipeline base_pipeline;
    mgfx_buffer batch_vb, batch_ib;

    mgfx_sampler image_sampler;
    mgfx_sampler font_sampler;
    mgfx_image image_atlas;
    mgfx_image font_atlas;

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
    return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g <<  8) |  (uint32_t)c.r;
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

static void naui_push_textured_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, Naui_Vec4 texture_area, uint32_t color, int32_t texture_id)
{
    uint32_t i = rdata->geometry_count;
    Naui_BatchGeometry *g = &rdata->geometry_vertices[i];

    // long boooooi
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

static void naui_push_textured_rect(Naui_Vec2 tl, Naui_Vec2 br, Naui_Vec4 texture_area, uint32_t color, int32_t texture_id)
{
    naui_push_textured_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        texture_area,
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
    g->vertices[3] = g->vertices[2]; // degenerate

    uint32_t base = i * 4;
    uint32_t *idx = &rdata->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2; // degenerate

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

void naui_renderer_initialize(const char *default_font_path)
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
        float a = (float)(M_PI / 2.0) * i / NAUI_RENDERER_CORNER_SEGMENTS;
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
}

void naui_renderer_end(void)
{
    mgfx_begin();
    mgfx_bind_pass(&(mgfx_pass_info){});
    mgfx_bind_pipeline(rdata->base_pipeline);
    mgfx_bind_image(rdata->image_atlas, rdata->image_sampler, 0);
    //mgfx_bind_image(data->font_sampler, data->font_sampler, 1);

    struct
    {
        Naui_Vec2 u_resolution;
    }
    ub_data;
    ub_data.u_resolution = (Naui_Vec2){(float)rdata->width, (float)rdata->height};
    mgfx_bind_uniforms(0, sizeof(ub_data), &ub_data);

    mgfx_update_buffer(rdata->batch_vb, rdata->geometry_count * sizeof(Naui_BatchGeometry), rdata->geometry_vertices);
    mgfx_update_buffer(rdata->batch_ib, rdata->geometry_count * 6 * sizeof(uint32_t), rdata->geometry_indices);
    mgfx_bind_vertex_buffer(rdata->batch_vb);
    mgfx_bind_index_buffer(rdata->batch_ib, MGFX_INDEX_TYPE_UINT32);
    mgfx_draw_indexed(rdata->geometry_count * 6, 0, 0);
    mgfx_end();
}

void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color)
{
    Naui_Vec2 br = { position.x + scale.x, position.y + scale.y };
    naui_push_rect(position, br, naui_pack_color(color), -1);
}

void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    uint32_t c = naui_pack_color(color);

    naui_push_rect((Naui_Vec2){x0,    y0     }, (Naui_Vec2){x1,    y0+lw  }, c, -1); // top
    naui_push_rect((Naui_Vec2){x0,    y1-lw  }, (Naui_Vec2){x1,    y1     }, c, -1); // bottom
    naui_push_rect((Naui_Vec2){x0,    y0+lw  }, (Naui_Vec2){x0+lw, y1-lw  }, c, -1); // left
    naui_push_rect((Naui_Vec2){x1-lw, y0+lw  }, (Naui_Vec2){x1,    y1-lw  }, c, -1); // right
}

void naui_fill_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    uint32_t c = naui_pack_color(color);

    naui_push_rect((Naui_Vec2){x0+r, y0  }, (Naui_Vec2){x1-r, y1  }, c, -1); // center
    naui_push_rect((Naui_Vec2){x0,   y0+r}, (Naui_Vec2){x0+r, y1-r}, c, -1); // left wing
    naui_push_rect((Naui_Vec2){x1-r, y0+r}, (Naui_Vec2){x1,   y1-r}, c, -1); // right wing

    naui_push_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, c); // TL
    naui_push_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, c); // TR
    naui_push_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, c); // BR
    naui_push_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, c); // BL
}

void naui_draw_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f), lw = line_width;
    float inner_r = fmaxf(r - lw, 0.0f);
    uint32_t c = naui_pack_color(color);

    naui_push_rect((Naui_Vec2){x0+r, y0    }, (Naui_Vec2){x1-r, y0+lw }, c, -1); // top
    naui_push_rect((Naui_Vec2){x0+r, y1-lw }, (Naui_Vec2){x1-r, y1    }, c, -1); // bottom
    naui_push_rect((Naui_Vec2){x0,   y0+r  }, (Naui_Vec2){x0+lw, y1-r }, c, -1); // left
    naui_push_rect((Naui_Vec2){x1-lw,y0+r  }, (Naui_Vec2){x1,    y1-r }, c, -1); // right

    naui_push_corner_ring((Naui_Vec2){x0+r, y0+r}, inner_r, r, -1, -1, c); // TL
    naui_push_corner_ring((Naui_Vec2){x1-r, y0+r}, inner_r, r, +1, -1, c); // TR
    naui_push_corner_ring((Naui_Vec2){x1-r, y1-r}, inner_r, r, +1, +1, c); // BR
    naui_push_corner_ring((Naui_Vec2){x0+r, y1-r}, inner_r, r, -1, +1, c); // BL
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

void naui_draw_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint)
{
    Naui_Vec2 br = { position.x + scale.x, position.y + scale.y };
    const float s0 = image->texture_area[0],
    t0 = image->texture_area[1],
    s1 = image->texture_area[2],
    t1 = image->texture_area[3];

    naui_push_textured_rect(position, br, (Naui_Vec4){ s0,t0,s1,t1 }, naui_pack_color(tint), 0);
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
        g->vertices[3] = g->vertices[2]; // degenerate

        uint32_t base = i * 4;
        uint32_t *idx = &rdata->geometry_indices[i * 6];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2;

        rdata->geometry_count++;
    }
}

static inline Naui_Vec4 uv_for_sub(Naui_Vec4 uv, Naui_Vec2 img_tl, Naui_Vec2 img_size, float sx0, float sy0, float sx1, float sy1)
{
    float u0 = uv.x + ((sx0 - img_tl.x) / img_size.x) * uv.z;
    float v0 = uv.y + ((sy0 - img_tl.y) / img_size.y) * uv.w;
    float u1 = ((sx1 - sx0) / img_size.x) * uv.z;
    float v1 = ((sy1 - sy0) / img_size.y) * uv.w;
    return (Naui_Vec4){ u0, v0, u1, v1 };
}

void naui_draw_round_image(const Naui_Image *image, Naui_Vec2 position, Naui_Vec2 scale, Naui_Color tint, float rounding)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    const float s0 = image->texture_area[0], t0 = image->texture_area[1];
    const float s1 = image->texture_area[2], t1 = image->texture_area[3];
    Naui_Vec4 uv = { s0, t0, s1, t1 };

    Naui_Vec2 img_tl   = { x0, y0 };
    Naui_Vec2 img_size = { scale.x, scale.y };

    uint32_t c = naui_pack_color(tint);

    naui_push_textured_rect((Naui_Vec2){x0+r, y0  }, (Naui_Vec2){x1-r, y1  }, uv_for_sub(uv, img_tl, img_size, x0+r, y0,   x1-r, y1  ), c, 0);
    naui_push_textured_rect((Naui_Vec2){x0,   y0+r}, (Naui_Vec2){x0+r, y1-r}, uv_for_sub(uv, img_tl, img_size, x0,   y0+r, x0+r, y1-r), c, 0);
    naui_push_textured_rect((Naui_Vec2){x1-r, y0+r}, (Naui_Vec2){x1,   y1-r}, uv_for_sub(uv, img_tl, img_size, x1-r, y0+r, x1,   y1-r), c, 0);

    // Corners
    naui_push_textured_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, uv, img_tl, img_size, c); // TL
    naui_push_textured_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, uv, img_tl, img_size, c); // TR
    naui_push_textured_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, uv, img_tl, img_size, c); // BR
    naui_push_textured_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, uv, img_tl, img_size, c); // BL
}
