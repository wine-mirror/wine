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
#include "heap.h"
#include "handle32.h"
#include "pe_image.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           CreateMutexA    (KERNEL32.52)
 */
HANDLE32 CreateMutexA (SECURITY_ATTRIBUTES *sa, BOOL32 on, const char *a)
{
	return 0;
}

/***********************************************************************
 *           ReleaseMutex    (KERNEL32.435)
 */
BOOL32 ReleaseMutex (HANDLE32 h)
{
	return 0;
}

/***********************************************************************
 *           CreateEventA    (KERNEL32.43)
 */
HANDLE32 CreateEventA (SECURITY_ATTRIBUTES *sa, BOOL32 au, BOOL32 on, const char
*name)
{
	return 0;
}
/***********************************************************************
 *           SetEvent    (KERNEL32.487)
 */
BOOL32 SetEvent (HANDLE32 h)
{
	return 0;
}
/***********************************************************************
 *           ResetEvent    (KERNEL32.439)
 */
BOOL32 ResetEvent (HANDLE32 h)
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
BOOL32 DuplicateHandle(HANDLE32 a, HANDLE32 b, HANDLE32 c, HANDLE32 * d, DWORD e, BOOL32 f, DWORD g)
{
	*d = b;
	return 1;
}


/***********************************************************************
 *           LoadLibraryA         (KERNEL32.365)
 * copied from LoadLibrary
 * This does not currently support built-in libraries
 */
HINSTANCE32 LoadLibrary32A(LPCSTR libname)
{
	HINSTANCE32 handle;
	dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);
	handle = LoadModule16( libname, (LPVOID)-1 );
	if (handle == (HINSTANCE32) -1)
	{
		char buffer[256];
		strcpy( buffer, libname );
		strcat( buffer, ".dll" );
		handle = LoadModule16( buffer, (LPVOID)-1 );
	}
	/* Obtain module handle and call initialization function */
#ifndef WINELIB
	if (handle >= (HINSTANCE32)32) PE_InitializeDLLs( GetExePtr(handle));
#endif
	return handle;
}

/***********************************************************************
 *           LoadLibrary32W         (KERNEL32.368)
 */
HINSTANCE32 LoadLibrary32W(LPCWSTR libnameW)
{
    LPSTR libnameA = HEAP_strdupWtoA( GetProcessHeap(), 0, libnameW );
    HINSTANCE32 ret = LoadLibrary32A( libnameA );
    HeapFree( GetProcessHeap(), 0, libnameA );
    return ret;
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL32 FreeLibrary32(HINSTANCE32 hLibModule)
{
	fprintf(stderr,"FreeLibrary: empty stub\n");
	return TRUE;
}

/**********************************************************************
 *          GetProcessAffinityMask
 */
BOOL32 GetProcessAffinityMask(HANDLE32 hProcess, LPDWORD lpProcessAffinityMask,
  LPDWORD lpSystemAffinityMask)
{
	dprintf_task(stddeb,"GetProcessAffinityMask(%x,%lx,%lx)\n",
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
BOOL32 SetThreadAffinityMask(HANDLE32 hThread, DWORD dwThreadAffinityMask)
{
	dprintf_task(stddeb,"SetThreadAffinityMask(%x,%lx)\n",hThread,
		dwThreadAffinityMask);
	/* FIXME: We let it fail */
	return 1;
}
