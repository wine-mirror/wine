/*
 *      SETUPX library
 *
 *      Copyright 1998  Andreas Mohr
 *
 * FIXME: Rather non-functional functions for now.
 */

#include "windows.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(setupx);

/***********************************************************************
 *		SURegOpenKey
 */
DWORD WINAPI SURegOpenKey( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    FIXME("(%x,%s,%p), semi-stub.\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegOpenKeyA( hkey, lpszSubKey, retkey );
}

/***********************************************************************
 *		SURegQueryValueEx
 */
DWORD WINAPI SURegQueryValueEx( HKEY hkey, LPSTR lpszValueName,
                                LPDWORD lpdwReserved, LPDWORD lpdwType,
                                LPBYTE lpbData, LPDWORD lpcbData )
{
    FIXME("(%x,%s,%p,%p,%p,%ld), semi-stub.\n",hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData?*lpcbData:0);
    return RegQueryValueExA( hkey, lpszValueName, lpdwReserved, lpdwType,
                               lpbData, lpcbData );
}

/*
 * hwnd = parent window
 * hinst = instance of SETUPX.DLL
 * lpszCmdLine = e.g. "DefaultInstall 132 C:\MYINSTALL\MYDEV.INF"
 * Here "DefaultInstall" is the .inf file section to be installed (optional).
 * 132 is the standard parameter, it seems.
 * 133 means don't prompt user for reboot.
 * 
 * nCmdShow = nCmdShow of CreateProcess
 * FIXME: is the return type correct ?
 */
DWORD WINAPI InstallHinfSection16( HWND16 hwnd, HINSTANCE16 hinst, LPCSTR lpszCmdLine, INT16 nCmdShow)
{
	FIXME("(%04x, %04x, %s, %d), stub.\n", hwnd, hinst, lpszCmdLine, nCmdShow);
	return 0;
}
