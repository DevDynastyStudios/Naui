#include "panel.h"
#include "themes.h"
#include "input.h"
#include "utils/map.h"
#include "utils/list.h"

#include <leaf/leaf.h>
#include <stdio.h>

#define NAUI_PANEL_TITLEBAR_ID "__naui_panel_titlebar"
#define NAUI_PANEL_BODY_ID "__naui_panel_body"

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    char title[64];
    Naui_PanelType type;
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    void *user_data;
    Naui_Vec2 position, size;
};

typedef struct
{
    char *key;
    Naui_PanelType value;
}
Naui_PanelTypeMapEntry;

typedef struct
{
    Naui_Map(Naui_PanelTypeMapEntry) panel_type_map;
    Naui_List(Naui_PanelNode*) root_nodes;
    Naui_PanelNode *dragged_node;
    Naui_PanelNode *hovered_node;
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

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_title_bg_color");
    Leaf_Color text_color = naui_theme_leaf_color("naui_panel_title_text_color");
    Naui_Vec2 padding = naui_theme_vec2("naui_panel_title_padding");
    float font_size = naui_theme_float("naui_font_size");

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = bg_color
    })
    {
        leaf_text(node->title, {
            .font_size = font_size,
            .color = text_color,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

static inline void naui_render_panel_body(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_bg_color");
    Naui_Vec2 padding = naui_theme_vec2("naui_panel_padding");

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_BODY_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .color = bg_color
    })
    {
        if (node->type.on_render)
            node->type.on_render((Naui_PanelID)node, node->user_data);
    }
}

static void naui_render_panel(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_bg_col");
    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
        .floating_offset = {node->position.x, node->position.y},
        .clip_children = true
    })
    {
        naui_render_panel_titlebar(node);
        naui_render_panel_body(node);
    }
}

static void naui_panel_bring_to_from(Naui_PanelNode *node)
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
    static Naui_Vec2 first_position;

    Leaf_ID titlebar_id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node);
    Leaf_ID body_id = leaf_id_indexed(NAUI_PANEL_BODY_ID, (Naui_PanelID)node);

    bool hovered = leaf_hovered(titlebar_id) || leaf_hovered(body_id);

    if (hovered && !pm.dragged_node)
    {
        if (naui_mouse_clicked(NAUI_MOUSE_LEFT))
        {
            Leaf_BoundingBox box = leaf_get_bounding_box(titlebar_id);
            first_position.x = naui_mouse_x() - box.x;
            first_position.y = naui_mouse_y() - box.y;
            pm.dragged_node = node;
            naui_panel_bring_to_from(node);
        }
    }

    if (pm.dragged_node != node)
        return;
    
    node->position.x = naui_mouse_x() - first_position.x;
    node->position.y = naui_mouse_y() - first_position.y;

}

static void naui_update_panel(Naui_PanelNode *node)
{
    if (!node->children[0])
    {
        naui_update_panel_movement(node);
        if (node->type.on_update)
            node->type.on_update((Naui_PanelID)node, node->user_data);
        return;
    }

    naui_update_panel(node->children[0]);
    naui_update_panel(node->children[1]);
}

void naui_panel_manager_frame(void)
{
    if (!naui_mouse_down(NAUI_MOUSE_LEFT))
        pm.dragged_node = NULL;

    for (size_t i = naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_panel(pm.root_nodes[i]);

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

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{

}

void naui_undock_panel(Naui_PanelID id)
{

}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    strncpy(((Naui_PanelNode*)panel_id)->title, title, sizeof(((Naui_PanelNode*)0)->title));
}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->size = size;
}