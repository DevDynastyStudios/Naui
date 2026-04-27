@echo off

set SHD_PATH="naui/renderer/shaders"

.\scripts\sokol-shdc.exe -i "%SHD_PATH%/basic.glsl" -o "%SHD_PATH%/basic.h" -l glsl410:hlsl4:metal_macos
