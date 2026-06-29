#include <naui.h>
#include <stdio.h>

typedef struct
{
    float time;
    char buff[32];
}
TestData;

static void on_attach(TestData *data)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Panel");
    naui_panel_enable_flags(this, NAUI_PANEL_FLAG_SERIALIZABLE);
}

static void on_detach(TestData *data)
{
    
}

static void on_update(TestData *data)
{

}

static void on_render(TestData *data)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(64.0f), LEAF_SIZE_DERIVED},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .color = LEAF_GRADIENT_PERCENT(leaf_hex(0x353535), 0.0f, leaf_hex(0x181818), 1.0f, leaf_deg(90.0f)),
            .aspect_ratio = 1.0f,
            .border = {
                .width = 4.0f,
                .color = LEAF_GRADIENT_PERCENT(leaf_hex(0x5C5C5C), 0.0f, leaf_hex(0x171717), 0.59f, leaf_deg(90.0f))
            },
            .rounding = {INT_MAX, LEAF_CORNER_ALL}
        });
    }
}

NAUI_DEFINE_PANEL_TYPE(test, TestData);