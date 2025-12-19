#pragma once

#include "Base.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "MenuBar.h"

namespace Naui
{

class NAUI_API App
{
public:
    virtual ~App(void) = default;
	virtual void OnMenuBar() { }

	void SetMenuBar(MenuBar* menubar);
    void Run(void);

private:
    virtual void OnEnter(void) { }
    virtual void OnExit(void) { }

    void Render(void);

    PlatformWindow *m_window = nullptr;
    Renderer *m_renderer = nullptr;
	MenuBar m_menubar;
};

}