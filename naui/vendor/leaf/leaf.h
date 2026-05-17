#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LEAF_COLOR_WHITE (Leaf_Color) { 255, 255, 255, 255 }

#define LEAF_API // TODO(box): implement proper dll/wasm exporting

typedef struct
{
    uint64_t value;
    const char *label;
}
Leaf_ID;

typedef struct
{
    float x, y;
}
Leaf_Vec2;

typedef struct
{
    float width, height;
}
Leaf_Dimensions;

typedef struct
{
    uint8_t r, g, b, a;
}
Leaf_Color;

typedef uint8_t Leaf_ColorFillType;
enum
{
    LEAF_SOLID_COLOR_FILL = 0,
    LEAF_GRADIENT_LINEAR_COLOR_FILL = 1
};

typedef struct
{
    Leaf_Color color1;
    Leaf_Color color2;
    float angle;
    Leaf_ColorFillType type;
}
Leaf_ColorFill;

#define leaf_solid(c) (Leaf_ColorFill){ (c), {}, 0.0f, LEAF_SOLID_COLOR_FILL }
#define leaf_gradient(c1, c2, angle) (Leaf_ColorFill){ (c1), (c2), angle, LEAF_GRADIENT_LINEAR_COLOR_FILL }

#define leaf_deg(v) ((v) * 0.0174532925f)
#define leaf_rad(v) (v)

static inline Leaf_Color leaf_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (Leaf_Color){ r, g, b, 255 };
}

static inline Leaf_Color leaf_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (Leaf_Color){ r, g, b, a };
}

static inline Leaf_Color leaf_hex(uint32_t hex)
{
    return (Leaf_Color){
        (uint8_t)((hex >> 16) & 0xFF),
        (uint8_t)((hex >>  8) & 0xFF),
        (uint8_t)((hex >>  0) & 0xFF),
        255
    };
}

static inline Leaf_Color leaf_hexa(uint32_t hex)
{
    return (Leaf_Color){
        (uint8_t)((hex >> 24) & 0xFF),
        (uint8_t)((hex >> 16) & 0xFF),
        (uint8_t)((hex >>  8) & 0xFF),
        (uint8_t)((hex >>  0) & 0xFF)
    };
}

typedef struct
{
    float x, y, width, height;
}
Leaf_BoundingBox;

typedef uint8_t Leaf_Positioning;
enum
{
    LEAF_POSITIONING_RELATIVE,
    LEAF_POSITIONING_FLOATING_TO_PARENT,
    LEAF_POSITIONING_FLOATING_TO_ROOT
};

typedef uint8_t Leaf_LayoutDirection;
enum
{
    LEAF_LAYOUT_VERTICAL,
    LEAF_LAYOUT_HORIZONAL
};

typedef uint8_t Leaf_LayoutAlignmentX;
enum
{
    LEAF_ALIGN_X_LEFT,
    LEAF_ALIGN_X_RIGHT,
    LEAF_ALIGN_X_CENTER
};

typedef uint8_t Leaf_LayoutAlignmentY;
enum
{
    LEAF_ALIGN_Y_TOP,
    LEAF_ALIGN_Y_BOTTOM,
    LEAF_ALIGN_Y_CENTER
};

typedef struct
{
    Leaf_LayoutAlignmentX x;
    Leaf_LayoutAlignmentY y;
}
Leaf_Alignment;

typedef uint8_t Leaf_SizingType;
enum
{
    LEAF_SIZING_TYPE_FIT,
    LEAF_SIZING_TYPE_GROW,
    LEAF_SIZING_TYPE_PERCENT,
    LEAF_SIZING_TYPE_FIXED
};

typedef struct
{
    struct
    {
        struct { float min, max; } min_max;
        float percent;
    }
    size;
    Leaf_SizingType type;
}
Leaf_SizingAxis;

typedef struct
{
    Leaf_SizingAxis width;
    Leaf_SizingAxis height;
}
Leaf_Sizing;

typedef struct
{
    float left, right, top, bottom;
}
Leaf_Padding;
#define LEAF_PADDING_ALL(v) (Leaf_Padding) { v, v, v, v }

typedef struct
{
    Leaf_ColorFill color;
    float width;
}
Leaf_Border;

typedef struct
{
    void *handle;
}
Leaf_Image;

typedef void (*Leaf_CustomDrawFn)(Leaf_BoundingBox box, void *user_data);

typedef struct
{
    Leaf_ID id;

    Leaf_Image image;
    Leaf_CustomDrawFn custom_draw;
    void *custom_draw_user_data;

    Leaf_Sizing sizing;
    Leaf_Padding padding;
    Leaf_Border border;
    Leaf_ColorFill color;
    Leaf_Vec2 floating_offset;
    Leaf_Vec2 child_offset;
    Leaf_Alignment child_alignment;
    Leaf_Alignment floating_alignment;

    float child_gap;
    float rounding;
    float aspect_ratio;

    Leaf_LayoutDirection direction;
    Leaf_Positioning positioning;
}
Leaf_ElementConfig;

typedef struct
{
    Leaf_BoundingBox bounding_box;
    uint64_t id;
}
Leaf_LayoutFrameEntry;

typedef uint8_t Leaf_TextAlignment;
enum
{
    LEAF_TEXT_ALIGN_LEFT,
    LEAF_TEXT_ALIGN_CENTER,
    LEAF_TEXT_ALIGN_RIGHT
};

typedef uint8_t Leaf_TextWrapMode;
enum
{
    LEAF_TEXT_WRAP_MODE_NONE,
    LEAF_TEXT_WRAP_MODE_CHAR,
    LEAF_TEXT_WRAP_MODE_WORD
};

typedef struct
{
    Leaf_ColorFill color;
    float font_size;
    uint32_t font_id;
    Leaf_TextAlignment alignment;
    Leaf_TextWrapMode wrap_mode;
}
Leaf_TextConfig;

