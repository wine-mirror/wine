#!/bin/bash
set -e

./build.sh
mkdir -p "$PWD/prefix" # winedbg --gdb --no-start
WINEPREFIX="$PWD/prefix" ./build/64/wine /home/onni/Downloads/MarbleMaze_VS2017/MarbleMaze_VS2017/AppX/MarbleMaze_VS2017.exe
