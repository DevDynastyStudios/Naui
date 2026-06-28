#pragma once

#include "base.h"

typedef void (*Naui_AppEvent)(void);

NAUI_API void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update
);

NAUI_API int32_t    naui_app_width  (void);
NAUI_API int32_t    naui_app_height (void);

NAUI_API void       naui_app_set_caption_height (int32_t height);

#define NAUI_APP(title) \
void naui_app_start(void); \
void naui_app_end(void); \
void naui_app_update(void); \
int main(void) { \
    naui_app_run(title, naui_app_start, naui_app_end, naui_app_update); \
    return 0; \
}
