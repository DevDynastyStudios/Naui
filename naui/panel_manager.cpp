#include "panel_manager.h"

#include <ds/free_list.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

#include "default_panels/menu_bar.h"

struct NauiPanelLayerRegistry
{
    size_t size;
    NauiPanelFn create;
    NauiPanelFn render;
};

static std::unordered_map<std::string, NauiPanelLayerRegistry> panel_layers;

static NauiPanelInstance panels[NAUI_MAX_PANELS];
static uint32_t destroyed_panels[NAUI_MAX_PANELS];

static uint32_t panel_count = 0;
static uint32_t destroyed_panel_count = 0;

static NauiFreeList panel_data_pool;
static MenubarFunction menubar_render_func;

static void naui_manage_destroyed_panels(void)
{
    if (destroyed_panel_count > 0)
    {
        for (uint32_t i = 0; i < destroyed_panel_count; ++i)
        {
            uint32_t panel_index = destroyed_panels[i];
            NauiPanelInstance &panel = panels[panel_index];
            if (panel.data)
                naui_free_list_free(panel_data_pool, panel.data, panel_layers[panel.layer].size);
            for (uint32_t i = panel_index; i < panel_count - 1; ++i)
                panels[i] = panels[i + 1];
            --panel_count;
        }
        destroyed_panel_count = 0;
    }
}

void naui_panel_manager_initialize(void)
{
    naui_create_free_list(panel_data_pool, NAUI_MAX_PANEL_HEAP_SIZE);
	menubar_render_func = naui_render_menu_bar_default;
}

void naui_panel_manager_shutdown(void)
{
    naui_destroy_free_list(panel_data_pool);
}

void naui_panel_manager_render(void)
{
	#if NAUI_DISABLE_DEFAULT_MENUBAR == 0
    menubar_render_func();
	#endif

    for (uint32_t i = 0; i < panel_count; ++i)
    {
        NauiPanelInstance &panel = panels[i];
        if (!panel.is_open)
            continue;

        ImGui::PushID(panel.layer);
        ImGui::SetNextWindowDockID(panel.dock_id, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(panel.default_size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(panel.min_size, panel.max_size);
        bool *is_open = (panel.panel_flags & NauiPanelFlags_NoClose) ? nullptr : &panel.is_open;
        if (ImGui::Begin(panel.title, is_open, panel.window_flags) && panel.render)
            panel.render(panel);

        ImGui::End();
        ImGui::PopID();
    }

    naui_manage_destroyed_panels();
}

void naui_panel_set_menubar(MenubarFunction func)
{
	if(func == nullptr)
		return;

	menubar_render_func = func;
}

void naui_register_panel_layer(const char *layer, NauiPanelFn create, NauiPanelFn render, size_t data_size)
{
    panel_layers[layer] = 
	{
        .size = data_size,
        .create = create,
        .render = render
    };
}

NauiPanelInstance &naui_create_panel(const char *layer, const char *title)
{
    const NauiPanelLayerRegistry &panel_layer = panel_layers[layer];
    NauiPanelInstance panel = 
	{
        .title = title,
        .layer = layer,
        .create = panel_layer.create,
        .render = panel_layer.render
    };

    if (panel_layer.size != 0)
    {
        panel.data = (void*)naui_free_list_alloc(panel_data_pool, panel_layer.size);
        memset(panel.data, 0, panel_layer.size);
    }

    if (panel_layer.create)
        panel_layer.create(panel);

    panel.is_open = !(panel.panel_flags & NauiPanelFlags_ClosedByDefault);

    NauiPanelInstance &result = panels[panel_count++] = panel;
    return result;
}

uint32_t naui_get_panel_count(void)
{
    return panel_count;
}

NauiPanelInstance& naui_get_panel(uint32_t index) 
{
    assert(index < panel_count && "Panel index out of range");
    return panels[index];
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

void naui_destroy_panel(NauiPanelInstance &panel)
{
    const uint32_t panel_index = (uint32_t)(&panel - panels);
    destroyed_panels[destroyed_panel_count++] = panel_index;
}
