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

static void naui_app_event(const sapp_event* e)
{
    if (e->type == SAPP_EVENTTYPE_RESIZED)
    {
        int w = e->window_width;
        int h = e->window_height;
    }
}

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
        .event_cb = naui_app_event,
        .window_title = title,
    });
}