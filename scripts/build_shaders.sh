#!/bin/sh

shd_path="naui/renderer/shaders"

./scripts/sokol-shdc -i "$shd_path/basic.glsl" -o "$shd_path/basic.h" -l glsl410:hlsl4:metal_macos
