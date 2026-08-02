#!/bin/bash
set -e

./build.sh
mkdir -p "$PWD/prefix" # winedbg --gdb --no-start
WINEPREFIX="$PWD/prefix" ./build/64/wine /home/onni/Downloads/GAMELOFTSA.Asphalt8Airborne_7.9.10.0_neutral_~_0pp20fcewvvtj/Asphalt8.exe
