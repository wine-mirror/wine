/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

/***********************************************************************
 *           GetCurrentThreadId   (KERNEL32.200)
 */

int GetCurrentThreadId(void)
{
        return getpid();
}

/***********************************************************************
 *           GetThreadContext         (KERNEL32.294)
 */
BOOL GetThreadContext(HANDLE hThread, void *lpContext)
{
        return FALSE;
}
/***********************************************************************
 *           GetCurrentThread    (KERNEL32.200)
 */
HANDLE WINAPI GetCurrentThread(void)
{
	return 0;
}

/**********************************************************************
 *          Critical Sections are currently ignored
 */
void WINAPI InitializeCriticalSection(CRITICAL_SECTION *lpCrit)
{
	memset(lpCrit,0,sizeof(CRITICAL_SECTION));
}

void WINAPI EnterCriticalSection(CRITICAL_SECTION* lpCrit)
{
    if (lpCrit->LockCount)
        fprintf( stderr, "Error: re-entering critical section %08lx\n",
                 (DWORD)lpCrit );
    lpCrit->LockCount++;
}

void WINAPI LeaveCriticalSection(CRITICAL_SECTION* lpCrit)
{
    if (!lpCrit->LockCount)
        fprintf( stderr, "Error: leaving critical section %08lx again\n",
                 (DWORD)lpCrit );
    lpCrit->LockCount--;
}

void WINAPI DeleteCriticalSection(CRITICAL_SECTION* lpCrit)
{
	return;
}

/***********************************************************************
 *           Tls is available only for the single thread
 */
static LPVOID* Tls=0;
static int TlsCount=0;

DWORD WINAPI TlsAlloc()
{
	if(!Tls){
		TlsCount++;
		Tls=xmalloc(sizeof(LPVOID));
		return 0;
	}
	Tls=xrealloc(Tls,sizeof(LPVOID)*(++TlsCount));
	return TlsCount-1;
}

void TlsFree(DWORD index)
{
	/*FIXME: should remember that it has been freed */
	return;
}

LPVOID TlsGetValue(DWORD index)
{
	if(index>=TlsCount)
	{
		/* FIXME: Set last error*/
		return 0;
	}
	return Tls[index];
}

void TlsSetValue(DWORD index,LPVOID value)
{
	if(index>=TlsCount)
	{
		/* FIXME: Set last error*/
		return;
	}
	Tls[index]=value;
}
