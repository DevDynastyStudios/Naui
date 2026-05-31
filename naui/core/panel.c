#include "panel.h"
#include "themes.h"
#include "input.h"
#include "utils/map.h"
#include "utils/list.h"

#include <leaf/leaf.h>
#include <stdio.h>

#define NAUI_MIN_PANEL_SIZE 120.0f
#define NAUI_RESIZE_BORDER_WIDTH 8.0f

#define NAUI_PANEL_TITLEBAR_ID "__naui_panel_titlebar"
#define NAUI_PANEL_TAB_ID "__naui_panel_tab"
#define NAUI_PANEL_BODY_ID "__naui_panel_body"
#define NAUI_PANEL_CLOSE_ID "__naui_panel_close"

typedef uint8_t Naui_SplitAxis;
enum
{
    NAUI_SPLIT_AXIS_VERTICAL   = LEAF_LAYOUT_VERTICAL,
    NAUI_SPLIT_AXIS_HORIZONTAL = LEAF_LAYOUT_HORIZONAL
};

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    char            title[64];
    Naui_PanelType  type;
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    Naui_PanelNode  *root;
    Naui_List(Naui_PanelNode*) tabs;
    void           *user_data;
    Naui_Vec2       position, size;
    Naui_PanelFlags flags;
    int32_t         active_tab;
    float           split_ratio;
    Naui_SplitAxis  split_axis;
};

typedef uint8_t Naui_ResizeHandle;
enum
{
    NAUI_RESIZE_NONE        = 0,
    NAUI_RESIZE_LEFT        = 1 << 0,
    NAUI_RESIZE_RIGHT       = 1 << 1,
    NAUI_RESIZE_TOP         = 1 << 2,
    NAUI_RESIZE_BOTTOM      = 1 << 3,
    NAUI_RESIZE_TOP_LEFT    = NAUI_RESIZE_TOP    | NAUI_RESIZE_LEFT,
    NAUI_RESIZE_TOP_RIGHT   = NAUI_RESIZE_TOP    | NAUI_RESIZE_RIGHT,
    NAUI_RESIZE_BOTTOM_LEFT = NAUI_RESIZE_BOTTOM | NAUI_RESIZE_LEFT,
    NAUI_RESIZE_BOTTOM_RIGHT= NAUI_RESIZE_BOTTOM | NAUI_RESIZE_RIGHT,
};

typedef struct
{
    char *key;
    Naui_PanelType value;
}
Naui_PanelTypeMapEntry;

typedef uint8_t Naui_PanelEvent;
enum
{
    NAUI_PANEL_EVENT_UNDOCK,
    NAUI_PANEL_EVENT_DETACH
};

typedef struct
{
    Naui_PanelNode *node;
    Naui_PanelEvent event;
}
Naui_PanelEventEntry;

typedef struct
{
    Naui_Map(Naui_PanelTypeMapEntry)  panel_type_map;
    Naui_List(Naui_PanelNode*)        root_nodes;
    Naui_List(Naui_PanelEventEntry)   pending_events;

    Naui_PanelNode *dragged_node;
    Naui_PanelNode *resizing_node;

    Naui_PanelNode *splitting_node;
    float split_drag_start_ratio;

    Naui_Vec2 resize_drag_start_mouse;
    Naui_Vec2 resize_drag_start_pos;
    Naui_Vec2 resize_drag_start_size;

    Naui_ResizeHandle resize_handle;
    bool hovering_close;
}
Naui_PanelManager;
static Naui_PanelManager pm = { 0 };

static Naui_PanelNode *naui_alloc_panel_node(void)
{
    // Probably going to replace with a proper simpler allocator yet :3
    return (Naui_PanelNode*)calloc(1, sizeof(Naui_PanelNode));
}

static void naui_free_panel_node(Naui_PanelNode *node)
{
    free(node);
}

static inline bool naui_panel_has_children(Naui_PanelNode *node)
{
    return node->children[0] != NULL;
}

