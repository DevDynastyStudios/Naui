#include "panel.h"
#include "app.h"
#include "theme.h"
#include "input.h"

#include "math/math.h"

#include "utils/map.h"
#include "utils/list.h"

#include <leaf/leaf.h>
#include <stdio.h>

#define NAUI_PANEL_DEFAULT_WIDTH 1280
#define NAUI_PANEL_DEFAULT_HEIGHT 720

#define NAUI_PANEL_RESIZE_BORDER 6.0f

#define NAUI_MAIN_VIEWPORT_ID "__naui_main_viewport"

#define NAUI_ROOT_PANEL_ID "__naui_root_panel"
#define NAUI_CHILD_PANEL_ID "__naui_child_panel"
#define NAUI_PANEL_TAB_ID "__naui_panel_tab"
#define NAUI_PANEL_TITLEBAR_ID "__naui_panel_titlebar"

#define NAUI_CLOSE_BUTTON_ID "__naui_close_button"

#define NAUI_SPLIT_HANDLE_ID "__naui_split_handle"

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
    Naui_List(Naui_PanelNode*) tabs;
    void           *user_data;
    Naui_Vec2       position, size, min_size;
    Naui_PanelFlags flags;
    uint32_t        root_index;
    int32_t         active_tab;
    float           split_ratio;
    Naui_SplitAxis  split_axis;
    bool            occluded;
    bool            close_hovered;
};

typedef struct { char *key; Naui_PanelType value; } Naui_PanelTypeMapEntry;

typedef uint8_t Naui_PanelEventType;
enum
{
    NAUI_PANEL_EVENT_DETACH,
    NAUI_PANEL_EVENT_UNDOCK,
    NAUI_PANEL_EVENT_BRING_TO_FRONT
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
    Naui_PanelNode *split_resizing_node;

    Naui_PanelNode *current_panel;

    Leaf_BoundingBox dock_guide_area;
    bool any_panel_hovered;
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
        return 0;
    }

    Naui_PanelNode *node = naui_alloc_panel_node();
    node->type = pm.panel_type_map[type_index].value;
    node->position = (Naui_Vec2) { 32.0f, 32.0f };
    node->size = (Naui_Vec2) { NAUI_PANEL_DEFAULT_WIDTH, NAUI_PANEL_DEFAULT_HEIGHT };
    node->min_size = (Naui_Vec2) { 100.0f, 100.0f };
    node->root = node;
    node->root_index = (uint32_t)naui_list_len(pm.root_nodes);

    if (node->type.user_data_size)
        node->user_data = malloc(node->type.user_data_size);
    
    naui_list_push(pm.root_nodes, node);

    Naui_PanelNode *prev = pm.current_panel;
    pm.current_panel = node;
    if (node->type.on_attach)
        node->type.on_attach(node->user_data);
    pm.current_panel = prev;

    return (Naui_PanelID)node;
}

void naui_detach_panel(Naui_PanelID id)
{
    naui_list_push(pm.event_queue, ((Naui_PanelEvent){ (Naui_PanelNode*)id, NAUI_PANEL_EVENT_DETACH }));
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    ((Naui_PanelNode*)panel_id)->title = title;
}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->size = size;
}

void naui_panel_set_min_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->min_size = size;
}

void naui_panel_enable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags |= flags;
}

void naui_panel_disable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags &= ~flags;
}

static inline void naui_reset_root_indexes(uint32_t from)
{
    for (uint32_t i = from; i < naui_list_len(pm.root_nodes); i++)
        pm.root_nodes[i]->root_index = i;
}

void naui_set_main_viewport(Naui_PanelID id)
{
    pm.main_viewport = (Naui_PanelNode*)id;
    uint32_t removed_index = pm.main_viewport->root_index;
    naui_list_remove(pm.root_nodes, removed_index);
    naui_reset_root_indexes(removed_index);
}

Naui_PanelID naui_get_main_viewport(void)
{
    return (Naui_PanelID)pm.main_viewport;
}

