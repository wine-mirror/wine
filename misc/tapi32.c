/*
 * TAPI32
 *
 * Copyright (c) 1998 Andreas Mohr
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "debug.h"

UINT32 WINAPI lineInitialize(
  LPVOID lphLineApp, /* FIXME */
  HINSTANCE32 hInstance,
  LPVOID lpfnCallback, /* FIXME */
  LPCSTR lpszAppName,
  LPDWORD lpdwNumDevs)
{
    FIXME(comm, "stub.\n");
    return 0;
}

UINT32 WINAPI lineShutdown( HANDLE32 hLineApp ) /* FIXME */
{
    FIXME(comm, "stub.\n");
    return 0;
}

UINT32 WINAPI lineNegotiateAPIVersion(
  HANDLE32 hLineApp, /* FIXME */
  DWORD dwDeviceID,
  DWORD dwAPILowVersion,
  DWORD dwAPIHighVersion,
  LPDWORD lpdwAPIVersion,
  LPVOID lpExtensionID /* FIXME */
)
{
    FIXME(comm, "stub.\n");
    *lpdwAPIVersion = dwAPIHighVersion;
    return 0;
}