static void naui_render_close_button_icon(Leaf_BoundingBox box, void *user_data)
{
    Naui_Color *color = (Naui_Color*)user_data;
    float padding = box.width * 0.25f;
    float x0 = box.x + padding;
    float y0 = box.y + padding;
    float x1 = box.x + box.width - padding;
    float y1 = box.y + box.height - padding;
    naui_draw_line((Naui_Vec2){x0, y0}, (Naui_Vec2){x1, y1}, *color, 1.0f);
    naui_draw_line((Naui_Vec2){x1, y0}, (Naui_Vec2){x0, y1}, *color, 1.0f);
}

static inline void naui_render_panel_close_button(Naui_PanelNode *node, Leaf_Color color, Leaf_Color bg_color)
{
    Leaf_ID id = leaf_id_indexed(NAUI_PANEL_CLOSE_ID, (Naui_PanelID)node);
    bool hovered = leaf_hovered(id);
    if (hovered) pm.hovering_close = true;

    leaf({
        .id = id,
        .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_GROW},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = hovered ? bg_color : leaf_rgba(0, 0, 0, 0),
        .rounding = {6.0f, LEAF_CORNER_ALL},
        .aspect_ratio = 1.0f,
        .custom_draw = naui_render_close_button_icon,
        .custom_draw_user_data = &color
    });
}

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_title_bg_color");
    Leaf_Color text_color = naui_theme_leaf_color("naui_panel_title_text_color");
    Leaf_Color tab_color = naui_theme_leaf_color("naui_panel_tab_bg_color");
    Naui_Vec2 padding = naui_theme_vec2("naui_panel_title_padding");
    float font_size = naui_theme_float("naui_font_size");

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .color = bg_color
    })
    {
        if (node->parent)
        {
            Leaf_ID tab_id = leaf_id_indexed(NAUI_PANEL_TAB_ID, (Naui_PanelID)node);
            bool tab_hovered = leaf_hovered(tab_id) && !naui_mouse_down(NAUI_MOUSE_LEFT);

            leaf({
                .id = tab_id,
                .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIXED(font_size)},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .padding = LEAF_PADDING_AXES(padding.x, padding.y),
                .color = tab_color
            })
            {
                leaf({
                    .direction = LEAF_LAYOUT_HORIZONAL,
                    .size = {LEAF_SIZE_FIT, LEAF_SIZE_FULL},
                    .padding = { 0.0f, font_size - padding.x * 0.5f, 0.0f, 0.0f}
                })
                {
                    leaf_text(node->title, {
                        .font_size = font_size,
                        .color = text_color,
                        .alignment = LEAF_TEXT_ALIGN_LEFT
                    });
                }

                if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE) && tab_hovered)
                    leaf({
                        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                        .direction = LEAF_LAYOUT_HORIZONAL,
                        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
                        .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
                        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
                    }) naui_render_panel_close_button(node, text_color, bg_color);
            }
        }
        else
        {
            leaf({
                .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(font_size)},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .padding = LEAF_PADDING_AXES(padding.x, padding.y),
                .color = bg_color
            })
            {
                leaf_text(node->title, {
                    .font_size = font_size,
                    .color = text_color,
                    .alignment = LEAF_TEXT_ALIGN_CENTER
                });
            }
            if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE))
            leaf({
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .size = { LEAF_SIZE_FULL, LEAF_SIZE_FIXED(font_size) },
                .padding = LEAF_PADDING_AXES(padding.x, padding.y),
                .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
            }) naui_render_panel_close_button(node, text_color, tab_color);
        }
    }
}

