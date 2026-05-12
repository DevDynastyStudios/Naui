#include "app.h"

#include <leaf/leaf.h>
#include <magma/mgapp.h>
#include <magma/mgfx.h>

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

static void render(void)
{
    mgfx_begin();
    leaf_begin_frame(mg_app_width(), mg_app_height());
    state.events.update();
    Leaf_RenderCmdList cmd_list = leaf_end_frame();
    mgfx_end();
    (void)cmd_list;
}

static void __naui_app_event(const mg_app_event* event)
{
    if (event->type == MG_APP_EVENT_RESIZE)
        render();
}

static void __naui_app_start(void)
{
    mgfx_init(&(mgfx_init_info){
        .handle = mg_app_handle(),
        .width = mg_app_width(),
        .height = mg_app_height(),
        .vsync = true
    });
    leaf_initialize();
    state.events.start();
}

static void __naui_app_end(void)
{
    state.events.end();
    leaf_shutdown();
    mgfx_shutdown();
}

static void __naui_app_update(void)
{
    render();
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

    mg_app_run(&(mg_app_init_info){
        .title = title,
        .events = {
            .start = __naui_app_start,
            .end = __naui_app_end,
            .update = __naui_app_update,
            .event = __naui_app_event
        }
    });
}