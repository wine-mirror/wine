/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "windows.h"
#include "winerror.h"
#include "except.h"
#include "heap.h"
#include "task.h"
#include "debug.h"
  
/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.273)
 */
VOID WINAPI GetStartupInfo32A(LPSTARTUPINFO32A lpStartupInfo)
{
    lpStartupInfo->cb = sizeof(STARTUPINFO32A);
    lpStartupInfo->lpReserved = "<Reserved>";
    lpStartupInfo->lpDesktop = "Desktop";
    lpStartupInfo->lpTitle = "Title";

    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->dwFlags    = STARTF_USESTDHANDLES;
    lpStartupInfo->hStdInput  = (HANDLE32)0;
    lpStartupInfo->hStdOutput = (HANDLE32)1;
    lpStartupInfo->hStdError  = (HANDLE32)2;
}

/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.274)
 */
VOID WINAPI GetStartupInfo32W(LPSTARTUPINFO32W lpStartupInfo)
{
    lpStartupInfo->cb = sizeof(STARTUPINFO32W);
    lpStartupInfo->lpReserved = HEAP_strdupAtoW(GetProcessHeap(),0,"<Reserved>");
    lpStartupInfo->lpDesktop = HEAP_strdupAtoW(GetProcessHeap(), 0, "Desktop");
    lpStartupInfo->lpTitle = HEAP_strdupAtoW(GetProcessHeap(), 0, "Title");

    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->hStdInput  = (HANDLE32)0;
    lpStartupInfo->hStdOutput = (HANDLE32)1;
    lpStartupInfo->hStdError  = (HANDLE32)2;
}

/***********************************************************************
 *              GetComputerNameA         (KERNEL32.165)
 */
BOOL32 WINAPI GetComputerName32A(LPSTR name,LPDWORD size)
{
	if (-1==gethostname(name,*size))
		return FALSE;
	*size = lstrlen32A(name);
	return TRUE;
}

/***********************************************************************
 *              GetComputerNameW         (KERNEL32.166)
 */
BOOL32 WINAPI GetComputerName32W(LPWSTR name,LPDWORD size)
{
    LPSTR nameA = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *size);
    BOOL32 ret = GetComputerName32A(nameA,size);
    if (ret) lstrcpynAtoW(name,nameA,*size+1);
    HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