static void naui_render_docking_guide(Naui_PanelNode *node)
{
    if (node->flags & NAUI_PANEL_FLAG_NO_DOCKING ||
        !pm.dragged_node ||
        pm.dragged_node == node ||
        pm.dragged_node->flags & NAUI_PANEL_FLAG_NO_DOCKING ||
        node->root == pm.dragged_node->root
    ) return;

    Naui_PanelID id = (Naui_PanelID)node;
    Leaf_ID guide_left_id   = leaf_id_indexed("__naui_dock_guide_left",   id);
    Leaf_ID guide_right_id  = leaf_id_indexed("__naui_dock_guide_right",  id);
    Leaf_ID guide_center_id = leaf_id_indexed("__naui_dock_guide_center", id);
    Leaf_ID guide_top_id    = leaf_id_indexed("__naui_dock_guide_top",    id);
    Leaf_ID guide_bottom_id = leaf_id_indexed("__naui_dock_guide_bottom", id);

    bool guide_left_hovered   = leaf_hovered(guide_left_id);
    bool guide_right_hovered  = leaf_hovered(guide_right_id);
    bool guide_center_hovered = leaf_hovered(guide_center_id);
    bool guide_top_hovered    = leaf_hovered(guide_top_id);
    bool guide_bottom_hovered = leaf_hovered(guide_bottom_id);

    Leaf_Color bg_color      = leaf_rgba(100, 100, 255, 100);
    Leaf_Color hovered_color = leaf_rgba(100, 100, 255, 200);

    leaf({
        .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    leaf({
        .size         = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT_MIN_MAX(0.6f, 32.0f, 400.0f)},
        .aspect_ratio = 1.0f
    })
    {
        Leaf_Rounding rounding = {6.0f, LEAF_CORNER_ALL};
        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap       = 6.0f
        })
        {
            leaf({ .id = guide_top_id,    .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_top_hovered    ? hovered_color : bg_color, .rounding = rounding });
            leaf({ .id = guide_center_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_center_hovered ? hovered_color : bg_color, .rounding = rounding });
            leaf({ .id = guide_bottom_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_bottom_hovered ? hovered_color : bg_color, .rounding = rounding });
        }
        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .direction       = LEAF_LAYOUT_HORIZONAL,
            .size            = {LEAF_SIZE_PERCENT(1.0f), LEAF_SIZE_PERCENT(1.0f)},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap       = 6.0f
        })
        {
            leaf({ .id = guide_left_id,  .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_left_hovered  ? hovered_color : bg_color, .rounding = rounding });
            leaf({ .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)} });
            leaf({ .id = guide_right_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_right_hovered ? hovered_color : bg_color, .rounding = rounding });
        }
    }
}

static void naui_update_docking_guide(Naui_PanelNode *node)
{
    if (!pm.dragged_node || !naui_mouse_released(NAUI_MOUSE_LEFT))
        return;

    Naui_PanelID id = (Naui_PanelID)node;

    Leaf_ID guide_left_id   = leaf_id_indexed("__naui_dock_guide_left",   id);
    Leaf_ID guide_right_id  = leaf_id_indexed("__naui_dock_guide_right",  id);
    Leaf_ID guide_center_id = leaf_id_indexed("__naui_dock_guide_center", id);
    Leaf_ID guide_top_id    = leaf_id_indexed("__naui_dock_guide_top",    id);
    Leaf_ID guide_bottom_id = leaf_id_indexed("__naui_dock_guide_bottom", id);

    bool guide_left_hovered   = leaf_hovered(guide_left_id);
    bool guide_right_hovered  = leaf_hovered(guide_right_id);
    bool guide_center_hovered = leaf_hovered(guide_center_id);
    bool guide_top_hovered    = leaf_hovered(guide_top_id);
    bool guide_bottom_hovered = leaf_hovered(guide_bottom_id);

    if (guide_left_hovered)
        naui_dock_panel(id, (Naui_PanelID)pm.dragged_node, NAUI_DOCK_DIRECTION_LEFT, 0.5f);
    else if (guide_right_hovered)
        naui_dock_panel(id, (Naui_PanelID)pm.dragged_node, NAUI_DOCK_DIRECTION_RIGHT, 0.5f);
    else if (guide_top_hovered)
        naui_dock_panel(id, (Naui_PanelID)pm.dragged_node, NAUI_DOCK_DIRECTION_TOP, 0.5f);
    else if (guide_bottom_hovered)
        naui_dock_panel(id, (Naui_PanelID)pm.dragged_node, NAUI_DOCK_DIRECTION_BOTTOM, 0.5f);
}

