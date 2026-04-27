#!/bin/sh

shd_path="naui/renderer/shaders"

premake5 gmake
./sokol-shdc -i "$shd_path/basic.glsl" -o "$shd_path/basic.h" -l glsl410:hlsl4:metal_macos
make -j4
