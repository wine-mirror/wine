/*
 * W32SKRNL
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include "windows.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

LPSTR WINAPI GetWin32sDirectory(void)
{
    static char *sysdir;
    LPSTR text;

    sysdir = getenv("winsysdir");
    if (!sysdir) return NULL;
    strcat(sysdir, "\\WIN32S");
    text = HeapAlloc(GetProcessHeap(), 0, strlen(sysdir)+1);
    strcpy(text, sysdir);
    return text; 
}

/* FIXME */
SEGPTR WINAPI _GetThunkBuff(void)
{
	return (SEGPTR)NULL;
}