typedef uint8_t Leaf_RenderCmdType;
enum
{
    LEAF_RENDER_CMD_RECT,
    LEAF_RENDER_CMD_RECT_LINES,
    LEAF_RENDER_CMD_TEXT,
    LEAF_RENDER_CMD_IMAGE,
    LEAF_RENDER_CMD_SCISSOR_PUSH,
    LEAF_RENDER_CMD_SCISSOR_POP,
    LEAF_RENDER_CMD_CUSTOM
};

typedef struct
{
    union
    {
        struct
        {
            float rounding;
            float line_width;
        }
        rect;

        struct
        {
            const char *text;
            float font_size;
            uint32_t font_id;
        }
        text;

        struct
        {
            void *handle;
            float rounding;
        }
        image;

        struct
        {
            Leaf_CustomDrawFn draw;
            void *user_data;
        }
        custom;
    };

    Leaf_RenderCmdType type;
    Leaf_BoundingBox bounding_box;
    Leaf_ColorFill color;
}
Leaf_RenderCmd;

typedef struct
{
    Leaf_RenderCmd *cmds;
    uint32_t count;
}
Leaf_RenderCmdList;

typedef Leaf_Dimensions (*Leaf_MeasureTextFn)(const char *text, uint32_t length, const Leaf_TextConfig *config);

#define LEAF_SIZING_GROW            (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_GROW }
#define LEAF_SIZING_GROW_MIN(v)     (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_GROW, .size.min_max.min = (v) }
#define LEAF_SIZING_GROW_MAX(v)     (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_GROW, .size.min_max.max = (v) }
#define LEAF_SIZING_FIT             (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_FIT  }
#define LEAF_SIZING_FIT_MIN(v)      (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_FIT,     .size.min_max.min = (v) }
#define LEAF_SIZING_FIXED(v)        (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_FIXED,   .size.min_max.min = (v) }
#define LEAF_SIZING_PERCENT(v)      (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_PERCENT, .size.percent     = (v) }
#define LEAF_SIZING_PERCENT_MIN(p, m)   (Leaf_SizingAxis){ .type = LEAF_SIZING_TYPE_PERCENT, .size.percent     = (p), .size.min_max.min = (m) }

#define leaf(...) \
    for (int leaf__i_ = (leaf_begin_element((Leaf_ElementConfig)__VA_ARGS__), 1); \
         leaf__i_; \
         leaf__i_ = (leaf_end_element(), 0))

LEAF_API Leaf_ID leaf_id(const char *label);
LEAF_API Leaf_ID leaf_id_indexed(const char *label, uint64_t index);

LEAF_API void leaf_initialize(void);
LEAF_API void leaf_shutdown(void);

LEAF_API void leaf_set_measure_text(Leaf_MeasureTextFn fn);
LEAF_API void leaf_set_pointer_pos(float x, float y);

LEAF_API void leaf_begin_element(Leaf_ElementConfig config);
LEAF_API void leaf_end_element(void);

LEAF_API bool leaf_hovered(Leaf_ID id);
LEAF_API Leaf_BoundingBox leaf_get_bounding_box(Leaf_ID id);

LEAF_API void leaf_begin_frame(int32_t width, int32_t height);
LEAF_API Leaf_RenderCmdList leaf_end_frame(void);

#define leaf_text(text, ...) do { Leaf_TextConfig __cfg = __VA_ARGS__; __leaf_text(text, __cfg); } while(0)
LEAF_API void __leaf_text(const char *text, Leaf_TextConfig config);

#ifndef LEAF_NO_DEBUG_TOOLS

#define leaf_debug(show, menu_width, delta_time, scroll_delta) \
    for (int leaf__i_ = (leaf_begin_debug_context(show, delta_time, scroll_delta), 1); \
         leaf__i_; \
         leaf__i_ = (leaf_end_debug_context(menu_width), 0))

LEAF_API void leaf_begin_debug_context(bool show, float delta_time, float scroll_delta);
LEAF_API void leaf_end_debug_context(float menu_width);

#endif

#ifdef LEAF_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t Leaf_NodeType;
enum
{
    LEAF_NODE_TYPE_ELEMENT,
    LEAF_NODE_TYPE_TEXT
};

typedef struct Leaf_Node Leaf_Node;
struct Leaf_Node
{
    union
    {
        struct
        {
            Leaf_ElementConfig config;
        }
        element;

        struct
        {
            Leaf_TextConfig config;
            const char *text;
            uint32_t size;
        }
        text;
    };

    Leaf_NodeType type;
    Leaf_BoundingBox bounding_box;

    Leaf_Node *parent;
    Leaf_Node *first_child;
    Leaf_Node *last_child;
    Leaf_Node *next_sibling;
    int32_t child_count;
};

#define LEAF_ASSERT(x, msg) if (!(x)) {fprintf(stderr, "[Leaf]: %s", msg); return;}
#define LEAF_ASSERT_NULL(x, msg) if (!(x)) {fprintf(stderr, "[Leaf]: %s", msg); return NULL;}

#ifndef LEAF_CONFIG_MAX_NODES
    #define LEAF_CONFIG_MAX_NODES (1<<13)
#endif

#ifndef LEAF_CONFIG_MAX_STACK
    #define LEAF_CONFIG_MAX_STACK 256
#endif

#ifndef LEAF_CONFIG_MAX_RENDER_CMDS
    #define LEAF_CONFIG_MAX_RENDER_CMDS (1<<13)
#endif

#ifndef LEAF_CONFIG_TEXT_WRAP_CACHE
    #define LEAF_CONFIG_TEXT_WRAP_CACHE (1<<14)
#endif

typedef struct
{
    Leaf_Node nodes[LEAF_CONFIG_MAX_NODES];
    uint32_t node_count;

    Leaf_LayoutFrameEntry layout_entires[LEAF_CONFIG_MAX_NODES];

    Leaf_Node *stack[LEAF_CONFIG_MAX_STACK];
    uint32_t stack_top;

    Leaf_RenderCmd render_cmds[LEAF_CONFIG_MAX_RENDER_CMDS];
    uint32_t render_cmd_count;

    char text_wrap_cache[LEAF_CONFIG_TEXT_WRAP_CACHE];
    uint32_t wrap_cache_cursor;

    Leaf_Vec2 pointer_pos;
    Leaf_MeasureTextFn measure_text;
}
Leaf_Context;

