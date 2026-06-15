#include <naui.h>

typedef struct
{
    float time;
}
TestData;

#include <stdio.h>

static void on_attach(Naui_PanelID panel_id, TestData *data)
{
    naui_panel_set_title(panel_id, "grguahergaefg");
    naui_panel_set_size(panel_id, (Naui_Vec2){ 256, 256 });
}

static void on_detach(Naui_PanelID panel_id, TestData *data)
{
    
}

static void on_update(Naui_PanelID panel_id, TestData *data)
{
    data->time = naui_time();
}

static void on_render(Naui_PanelID panel_id, TestData *data)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP}
    })
    {

        leaf_text("Yipeee :3", {
            .color = LEAF_COLOR_WHITE,
            .font_size = 24.0f,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test2, TestData);