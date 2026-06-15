#include "panel.h"
#include "theme.h"
#include "input.h"
#include "math.h"

#include "utils/map.h"
#include "utils/list.h"

#include <leaf/leaf.h>
#include <stdio.h>

#define NAUI_PANEL_DEFAULT_WIDTH 1280
#define NAUI_PANEL_DEFAULT_HEIGHT 720

#define NAUI_PANEL_RESIZE_BORDER 6.0f
#define NAUI_PANEL_MIN_SIZE 100.0f

#define NAUI_ROOT_PANEL_ID "__naui_root_panel"
#define NAUI_CHILD_PANEL_ID "__naui_child_panel"
#define NAUI_PANEL_TAB_ID "__naui_panel_tab"

#define NAUI_DOCK_GUIDE_LEFT_ID "__naui_dock_guide_left"
#define NAUI_DOCK_GUIDE_RIGHT_ID "__naui_dock_guide_right"
#define NAUI_DOCK_GUIDE_TOP_ID "__naui_dock_guide_top"
#define NAUI_DOCK_GUIDE_BOTTOM_ID "__naui_dock_guide_bottom"
#define NAUI_DOCK_GUIDE_CENTER_ID "__naui_dock_guide_center"
#define NAUI_DOCK_GUIDE_VIEWPORT_ID "__naui_dock_guide_viewport"

typedef uint8_t Naui_SplitAxis;
enum
{
    NAUI_SPLIT_AXIS_VERTICAL = LEAF_DIRECTION_VERTICAL,
    NAUI_SPLIT_AXIS_HORIZONTAL = LEAF_DIRECTION_HORIZONAL
};

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    const char     *title;
    Naui_PanelType  type;
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    Naui_PanelNode *root;
    void           *user_data;
    Naui_Vec2       position, size;
    Naui_PanelFlags flags;
    uint32_t        root_index;
    float           split_ratio;
    Naui_SplitAxis  split_axis;
};

typedef struct { char *key; Naui_PanelType value; } Naui_PanelTypeMapEntry;

typedef uint8_t Naui_PanelEventType;
enum
{
    NAUI_PANEL_EVENT_UNDOCK
};

typedef struct
{
    Naui_PanelNode *node;
    Naui_PanelEventType type;
}
Naui_PanelEvent;

typedef struct
{
    Naui_Map(Naui_PanelTypeMapEntry) panel_type_map;
    Naui_List(Naui_PanelNode*) root_nodes;
    Naui_List(Naui_PanelEvent) event_queue;

    Naui_PanelNode *main_viewport;

    Naui_PanelNode *dragging_node;
    Naui_PanelNode *resizing_node;
}
Naui_PanelManager;
static Naui_PanelManager pm = { 0 };

static Naui_PanelNode *naui_alloc_panel_node(void) { return (Naui_PanelNode*)calloc(1, sizeof(Naui_PanelNode)); }
static void naui_free_panel_node(Naui_PanelNode *node) { free(node); }

