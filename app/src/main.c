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

}
