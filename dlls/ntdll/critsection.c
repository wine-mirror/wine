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
#include "ntddk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ntdll);
DECLARE_DEBUG_CHANNEL(relay);

/* Define the atomic exchange/inc/dec functions.
 * These are available in kernel32.dll already,
 * but we don't want to import kernel32 from ntdll.
 */

#ifdef __i386__

# ifdef __GNUC__

inline static PVOID interlocked_cmpxchg( PVOID *dest, PVOID xchg, PVOID compare )
{
    PVOID ret;
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (dest), "r" (xchg), "0" (compare) : "memory" );
    return ret;
}
inline static LONG interlocked_inc( PLONG dest )
{
    LONG ret;
    __asm__ __volatile__( "lock; xaddl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (1) : "memory" );
    return ret + 1;
}
inline static LONG interlocked_dec( PLONG dest )
{
    LONG ret;
    __asm__ __volatile__( "lock; xaddl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (-1) : "memory" );
    return ret - 1;
}

# else  /* __GNUC__ */

PVOID WINAPI interlocked_cmpxchg( PVOID *dest, PVOID xchg, PVOID compare );
__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret $12");
LONG WINAPI interlocked_inc( PLONG dest );
__ASM_GLOBAL_FUNC(interlocked_inc,
                  "movl 4(%esp),%edx\n\t"
                  "movl $1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "incl %eax\n\t"
                  "ret $4");
LONG WINAPI interlocked_dec( PLONG dest );
__ASM_GLOBAL_FUNC(interlocked_dec,
                  "movl 4(%esp),%edx\n\t"
                  "movl $-1,%eax\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "decl %eax\n\t"
                  "ret $4");
# endif  /* __GNUC__ */

#elif defined(__sparc__) && defined(__sun__)
/*
 * As the earlier Sparc processors lack necessary atomic instructions,
 * I'm simply falling back to the library-provided _lwp_mutex routines
 * to ensure mutual exclusion in a way appropriate for the current 
 * architecture.  
 *
 * FIXME:  If we have the compare-and-swap instruction (Sparc v9 and above)
 *         we could use this to speed up the Interlocked operations ...
 */
#include <synch.h>
static lwp_mutex_t interlocked_mutex = DEFAULTMUTEX;

static PVOID interlocked_cmpxchg( PVOID *dest, PVOID xchg, PVOID compare )
{
    _lwp_mutex_lock( &interlocked_mutex );
    if ( *dest == compare )
        *dest = xchg;
    else
        compare = *dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return compare;
}

static LONG interlocked_inc( PLONG dest )
{
    LONG retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = ++*dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

static LONG interlocked_dec( PLONG dest )
{
    LONG retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = --*dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}
#else
# error You must implement the interlocked* functions for your CPU
#endif


/***********************************************************************
 *           get_semaphore
 */
static inline HANDLE get_semaphore( RTL_CRITICAL_SECTION *crit )
{
    HANDLE ret = crit->LockSemaphore;
    if (!ret)
    {
        HANDLE sem;
        if (NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 )) return 0;
        if (!(ret = (HANDLE)InterlockedCompareExchange( (PVOID *)&crit->LockSemaphore,
                                                        (PVOID)sem, 0 )))
            ret = sem;
        else
            NtClose(sem);  /* somebody beat us to it */
    }
    return ret;
}

/***********************************************************************
 *           RtlInitializeCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           RtlInitializeCriticalSectionAndSpinCount   (NTDLL.@)
 * The InitializeCriticalSectionAndSpinCount (KERNEL32) function is
 * available on NT4SP3 or later, and Win98 or later.
 * I am assuming that this is the correct definition given the MSDN
 * docs for the kernel32 functions.
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount( RTL_CRITICAL_SECTION *crit, DWORD spincount )
{
    if(spincount) FIXME("critsection=%p: spincount=%ld not supported\n", crit, spincount);
    crit->SpinCount = spincount;
    return RtlInitializeCriticalSection( crit );
}


/***********************************************************************
 *           RtlDeleteCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    if (crit->LockSemaphore) NtClose( crit->LockSemaphore );
    crit->LockSemaphore  = 0;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlpWaitForCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit )
{
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
        if (res == STATUS_WAIT_0) return STATUS_SUCCESS;

        rec.ExceptionCode    = EXCEPTION_CRITICAL_SECTION_WAIT;
        rec.ExceptionFlags   = 0;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = RtlRaiseException;  /* sic */
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD)crit;
        RtlRaiseException( &rec );
    }
}


/***********************************************************************
 *           RtlpUnWaitCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    HANDLE sem = get_semaphore( crit );
    NTSTATUS res = NtReleaseSemaphore( sem, 1, NULL );
    if (res) RtlRaiseStatus( res );
    return res;
}


/***********************************************************************
 *           RtlEnterCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (interlocked_inc( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return STATUS_SUCCESS;
        }

        /* Now wait for it */
        RtlpWaitForCriticalSection( crit );
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlTryEnterCriticalSection   (NTDLL.@)
 */
BOOL WINAPI RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    BOOL ret = FALSE;
    if (interlocked_cmpxchg( (PVOID *)&crit->LockCount, (PVOID)0L, (PVOID)-1L ) == (PVOID)-1L)
    {
        crit->OwningThread   = GetCurrentThreadId();
        crit->RecursionCount = 1;
        ret = TRUE;
    }
    else if (crit->OwningThread == GetCurrentThreadId())
    {
        interlocked_inc( &crit->LockCount );
        crit->RecursionCount++;
        ret = TRUE;
    }
    return ret;
}


/***********************************************************************
 *           RtlLeaveCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (--crit->RecursionCount) interlocked_dec( &crit->LockCount );
    else
    {
        crit->OwningThread = 0;
        if (interlocked_dec( &crit->LockCount ) >= 0)
        {
            /* someone is waiting */
            RtlpUnWaitCriticalSection( crit );
        }
    }
    return STATUS_SUCCESS;
}
