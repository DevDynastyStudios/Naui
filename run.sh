#!/bin/sh

project=NauiApp
config=Debug

if [ "$1" = "-release" ]; then
    config=Release
elif [ "$1" = "-r" ]; then
    config=Release
fi

./build/$config/$project "$@"
