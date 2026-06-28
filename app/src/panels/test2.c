#include <naui.h>

#include <stdio.h>

static void on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "grguahergaefg");
    naui_panel_set_size(this, (Naui_Vec2){ 256, 256 });
}

static void on_detach(void)
{
    
}

static void on_update(void)
{
    
}

static void on_render(void)
{
    /*leaf({
        .direction = LEAF_DIRECTION_HORIZONAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_TOP},
        .wrap_children = true,
        .child_gap = 4.0f,
        .child_cross_gap = 4.0f
    })
    for (int i = 0; i < 256; i++)
    leaf({
        .size = {LEAF_SIZE_FIXED(64.0f), LEAF_SIZE_DERIVED},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_TOP},
        .color = leaf_rgb(i, i, i),
        .aspect_ratio = 1.0f,
    });*/
}

NAUI_DEFINE_PANEL_TYPE_NO_DATA(test2);