static void naui_panel_bring_to_front_immediate(Naui_PanelNode *node)
{
    Naui_PanelNode *root = node->root;

    if (root == pm.main_viewport)
        return;

    uint32_t last = (uint32_t)(naui_list_len(pm.root_nodes) - 1);
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

static void naui_panel_bring_to_front(Naui_PanelNode *node)
{
    naui_list_push(pm.event_queue, ((Naui_PanelEvent){ node, NAUI_PANEL_EVENT_BRING_TO_FRONT }));
}

static void naui_set_root_recursive(Naui_PanelNode *node, Naui_PanelNode *new_root)
{
    if (!node)
        return;
    node->root = new_root;
    naui_set_root_recursive(node->children[0], new_root);
    naui_set_root_recursive(node->children[1], new_root);
}

static inline bool naui_can_dock_center(Naui_PanelNode *node)
{
    return !pm.dragging_node->children[0];
}

void naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{
    Naui_PanelNode *target = (Naui_PanelNode*)target_id;
    Naui_PanelNode *guest  = (Naui_PanelNode*)guest_id;

    bool target_is_viewport = (target == pm.main_viewport);

    if (direction == NAUI_DOCK_DIRECTION_CENTER)
    {
        naui_list_remove(pm.root_nodes, guest->root_index);
        if (!target_is_viewport && !target->parent && guest->root_index < target->root_index)
            target->root_index--;

        Naui_List(Naui_PanelNode*) target_tabs;
        if (target->parent && target->parent->tabs)
            target_tabs = target->parent->tabs;
        else if (!target->tabs)
        {
            Naui_PanelNode *target_copy = naui_alloc_panel_node();
            *target_copy = *target;
            target_copy->parent = target;
            naui_list_push(target->tabs, target_copy);
            target_tabs = target->tabs;
        }
        else
            target_tabs = target->tabs;

        if (guest->tabs)
        {
            for (int32_t i = 0; i < naui_list_len(guest->tabs); i++)
            {
                Naui_PanelNode *guest_tab = guest->tabs[i];
                guest_tab->parent = target;
                guest_tab->root = target->root;
                naui_list_push(target_tabs, guest_tab);
            }
            naui_list_free(guest->tabs);
            naui_free_panel_node(guest);
        }
        else
        {
            guest->parent = target;
            guest->root = target->root;
            naui_list_push(target_tabs, guest);
        }

        if (target->parent && target->parent->tabs)
            target->parent->tabs = target_tabs;
        else
            target->tabs = target_tabs;

        return;
    }

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

static void naui_undock_panel_immediate(Naui_PanelNode *node)
{
    if (node == pm.main_viewport)
    {
        pm.main_viewport = NULL;

        node->parent = NULL;
        node->root = node;
        node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
        naui_list_push(pm.root_nodes, node);
        return;
    }

    Naui_PanelNode *dock_node = node->parent;

    if (!dock_node)
        return;

    if (dock_node->tabs)
    {
        Naui_PanelNode *group = dock_node;
        int32_t count = naui_list_len(group->tabs);
        int32_t found = -1;

        for (int32_t i = 0; i < count; i++)
        {
            if (group->tabs[i] == node)
            {
                found = i;
                break;
            }
        }

        if (found < 0)
            return;

        naui_list_remove(group->tabs, found);
        count--;

        if (group->active_tab >= count)
            group->active_tab = count - 1;
        else if (found < group->active_tab)
            group->active_tab--;

        if (count == 1)
        {
            Naui_PanelNode *remaining = group->tabs[0];

            naui_list_free(group->tabs);
            remaining->tabs = NULL;
            remaining->active_tab = 0;
            remaining->parent = group->parent;
            remaining->root = group->root;
            remaining->position = group->position;
            remaining->size = group->size;

            if (group->parent)
            {
                int slot = group->parent->children[0] == group ? 0 : 1;
                group->parent->children[slot] = remaining;
            }
            else if (group == pm.main_viewport)
            {
                pm.main_viewport = remaining;
                naui_set_root_recursive(remaining, remaining);
            }
            else
            {
                remaining->root_index = group->root_index;
                pm.root_nodes[group->root_index] = remaining;
                naui_set_root_recursive(remaining, remaining);
            }

            naui_free_panel_node(group);
        }

        node->parent = NULL;
        node->root = node;
        node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
        naui_list_push(pm.root_nodes, node);
        return;
    }

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
    node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
    naui_list_push(pm.root_nodes, node);
}

void naui_undock_panel(Naui_PanelID id)
{
    naui_list_push(pm.event_queue, ((Naui_PanelEvent){ (Naui_PanelNode*)id, NAUI_PANEL_EVENT_UNDOCK }));
}

static void naui_detach_panel_immediate(Naui_PanelNode *node)
{
    naui_undock_panel_immediate(node);

    Naui_PanelNode *prev = pm.current_panel;
    pm.current_panel = node;
    if (node->type.on_detach)
        node->type.on_detach(node->user_data);
    pm.current_panel = prev;

    if (node->user_data)
        free(node->user_data);

    naui_list_remove(pm.root_nodes, node->root_index);
    naui_free_panel_node(node);
}

static bool naui_range_occludes_point(float mx, float my, uint32_t from, Naui_PanelNode *skip)
{
    for (uint32_t i = from; i < naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *other = pm.root_nodes[i];
        if (skip && (other == skip || other == skip->root))
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
    uint32_t from = (root == pm.main_viewport) ? 0 : root->root_index + 1;
    return naui_range_occludes_point((float)naui_mouse_x(), (float)naui_mouse_y(), from, pm.dragging_node);
}

static bool naui_point_occluded_above(float mx, float my, Naui_PanelNode *root)
{
    uint32_t from = (root == pm.main_viewport) ? 0 : root->root_index + 1;
    return naui_range_occludes_point(mx, my, from, NULL);
}

bool naui_panel_hovered(Naui_PanelID panel_id)
{
    return !((Naui_PanelNode*)panel_id)->occluded;
}

bool naui_any_panel_hovered(void)
{
    return pm.any_panel_hovered;
}

Naui_PanelID naui_current_panel(void)
{
    return (Naui_PanelID)pm.current_panel;
}

static inline bool naui_can_show_dock_guides(Naui_PanelNode *node)
{
    return
        !(node->flags & NAUI_PANEL_FLAG_NO_DOCK_FROM_OTHER) &&
        pm.dragging_node &&
        pm.dragging_node != node &&
        node->root != pm.dragging_node->root &&
        !(pm.dragging_node->flags & NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER);
}

static inline void naui_render_dock_guide_slot(const char *label, Naui_PanelNode *node, Leaf_BoundingBox bb, Naui_DockDirection direciton, bool horizontal, bool occluded)
{
    Leaf_ID id = occluded ? (Leaf_ID){0}: leaf_id_indexed(label, (Naui_PanelID)node);
    bool hovered = occluded ? 0 : leaf_hovered(id);
    leaf({
        .id = id,
        .size = horizontal ?
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_DERIVED} :
            (Leaf_Size){LEAF_SIZE_DERIVED, LEAF_SIZE_GROW},
        .color = hovered ?
            naui_theme_leaf_color(NAUI_DOCK_GUIDE_HOVERED_COLOR) :
            naui_theme_leaf_color(NAUI_DOCK_GUIDE_COLOR),
        .rounding = {
            .value = 8.0f,
            .corners = LEAF_CORNER_ALL
        },
        .border = {
            .width = 1.0f,
            .color = naui_theme_leaf_color(NAUI_DOCK_GUIDE_OUTLINE_COLOR)
        },
        .aspect_ratio = 1.0f
    });

    if (!hovered)
        return;

    switch (direciton)
    {
        case NAUI_DOCK_DIRECTION_TOP:       bb.height *= 0.5f; break;
        case NAUI_DOCK_DIRECTION_BOTTOM:    bb.height *= 0.5f; bb.y += bb.height; break;
        case NAUI_DOCK_DIRECTION_LEFT:      bb.width  *= 0.5f; break;
        case NAUI_DOCK_DIRECTION_RIGHT:     bb.width  *= 0.5f; bb.x  += bb.width; break;
    }

    pm.dock_guide_area = bb;
}

static void naui_render_dock_guides(Naui_PanelNode *node)
{
    const bool occluded = naui_point_occluded_by_higher_panel(node);
    const Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));

    const float scale = 0.5f;

    const Leaf_Size size = box.width > box.height ?
        (Leaf_Size){LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(scale)} :
        (Leaf_Size){LEAF_SIZE_PERCENT(scale), LEAF_SIZE_DERIVED};

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .size = size,
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .aspect_ratio = 1.0f,
        .child_gap = 8.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_TOP_ID, node, box, NAUI_DOCK_DIRECTION_TOP, false, occluded);
        if (naui_can_dock_center(pm.dragging_node))
            naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_CENTER_ID, node, box, NAUI_DOCK_DIRECTION_CENTER, false, occluded);
        else leaf({.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_GROW}, .aspect_ratio = 1.0f});
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_BOTTOM_ID, node, box, NAUI_DOCK_DIRECTION_BOTTOM, false, occluded);
    }

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .direction = LEAF_DIRECTION_HORIZONAL,
        .size = size,
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .aspect_ratio = 1.0f,
        .child_gap = 8.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_LEFT_ID, node, box, NAUI_DOCK_DIRECTION_LEFT, true, occluded);
        leaf({.size = {LEAF_SIZE_GROW, LEAF_SIZE_DERIVED}, .aspect_ratio = 1.0f});
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_RIGHT_ID, node, box, NAUI_DOCK_DIRECTION_RIGHT, true, occluded);
    }
}