static inline void naui_render_panel_body(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_bg_color");
    Naui_Vec2 padding = naui_theme_vec2("naui_panel_padding");

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_BODY_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .color = bg_color,
        .clip_children = true
    })
    {
        if (node->type.on_render)
            node->type.on_render((Naui_PanelID)node, node->user_data);
        naui_render_docking_guide(node);
    }
}

static inline void naui_render_next_panel_child(Naui_PanelNode *node)
{
    if (!naui_panel_has_children(node))
    {
        naui_render_panel_titlebar(node);
        naui_render_panel_body(node);
        return;
    }

    Leaf_Color border_color = naui_theme_leaf_color("naui_panel_border_color");
    float border_width = naui_theme_float("naui_panel_border_width");

    leaf({
        .direction = (Leaf_LayoutDirection)node->split_axis,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        leaf({
            .size = node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_PERCENT(node->split_ratio)} :
            (Leaf_Size){LEAF_SIZE_PERCENT(node->split_ratio), LEAF_SIZE_FULL}
        })
        naui_render_next_panel_child(node->children[0]);

        leaf({
            .size = node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_GROW} :
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .border = {border_width, border_color}
        })
        naui_render_next_panel_child(node->children[1]);
    }
}

static void naui_render_panel(Naui_PanelNode *node)
{
    Leaf_Color border_color = naui_theme_leaf_color("naui_panel_border_color");
    float border_width = naui_theme_float("naui_panel_border_width");

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
        .floating_offset = {node->position.x, node->position.y},
        .border = {border_width, border_color},
        .clip_children = true
    })
    {
        naui_render_next_panel_child(node);
    }
}

static void naui_panel_bring_to_front(Naui_PanelNode *node)
{
    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        if (pm.root_nodes[i] == node)
        {
            naui_list_remove(pm.root_nodes, i);
            naui_list_push(pm.root_nodes, node);
            break;
        }
    }
}

static void naui_update_panel_movement(Naui_PanelNode *node)
{
    if (pm.resizing_node || pm.splitting_node)
        return;

    static Naui_Vec2 first_position;

    Leaf_ID titlebar_id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node);
    Leaf_ID body_id     = leaf_id_indexed(NAUI_PANEL_BODY_ID,     (Naui_PanelID)node);

    bool hovered = leaf_hovered(titlebar_id) || leaf_hovered(body_id);

    Naui_PanelNode *root = node->root;

    if (hovered && !pm.hovering_close && !pm.dragged_node && naui_mouse_clicked(NAUI_MOUSE_LEFT))
    {
        first_position.x = naui_mouse_x() - root->position.x;
        first_position.y = naui_mouse_y() - root->position.y;
        pm.dragged_node  = root;
        naui_panel_bring_to_front(root);
    }

    if (pm.dragged_node != root)
        return;

    if (node->parent && !(node->flags & NAUI_PANEL_FLAG_NO_DOCKING) && !(node->flags & NAUI_PANEL_FLAG_NO_MOVE) &&
        naui_mouse_clicked(NAUI_MOUSE_LEFT) && leaf_hovered(leaf_id_indexed(NAUI_PANEL_TAB_ID, (Naui_PanelID)node)))
    {
        Leaf_BoundingBox body_box     = leaf_get_bounding_box(body_id);
        Leaf_BoundingBox titlebar_box = leaf_get_bounding_box(titlebar_id);

        naui_undock_panel((Naui_PanelID)node);
        node->size = (Naui_Vec2){ body_box.width, body_box.height + titlebar_box.height };
        pm.dragged_node = node;
        first_position.x = naui_mouse_x() - titlebar_box.x;
        first_position.y = naui_mouse_y() - titlebar_box.y;
        naui_panel_bring_to_front(node);
        root = node;
    }

    if (root->flags & NAUI_PANEL_FLAG_NO_MOVE)
        return;

    root->position.x = naui_mouse_x() - first_position.x;
    root->position.y = naui_mouse_y() - first_position.y;
}

