#include "defaults/titlebar.h"
#include "defaults/widgets/widgets.h"
#include <naui/naui.h>

NAUI_APP("Naui Sandbox")

void naui_app_start(void)
{
	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
	naui_set_main_viewport(NAUI_ATTACH_PANEL(welcome));

    const int six_seven = 67;
    naui_log(NAUI_LOG_DEBUG, "secret debug message :) guess what... %d", six_seven);
    naui_log(NAUI_LOG_INFO, "hello everybody my name is markiplier");
    naui_log(NAUI_LOG_WARNING, "im thinking miku");
    naui_log(NAUI_LOG_ERROR, "teto teto teeeto");

    // this aborts the program...
    // naui_log(NAUI_LOG_FUCKED, "hello everybody my name is markiplier");
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
