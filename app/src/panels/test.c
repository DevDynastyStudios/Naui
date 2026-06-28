#include <naui.h>
#include <stdio.h>

typedef struct
{
    float time;
    char buff[32];
}
TestData;

Naui_PanelID child_panel;
Naui_PanelID panel_slot;

static void on_attach(Naui_PanelID panel_id, TestData *data)
{
    naui_panel_set_title(panel_id, "Panel");
    // panel_id and child_panel are now sibling panels docked horizonatly next to eachother

    //Naui_PanelID child_panel_2 = NAUI_ATTACH_PANEL(test2);
    //naui_dock_panel(panel_slot, child_panel_2, NAUI_DOCK_DIRECTION_TOP);
    panel_slot = panel_id;
}

static void on_detach(Naui_PanelID panel_id, TestData *data)
{
    
}

static void on_update(Naui_PanelID panel_id, TestData *data)
{
    data->time = naui_time();
    if (naui_key_pressed(NAUI_KEY_W))
    {
        child_panel = NAUI_ATTACH_PANEL(test2);
        //naui_dock_panel(panel_slot, child_panel, NAUI_DOCK_DIRECTION_CENTER, 0.5f);
    }
    if (naui_key_pressed(NAUI_KEY_S))
    {
        if (panel_slot)
        {
            naui_undock_panel(child_panel);
            panel_slot = panel_id;
        }
    }
}

static void on_render(Naui_PanelID panel_id, TestData *data)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(64.0f), LEAF_SIZE_DERIVED},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .color = leaf_gradient_percent(leaf_hex(0x353535), 0.0f, leaf_hex(0x181818), 1.0f, leaf_deg(90.0f)),
            .aspect_ratio = 1.0f,
            .border = {
                .width = 4.0f,
                .color = leaf_gradient_percent(leaf_hex(0x5C5C5C), 0.0f, leaf_hex(0x171717), 0.59f, leaf_deg(90.0f))
            },
            .rounding = {INT_MAX, LEAF_CORNER_ALL}
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test, TestData);