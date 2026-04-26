#include "app.h"

#include <leaf/leaf.h>
#include <sokol/sokol_app.h>

typedef struct
{
    struct
    {
        Naui_AppEvent start;
        Naui_AppEvent end;
        Naui_AppEvent update;
    }
    events;
}
Naui_AppState;
static Naui_AppState state;

static void __naui_app_event(const sapp_event* e)
{
    if (e->type == SAPP_EVENTTYPE_RESIZED)
    {
        extern void naui_renderer_resize(int32_t width, int32_t height);
        naui_renderer_resize(e->window_width, e->window_height);
    }
}

static void __naui_app_start(void)
{
    extern void naui_renderer_initialize(void);
    naui_renderer_initialize();
    leaf_initialize();
    state.events.start();
}

static void __naui_app_end(void)
{
    extern void naui_renderer_shutdown(void);
    state.events.end();
    leaf_shutdown();
    naui_renderer_shutdown();
}

static void __naui_app_update(void)
{
    leaf_begin_frame(sapp_width(), sapp_height());
    state.events.update();
    Leaf_RenderCmdList cmd_list = leaf_end_frame();
    (void)cmd_list;
}

void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update
)
{
    state.events.start = start;
    state.events.end = end;
    state.events.update = update;

    sapp_run(&(sapp_desc){
        .init_cb = __naui_app_start,
        .frame_cb = __naui_app_update,
        .cleanup_cb = __naui_app_end,
        .event_cb = __naui_app_event,
        .window_title = title,
    });
}