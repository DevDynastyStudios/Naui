#include "renderer.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_log.h>

typedef struct
{
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
}
Naui_RendererState;
static Naui_RendererState state;

void naui_renderer_initialize(void)
{
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func
    });
}

void naui_renderer_shutdown(void)
{
    sg_shutdown();
}

void naui_renderer_resize(int32_t width, int32_t height)
{
    
}

void naui_renderer_begin(void)
{
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 3, 1);
}

void naui_renderer_end(void)
{
    sg_end_pass();
    sg_commit();
}