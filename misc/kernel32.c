/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

/* This file contains only wrappers to existing Wine functions or trivial
   stubs. 'Real' implementations go into context specific files */

#include "windows.h"
#include "winerror.h"
#include <unistd.h>

int WIN32_LastError;

/***********************************************************************
 *           GetCommandLineA      (KERNEL32.161)
 */
LPSTR GetCommandLineA(void)
{
	return 0;
}

/***********************************************************************
 *           GetCurrentThreadId   (KERNEL32.200)
 */

int GetCurrentThreadId(void)
{
	return getpid();
}


/***********************************************************************
 *           GetEnvironmentStrings    (KERNEL32.210)
 */
LPSTR GetEnvironmentStrings(void)
{
	return 0;
}

/***********************************************************************
 *           GetStdHandle             (KERNEL32.276)
 */
HANDLE GetStdHandle(DWORD nStdHandle)
{
	switch(nStdHandle)
	{
		case -10/*STD_INPUT_HANDLE*/:return 0;
		case -11/*STD_OUTPUT_HANDLE*/:return 1;
		case -12/*STD_ERROR_HANDLE*/:return 2;
	}
	return -1;
}

/***********************************************************************
 *           GetThreadContext         (KERNEL32.294)
 */
BOOL GetThreadContext(HANDLE hThread, void *lpContext)
{
	return FALSE;
}
