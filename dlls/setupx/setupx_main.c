/*
 *      SETUPX library
 *
 *      Copyright 1998  Andreas Mohr
 *
 * FIXME: Rather non-functional functions for now.
 */

#include "windows.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(setupx)

DWORD WINAPI SURegOpenKey( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    FIXME("(%x,%s,%p), semi-stub.\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegOpenKeyA( hkey, lpszSubKey, retkey );
}

DWORD WINAPI SURegQueryValueEx( HKEY hkey, LPSTR lpszValueName,
                                LPDWORD lpdwReserved, LPDWORD lpdwType,
                                LPBYTE lpbData, LPDWORD lpcbData )
{
    FIXME("(%x,%s,%p,%p,%p,%ld), semi-stub.\n",hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData?*lpcbData:0);
    return RegQueryValueExA( hkey, lpszValueName, lpdwReserved, lpdwType,
                               lpbData, lpcbData );
}
