#include <naui/naui.h>

NAUI_APP("Hello World")

void naui_app_start(void)
{
    
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{
    leaf({
        .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_GROW},
        .color = leaf_solid(LEAF_COLOR_WHITE),
        .padding = LEAF_PADDING_ALL(25.0f),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_GROW},
            .color = leaf_solid(leaf_rgb(200, 0, 200)),
            .rounding = 16.0f
        });
    }
}
