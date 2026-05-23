#include "panel.h"
#include "input.h"
#include "utils/map.h"
#include "utils/list.h"
#include "utils/uuid.h"

#include <leaf/leaf.h>

#define NAUI_PANEL_STACK_SIZE (1 << 12)
typedef struct
{
    char title[128];
    Naui_PanelType type;
    uint8_t _stack[NAUI_PANEL_STACK_SIZE];
}
Naui_PanelData;

typedef uint8_t Naui_SplitAxis;
enum
{
    NAUI_SPLIT_AXIS_VERTICAL   = LEAF_LAYOUT_VERTICAL,
    NAUI_SPLIT_AXIS_HORIZONTAL = LEAF_LAYOUT_HORIZONAL
};

typedef struct Naui_PanelNode
{
    struct Naui_PanelNode *children[2];
    struct Naui_PanelNode *parent;
    struct Naui_PanelNode *root;
    Naui_PanelData *panel_data;
    Naui_Vec2 position;
    Naui_Vec2 size;
    float split_ratio;
    Naui_SplitAxis split_axis;
}
Naui_PanelNode;

static inline bool naui_panel_node_leaf(const Naui_PanelNode *node)
{
    return !node->children[0] && !node->children[1];
}

static inline Naui_PanelData *naui_get_panel_data(Naui_PanelID panel_id)
{
    return ((Naui_PanelNode*)panel_id)->panel_data;
}

static inline Naui_PanelNode *naui_alloc_panel_node(void)
{
    return calloc(1, sizeof(Naui_PanelNode));
}

static inline void naui_free_panel_node(Naui_PanelNode *node)
{
    free(node);
}

static void naui_set_root(Naui_PanelNode *node, Naui_PanelNode *root)
{
    if (!node) return;
    node->root = root;
    naui_set_root(node->children[0], root);
    naui_set_root(node->children[1], root);
}

static Naui_List(Naui_PanelNode*) root_panel_node_list = NULL;

typedef struct
{
    char *key;
    Naui_PanelType value;
}
Naui_PanelTypeMapEntry;
static Naui_Map(Naui_PanelTypeMapEntry) panel_type_map = NULL;

static void naui_docking_guide(Naui_PanelID id)
{
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
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    leaf({
        .size         = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT_MIN_MAX(0.8f, 32.0f, 400.0f)},
        .aspect_ratio = 1.0f
    })
    {
        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap       = 6.0f
        })
        {
            leaf({ .id = guide_top_id,    .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_top_hovered    ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .id = guide_center_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_center_hovered ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .id = guide_bottom_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_bottom_hovered ? hovered_color : bg_color, .rounding = 6.0f });
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
                   .color = guide_left_hovered  ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)} });
            leaf({ .id = guide_right_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_right_hovered ? hovered_color : bg_color, .rounding = 6.0f });
        }
    }
}

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction)
{
    Naui_PanelNode *target_node = (Naui_PanelNode*)target_id;
    Naui_PanelNode *guest_node  = (Naui_PanelNode*)guest_id;
    Naui_PanelNode *guest_root  = guest_node->root;
    Naui_PanelNode *target_copy = naui_alloc_panel_node();

    int32_t len = naui_list_len(root_panel_node_list);
    for (int32_t i = 0; i < len; i++)
    {
        if (root_panel_node_list[i] == guest_root)
        {
            naui_list_uremove(root_panel_node_list, i);
            break;
        }
    }

    target_copy->panel_data = target_node->panel_data;
    target_copy->parent     = target_node;
    guest_root->parent      = target_node;

    bool guest_first = (direction == NAUI_DOCK_DIRECTION_LEFT || direction == NAUI_DOCK_DIRECTION_TOP);

    target_node->children[0] = guest_first ? guest_root  : target_copy;
    target_node->children[1] = guest_first ? target_copy : guest_root;

    target_node->split_ratio = 0.5f;
    target_node->split_axis  =
        (direction == NAUI_DOCK_DIRECTION_LEFT ||
         direction == NAUI_DOCK_DIRECTION_RIGHT)
        ? NAUI_SPLIT_AXIS_HORIZONTAL
        : NAUI_SPLIT_AXIS_VERTICAL;

    naui_set_root(target_copy, target_node->root);
    naui_set_root(guest_root,  target_node->root);

    return target_copy;
}

void naui_undock_panel(Naui_PanelID id)
{
    Naui_PanelNode *node   = (Naui_PanelNode*)id;
    Naui_PanelNode *parent = node->parent;

    if (!parent)
        return;

    bool node_is_first      = (parent->children[0] == node);
    Naui_PanelNode *sibling = parent->children[node_is_first ? 1 : 0];

    Naui_Vec2 inherited_position = parent->root->position;
    Naui_Vec2 inherited_size     = parent->size;

    parent->panel_data  = sibling->panel_data;
    parent->split_ratio = sibling->split_ratio;
    parent->split_axis  = sibling->split_axis;
    parent->children[0] = sibling->children[0];
    parent->children[1] = sibling->children[1];

    if (parent->children[0]) parent->children[0]->parent = parent;
    if (parent->children[1]) parent->children[1]->parent = parent;

    naui_free_panel_node(sibling);

    naui_set_root(parent, parent->root);

    node->parent = NULL;
    node->root   = node;
    node->position = inherited_position;
    node->size     = inherited_size;

    naui_set_root(node, node);

    naui_list_push(root_panel_node_list, node);
}

