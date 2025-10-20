#include "menu_bar.h"
#include "../io/layout_loader.h"
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
            std::string filename = std::string(LAYOUT_FOLDER_PATH) + new_layout_name + INI_EXTENSION;
            naui_save_layout(filename.c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) 
		{
            ImGui::CloseCurrentPopup();
        }

	    if (ImGui::Button("Save")) 
		{
	        std::string filename = std::string(LAYOUT_FOLDER_PATH) + new_layout_name + INI_EXTENSION;
	        naui_save_layout(filename.c_str());
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
	std::filesystem::path layoutPath = "";
	std::vector<std::string> layouts = naui_layout_list(layoutPath);

    for (auto& layout : layouts)
    {
        bool selected = (layout == current_layout);
        if (ImGui::MenuItem(layout.c_str(), nullptr, selected))
        {
            current_layout = layout;
			//Defer layout load here
            //uph_layout_request_load(("layouts/" + layout).c_str());
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Save Layout As..."))
    {
        new_layout_name[0] = '\0';
        open_save_as_popup = true;
        popup_focus_request = true;
    }

    if (ImGui::BeginMenu("Delete Layout", layouts.size() > 0))
    {
    	if (!initialized)
    	{
    	    std::vector<std::string> all_layouts = naui_layout_list(LAYOUT_FOLDER_PATH);

    	    for (const std::string& full_path : all_layouts)
    	    {
    	        NauiIniData data;
    	        if (!naui_read_ini(full_path, data))
    	            continue;

    	        if (naui_layout_is_immutable(&data))
    	            continue;

    	        // Store just the filename for display
    	        deletable_layouts.push_back(std::filesystem::path(full_path).filename().string());
    	    }

    	    initialized = true;
    	}

    	for (const std::string& layout : deletable_layouts)
    	{
    	    std::string full_path = (std::filesystem::path(LAYOUT_FOLDER_PATH) / layout).string();

    	    if (ImGui::MenuItem(layout.c_str()))
    	    {
    	        naui_delete_layout(full_path.c_str());

    	        if (current_layout == layout)
    	            current_layout = "Layout";
    	    }
    	}

    	ImGui::EndMenu();
    } else
		initialized = false;
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