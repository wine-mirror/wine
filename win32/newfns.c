/*
 * Win32 miscellaneous functions
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 */

/* Misc. new functions - they should be moved into appropriate files
at a later date. */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "stddebug.h"
#include "debug.h"

/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.415)
 */
BOOL32
QueryPerformanceCounter(LPLARGE_INTEGER counter) {
	/* FIXME: don't know what are good values */
	counter->LowPart	= 1;
	counter->HighPart	= 0;
	return TRUE;
}

DWORD
GetWindowThreadProcessId(HWND32 hwnd,LPDWORD processid) {
	fprintf(stdnimp,"GetWindowThreadProcessId(%04x,%p),stub\n",hwnd,processid);
	return 0;
}

/****************************************************************************
 *		DisableThreadLibraryCalls (KERNEL32.74)
 */
BOOL32
DisableThreadLibraryCalls(HMODULE32 hModule) {
	/* FIXME: stub for now */
    fprintf(stdnimp, "DisableThreadLibraryCalls Stub called!\n");
    return TRUE;
}

