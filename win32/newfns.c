/*
 * Win32 miscellaneous functions
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 */

/* Misc. new functions - they should be moved into appropriate files
at a later date. */

#include <stdio.h>
#include "module.h"
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           RaiseException		(KERNEL32.??)
 *
 * Stub function - does not allow exceptions to be caught yet
 */
WINAPI VOID RaiseException(DWORD dwExceptionCode,
		    DWORD dwExceptionFlags,
		    DWORD cArguments,
		    const DWORD * lpArguments)
{
    ExitProcess(dwExceptionCode); /* what status should be used here ? */
}

/***********************************************************************
 *           GetProcAddress		(KERNEL32.257)
 *
 */
 /* FIXME: This is currently not used, see WIN32_GetProcAddress */
WINAPI FARPROC W32_GetProcAddress(HMODULE hModule,
			      LPCSTR lpszProc)
{
    char *modulename;

    modulename = MODULE_GetModuleName(hModule);
    if (modulename == NULL)
	return (FARPROC) NULL;
    if (((int) lpszProc) & 0xffff) 
	return RELAY32_GetEntryPoint(modulename, lpszProc, 0);
    else
	return RELAY32_GetEntryPoint(modulename, (char *) NULL, (int) lpszProc);
}

/***********************************************************************
 *          WinHelpA           (USER32.578)
 */
BOOL WIN32_WinHelpA(HWND hWnd,LPCSTR lpszHelp,UINT uCommand, DWORD dwData)
{
	/* Should do parameter conversion here, but WinHelp is not working,
	   anyways */
	return WinHelp(hWnd,lpszHelp,uCommand,dwData);
}
