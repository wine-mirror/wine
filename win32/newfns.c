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
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *          WinHelpA           (USER32.578)
 */
BOOL WIN32_WinHelpA(HWND hWnd,LPCSTR lpszHelp,UINT uCommand, DWORD dwData)
{
	/* Should do parameter conversion here, but WinHelp is not working,
	   anyways */
	return WinHelp(hWnd,lpszHelp,uCommand,dwData);
}

HHOOK SetWindowsHookEx32A(int HookId, HOOKPROC32 hookfn, HINSTANCE hModule,
				DWORD ThreadId)

{
	/* Stub for now */
	fprintf(stdnimp, "WIN32_SetWindowsHookEx Stub called!\n");
	
	return (HHOOK) NULL;
}