static Leaf_Context *leaf_ctx;

static inline int32_t leaf_max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

static inline float leaf_maxf(float a, float b)
{
    return a > b ? a : b;
}

static uint64_t leaf_murmur(const void *key, int len, uint64_t seed)
{
    const uint8_t *data = (const uint8_t *)key;
    uint64_t h = seed;
    for (int i = 0; i < len; i++)
    {
        uint64_t k = data[i];
        k *= 0x87c37b91114253d5ULL; k = (k << 31) | (k >> 33); k *= 0x4cf5ad432745937fULL;
        h ^= k; h = (h << 27) | (h >> 37); h = h * 5 + 0x52dce729ULL;
    }
    h ^= (uint64_t)len;
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h ? h : 1;
}

Leaf_ID leaf_id(const char *label)
{
    return (Leaf_ID){leaf_murmur(label, (int)strlen(label), 0), label};
}

Leaf_ID leaf_id_indexed(const char *label, uint64_t index)
{
    uint64_t base = leaf_murmur(label, (int)strlen(label), 0);
    return (Leaf_ID){leaf_murmur(label, (int)strlen(label), base + index), label};
}

void leaf_initialize(void)
{
    leaf_ctx = (Leaf_Context*)calloc(1, sizeof(Leaf_Context));
}

void leaf_shutdown(void)
{
    free(leaf_ctx);
}

void leaf_set_measure_text(Leaf_MeasureTextFn fn)
{
    leaf_ctx->measure_text = fn;
}

void leaf_set_pointer_pos(float x, float y)
{
    leaf_ctx->pointer_pos.x = x;
    leaf_ctx->pointer_pos.y = y;
}

static void leaf_set_layout_entry(Leaf_ID id, Leaf_LayoutFrameEntry entry)
{
    entry.id = id.value;
    uint32_t i = id.value % LEAF_CONFIG_MAX_NODES;
    while (leaf_ctx->layout_entires[i].id != 0 && leaf_ctx->layout_entires[i].id != id.value)
        i = (i + 1) % LEAF_CONFIG_MAX_NODES;
    leaf_ctx->layout_entires[i] = entry;
}

static Leaf_LayoutFrameEntry leaf_get_layout_entry(Leaf_ID id)
{
    uint32_t i = id.value % LEAF_CONFIG_MAX_NODES;
    while (leaf_ctx->layout_entires[i].id != 0 && leaf_ctx->layout_entires[i].id != id.value)
        i = (i + 1) % LEAF_CONFIG_MAX_NODES;
    return leaf_ctx->layout_entires[i];
}

static inline bool leaf_point_in_box(float px, float py, Leaf_BoundingBox bb)
{
    return px > bb.x && px <= bb.x + bb.width &&
        py > bb.y && py <= bb.y + bb.height;
}

bool leaf_hovered(Leaf_ID id)
{
    Leaf_BoundingBox bounding_box = leaf_get_layout_entry(id).bounding_box;
    return leaf_point_in_box(leaf_ctx->pointer_pos.x, leaf_ctx->pointer_pos.y, bounding_box);
}

Leaf_BoundingBox leaf_get_bounding_box(Leaf_ID id)
{
    return leaf_get_layout_entry(id).bounding_box;
}

static inline Leaf_Node *leaf_alloc_node(void)
{
    LEAF_ASSERT_NULL(leaf_ctx->node_count < LEAF_CONFIG_MAX_NODES,
        "Node limit exceeded. Increase LEAF_CONFIG_MAX_NODES.");
    Leaf_Node *node = &leaf_ctx->nodes[leaf_ctx->node_count++];
    memset(node, 0, sizeof(Leaf_Node));
    return node;
}

static inline void leaf_stack_push(Leaf_Node *node)
{
    LEAF_ASSERT(leaf_ctx->stack_top < LEAF_CONFIG_MAX_STACK,
        "Stack limit exceeded. Increase LEAF_CONFIG_MAX_STACK.");
    leaf_ctx->stack[leaf_ctx->stack_top++] = node;
}

static inline void leaf_stack_pop(void)
{
    leaf_ctx->stack_top--;
}

static inline Leaf_Node *leaf_stack_top(void)
{
    return leaf_ctx->stack[leaf_ctx->stack_top - 1];
}

static inline void leaf_append_child(Leaf_Node *parent, Leaf_Node *child)
{
    child->parent = parent;
    child->next_sibling = NULL;
    parent->child_count++;

    if (!parent->first_child)
    {
        parent->first_child = child;
        parent->last_child = child;
    }
    else
    {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }
}

void __leaf_text(const char *text, Leaf_TextConfig config)
{
    Leaf_Node *parent = leaf_stack_top();
    Leaf_Node *node = leaf_alloc_node();

    node->type = LEAF_NODE_TYPE_TEXT;
    node->text.config = config;
    node->text.text = text;
    node->text.size = (uint32_t)strlen(text);

    Leaf_Dimensions size = leaf_ctx->measure_text(text, node->text.size, &config);
    node->bounding_box.width  = size.width;
    node->bounding_box.height = size.height;

    leaf_append_child(parent, node);
}

void leaf_begin_element(Leaf_ElementConfig config)
{
    Leaf_Node *node = leaf_alloc_node();

    node->type = LEAF_NODE_TYPE_ELEMENT;
    node->element.config = config;

    if (config.sizing.width.type == LEAF_SIZING_TYPE_FIXED)
        node->bounding_box.width = config.sizing.width.size.min_max.min;
    if (config.sizing.height.type == LEAF_SIZING_TYPE_FIXED)
        node->bounding_box.height = config.sizing.height.size.min_max.min;
    
    node->parent = leaf_stack_top();
    leaf_stack_push(node);
}

