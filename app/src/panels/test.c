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
    naui_panel_enable_flags(panel_id, NAUI_PANEL_FLAG_NO_RESIZE);
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
        panel_slot = naui_dock_panel(panel_slot, child_panel, rand() % NAUI_DOCK_DIRECTION_CENTER, 0.5f);


    }
    if (naui_key_pressed(NAUI_KEY_S))
    {
        if (panel_slot)
        {
            naui_undock_panel(child_panel);
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
            .size = {LEAF_SIZE_FIXED(32.0f), LEAF_SIZE_DERIVED},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .color = leaf_rgb(255, 0, 0),
            .aspect_ratio = 1.0f
        });

    }
}

NAUI_DEFINE_PANEL_TYPE(test, TestData);