#!/bin/bash
set -e

./build.sh
rm -rf "$PWD/prefix"
mkdir -p "$PWD/prefix"
export WINEPREFIX="$PWD/prefix"

# create wineprefix
# ./build/64/wine cmd /c ver

# default directories
dxvk_lib32=${dxvk_lib32:-"x32"}
dxvk_lib64=${dxvk_lib64:-"x64"}

basedir="$PWD/build/dxvk"

file_cmd="cp -v --reflink=auto"

# find wine executable
export WINEDEBUG=-all
# disable mscoree and mshtml to avoid downloading
# wine gecko and mono
export WINEDLLOVERRIDES="mscoree,mshtml="

wine="$PWD/build/64/wine"
wine64="$PWD/build/64/wine64"

# resolve 32-bit and 64-bit system32 path
winever=$($wine --version | grep wine)
if [ -z "$winever" ]; then
    echo "$wine:"' Not a wine executable. Check your $wine.' >&2
    exit 1
fi

# ensure wine placeholder dlls are recreated
# if they are missing
./build/64/wine wineboot -u

win64_sys_path=$($wine64 winepath -u 'C:\windows\system32' 2> /dev/null)
win64_sys_path="${win64_sys_path/$'\r'/}"
win32_sys_path=$($wine winepath -u 'C:\windows\system32' 2> /dev/null)
win32_sys_path="${win32_sys_path/$'\r'/}"

if [ -z "$win32_sys_path" ] && [ -z "$win64_sys_path" ]; then
  echo 'Failed to resolve C:\windows\system32.' >&2
  exit 1
fi

# create native dll override
overrideDll() {
  $wine reg add 'HKEY_CURRENT_USER\Software\Wine\DllOverrides' /v $1 /d native /f >/dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo -e "Failed to add override for $1"
    exit 1
  fi
}

# remove dll override
restoreDll() {
  $wine reg delete 'HKEY_CURRENT_USER\Software\Wine\DllOverrides' /v $1 /f > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo "Failed to remove override for $1"
  fi
}

# copy or link dxvk dll, back up original file
installFile() {
  dstfile="${1}/${3}.dll"
  srcfile="${basedir}/${2}/bin/${3}.dll"

  if [ -f "${srcfile}.so" ]; then
    srcfile="${srcfile}.so"
  fi

  if ! [ -f "${srcfile}" ]; then
    echo "${srcfile}: File not found. Skipping." >&2
    return 1
  fi

  if [ -n "$1" ]; then
    if [ -f "${dstfile}" ] || [ -h "${dstfile}" ]; then
      if ! [ -f "${dstfile}.old" ]; then
        mv -v "${dstfile}" "${dstfile}.old"
      else
        rm -v "${dstfile}"
      fi
      $file_cmd "${srcfile}" "${dstfile}"
    else
      echo "${dstfile}: File not found in wine prefix" >&2
      return 1
    fi
  fi
  return 0
}

install() {
  installFile "$win64_sys_path" "$dxvk_lib64" "$1"
  inst64_ret="$?"

  inst32_ret=-1
  installFile "$win32_sys_path" "$dxvk_lib32" "$1"
  inst32_ret="$?"

  if (( ($inst32_ret == 0) || ($inst64_ret == 0) )); then
    overrideDll "$1"
  fi
}

install d3d8
install d3d9
install d3d10core
install d3d11
install dxgi


# DEBUG version of pfx.sh, also setup the VS remote debugging utilities
./build/64/wine /home/onni/Downloads/VS_RemoteTools.exe
cp /mnt/windrive/Windows/SysWOW64/webservices.dll "$WINEPREFIX/drive_c/windows/syswow64/"
cp /mnt/windrive/Windows/System32/webservices.dll "$WINEPREFIX/drive_c/windows/system32/"
rm -rf "$WINEPREFIX/drive_c/Program Files/Common Files/Microsoft Shared/VS7Debug/"
rm -rf "$WINEPREFIX/drive_c/Program Files (x86)/Common Files/Microsoft Shared/VS7Debug/"

