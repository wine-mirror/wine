#!/bin/bash
set -e

./build.sh
export WINEPREFIX="$PWD/prefix"
export WINE="$PWD/build/64/wine"
mkdir -p "$WINEPREFIX"
cd "$WINEPREFIX/drive_c/Program Files/Microsoft Visual Studio 17.0/Common7/IDE/Remote Debugger/x64/"
WINEDLLOVERRIDES=webservices=n $WINE msvsmon.exe /noclrwarn /nowowwarn /nofirewallwarn /anyuser /noauth /nosecuritywarn
