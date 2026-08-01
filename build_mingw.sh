cc_flags="-DMGFX_D3D11 -I. -Inaui/vendor -Iapp"
ld_flags="-luser32 -ldxgi -ld3d11 -ld3dcompiler -ldxguid"

profile="Debug"

x86_64-w64-mingw32-gcc $cc_flags app/main.c -o build/$profile/NauiApp.exe $ld_flags
