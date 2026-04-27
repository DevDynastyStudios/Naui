#!/bin/sh

project=NauiApp
build_path=build
config=Debug

if [ "$1" = "-release" ]; then
    config=Release
elif [ "$1" = "-r" ]; then
    config=Release
fi

cd $build_path/$config
./$project "$@"
cd ..