void naui_register_panel_type(const char *name, Naui_PanelType type)
{
    naui_strmap_put(pm.panel_type_map, name, type);
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    ptrdiff_t type_index = naui_strmap_get_index(pm.panel_type_map, type_name);
    if (type_index < 0)
    {
        fprintf(stderr, "[Naui]: Panel of type `%s` not found!", type_name);
        return NULL;
    }

    Naui_PanelNode *node = naui_alloc_panel_node();
    node->type = pm.panel_type_map[type_index].value;
    node->size = (Naui_Vec2) { NAUI_PANEL_DEFAULT_WIDTH, NAUI_PANEL_DEFAULT_HEIGHT };
    node->root = node;
    node->root_index = naui_list_len(pm.root_nodes);

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

    naui_list_remove(pm.root_nodes, node->root_index);
    naui_free_panel_node(node);
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    ((Naui_PanelNode*)panel_id)->title = title;
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

void naui_set_main_viewport(Naui_PanelID id)
{
    pm.main_viewport = (Naui_PanelNode*)id;
    naui_list_remove(pm.root_nodes, pm.main_viewport->root_index);
}

Naui_PanelID naui_get_main_viewport(void)
{
    return (Naui_PanelID)pm.main_viewport;
}

static void naui_panel_bring_to_front(Naui_PanelNode *node)
{
    Naui_PanelNode *root = node->root;

    if (root == pm.main_viewport)
        return;

    uint32_t last = naui_list_len(pm.root_nodes) - 1;
    if (root->root_index == last)
        return;

    for (uint32_t i = root->root_index; i < last; i++)
    {
        pm.root_nodes[i] = pm.root_nodes[i + 1];
        pm.root_nodes[i]->root_index = i;
    }

    pm.root_nodes[last] = root;
    root->root_index = last;
}

static void naui_set_root_recursive(Naui_PanelNode *node, Naui_PanelNode *new_root)
{
    if (!node)
        return;
    node->root = new_root;
    naui_set_root_recursive(node->children[0], new_root);
    naui_set_root_recursive(node->children[1], new_root);
}

void naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{
    Naui_PanelNode *target = (Naui_PanelNode*)target_id;
    Naui_PanelNode *guest  = (Naui_PanelNode*)guest_id;

    if (direction == NAUI_DOCK_DIRECTION_CENTER)
    {
        return;
    }

    bool target_is_viewport = (target == pm.main_viewport);

    naui_list_remove(pm.root_nodes, guest->root_index);
    if (!target_is_viewport && !target->parent && guest->root_index < target->root_index)
        target->root_index--;

    Naui_PanelNode *dock_node = naui_alloc_panel_node();

    if (target_is_viewport)
    {
        pm.main_viewport = dock_node;
        dock_node->root = dock_node;
    }
    else if (target->parent)
    {
        dock_node->parent = target->parent;
        target->parent->children[target->parent->children[0] == target ? 0 : 1] = dock_node;
        dock_node->root = target->root;
    }
    else
    {
        dock_node->root_index = target->root_index;
        pm.root_nodes[target->root_index] = dock_node;
        dock_node->root = dock_node;
    }

    target->parent = dock_node;

    if (direction == NAUI_DOCK_DIRECTION_RIGHT || direction == NAUI_DOCK_DIRECTION_BOTTOM)
    {
        dock_node->children[0] = target;
        dock_node->children[1] = guest;
    }
    else
    {
        dock_node->children[0] = guest;
        dock_node->children[1] = target;
    }

    dock_node->split_ratio = split_ratio;
    dock_node->split_axis =
        (direction == NAUI_DOCK_DIRECTION_LEFT || direction == NAUI_DOCK_DIRECTION_RIGHT) ?
        NAUI_SPLIT_AXIS_HORIZONTAL : NAUI_SPLIT_AXIS_VERTICAL;

    naui_set_root_recursive(target, dock_node->root);
    naui_set_root_recursive(guest, dock_node->root);

    guest->parent = dock_node;

    dock_node->size = target->size;
    dock_node->position = target->position;
}

static void naui_undock_panel_immediate(Naui_PanelID id)
{
    Naui_PanelNode *node = (Naui_PanelNode*)id;

    if (node == pm.main_viewport)
    {
        pm.main_viewport = NULL;

        node->parent = NULL;
        node->root = node;
        node->root_index = naui_list_len(pm.root_nodes);
        naui_list_push(pm.root_nodes, node);
        return;
    }

    Naui_PanelNode *dock_node = node->parent;

    if (!dock_node)
        return;

    Naui_PanelNode *sibling = dock_node->children[dock_node->children[0] == node ? 1 : 0];
    sibling->position = node->root->position;
    sibling->size = node->root->size;

    if (dock_node->parent)
    {
        int slot = dock_node->parent->children[0] == dock_node ? 0 : 1;
        dock_node->parent->children[slot] = sibling;
        sibling->parent = dock_node->parent;
    }
    else if (dock_node == pm.main_viewport)
    {
        pm.main_viewport = sibling;
        sibling->parent = NULL;
        sibling->root = sibling;
        naui_set_root_recursive(sibling, sibling);
    }
    else
    {
        sibling->root_index = dock_node->root_index;
        pm.root_nodes[dock_node->root_index] = sibling;
        sibling->parent = NULL;
        naui_set_root_recursive(sibling, sibling);
    }

    naui_free_panel_node(dock_node);

    node->parent = NULL;
    node->root = node;
    node->root_index = naui_list_len(pm.root_nodes);
    naui_list_push(pm.root_nodes, node);
}

void naui_undock_panel(Naui_PanelID id)
{
    naui_list_push(pm.event_queue, ((Naui_PanelEvent){ (Naui_PanelNode*)id, NAUI_PANEL_EVENT_UNDOCK }));
}

static bool naui_point_occluded_by_panel_list(void)
{
    float mx = naui_mouse_x();
    float my = naui_mouse_y();

    for (uint32_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *other = pm.root_nodes[i];
        if (other == pm.dragging_node || other == pm.dragging_node->root)
            continue;

        Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)other));
        if (mx >= b.x && mx <= b.x + b.width && my >= b.y && my <= b.y + b.height)
            return true;
    }

    return false;
}

