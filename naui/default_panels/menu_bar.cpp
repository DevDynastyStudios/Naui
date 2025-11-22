#include "menu_bar.h"
#include "../io/layout_loader.h"
#include "../util/defer.h"

#include <filesystem>
#include <string>
#include <vector>
#include <imgui.h>

static std::string current_layout;
static char new_layout_name[64];
static bool open_save_as_popup = false;
static bool popup_focus_request = false;
static bool initialized = false;
static std::vector<std::string> deletable_layouts;

static void naui_menu_bar_popup()
{
	if (open_save_as_popup) 
	{
	    ImGui::OpenPopup("Save Layout As");
	    open_save_as_popup = false; // reset trigger
	}

	if (ImGui::BeginPopup("Save Layout As", ImGuiWindowFlags_AlwaysAutoResize)) 
	{
	    if (popup_focus_request) {
	        ImGui::SetKeyboardFocusHere();
	        popup_focus_request = false;
	    }

	    ImGui::InputText("Layout Name", new_layout_name, IM_ARRAYSIZE(new_layout_name));
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) 
		{
			naui_save_layout(new_layout_name);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) 
		{
            ImGui::CloseCurrentPopup();
        }

	    if (ImGui::Button("Save")) 
		{
			naui_save_layout(new_layout_name);
	        ImGui::CloseCurrentPopup();
	    }
	    ImGui::SameLine();
	    if (ImGui::Button("Cancel")) 
		{
	        ImGui::CloseCurrentPopup();
	    }

	    ImGui::EndPopup();
	}
}

static void naui_menu_bar_layouts_menu()
{
	for(const NauiLayoutInfo& info : layout_cache)
	{
		bool selected = (info.filename == current_layout);
		if(ImGui::MenuItem(info.filename.c_str(), nullptr, selected))
		{
			current_layout = info.filename;
			naui_load_layout_deferred(info.filename);
		}
		
	}

    ImGui::Separator();

    if (ImGui::MenuItem("Save Layout As..."))
    {
        new_layout_name[0] = '\0';
        open_save_as_popup = true;
        popup_focus_request = true;
    }

	if (ImGui::BeginMenu("Delete Layout", !layout_cache.empty()))
	{
	    for (const NauiLayoutInfo& info : layout_cache)
	    {
			if(info.immutable)
				continue;

			if (ImGui::MenuItem(info.filename.c_str()))
	        {
	            naui_delete_layout(info.filename);
	            if (current_layout == info.filename)
	                current_layout = "Layout";
	        }
	    }
		
	    ImGui::EndMenu();
	}
}

void naui_render_menu_bar_default()
{
	if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Files"))
        {
            if (ImGui::MenuItem("Exit"))
				exit(0);

            ImGui::EndMenu();
        }

		if(ImGui::BeginMenu("View"))
		{
			
			for(uint32_t i = 0; i < naui_get_panel_count(); ++i)
			{
				NauiPanelInstance& panel = naui_get_panel(i);

				if(ImGui::MenuItem(panel.title, nullptr, panel.is_open))
				{
					panel.is_open = !panel.is_open;
				}
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Layout"))
		{
			naui_menu_bar_layouts_menu();
			ImGui::EndMenu();
		}

    }

    ImGui::EndMainMenuBar();
	naui_menu_bar_popup();
}