static void naui_close_button_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const Leaf_Color leaf_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Color color = { leaf_color.r, leaf_color.g, leaf_color.b, leaf_color.a };
    const float line_width = 0.75f;

    float cx = box.x + box.width * 0.5f;
    float cy = box.y + box.height * 0.5f;
    float half_w = box.width * 0.5f * 0.5f;
    float half_h = box.height * 0.5f * 0.5f;

    Naui_Vec2 top_left     = { cx - half_w, cy - half_h };
    Naui_Vec2 top_right    = { cx + half_w, cy - half_h };
    Naui_Vec2 bottom_left  = { cx - half_w, cy + half_h };
    Naui_Vec2 bottom_right = { cx + half_w, cy + half_h };

    naui_draw_line(top_left,  bottom_right, color, line_width);
    naui_draw_line(top_right, bottom_left,  color, line_width);
}

static void naui_render_close_button(Naui_PanelNode *node, Naui_PanelNode *occlusion_node, float size)
{
    Leaf_ID id = leaf_id_indexed(NAUI_CLOSE_BUTTON_ID, (Naui_PanelID)node);

    bool hovered = pm.resizing_node ? false : leaf_hovered(id);
    node->close_hovered = hovered;

    if (hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT) && naui_panel_hovered((Naui_PanelID)occlusion_node))
    {
        naui_detach_panel((Naui_PanelID)node);
        pm.dragging_node = NULL;
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(size), LEAF_SIZE_FIXED(size)},
        .padding = LEAF_PADDING_AXES(3.0f, 3.0f),
        .color = hovered ?
            naui_theme_leaf_color(NAUI_PANEL_CLOSE_BG_COLOR_TAG) :
            naui_theme_leaf_color(NAUI_PANEL_CLOSE_HOVERED_COLOR_TAG),
        .rounding = {
            .value = 8.0f,
            .corners = LEAF_CORNER_ALL
        },
        .custom_draw = naui_close_button_custom_draw
    });
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
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        leaf_text(node->title, {
            .font_size = font_size,
            .color = text_color,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });

        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(padding.x * 0.5f, padding.y)
        })

        if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE))
            naui_render_close_button(node, node, font_size);
    }
}