void leaf_end_element(void)
{
    Leaf_Node *node = leaf_stack_top();
    Leaf_Node *parent = node->parent;

    const Leaf_ElementConfig *parent_config = &parent->element.config;
    const Leaf_ElementConfig *child_config = &node->element.config;

    node->bounding_box.width += child_config->padding.left + child_config->padding.right;
    node->bounding_box.height += child_config->padding.top + child_config->padding.bottom;

    const float child_gap = leaf_max(node->child_count - 1, 0) * child_config->child_gap;
    if (child_config->sizing.width.type == LEAF_SIZING_TYPE_FIT && child_config->direction == LEAF_LAYOUT_HORIZONAL)
        node->bounding_box.width += child_gap;
    else if (child_config->sizing.height.type == LEAF_SIZING_TYPE_FIT && child_config->direction == LEAF_LAYOUT_VERTICAL)
        node->bounding_box.height += child_gap;

    if (node->parent)
        leaf_append_child(node->parent, node);

    leaf_stack_pop();
}

#define LEAF_FOREACH_CHILD(x, _parent)\
    for (Leaf_Node *x = _parent->first_child; x != NULL; x = x->next_sibling)

static inline void leaf_push_render_cmd(Leaf_RenderCmd cmd)
{
    LEAF_ASSERT(leaf_ctx->render_cmd_count < LEAF_CONFIG_MAX_RENDER_CMDS,
        "Render command limit exceeded. Increase LEAF_CONFIG_MAX_RENDER_CMDS.");
    leaf_ctx->render_cmds[leaf_ctx->render_cmd_count++] = cmd;
}

static inline void leaf_element_clamp_min_max(Leaf_Node *node, const Leaf_ElementConfig *config)
{
    float mn = config->sizing.width.size.min_max.min;
    float mx = config->sizing.width.size.min_max.max;
    if (mn > 0 && node->bounding_box.width < mn)
        node->bounding_box.width = mn;
    if (mx > 0 && node->bounding_box.width > mx)
        node->bounding_box.width = mx;

    mn = config->sizing.height.size.min_max.min;
    mx = config->sizing.height.size.min_max.max;
    if (mn > 0 && node->bounding_box.height < mn)
        node->bounding_box.height = mn;
    if (mx > 0 && node->bounding_box.height > mx)
        node->bounding_box.height = mx;
}

static inline bool leaf_is_color_fill_empty(Leaf_ColorFill fill)
{
    return fill.color1.a == 0 && fill.color2.a == 0;
}

static void leaf_render_node(Leaf_Node *node)
{
    if (node->bounding_box.width <= 0 || node->bounding_box.height <= 0)
        return;

    switch (node->type)
    {
    case LEAF_NODE_TYPE_ELEMENT:
    {
        const Leaf_ElementConfig *config = &node->element.config;
        if (!leaf_is_color_fill_empty(config->color))
        {
            if (config->image.handle)
            {
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_IMAGE,
                    .color = config->color,
                    .bounding_box = node->bounding_box,
                    .image.handle = config->image.handle,
                    .image.rounding = config->rounding
                });
            }
            else
            {
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_RECT,
                    .color = config->color,
                    .bounding_box = node->bounding_box,
                    .rect.rounding = config->rounding,
                });
            }
        }

        if (config->custom_draw)
        {
            leaf_push_render_cmd((Leaf_RenderCmd){
                .type = LEAF_RENDER_CMD_CUSTOM,
                .bounding_box = node->bounding_box,
                .custom.draw = config->custom_draw,
                .custom.user_data = config->custom_draw_user_data
            });
        }

        if (!leaf_is_color_fill_empty(config->border.color))
            leaf_push_render_cmd((Leaf_RenderCmd){
                .type = LEAF_RENDER_CMD_RECT_LINES,
                .color = config->border.color,
                .bounding_box = node->bounding_box,
                .rect.rounding = config->rounding,
                .rect.line_width = config->border.width
            });
        break;
    }
    case LEAF_NODE_TYPE_TEXT:
    {
        const Leaf_TextConfig *config = &node->text.config;
        if (!leaf_is_color_fill_empty(config->color))
            leaf_push_render_cmd((Leaf_RenderCmd){
                .type = LEAF_RENDER_CMD_TEXT,
                .color = config->color,
                .bounding_box = node->bounding_box,
                .text.text = node->text.text,
                .text.font_size = config->font_size,
                .text.font_id = config->font_id
            });
        break;
    }
    }
}

static const char *leaf_cache_str(const char *src, uint32_t size)
{
    LEAF_ASSERT_NULL(leaf_ctx->wrap_cache_cursor + size + 1 <= LEAF_CONFIG_TEXT_WRAP_CACHE,
        "Text wrap cache limit exceeded. Increase LEAF_CONFIG_TEXT_WRAP_CACHE.");
    char *dst = &leaf_ctx->text_wrap_cache[leaf_ctx->wrap_cache_cursor];
    memcpy(dst, src, size);
    dst[size] = '\0';
    leaf_ctx->wrap_cache_cursor += size + 1;
    return dst;
}

static inline void leaf_char_from_utf8(const char *str, uint32_t i, int *out_cp, int *out_bytes)
{
    unsigned char c = (unsigned char)str[i];
    if      (c < 0x80) { *out_cp = c;        *out_bytes = 1; }
    else if (c < 0xE0) { *out_cp = c & 0x1F; *out_bytes = 2; }
    else if (c < 0xF0) { *out_cp = c & 0x0F; *out_bytes = 3; }
    else               { *out_cp = c & 0x07; *out_bytes = 4; }
    for (int b = 1; b < *out_bytes; b++)
        *out_cp = (*out_cp << 6) | ((unsigned char)str[i + b] & 0x3F);
}

