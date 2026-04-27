@echo off

set PROJECT=NauiApp

set CONFIG=Debug
if /I "%~1"=="-release" (
    set CONFIG=Release
    shift
) else if /I "%~1"=="-r" (
    set CONFIG=Release
    shift
)

set SHD_PATH="naui/renderer/shaders"

premake5 vs2022
.\sokol-shdc.exe -i "%SHD_PATH%/basic.glsl" -o "%SHD_PATH%/basic.h" -l glsl410:hlsl4:metal_macos
msbuild "%PROJECT%.sln" /p:Configuration=%CONFIG%