static inline void naui_render_docked_panel_tab(Naui_PanelNode *node, Naui_PanelNode *group, bool is_active, Leaf_ID id)
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
        .direction = LEAF_DIRECTION_HORIZONAL,
        .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIXED(font_size)},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .rounding = { rounding, LEAF_CORNER_TL | LEAF_CORNER_TR },
        .color = bg_color,
        .child_gap = 2.0f
    })
    {
        leaf_text(node->title, { .font_size = font_size, .color = text_color });
        if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE) && !pm.resizing_node && leaf_hovered(id))
            naui_render_close_button(node, group, font_size);
        else
        {
            leaf({
                .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                .padding = LEAF_PADDING_AXES(3.0f, 3.0f)
            });
            node->close_hovered = false;
        }
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
            if (node->tabs)
            {
                for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
                {
                    Naui_PanelNode *tab = node->tabs[i];
                    naui_render_docked_panel_tab(tab, node, i == node->active_tab, leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)tab));
                }
            }
            else naui_render_docked_panel_tab(node, node, true, leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node));
        }
    }
}

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    const Leaf_Color bg_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .color = bg_color,
        .rounding = {
            naui_theme_float(NAUI_PANEL_ROUNDING_TAG),
            LEAF_CORNER_TL | LEAF_CORNER_TR
        }
    })
    {
        if (node->parent || node->root == pm.main_viewport || node->tabs)
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
        .inner_shadow = {
            .blur_radius = 64.0f,
            .color = naui_theme_leaf_color(NAUI_PANEL_INNER_SHADOW_COLOR_TAG)
        },
        .clip_children = true
    })
    {
        Naui_PanelNode *tab = node->tabs ? node->tabs[node->active_tab] : node;
        pm.current_panel = tab;
        if (tab->type.on_render)
            tab->type.on_render(tab->user_data);
        if (naui_can_show_dock_guides(node))
            naui_render_dock_guides(node);
    }
}

