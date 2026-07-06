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
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
    })
    {
        leaf({
            .size = {LEAF_SIZE_FIXED(256), LEAF_SIZE_FIXED(256)},
            .color = LEAF_COLOR_WHITE,
            .rounding = {
                .value = fabsf(sinf(naui_time())) * 128.0f,
                .corners = LEAF_CORNER_ALL
            },
            .shadow = {
                .blur_radius = 24.0f,
                .offset = {0.0f, 12.0f},
                .color = leaf_rgba(0, 0, 0, 64)
            }
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