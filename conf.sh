#!/bin/bash
rm -rf build/

mkdir build
mkdir build/32
mkdir build/64
mkdir build/dxvk
mkdir build/dxvki
mkdir build/dxvk/x64
mkdir build/dxvk/x32

./conf_dxvk.sh

cd build/64
../../configure CROSSDEBUG=pdb CC="ccache clang" --enable-win64 --with-mingw=clang
cd ../..

cd build/32
PKG_CONFIG_PATH=/usr/lib32 ../../configure CROSSDEBUG=pdb CC="ccache clang" --with-mingw=clang --with-wine64=../64
cd ../..
