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
    for (uint32_t i = 0; i < 100; i++)
    child_panel = NAUI_ATTACH_PANEL(test2);
    // panel_id and child_panel are now sibling panels docked horizonatly next to eachother

    //Naui_PanelID child_panel_2 = NAUI_ATTACH_PANEL(test2);
    //naui_dock_panel(panel_slot, child_panel_2, NAUI_DOCK_DIRECTION_TOP);
}

static void on_detach(Naui_PanelID panel_id, TestData *data)
{
    
}

static void on_update(Naui_PanelID panel_id, TestData *data)
{
    data->time = naui_time();
    if (naui_key_pressed(NAUI_KEY_W))
    {
        if (!panel_slot)
        panel_slot = naui_dock_panel(panel_id, child_panel, NAUI_DOCK_DIRECTION_LEFT, 0.5f);

    }
    if (naui_key_pressed(NAUI_KEY_S))
    {
        if (panel_slot)
        {
            naui_undock_panel(child_panel);
            panel_slot = NULL;
        }
    }
}

static void on_render(Naui_PanelID panel_id, TestData *data)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {

        leaf_text("ahguprephauger", {
            .color = LEAF_COLOR_WHITE,
            .font_size = LEAF_SIZE_GROW,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test, TestData);