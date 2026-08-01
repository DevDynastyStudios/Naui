// header files
#include "base.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#if NAUI_WINDOWS
#include "vendor/dirent/dirent.h"
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "vendor/stb/stb.c"
#include "vendor/miniz/miniz.c"
#include "vendor/magma/magma.c"
#include "vendor/leaf/leaf.c"

#include "utils/list.h"
#include "utils/hash.h"
#include "utils/uuid.h"
#include "utils/arena.h"
#include "utils/string.h"
#include "utils/map.h"

#include "math/math.h"
#include "math/vec2.h"
#include "math/vec4.h"

#include "core/action.h"
#include "core/time.h"
#include "core/app.h"
#include "core/input.h"
#include "core/panel.h"
#include "core/log.h"
#include "core/shortcut.h"

#include "filesystem/filesystem.h"
#include "filesystem/iterator.h"
#include "filesystem/archive.h"

#include "serialization/json_writer.h"
#include "serialization/json_reader.h"
#include "serialization/json.h"

#include "renderer/renderer.h"
#include "renderer/shaders/base.glsl.h"
#include "renderer/asset_manager.h"
#include "core/theme.h" // need Naui_Color

#include "threading/jobs.h"
#include "threading/threads.h"

#include "localization/localization.h"

// source files
#include "utils/list.cpp"
#include "utils/arena.cpp"
#include "utils/uuid_unix.cpp"
#include "utils/uuid_win32.cpp"
#include "utils/string.cpp"

#include "core/shortcut.cpp"
#include "core/time.cpp"
#include "core/app.cpp"
#include "core/action.cpp"
#include "core/input.cpp"
#include "core/log.cpp"
#include "core/theme.cpp"
#include "core/panel.cpp"

#include "serialization/json.cpp"
#include "serialization/json_writer.cpp"
#include "serialization/json_reader.cpp"

#include "renderer/renderer.cpp"
#include "renderer/asset_manager.cpp"

#include "threading/jobs.cpp"
#include "threading/thread_win32.cpp"
#include "threading/thread_unix.cpp"

#include "filesystem/iterator_unix.cpp"
#include "filesystem/filesystem_win32.cpp"
#include "filesystem/iterator_win32.cpp"
#include "filesystem/filesystem_unix.cpp"
#include "filesystem/archive.cpp"

#include "localization/localization.cpp"
