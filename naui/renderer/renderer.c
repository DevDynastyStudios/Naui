#include "renderer.h"
#include "shaders/base.h"

#include <magma/mgapp.h>
#include <magma/mgfx.h>

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

static Naui_RendererData *data;
static float s_cos[NAUI_RENDERER_CORNER_SEGMENTS + 1];
static float s_sin[NAUI_RENDERER_CORNER_SEGMENTS + 1];

static inline float naui_min(float a, float b)
{
    return a < b ? a : b;
}

static inline uint32_t pack_color(Naui_Color c)
{
    return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g <<  8) |  (uint32_t)c.r;
}

static void push_quad4(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, Naui_Vec2 d, uint32_t color, int32_t texture_id)
{
    uint32_t i = data->geometry_count;
    Naui_BatchGeometry *g = &data->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, texture_id };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, texture_id };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, texture_id };
    g->vertices[3] = (Naui_BatchVertex){ {d.x, d.y}, {0, 0}, color, texture_id };

    uint32_t base = i * 4;
    uint32_t *idx = &data->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 0;

    data->geometry_count++;
}

static void push_rect(Naui_Vec2 tl, Naui_Vec2 br, uint32_t color, int32_t texture_id)
{
    push_quad4(
        (Naui_Vec2){tl.x, tl.y},
        (Naui_Vec2){br.x, tl.y},
        (Naui_Vec2){br.x, br.y},
        (Naui_Vec2){tl.x, br.y},
        color, texture_id
    );
}

static void push_triangle(Naui_Vec2 a, Naui_Vec2 b, Naui_Vec2 c, uint32_t color)
{
    uint32_t i = data->geometry_count;
    Naui_BatchGeometry *g = &data->geometry_vertices[i];

    g->vertices[0] = (Naui_BatchVertex){ {a.x, a.y}, {0, 0}, color, -1 };
    g->vertices[1] = (Naui_BatchVertex){ {b.x, b.y}, {0, 0}, color, -1 };
    g->vertices[2] = (Naui_BatchVertex){ {c.x, c.y}, {0, 0}, color, -1 };
    g->vertices[3] = g->vertices[2]; // degenerate

    uint32_t base = i * 4;
    uint32_t *idx = &data->geometry_indices[i * 6];
    idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
    idx[3] = base + 2; idx[4] = base + 3; idx[5] = base + 2; // degenerate

    data->geometry_count++;
}

static void push_corner_fan(Naui_Vec2 center, float r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        Naui_Vec2 p0 = { center.x + ax * s_cos[s]     * r, center.y + ay * s_sin[s]     * r };
        Naui_Vec2 p1 = { center.x + ax * s_cos[s + 1] * r, center.y + ay * s_sin[s + 1] * r };
        push_triangle(center, p0, p1, color);
    }
}

static void push_corner_ring(Naui_Vec2 center, float inner_r, float outer_r, float ax, float ay, uint32_t color)
{
    for (int s = 0; s < NAUI_RENDERER_CORNER_SEGMENTS; s++)
    {
        float cx = s_cos[s],     cy = s_sin[s];
        float nx = s_cos[s + 1], ny = s_sin[s + 1];

        Naui_Vec2 o0 = { center.x + ax * cx * outer_r, center.y + ay * cy * outer_r };
        Naui_Vec2 o1 = { center.x + ax * nx * outer_r, center.y + ay * ny * outer_r };
        Naui_Vec2 i0 = { center.x + ax * cx * inner_r, center.y + ay * cy * inner_r };
        Naui_Vec2 i1 = { center.x + ax * nx * inner_r, center.y + ay * ny * inner_r };

        push_quad4(o0, o1, i1, i0, color, -1);
    }
}

void naui_renderer_initialize(const char *default_font_path)
{
    data = calloc(1, sizeof(Naui_RendererData));

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

    data->batch_vb = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_VERTEX,
        .access = MGFX_ACCESS_CPU,
        .size   = sizeof(data->geometry_vertices)
    });

    data->batch_ib = mgfx_create_buffer(&(mgfx_buffer_create_info){
        .usage  = MGFX_BUFFER_USAGE_INDEX,
        .access = MGFX_ACCESS_CPU,
        .size   = sizeof(data->geometry_indices)
    });

    data->base_pipeline = mgfx_create_pipeline(&(mgfx_pipeline_create_info){
        .shader = get_base_shader(mgfx_get_shader_lang()),
        .vertex_attributes = {
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_FLOAT2,
            MGFX_VERTEX_FORMAT_UBYTE4N,
            MGFX_VERTEX_FORMAT_INT
        }
    });

    data->image_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .mag_filter     = MGFX_SAMPLER_FILTER_NEAREST,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    data->font_sampler = mgfx_create_sampler(&(mgfx_sampler_create_info){
        .min_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .mag_filter     = MGFX_SAMPLER_FILTER_LINEAR,
        .address_mode_u = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = MGFX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    });

    data->width = mg_app_width();
    data->height = mg_app_height();
}

void naui_renderer_shutdown(void)
{
    mgfx_destroy_sampler(data->image_sampler);
    mgfx_destroy_sampler(data->font_sampler);
    mgfx_destroy_buffer(data->batch_vb);
    mgfx_destroy_buffer(data->batch_ib);
    mgfx_destroy_pipeline(data->base_pipeline);
    mgfx_shutdown();
    free(data);
}

