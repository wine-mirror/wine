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

/*************************************************************************
 * lineRedirect32 [TAPI32.53]
 * 
 */
LONG WINAPI lineRedirect32(
  HANDLE32* hCall,
  LPCSTR lpszDestAddress,
  DWORD  dwCountryCode) {

  FIXME(comm, ": stub.\n");
  return -1;
/*  return LINEERR_OPERATIONFAILED; */
}

/*************************************************************************
 * tapiRequestMakeCall32 [TAPI32.113]
 * 
 */
LONG WINAPI tapiRequestMakeCall32(
  LPCSTR lpszDestAddress,
  LPCSTR lpszAppName,
  LPCSTR lpszCalledParty,
  LPCSTR lpszComment) {

  FIXME(comm, ": stub.\n");
  return -1;
/*  return TAPIERR_REQUESTQUEUEFULL; */
}





