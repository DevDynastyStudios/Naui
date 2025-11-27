#include "platform.h"

#if NAUI_PLATFORM_LINUX

// NOTE(smoke): using SDL2 for this since
// ImGui doesn't have an Xlib (or xcb) backend...
// another excuse for me not having to deal with
// Xlib and modern opengl contexts using GLX ;)

// NOTE(smoke): me using 4coder, if u don't like
// the style of anything plz change it :)

#include "event.h"
#include "event_types.h"

#include <assert.h>
#include <cstdint>
#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <string.h> 

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <stb_image.h>

// NOTE(smoke): no need for GLAD, ImGui comes with
// it's own opengl loader (imgui_impl_opengl3_loader.h)

typedef struct NauiSdl2Platform
{
    SDL_Window *window;
    uint32_t window_width, window_height;
    
    SDL_GLContext gl_context;
}
NauiSdl2Platform;

static NauiSdl2Platform *platform;

void naui_platform_initialize(const NauiWindowProps &props)
{
	platform = new NauiSdl2Platform;
    
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0 && "SDL2_Init failed");
    
    // NOTE(smoke): should we use an older opengl version
    // for compatiblity with older devices? afterall ImGui
    // is the one handling the graphics, not us.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    platform->window = SDL_CreateWindow(props.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(props.width * main_scale), (int)(props.height * main_scale), window_flags);
    assert(window && "SDL_CreateWindow failed");
    platform->window_width = props.width;
    platform->window_height = props.height;
    
    platform->gl_context = SDL_GL_CreateContext(platform->window);
    assert(platform->gl_context && "SDL_GL_CreateContext failed");
    SDL_GL_MakeCurrent(platform->window, platform->gl_context);
    SDL_GL_SetSwapInterval(1);
    
    ImGui_ImplSDL2_InitForOpenGL(platform->window, platform->gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void naui_platform_shutdown(void)
{
	ImGui::DestroyPlatformWindows();
    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    
    SDL_GL_DeleteContext(platform->gl_context);
    SDL_DestroyWindow(platform->window);
    SDL_Quit();
    
	delete platform;
}

void naui_platform_begin(void)
{
    // NOTE(smoke): move event handling into a seperate function???
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            {
                NauiQuitEvent data;
                naui_event_call(NauiSystemEventCode_Quit, (void*)&data);
            }
            break;
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    platform->window_width = event.window.data1;
                    platform->window_height = event.window.data2;
                    glViewport(0, 0, platform->window_width, platform->window_height);
                }
            }
            break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                bool pressed = event.key.state == SDL_PRESSED;
                
                NauiKeyEvent data;
                data.key = (NauiKey)event.key.keysym.sym;
                naui_event_call(pressed ? NauiSystemEventCode_KeyPressed : NauiSystemEventCode_KeyReleased, (void*)&data);
            }
            break;
            
            // TODO(smoke): support for NauiCharEvent
            default: break;
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void naui_platform_end(void)
{
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    SDL_GL_SwapWindow(platform->window);
}

// TODO(smoke): implement child windows
NauiChildWindow naui_create_child_window(const NauiWindowProps &props)
{
}

void naui_destroy_child_window(const NauiChildWindow *window)
{
}

NauiLibrary naui_load_library(const char *path)
{
    return (NauiLibrary)dlopen(path, RTLD_LAZY);
}

NauiProcAddress naui_get_proc_address(NauiLibrary library, const char *name)
{
    return (NauiProcAddress)dlsym((void*)library, name);
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

	uint32_t gl_id;
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    stbi_image_free(image_data);

    NauiImage image;
    image.width = (uint32_t) width;
    image.height = (uint32_t) height;
	image.internal.gl_id = gl_id;

    return image;
}

void naui_destroy_image(const NauiImage *image)
{
	glDeleteTextures(1, &image->internal.gl_id);
}

const char* naui_get_executable_path(void)
{
    static char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
        return "";

    path[len] = '\0';
    return path;
}

const char* naui_get_working_directory(void)
{
    static char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
        return "";

    return cwd;
}

#endif