static Leaf_Node *leaf_wrap_text_node(Leaf_Node *parent, Leaf_Node *node, float avail_width)
{
    const Leaf_TextConfig *cfg  = &node->text.config;
    const char *text = node->text.text;
    uint32_t len = node->text.size;
    bool word_mode = cfg->wrap_mode == LEAF_TEXT_WRAP_MODE_WORD;

    Leaf_Node *last_inserted = node;
    uint32_t line_start = 0;

    while (line_start < len)
    {
        uint32_t line_end = line_start;
        uint32_t last_word_break = line_start;
        uint32_t i = line_start;

        while (i <= len)
        {
            int cp = 0, cp_bytes = 1;
            if (i < len)
                leaf_char_from_utf8(text, i, &cp, &cp_bytes);

            if (word_mode && (i == len || cp == ' '))
                last_word_break = i;

            Leaf_Dimensions d = leaf_ctx->measure_text(text + line_start, i - line_start, cfg);

            if (d.width > avail_width)
            {
                if (word_mode && last_word_break > line_start)
                    line_end = last_word_break;
                else
                    line_end = i;
                break;
            }

            line_end = i;
            if (i >= len) break;
            i += cp_bytes;
        }

        uint32_t seg_start = line_start;
        if (word_mode && seg_start > 0 && text[seg_start] == ' ')
            seg_start++;

        uint32_t seg_len = (line_end > seg_start) ? line_end - seg_start : 0;
        const char *seg  = leaf_cache_str(text + seg_start, seg_len);
        if (!seg) return last_inserted;

        Leaf_Dimensions d = leaf_ctx->measure_text(seg, seg_len, cfg);

        if (line_start == 0)
        {
            node->text.text   = seg;
            node->text.size = seg_len;
            node->bounding_box.width  = d.width;
            node->bounding_box.height = d.height;
        }
        else
        {
            Leaf_Node *line_node = leaf_alloc_node();
            line_node->type = LEAF_NODE_TYPE_TEXT;
            line_node->text.config = *cfg;
            line_node->text.text   = seg;
            line_node->text.size = seg_len;
            line_node->bounding_box.width  = d.width;
            line_node->bounding_box.height = d.height;
            line_node->parent = parent;

            line_node->next_sibling = last_inserted->next_sibling;
            last_inserted->next_sibling = line_node;
            if (parent->last_child == last_inserted)
                parent->last_child = line_node;
            parent->child_count++;

            last_inserted = line_node;
        }

        uint32_t next_start = line_end;
        if (word_mode && next_start < len && text[next_start] == ' ')
            next_start++;

        if (next_start <= line_start)
        {
            int cp = 0, cp_bytes = 1;
            if (line_start < len)
                leaf_char_from_utf8(text, line_start, &cp, &cp_bytes);
            next_start = line_start + cp_bytes;
        }

        if (next_start >= len) break;
        line_start = next_start;
    }

    return last_inserted;
}

static void leaf_wrap_text_children(Leaf_Node *parent)
{
    if (parent->type != LEAF_NODE_TYPE_ELEMENT) return;

    const Leaf_ElementConfig *cfg = &parent->element.config;
    float avail = parent->bounding_box.width - cfg->padding.left - cfg->padding.right;

    bool fit_w = cfg->sizing.width.type == LEAF_SIZING_TYPE_FIT;
    bool fit_h = cfg->sizing.height.type == LEAF_SIZING_TYPE_FIT;
    if (fit_w) parent->bounding_box.width  = cfg->padding.left + cfg->padding.right;
    if (fit_h) parent->bounding_box.height = cfg->padding.top  + cfg->padding.bottom;

    Leaf_Node *child = parent->first_child;
    while (child)
    {
        if (child->type == LEAF_NODE_TYPE_TEXT &&
            child->text.config.wrap_mode != LEAF_TEXT_WRAP_MODE_NONE &&
            child->bounding_box.width > avail)
            child = leaf_wrap_text_node(parent, child, avail);

        if (fit_w)
        {
            if (cfg->direction == LEAF_LAYOUT_HORIZONAL)
                parent->bounding_box.width += child->bounding_box.width;
            else parent->bounding_box.width = leaf_maxf(
                    child->bounding_box.width  + cfg->padding.left + cfg->padding.right,
                    parent->bounding_box.width);
        }
        if (fit_h)
        {
            if (cfg->direction == LEAF_LAYOUT_HORIZONAL)
                parent->bounding_box.height = leaf_maxf(
                    child->bounding_box.height + cfg->padding.top + cfg->padding.bottom,
                    parent->bounding_box.height);
            else
                parent->bounding_box.height += child->bounding_box.height;
        }

        child = child->next_sibling;
    }

    float gap = leaf_max(parent->child_count - 1, 0) * cfg->child_gap;
    if (fit_h && cfg->direction == LEAF_LAYOUT_VERTICAL)
        parent->bounding_box.height += gap;
    if (fit_w && cfg->direction == LEAF_LAYOUT_HORIZONAL)
        parent->bounding_box.width  += gap;
}

