/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "handle32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           ExitProcess   (KERNEL32.100)
 */

void ExitProcess(DWORD status)
{
        exit(status);
}

/***********************************************************************
 *           CreateMutexA    (KERNEL32.52)
 */
WINAPI HANDLE32 CreateMutexA (SECURITY_ATTRIBUTES *sa, BOOL on, const char *a)
{
	return 0;
}

/***********************************************************************
 *           ReleaseMutex    (KERNEL32.435)
 */
WINAPI BOOL ReleaseMutex (HANDLE32 h)
{
	return 0;
}

/***********************************************************************
 *           CreateEventA    (KERNEL32.43)
 */
WINAPI HANDLE32 CreateEventA (SECURITY_ATTRIBUTES *sa, BOOL au, BOOL on, const char
*name)
{
	return 0;
}
/***********************************************************************
 *           SetEvent    (KERNEL32.487)
 */
WINAPI BOOL SetEvent (HANDLE32 h)
{
	return 0;
}
/***********************************************************************
 *           ResetEvent    (KERNEL32.439)
 */
WINAPI BOOL ResetEvent (HANDLE32 h)
{
	return 0;
}
/***********************************************************************
 *           WaitForSingleObject    (KERNEL32.561)
 */
DWORD WINAPI WaitForSingleObject(HANDLE32 h, DWORD a)
{
	return 0;
}
/***********************************************************************
 *           DuplicateHandle    (KERNEL32.78)
 */
BOOL WINAPI DuplicateHandle(HANDLE32 a, HANDLE32 b, HANDLE32 c, HANDLE32 * d, DWORD e, BOOL f, DWORD g)
{
	*d = b;
	return 1;
}
/***********************************************************************
 *           GetCurrentProcess    (KERNEL32.198)
 */
HANDLE32 WINAPI GetCurrentProcess(void)
{
	return 0;
}
