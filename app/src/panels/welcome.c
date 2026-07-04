#include <naui.h>
#include "defaults/widgets/widgets.h"
#include "defaults/widgets/tooltip.h"

#include <stdio.h>

static void on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Welcome");
    naui_panel_set_size(this, (Naui_Vec2){ 256, 256 });
    naui_panel_enable_flags(this, NAUI_PANEL_FLAG_SERIALIZABLE);
}

static void on_detach(void)
{
    
}

static void on_update(void)
{
    
}

static void on_render(void)
{
    Naui_Image *image = naui_get_image("logo-large-light");

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(image->width), LEAF_SIZE_FIXED(image->height)},
            .image = image,
            .color = LEAF_COLOR_WHITE
        });

        leaf({
            .id = leaf_id("hi_button")
        })
        naui_button("Hiii :3");

        NAUI_TOOLTIP(leaf_id("hi_button"))
        {
            leaf_text("Hiii :3", {.color = LEAF_COLOR_WHITE, .font_size = 13.5f * naui_app_dpi_scale()});
        }
    }
}

NAUI_DEFINE_PANEL_TYPE_NO_DATA(welcome);