static void naui_update_panel_close(Naui_PanelNode *node)
{
    if (naui_list_len(pm.pending_events) != 0)
        return;

    Leaf_ID close_id = leaf_id_indexed(NAUI_PANEL_CLOSE_ID, (Naui_PanelID)node);

    if (!leaf_hovered(close_id) || !naui_mouse_clicked(NAUI_MOUSE_LEFT))
        return;

    if (node->parent)
    {
        naui_list_push(pm.pending_events, ((Naui_PanelEventEntry){ node, NAUI_PANEL_EVENT_UNDOCK }));
        naui_list_push(pm.pending_events, ((Naui_PanelEventEntry){ node, NAUI_PANEL_EVENT_DETACH }));
    }
    else
    {
        naui_list_push(pm.pending_events, ((Naui_PanelEventEntry){ node, NAUI_PANEL_EVENT_DETACH }));
    }
}

static void naui_update_panel_split(Naui_PanelNode *node, Naui_Vec2 rect_pos, Naui_Vec2 rect_size)
{
    if (!naui_panel_has_children(node))
        return;

    if (pm.dragged_node || pm.resizing_node)
        return;

    float half = NAUI_RESIZE_BORDER_WIDTH * 0.5f;
    float mx   = naui_mouse_x();
    float my   = naui_mouse_y();

    bool vertical = (node->split_axis == NAUI_SPLIT_AXIS_VERTICAL);

    float sep = vertical
        ? rect_pos.y + rect_size.y * node->split_ratio
        : rect_pos.x + rect_size.x * node->split_ratio;

    bool on_sep;
    if (vertical)
        on_sep = my >= sep - half && my <= sep + half
              && mx >= rect_pos.x  && mx <= rect_pos.x + rect_size.x;
    else
        on_sep = mx >= sep - half && mx <= sep + half
              && my >= rect_pos.y  && my <= rect_pos.y + rect_size.y;

    if (!pm.splitting_node)
    {
        if (on_sep)
        {
            naui_set_cursor(vertical ? NAUI_CURSOR_RESIZE_NS : NAUI_CURSOR_RESIZE_EW);
            if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
            {
                pm.splitting_node         = node;
                pm.split_drag_start_ratio = node->split_ratio;
            }
        }
        return;
    }

    if (pm.splitting_node != node)
        return;

    naui_set_cursor(vertical ? NAUI_CURSOR_RESIZE_NS : NAUI_CURSOR_RESIZE_EW);

    float span = vertical ? rect_size.y : rect_size.x;
    float mouse_pos = vertical ? my : mx;
    float origin = vertical ? rect_pos.y : rect_pos.x;

    float min_ratio = NAUI_MIN_PANEL_SIZE / span;
    float max_ratio = 1.0f - min_ratio;

    float new_ratio = (mouse_pos - origin) / span;
    node->split_ratio = fmaxf(min_ratio, fminf(max_ratio, new_ratio));
}

static void naui_set_cursor_for_handle(Naui_ResizeHandle handle)
{
    switch (handle)
    {
        case NAUI_RESIZE_LEFT:
        case NAUI_RESIZE_RIGHT: naui_set_cursor(NAUI_CURSOR_RESIZE_EW); break;
        case NAUI_RESIZE_TOP:
        case NAUI_RESIZE_BOTTOM: naui_set_cursor(NAUI_CURSOR_RESIZE_NS); break;
        case NAUI_RESIZE_TOP_LEFT:
        case NAUI_RESIZE_BOTTOM_RIGHT: naui_set_cursor(NAUI_CURSOR_RESIZE_NWSE); break;
        case NAUI_RESIZE_TOP_RIGHT:
        case NAUI_RESIZE_BOTTOM_LEFT: naui_set_cursor(NAUI_CURSOR_RESIZE_NESW); break;
        default: break;
    }
}

