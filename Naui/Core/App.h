#pragma once

#include "Base.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

namespace Naui
{

class NAUI_API App
{
public:
    virtual ~App(void) = default;
    void Run(void);

private:
    virtual void OnEnter(void) { }
    virtual void OnExit(void) { }

    void Render(void);

    PlatformWindow *m_window = nullptr;
    Renderer *m_renderer = nullptr;
};

}