/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * 1999 Peter Ganten
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "winerror.h"
#include "wine/winestring.h"
#include "wine/exception.h"
#include "heap.h"
#include "task.h"
#include "debugtools.h"
#include "process.h"

DEFAULT_DEBUG_CHANNEL(win32);
  
/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.273)
 */
VOID WINAPI GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
  
  LPSTARTUPINFOA  startup;

  startup = ((LPSTARTUPINFOA )PROCESS_Current()->env_db->startup_info);
  memcpy ( lpStartupInfo, startup, sizeof (STARTUPINFOA) );

  TRACE("size: %ld\n"
	  "\tlpReserverd: %s, lpDesktop: %s, lpTitle: %s\n"
	  "\tdwX: %ld, dwY: %ld, dwXSize: %ld, dwYSize: %ld\n"
	  "\tdwFlags: %lx, wShowWindow: %x\n", lpStartupInfo->cb, 
	  debugstr_a(lpStartupInfo->lpReserved),
	  debugstr_a(lpStartupInfo->lpDesktop), 
	  debugstr_a(lpStartupInfo->lpTitle),
	  lpStartupInfo->dwX, lpStartupInfo->dwY,
	  lpStartupInfo->dwXSize, lpStartupInfo->dwYSize,
	  lpStartupInfo->dwFlags, lpStartupInfo->wShowWindow );
}

/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.274)
 */
VOID WINAPI GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo)
{

  STARTUPINFOA startup;
  GetStartupInfoA ( &startup );

  lpStartupInfo->cb = sizeof(STARTUPINFOW);
  lpStartupInfo->lpReserved = 
    HEAP_strdupAtoW (GetProcessHeap(), 0, startup.lpReserved );
  lpStartupInfo->lpDesktop = 
    HEAP_strdupAtoW (GetProcessHeap(), 0, startup.lpDesktop );
  lpStartupInfo->lpTitle = 
    HEAP_strdupAtoW (GetProcessHeap(), 0, startup.lpTitle );
  lpStartupInfo->dwX = startup.dwX;
  lpStartupInfo->dwY = startup.dwY;
  lpStartupInfo->dwXSize = startup.dwXSize;
  lpStartupInfo->dwXCountChars = startup.dwXCountChars;
  lpStartupInfo->dwYCountChars = startup.dwYCountChars;
  lpStartupInfo->dwFillAttribute = startup.dwFillAttribute;
  lpStartupInfo->dwFlags = startup.dwFlags;
  lpStartupInfo->wShowWindow = startup.wShowWindow;
  lpStartupInfo->cbReserved2 = startup.cbReserved2;
  lpStartupInfo->lpReserved2 = startup.lpReserved2;
  lpStartupInfo->hStdInput = startup.hStdInput;
  lpStartupInfo->hStdOutput = startup.hStdOutput;
  lpStartupInfo->hStdError = startup.hStdError;
}

/***********************************************************************
 *              GetComputerNameA         (KERNEL32.165)
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
 *              GetComputerNameW         (KERNEL32.166)
 */
BOOL WINAPI GetComputerNameW(LPWSTR name,LPDWORD size)
{
    LPSTR nameA = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *size);
    BOOL ret = GetComputerNameA(nameA,size);
    if (ret) lstrcpynAtoW(name,nameA,*size+1);
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

