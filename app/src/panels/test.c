#include <naui.h>

typedef struct
{
    float time;
    char buff[32];
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

        leaf_text(itoa(panel_id, data->buff, 10), {
            .color = LEAF_COLOR_WHITE,
            .font_size = LEAF_SIZE_GROW,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test);