static Naui_ResizeHandle naui_get_resize_handle(Naui_PanelNode *root)
{
    float mx = naui_mouse_x();
    float my = naui_mouse_y();
    float x  = root->position.x;
    float y  = root->position.y;
    float w  = root->size.x;
    float h  = root->size.y;
    float hb = NAUI_RESIZE_BORDER_WIDTH * 0.5f;

    bool on_left   = mx >= x - hb       && mx <= x + hb;
    bool on_right  = mx >= x + w - hb   && mx <= x + w + hb;
    bool on_top    = my >= y - hb       && my <= y + hb;
    bool on_bottom = my >= y + h - hb   && my <= y + h + hb;

    bool in_x = mx >= x - hb && mx <= x + w + hb;
    bool in_y = my >= y - hb && my <= y + h + hb;

    if (!in_x || !in_y) return NAUI_RESIZE_NONE;

    Naui_ResizeHandle handle = NAUI_RESIZE_NONE;
    if (on_left)   handle |= NAUI_RESIZE_LEFT;
    if (on_right)  handle |= NAUI_RESIZE_RIGHT;
    if (on_top)    handle |= NAUI_RESIZE_TOP;
    if (on_bottom) handle |= NAUI_RESIZE_BOTTOM;
    return handle;
}

static void naui_update_panel_resize(Naui_PanelNode *node)
{
    if (node->flags & NAUI_PANEL_FLAG_NO_RESIZE)
        return;

    Naui_PanelNode *root = node->root;

    if (!pm.resizing_node && !pm.dragged_node)
    {
        Naui_ResizeHandle handle = naui_get_resize_handle(root);
        if (handle != NAUI_RESIZE_NONE)
        {
            naui_set_cursor_for_handle(handle);
            if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
            {
                pm.resizing_node           = root;
                pm.resize_handle           = handle;
                pm.resize_drag_start_mouse = (Naui_Vec2){ naui_mouse_x(), naui_mouse_y() };
                pm.resize_drag_start_pos   = root->position;
                pm.resize_drag_start_size  = root->size;
                naui_panel_bring_to_front(root);
            }
        }
    }

    if (pm.resizing_node != root)
        return;

    naui_set_cursor_for_handle(pm.resize_handle);

    float dx = naui_mouse_x() - pm.resize_drag_start_mouse.x;
    float dy = naui_mouse_y() - pm.resize_drag_start_mouse.y;

    Naui_Vec2 new_pos  = pm.resize_drag_start_pos;
    Naui_Vec2 new_size = pm.resize_drag_start_size;

    if (pm.resize_handle & NAUI_RESIZE_LEFT)
    {
        float clamped_dx = fminf(dx, new_size.x - NAUI_MIN_PANEL_SIZE);
        new_pos.x  += clamped_dx;
        new_size.x -= clamped_dx;
    }
    if (pm.resize_handle & NAUI_RESIZE_RIGHT)
    {
        new_size.x = fmaxf(new_size.x + dx, NAUI_MIN_PANEL_SIZE);
    }
    if (pm.resize_handle & NAUI_RESIZE_TOP)
    {
        float clamped_dy = fminf(dy, new_size.y - NAUI_MIN_PANEL_SIZE);
        new_pos.y  += clamped_dy;
        new_size.y -= clamped_dy;
    }
    if (pm.resize_handle & NAUI_RESIZE_BOTTOM)
    {
        new_size.y = fmaxf(new_size.y + dy, NAUI_MIN_PANEL_SIZE);
    }

    root->position = new_pos;
    root->size = new_size;
}

