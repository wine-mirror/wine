/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include "windows.h"
#include "winerror.h"
#include "debug.h"
#include "heap.h"

#include <unistd.h>

/***********************************************************************
 *           GetUserNameA   [ADVAPI32.67]
 */
BOOL32 WINAPI GetUserName32A(LPSTR lpszName, LPDWORD lpSize)
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

/***********************************************************************
 *           GetUserNameW   [ADVAPI32.68]
 */
BOOL32 WINAPI GetUserName32W(LPWSTR lpszName, LPDWORD lpSize)
{
	LPSTR name = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *lpSize );
	DWORD	size = *lpSize;
	BOOL32 res = GetUserName32A(name,lpSize);

	lstrcpynAtoW(lpszName,name,size);
        HeapFree( GetProcessHeap(), 0, name );
	return res;
}