static bool naui_point_occluded_by_higher_panel(Naui_PanelNode *node)
{
    Naui_PanelNode *root = node->root;
    bool in_viewport = (root == pm.main_viewport);

    if (in_viewport)
        return naui_point_occluded_by_panel_list();

    float mx = naui_mouse_x();
    float my = naui_mouse_y();

    for (uint32_t i = root->root_index + 1; i < naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *other = pm.root_nodes[i];
        if (other == pm.dragging_node || other == pm.dragging_node->root)
            continue;

        Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)other));
        if (mx >= b.x && mx <= b.x + b.width && my >= b.y && my <= b.y + b.height)
            return true;
    }

    return false;
}

static inline bool naui_can_show_dock_guides(Naui_PanelNode *node)
{
    return pm.dragging_node && !pm.dragging_node->children[0] && pm.dragging_node != node && node->root != pm.dragging_node->root;
}

static inline void naui_render_dock_guide_slot(const char *label, Naui_PanelNode *node, bool horizontal, bool occluded)
{
    Leaf_ID id = occluded ? (Leaf_ID){0}: leaf_id_indexed(label, (Naui_PanelID)node);
    bool hovered = occluded ? 0 : leaf_hovered(id);
    leaf({
        .id = id,
        .size = horizontal ?
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_DERIVED} :
            (Leaf_Size){LEAF_SIZE_DERIVED, LEAF_SIZE_GROW},
        .color = hovered ?
            naui_theme_leaf_color(NAUI_DOCK_GUIDE_COLOR) :
            naui_theme_leaf_color(NAUI_DOCK_GUIDE_HOVERED_COLOR),
        .rounding = {
            .value = 8.0f,
            .corners = LEAF_CORNER_ALL
        },
        .aspect_ratio = 1.0f
    });
}

static void naui_render_dock_guides(Naui_PanelNode *node, bool occluded)
{
    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(0.65f)},
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .aspect_ratio = 1.0f,
        .child_gap = 4.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_TOP_ID, node, false, occluded);
        leaf({.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_GROW}, .aspect_ratio = 1.0f});
        //naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_CENTER_ID, node, false, occluded);
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_BOTTOM_ID, node, false, occluded);
    }

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .direction = LEAF_DIRECTION_HORIZONAL,
        .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(0.65f)},
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .aspect_ratio = 1.0f,
        .child_gap = 4.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_LEFT_ID, node, true, occluded);
        leaf({.size = {LEAF_SIZE_GROW, LEAF_SIZE_DERIVED}, .aspect_ratio = 1.0f});
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_RIGHT_ID, node, true, occluded);
    }
}

static inline void naui_render_basic_panel_titlebar(Naui_PanelNode *node)
{
    const float font_size = naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG);
    const Leaf_Color text_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG);

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(font_size)},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y)
    })
    {
        leaf_text(node->title, {
            .font_size = font_size,
            .color = text_color,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

static inline void naui_render_docked_panel_tab(Naui_PanelNode *node, bool is_active, Leaf_ID id)
{
    const float font_size = naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG);
    const float rounding = naui_theme_float(NAUI_PANEL_ROUNDING_TAG);
    const Leaf_Color text_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG);

    const Leaf_Color bg_color = is_active
        ? naui_theme_leaf_color(NAUI_PANEL_BODY_BG_COLOR_TAG)
        : naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIXED(font_size)},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .color = bg_color,
        .rounding = { rounding, LEAF_CORNER_TL | LEAF_CORNER_TR }
    })
    {
        leaf_text(node->title, { .font_size = font_size, .color = text_color });
    }
}

static inline void naui_render_docked_panel_titlebar(Naui_PanelNode *node)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONAL,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .child_gap = 2.0f
        })
        {
            naui_render_docked_panel_tab(node, true, leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node));
        }
    }
}

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    const Leaf_Color bg_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .color = bg_color,
        .rounding = {
            naui_theme_float(NAUI_PANEL_ROUNDING_TAG),
            LEAF_CORNER_TL | LEAF_CORNER_TR
        }
    })
    {
        if (node->parent || node->root == pm.main_viewport)
            naui_render_docked_panel_titlebar(node);
        else naui_render_basic_panel_titlebar(node);
    }
}

static inline void naui_render_panel_body(Naui_PanelNode *node)
{
    Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_BODY_PADDING_TAG);
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .color = naui_theme_leaf_color(NAUI_PANEL_BODY_BG_COLOR_TAG),
        .rounding = {
            naui_theme_float(NAUI_PANEL_ROUNDING_TAG),
            LEAF_CORNER_BL | LEAF_CORNER_BR
        },
        .clip_children = true
    })
    {
        if (node->type.on_render)
            node->type.on_render((Naui_PanelID)node, node->user_data);
        
        if (naui_can_show_dock_guides(node))
            naui_render_dock_guides(node, naui_point_occluded_by_higher_panel(node));
    }
}

