/*
 * W32SKRNL
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <string.h>

#include "wine/w32skrnl.h"
#include "winbase.h"

/***********************************************************************
 *		GetWin32sDirectory
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
