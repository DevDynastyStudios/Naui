#pragma once
#include "base.h"
#include "math/vec2.h"

typedef uint64_t Naui_PanelID;
typedef void(*NauiPanelEvent)(Naui_PanelID panel_id, void *data);

typedef struct
{
    NauiPanelEvent on_attach;
    NauiPanelEvent on_detach;
    NauiPanelEvent on_update;
}
Naui_PanelType;

#define NAUI_ATTACH_PANEL(type_name) naui_attach_panel(#type_name);

NAUI_API Naui_PanelID naui_attach_panel(const char *type_name);
NAUI_API void         naui_detach_panel(Naui_PanelID id);
NAUI_API void         naui_register_panel_type(const char *name, Naui_PanelType type);
NAUI_API void         naui_panel_set_title(Naui_PanelID panel_id, const char *title);

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
    static Naui_PanelType _events = { \
        (NauiPanelEvent)on_attach, \
        (NauiPanelEvent)on_detach, \
        (NauiPanelEvent)on_update \
    }; \
    NAUI_CONSTRUCTOR(_register) { \
        naui_register_panel_type(#name, _events); \
    }