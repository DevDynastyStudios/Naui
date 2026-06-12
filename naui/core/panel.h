#pragma once
#include "base.h"
#include "math/vec2.h"

#ifndef NAUI_PANEL_BORDER_COLOR_TAG
    #define NAUI_PANEL_BORDER_COLOR_TAG "naui_panel_border_color"
#endif

#ifndef NAUI_PANEL_BORDER_WIDTH_TAG
    #define NAUI_PANEL_BORDER_WIDTH_TAG "naui_panel_border_width"
#endif

#ifndef NAUI_PANEL_TITLEBAR_BG_COLOR_TAG
    #define NAUI_PANEL_TITLEBAR_BG_COLOR_TAG "naui_panel_title_bg_color"
#endif

#ifndef NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG
    #define NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG "naui_panel_title_text_color"
#endif

#ifndef NAUI_PANEL_TITLEBAR_PADDING_TAG
    #define NAUI_PANEL_TITLEBAR_PADDING_TAG "naui_panel_title_padding"
#endif

#ifndef NAUI_PANEL_BODY_BG_COLOR_TAG
    #define NAUI_PANEL_BODY_BG_COLOR_TAG "naui_panel_body_bg_color"
#endif

#ifndef NAUI_PANEL_BODY_PADDING_TAG
    #define NAUI_PANEL_BODY_PADDING_TAG "naui_panel_body_padding"
#endif

#ifndef NAUI_PANEL_ROUNDING_TAG
    #define NAUI_PANEL_ROUNDING_TAG "naui_panel_rounding"
#endif

#ifndef NAUI_PANEL_SHADOW_COLOR_TAG
    #define NAUI_PANEL_SHADOW_COLOR_TAG "naui_panel_shadow_color"
#endif

#ifndef NAUI_PANEL_FONT_SIZE_TAG
    #define NAUI_PANEL_FONT_SIZE_TAG "naui_panel_font_size"
#endif

#ifndef NAUI_DOCK_GUIDE_COLOR
    #define NAUI_DOCK_GUIDE_COLOR "naui_dock_guide_color"
#endif

typedef uint64_t Naui_PanelID;
typedef void(*NauiPanelEvent)(Naui_PanelID panel_id, void *data);

typedef uint8_t Naui_DockDirection;
enum
{
    NAUI_DOCK_DIRECTION_LEFT,
    NAUI_DOCK_DIRECTION_RIGHT,
    NAUI_DOCK_DIRECTION_TOP,
    NAUI_DOCK_DIRECTION_BOTTOM,
    NAUI_DOCK_DIRECTION_CENTER // For tabs (don't use it for now)
};

typedef struct
{
    NauiPanelEvent on_attach;
    NauiPanelEvent on_detach;
    NauiPanelEvent on_update;
    NauiPanelEvent on_render;
    size_t user_data_size;
}
Naui_PanelType;

typedef uint32_t Naui_PanelFlags;
enum
{
    NAUI_PANEL_FLAG_NONE = 0,
    NAUI_PANEL_FLAG_NO_CLOSE = 1 << 0,
    NAUI_PANEL_FLAG_NO_DOCKING = 1 << 1,
    NAUI_PANEL_FLAG_NO_MOVE = 1 << 2,
    NAUI_PANEL_FLAG_NO_RESIZE = 1 << 3,
    NAUI_PANEL_FLAG_ALWAYS_TO_FRONT = 1 << 4,
    __NAUI_PANEl_FLAG_VIEWPORT = 1 << 5 // internal flag for whether this is the main window viewport panel
};

#define NAUI_ATTACH_PANEL(type_name) naui_attach_panel(#type_name)

NAUI_API Naui_PanelID naui_attach_panel(const char *type_name);
NAUI_API void         naui_detach_panel(Naui_PanelID id);
NAUI_API void         naui_register_panel_type(const char *name, Naui_PanelType type);

NAUI_API void         naui_panel_set_title      (Naui_PanelID panel_id, const char *title);
NAUI_API void         naui_panel_set_size       (Naui_PanelID panel_id, Naui_Vec2 size);
NAUI_API void         naui_panel_enable_flags   (Naui_PanelID panel_id, Naui_PanelFlags flags);
NAUI_API void         naui_panel_disable_flags  (Naui_PanelID panel_id, Naui_PanelFlags flags);

NAUI_API void         naui_dock_panel   (Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio);
NAUI_API void         naui_undock_panel (Naui_PanelID id);

#ifdef _MSC_VER
  #pragma section(".CRT$XCU", read)
  #define NAUI_CONSTRUCTOR_NAMED(fn) \
      static void fn(void); \
      __declspec(allocate(".CRT$XCU")) void(*fn##_)(void) = fn; \
      static void fn(void)
#else
  #define NAUI_CONSTRUCTOR_NAMED(fn) \
      __attribute__((constructor)) static void fn(void)
#endif

#define __NAUI_DEFINE_PANEL_TYPE(name, data_size) \
    static Naui_PanelType _##name##_events = { \
        (NauiPanelEvent)on_attach, \
        (NauiPanelEvent)on_detach, \
        (NauiPanelEvent)on_update, \
        (NauiPanelEvent)on_render, \
        data_size \
    }; \
    NAUI_CONSTRUCTOR_NAMED(_register_##name) { \
        naui_register_panel_type(#name, _##name##_events); \
    }

#define NAUI_DEFINE_PANEL_TYPE(name, data_type) __NAUI_DEFINE_PANEL_TYPE(name, sizeof(data_type))
#define NAUI_DEFINE_PANEL_TYPE_NO_DATA(name) __NAUI_DEFINE_PANEL_TYPE(name, 0)