#pragma once

typedef void (*Naui_AppEvent)(void);

void naui_app_run(
    const char *title,
    Naui_AppEvent start,
    Naui_AppEvent end,
    Naui_AppEvent update
);

#define NAUI_APP(title) \
void naui_app_start(void); \
void naui_app_end(void); \
void naui_app_update(void); \
int main(void) { \
    naui_app_run(title, naui_app_start, naui_app_end, naui_app_update); \
    return 0; \
}
