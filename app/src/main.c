#include "naui/math/math.h"
#include "naui/utils/list.h"

#include <stdio.h>
#include <naui/naui.h>
#include <naui/renderer/asset_manager.h>
#include <magma/mgapp.h>

NAUI_APP("Sandbox")

void naui_app_start(void)
{
	NAUI_ATTACH_PANEL(test);
	NAUI_ATTACH_PANEL(test);
	NAUI_ATTACH_PANEL(test);
}

void naui_app_end(void)
{

}

void naui_app_update(void)
{

}
