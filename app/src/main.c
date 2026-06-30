#include <naui/naui.h>
#include <widgets/widgets.h>
#include <widgets/titlebar.h>

NAUI_APP("Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	NAUI_ATTACH_PANEL(test);
	NAUI_ATTACH_PANEL(test2);
	NAUI_ATTACH_PANEL(test3);
	naui_deserialize_viewport("Default_Layout.json");
}

void naui_app_end(void)
{
	naui_serialize_viewport("Default_Layout.json");
}

void naui_app_update(void)
{
	naui_render_main_titlebar("Naui Editor");
	naui_render_panels_and_viewport();
}
