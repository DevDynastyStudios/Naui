#pragma once

#include <base.h>
#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef NAUI_MAX_PANEL_SCRATCH_SIZE
    #define NAUI_MAX_PANEL_SCRATCH_SIZE (1 << 16)
#endif

#ifndef NAUI_MAX_PANELS
    #define NAUI_MAX_PANELS (1 << 10)
#endif

typedef uint32_t NauiPanelFlags;
enum
{
    NauiPanelFlags_None = 0,
    NauiPanelFlags_ClosedByDefault = 1 << 0,
};

typedef void (*NauiPanelFn)(struct NauiPanelInstance &panel);
struct NauiPanelInstance
{
    NauiPanelFlags panel_flags = NauiPanelFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

    ImVec2 default_size = ImVec2(300, 300);
    ImVec2 min_size = ImVec2(0, 0);
    ImVec2 max_size = ImVec2(FLT_MAX, FLT_MAX);

    const char *title;
    const char *layer;
    void *data;

    bool is_open;

    NauiPanelFn create;
    NauiPanelFn render;
};

NAUI_API void naui_panel_manager_initialize(void);
NAUI_API void naui_panel_manager_shutdown(void);
NAUI_API void naui_panel_manager_render(void);

NAUI_API void naui_register_panel_layer(const char *layer, NauiPanelFn create = nullptr, NauiPanelFn render = nullptr, size_t data_size = 0);

template<typename T>
void naui_register_panel_layer(const char *layer, NauiPanelFn create = nullptr, NauiPanelFn render = nullptr)
{
    naui_register_panel_layer(layer, create, render, sizeof(T));
}

NAUI_API NauiPanelInstance &naui_create_panel(const char *layer, const char *title);
NAUI_API NauiPanelInstance &naui_get_first_panel_of_layer(const char *layer);
NAUI_API std::vector<NauiPanelInstance*> &naui_get_all_panels_of_layer(const char *layer);