#include "time.h"
#include <sokol/sokol_app.h>

float naui_delta_time(void)
{
    return (float)sapp_frame_duration();
}