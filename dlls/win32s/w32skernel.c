/*
 * W32SKRNL
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <string.h>

#include "winbase.h"
#include "wine/windef16.h"
#include "thread.h"

/***********************************************************************
 *		GetWin32sDirectory (W32SKRNL.14)
 */
LPSTR WINAPI GetWin32sDirectory(void)
{
    static char sysdir[0x80];
    LPSTR text;

    GetEnvironmentVariableA("winsysdir", sysdir, 0x80);
    if (!sysdir) return NULL;
    strcat(sysdir, "\\WIN32S");
    text = HeapAlloc(GetProcessHeap(), 0, strlen(sysdir)+1);
    strcpy(text, sysdir);
    return text; 
}

/***********************************************************************
 *		_GetThunkBuff
 * FIXME: ???
 */
SEGPTR WINAPI _GetThunkBuff(void)
{
	return (SEGPTR)NULL;
}


/***********************************************************************
 *           GetCurrentTask32   (W32SKRNL.3)
 */
HTASK16 WINAPI GetCurrentTask32(void)
{
    return NtCurrentTeb()->htask16;
}