static void naui_render_next_panel_child(Naui_PanelNode *node)
{
    if (node->children[0])
    {
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
        return;
    }

    leaf({
        .id = leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        if (!(node->flags & NAUI_PANEL_FLAG_NO_TITLE) || node->parent)
            naui_render_panel_titlebar(node);
        naui_render_panel_body(node);
    }
}

static void naui_render_panel(Naui_PanelNode *node)
{
    leaf({
        .id = leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)node),
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
        .floating_offset = {node->position.x, node->position.y},
        .border = {
            naui_theme_float(NAUI_PANEL_BORDER_WIDTH_TAG),
            naui_theme_leaf_color(NAUI_PANEL_BORDER_COLOR_TAG)
        },
        .shadow = {
            .blur_radius = 32.0f,
            .color = naui_theme_leaf_color(NAUI_PANEL_SHADOW_COLOR_TAG)
        },
        .rounding = {
            naui_theme_float(NAUI_PANEL_ROUNDING_TAG),
            LEAF_CORNER_ALL
        },
        .color = naui_theme_leaf_color(NAUI_PANEL_BORDER_COLOR_TAG),
        .clip_children = true
    })
    {
        naui_render_next_panel_child(node);
    }
}

static void naui_render_main_viewport(void)
{
    Naui_PanelNode *node = pm.main_viewport;
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .color = naui_theme_leaf_color(NAUI_VIEWPORT_BG_COLOR_TAG)
    })
    {
        if (node)
            naui_render_next_panel_child(node);
        else
        {
            if (pm.dragging_node && !pm.dragging_node->children[0])
            leaf({
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FIXED(200.0f)},
                .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .aspect_ratio = 1.0f
            })
            {
                naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_VIEWPORT_ID, NULL, false,
                    naui_point_occluded_by_panel_list());
            }
        }
    }
}

static void naui_update_panel_dragging(Naui_PanelNode *node)
{
    static Naui_Vec2 drag_offset;
    if (pm.resizing_node)
        return;

    Naui_PanelNode *root = node->root;

    if (naui_mouse_clicked(NAUI_MOUSE_LEFT) && (!pm.dragging_node || pm.dragging_node->root == root))
    {
        Naui_PanelNode *drag_target = NULL;

        if (leaf_hovered(leaf_id_indexed(NAUI_PANEL_TAB_ID, (Naui_PanelID)node)))
        {
            naui_undock_panel((Naui_PanelID)node);
            Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));
            node->position = (Naui_Vec2){ box.x, box.y };
            node->size = node == pm.main_viewport ?
                (Naui_Vec2){ box.width * 0.5f, box.height * 0.5f } :
                (Naui_Vec2){ box.width, box.height };
            drag_target = node;
        }
        else if (!pm.dragging_node && leaf_hovered(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root)))
            drag_target = root;

        if (drag_target)
        {
            pm.dragging_node = drag_target;
            drag_offset.x = naui_mouse_x() - drag_target->position.x;
            drag_offset.y = naui_mouse_y() - drag_target->position.y;
            naui_panel_bring_to_front(drag_target);
        }
    }

    if (root == pm.main_viewport)
        return;

    if (pm.dragging_node == root)
    {
        root->position.x = naui_mouse_x() - drag_offset.x;
        root->position.y = naui_mouse_y() - drag_offset.y;
    }
}

static inline Naui_Cursor naui_resize_cursor(int8_t ex, int8_t ey)
{
    if (ex && ey) return (ex == ey) ? NAUI_CURSOR_RESIZE_NWSE : NAUI_CURSOR_RESIZE_NESW;
    return ex ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS;
}

