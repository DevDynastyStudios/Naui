#include "platform.h"

#if NAUI_PLATFORM_WINDOWS

#include "event.h"
#include "event_types.h"

#include <stb_image.h>

#include <cstdint>
#include <assert.h>

#include <windows.h>
#include <windowsx.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

typedef struct NauiWin32Platform
{
    HINSTANCE h_instance;
    HWND hwnd;
    uint32_t window_width, window_height;

    ID3D11Device *device;
    ID3D11DeviceContext *device_context;
    IDXGISwapChain *swap_chain;
    ID3D11RenderTargetView *render_target_view;
}
NauiWin32Platform;

static NauiWin32Platform *platform;

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static void create_wide_string_from_utf8(const char *source, WCHAR *target)
{
    uint32_t count;
    count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    if (!count)
    {
        MessageBoxA(0, "Failed to convert string from UTF-8", "Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, count))
    {
        MessageBoxA(NULL, "Failed to convert string from UTF-8", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return;
    }
}

static void create_render_target(void)
{
    ID3D11Texture2D* back_buffer = nullptr;
    platform->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    platform->device->CreateRenderTargetView(back_buffer, NULL, &platform->render_target_view);
    back_buffer->Release();
}

static void cleanup_render_target(void)
{
    if (platform->render_target_view) { platform->render_target_view->Release(); platform->render_target_view = nullptr; }
}

static bool create_device_d3d(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &platform->swap_chain, &platform->device, &featureLevel, &platform->device_context) != S_OK)
        return false;

    create_render_target();
    return true;
}

static void cleanup_device_d3d()
{
    cleanup_render_target();
    if (platform->swap_chain) { platform->swap_chain->Release(); platform->swap_chain = nullptr; }
    if (platform->device_context) { platform->device_context->Release(); platform->device_context = nullptr; }
    if (platform->device) { platform->device->Release(); platform->device = nullptr; }
}

static void naui_apply_window_style(uint32_t &style, uint32_t &ex_style, NauiPlatformWindowFlags flags)
{
    style = WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CAPTION;
    ex_style = WS_EX_APPWINDOW;

    if (flags & NauiPlatformWindowFlags_Resizeable)
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    if (flags & NauiPlatformWindowFlags_Minimizable)
        style |= WS_MINIMIZEBOX;
    if (flags & NauiPlatformWindowFlags_Closable)
        ex_style |= WS_EX_OVERLAPPEDWINDOW;
}

static bool set_window_titlebar_dark_mode(HWND hwnd, bool enabled)
{
    if (!hwnd)
        return false;

    BOOL useDark = enabled ? TRUE : FALSE;

    // Try attribute 20 (Windows 10 1809)
    HRESULT hr = DwmSetWindowAttribute(
        hwnd,
        20,
        &useDark,
        sizeof(useDark)
    );

    // If that fails, try attribute 19 (Windows 11)
    if (FAILED(hr))
    {
        hr = DwmSetWindowAttribute(
            hwnd,
            19,
            &useDark,
            sizeof(useDark)
        );
    }

    return SUCCEEDED(hr);
}

void naui_platform_initialize(const NauiWindowProps &props)
{
    platform = new NauiWin32Platform;
    platform->h_instance = GetModuleHandleA(0);

    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    HICON icon = (HICON)LoadImageA(platform->h_instance, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!icon)
        icon = LoadIcon(platform->h_instance, IDI_APPLICATION);

    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = platform->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "naui_window_class";

    RegisterClassA(&wc);

    uint32_t window_style, window_ex_style;
    naui_apply_window_style(window_style, window_ex_style, props.flags);

    WCHAR wide_title[128];
    create_wide_string_from_utf8(props.title, wide_title);

    platform->hwnd = CreateWindowExA(
        window_ex_style, "naui_window_class", (LPCSTR)wide_title,
        window_style, CW_USEDEFAULT, CW_USEDEFAULT,
        props.width, props.height,
        NULL, NULL, wc.hInstance, NULL);

    if (!create_device_d3d(platform->hwnd))
    {
        cleanup_device_d3d();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return;
    }

    set_window_titlebar_dark_mode(platform->hwnd, true);

    DragAcceptFiles(platform->hwnd, TRUE);

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale; 

    ImGui_ImplWin32_Init(platform->hwnd);
    ImGui_ImplDX11_Init(platform->device, platform->device_context);

    wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = platform->h_instance;
    wc.lpszClassName = "naui_child_window_class";
    RegisterClassA(&wc);

    ShowWindow(platform->hwnd, SW_SHOWDEFAULT);
    UpdateWindow(platform->hwnd);
}

void naui_platform_shutdown(void)
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    cleanup_device_d3d();
    DestroyWindow(platform->hwnd);
    UnregisterClassA("naui_window_class", platform->h_instance);
    UnregisterClassA("naui_child_window_class", platform->h_instance);

    delete platform;
}

void naui_platform_begin(void)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();
}

