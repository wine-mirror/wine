/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include "winbase.h"
#include "windef.h"
#include "winnls.h"
#include "winerror.h"

#include "debugtools.h"


/******************************************************************************
 * GetUserNameA [ADVAPI32.67]
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
  size_t len;
  char *name;

  struct passwd *pwd = getpwuid( getuid() );
  if (!pwd) return 0;
  name = pwd->pw_name;
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
 * GetUserNameW [ADVAPI32.68]
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

        /* FIXME: should set lpSize in WCHARs */
        if (size && !MultiByteToWideChar( CP_ACP, 0, name, -1, lpszName, size ))
            lpszName[size-1] = 0;
        HeapFree( GetProcessHeap(), 0, name );
	return res;
}
