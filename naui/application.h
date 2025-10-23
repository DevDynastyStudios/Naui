#pragma once

#include "platform/platform.h"
#include <cstdint>

NAUI_API void naui_app_run(const NauiWindowProps &props, void (*on_initialize)(ImGuiID main_dock_id), void (*on_shutdown)(void), int32_t argc, char* const* argv);

#define NAUI_APP_DEFINE_ENTRY(props, on_initialize, on_shutdown)\
int32_t main(int32_t argc, char* const* argv)\
{\
    naui_app_run(props, on_initialize, on_shutdown, argc, argv);\
    return 0;\
}