#include "panel.h"
#include "input.h"
#include "utils/map.h"
#include "utils/list.h"
#include "utils/uuid.h"

#include <leaf/leaf.h>

static Naui_List(Naui_Panel*) panel_list = NULL;

typedef struct
{
    char *key;
    Naui_PanelType value;
}
Naui_PanelTypeMapEntry;
static Naui_Map(Naui_PanelTypeMapEntry) panel_type_map = NULL;

static Naui_PanelID s_drag_id      = 0;
static Naui_Vec2    s_drag_offset  = {0, 0};

static Naui_PanelID s_resize_id        = 0;
static Naui_Vec2    s_resize_origin    = {0, 0};
static Naui_Vec2    s_resize_orig_size = {0, 0};

#define RESIZE_HANDLE_SIZE 14.0f
#define PANEL_MIN_WIDTH    120.0f
#define PANEL_MIN_HEIGHT    80.0f

static void naui_panel_bring_to_front(Naui_Panel *panel)
{
    size_t len = naui_list_len(panel_list);
    for (size_t i = 0; i < len; i++)
    {
        if (panel_list[i] == panel)
        {
            for (size_t j = i; j < len - 1; j++)
                panel_list[j] = panel_list[j + 1];
            panel_list[len - 1] = panel;
            return;
        }
    }
}

static void naui_docking_guide(uint64_t id)
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
        .size         = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT_MIN_MAX(1.0f, 64.0f, 300.0f)},
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
            leaf({ .id = guide_left_id,   .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_left_hovered   ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .id = guide_center_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_center_hovered ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .id = guide_right_id,  .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_right_hovered  ? hovered_color : bg_color, .rounding = 6.0f });
        }
        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .direction       = LEAF_LAYOUT_HORIZONAL,
            .size            = {LEAF_SIZE_PERCENT(1.0f), LEAF_SIZE_PERCENT(1.0f)},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap       = 6.0f
        })
        {
            leaf({ .id = guide_top_id,    .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_top_hovered    ? hovered_color : bg_color, .rounding = 6.0f });
            leaf({ .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)} });
            leaf({ .id = guide_bottom_id, .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                   .color = guide_bottom_hovered ? hovered_color : bg_color, .rounding = 6.0f });
        }
    }
}

