#include "App.h"
#include "Panel.h"
#include "Theme.h"
#include "Layout.h"

namespace Naui
{

class PanelRenderer
{
private:
    static void Render(void);
    static void RenderMenuBar(void);
    friend class App;
};

void PanelRenderer::Render(void)
{
    auto &panels = GetAllPanels();
    for (const auto &[id, panel_ptr] : panels)
    {
        Naui::Panel& panel = *panel_ptr;
        ImGui::SetNextWindowSizeConstraints(panel.m_minSize, panel.m_maxSize);
        static char title[1024];
        snprintf(title, sizeof(title), "%s##%d", panel.m_title.c_str(), id);
        ImGui::Begin(title, panel.m_closable ? &panel.m_open : nullptr, panel.m_imguiFlags);
        panel.OnRender();
        ImGui::End();
    }
}

static void ImGuiInitialize(void)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.IniFilename = nullptr;

	ImGuiStyle &style = ImGui::GetStyle();
	style.FramePadding = ImVec2(8.0f, 8.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.FrameRounding = 6.0f;
	style.GrabRounding = 2.0f;

	ImFontConfig config{};
	config.RasterizerMultiply = 2.0f;
	io.Fonts->AddFontFromFileTTF("Fonts/Nunito.ttf", 18.0f, &config);
}

static void ImGuiShutdown(void)
{
    ImGui::DestroyContext();
}

void App::Render(void)
{
    m_window->PollEvents();
    m_renderer->Begin();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Exit"))
            m_window->Close();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
    PanelRenderer::Render();
    ImGui::EndFrame();
    ImGui::Render();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    m_renderer->End();
}

void App::Run(void)
{
    ImGuiInitialize();
    Theme::LoadDefault();

    m_window = CreatePlatformWindow(1280, 720, "Naui");
    m_renderer = CreateRenderer(*m_window);
    m_window->Show(true);
    m_window->SetResizeEvent([&](uint32_t width, uint32_t height)
    {
        m_renderer->Resize(width, height);
        Render();
    });
    OnEnter();
    while (m_window->IsOpen())
        Render();
    OnExit();
    DestroyAllPanels();
    delete m_renderer;
    delete m_window;
    ImGuiShutdown();
}

}