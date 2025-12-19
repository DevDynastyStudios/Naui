#include "MenuBar.h"
#include "Layout.h"
#include "Panel.h"
#include <imgui.h>

namespace Naui {

MenuBar::MenuBar()
{
    newLayoutName[0] = '\0';
    openSaveAsPopup = false;
    popupFocusRequest = false;
    currentLayout = "Layout";
}

void MenuBar::LayoutMenu()
{
    for (const LayoutInfo& info : Layout::Cache()) {
        bool selected = (info.filename == currentLayout);
        if (ImGui::MenuItem(info.filename.c_str(), nullptr, selected)) {
            currentLayout = info.filename;
            Layout::LoadDeferred(info.filename);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Save Layout As...")) {
        newLayoutName[0] = '\0';
        openSaveAsPopup = true;
        popupFocusRequest = true;
    }

    if (ImGui::BeginMenu("Delete Layout", !Layout::Cache().empty())) {
        for (const LayoutInfo& info : Layout::Cache()) {
            if (info.immutable)
                continue;

            if (ImGui::MenuItem(info.filename.c_str())) {
                Layout::Delete(info.filename);
                if (currentLayout == info.filename)
                    currentLayout = "Layout";
            }
        }
		
        ImGui::EndMenu();
    }
}

void MenuBar::LayoutPopup()
{
    if (openSaveAsPopup) {
        ImGui::OpenPopup("Save Layout As");
        openSaveAsPopup = false;
    }

    if (ImGui::BeginPopup("Save Layout As", ImGuiWindowFlags_AlwaysAutoResize)) {

        if (popupFocusRequest) {
            ImGui::SetKeyboardFocusHere();
            popupFocusRequest = false;
        }

        ImGui::InputText("Layout Name", newLayoutName, IM_ARRAYSIZE(newLayoutName));

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            Layout::Save(newLayoutName);
            currentLayout = newLayoutName;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();

        if (ImGui::Button("Save")) {
            Layout::Save(newLayoutName);
            currentLayout = newLayoutName;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void MenuBar::RenderMenuBar()
{
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit"))
            exit(0);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        for (auto& [id, panelPtr] : GetAllPanels()) {
            Panel& panel = *panelPtr;
            if (ImGui::MenuItem(panel.GetTitle().c_str(), nullptr, panel.IsOpen()))
                panel.SetOpen(!panel.IsOpen());
        }
			
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Layout")) {
        LayoutMenu();
        ImGui::EndMenu();
    }

    LayoutPopup();
}

}