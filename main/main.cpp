#include <naui/application.h>
#include <naui/panel_manager.h>
#include <naui/io/asset_manager.h>
#include <cstdio>

#include <imgui_internal.h>
static ImGuiID bottom_dock_id;

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

void naui_app_initialize(ImGuiID main_dock_id)
{
    naui_register_panel_layer("wizard",
    [](NauiPanelInstance &panel) {
        panel.window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration;
    },
    [](NauiPanelInstance &panel) {
        NauiImage &logo = naui_get_image("logo-large-white");
        ImGui::Image(logo, ImVec2(logo.width * 0.3f, logo.height * 0.3f));
        
        if (panel.is_open == false)
            naui_destroy_panel(panel);
    });

    naui_register_panel_layer("basic", nullptr, [](NauiPanelInstance &panel) {
        if (ImGui::Button("Hello World!"))
            ImGui::OpenPopup("Test4");
        
        if (ImGui::BeginPopupModal("Test4", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
        {
            if (ImGui::Button("Hello World!"))
            {
                //naui_get_first_panel_of_layer("complex").is_open = true;
                naui_create_panel("complex", "Complex");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (panel.is_open == false)
            naui_destroy_panel(panel);
    });

    naui_register_panel_layer<TestPanelData>("complex",
    [](NauiPanelInstance &panel)
    {
        TestPanelData *data = (TestPanelData*)panel.data;
        panel.panel_flags = NauiPanelFlags_NoClose;
        panel.window_flags = ImGuiWindowFlags_NoCollapse;
    }, complex_panel_render);

    naui_create_panel("wizard", "Welcome");
    naui_create_panel("basic", "Test");
    naui_create_panel("basic", "Test2");
    naui_create_panel("complex", "Test3");
}

void naui_app_shutdown(void)
{

}

static NauiWindowProps window_props{};
NAUI_APP_DEFINE_ENTRY
(
    window_props,
    naui_app_initialize,
    naui_app_shutdown
);