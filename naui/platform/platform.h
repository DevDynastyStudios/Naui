#pragma once

#include "base.h"

#include <filesystem>
#include <imgui.h>

typedef void *NauiLibrary;
typedef void *NauiProcAddress;

typedef uint32_t NauiPlatformWindowFlags;
enum
{
    NauiPlatformWindowFlags_None = 0,
    NauiPlatformWindowFlags_Resizeable = 1 << 1,
    NauiPlatformWindowFlags_Minimizable = 1 << 2,
    NauiPlatformWindowFlags_Closable = 1 << 3
};

struct NauiWindowProps
{
    uint32_t width = 1600, height = 900;
    const char *title = "Naui";
    NauiPlatformWindowFlags flags = 
        NauiPlatformWindowFlags_Resizeable |
        NauiPlatformWindowFlags_Minimizable |
        NauiPlatformWindowFlags_Closable;
};

typedef void *NauiChildWindow;

struct NauiImage
{
    uint32_t width, height;
    union
    {
        struct
        {
            void *d3d11_texture;
            void *d3d11_texture_view;
        };
        uint32_t gl_id;
    }
    internal;

    operator ImTextureRef(void) const
    {
    #if NAUI_PLATFORM_WINDOWS
        return ImTextureRef(internal.d3d11_texture_view);
    #elif NAUI_PLATFORM_LINUX
        return ImTextureRef(internal.gl_id);
    #endif
    }
};

NAUI_API void naui_platform_initialize(const NauiWindowProps &props = {});
NAUI_API void naui_platform_shutdown(void);

NAUI_API void naui_platform_begin(void);
NAUI_API void naui_platform_end(void);

NAUI_API NauiChildWindow naui_create_child_window(const NauiWindowProps &props = {});
NAUI_API void naui_destroy_child_window(const NauiChildWindow *window);

NAUI_API NauiLibrary naui_load_library(const char *path);
NAUI_API void naui_unload_library(NauiLibrary library);
NAUI_API NauiProcAddress naui_get_proc_address(NauiLibrary library, const char *name);

NAUI_API std::filesystem::path naui_open_file_dialog(const wchar_t* filter, const wchar_t* title);
NAUI_API std::filesystem::path naui_save_file_dialog(const wchar_t* filter, const wchar_t* title);

NAUI_API NauiImage naui_create_image(const char *path);
NAUI_API void naui_destroy_image(const NauiImage *image);

const char* naui_get_executable_path(void);
const char* naui_get_working_directory(void);