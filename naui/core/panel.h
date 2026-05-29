#pragma once
#include "base.h"
#include "math/vec2.h"

typedef uint64_t Naui_PanelID;
typedef void(*NauiPanelEvent)(Naui_PanelID panel_id, void *data);

typedef uint8_t Naui_DockDirection;
enum
{
    NAUI_DOCK_DIRECTION_LEFT,
    NAUI_DOCK_DIRECTION_RIGHT,
    NAUI_DOCK_DIRECTION_TOP,
    NAUI_DOCK_DIRECTION_BOTTOM,
    NAUI_DOCK_DIRECTION_CENTER // For tabs
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

#define NAUI_ATTACH_PANEL(type_name) naui_attach_panel(#type_name);

NAUI_API Naui_PanelID naui_attach_panel(const char *type_name);
NAUI_API void         naui_detach_panel(Naui_PanelID id);
NAUI_API void         naui_register_panel_type(const char *name, Naui_PanelType type);

NAUI_API void         naui_panel_set_title  (Naui_PanelID panel_id, const char *title);
NAUI_API void         naui_panel_set_size   (Naui_PanelID panel_id, Naui_Vec2 size);

NAUI_API Naui_PanelID naui_dock_panel   (Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio);
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

#define NAUI_DEFINE_PANEL_TYPE(name, data_type) \
    static Naui_PanelType _##name##_events = { \
        (NauiPanelEvent)on_attach, \
        (NauiPanelEvent)on_detach, \
        (NauiPanelEvent)on_update, \
        (NauiPanelEvent)on_render, \
        sizeof(data_type) \
    }; \
    NAUI_CONSTRUCTOR_NAMED(_register_##name) { \
        naui_register_panel_type(#name, _##name##_events); \
    }