static void naui_render_next_panel_child(Naui_PanelNode *node)
{
    if (node->children[0])
    {
        Leaf_Color border_color = naui_theme_leaf_color(NAUI_PANEL_BORDER_COLOR_TAG);
        float border_width = naui_theme_float(NAUI_PANEL_BORDER_WIDTH_TAG);

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
                .id = leaf_id_indexed(NAUI_SPLIT_HANDLE_ID, (Naui_PanelID)node),
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
    Leaf_ID id = leaf_id(NAUI_MAIN_VIEWPORT_ID);
    leaf({
        .id = id,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .color = node ?
            naui_theme_leaf_color(NAUI_PANEL_BORDER_COLOR_TAG) :
            naui_theme_leaf_color(NAUI_VIEWPORT_BG_COLOR_TAG),
        .inner_shadow = {
            .blur_radius = 128.0f,
            .color = naui_theme_leaf_color(NAUI_PANEL_INNER_SHADOW_COLOR_TAG)
        }
    })
    {
        if (node)
            naui_render_next_panel_child(node);
        else
        {
            if (pm.dragging_node && !(pm.dragging_node->flags & NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER))
            leaf({
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FIXED(128.0f)},
                .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .aspect_ratio = 1.0f
            })
            {
                naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_VIEWPORT_ID, NULL,
                    leaf_get_bounding_box(id), NAUI_DOCK_DIRECTION_CENTER, false,
                    naui_range_occludes_point((float)naui_mouse_x(), (float)naui_mouse_y(), 0, pm.dragging_node));
            }
        }
    }
}

static void naui_render_dock_guide_area(void)
{
    if (pm.dock_guide_area.width <= 0 || pm.dock_guide_area.height <= 0)
        return;
    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(pm.dock_guide_area.width), LEAF_SIZE_FIXED(pm.dock_guide_area.height)},
        .floating_offset = {pm.dock_guide_area.x, pm.dock_guide_area.y},
        .color = naui_theme_leaf_color(NAUI_DOCK_GUIDE_HOVERED_COLOR)
    });
}

static bool naui_subtree_has_close_hovered(Naui_PanelNode *node)
{
    if (!node) return false;
    if (!node->children[0])
        return node->close_hovered;
    return naui_subtree_has_close_hovered(node->children[0]) ||
           naui_subtree_has_close_hovered(node->children[1]);
}

static Naui_PanelNode *naui_find_hovered_tab(Naui_PanelNode *node, Naui_PanelNode *active_tab, Leaf_ID *out_id)
{
    if (node->flags & NAUI_PANEL_FLAG_NO_UNDOCK)
        return NULL;
 
    if (node->tabs)
    {
        for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
        {
            Leaf_ID id = leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node->tabs[i]);
            if (leaf_hovered(id))
            {
                if (out_id) *out_id = id;
                return node->tabs[i];
            }
        }
        return NULL;
    }
 
    Leaf_ID id = leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)active_tab);
    if (leaf_hovered(id))
    {
        if (out_id) *out_id = id;
        return active_tab;
    }
    return NULL;
}


static inline bool naui_try_begin_panel_move(Naui_PanelNode *node, Naui_Vec2 *drag_offset)
{
    Naui_PanelNode *root = node->root;
 
    if (pm.dragging_node || node == pm.main_viewport)
    {
        if (leaf_hovered(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root)))
            naui_panel_bring_to_front(root);
        return false;
    }

    if (leaf_hovered(leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node)))
    {
        pm.dragging_node = root;
        drag_offset->x = naui_mouse_x() - root->position.x;
        drag_offset->y = naui_mouse_y() - root->position.y;
        naui_panel_bring_to_front(root);
        return true;
    }
 
    if (leaf_hovered(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root)))
        naui_panel_bring_to_front(root);
 
    return false;
}

static inline void naui_handle_panel_click(Naui_PanelNode *node, Naui_PanelNode *active_tab, Naui_Vec2 *drag_offset, Naui_PanelNode **pending_undock_node)
{
    Naui_PanelNode *root = node->root;
 
    if (node->occluded)
        return;
    if (!naui_mouse_clicked(NAUI_MOUSE_LEFT))
        return;
    if (pm.dragging_node && pm.dragging_node->root != root)
        return;
    if (naui_subtree_has_close_hovered(root))
        return;
 
    Naui_PanelNode *hovered_tab = naui_find_hovered_tab(node, active_tab, NULL);
    if (hovered_tab)
    {
        *pending_undock_node = hovered_tab;
        return;
    }
 
    if (!pm.dragging_node && node != pm.main_viewport)
        naui_try_begin_panel_move(node, drag_offset);
}

