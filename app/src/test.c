#include <naui.h>
#include <stdio.h>

typedef struct
{
    float time;
}
TestData;

void test_on_attach(Naui_PanelUID uid, TestData *data)
{
    Naui_Panel *panel = naui_get_panel(uid);
}

void test_on_detach(Naui_PanelUID uid, TestData *data)
{
    
}

void test_on_update(Naui_PanelUID uid, TestData *data)
{
    data->time = naui_time();
    Naui_Panel *panel = naui_get_panel(uid);

}

NAUI_DEFINE_PANEL_TYPE(test);