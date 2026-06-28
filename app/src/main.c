#include <naui/naui.h>

NAUI_APP("Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	NAUI_ATTACH_PANEL(test);
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{
	leaf({
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(32.0f)}
	})
	{
		leaf_text("Example Titlebar", {
			.font_size = 20.0f,
			.color = LEAF_COLOR_WHITE
		});
	}
	naui_render_panels_and_viewport();
}