static inline void naui_apply_pending_undock(Naui_PanelNode *node, Naui_PanelNode *active_tab, Naui_Vec2 *drag_offset, Naui_PanelNode **pending_undock_node)
{
    if (*pending_undock_node != active_tab)
        return;
 
    if (naui_mouse_released(NAUI_MOUSE_LEFT))
    {
        *pending_undock_node = NULL;
        return;
    }
 
    if (!naui_mouse_dragging(NAUI_MOUSE_LEFT))
        return;
 
    naui_undock_panel((Naui_PanelID)active_tab);
 
    Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));
    box.width  = fmaxf(box.width,  active_tab->min_size.x);
    box.height = fmaxf(box.height, active_tab->min_size.y);
 
    active_tab->position = (Naui_Vec2){ box.x, box.y };
    active_tab->size = (node == pm.main_viewport)
        ? (Naui_Vec2){ box.width * 0.5f, box.height * 0.5f }
        : (Naui_Vec2){ box.width, box.height };
 
    pm.dragging_node = active_tab;
    drag_offset->x = naui_mouse_x() - active_tab->position.x;
    drag_offset->y = naui_mouse_y() - active_tab->position.y;
    naui_panel_bring_to_front(active_tab);
 
    *pending_undock_node = NULL;
}
 
static inline void naui_apply_panel_move(Naui_PanelNode *root, Naui_Vec2 drag_offset)
{
    if (root == pm.main_viewport || root->flags & NAUI_PANEL_FLAG_NO_MOVE)
        return;
    if (pm.dragging_node != root)
        return;
 
    root->position.x = naui_mouse_x() - drag_offset.x;
    root->position.y = naui_mouse_y() - drag_offset.y;
}
 
