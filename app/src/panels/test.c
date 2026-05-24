#include <naui.h>
#include <stdio.h>

typedef struct
{
    float time;
    char buff[32];
}
TestData;

static void on_attach(Naui_PanelID panel_id, TestData *data)
{
    naui_panel_set_title(panel_id, "Panel");
    Naui_PanelID child_panel = NAUI_ATTACH_PANEL(test2);
    //Naui_PanelID panel_slot  = naui_dock_panel(panel_id, child_panel, NAUI_DOCK_DIRECTION_RIGHT);
    Naui_PanelID child_panel_2 = NAUI_ATTACH_PANEL(test2);
    //naui_dock_panel(panel_slot, child_panel_2, NAUI_DOCK_DIRECTION_TOP);
    for (uint32_t i = 0; i < 36; i ++)
    NAUI_ATTACH_PANEL(test2);
}

static void on_detach(Naui_PanelID panel_id, TestData *data)
{
    
}

static void on_update(Naui_PanelID panel_id, TestData *data)
{
    data->time = naui_time();
    //printf("A");
}

static void on_render(Naui_PanelID panel_id, TestData *data)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {

        leaf_text("Tung Tung Tung", {
            .color = LEAF_COLOR_WHITE,
            .font_size = LEAF_SIZE_GROW,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test);