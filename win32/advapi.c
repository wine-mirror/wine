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
 *           GetUserNameA   [ADVAPI32.67]
 */

BOOL GetUserNameA(LPSTR lpszName, LPDWORD lpSize)
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
