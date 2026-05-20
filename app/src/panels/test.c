#include <naui.h>

typedef struct
{
    float time;
}
TestData;

static void on_attach(Naui_PanelID panel_id, TestData *data)
{
    naui_panel_set_title(panel_id, "Panel");
}

static void on_detach(Naui_PanelID panel_id, TestData *data)
{
    
}

static void on_update(Naui_PanelID panel_id, TestData *data)
{
    data->time = naui_time();
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf_text("Hello World! :3", {
            .color = leaf_rgb(255, 0, 0),
            .font_size = LEAF_SIZE_GROW,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test);