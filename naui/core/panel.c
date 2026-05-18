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
                .color = LEAF_COLOR_WHITE,
                .padding = LEAF_PADDING_ALL(8.0f),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                if (panel->events.on_update)
                    panel->events.on_update(panel->uid, (void*)panel->_stack);
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