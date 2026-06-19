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
        .direction = LEAF_DIRECTION_HORIZONAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
        .wrap_children = true,
        .child_gap = 4.0f,
        .child_cross_gap = 4.0f
    })
    for (int i = 0; i < 8; i++)
    leaf({
        .size = {LEAF_SIZE_FIXED(256.0f), LEAF_SIZE_DERIVED},
        .image = { naui_get_image("dude") },
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
        .color = LEAF_COLOR_WHITE,
        .aspect_ratio = 1.0f,
    })
    {

    }
}

NAUI_DEFINE_PANEL_TYPE(test2, TestData);