@echo off

set cc_flags=-DMGFX_D3D11 -I. -Inaui/vendor -Iapp
set ld_flags=-lshell32 -luser32 -ldxgi -ld3d11 -ld3dcompiler -ldxguid

set profile=Debug

clang %cc_flags% app/main.c -o build/%profile%/NauiApp.exe %ld_flags%
