/*
 * Win32 critical sections
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include "winerror.h"
#include "winbase.h"
#include "ntddk.h"
#include "heap.h"
#include "debugtools.h"
#include "thread.h"

DEFAULT_DEBUG_CHANNEL(win32)
DECLARE_DEBUG_CHANNEL(relay)


/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.472) (NTDLL.406)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = CreateSemaphoreA( NULL, 0, 1, NULL );
    crit->Reserved       = GetCurrentProcessId();
}


/***********************************************************************
 *           DeleteCriticalSection   (KERNEL32.185) (NTDLL.327)
 */
void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->LockSemaphore)
    {
        if (crit->RecursionCount)  /* Should not happen */
            ERR_(win32)("Deleting owned critical section (%p)\n", crit );

        crit->LockCount      = -1;
        crit->RecursionCount = 0;
        crit->OwningThread   = 0;
        CloseHandle( crit->LockSemaphore );
        crit->LockSemaphore  = 0;
        crit->Reserved       = (DWORD)-1;
    }
}


/***********************************************************************
 *           EnterCriticalSection   (KERNEL32.195) (NTDLL.344)
 */
void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
    DWORD res;

    if (!crit->LockSemaphore)
    {
    	FIXME_(win32)("entering uninitialized section(%p)?\n",crit);
    	InitializeCriticalSection(crit);
    }
    if ( crit->Reserved && crit->Reserved != GetCurrentProcessId() )
    {
	FIXME_(win32)("Crst %p belongs to process %ld, current is %ld!\n", 
		      crit, crit->Reserved, GetCurrentProcessId() );
	return;
    }
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return;
        }

        /* Now wait for it */
        for (;;)
        {
            EXCEPTION_RECORD rec;

            res = WaitForSingleObject( crit->LockSemaphore, 5000L );
            if ( res == WAIT_TIMEOUT )
            {
                ERR_(win32)("Critical section %p wait timed out, retrying (60 sec)\n", crit );
                res = WaitForSingleObject( crit->LockSemaphore, 60000L );
                if ( res == WAIT_TIMEOUT && TRACE_ON(relay) )
                {
                    ERR_(win32)("Critical section %p wait timed out, retrying (5 min)\n", crit );
                    res = WaitForSingleObject( crit->LockSemaphore, 300000L );
                }
            }
            if (res == STATUS_WAIT_0) break;

            rec.ExceptionCode    = EXCEPTION_CRITICAL_SECTION_WAIT;
            rec.ExceptionFlags   = 0;
            rec.ExceptionRecord  = NULL;
            rec.ExceptionAddress = RtlRaiseException;  /* sic */
            rec.NumberParameters = 1;
            rec.ExceptionInformation[0] = (DWORD)crit;
            RtlRaiseException( &rec );
        }
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
    crit->Reserved      = 0L;
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.581)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    if ( !crit->LockSemaphore )
        InitializeCriticalSection( crit );

    else if ( crit->Reserved && crit->Reserved != GetCurrentProcessId() )
    {
        FIXME_(win32)("(%p) called for %08lx first, %08lx now: making global\n", 
               crit, crit->Reserved, GetCurrentProcessId() );

        MakeCriticalSectionGlobal( crit );
    }
}


/***********************************************************************
 *           UninitializeCriticalSection   (KERNEL32.703)
 */
void WINAPI UninitializeCriticalSection( CRITICAL_SECTION *crit )
{
    if ( crit->LockSemaphore )
    {
        if ( crit->Reserved )  /* not global */
            DeleteCriticalSection( crit );
        else
            FIXME_(win32)("(%p) for %08lx: Crst is global, don't know whether to delete\n", 
                   crit, GetCurrentProcessId() );
    }
}

