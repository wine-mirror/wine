/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "advapi32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           GetUserNameA   (ADVAPI32.67)
 */

BOOL WINAPI GetUserNameA(LPSTR lpszName, LPDWORD lpSize)
{
  size_t len;
  char *name;

  name=getlogin();
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
 *           RegCreateKeyEx   (ADVAPI32.130)
 */
LONG RegCreateKey(HKEY,LPCTSTR,LPHKEY);
WINAPI LONG RegCreateKeyEx(HKEY key,
                            const char *subkey,
                            long dontuse,
                            const char *keyclass,
                            DWORD options,
                            REGSAM sam,
                            SECURITY_ATTRIBUTES *atts,
                            HKEY *res,
                            DWORD *disp)
{
	/* ahum */
	return RegCreateKey(key, subkey, res);
}

/***********************************************************************
 *           RegSetValueEx   (ADVAPI32.169)
 */

WINAPI LONG RegSetValueEx (HKEY key,
                const char *name,
                DWORD dontuse,
                DWORD type,
                const void* data,
                DWORD len
                )
{
	return 0;
}

/***********************************************************************
 *           RegQueryValueEx   (ADVAPI32.157)
 */

WINAPI LONG RegQueryValueEx(HKEY key,
                             const char *subkey,
                             DWORD dontuse,
                             DWORD *type,
                             void *ptr,
                             DWORD *len)
{
	return 0;
}

