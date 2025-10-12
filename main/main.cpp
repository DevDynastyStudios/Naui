#include <naui/application.h>
#include <naui/panel_manager.h>

struct TestPanelData
{
    int counter = 0;
};

static void complex_panel_render(NauiPanelInstance &panel)
{
    TestPanelData *data = (TestPanelData*)panel.data;
    ImGui::Text("Counter = %d", data->counter);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
    if (ImGui::Button("Add")) data->counter++;
    ImGui::PopStyleVar();
    if (ImGui::Button("Add2")) data->counter++;

}

void naui_app_initialize(void)
{
    naui_register_panel_layer("basic", nullptr, [](NauiPanelInstance &panel) {
        if (ImGui::Button("Hello World!"))
            ImGui::OpenPopup("Test4");
        
        if (ImGui::BeginPopupModal("Test4", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            if (ImGui::Button("Hello World!"))
            {
                naui_get_first_panel_of_layer("complex").is_open = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    });

    naui_register_panel_layer<TestPanelData>("complex",
    [](NauiPanelInstance &panel)
    {
        TestPanelData *data = (TestPanelData*)panel.data;
        panel.panel_flags = NauiPanelFlags_ClosedByDefault;
    }, complex_panel_render);

    naui_create_panel("basic", "Test");
    naui_create_panel("basic", "Test2");
    naui_create_panel("complex", "Test3");
}

void naui_app_shutdown(void)
{

}

NAUI_APP_DEFINE_ENTRY
(
    naui_app_initialize,
    naui_app_shutdown
);