void naui_renderer_resize(int32_t width, int32_t height)
{
    mgfx_resize(width, height);
    data->width = width;
    data->height = height;
}

void naui_renderer_begin(void)
{
    data->geometry_count = 0;
}

void naui_renderer_end(void)
{
    mgfx_begin();
    mgfx_bind_pass(&(mgfx_pass_info){});
    mgfx_bind_pipeline(data->base_pipeline);

    struct
    {
        Naui_Vec2 u_resolution;
    }
    ub_data;
    ub_data.u_resolution = (Naui_Vec2){(float)data->width, (float)data->height};
    mgfx_bind_uniforms(0, sizeof(ub_data), &ub_data);

    mgfx_update_buffer(data->batch_vb, data->geometry_count * sizeof(Naui_BatchGeometry), data->geometry_vertices);
    mgfx_update_buffer(data->batch_ib, data->geometry_count * 6 * sizeof(uint32_t), data->geometry_indices);
    mgfx_bind_vertex_buffer(data->batch_vb);
    mgfx_bind_index_buffer(data->batch_ib, MGFX_INDEX_TYPE_UINT32);
    mgfx_draw_indexed(data->geometry_count * 6, 0, 0);
    mgfx_end();
}

void naui_fill_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color)
{
    Naui_Vec2 br = { position.x + scale.x, position.y + scale.y };
    push_rect(position, br, pack_color(color), -1);
}

void naui_draw_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float lw = line_width;
    uint32_t c = pack_color(color);

    push_rect((Naui_Vec2){x0,    y0     }, (Naui_Vec2){x1,    y0+lw  }, c, -1); // top
    push_rect((Naui_Vec2){x0,    y1-lw  }, (Naui_Vec2){x1,    y1     }, c, -1); // bottom
    push_rect((Naui_Vec2){x0,    y0+lw  }, (Naui_Vec2){x0+lw, y1-lw  }, c, -1); // left
    push_rect((Naui_Vec2){x1-lw, y0+lw  }, (Naui_Vec2){x1,    y1-lw  }, c, -1); // right
}

void naui_fill_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float rounding)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f);

    uint32_t c = pack_color(color);

    push_rect((Naui_Vec2){x0+r, y0  }, (Naui_Vec2){x1-r, y1  }, c, -1); // center
    push_rect((Naui_Vec2){x0,   y0+r}, (Naui_Vec2){x0+r, y1-r}, c, -1); // left wing
    push_rect((Naui_Vec2){x1-r, y0+r}, (Naui_Vec2){x1,   y1-r}, c, -1); // right wing

    push_corner_fan((Naui_Vec2){x0+r, y0+r}, r, -1, -1, c); // TL
    push_corner_fan((Naui_Vec2){x1-r, y0+r}, r, +1, -1, c); // TR
    push_corner_fan((Naui_Vec2){x1-r, y1-r}, r, +1, +1, c); // BR
    push_corner_fan((Naui_Vec2){x0+r, y1-r}, r, -1, +1, c); // BL
}

void naui_draw_round_rect(Naui_Vec2 position, Naui_Vec2 scale, Naui_Color color, float line_width, float rounding)
{
    float x0 = position.x, y0 = position.y;
    float x1 = x0 + scale.x, y1 = y0 + scale.y;
    float r = naui_min(rounding, naui_min(scale.x, scale.y) * 0.5f), lw = line_width;
    float inner_r = fmaxf(r - lw, 0.0f);
    uint32_t c = pack_color(color);

    push_rect((Naui_Vec2){x0+r, y0    }, (Naui_Vec2){x1-r, y0+lw }, c, -1); // top
    push_rect((Naui_Vec2){x0+r, y1-lw }, (Naui_Vec2){x1-r, y1    }, c, -1); // bottom
    push_rect((Naui_Vec2){x0,   y0+r  }, (Naui_Vec2){x0+lw, y1-r }, c, -1); // left
    push_rect((Naui_Vec2){x1-lw,y0+r  }, (Naui_Vec2){x1,    y1-r }, c, -1); // right

    push_corner_ring((Naui_Vec2){x0+r, y0+r}, inner_r, r, -1, -1, c); // TL
    push_corner_ring((Naui_Vec2){x1-r, y0+r}, inner_r, r, +1, -1, c); // TR
    push_corner_ring((Naui_Vec2){x1-r, y1-r}, inner_r, r, +1, +1, c); // BR
    push_corner_ring((Naui_Vec2){x0+r, y1-r}, inner_r, r, -1, +1, c); // BL
}

void naui_draw_line(Naui_Vec2 a, Naui_Vec2 b, Naui_Color color, float line_width)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float inv_len = line_width * 0.5f / sqrtf(dx * dx + dy * dy);
    if (inv_len > 1e10f) return;

    float nx = -dy * inv_len;
    float ny =  dx * inv_len;

    push_quad4(
        (Naui_Vec2){ a.x + nx, a.y + ny },
        (Naui_Vec2){ b.x + nx, b.y + ny },
        (Naui_Vec2){ b.x - nx, b.y - ny },
        (Naui_Vec2){ a.x - nx, a.y - ny },
        pack_color(color), -1
    );
}