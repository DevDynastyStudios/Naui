#include "app.h"
#include <stddef.h>

#include <leaf/leaf.h>
#include <magma/mgapp.h>

#include "renderer/asset_manager.h"

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

extern void naui_renderer_initialize(const char *default_font_path);
extern void naui_renderer_shutdown(void);
extern void naui_renderer_resize(int32_t width, int32_t height);
extern void naui_renderer_begin(void);
extern void naui_renderer_end(void);

static void render(void)
{
    naui_renderer_begin();
    leaf_begin_frame(mg_app_width(), mg_app_height());
    state.events.update();
    Leaf_RenderCmdList cmd_list = leaf_end_frame();
    for (uint32_t i = 0; i < cmd_list.count; i++)
    {
        Leaf_RenderCmd cmd = cmd_list.cmds[i];
        switch (cmd.type)
        {
        case LEAF_RENDER_CMD_RECT:
            naui_fill_round_rect(
                (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y},
                (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height},
                (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a},
                cmd.rect.rounding
            );
            break;
        case LEAF_RENDER_CMD_RECT_LINES:
            naui_draw_round_rect(
                (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y},
                (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height},
                (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a},
                cmd.rect.line_width, cmd.rect.rounding
            );
            break;
        }
    }
    const Naui_Image img = naui_get_image("ded");
    naui_draw_image(img, (Naui_Vec2){0}, (Naui_Vec2){500.0f, 500.0f});
    naui_renderer_end();
}

static void __naui_app_event(const mg_app_event* event)
{
    if (event->type == MG_APP_EVENT_RESIZE)
    {
        naui_renderer_resize(event->window_width, event->window_height);
        render();
    }
}

static void __naui_app_start(void)
{
    naui_renderer_initialize(NULL);
    naui_asset_manager_load_images("res");
    leaf_initialize();
    state.events.start();
}

static void __naui_app_end(void)
{
    state.events.end();
    leaf_shutdown();
    naui_renderer_shutdown();
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
