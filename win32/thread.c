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
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

/**********************************************************************
 *          Critical Sections are currently ignored
 */
void InitializeCriticalSection(CRITICAL_SECTION *lpCrit)
{
	memset(lpCrit,0,sizeof(CRITICAL_SECTION));
}

void EnterCriticalSection(CRITICAL_SECTION* lpCrit)
{
    if (lpCrit->LockCount)
        fprintf( stderr, "Error: re-entering critical section %08lx\n",
                 (DWORD)lpCrit );
    lpCrit->LockCount++;
}

void LeaveCriticalSection(CRITICAL_SECTION* lpCrit)
{
    if (!lpCrit->LockCount)
        fprintf( stderr, "Error: leaving critical section %08lx again\n",
                 (DWORD)lpCrit );
    lpCrit->LockCount--;
}

void DeleteCriticalSection(CRITICAL_SECTION* lpCrit)
{
	return;
}

void ReinitializeCriticalSection(CRITICAL_SECTION *lpCrit) {
	/* hmm */
}

void MakeCriticalSectionGlobal(CRITICAL_SECTION *lpCrit) {
	/* hmm */
}


/* FIXME: This is required to work cross-addres space as well */
static CRITICAL_SECTION interlocked;
static int interlocked_init;

static void get_interlocked()
{
	if(!interlocked_init)
		InitializeCriticalSection(&interlocked);
	interlocked_init=1;
	EnterCriticalSection(&interlocked);
}

static void release_interlocked()
{
	LeaveCriticalSection(&interlocked);
}

/***********************************************************************
 *           InterlockedIncrement
 */
LONG InterlockedIncrement(LPLONG lpAddend)
{
	int ret;
	get_interlocked();
	(*lpAddend)++;
	ret=*lpAddend;
	release_interlocked();
	return ret;
}

/***********************************************************************
 *           InterlockedDecrement
 */
LONG InterlockedDecrement(LPLONG lpAddend)
{
	int ret;
	get_interlocked();
	(*lpAddend)--;
	ret=*lpAddend;
	release_interlocked();
	return ret;
}

/***********************************************************************
 *           InterlockedExchange
 */
LONG InterlockedExchange(LPLONG target, LONG value)
{
	int ret;
	get_interlocked();
	ret=*target;
	*target=value;
	release_interlocked();
	return ret;
}
