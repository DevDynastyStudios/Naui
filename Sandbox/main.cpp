#include "Naui.h"
using namespace ImGui;

class MidiEditor : public Naui::Panel
{
public:
    MidiEditor(void) : Naui::Panel("Midi Editor")
    {
        SetCategory("Windows/Editors");
        SetClosable(false);
    }

protected:
    void OnRender(void) override
    {
        Text("Hello from Midi Editor Panel!");
        if (Button("Click Me"))
        {
            Text("Button Clicked!");
        }
    }

    void OnClose(void) override
    {
        Naui::DestroyPanel(GetUID());
    }
};

class UphonicApp : public Naui::App
{
private:
    void OnEnter(void) override
    {
        Naui::AddPanel<MidiEditor>();
        Naui::AddPanel<MidiEditor>();
        printf("UphonicApp has started.\n");
    }

    void OnExit(void) override
    {
        printf("UphonicApp is exiting.\n");
    }
};

int main(void)
{
    UphonicApp app;
    app.Run();
    return 0;
}