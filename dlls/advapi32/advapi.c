/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "wine/winestring.h"
#include "heap.h"

#include "debugtools.h"


/******************************************************************************
 * GetUserName32A [ADVAPI32.67]
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
  size_t len;
  char *name;

  name=getlogin();
#if 0
  /* FIXME: should use getpwuid() here */
  if (!name) name=cuserid(NULL);
#endif
  len = name ? strlen(name) : 0;
  if (!len || !lpSize || len > *lpSize) {
    if (lpszName) *lpszName = 0;
    return 0;
  }
  *lpSize=len;
  strcpy(lpszName, name);
  return 1;
}

/******************************************************************************
 * GetUserName32W [ADVAPI32.68]
 *
 * PARAMS
 *   lpszName []
 *   lpSize   []
 */
BOOL WINAPI
GetUserNameW( LPWSTR lpszName, LPDWORD lpSize )
{
	LPSTR name = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *lpSize );
	DWORD	size = *lpSize;
	BOOL res = GetUserNameA(name,lpSize);

	lstrcpynAtoW(lpszName,name,size);
        HeapFree( GetProcessHeap(), 0, name );
	return res;
}
