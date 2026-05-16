#include <naui/naui.h>
#include <naui/renderer/asset_manager.h>

NAUI_APP("test")

void naui_app_start(void)
{
    
}

void naui_app_end(void)
{

}
static float font_size = 128.0f;
void naui_app_update(void)
{
    leaf_debug(true, 400, naui_delta_time(), 0)
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
                .image = naui_get_image("dude"),
                .padding = LEAF_PADDING_ALL(25.0f),
                .rounding = naui_time() * 10.0f,
                .border = {leaf_gradient(leaf_rgb(0, 0, 255), leaf_rgb(0, 255, 0), leaf_deg(0)), 8.0f},
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                leaf_text("Hello", {
                    .font_size = font_size,
                    .color = leaf_rgb(0, 0, 0),
                    .alignment = LEAF_TEXT_ALIGN_CENTER
                });
            }
            leaf({
                .sizing = {LEAF_SIZING_GROW, LEAF_SIZING_GROW},
                .color = leaf_solid(leaf_rgb(200, 0, 200)),
                .image = naui_get_image("dude"),
                .padding = LEAF_PADDING_ALL(25.0f),
                .rounding = naui_time() * 10.0f,
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            })
            {
                leaf_text("Hello", {
                    .font_size = font_size,
                    .color = leaf_rgb(0, 0, 0),
                    .alignment = LEAF_TEXT_ALIGN_CENTER,
                    .wrap_mode = LEAF_TEXT_WRAP_MODE_CHAR
                });
            }
        }
    }

}
