#include "input.h"
#include <magma/mgapp.h>

bool naui_key_down(Naui_Key key)
{
    return mg_app_key_down((mg_key)key);
}

bool naui_key_pressed(Naui_Key key)
{
    return mg_app_key_pressed((mg_key)key);
}
bool naui_mouse_down(Naui_MouseButton button)
{
    return mg_app_mouse_down((mg_mouse_button)button);
}

bool naui_mouse_clicked(Naui_MouseButton button)
{
    return mg_app_mouse_clicked((mg_mouse_button)button);
}

int8_t naui_mouse_scroll_delta(void)
{
    return mg_app_mouse_scroll_delta();
}

int32_t naui_mouse_x(void)
{
    return mg_app_mouse_x();
}

int32_t naui_mouse_y(void)
{
    return mg_app_mouse_y();
}