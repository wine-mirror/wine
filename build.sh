#!/bin/bash
set -e

# Begin with building dxvk-uwp
cd build/dxvki/w64
ninja install
cd ..
cd w32
ninja install
cd ../..

cd "64"
make -j24
cd ../

cd "32"
make -j24
cd ../..