void naui_panel_manager_render(void)
{
    static const Leaf_Color LEAF_DBG_BG1      = {37,  35,  33,  255};
    static const Leaf_Color LEAF_DBG_BG2      = {46,  44,  42,  255};
    static const Leaf_Color LEAF_DBG_TEXT     = {237, 226, 231, 255};
    static const Leaf_Color LEAF_DBG_BORDER   = {90,  88,  85,  255};
    static const Leaf_Color RESIZE_HANDLE_CLR = {120, 118, 115, 255};

    float mx          = (float)naui_mouse_x();
    float my          = (float)naui_mouse_y();
    bool  lmb_down    = naui_mouse_down(NAUI_MOUSE_BUTTON_LEFT);
    bool  lmb_clicked = naui_mouse_clicked(NAUI_MOUSE_BUTTON_LEFT);

    if (s_drag_id && lmb_down)
    {
        Naui_Panel *p = (Naui_Panel*)s_drag_id;
        if (p)
        {
            p->position.x = mx - s_drag_offset.x;
            p->position.y = my - s_drag_offset.y;
        }
    }
    else
    {
        s_drag_id = 0;
    }

    if (s_resize_id && lmb_down)
    {
        Naui_Panel *p = (Naui_Panel*)s_resize_id;
        if (p)
        {
            float new_w = s_resize_orig_size.x + (mx - s_resize_origin.x);
            float new_h = s_resize_orig_size.y + (my - s_resize_origin.y);
            if (new_w < PANEL_MIN_WIDTH)  new_w = PANEL_MIN_WIDTH;
            if (new_h < PANEL_MIN_HEIGHT) new_h = PANEL_MIN_HEIGHT;
            p->size.x = new_w;
            p->size.y = new_h;
        }
    }
    else
    {
        s_resize_id = 0;
    }

    if (lmb_clicked && s_drag_id == 0 && s_resize_id == 0)
    {
        for (int32_t i = naui_list_len(panel_list) - 1; i >= 0; i--)
        {
            Naui_Panel *panel = panel_list[i];
            if (panel->docked) continue;

            Leaf_ID titlebar_id      = leaf_id_indexed("__naui_panel_titlebar",      (Naui_PanelID)panel);
            Leaf_ID resize_handle_id = leaf_id_indexed("__naui_panel_resize_handle", (Naui_PanelID)panel);

            if (leaf_hovered(titlebar_id))
            {
                naui_panel_bring_to_front(panel);
                s_drag_id       = (Naui_PanelID)panel;
                s_drag_offset.x = mx - panel->position.x;
                s_drag_offset.y = my - panel->position.y;
                break;
            }

            if (leaf_hovered(resize_handle_id))
            {
                naui_panel_bring_to_front(panel);
                s_resize_id        = (Naui_PanelID)panel;
                s_resize_origin    = (Naui_Vec2){mx, my};
                s_resize_orig_size = panel->size;
                break;
            }
        }
    }

    for (int32_t i = 0; i < naui_list_len(panel_list); i++)
    {
        Naui_Panel *panel = panel_list[i];
        if (panel->docked) continue;

        Leaf_ID titlebar_id      = leaf_id_indexed("__naui_panel_titlebar",      (Naui_PanelID)panel);
        Leaf_ID resize_handle_id = leaf_id_indexed("__naui_panel_resize_handle", (Naui_PanelID)panel);

        bool resize_handle_hovered = leaf_hovered(resize_handle_id);

        leaf({
            .positioning     = LEAF_POSITIONING_FLOATING_TO_ROOT,
            .size            = {LEAF_SIZE_FIXED(panel->size.x), LEAF_SIZE_FIXED(panel->size.y)},
            .floating_offset = {panel->position.x, panel->position.y},
            .border          = {leaf_solid(LEAF_DBG_BORDER), 1.0f},
            .clip_children   = true
        })
        {
            leaf({
                .id              = titlebar_id,
                .size            = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(30.0f)},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .color           = LEAF_DBG_BG2
            })
            {
                leaf_text(panel->title, {
                    .color     = LEAF_DBG_TEXT,
                    .alignment = LEAF_TEXT_ALIGN_CENTER,
                    .font_size = 20.0f
                });
            }

            leaf({
                .size  = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
                .color = LEAF_DBG_BG1
            })
            {
                if (panel->type.on_update)
                    panel->type.on_update((Naui_PanelID)panel, (void*)panel->_stack);
                naui_docking_guide((Naui_PanelID)panel);
            }

            leaf({
                .positioning     = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .floating_offset = {
                    panel->size.x - RESIZE_HANDLE_SIZE,
                    panel->size.y - RESIZE_HANDLE_SIZE
                },
                .id              = resize_handle_id,
                .size            = {
                    LEAF_SIZE_FIXED(RESIZE_HANDLE_SIZE),
                    LEAF_SIZE_FIXED(RESIZE_HANDLE_SIZE)
                },
                .color           = leaf_solid(resize_handle_hovered
                                       ? leaf_rgba(160, 158, 155, 255)
                                       : RESIZE_HANDLE_CLR),
                .rounding        = 2.0f
            });
        }
    }
}

void naui_register_panel_type(const char *name, Naui_PanelType type_events)
{
    naui_strmap_put(panel_type_map, strdup(name), type_events);
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    Naui_Panel *panel = (Naui_Panel*)malloc(sizeof(Naui_Panel));
    panel->position   = (Naui_Vec2){100, 100};
    panel->size       = (Naui_Vec2){400, 300};
    panel->docked     = false;
    panel->type       = naui_strmap_get(panel_type_map, type_name);
    naui_list_push(panel_list, panel);
    if (panel->type.on_attach)
        panel->type.on_attach((Naui_PanelID)panel, (void*)panel->_stack);
    return (Naui_PanelID)panel;
}

void naui_detach_panel(Naui_PanelID id)
{
    Naui_Panel *panel = (Naui_Panel*)id;
    if (panel->type.on_detach)
        panel->type.on_detach((Naui_PanelID)panel, (void*)panel->_stack);
    if (s_drag_id   == id) s_drag_id   = 0;
    if (s_resize_id == id) s_resize_id = 0;
    for (int32_t i = 0; i < naui_list_len(panel_list) && panel_list[i] != panel; i++)
        naui_list_uremove(panel_list, i);
    free(panel);
}

static inline Naui_Panel *naui_get_panel(Naui_PanelID panel_id)
{
    return (Naui_Panel*)panel_id;
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    Naui_Panel *panel = naui_get_panel(panel_id);
    strncpy(panel->title, title, sizeof(panel->title));
}