// TODO(doomguy): refactor this whole piece of hot garbage into build.h (core) and build.c (build recipe)
// TODO(doomguy): specify graphics APIs from args
// TODO(doomguy): unit tests
// TODO(doomguy): its kinda annoying to use -lshell32 on windows, find workaround

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

#include "naui/base.h"

#if NAUI_WINDOWS
#include "naui/vendor/dirent/dirent.h"
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#define STB_DS_IMPLEMENTATION
#include "naui/vendor/stb/stb_ds.h"

#include "naui/utils/list.h"
#include "naui/utils/arena.h"
#include "naui/utils/string.h"
#include "naui/core/log.h"
#include "naui/filesystem/filesystem.h"

#include "naui/utils/arena.c"
#include "naui/utils/string.c"
#include "naui/core/log.c"
#include "naui/filesystem/filesystem_win32.c"
#include "naui/filesystem/filesystem_unix.c"

#define APP "NauiApp"
#define SRC "app/build.c"
static char *out;

#define cmd_append(cmd, s) naui_sb_append_string(cmd, naui_string_lit(s" "));
#define cmd_append_string(cmd, s) naui_sb_append_string(cmd, s);

typedef enum {
    COMPILE_MODE_DEBUG,
    COMPILE_MODE_RELEASE
} CompileMode;

void cmd_cc(Naui_StringBuilder cmd) {
    cmd_append(cmd, "clang");
}

void cmd_flags(Naui_StringBuilder cmd) {
#if NAUI_WINDOWS
    cmd_append(cmd, "-DMGFX_D3D11");
#else
    cmd_append(cmd, "-DMGFX_OPENGL");
#endif
    cmd_append(cmd, "-I. -Inaui/vendor -Iapp");
}

void cmd_links(Naui_StringBuilder cmd) {
#if NAUI_WINDOWS
    cmd_append(cmd, "-lshell32 -luser32 -ldxgi -ld3d11 -ld3dcompiler -ldxguid");
#elif NAUI_LINUX
    cmd_append(cmd, "-lX11 -lEGL -lm");
#endif
}

int cmd_compile(Naui_Arena *arena, Naui_StringBuilder cmd, CompileMode compile_mode) {
    if (!naui_path_exists(NAUI_PATH("bin"))) {
        naui_log(NAUI_LOG_INFO, "created directory 'bin'");
        naui_directory_create(NAUI_PATH("bin"));
    }
    const Naui_Path path = (compile_mode == COMPILE_MODE_DEBUG) ? NAUI_PATH("bin/Debug") : NAUI_PATH("bin/Release");
    if (!naui_path_exists(path)) {
        naui_log(NAUI_LOG_INFO, "created directory '%.*s'", path.length, path.data);
        naui_directory_create(path);
    }

    cmd_cc(cmd);
    cmd_flags(cmd);
    cmd_append(cmd, SRC" -o ");
    cmd_append_string(cmd, naui_string_from_cstring(out));
    cmd_append(cmd, " ");
    cmd_links(cmd);

    char *cmd_cstring = naui_string_clone_to_cstring(arena, naui_sb_to_string(cmd));
    naui_log(NAUI_LOG_INFO, "running compile command: %s", cmd_cstring);
    int return_code = system(cmd_cstring);
    if (return_code == 0)
        naui_log(NAUI_LOG_INFO, "compilation for '%s' succeeded with return code 0", out);
    else
        naui_log(NAUI_LOG_ERROR, "compilation for '%s' failed with return code %i", out, return_code);
    return return_code;
}

int cmd_run_target(Naui_Arena *arena, Naui_StringBuilder cmd) {
    int return_code;

    Naui_FileHandle out_file;
    bool out_file_exists = naui_file_open(&out_file, NAUI_PATH(out), NAUI_FILE_READ);

    if (!out_file_exists) {
        naui_log(NAUI_LOG_ERROR, "target '%s' does not exist", out);
        return -1;
    }
    naui_log(NAUI_LOG_INFO, "running target '%s'...\n", out);
    return_code = system(out);
    puts("");
    if (return_code == 0)
        naui_log(NAUI_LOG_INFO, "target '%s' exited successfully with return code 0", out);
    else
        naui_log(NAUI_LOG_ERROR, "target '%s' exited with return code %i", out, return_code);

    return return_code;
}

int cmd_clean(Naui_Arena *arena, Naui_StringBuilder cmd) {
    if (naui_path_exists(NAUI_PATH("bin"))) {
        naui_log(NAUI_LOG_INFO, "found 'bin' directory, deleting...");
        naui_directory_remove_all(NAUI_PATH("bin"));
        naui_log(NAUI_LOG_INFO, "cleaning successful");
    } else
        naui_log(NAUI_LOG_INFO, "there is nothing to clean...");
    return 0;
}

int main(int argc, char **argv) {
    Naui_Arena arena = {0};
    Naui_StringBuilder cmd = naui_sb_create();

#if NAUI_WINDOWS
    out = "bin\\Debug\\"APP".exe";
#else
    out = "bin/Debug/"APP;
#endif

    int return_code;
    if (argc == 1) {
        fprintf(stderr, "build-cli: simple build utility for naui and app (very WIP)\n"
                "usage: ./build-cli [commands]\n"
                "arguments:\n"
                "  no arguments or compile: compiles project (debug)\n"
                "  release: compiles with release mode\n"
                "  run: runs compiled project\n");
        return 0;
    }
    else {
        for (int i = 1; i < argc; i++) {
            if (naui_cstr_strcmp(argv[i], "compile", true) == 0) return_code = cmd_compile(&arena, cmd, COMPILE_MODE_DEBUG);
            else if (naui_cstr_strcmp(argv[i], "run", true) == 0) return_code = cmd_run_target(&arena, cmd);
            else if (naui_cstr_strcmp(argv[i], "clean", true) == 0) return_code = cmd_clean(&arena, cmd);
            else if (naui_cstr_strcmp(argv[i], "release", true) == 0) {
#if NAUI_WINDOWS
                out = "bin\\Release\\"APP".exe";
#else
                out = "bin/Release/"APP;
#endif
                return_code = cmd_compile(&arena, cmd, COMPILE_MODE_RELEASE);
            }
            else naui_log(NAUI_LOG_ERROR, "unknown command '%s'", argv[i]);
        }
    }

    naui_sb_destroy(cmd);
    return return_code;
}
