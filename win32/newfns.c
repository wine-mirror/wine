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

/****************************************************************************
 *		DisableThreadLibraryCalls (KERNEL32.74)
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL32
DisableThreadLibraryCalls(HMODULE32 hModule) {
    fprintf(stdnimp, "DisableThreadLibraryCalls Stub called!\n");
    return TRUE;
}

