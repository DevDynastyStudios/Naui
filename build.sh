cc_flags="-DMGFX_OPENGL -I. -Inaui/vendor -Iapp"
ld_flags="-lX11 -lEGL -lm"

profile="Debug"

clang $cc_flags app/main.c -o build/$profile/NauiApp $ld_flags
