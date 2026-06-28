#include <naui/naui.h>

NAUI_APP("Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	naui_load_font(1, "NotoSansJP-Regular");
	NAUI_ATTACH_PANEL(test);
}

void naui_app_end(void)
{

}

static void render_main_titlebar(void)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(32.0f)},
		.child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
		.color = {naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG)}
	})
	{
		leaf_text("ありがとございます.png", {
			.font_id = 1,
			.font_size = 20.0f,
			.color = naui_theme_leaf_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG),
			.alignment = LEAF_TEXT_ALIGN_CENTER
		});
	}
}

void naui_app_update(void)
{
	render_main_titlebar();
	naui_render_panels_and_viewport();
}
