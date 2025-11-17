#include "platform.h"

#if NAUI_PLATFORM_MAC
// Objective-C++ implementation for macOS
#include "event.h"
#include "event_types.h"
#include "input.h"

#include <AppKit/AppKit.h>
#include <dlfcn.h>
#include <chrono>
#include <filesystem>

// --- Keycode mapping ---
static NauiKey mac_keycode_to_naui(unsigned short keyCode)
{
    switch (keyCode)
    {
        // Letters
        case 0x00: return NauiKey::A;
        case 0x0B: return NauiKey::B;
        case 0x08: return NauiKey::C;
        case 0x02: return NauiKey::D;
        case 0x0E: return NauiKey::E;
        case 0x03: return NauiKey::F;
        case 0x05: return NauiKey::G;
        case 0x04: return NauiKey::H;
        case 0x22: return NauiKey::I;
        case 0x26: return NauiKey::J;
        case 0x28: return NauiKey::K;
        case 0x25: return NauiKey::L;
        case 0x2E: return NauiKey::M;
        case 0x2D: return NauiKey::N;
        case 0x1F: return NauiKey::O;
        case 0x23: return NauiKey::P;
        case 0x0C: return NauiKey::Q;
        case 0x0F: return NauiKey::R;
        case 0x01: return NauiKey::S;
        case 0x11: return NauiKey::T;
        case 0x20: return NauiKey::U;
        case 0x09: return NauiKey::V;
        case 0x0D: return NauiKey::W;
        case 0x07: return NauiKey::X;
        case 0x10: return NauiKey::Y;
        case 0x06: return NauiKey::Z;

        // Numbers
        case 0x12: return NauiKey::Num1;
        case 0x13: return NauiKey::Num2;
        case 0x14: return NauiKey::Num3;
        case 0x15: return NauiKey::Num4;
        case 0x17: return NauiKey::Num5;
        case 0x16: return NauiKey::Num6;
        case 0x1A: return NauiKey::Num7;
        case 0x1C: return NauiKey::Num8;
        case 0x19: return NauiKey::Num9;
        case 0x1D: return NauiKey::Num0;

        // Control keys
        case 0x33: return NauiKey::Backspace;
        case 0x30: return NauiKey::Tab;
        case 0x24: return NauiKey::Enter;
        case 0x35: return NauiKey::Escape;
        case 0x31: return NauiKey::Space;

        case 0x7B: return NauiKey::LeftArrow;
        case 0x7C: return NauiKey::RightArrow;
        case 0x7D: return NauiKey::DownArrow;
        case 0x7E: return NauiKey::UpArrow;

        case 0x73: return NauiKey::Home;
        case 0x77: return NauiKey::End;
        case 0x74: return NauiKey::PageUp;
        case 0x79: return NauiKey::PageDown;
        case 0x72: return NauiKey::Insert;
        case 0x75: return NauiKey::Delete;

        // Modifiers
        case 0x38: return NauiKey::LShift;
        case 0x3C: return NauiKey::RShift;
        case 0x3B: return NauiKey::LControl;
        case 0x3E: return NauiKey::RControl;
        case 0x3A: return NauiKey::LAlt;
        case 0x3D: return NauiKey::RAlt;
        case 0x37: return NauiKey::LSuper;
        case 0x36: return NauiKey::RSuper;

        // Function keys
        case 0x7A: return NauiKey::F1;
        case 0x78: return NauiKey::F2;
        case 0x63: return NauiKey::F3;
        case 0x76: return NauiKey::F4;
        case 0x60: return NauiKey::F5;
        case 0x61: return NauiKey::F6;
        case 0x62: return NauiKey::F7;
        case 0x64: return NauiKey::F8;
        case 0x65: return NauiKey::F9;
        case 0x6D: return NauiKey::F10;
        case 0x67: return NauiKey::F11;
        case 0x6F: return NauiKey::F12;

        // Punctuation (US layout)
        case 0x1B: return NauiKey::Minus;
        case 0x18: return NauiKey::Equal;
        case 0x21: return NauiKey::LBracket;
        case 0x1E: return NauiKey::RBracket;
        case 0x2A: return NauiKey::Backslash;
        case 0x29: return NauiKey::Grave;
        case 0x27: return NauiKey::Semicolon;
        case 0x2B: return NauiKey::Quote;
        case 0x2C: return NauiKey::Comma;
        case 0x2F: return NauiKey::Period;
        case 0x2D: return NauiKey::Slash;

        default:   return NauiKey::MaxKeys;
    }
}

// --- Custom NSView to capture input ---
@interface NauiContentView : NSView <NSDraggingDestination>
@end

@implementation NauiContentView
- (BOOL)acceptsFirstResponder { return YES; }

- (void)keyDown:(NSEvent *)event {
    NSString *chars = [event characters];
    if (chars.length > 0) {
        unichar uc = [chars characterAtIndex:0];
        char c = (uc <= 0x7F) ? (char)uc : 0;
        if (c) {
            NauiCharEvent ce{ c };
            naui_event_call(NauiSystemEventCode_Char, &ce);
        }
    }
    NauiKeyEvent e{ mac_keycode_to_naui(event.keyCode) };
    naui_event_call(NauiSystemEventCode_KeyPressed, &e);
}