static void leaf_size_pass(Leaf_Node *parent)
{
    if (parent->type != LEAF_NODE_TYPE_ELEMENT)
        return;

    const Leaf_ElementConfig *config = &parent->element.config;
    bool h = config->direction == LEAF_LAYOUT_HORIZONAL;

    int32_t growing_width_count  = 0;
    int32_t growing_height_count = 0;
    float   free_width  = parent->bounding_box.width;
    float   free_height = parent->bounding_box.height;

    const float child_gap = leaf_max(parent->child_count - 1, 0) * config->child_gap;
    if (h)  free_width  -= child_gap;
    else    free_height -= child_gap;
    free_width  -= config->padding.left + config->padding.right;
    free_height -= config->padding.top  + config->padding.bottom;

    LEAF_FOREACH_CHILD(child, parent)
    {
        if (child->type != LEAF_NODE_TYPE_ELEMENT)
        {
            if (h) free_width  -= child->bounding_box.width;
            else   free_height -= child->bounding_box.height;
            continue;
        }

        const Leaf_ElementConfig *cc = &child->element.config;

        if (cc->sizing.width.type == LEAF_SIZING_TYPE_PERCENT)
        {
            float avail = parent->bounding_box.width - config->padding.left - config->padding.right;
            child->bounding_box.width = avail * cc->sizing.width.size.percent;
        }
        if (cc->sizing.height.type == LEAF_SIZING_TYPE_PERCENT)
        {
            float avail = parent->bounding_box.height - config->padding.top - config->padding.bottom;
            child->bounding_box.height = avail * cc->sizing.height.size.percent;
        }

        if (cc->positioning != LEAF_POSITIONING_RELATIVE)
            continue;

        if (cc->sizing.width.type == LEAF_SIZING_TYPE_GROW)
            growing_width_count++;
        else if (h)
            free_width -= child->bounding_box.width;

        if (cc->sizing.height.type == LEAF_SIZING_TYPE_GROW)
            growing_height_count++;
        else if (!h)
            free_height -= child->bounding_box.height;
    }

    if (growing_width_count  == 0) growing_width_count  = 1;
    if (growing_height_count == 0) growing_height_count = 1;

    LEAF_FOREACH_CHILD(child, parent)
    {
        if (child->type != LEAF_NODE_TYPE_ELEMENT)
            continue;

        const Leaf_ElementConfig *cc = &child->element.config;
        if (cc->positioning == LEAF_POSITIONING_RELATIVE) {
            if (cc->sizing.width.type == LEAF_SIZING_TYPE_GROW) {
                child->bounding_box.width = free_width;
                if (h) child->bounding_box.width /= growing_width_count;
            }
            if (cc->sizing.height.type == LEAF_SIZING_TYPE_GROW) {
                child->bounding_box.height = free_height;
                if (!h) child->bounding_box.height /= growing_height_count;
            }
        }

        leaf_element_clamp_min_max(child, cc);
    }

    LEAF_FOREACH_CHILD(child, parent)
        leaf_size_pass(child);

    leaf_wrap_text_children(parent);

    bool fit_w = config->sizing.width.type  == LEAF_SIZING_TYPE_FIT;
    bool fit_h = config->sizing.height.type == LEAF_SIZING_TYPE_FIT;

    if (fit_w || fit_h)
    {
        if (fit_w) parent->bounding_box.width  = config->padding.left + config->padding.right;
        if (fit_h) parent->bounding_box.height = config->padding.top  + config->padding.bottom;

        LEAF_FOREACH_CHILD(child, parent)
        {
            if (fit_w)
            {
                if (config->direction == LEAF_LAYOUT_HORIZONAL)
                    parent->bounding_box.width += child->bounding_box.width;
                else
                    parent->bounding_box.width = leaf_maxf(
                        child->bounding_box.width + config->padding.left + config->padding.right,
                        parent->bounding_box.width);
            }
            if (fit_h)
            {
                if (config->direction == LEAF_LAYOUT_HORIZONAL)
                    parent->bounding_box.height = leaf_maxf(
                        child->bounding_box.height + config->padding.top + config->padding.bottom,
                        parent->bounding_box.height);
                else
                    parent->bounding_box.height += child->bounding_box.height;
            }
        }

        float gap = leaf_max(parent->child_count - 1, 0) * config->child_gap;
        if (fit_h && config->direction == LEAF_LAYOUT_VERTICAL)
            parent->bounding_box.height += gap;
        if (fit_w && config->direction == LEAF_LAYOUT_HORIZONAL)
            parent->bounding_box.width  += gap;
    }

    if (config->aspect_ratio > 0.0f)
    {
        if (config->sizing.height.type == LEAF_SIZING_TYPE_FIT)
            parent->bounding_box.height = parent->bounding_box.width / config->aspect_ratio;
        else parent->bounding_box.width = parent->bounding_box.height * config->aspect_ratio;
    }
}

static void leaf_position_render(Leaf_Node *parent)
{
    if (parent->type != LEAF_NODE_TYPE_ELEMENT)
        return;

#define LEAF_MAIN(horizontal, a, b)   ((horizontal) ? (a) : (b))
#define LEAF_CROSS(horizontal, a, b)  ((horizontal) ? (b) : (a))
#define LEAF_ALIGN_OFFSET(align, end_val, center_val, free) \
    ((align) == (end_val)    ? (free) :                     \
     (align) == (center_val) ? (free) * 0.5f : 0)

    const Leaf_ElementConfig *config = &parent->element.config;
    bool h = config->direction == LEAF_LAYOUT_HORIZONAL;

    float available_main  =
        LEAF_MAIN (h, parent->bounding_box.width,  parent->bounding_box.height) -
        LEAF_MAIN (h, config->padding.left + config->padding.right,
                      config->padding.top  + config->padding.bottom);
    float available_cross =
        LEAF_CROSS(h, parent->bounding_box.width,  parent->bounding_box.height) -
        LEAF_CROSS(h, config->padding.left + config->padding.right,
                      config->padding.top  + config->padding.bottom);

    float children_total = 0;
    LEAF_FOREACH_CHILD(child, parent)
    {
        if (child->type == LEAF_NODE_TYPE_ELEMENT &&
            child->element.config.positioning != LEAF_POSITIONING_RELATIVE)
            continue;
        children_total += LEAF_MAIN(h, child->bounding_box.width,
                                       child->bounding_box.height);
    }
    children_total += leaf_max(parent->child_count - 1, 0) * config->child_gap;

    float free_main = available_main - children_total;

    float main_offset =
        LEAF_MAIN(h, config->padding.left, config->padding.top) +
        LEAF_ALIGN_OFFSET(
            LEAF_MAIN(h, config->child_alignment.x, config->child_alignment.y),
            LEAF_MAIN(h, LEAF_ALIGN_X_RIGHT,   LEAF_ALIGN_Y_BOTTOM),
            LEAF_MAIN(h, LEAF_ALIGN_X_CENTER,  LEAF_ALIGN_Y_CENTER),
            free_main);

    LEAF_FOREACH_CHILD(child, parent)
    {
        if (child->type == LEAF_NODE_TYPE_ELEMENT)
        {
            const Leaf_ElementConfig *cc = &child->element.config;

            if (cc->positioning != LEAF_POSITIONING_RELATIVE)
            {
                Leaf_BoundingBox anchor;
                if (cc->positioning == LEAF_POSITIONING_FLOATING_TO_PARENT)
                    anchor = parent->bounding_box;
                else
                    anchor = leaf_ctx->stack[0]->bounding_box;

                float align_x = 0.f, align_y = 0.f;
                if (cc->floating_alignment.x == LEAF_ALIGN_X_RIGHT)
                    align_x = anchor.width  - child->bounding_box.width;
                else if (cc->floating_alignment.x == LEAF_ALIGN_X_CENTER)
                    align_x = (anchor.width  - child->bounding_box.width)  * 0.5f;
                if (cc->floating_alignment.y == LEAF_ALIGN_Y_BOTTOM)
                    align_y = anchor.height - child->bounding_box.height;
                else if (cc->floating_alignment.y == LEAF_ALIGN_Y_CENTER)
                    align_y = (anchor.height - child->bounding_box.height) * 0.5f;

                child->bounding_box.x = anchor.x + align_x + cc->floating_offset.x;
                child->bounding_box.y = anchor.y + align_y + cc->floating_offset.y;

                leaf_render_node(child);
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_SCISSOR_PUSH,
                    .bounding_box = child->bounding_box
                });
                leaf_position_render(child);
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_SCISSOR_POP
                });
                continue;
            }
        }

        // relative child
        {
            float layout_w = (child->type == LEAF_NODE_TYPE_TEXT) ? 0.f : child->bounding_box.width;
            float layout_h = (child->type == LEAF_NODE_TYPE_TEXT) ? 0.f : child->bounding_box.height;

            float child_cross = LEAF_CROSS(h, layout_w, layout_h);
            float free_cross  = available_cross - child_cross;
            float cross_offset =
                LEAF_CROSS(h, config->padding.left, config->padding.top) +
                LEAF_ALIGN_OFFSET(
                    LEAF_CROSS(h, config->child_alignment.x, config->child_alignment.y),
                    LEAF_CROSS(h, LEAF_ALIGN_X_RIGHT,  LEAF_ALIGN_Y_BOTTOM),
                    LEAF_CROSS(h, LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER),
                    free_cross);

            child->bounding_box.x = parent->bounding_box.x
                + LEAF_MAIN (h, main_offset,  cross_offset)
                - config->child_offset.x;
            child->bounding_box.y = parent->bounding_box.y
                + LEAF_CROSS(h, main_offset,  cross_offset)
                - config->child_offset.y;

            if (child->type == LEAF_NODE_TYPE_TEXT) {
                const Leaf_TextConfig *tc = &child->text.config;
                if (tc->alignment == LEAF_TEXT_ALIGN_CENTER)
                    child->bounding_box.x -= child->bounding_box.width * 0.5f;
                else if (tc->alignment == LEAF_TEXT_ALIGN_RIGHT)
                    child->bounding_box.x -= child->bounding_box.width;
            }

            leaf_render_node(child);

            main_offset += LEAF_MAIN(h, child->bounding_box.width, child->bounding_box.height) + config->child_gap;

            if (child->type == LEAF_NODE_TYPE_ELEMENT)
            {
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_SCISSOR_PUSH,
                    .bounding_box = child->bounding_box
                });
                leaf_position_render(child);
                leaf_push_render_cmd((Leaf_RenderCmd){
                    .type = LEAF_RENDER_CMD_SCISSOR_POP
                });
            }
        }
    }

    Leaf_ID id = config->id;
    if (id.value)
        leaf_set_layout_entry(id, (Leaf_LayoutFrameEntry){ parent->bounding_box });