static void naui_update_panel(Naui_PanelNode *node, Naui_Vec2 rect_pos, Naui_Vec2 rect_size)
{
    if (!naui_panel_has_children(node))
    {
        naui_update_panel_close(node);
        naui_update_panel_resize(node);
        naui_update_panel_movement(node);
        naui_update_docking_guide(node);
        if (node->type.on_update)
            node->type.on_update((Naui_PanelID)node, node->user_data);
        return;
    }

    naui_update_panel_split(node, rect_pos, rect_size);

    bool vertical = (node->split_axis == NAUI_SPLIT_AXIS_VERTICAL);

    Naui_Vec2 c0_pos  = rect_pos;
    Naui_Vec2 c0_size = rect_size;
    Naui_Vec2 c1_pos  = rect_pos;
    Naui_Vec2 c1_size = rect_size;

    if (vertical)
    {
        c0_size.y = rect_size.y * node->split_ratio;
        c1_pos.y  = rect_pos.y  + c0_size.y;
        c1_size.y = rect_size.y - c0_size.y;
    }
    else
    {
        c0_size.x = rect_size.x * node->split_ratio;
        c1_pos.x  = rect_pos.x  + c0_size.x;
        c1_size.x = rect_size.x - c0_size.x;
    }

    naui_update_panel(node->children[0], c0_pos, c0_size);
    naui_update_panel(node->children[1], c1_pos, c1_size);
}

static void naui_undock_panel_immediate(Naui_PanelNode *node)
{
    Naui_PanelNode *parent = node->parent;

    if (!parent)
        return;

    int sibling_index = (parent->children[1] == node) ? 0 : 1;
    Naui_PanelNode *sibling = parent->children[sibling_index];
    Naui_PanelNode *sibling_c0 = sibling->children[0];
    Naui_PanelNode *sibling_c1 = sibling->children[1];
    Naui_PanelNode *saved_parent = parent->parent;
    Naui_PanelNode *saved_root = parent->root;

    sibling->position = saved_root->position;
    sibling->size = saved_root->size;

    *parent = *sibling;
    parent->parent = saved_parent;
    parent->root = saved_root;
    parent->children[0] = sibling_c0;
    parent->children[1] = sibling_c1;
    if (sibling_c0) sibling_c0->parent = parent;
    if (sibling_c1) sibling_c1->parent = parent;

    naui_free_panel_node(sibling);

    node->parent = NULL;
    node->root = node;

    naui_list_push(pm.root_nodes, node);
}

static void naui_detach_panel_immediate(Naui_PanelNode *node)
{
    if (node->type.on_detach)
        node->type.on_detach((Naui_PanelID)node, node->user_data);

    if (node->user_data)
        free(node->user_data);

    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        if (pm.root_nodes[i] == node)
        {
            naui_list_remove(pm.root_nodes, i);
            break;
        }
    }
    naui_free_panel_node(node);
}

static inline void naui_process_pending_events(void)
{
    size_t len = naui_list_len(pm.pending_events);
    if (len == 0)
        return;
    for (size_t i = 0; i < len; i++)
    {
        Naui_PanelEventEntry e = pm.pending_events[i];
        switch (e.event)
        {
            case NAUI_PANEL_EVENT_UNDOCK: naui_undock_panel_immediate(e.node); break;
            case NAUI_PANEL_EVENT_DETACH: naui_detach_panel_immediate(e.node); break;
        }
    }
    naui_list_clear(pm.pending_events);
}

void naui_panel_manager_frame(void)
{
    for (size_t i = naui_list_len(pm.root_nodes); i-- > 0;)
    {
        Naui_PanelNode *root = pm.root_nodes[i];
        naui_update_panel(root, root->position, root->size);
    }

    pm.hovering_close = false;
    if (!naui_mouse_down(NAUI_MOUSE_LEFT))
    {
        pm.dragged_node = NULL;
        pm.resizing_node = NULL;
        pm.splitting_node = NULL;
    }

    naui_process_pending_events();

    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
        naui_render_panel(pm.root_nodes[i]);
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    ptrdiff_t type_index = naui_strmap_get_index(pm.panel_type_map, type_name);
    if (type_index < 0)
    {
        fprintf(stderr, "Panel of type `%s` not found!", type_name);
        return NULL;
    }

    Naui_PanelNode *node = naui_alloc_panel_node();
    node->type = pm.panel_type_map[type_index].value;
    node->size = (Naui_Vec2) { 1280, 720 };
    node->root = node;

    if (node->type.user_data_size)
        node->user_data = malloc(node->type.user_data_size);

    naui_list_push(pm.root_nodes, node);
    if (node->type.on_attach)
        node->type.on_attach((Naui_PanelID)node, node->user_data);
    return (Naui_PanelID)node;
}

