#include "defaults/titlebar.h"
#include <naui/naui.h>

NAUI_APP("Naui Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	naui_set_main_viewport(NAUI_ATTACH_PANEL(welcome));
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{
	naui_widgets_reset();

	naui_render_main_titlebar("Naui Editor");
	naui_render_panels_and_viewport();
}