#undef LEAF_MAIN
#undef LEAF_CROSS
#undef LEAF_ALIGN_OFFSET
}

#ifndef LEAF_NO_DEBUG_TOOLS
static Leaf_Node *leaf_debug_selected_node;
static Leaf_BoundingBox leaf_debug_selected_bounding_box;
#endif

void leaf_begin_frame(int32_t width, int32_t height)
{
    leaf_ctx->stack_top = 0;
    leaf_ctx->node_count = 0;
    leaf_ctx->render_cmd_count = 0;
    leaf_ctx->wrap_cache_cursor = 0;

#ifndef LEAF_NO_DEBUG_TOOLS
    leaf_debug_selected_node = NULL;
#endif

    Leaf_Node *root = leaf_alloc_node();
    root->bounding_box.width = (float)width;
    root->bounding_box.height = (float)height;
    leaf_stack_push(root);
}

Leaf_RenderCmdList leaf_end_frame(void)
{
    Leaf_Node *root = leaf_stack_top();
    leaf_size_pass(root);
    leaf_position_render(root);

#ifndef LEAF_NO_DEBUG_TOOLS
    if (leaf_debug_selected_node)
    {
        leaf_debug_selected_bounding_box = leaf_debug_selected_node->bounding_box;
        leaf_push_render_cmd((Leaf_RenderCmd){
            .type = LEAF_RENDER_CMD_RECT,
            .bounding_box = leaf_debug_selected_node->bounding_box,
            .color = leaf_solid(leaf_rgba(150, 170, 255, 100))
        });
    }
#endif

    Leaf_RenderCmdList list;
    list.cmds = leaf_ctx->render_cmds;
    list.count = leaf_ctx->render_cmd_count;
    return list;
}

#ifndef LEAF_NO_DEBUG_TOOLS
static const float LEAF_BASE_DEBUG_FONT_SIZE = 20.0f;

static const Leaf_Color LEAF_DBG_BG1 = {37, 35, 33, 255};
static const Leaf_Color LEAF_DBG_BG2 = {46, 44, 42, 255};
static const Leaf_Color LEAF_DBG_BG3 = {62, 60, 58, 255};
static const Leaf_Color LEAF_DBG_SELECTED = {102, 80, 78, 255};
static const Leaf_Color LEAF_DBG_TEXT_PRI = {237, 226, 231, 255};
static const Leaf_Color LEAF_DBG_TEXT_SEC = {141, 133, 135, 255};
static const Leaf_Color LEAF_DBG_BORDER = {90, 88, 85, 255};

static bool leaf_debug_menu_secondary_tab = false;
static int32_t leaf_debug_button_index;
static float leaf_debug_menu_show_factor = 0.0f;
static float leaf_debug_scroll_offset = 0.0f;
static float leaf_debug_scroll_offset_smooth = 0.0f;

