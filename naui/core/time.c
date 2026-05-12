#include "time.h"
#include <magma/mgapp.h>

float naui_delta_time(void)
{
    return (float)mg_app_delta_time();
}