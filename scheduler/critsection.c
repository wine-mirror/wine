/*
 * Win32 critical sections
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include "debug.h"
#include "winerror.h"
#include "winbase.h"
#include "heap.h"
#include "debug.h"
#include "thread.h"


/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.472) (NTDLL.406)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    crit->LockSemaphore  = CreateSemaphoreA( NULL, 0, 1, NULL );
    crit->Reserved       = (DWORD)-1;
}


/***********************************************************************
 *           DeleteCriticalSection   (KERNEL32.185) (NTDLL.327)
 */
void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->LockSemaphore)
    {
        if (crit->RecursionCount)  /* Should not happen */
            MSG("Deleting owned critical section (%p)\n", crit );
        crit->LockCount      = -1;
        crit->RecursionCount = 0;
        crit->OwningThread   = 0;
        CloseHandle( crit->LockSemaphore );
        crit->LockSemaphore  = 0;
    }
}


/***********************************************************************
 *           EnterCriticalSection   (KERNEL32.195) (NTDLL.344)
 */
void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (!crit->LockSemaphore)
    {
    	FIXME(win32,"entering uninitialized section(%p)?\n",crit);
    	InitializeCriticalSection(crit);
    }
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return;
        }
        /* Now wait for it */
        /* FIXME: should set a timeout and raise an exception */
        WaitForSingleObject( crit->LockSemaphore, INFINITE );
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
}


/***********************************************************************
 *           TryEnterCriticalSection   (KERNEL32.898) (NTDLL.969)
 */
BOOL WINAPI TryEnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return TRUE;
        }
        /* FIXME: this doesn't work */
        InterlockedDecrement( &crit->LockCount );
        return FALSE;
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
    return TRUE;
}


/***********************************************************************
 *           LeaveCriticalSection   (KERNEL32.494) (NTDLL.426)
 */
void WINAPI LeaveCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->OwningThread != GetCurrentThreadId()) return;
       
    if (--crit->RecursionCount)
    {
        InterlockedDecrement( &crit->LockCount );
        return;
    }
    crit->OwningThread = 0;
    if (InterlockedDecrement( &crit->LockCount ) >= 0)
    {
        /* Someone is waiting */
        ReleaseSemaphore( crit->LockSemaphore, 1, NULL );
    }
}


/***********************************************************************
 *           MakeCriticalSectionGlobal   (KERNEL32.515)
 */
void WINAPI MakeCriticalSectionGlobal( CRITICAL_SECTION *crit )
{
    crit->LockSemaphore = ConvertToGlobalHandle( crit->LockSemaphore );
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.581)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    DeleteCriticalSection( crit );
    InitializeCriticalSection( crit );
}


/***********************************************************************
 *           UninitializeCriticalSection   (KERNEL32.703)
 */
void WINAPI UninitializeCriticalSection( CRITICAL_SECTION *crit )
{
    FIXME(win32, "(%p) half a stub\n", crit);
    DeleteCriticalSection( crit );
}
