cc_flags="-DMGFX_OPENGL -I. -Inaui/vendor -Iapp"
ld_flags="-lX11 -lEGL -lm"

profile="Debug"

clang $cc_flags app/main.c -o bin/$profile/NauiApp $ld_flags
