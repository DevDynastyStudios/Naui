#pragma once

#include "base.h"

#include <functional>
#include <cstdint>

typedef std::function<void(void *data)> NauiEvent;

enum NauiSystemEventCode : uint8_t
{
    NauiSystemEventCode_Quit,
    NauiSystemEventCode_KeyPressed,
    NauiSystemEventCode_KeyReleased,
    NauiSystemEventCode_Char,
    NauiSystemEventCode_FileDropped,
    NauiSystemEventCode_Resize,
    NauiSystemEventCode_MAX
};

NAUI_API void naui_event_connect(const NauiSystemEventCode code, NauiEvent on_event);
NAUI_API void naui_event_call(const NauiSystemEventCode code, void *data);
