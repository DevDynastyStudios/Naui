#include "renderer.h"

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_log.h>

#include "shaders/basic.h"

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

    float vertices[] = {
         0.0f,  0.5f,     1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,     0.0f, 0.0f, 1.0f,
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
    });

    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(basic_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [ATTR_basic_in_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_basic_in_color].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
    });

    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={1.0f, 0.5f, 0.5f, 1.0f } }
    };
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