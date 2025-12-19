#include "Window.h"
#include "Base.h"

#if NAUI_PLATFORM_WINDOWS
    #include "Win32.h"
#endif

namespace Naui
{

PlatformWindow *CreatePlatformWindow(int width, int height, const char *title)
{
#if NAUI_PLATFORM_WINDOWS
    return new PlatformWin32Window(width, height, title);
#else
    return nullptr;
#endif
}

}