static void naui_update_panel_dragging(Naui_PanelNode *node)
{
    static Naui_Vec2 drag_offset;
    static Naui_PanelNode *pending_undock_node;
 
    if (pm.resizing_node || pm.split_resizing_node)
        return;
 
    Naui_PanelNode *root = node->root;
    Naui_PanelNode *active_tab = node->tabs ? node->tabs[node->active_tab] : node;
 
    naui_handle_panel_click(node, active_tab, &drag_offset, &pending_undock_node);
    naui_apply_pending_undock(node, active_tab, &drag_offset, &pending_undock_node);
    naui_apply_panel_move(root, drag_offset);
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

    if (node->flags & NAUI_PANEL_FLAG_NO_RESIZE || pm.split_resizing_node)
        return;

    Naui_PanelNode *root = node->root;
    if (root == pm.main_viewport)
        return;

    if (pm.resizing_node)
    {
        if (pm.resizing_node != root)
            return;

        float dx = naui_mouse_x() - drag_mouse.x;
        float dy = naui_mouse_y() - drag_mouse.y;

        if (drag_x < 0) dx = fminf(dx, drag_size.x - node->min_size.x);
        if (drag_y < 0) dy = fminf(dy, drag_size.y - node->min_size.y);

        if (drag_x < 0) { root->position.x = drag_pos.x + dx; root->size.x = drag_size.x - dx; }
        else if (drag_x > 0) root->size.x = fmaxf(drag_size.x + dx, node->min_size.x);

        if (drag_y < 0) { root->position.y = drag_pos.y + dy; root->size.y = drag_size.y - dy; }
        else if (drag_y > 0) root->size.y = fmaxf(drag_size.y + dy, node->min_size.y);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            pm.resizing_node = NULL;

        naui_set_cursor(naui_resize_cursor(drag_x, drag_y));
        return;
    }

    if (pm.dragging_node)
        return;

    Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();

    int8_t ex = (mx < b.x + NAUI_PANEL_RESIZE_BORDER) ? -1 : (mx > b.x + b.width - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;
    int8_t ey = (my < b.y + NAUI_PANEL_RESIZE_BORDER) ? -1 : (my > b.y + b.height - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;

    bool inside = mx > b.x - NAUI_PANEL_RESIZE_BORDER && mx < b.x + b.width + NAUI_PANEL_RESIZE_BORDER
               && my > b.y - NAUI_PANEL_RESIZE_BORDER && my < b.y + b.height + NAUI_PANEL_RESIZE_BORDER;

    if (!inside || (!ex && !ey))
        return;

    if (naui_point_occluded_above(mx, my, root))
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

static void naui_update_split_resizing(Naui_PanelNode *node)
{
    static Naui_Vec2 drag_start_mouse;
    static float drag_start_ratio;
    static float drag_total_size;

    if (pm.split_resizing_node)
    {
        if (pm.split_resizing_node != node)
            return;

        float delta = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
            ? naui_mouse_x() - drag_start_mouse.x
            : naui_mouse_y() - drag_start_mouse.y;

        node->split_ratio = fmaxf(0.05f, fminf(0.95f,
            drag_start_ratio + delta / drag_total_size));

        naui_set_cursor(node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
            ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            pm.split_resizing_node = NULL;
        return;
    }

    if (pm.dragging_node || pm.resizing_node)
        return;

    Leaf_BoundingBox b = leaf_get_bounding_box(
        leaf_id_indexed(NAUI_SPLIT_HANDLE_ID, (Naui_PanelID)node));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();

    bool near_divider = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? (fabsf(mx - b.x) < NAUI_PANEL_RESIZE_BORDER && my >= b.y && my <= b.y + b.height)
        : (fabsf(my - b.y) < NAUI_PANEL_RESIZE_BORDER && mx >= b.x && mx <= b.x + b.width);

    if (!near_divider)
        return;

    if (naui_point_occluded_above(mx, my, node->root))
        return;

    naui_set_cursor(node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS);

    if (!naui_mouse_clicked(NAUI_MOUSE_LEFT))
        return;

    pm.split_resizing_node = node;
    drag_start_mouse = (Naui_Vec2){ mx, my };
    drag_start_ratio = node->split_ratio;
    drag_total_size  = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? b.width  / fmaxf(1.0f - drag_start_ratio, 0.001f)
        : b.height / fmaxf(1.0f - drag_start_ratio, 0.001f);
}

static void naui_update_panel_dock_guides(Naui_PanelNode *node)
{
    if (!naui_mouse_released(NAUI_MOUSE_LEFT))
        return;
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_LEFT_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_LEFT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_RIGHT_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_RIGHT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_TOP_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_TOP, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_BOTTOM_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_BOTTOM, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_CENTER_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_CENTER, 0.0f);
}

static void naui_update_splits_only(Naui_PanelNode *node)
{
    if (!node->children[0]) return;
    naui_update_split_resizing(node);
    naui_update_splits_only(node->children[0]);
    naui_update_splits_only(node->children[1]);
}

static inline void naui_calculate_and_cache_panel_occlusion(Naui_PanelNode *node)
{
    Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();
    bool in_bounds = mx >= box.x && mx <= box.x + box.width && my >= box.y && my <= box.y + box.height;
    node->occluded = !in_bounds || naui_point_occluded_above(mx, my, node->root);
    if (!node->occluded)
        pm.any_panel_hovered = true;
}

static void naui_update_panel_tabs(Naui_PanelNode *node)
{
    if (!node->tabs)
        return;
    for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
    {
        if (naui_mouse_clicked(NAUI_MOUSE_LEFT) && !node->tabs[i]->close_hovered && leaf_hovered(leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node->tabs[i])))
        {
            node->active_tab = i;
            break;
        }
    }
}

static void naui_update_panel(Naui_PanelNode *node)
{
    if (node->children[0])
    {
        naui_update_panel(node->children[0]);
        naui_update_panel(node->children[1]);
        return;
    }

    naui_calculate_and_cache_panel_occlusion(node);

    naui_update_panel_tabs(node);
    naui_update_panel_resizing(node);
    naui_update_panel_dragging(node);

    node->position.x = fminf(node->position.x, naui_app_width() - 24.0f);
    node->position.y = fminf(node->position.y, naui_app_height() - 24.0f);

    if (naui_can_show_dock_guides(node))
        naui_update_panel_dock_guides(node);

    Naui_PanelNode *tab = node->tabs ? node->tabs[node->active_tab] : node;
    pm.current_panel = tab;
    if (tab->type.on_update)
        tab->type.on_update(tab->user_data);
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
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_VIEWPORT_ID, 0)))
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
        case NAUI_PANEL_EVENT_DETACH: naui_detach_panel_immediate(event.node); break;
        case NAUI_PANEL_EVENT_BRING_TO_FRONT: naui_panel_bring_to_front_immediate(event.node); break;
        }
    }

    naui_list_clear(pm.event_queue);
}

