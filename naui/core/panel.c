#include "panel.h"
#include "utils/map.h"
#include "utils/uuid.h"

#include <leaf/leaf.h>

typedef struct
{
    Naui_PanelUID key;
    Naui_Panel *value;
}
Naui_PanelMapEntry;
static Naui_Map(Naui_PanelMapEntry) panel_map = NULL;

typedef struct
{
    char *key;
    Naui_PanelTypeEvents value;
}
Naui_PanelTypeMapEntry;
static Naui_Map(Naui_PanelTypeMapEntry) panel_type_map = NULL;

static void naui_docking_guide(uint64_t uid)
{
    Leaf_ID guide_left_id = leaf_id_indexed("__naui_dock_guide_left", uid);
    Leaf_ID guide_right_id = leaf_id_indexed("__naui_dock_guide_right", uid);
    Leaf_ID guide_center_id = leaf_id_indexed("__naui_dock_guide_center", uid);
    Leaf_ID guide_top_id = leaf_id_indexed("__naui_dock_guide_top", uid);
    Leaf_ID guide_bottom_id = leaf_id_indexed("__naui_dock_guide_bottom", uid);

    bool guide_left_hovered = leaf_hovered(guide_left_id);
    bool guide_right_hovered = leaf_hovered(guide_right_id);
    bool guide_center_hovered = leaf_hovered(guide_center_id);
    bool guide_top_hovered = leaf_hovered(guide_top_id);
    bool guide_bottom_hovered = leaf_hovered(guide_bottom_id);

    Leaf_Color bg_color = leaf_rgba(100, 100, 255, 100);
    Leaf_Color hovered_color = leaf_rgba(100, 100, 255, 200);

    leaf ({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    leaf({
        .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT_MIN_MAX(1.0f, 64.0f, 300.0f)},
        .aspect_ratio = 1.0f
    })
    {
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap = 6.0f
        })
        {
            leaf({
                .id = guide_left_id,
                .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                .color = guide_left_hovered ? hovered_color : bg_color,
                .rounding = 6.0f
            });

            leaf({
                .id = guide_center_id,
                .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                .color = guide_center_hovered ? hovered_color : bg_color,
                .rounding = 6.0f
            });

            leaf({
                .id = guide_right_id,
                .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                .color = guide_right_hovered ? hovered_color : bg_color,
                .rounding = 6.0f
            });
        }
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .direction = LEAF_LAYOUT_HORIZONAL,
            .size = {LEAF_SIZE_PERCENT(1.0f), LEAF_SIZE_PERCENT(1.0f)},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap = 6.0f
        })
        {
            leaf({
                .id = guide_top_id,
                .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                .color = guide_top_hovered ? hovered_color : bg_color,
                .rounding = 6.0f
            });

            leaf({.size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)}});

            leaf({
                .id = guide_bottom_id,
                .size = {LEAF_SIZE_PERCENT(0.3f), LEAF_SIZE_PERCENT(0.3f)},
                .color = guide_bottom_hovered ? hovered_color : bg_color,
                .rounding = 6.0f
            });
        }
    }
}

void naui_panel_manager_render(void)
{
    for (int32_t i = 0; i < naui_intmap_len(panel_map); i++)
    {
        Naui_Panel *panel = panel_map[i].value;
        if (!panel->docked)
            leaf({
                .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
                .size = {LEAF_SIZE_FIXED(panel->size.x), LEAF_SIZE_FIXED(panel->size.y)},
                .floating_offset = {panel->position.x, panel->position.y},
                .border = { leaf_solid(LEAF_COLOR_WHITE), 1.0f },
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .clip_children = true
            })
            {
                if (panel->events.on_update)
                    panel->events.on_update(panel->uid, (void*)panel->_stack);
                naui_docking_guide(panel->uid);
            }
    }
}

void naui_register_panel_type(const char *name, Naui_PanelTypeEvents type_events)
{
    naui_strmap_put(panel_type_map, name, type_events);
}

void naui_unregister_panel_type(const char *name)
{
    naui_strmap_del(panel_type_map, name);
}

Naui_PanelUID naui_attach_panel(const char *type_name)
{
    Naui_PanelUID uuid = naui_uuid_generate();
    Naui_Panel *panel = (Naui_Panel*)malloc(sizeof(Naui_Panel));
    panel->uid = uuid;
    panel->position = (Naui_Vec2){100, 100};
    panel->size = (Naui_Vec2){400, 300};
    panel->docked = false;
    panel->events = naui_strmap_get(panel_type_map, type_name);
    naui_intmap_put(panel_map, uuid, panel);
    if (panel->events.on_attach)
        panel->events.on_attach(panel->uid, (void*)panel->_stack);
    return uuid;
}

void naui_detach_panel(Naui_PanelUID uid)
{
    Naui_Panel *panel = naui_get_panel(uid);
    if (panel->events.on_detach)
        panel->events.on_detach(panel->uid, (void*)panel->_stack);
    naui_intmap_del(panel_map, uid);
    free(panel);
}

Naui_Panel *naui_get_panel(Naui_PanelUID uid)
{
    return naui_intmap_get(panel_map, uid);
}