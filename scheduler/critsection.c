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

DEFAULT_DEBUG_CHANNEL(win32);
DECLARE_DEBUG_CHANNEL(relay);

/***********************************************************************
 *           get_semaphore
 */
static inline HANDLE get_semaphore( CRITICAL_SECTION *crit )
{
    HANDLE ret = crit->LockSemaphore;
    if (!ret)
    {
        HANDLE sem = CreateSemaphoreA( NULL, 0, 1, NULL );
        if (!(ret = (HANDLE)InterlockedCompareExchange( (PVOID *)&crit->LockSemaphore,
                                                        (PVOID)sem, 0 )))
            ret = sem;
        else
            CloseHandle(sem);  /* somebody beat us to it */
    }
    return ret;
}

/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.472) (NTDLL.406)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
}


/***********************************************************************
 *           DeleteCriticalSection   (KERNEL32.185) (NTDLL.327)
 */
void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->RecursionCount && crit->OwningThread != GetCurrentThreadId())
        ERR("Deleting owned critical section (%p)\n", crit );

    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    if (crit->LockSemaphore) CloseHandle( crit->LockSemaphore );
    crit->LockSemaphore  = 0;
}


/***********************************************************************
 *           EnterCriticalSection   (KERNEL32.195) (NTDLL.344)
 */
void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
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
            HANDLE sem = get_semaphore( crit );

            DWORD res = WaitForSingleObject( sem, 5000L );
            if ( res == WAIT_TIMEOUT )
            {
                ERR("Critical section %p wait timed out, retrying (60 sec)\n", crit );
                res = WaitForSingleObject( sem, 60000L );
                if ( res == WAIT_TIMEOUT && TRACE_ON(relay) )
                {
                    ERR("Critical section %p wait timed out, retrying (5 min)\n", crit );
                    res = WaitForSingleObject( sem, 300000L );
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
    BOOL ret = FALSE;
    if (InterlockedCompareExchange( (PVOID *)&crit->LockCount,
                                    (PVOID)0L, (PVOID)-1L ) == (PVOID)-1L)
    {
        crit->OwningThread   = GetCurrentThreadId();
        crit->RecursionCount = 1;
        ret = TRUE;
    }
    else if (crit->OwningThread == GetCurrentThreadId())
    {
	InterlockedIncrement( &crit->LockCount );
	crit->RecursionCount++;
	ret = TRUE;
    }
    return ret;
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
        HANDLE sem = get_semaphore( crit );
        ReleaseSemaphore( sem, 1, NULL );
    }
}


/***********************************************************************
 *           MakeCriticalSectionGlobal   (KERNEL32.515)
 */
void WINAPI MakeCriticalSectionGlobal( CRITICAL_SECTION *crit )
{
    crit->LockSemaphore = ConvertToGlobalHandle( get_semaphore( crit ) );
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.581)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    if ( !crit->LockSemaphore )
        InitializeCriticalSection( crit );
}


/***********************************************************************
 *           UninitializeCriticalSection   (KERNEL32.703)
 */
void WINAPI UninitializeCriticalSection( CRITICAL_SECTION *crit )
{
    DeleteCriticalSection( crit );
}

#ifdef __i386__

PVOID WINAPI InterlockedCompareExchange( PVOID *dest, PVOID xchg, PVOID compare );
__ASM_GLOBAL_FUNC(InterlockedCompareExchange,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret $12");

LONG WINAPI InterlockedExchange( PLONG dest, LONG val );
__ASM_GLOBAL_FUNC(InterlockedExchange,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret $8");

LONG WINAPI InterlockedExchangeAdd( PLONG dest, LONG incr );
__ASM_GLOBAL_FUNC(InterlockedExchangeAdd,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret $8");

LONG WINAPI InterlockedIncrement( PLONG dest );
__ASM_GLOBAL_FUNC(InterlockedIncrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "incl %eax\n\t"
                  "ret $4");

LONG WINAPI InterlockedDecrement( PLONG dest );
__ASM_GLOBAL_FUNC(InterlockedDecrement,
                  "movl 4(%esp),%edx\n\t"
                  "movl $-1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "decl %eax\n\t"
                  "ret $4");

#else /* __i386__ */
#error You must implement the Interlocked* functions for your CPU
#endif /* __i386__ */
