#!/bin/bash
set -e

./build.sh
mkdir -p "$PWD/prefix"
WINEPREFIX="$PWD/prefix" ./build/64/wine /home/onni/Downloads/XboxAppPackage_2408.1001.14.0_x64/XboxPcApp.exe