static const Leaf_Color LEAF_DBG_BG1      = {37,  35,  33,  255};
static const Leaf_Color LEAF_DBG_BG2      = {46,  44,  42,  255};
static const Leaf_Color LEAF_DBG_TEXT     = {237, 226, 231, 255};
static const Leaf_Color LEAF_DBG_BORDER   = {90,  88,  85,  255};
static const Leaf_Color RESIZE_HANDLE_CLR = {120, 118, 115, 255};

static Naui_PanelNode *dragged_node  = NULL;
static bool dragging_titlebar        = false;

static void naui_render_leaf_panel_node(Naui_PanelNode *node)
{
    Naui_PanelData *panel_data = node->panel_data;
    Leaf_ID titlebar_tab_id = leaf_id_indexed("__naui_panel_titlebar_tab",  (Naui_PanelID)node);
    Leaf_ID titlebar_id     = leaf_id_indexed("__naui_panel_titlebar",      (Naui_PanelID)node);
    Leaf_ID body_id         = leaf_id_indexed("__naui_panel_body",          (Naui_PanelID)node);

    leaf({
        .id              = titlebar_id,
        .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP},
        .color           = LEAF_DBG_BG2
    })
    {
        if (node->parent)
            leaf({
                .id              = titlebar_tab_id,
                .padding         = LEAF_PADDING_ALL(8.0f),
                .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
                .color           = LEAF_DBG_BG1,
                .clip_children   = true
            })
            {
                leaf({
                    .direction = LEAF_LAYOUT_HORIZONAL
                })
                leaf_text(panel_data->title, {
                    .color     = LEAF_DBG_TEXT,
                    .alignment = LEAF_TEXT_ALIGN_LEFT,
                    .font_size = 26.0f
                });
            }
        else
            leaf({
                .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
                .padding         = LEAF_PADDING_ALL(8.0f),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .clip_children   = true
            })
            {
                leaf_text(panel_data->title, {
                    .color     = LEAF_DBG_TEXT,
                    .alignment = LEAF_TEXT_ALIGN_CENTER,
                    .font_size = 26.0f
                });
            }
    }

    leaf({
        .id            = body_id,
        .size          = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .color         = LEAF_DBG_BG1,
        .clip_children = true
    })
    {
        if (panel_data->type.on_render)
            panel_data->type.on_render((Naui_PanelID)node, (void*)panel_data->_stack);
        if (dragged_node && dragged_node != node && dragging_titlebar)
            naui_docking_guide((Naui_PanelID)node);
    }
}

static void naui_update_leaf_panel_node(Naui_PanelNode *node)
{
    Naui_PanelData *panel_data = node->panel_data;
    if (panel_data->type.on_update)
        panel_data->type.on_update((Naui_PanelID)node, (void*)panel_data->_stack);

    if (!dragged_node)
        return;

    Naui_PanelID id         = (Naui_PanelID)node;
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

    if (naui_mouse_released(NAUI_MOUSE_BUTTON_LEFT))
    {
        if (guide_left_hovered)
        {
            naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)dragged_node, NAUI_DOCK_DIRECTION_LEFT);
            dragged_node = NULL;
        }
        else if (guide_right_hovered)
        {
            naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)dragged_node, NAUI_DOCK_DIRECTION_RIGHT);
            dragged_node = NULL;
        }
        else if (guide_top_hovered)
        {
            naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)dragged_node, NAUI_DOCK_DIRECTION_TOP);
            dragged_node = NULL;
        }
        else if (guide_bottom_hovered)
        {
            naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)dragged_node, NAUI_DOCK_DIRECTION_BOTTOM);
            dragged_node = NULL;
        }
    }
}

static void naui_panel_bring_to_front(Naui_PanelID id)
{
    Naui_PanelNode *node = ((Naui_PanelNode *)id)->root;

    int32_t len   = naui_list_len(root_panel_node_list);
    int32_t index = -1;
    for (int32_t i = 0; i < len; i++)
    {
        if (root_panel_node_list[i] == node)
        {
            index = i;
            break;
        }
    }

    if (index < 0 || index == len - 1)
        return;

    for (int32_t i = index; i < len - 1; i++)
        root_panel_node_list[i] = root_panel_node_list[i + 1];

    root_panel_node_list[len - 1] = node;
}

