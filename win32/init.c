/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * 1999 Peter Ganten
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "winnls.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/exception.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);
  
/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *              GetComputerNameA         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameA(LPSTR name,LPDWORD size)
{
    /* At least Win95OSR2 survives if size is not a pointer (NT crashes though) */
    BOOL ret;
    __TRY
    {
      ret = (-1!=gethostname(name,*size));
      if (ret)
	*size = strlen(name);
    }
    __EXCEPT(page_fault)
    {
      SetLastError( ERROR_INVALID_PARAMETER );
      return FALSE;
    }
    __ENDTRY
     
    return ret;
}

/***********************************************************************
 *              GetComputerNameW         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameW(LPWSTR name,LPDWORD size)
{
    LPSTR nameA = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *size);
    BOOL ret = GetComputerNameA(nameA,size);
    /* FIXME: should set *size in Unicode chars */
    if (ret) MultiByteToWideChar( CP_ACP, 0, nameA, -1, name, *size+1 );
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

