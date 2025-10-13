#pragma once

#include "base.h"
#include <cstdint>

NAUI_API void naui_app_run(void (*on_initialize)(void), void (*on_shutdown)(void), int32_t argc, char* const* argv);

#define NAUI_APP_DEFINE_ENTRY(on_initialize, on_shutdown)\
int32_t main(int32_t argc, char* const* argv)\
{\
    naui_app_run(on_initialize, on_shutdown, argc, argv);\
    return 0;\
}