/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * 1999 Peter Ganten
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "winnls.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/exception.h"
#include "msvcrt/excpt.h"
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
	char host_name[256];
	TRACE("*size = %ld\n", *size);
	ret = (gethostname(host_name, sizeof(host_name)) != -1);
	if (ret)
	{
	    lstrcpynA(name, host_name, *size);
	    *size = strlen(name);
	}
	else
	    WARN("gethostname: %s\n", strerror(errno));
    }
    __EXCEPT(page_fault)
    {
      SetLastError( ERROR_INVALID_PARAMETER );
      return FALSE;
    }
    __ENDTRY

    TRACE("returning (%ld) %s\n", *size, debugstr_a(name));
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

/***********************************************************************
 *              GetComputerNameExA         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameExA(COMPUTER_NAME_FORMAT type, LPSTR name, LPDWORD size)
{
    FIXME("(%d, %p, %p) semi-stub!\n", type, name, size);
    return GetComputerNameA(name, size);
}

/***********************************************************************
 *              GetComputerNameExW         (KERNEL32.@)
 */
BOOL WINAPI GetComputerNameExW(COMPUTER_NAME_FORMAT type, LPWSTR name, LPDWORD size)
{
    FIXME("(%d, %p, %p) semi-stub!\n", type, name, size);
    return GetComputerNameW(name, size);
}
