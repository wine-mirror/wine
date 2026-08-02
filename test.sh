#!/bin/bash
set -e

./build.sh
mkdir -p "$PWD/prefix"
WINEPREFIX="$PWD/prefix" ./build/64/wine /home/onni/Downloads/Microsoft.MinecraftUWP_1.21.2101.0_x64__8wekyb3d8bbwe/Minecraft.Windows.exe
