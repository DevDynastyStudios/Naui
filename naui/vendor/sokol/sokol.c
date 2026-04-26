#include <base.h>

#if NAUI_WINDOWS
#define SOKOL_D3D11
#elif NAUI_LINUX
#define SOKOL_GLCORE
#endif

#define SOKOL_NO_ENTRY
#define SOKOL_IMPL
#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_log.h>