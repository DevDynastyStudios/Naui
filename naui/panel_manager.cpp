#include "panel_manager.h"

#include <ds/arena.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

struct NauiPanelLayerRegistry
{
    size_t size;
    NauiPanelFn create;
    NauiPanelFn render;
};

static std::unordered_map<std::string, NauiPanelLayerRegistry> panel_layers;

static NauiPanelInstance panels[NAUI_MAX_PANELS];
static uint32_t panel_count = 0;

static NauiArena arena;

void naui_panel_manager_initialize(void)
{
    naui_create_arena(arena, NAUI_MAX_PANEL_SCRATCH_SIZE);
}

void naui_panel_manager_shutdown(void)
{
    naui_destroy_arena(arena);
}

void naui_panel_manager_render(void)
{
    for (uint32_t i = 0; i < panel_count; ++i)
    {
        NauiPanelInstance &panel = panels[i];
        if (!panel.is_open)
            continue;
        ImGui::PushID(panel.layer);
        ImGui::SetNextWindowSize(panel.default_size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(panel.min_size, panel.max_size);
        if (ImGui::Begin(panel.title, &panel.is_open, panel.window_flags) && panel.render)
            panel.render(panel);
        ImGui::End();
        ImGui::PopID();
    }
}

void naui_register_panel_layer(const char *layer, NauiPanelFn create, NauiPanelFn render, size_t data_size)
{
    panel_layers[layer] = {
        .size = data_size,
        .create = create,
        .render = render
    };
}

NauiPanelInstance &naui_create_panel(const char *layer, const char *title)
{
    const NauiPanelLayerRegistry &panel_layer = panel_layers[layer];
    NauiPanelInstance panel = {
        .title = title,
        .layer = layer,
        .create = panel_layer.create,
        .render = panel_layer.render
    };
    if (panel_layer.size != 0)
    {
        panel.data = (void*)naui_arena_alloc(arena, panel_layer.size);
        memset(panel.data, 0, panel_layer.size);
    }

    if (panel_layer.create)
        panel_layer.create(panel);

    panel.is_open = (panel.panel_flags & NauiWindowFlags_ClosedByDefault) == 0;
    NauiPanelInstance &result = panels[panel_count++] = panel;
    return result;
}

NauiPanelInstance &naui_get_first_panel_of_layer(const char *layer)
{
    for (NauiPanelInstance &panel : panels)
        if (strcmp(panel.layer, layer) == 0)
            return panel;
    return panels[0];
}

std::vector<NauiPanelInstance*> &naui_get_all_panels_of_layer(const char *layer)
{
    static std::vector<NauiPanelInstance*> result;
    result.clear();
    for (NauiPanelInstance &panel : panels)
        if (strcmp(panel.layer, layer) == 0)
            result.push_back(&panel);
    return result;
}