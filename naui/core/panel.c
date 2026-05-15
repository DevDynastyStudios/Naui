#include "panel.h"
#include "utils/map.h"
#include "utils/uuid.h"

#include <leaf/leaf.h>

static Naui_Map(uint64_t, Naui_Panel) panel_map = NULL;

void naui_panel_manager_render(void)
{
    for (int32_t i = 0; i < naui_intmap_len(panel_map); i++)
    {
        Naui_Panel *panel = &panel_map[i].value;
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
            .sizing = {LEAF_SIZING_FIXED(panel->size.x), LEAF_SIZING_FIXED(panel->size.y)},
            .floating_offset = {panel->position.x, panel->position.y},
            .color = leaf_solid(LEAF_COLOR_WHITE),
            .padding = LEAF_PADDING_ALL(8.0f),
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        })
        {
            if (panel->on_update)
                panel->on_update((void*)panel->_stack);
        }
    }
}

void naui_attach_panel(void)
{
    uint64_t uuid = naui_uuid_generate();
    naui_intmap_put(panel_map, uuid, ((Naui_Panel){
        .position = {100, 100},
        .size = {500, 300}
    }));
}

Naui_Panel *naui_get_panel(uint64_t uid)
{
    return &naui_intmap_get(panel_map, (int)uid);
}