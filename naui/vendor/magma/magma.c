#define MG_IMPL

#if defined(_WIN32)
#define MGFX_D3D11
#elif defined(__linux__)
#define MGFX_OPENGL
#endif

#include "mgapp.h"
#include "mgfx.h"
