#pragma once

#include "base.h"
#include "math/vec2.h"

typedef uint64_t Naui_PanelUID;
typedef void(*NauiPanelEvent)(Naui_PanelUID uid, void *data);

typedef struct
{
    NauiPanelEvent on_attach;
    NauiPanelEvent on_detach;
    NauiPanelEvent on_update;
}
Naui_PanelTypeEvents;

#define NAUI_PANEL_STACK_SIZE 1 << 13
typedef struct
{
    Naui_PanelUID uid;
    Naui_PanelTypeEvents events;
    Naui_Vec2 position;
    Naui_Vec2 size;
    uint8_t _stack[NAUI_PANEL_STACK_SIZE];
    bool docked;
}
Naui_Panel;

#define NAUI_ATTACH_PANEL(type_name) naui_attach_panel(#type_name);
Naui_PanelUID naui_attach_panel(const char *type_name);
void naui_detach_panel(Naui_PanelUID uid);
Naui_Panel *naui_get_panel(Naui_PanelUID uid);

void naui_register_panel_type(const char *name, Naui_PanelTypeEvents type_events);
void naui_unregister_panel_type(const char *name);

#ifdef _MSC_VER
  #pragma section(".CRT$XCU", read)
  #define NAUI_CONSTRUCTOR(fn) \
      static void fn(void); \
      __declspec(allocate(".CRT$XCU")) void(*fn##_)(void) = fn; \
      static void fn(void)
#else
  #define NAUI_CONSTRUCTOR(fn) \
      __attribute__((constructor)) static void fn(void)
#endif

#define NAUI_DEFINE_PANEL_TYPE(name) \
    Naui_PanelTypeEvents name##_events = { \
        (NauiPanelEvent)name##_on_attach, \
        (NauiPanelEvent)name##_on_detach, \
        (NauiPanelEvent)name##_on_update \
    }; \
    NAUI_CONSTRUCTOR(name##_register) { \
        naui_register_panel_type(#name, name##_events); \
    }