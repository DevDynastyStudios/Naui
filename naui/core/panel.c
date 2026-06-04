#include "panel.h"
#include "theme.h"

#include "utils/map.h"
#include "utils/list.h"

#include <leaf/leaf.h>
#include <stdio.h>

#define NAUI_PANEL_DEFAULT_WIDTH 1280
#define NAUI_PANEL_DEFAULT_HEIGHT 720

#define NAUI_ROOT_PANEL_ID "__naui_root_panel"

typedef uint8_t Naui_SplitAxis;
enum
{
    NAUI_SPLIT_AXIS_VERTICAL = LEAF_LAYOUT_VERTICAL,
    NAUI_SPLIT_AXIS_HORIZONTAL = LEAF_LAYOUT_HORIZONAL
};

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    const char     *title;
    Naui_PanelType  type;
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    Naui_PanelNode  *root;
    Naui_List(Naui_PanelNode*) tabs;
    void           *user_data;
    Naui_Vec2       position, size;
    Naui_PanelFlags flags;
    uint32_t        root_index;
    int32_t         active_tab;
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

static void naui_panel_bring_to_front(Naui_PanelNode *node)
{
    Naui_PanelNode *root = node->root;

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

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{
    Naui_PanelNode *target = (Naui_PanelNode*)target_id;
    Naui_PanelNode *guest  = (Naui_PanelNode*)guest_id;

    naui_list_remove(pm.root_nodes, guest->root_index);

    Naui_PanelNode *dock_node = naui_alloc_panel_node();

    if (target->parent)
    {
        dock_node->parent = target->parent;
        target->parent->children[target->parent->children[0] == target ? 0 : 1] = dock_node;
    }
    else
    {
        dock_node->root_index = target->root_index;
        pm.root_nodes[target->root_index] = dock_node;
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

    guest->root  = target->root;
    guest->parent = dock_node;

    dock_node->size = target->size;
    dock_node->position = target->position;

    return dock_node;
}

static void naui_undock_panel_immediate(Naui_PanelID id)
{
    Naui_PanelNode *node = (Naui_PanelNode*)id;
    Naui_PanelNode *dock_node = node->parent;

    if (!dock_node)
        return;

    Naui_PanelNode *sibling = dock_node->children[dock_node->children[0] == node ? 1 : 0];

    if (dock_node->parent)
    {
        int slot = dock_node->parent->children[0] == dock_node ? 0 : 1;
        dock_node->parent->children[slot] = sibling;
        sibling->parent = dock_node->parent;
    }
    else
    {
        sibling->root_index = dock_node->root_index;
        pm.root_nodes[dock_node->root_index] = sibling;
        sibling->parent = NULL;
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

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    const Leaf_Color bg_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);
    const Leaf_Color text_color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Vec2 padding = naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG);
    const float font_size = naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG);

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .color = bg_color,
        .rounding = {
            naui_theme_float(NAUI_PANEL_ROUNDING_TAG),
            LEAF_CORNER_TL | LEAF_CORNER_TR
        }
    })
    {
        leaf({
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(font_size)},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        })
        {
            leaf_text(node->title, {
                .font_size = font_size,
                .color = text_color,
                .alignment = LEAF_TEXT_ALIGN_CENTER
            });
        }
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

    naui_render_panel_titlebar(node);
    naui_render_panel_body(node);
}

static void naui_render_panel(Naui_PanelNode *node)
{
    leaf({
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
        .clip_children = true
    })
    {
        naui_render_next_panel_child(node);
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

    if (node->type.on_update)
        node->type.on_update((Naui_PanelID)node, node->user_data);
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

    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
        naui_render_panel(pm.root_nodes[i]);

    naui_process_events();
}