void naui_platform_end(void)
{
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    platform->device_context->OMSetRenderTargets(1, &platform->render_target_view, NULL);
    platform->device_context->ClearRenderTargetView(platform->render_target_view, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    platform->swap_chain->Present(1, 0);
}

NauiChildWindow naui_create_child_window(const NauiWindowProps &props)
{
    WCHAR wide_title[128];
    create_wide_string_from_utf8(props.title, wide_title);

    uint32_t window_style, window_ex_style;
    naui_apply_window_style(window_style, window_ex_style, props.flags);

    HWND hwndPlugin = CreateWindowExA(
        window_ex_style,
        "naui_child_window_class",
        (LPCSTR)wide_title,
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        props.width, props.height + 32,
        platform->hwnd,
        NULL,
        platform->h_instance,
        NULL
    );

    ShowWindow(hwndPlugin, SW_SHOWDEFAULT);
    UpdateWindow(hwndPlugin);

    return (NauiChildWindow)hwndPlugin;
}

void naui_destroy_child_window(const NauiChildWindow *window)
{
    DestroyWindow((HWND)window);
}

NauiLibrary naui_load_library(const char *path)
{
    return (NauiLibrary)LoadLibraryA(path);
}

void naui_unload_library(NauiLibrary library)
{
    FreeLibrary((HMODULE)library);
}

NauiProcAddress naui_get_proc_address(NauiLibrary library, const char *name)
{
    return (NauiProcAddress)GetProcAddress((HMODULE)library, name);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param))
        return true;

    switch (msg)
    {
        case WM_ERASEBKGND:
            return 1;
        break;
        case WM_CLOSE:
        {
            NauiQuitEvent data;
            naui_event_call(NauiSystemEventCode_Quit, (void*)&data);
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_NCCALCSIZE:
        {
            if (w_param == TRUE)
            {
                LRESULT result = DefWindowProc(hwnd, msg, w_param, l_param);
                
                if (IsMaximized(hwnd))
                {
                    NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)l_param;
                    params->rgrc[0].left -= 2;
                    params->rgrc[0].right += 2;
                    params->rgrc[0].bottom += 2;
                }
                return result;
            }
            return DefWindowProc(hwnd, msg, w_param, l_param);
        }
        case WM_SIZE:
        {
            RECT r;
            GetClientRect(hwnd, &r);

            const uint32_t width = r.right - r.left; 
            const uint32_t height = r.bottom - r.top; 

            if (w_param != SIZE_MINIMIZED)
            {
                cleanup_render_target();
                platform->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                create_render_target();
            }

            NauiResizeEvent data;
            data.width = width;
            data.height = height;
            naui_event_call(NauiSystemEventCode_Resize, (void*)&data);
        }
        break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

            NauiKeyEvent data;
            data.key = (NauiKey)w_param;
            naui_event_call(pressed ? NauiSystemEventCode_KeyPressed : NauiSystemEventCode_KeyReleased, (void*)&data);
        }
        break;
        case WM_CHAR:
        {
            NauiCharEvent data;
            data.ch = (char)w_param;
            naui_event_call(NauiSystemEventCode_Char, (void*)&data);
        }
        break;
        case WM_DROPFILES:
        {
            HDROP h_drop = (HDROP)w_param;
            UINT file_count = DragQueryFileA(h_drop, 0xFFFFFFFF, NULL, 0);

            for (UINT i = 0; i < file_count; ++i)
            {
                char file_path[MAX_PATH];
                DragQueryFileA(h_drop, i, file_path, MAX_PATH);

                NauiFileDropEvent data;
                data.path = file_path;
                naui_event_call(NauiSystemEventCode_FileDropped, (void*)&data);
            }

            DragFinish(h_drop);
        }
        break;
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}

std::filesystem::path naui_open_file_dialog(const wchar_t* filter, const wchar_t* title) 
{
    OPENFILENAMEW ofn{};
    wchar_t szFile[MAX_PATH] = {0};

    ofn.lStructSize  = sizeof(ofn);
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = filter;
    ofn.lpstrTitle   = title;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
        return std::filesystem::path(ofn.lpstrFile);

    return {};
}

std::filesystem::path naui_save_file_dialog(const wchar_t* filter, const wchar_t* title) 
{
    OPENFILENAMEW ofn{};
    wchar_t szFile[MAX_PATH] = {0};

    ofn.lStructSize  = sizeof(ofn);
    ofn.lpstrFile    = szFile;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = filter;
    ofn.lpstrTitle   = title;
    ofn.Flags        = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn))
        return std::filesystem::path(ofn.lpstrFile);
		
    return {};
}

NauiImage naui_create_image(const char *path)
{
    int width, height, channels;
    unsigned char* image_data = stbi_load(path, &width, &height, &channels, 0);
    if (!image_data)
    {
        fprintf(stderr, "[Naui] Failed to load image!\n");
        return{};
    }

    ID3D11Texture2D* texture;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource{};
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = width * channels;

    HRESULT hr = platform->device->CreateTexture2D(&desc, &subResource, &texture);
    stbi_image_free(image_data);

    if (FAILED(hr))
    {
        fprintf(stderr, "[Naui] Failed to create texture from image!\n");
        return{};
    }

    ID3D11ShaderResourceView* texture_view = nullptr;
    hr = platform->device->CreateShaderResourceView(texture, nullptr, &texture_view);
    texture->Release();
    if (FAILED(hr)) 
    {
        fprintf(stderr, "[Naui] Failed to create shader resource view!\n");
        return{};
    }

    NauiImage image;
    image.width = width;
    image.height = height;

    image.internal.d3d11_texture = texture;
    image.internal.d3d11_texture_view = texture_view;

    return image;
}

void naui_destroy_image(const NauiImage *image)
{
    ((ID3D11Texture2D*)image->internal.d3d11_texture)->Release();
    ((ID3D11ShaderResourceView*)image->internal.d3d11_texture_view)->Release();
}

#endif
