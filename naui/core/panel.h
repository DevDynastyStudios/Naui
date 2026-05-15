#pragma once

#include "base.h"
#include "math/vec2.h"

typedef void(*NauiPanelEvent)(void *data);

#define NAUI_PANEL_STACK_SIZE 1 << 13
typedef struct
{
    uint64_t uid;
    NauiPanelEvent on_attach;
    NauiPanelEvent on_detach;
    NauiPanelEvent on_update;
    Naui_Vec2 position;
    Naui_Vec2 size;
    uint8_t _stack[NAUI_PANEL_STACK_SIZE];
    bool docked;
}
Naui_Panel;

void naui_attach_panel(void);