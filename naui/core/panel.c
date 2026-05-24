#include "panel.h"
#include "themes.h"
#include "utils/list.h"

#include <leaf/leaf.h>

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    Naui_Vec2 position, size;
};

typedef struct
{
    Naui_List(Naui_PanelNode*) root_nodes;
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

static void naui_render_panel(Naui_PanelNode *node)
{
    Leaf_Color bg_color = naui_theme_leaf_color("naui_panel_bg_col");

    leaf({
        .size = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
        .color = bg_color
    })
    {

    }
}

static void naui_update_panel(Naui_PanelNode *node)
{
    
}

void naui_panel_manager_frame(void)
{
    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        naui_update_panel(pm.root_nodes[i]);
    }

    for (size_t i = 0; i < naui_list_len(pm.root_nodes); i++)
    {
        naui_render_panel(pm.root_nodes[i]);
    }
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    Naui_PanelNode *node = naui_alloc_panel_node();
    node->size = (Naui_Vec2) { 1280, 720 };
    naui_list_push(pm.root_nodes, node);
}

void naui_detach_panel(Naui_PanelID id)
{
    Naui_PanelNode *node = (Naui_PanelNode*)id;
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

void naui_register_panel_type(const char *name, Naui_PanelType type)
{

}

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{

}

void naui_undock_panel(Naui_PanelID id)
{

}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{

}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    
}