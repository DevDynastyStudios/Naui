#!/bin/sh

premake5 gmake
./scripts/build_shaders.sh
make -j4