static inline float leaf_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void leaf_debug_child_tab(Leaf_Node *node, int32_t level)
{
    static const char *labels[] = {
        "Element",
        "Text",
        "Image"
    };
    leaf_debug_menu_secondary_tab = !leaf_debug_menu_secondary_tab;
    const Leaf_ID id = leaf_id_indexed("__leaf_debug_button", leaf_debug_button_index++);
    bool hovered;
    if ((hovered = leaf_hovered(id)))
        leaf_debug_selected_node = node;
    leaf({
        .id = id,
        .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_FIT},
        .color = leaf_solid(hovered ? LEAF_DBG_SELECTED : leaf_debug_menu_secondary_tab ? LEAF_DBG_BG2 : LEAF_DBG_BG1),
        .padding = {16.0f + level * 24.0f, 16, 16, 16},
        .direction = LEAF_LAYOUT_HORIZONAL,
        .child_gap = 32.0f
    })
    {
        leaf_text((node->type == LEAF_NODE_TYPE_ELEMENT && node->element.config.id.label) ?
            node->element.config.id.label : labels[node->type], {
            .font_size = LEAF_BASE_DEBUG_FONT_SIZE,
            .color = leaf_solid(LEAF_DBG_TEXT_PRI)
        });

        if (node->type == LEAF_NODE_TYPE_TEXT)
        {
            leaf_text(node->text.text, {
                .font_size = LEAF_BASE_DEBUG_FONT_SIZE,
                .color = leaf_solid(LEAF_DBG_TEXT_SEC)
            });
        }
    }
    LEAF_FOREACH_CHILD(child, node)
        leaf_debug_child_tab(child, level + 1);
}

void leaf_begin_debug_context(bool show, float delta_time, float scroll_delta)
{
    leaf_debug_menu_secondary_tab = false;
    leaf_debug_button_index = 0;
    if (leaf_hovered(leaf_id("__leaf_debug_menu")))
        leaf_debug_scroll_offset = leaf_maxf(0.0f, leaf_debug_scroll_offset - scroll_delta * 100.0f);
    leaf_debug_menu_show_factor = leaf_lerp(leaf_debug_menu_show_factor, show ? 1.0f : 0.0f, delta_time * 16.0f);
    leaf_debug_scroll_offset_smooth = leaf_lerp(leaf_debug_scroll_offset_smooth, leaf_debug_scroll_offset, delta_time * 25.0f);
    leaf_begin_element((Leaf_ElementConfig){
        .direction = LEAF_LAYOUT_HORIZONAL,
        .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_TOP},
        .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_GROW}
    });
}

void leaf_end_debug_context(float menu_width)
{
    Leaf_Node *inner_root = leaf_stack_top();
    leaf({
        .sizing = {LEAF_SIZING_FIXED(menu_width * leaf_debug_menu_show_factor), LEAF_SIZING_GROW},
        .color = leaf_solid(LEAF_DBG_BG1),
        .border = {leaf_solid(LEAF_DBG_BORDER), 1.0f}
    })
    {
        leaf({
            .id = leaf_id("__leaf_debug_menu"),
            .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_PERCENT(0.6f)},
            .color = leaf_solid(LEAF_DBG_BG1),
            .child_offset = {0.0f, leaf_debug_scroll_offset_smooth}
        })
        {
            leaf({
                .sizing = { LEAF_SIZING_GROW, LEAF_SIZING_FIT },
                .padding = {16, 16, 16, 16},
            })
            {
                leaf_text("Leaf Debug Tools", {
                    .font_size = LEAF_BASE_DEBUG_FONT_SIZE,
                    .color = leaf_solid(LEAF_DBG_TEXT_PRI)
                });
            }
            leaf({
                .sizing = { LEAF_SIZING_GROW, LEAF_SIZING_FIXED(1.0f) },
                .color = leaf_solid(LEAF_DBG_BORDER)
            });

            leaf_debug_child_tab(inner_root, 0);
        }

        leaf({
            .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_FIXED(1.0f)},
            .color = leaf_solid(LEAF_DBG_BG3),
        });

        leaf({
            .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_GROW},
            .padding = {16, 16, 16, 16},
            .color = leaf_solid(LEAF_DBG_BG1),
            .child_gap = 20.0f
        })
        {
            if (!leaf_debug_selected_node)
                continue;
            const Leaf_TextConfig title = { .font_size = LEAF_BASE_DEBUG_FONT_SIZE, .color = leaf_solid(LEAF_DBG_TEXT_SEC) };
            const Leaf_TextConfig data = { .font_size = LEAF_BASE_DEBUG_FONT_SIZE, .color = leaf_solid(LEAF_DBG_TEXT_PRI) };

            leaf({})
            {
                leaf_text("Bounding Box", title);
                static char buffer[256];
                snprintf(buffer, sizeof(buffer), "{ x: %i, y: %i, width: %i, height: %i }",
                    (int)leaf_debug_selected_bounding_box.x, (int)leaf_debug_selected_bounding_box.y,
                    (int)leaf_debug_selected_bounding_box.width, (int)leaf_debug_selected_bounding_box.height);
                leaf_text(buffer, data);
            }

            if (leaf_debug_selected_node->type == LEAF_NODE_TYPE_ELEMENT)
            {
                Leaf_ElementConfig config = leaf_debug_selected_node->element.config;
                leaf({}){
                    leaf_text("Direction", title);
                    leaf_text(config.direction == LEAF_LAYOUT_HORIZONAL ? "Horizontal" : "Vertical", data);
                }

                leaf({}){
                    static const char *labels[] = {
                        "Fit",
                        "Grow",
                        "Percent",
                        "Fixed"
                    };
                    static char width[16], height[16];
                    snprintf(width, sizeof(width), "Width: %s", labels[config.sizing.width.type]);
                    snprintf(height, sizeof(height), "Height: %s", labels[config.sizing.height.type]);
                    leaf_text("Sizing", title);
                    leaf_text(width, data);
                    leaf_text(height, data);
                }

                leaf({}){
                    static char buffer[16];
                    snprintf(buffer, sizeof(buffer), "%d", (int)config.child_gap);
                    leaf_text("Child Gap", title);
                    leaf_text(buffer, data);
                }
            }
        }
    }
    leaf_end_element();
}
#endif // LEAF_NO_DEBUG_TOOLS

#endif // LEAF_IMPLEMENTATION

#ifdef __cplusplus
}
#endif