- (void)keyUp:(NSEvent *)event {
    NauiKeyEvent e{ mac_keycode_to_naui(event.keyCode) };
    naui_event_call(NauiSystemEventCode_KeyReleased, &e);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender { return NSDragOperationCopy; }
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard *pb = [sender draggingPasteboard];
    NSArray<NSURL*> *urls = [pb readObjectsForClasses:@[NSURL.class]
                                              options:@{ NSPasteboardURLReadingFileURLsOnlyKey:@YES }];
    if (urls.count > 0) {
        NSURL *url = urls.firstObject;
        NauiFileDropEvent e{ url.path.UTF8String };
        naui_event_call(NauiSystemEventCode_FileDropped, &e);
        return YES;
    }
    return NO;
}
@end

// --- Window delegate for quit ---
@interface NauiWindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation NauiWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
    NauiQuitEvent e{};
    naui_event_call(NauiSystemEventCode_Quit, &e);
    // Return YES so the window actually closes; app can decide to exit or not
    return YES;
}
@end

static NSApplication *g_app = nil;
static NSWindow *g_mainWindow = nil;
static NauiWindowDelegate *g_mainDelegate = nil;

void naui_platform_initialize(const NauiPlatformCreateInfo *create_info)
{
    if (!g_app) {
        [NSApplication sharedApplication];
        g_app = NSApp;
        [g_app setActivationPolicy:NSApplicationActivationPolicyRegular];
    }

    NSRect rect = NSMakeRect(100, 100, create_info->width, create_info->height);

    g_mainWindow = [[NSWindow alloc] initWithContentRect:rect
                                               styleMask:(NSWindowStyleMaskTitled |
                                                          NSWindowStyleMaskClosable |
                                                          NSWindowStyleMaskResizable |
                                                          NSWindowStyleMaskMiniaturizable)
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    [g_mainWindow setTitle:[NSString stringWithUTF8String:create_info->title]];

    NauiContentView *content = [[NauiContentView alloc] initWithFrame:rect];
    [content registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
    [g_mainWindow setContentView:content];
    [g_mainWindow makeFirstResponder:content];

    g_mainDelegate = [NauiWindowDelegate new];
    [g_mainWindow setDelegate:g_mainDelegate];

    [g_mainWindow makeKeyAndOrderFront:nil];
    [g_app activateIgnoringOtherApps:YES];
}

void naui_platform_shutdown(void)
{
    if (g_mainWindow) {
        [g_mainWindow close];
        g_mainWindow = nil;
    }
    g_mainDelegate = nil;
    g_app = nil;
}

void naui_platform_begin(void)
{
    NSEvent *event;
    while ((event = [g_app nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES])) {
        [g_app sendEvent:event];
    }
}

void naui_platform_end(void)
{
    // Placeholder for buffer swap or flush if needed
}

NauiChildWindow naui_create_child_window(const NauiChildWindowCreateInfo *create_info)
{
    NauiChildWindow child{};

    NSRect rect = NSMakeRect(120, 120, create_info->width, create_info->height);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:rect
                                                   styleMask:(NSWindowStyleMaskTitled |
                                                              NSWindowStyleMaskClosable |
                                                              NSWindowStyleMaskResizable |
                                                              NSWindowStyleMaskMiniaturizable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window setTitle:[NSString stringWithUTF8String:create_info->title]];

    NauiContentView *content = [[NauiContentView alloc] initWithFrame:rect];
    [content registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
    [window setContentView:content];
    [window makeFirstResponder:content];
    [window makeKeyAndOrderFront:nil];

    child.handle = (__bridge_retained void *)window;
    return child;
}

void naui_destroy_child_window(const NauiChildWindow *window)
{
    if (window && window->handle) {
        NSWindow *nsWindow = (__bridge_transfer NSWindow *)window->handle;
        [nsWindow close];
    }
}

double naui_get_time(void)
{
    using namespace std::chrono;
    static auto start = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    return duration<double>(now - start).count();
}

NauiLibrary naui_load_library(const char *path)
{
    return dlopen(path, RTLD_NOW);
}

void naui_unload_library(NauiLibrary library)
{
    if (library) dlclose(library);
}

NauiProcAddress naui_get_proc_address(NauiLibrary library, const char *name)
{
    if (!library) return nullptr;
    return dlsym(library, name);
}

std::filesystem::path naui_open_file_dialog(const wchar_t* /*filter*/, const wchar_t* title)
{
    @autoreleasepool {
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];

        if (title) {
            [panel setTitle:[NSString stringWithCharacters:(const unichar*)title
                                                    length:wcslen(title)]];
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL *url = panel.URLs.firstObject;
            if (url) return std::filesystem::path(url.path.UTF8String);
        }
    }
    return {};
}

std::filesystem::path naui_save_file_dialog(const wchar_t* /*filter*/, const wchar_t* title)
{
    @autoreleasepool {
        NSSavePanel *panel = [NSSavePanel savePanel];
        if (title) {
            [panel setTitle:[NSString stringWithCharacters:(const unichar*)title
                                                    length:wcslen(title)]];
        }
        if ([panel runModal] == NSModalResponseOK) {
            NSURL *url = panel.URL;
            if (url) return std::filesystem::path(url.path.UTF8String);
        }
    }
    return {};
}

const char* naui_get_executable_path(void)
{
    static char path[_MAX_PATH];
    uint32_t size = sizeof(path);

    if (_NSGetExecutablePath(path, &size) != 0)
        return "";

    size_t safe_len = strnlen_s(path, sizeof(path));
    path[safe_len] = '\0';
    return path;
}

const char* naui_get_working_directory(void)
{
    static char cwd[_MAX_PATH];
    if (!getcwd(cwd, sizeof(cwd)))
        return "";

    size_t safe_len = strnlen_s(cwd, sizeof(cwd));
    cwd[safe_len] = '\0';
    return cwd;
}

#endif