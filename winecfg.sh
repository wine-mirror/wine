#!/bin/bash
set -e

./build.sh
mkdir -p "$PWD/prefix"
WINEPREFIX="$PWD/prefix" ./build/64/wine winecfg
