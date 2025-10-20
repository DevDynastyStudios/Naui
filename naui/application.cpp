#include "application.h"

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/event_types.h"

#include "io/theme_loader.h"
#include "io/asset_manager.h"
#include "panel_manager.h"
#include "util/defer.h"

static void naui_render(void)
{
    naui_platform_begin();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    naui_panel_manager_render();
    ImGui::Render();
    naui_platform_end();
}

void naui_app_run(void (*on_initialize)(void), void (*on_shutdown)(void), int32_t argc, char* const* argv)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    ImGuiStyle &style = ImGui::GetStyle();
    style.FramePadding = ImVec2(8.0f, 8.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.FrameRounding = 6.0f;
    style.GrabRounding = 2.0f;

    naui_load_theme_from_json("Themes/Default.json");
    naui_platform_initialize();

    ImFontConfig config;
    config.RasterizerMultiply = 2.0f;
    io.Fonts->AddFontFromFileTTF("Fonts/Nunito.ttf", 18.0f, &config);

    bool is_running = true;
    naui_event_connect(NauiSystemEventCode_Quit, [&](void *data) { is_running = false; });
    naui_event_connect(NauiSystemEventCode_Resize, [&](void *data) { naui_render(); });

    naui_asset_manager_initialize();
    naui_panel_manager_initialize();

    on_initialize();

    while (is_running)
	{
        naui_render();
		naui_process_deferred();
	}

    on_shutdown();

    naui_panel_manager_shutdown();
    naui_asset_manager_shutdown();

    naui_platform_shutdown();
    ImGui::DestroyContext();
}