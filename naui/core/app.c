#include "app.h"
#include "base.h"

#if NAUI_WINDOWS
#define SOKOL_D3D11
#elif NAUI_LINUX
#define SOKOL_GLCORE
#endif

#define SOKOL_NO_ENTRY
#define SOKOL_IMPL
#include "sokol/sokol_app.h"

void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update
)
{
    sapp_run(&(sapp_desc){
        .init_cb = start,
        .frame_cb = end,
        .cleanup_cb = update,
        .window_title = title,
    });
}