void naui_detach_panel(Naui_PanelID id)
{
    Naui_PanelNode *node = (Naui_PanelNode*)id;

    if (node->type.on_detach)
        node->type.on_detach((Naui_PanelID)node, node->user_data);

    if (node->user_data)
        free(node->user_data);

    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        if (pm.root_nodes[i] == id)
        {
            naui_list_remove(pm.root_nodes, i);
            break;
        }
    }
    naui_free_panel_node(node);
}

void naui_register_panel_type(const char *name, Naui_PanelType type_events)
{
    naui_strmap_put(pm.panel_type_map, name, type_events);
}

static void naui_set_subtree_root(Naui_PanelNode *node, Naui_PanelNode *root)
{
    node->root = root;
    if (!naui_panel_has_children(node)) return;
    naui_set_subtree_root(node->children[0], root);
    naui_set_subtree_root(node->children[1], root);
}

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{
    Naui_PanelNode *target = (Naui_PanelNode *)target_id;
    Naui_PanelNode *guest  = (Naui_PanelNode *)guest_id;

    Naui_PanelNode *content_clone = naui_alloc_panel_node();
    *content_clone = *target;
    content_clone->parent = target;
    content_clone->children[0] = NULL;
    content_clone->children[1] = NULL;

    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        if (pm.root_nodes[i] == guest)
        {
            naui_list_remove(pm.root_nodes, i);
            break;
        }
    }

    Naui_PanelNode *child0, *child1;
    switch (direction)
    {
        case NAUI_DOCK_DIRECTION_LEFT:
            child0 = guest; child1 = content_clone;
            split_ratio = 1.0f - split_ratio;
            target->split_axis = NAUI_SPLIT_AXIS_HORIZONTAL;
            break;
        case NAUI_DOCK_DIRECTION_RIGHT:
            child0 = content_clone; child1 = guest;
            target->split_axis = NAUI_SPLIT_AXIS_HORIZONTAL;
            break;
        case NAUI_DOCK_DIRECTION_TOP:
            child0 = guest; child1 = content_clone;
            split_ratio = 1.0f - split_ratio;
            target->split_axis = NAUI_SPLIT_AXIS_VERTICAL;
            break;
        case NAUI_DOCK_DIRECTION_BOTTOM:
            child0 = content_clone; child1 = guest;
            target->split_axis = NAUI_SPLIT_AXIS_VERTICAL;
            break;
        default:
            naui_free_panel_node(content_clone);
            naui_list_push(pm.root_nodes, guest);
            return target_id;
    }

    target->children[0] = child0;
    target->children[1] = child1;
    target->split_ratio  = split_ratio;
    target->type = (Naui_PanelType){ 0 };

    child0->parent = target;
    child1->parent = target;

    naui_set_subtree_root(child0, target->root);
    naui_set_subtree_root(child1, target->root);

    return (Naui_PanelID)child1;
}

void naui_undock_panel(Naui_PanelID id)
{
    naui_list_push(pm.pending_events, ((Naui_PanelEventEntry){ (Naui_PanelNode *)id, NAUI_PANEL_EVENT_UNDOCK }));
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    strncpy(((Naui_PanelNode*)panel_id)->title, title, sizeof(((Naui_PanelNode*)0)->title));
}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->size = size;
}

void naui_panel_enable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags |= flags;
}

void naui_panel_disable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags &= ~flags;
}