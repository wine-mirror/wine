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
 *		UTRegister (KERNEL32.697)
 */
BOOL32 WINAPI UTRegister(HMODULE32 hModule,
                      LPSTR lpsz16BITDLL,
                      LPSTR lpszInitName,
                      LPSTR lpszProcName,
                      /*UT32PROC*/ LPVOID *ppfn32Thunk,
                      /*FARPROC*/ LPVOID pfnUT32CallBack,
                      LPVOID lpBuff)
{
    fprintf(stdnimp, "UTRegister Stub called!\n");
    return TRUE;
}

/****************************************************************************
 *		UTUnRegister (KERNEL32.698)
 */
BOOL32 WINAPI UTUnRegister(HMODULE32 hModule)
{
    fprintf(stdnimp, "UTUnRegister Stub called!\n");
    return TRUE;
}

/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.415)
 */
BOOL32 WINAPI QueryPerformanceCounter(LPLARGE_INTEGER counter)
{
	/* FIXME: don't know what are good values */
	counter->LowPart	= 1;
	counter->HighPart	= 0;
	return TRUE;
}
