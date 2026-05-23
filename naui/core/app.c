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

extern void naui_renderer_initialize(void);
extern void naui_renderer_shutdown(void);
extern void naui_renderer_resize(int32_t width, int32_t height);
extern void naui_renderer_begin(void);
extern void naui_renderer_end(void);

extern void naui_panel_manager_render(void);
extern void naui_input_update(void);

static Leaf_Dimensions measure_text_bridge(const char *text, uint32_t length, float resolved_font_size, const Leaf_TextConfig *config)
{
    Naui_Vec2 size = naui_measure_text(text, length, resolved_font_size, config->font_id);
    return (Leaf_Dimensions){size.x, size.y};
}

static void render_leaf_cmd_list(const Leaf_RenderCmdList *list)
{
    for (uint32_t i = 0; i < list->count; i++)
    {
        Leaf_RenderCmd cmd = list->cmds[i];
        switch (cmd.type)
        {
        case LEAF_RENDER_CMD_RECT:
        {
            Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
            Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
            Naui_Color color1 = (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a};
            if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                naui_fill_gradient_rect(position, size, (Naui_Gradient){ color1, (Naui_Color){cmd.color.color2.r, cmd.color.color2.g, cmd.color.color2.b, cmd.color.color2.a}, cmd.color.angle }, cmd.rect.rounding);
            else naui_fill_rect(position, size, color1, cmd.rect.rounding);
            break;
        }
        case LEAF_RENDER_CMD_RECT_LINES:
            Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
            Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
            Naui_Color color1 = (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a};
            if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                naui_draw_gradient_rect(position, size, (Naui_Gradient){ color1, (Naui_Color){cmd.color.color2.r, cmd.color.color2.g, cmd.color.color2.b, cmd.color.color2.a}, cmd.color.angle }, cmd.rect.line_width, cmd.rect.rounding);
            else naui_draw_rect(position, size, color1, cmd.rect.line_width, cmd.rect.rounding);
            break;
        case LEAF_RENDER_CMD_IMAGE:
        {
            Naui_Image *image = (Naui_Image*)cmd.image.handle;
            Naui_Vec2 position = (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y};
            Naui_Vec2 size = (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height};
            Naui_Color color1 = (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a};
            if (cmd.color.type == LEAF_GRADIENT_LINEAR_COLOR_FILL)
                naui_draw_gradient_image(image, position, size, (Naui_Gradient){ color1, (Naui_Color){cmd.color.color2.r, cmd.color.color2.g, cmd.color.color2.b, cmd.color.color2.a}, cmd.color.angle }, cmd.image.rounding);
            else naui_draw_image(image, position, size, color1, cmd.image.rounding);
            break;
        }
        case LEAF_RENDER_CMD_TEXT:
            naui_draw_text((Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y}, cmd.text.text, cmd.text.font_size, cmd.text.font_id, (Naui_Color){cmd.color.color1.r, cmd.color.color1.g, cmd.color.color1.b, cmd.color.color1.a});
            break;
        case LEAF_RENDER_CMD_SCISSOR_PUSH:
            naui_push_clip_rect(
                (Naui_Vec2){cmd.bounding_box.x, cmd.bounding_box.y},
                (Naui_Vec2){cmd.bounding_box.width, cmd.bounding_box.height}
            );
            break;
        case LEAF_RENDER_CMD_SCISSOR_POP:
            naui_pop_clip_rect();
            break;
        case LEAF_RENDER_CMD_CUSTOM:
            cmd.custom.draw(cmd.bounding_box, cmd.custom.user_data);
            break;
        }
    }
}

static void render(void)
{
    naui_renderer_begin();
    leaf_begin_frame(mg_app_width(), mg_app_height());
    leaf_set_pointer_pos((float)mg_app_mouse_x(), (float)mg_app_mouse_y());
    state.events.update();
    naui_panel_manager_render();
    Leaf_RenderCmdList cmd_list = leaf_end_frame();
    render_leaf_cmd_list(&cmd_list);
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
    naui_renderer_initialize();
    naui_set_font(0, "Assets/Fonts/Quicksand-SemiBold.ttf");
    naui_asset_manager_load_images("Assets/Images");
    leaf_initialize();
    leaf_set_measure_text(measure_text_bridge);
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
    naui_input_update();
    render();
}

int32_t naui_app_width(void)
{
    return mg_app_width();
}

int32_t naui_app_height(void)
{
    return mg_app_height();
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
