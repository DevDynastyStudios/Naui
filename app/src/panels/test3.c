#include <naui.h>

#include <stdio.h>

static void on_attach(void)
{
    Naui_PanelID this = naui_current_panel();
    naui_panel_set_title(this, "Test 3");
    naui_panel_set_size(this, (Naui_Vec2){ 256, 256 });
    naui_panel_enable_flags(this, NAUI_PANEL_FLAG_SERIALIZABLE);
}

static void on_detach(void)
{
    
}

static void on_update(void)
{
    
}

static void on_render(void)
{

}

NAUI_DEFINE_PANEL_TYPE_NO_DATA(test3);