static void naui_update_panel_node(Naui_PanelNode *node)
{
    Leaf_ID titlebar_tab_id = leaf_id_indexed("__naui_panel_titlebar_tab", (Naui_PanelID)node);
    Leaf_ID titlebar_id     = leaf_id_indexed("__naui_panel_titlebar", (Naui_PanelID)node);
    Leaf_ID body_id         = leaf_id_indexed("__naui_panel_body",     (Naui_PanelID)node);

    if (naui_panel_node_leaf(node))
    {
        bool hovered_titlebar = leaf_hovered(titlebar_id);
        bool hovered_body     = leaf_hovered(body_id);

        static Naui_Vec2 position_offset;
        if (!dragged_node && hovered_titlebar)
        {
            if (naui_mouse_clicked(NAUI_MOUSE_BUTTON_LEFT))
            {
                if (leaf_hovered(titlebar_tab_id))
                {
                    naui_undock_panel(node);
                    Leaf_BoundingBox bbox = leaf_get_bounding_box(body_id);
                    node->position.x = bbox.x;
                    node->position.y = bbox.y - 30.0f;
                    node->size.x = bbox.width;
                    node->size.y = bbox.height;
                }
                naui_panel_bring_to_front((Naui_PanelID)node);
                Naui_PanelNode *root = node->root;
                position_offset.x = naui_mouse_x() - root->position.x;
                position_offset.y = naui_mouse_y() - root->position.y;
                dragged_node      = node;
                dragging_titlebar = hovered_titlebar;
            }
        }
        if (dragged_node == node)
        {
            Naui_PanelNode *root = node->root;
            root->position.x = naui_mouse_x() - position_offset.x;
            root->position.y = naui_mouse_y() - position_offset.y;
        }

        naui_update_leaf_panel_node(node);
        return;
    }

    naui_update_panel_node(node->children[0]);
    if (node->children[1])
        naui_update_panel_node(node->children[1]);
}

static void naui_render_panel_node(Naui_PanelNode *node)
{
    if (naui_panel_node_leaf(node))
    {
        naui_render_leaf_panel_node(node);
        return;
    }

    leaf({
        .size      = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .direction = (Leaf_LayoutDirection)node->split_axis
    })
    {
        const float split_aspect = node->split_ratio;
        const Leaf_Size size = node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ?
            (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_PERCENT(split_aspect)} :
            (Leaf_Size){LEAF_SIZE_PERCENT(split_aspect), LEAF_SIZE_FULL};

        leaf({ .size = size, .border = {1.0f, LEAF_DBG_BORDER} })
            naui_render_panel_node(node->children[0]);

        leaf({ .size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW}, .border = {1.0f, LEAF_DBG_BORDER} })
            naui_render_panel_node(node->children[1]);
    }
}

void naui_panel_manager_render(void)
{
    static Naui_PanelNode *pending_front = NULL;

    for (int32_t i = naui_list_len(root_panel_node_list) - 1; i >= 0; i--)
    {
        Naui_PanelNode *node = root_panel_node_list[i];
        naui_update_panel_node(node);
    }

    if (pending_front)
    {
        naui_panel_bring_to_front((Naui_PanelID)pending_front);
        pending_front = NULL;
    }

    for (int32_t i = 0; i < naui_list_len(root_panel_node_list); i++)
    {
        Naui_PanelNode *node = root_panel_node_list[i];
        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_ROOT,
            .size            = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
            .floating_offset = {node->position.x, node->position.y},
            .border          = {1.0f, LEAF_DBG_BORDER},
            .clip_children   = true
        })
        {
            naui_render_panel_node(node);
        }
    }

    if (!naui_mouse_down(NAUI_MOUSE_BUTTON_LEFT))
        dragged_node = NULL;
}

void naui_register_panel_type(const char *name, Naui_PanelType type_events)
{
    naui_strmap_put(panel_type_map, name, type_events);
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    Naui_PanelNode *node  = naui_alloc_panel_node();
    Naui_PanelData *panel = (Naui_PanelData*)malloc(sizeof(Naui_PanelData));
    node->panel_data      = panel;
    node->position        = (Naui_Vec2){100, 100};
    node->size            = (Naui_Vec2){1280, 720};
    node->root            = node;
    panel->type           = naui_strmap_get(panel_type_map, type_name);
    naui_list_push(root_panel_node_list, node);
    if (panel->type.on_attach)
        panel->type.on_attach((Naui_PanelID)node, (void*)panel->_stack);
    return (Naui_PanelID)node;
}

void naui_detach_panel(Naui_PanelID id)
{
    Naui_PanelNode *node = (Naui_PanelNode*)id;
    Naui_PanelData *data = naui_get_panel_data(id);
    if (data->type.on_detach)
        data->type.on_detach(id, (void*)data->_stack);
    uint32_t len = (uint32_t)naui_list_len(root_panel_node_list);
    for (uint32_t i = 0; i < len; i++)
    {
        if (root_panel_node_list[i] == node)
        {
            naui_list_uremove(root_panel_node_list, i);
            break;
        }
    }
    free(data);
    free(node);
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    Naui_PanelData *panel = naui_get_panel_data(panel_id);
    strncpy(panel->title, title, sizeof(panel->title));
}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->root->size = size;
}