void naui_render_panels_and_viewport(void)
{
    pm.any_panel_hovered = false;

    for (int32_t i = (int32_t)naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_splits_only(pm.root_nodes[i]);
    if (pm.main_viewport)
        naui_update_splits_only(pm.main_viewport);

    for (int32_t i = (int32_t)naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_panel(pm.root_nodes[i]);
    naui_update_main_viewport();
    
    pm.dock_guide_area = (Leaf_BoundingBox){0};
    if (naui_mouse_released(NAUI_MOUSE_LEFT))
    {
        pm.dragging_node = NULL;
        pm.resizing_node = NULL;
        pm.split_resizing_node = NULL;
    }

    naui_render_main_viewport();
    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
        naui_render_panel(pm.root_nodes[i]);

    naui_render_dock_guide_area();

    naui_process_events();
}

#pragma region Serialization
#include <serialization/json.h>

static inline const char *naui_get_panel_type(Naui_PanelNode *node)
{
    return node->type.type_name;
}

static inline Naui_PanelNode *naui_find_panel_of_type(const char *type_name)
{
    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
        if (!strcmp(pm.root_nodes[i]->type.type_name, type_name))
            return pm.root_nodes[i];
    return NULL;
}

static bool naui_serialize_panel_node(Naui_Json *json, Naui_JsonValue *out, Naui_PanelNode *node)
{
    if (!node)
        return false;

    if (node->children[0])
    {
        Naui_JsonValue *child0 = naui_json_set_object(json, out, "child0");
        Naui_JsonValue *child1 = naui_json_set_object(json, out, "child1");

        bool ok0 = naui_serialize_panel_node(json, child0, node->children[0]);
        bool ok1 = naui_serialize_panel_node(json, child1, node->children[1]);

        if (!ok0 && !ok1)
            return false;

        naui_json_set_string(json, out, "kind", "split");
        naui_json_set_string(json, out, "axis",
            node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ? "vertical" : "horizontal");
        naui_json_set_number(json, out, "ratio", node->split_ratio);
        return true;
    }

    if (node->tabs)
    {
        Naui_JsonValue *tabs_arr = naui_json_set_array(json, out, "tabs");
        bool any = false;
        for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
        {
            Naui_PanelNode *tab = node->tabs[i];
            if (!(tab->flags & NAUI_PANEL_FLAG_SERIALIZABLE))
                continue;
            const char *name = naui_get_panel_type(tab);
            if (!name)
                continue;
            naui_json_push_string(json, tabs_arr, name);
            any = true;
        }
        if (!any)
            return false;

        naui_json_set_string(json, out, "kind", "tabs");
        naui_json_set_int(json, out, "active_tab", node->active_tab);
        return true;
    }

    if (!(node->flags & NAUI_PANEL_FLAG_SERIALIZABLE))
        return false;

    const char *name = naui_get_panel_type(node);
    if (!name)
        return false;

    naui_json_set_string(json, out, "kind", "panel");
    naui_json_set_string(json, out, "type", name);

    return true;
}

bool naui_serialize_viewport(const char *file_path)
{
    if (!pm.main_viewport)
        return false;

    Naui_Json json = naui_json_result_create();
    Naui_JsonValue *root = naui_json_object(&json);
    naui_serialize_panel_node(&json, root, pm.main_viewport);
    naui_json_write_file(root, NAUI_PATH(file_path), true);
    naui_json_free(&json);
}

bool naui_deserialize_viewport(const char *file_path)
{
    Naui_Json json = naui_json_parse_file(NAUI_PATH(file_path));
    Naui_JsonValue *child0 = naui_json_object_get(json.root, "child0");
    naui_json_get_string(child0, naui_json_object_get(child0, "type"));
    // hardcoded example of loading and force docking panels based on type names
    Naui_PanelNode *test = naui_find_panel_of_type("test");
    Naui_PanelNode *test2 = naui_find_panel_of_type("test2");
    if (test && test2)
    {
        naui_set_main_viewport(test);
        naui_dock_panel((Naui_PanelID)test, (Naui_PanelID)test2, NAUI_DOCK_DIRECTION_LEFT, 0.5f); // crashes
    }
}

#pragma endregion Serialization