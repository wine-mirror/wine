/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "handle32.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"

static HANDLE32 ProcessHeap = 0;  /* FIXME: should be in process database */

/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */

void ExitProcess(DWORD status)
{
    TASK_KillCurrentTask( status );
}

/***********************************************************************
 *           CreateMutexA    (KERNEL32.52)
 */
HANDLE32 CreateMutexA (SECURITY_ATTRIBUTES *sa, BOOL on, const char *a)
{
	return 0;
}

/***********************************************************************
 *           ReleaseMutex    (KERNEL32.435)
 */
BOOL ReleaseMutex (HANDLE32 h)
{
	return 0;
}

/***********************************************************************
 *           CreateEventA    (KERNEL32.43)
 */
HANDLE32 CreateEventA (SECURITY_ATTRIBUTES *sa, BOOL au, BOOL on, const char
*name)
{
	return 0;
}
/***********************************************************************
 *           SetEvent    (KERNEL32.487)
 */
BOOL SetEvent (HANDLE32 h)
{
	return 0;
}
/***********************************************************************
 *           ResetEvent    (KERNEL32.439)
 */
BOOL ResetEvent (HANDLE32 h)
{
	return 0;
}
/***********************************************************************
 *           WaitForSingleObject    (KERNEL32.561)
 */
DWORD WaitForSingleObject(HANDLE32 h, DWORD a)
{
	return 0;
}
/***********************************************************************
 *           DuplicateHandle    (KERNEL32.78)
 */
BOOL DuplicateHandle(HANDLE32 a, HANDLE32 b, HANDLE32 c, HANDLE32 * d, DWORD e, BOOL f, DWORD g)
{
	*d = b;
	return 1;
}
/***********************************************************************
 *           GetCurrentProcess    (KERNEL32.198)
 */
HANDLE32 GetCurrentProcess(void)
{
	return 0;
}

/***********************************************************************
 *           GetProcessHeap    (KERNEL32.259)
 */
HANDLE32 GetProcessHeap(void)
{
    if (!ProcessHeap) ProcessHeap = HeapCreate( 0, 0x10000, 0 );
    return ProcessHeap;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32.365)
 * copied from LoadLibrary
 * This does not currently support built-in libraries
 */
HANDLE32 LoadLibraryA(char *libname)
{
	HANDLE handle;
	dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);
	handle = LoadModule( libname, (LPVOID)-1 );
	if (handle == (HANDLE) -1)
	{
		char buffer[256];
		strcpy( buffer, libname );
		strcat( buffer, ".dll" );
		handle = LoadModule( buffer, (LPVOID)-1 );
	}
	/* Obtain module handle and call initialization function */
#ifndef WINELIB
	if (handle >= (HANDLE)32) PE_InitializeDLLs( GetExePtr(handle));
#endif
	return handle;
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL FreeLibrary32(HINSTANCE hLibModule)
{
	fprintf(stderr,"FreeLibrary: empty stub\n");
	return TRUE;
}

/**********************************************************************
 *          GetProcessAffinityMask
 */
BOOL GetProcessAffinityMask(HANDLE32 hProcess, LPDWORD lpProcessAffinityMask,
  LPDWORD lpSystemAffinityMask)
{
	dprintf_task(stddeb,"GetProcessAffinityMask(%x,%x,%x)\n",
		hProcess,(lpProcessAffinityMask?*lpProcessAffinityMask:0),
		(lpSystemAffinityMask?*lpSystemAffinityMask:0));
	/* It is definitely important for a process to know on what processor
	   it is running :-) */
	if(lpProcessAffinityMask)
		*lpProcessAffinityMask=1;
	if(lpSystemAffinityMask)
		*lpSystemAffinityMask=1;
	return TRUE;
}

/**********************************************************************
 *           SetThreadAffinityMask
 */
BOOL SetThreadAffinityMask(HANDLE32 hThread, DWORD dwThreadAffinityMask)
{
	dprintf_task(stddeb,"SetThreadAffinityMask(%x,%x)\n",hThread,
		dwThreadAffinityMask);
	/* FIXME: We let it fail */
	return 1;
}