static void naui_update_panel_resizing(Naui_PanelNode *node)
{
    static int8_t drag_x, drag_y;
    static Naui_Vec2 drag_mouse, drag_pos, drag_size;

    Naui_PanelNode *root = node->root;
    if (root == pm.main_viewport)
        return;

    if (pm.resizing_node)
    {
        if (pm.resizing_node != root)
            return;

        float dx = naui_mouse_x() - drag_mouse.x;
        float dy = naui_mouse_y() - drag_mouse.y;

        if (drag_x < 0) dx = fminf(dx, drag_size.x - NAUI_PANEL_MIN_SIZE);
        if (drag_y < 0) dy = fminf(dy, drag_size.y - NAUI_PANEL_MIN_SIZE);

        if (drag_x < 0) { root->position.x = drag_pos.x + dx; root->size.x = drag_size.x - dx; }
        else if (drag_x > 0) root->size.x = fmaxf(drag_size.x + dx, NAUI_PANEL_MIN_SIZE);

        if (drag_y < 0) { root->position.y = drag_pos.y + dy; root->size.y = drag_size.y - dy; }
        else if (drag_y > 0) root->size.y = fmaxf(drag_size.y + dy, NAUI_PANEL_MIN_SIZE);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            pm.resizing_node = NULL;

        naui_set_cursor(naui_resize_cursor(drag_x, drag_y));
        return;
    }

    if (pm.dragging_node)
        return;

    Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root));
    float mx = naui_mouse_x(), my = naui_mouse_y();

    int8_t ex = (mx < b.x + NAUI_PANEL_RESIZE_BORDER) ? -1 : (mx > b.x + b.width - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;
    int8_t ey = (my < b.y + NAUI_PANEL_RESIZE_BORDER) ? -1 : (my > b.y + b.height - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;

    bool inside = mx > b.x - NAUI_PANEL_RESIZE_BORDER && mx < b.x + b.width + NAUI_PANEL_RESIZE_BORDER
               && my > b.y - NAUI_PANEL_RESIZE_BORDER && my < b.y + b.height + NAUI_PANEL_RESIZE_BORDER;

    if (!inside || (!ex && !ey))
        return;

    if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
    {
        pm.resizing_node = root;
        drag_x = ex; drag_y = ey;
        drag_mouse = (Naui_Vec2){ mx, my };
        drag_pos = root->position;
        drag_size = root->size;
        naui_panel_bring_to_front(root);
        naui_set_cursor(naui_resize_cursor(ex, ey));
        return;
    }

    naui_set_cursor(naui_resize_cursor(ex, ey));
}

static void naui_update_panel_dock_guides(Naui_PanelNode *node)
{
    if (!naui_mouse_released(NAUI_MOUSE_LEFT))
        return;
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_LEFT_ID, (Naui_PanelID)node)))
        naui_dock_panel(node, pm.dragging_node, NAUI_DOCK_DIRECTION_LEFT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_RIGHT_ID, (Naui_PanelID)node)))
        naui_dock_panel(node, pm.dragging_node, NAUI_DOCK_DIRECTION_RIGHT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_TOP_ID, (Naui_PanelID)node)))
        naui_dock_panel(node, pm.dragging_node, NAUI_DOCK_DIRECTION_TOP, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_BOTTOM_ID, (Naui_PanelID)node)))
        naui_dock_panel(node, pm.dragging_node, NAUI_DOCK_DIRECTION_BOTTOM, 0.5f);
    //else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_CENTER_ID, (Naui_PanelID)node)))
        //naui_dock_panel(node, pm.dragging_node, NAUI_DOCK_DIRECTION_CENTER, 0.0f);
}

static void naui_update_panel(Naui_PanelNode *node)
{
    if (node->children[0])
    {
        naui_update_panel(node->children[0]);
        naui_update_panel(node->children[1]);
        return;
    }

    naui_update_panel_resizing(node);
    naui_update_panel_dragging(node);

    if (naui_can_show_dock_guides(node))
        naui_update_panel_dock_guides(node);

    if (node->type.on_update)
        node->type.on_update((Naui_PanelID)node, node->user_data);
}

static void naui_update_main_viewport(void)
{
    Naui_PanelNode *node = pm.main_viewport;
    if (node)
    {
        naui_update_panel(node);
        return;
    }

    if (!naui_mouse_released(NAUI_MOUSE_LEFT) || !pm.dragging_node)
        return;
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_VIEWPORT_ID, NULL)))
        naui_set_main_viewport((Naui_PanelID)pm.dragging_node);
}

static void naui_process_events(void)
{
    if (naui_list_len(pm.event_queue) == 0)
        return;

    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.event_queue); i++)
    {
        Naui_PanelEvent event = pm.event_queue[i];
        switch (event.type)
        {
        case NAUI_PANEL_EVENT_UNDOCK: naui_undock_panel_immediate(event.node); break;
        }
    }

    naui_list_clear(pm.event_queue);
}

void naui_panel_manager_frame(void)
{
    for (int32_t i = (int32_t)naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_panel(pm.root_nodes[i]);
    naui_update_main_viewport();
    
    if (naui_mouse_released(NAUI_MOUSE_LEFT))
    {
        pm.dragging_node = NULL;
        pm.resizing_node = NULL;
    }

    naui_render_main_viewport();
    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
        naui_render_panel(pm.root_nodes[i]);

    naui_process_events();
}