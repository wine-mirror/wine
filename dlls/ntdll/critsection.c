/*
 * Win32 critical sections
 *
 * Copyright 1998 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#include <time.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static inline LONG interlocked_inc( PLONG dest )
{
    return interlocked_xchg_add( dest, 1 ) + 1;
}

static inline LONG interlocked_dec( PLONG dest )
{
    return interlocked_xchg_add( dest, -1 ) - 1;
}

static inline void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

#ifdef __linux__

static int wait_op = 128; /*FUTEX_WAIT|FUTEX_PRIVATE_FLAG*/
static int wake_op = 129; /*FUTEX_WAKE|FUTEX_PRIVATE_FLAG*/

static inline int futex_wait( int *addr, int val, struct timespec *timeout )
{
    return syscall( __NR_futex, addr, wait_op, val, timeout, 0, 0 );
}

static inline int futex_wake( int *addr, int val )
{
    return syscall( __NR_futex, addr, wake_op, val, NULL, 0, 0 );
}

static inline int use_futexes(void)
{
    static int supported = -1;

    if (supported == -1)
    {
        futex_wait( &supported, 10, NULL );
        if (errno == ENOSYS)
        {
            wait_op = 0; /*FUTEX_WAIT*/
            wake_op = 1; /*FUTEX_WAKE*/
            futex_wait( &supported, 10, NULL );
        }
        supported = (errno != ENOSYS);
    }
    return supported;
}

static inline NTSTATUS fast_wait( RTL_CRITICAL_SECTION *crit, int timeout )
{
    int val;
    struct timespec timespec;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    timespec.tv_sec  = timeout;
    timespec.tv_nsec = 0;
    while ((val = interlocked_cmpxchg( (int *)&crit->LockSemaphore, 0, 1 )) != 1)
    {
        /* note: this may wait longer than specified in case of signals or */
        /*       multiple wake-ups, but that shouldn't be a problem */
        if (futex_wait( (int *)&crit->LockSemaphore, val, &timespec ) == -1 && errno == ETIMEDOUT)
            return STATUS_TIMEOUT;
    }
    return STATUS_WAIT_0;
}

static inline NTSTATUS fast_wake( RTL_CRITICAL_SECTION *crit )
{
    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    *(int *)&crit->LockSemaphore = 1;
    futex_wake( (int *)&crit->LockSemaphore, 1 );
    return STATUS_SUCCESS;
}

static inline void close_semaphore( RTL_CRITICAL_SECTION *crit )
{
    if (!use_futexes()) NtClose( crit->LockSemaphore );
}

#elif defined(__APPLE__)

#include <mach/mach.h>
#include <mach/task.h>
#include <mach/semaphore.h>

static inline semaphore_t get_mach_semaphore( RTL_CRITICAL_SECTION *crit )
{
    semaphore_t ret = *(int *)&crit->LockSemaphore;
    if (!ret)
    {
        semaphore_t sem;
        if (semaphore_create( mach_task_self(), &sem, SYNC_POLICY_FIFO, 0 )) return 0;
        if (!(ret = interlocked_cmpxchg( (int *)&crit->LockSemaphore, sem, 0 )))
            ret = sem;
        else
            semaphore_destroy( mach_task_self(), sem );  /* somebody beat us to it */
    }
    return ret;
}

static inline NTSTATUS fast_wait( RTL_CRITICAL_SECTION *crit, int timeout )
{
    mach_timespec_t timespec;
    semaphore_t sem = get_mach_semaphore( crit );

    timespec.tv_sec = timeout;
    timespec.tv_nsec = 0;
    for (;;)
    {
        switch( semaphore_timedwait( sem, timespec ))
        {
        case KERN_SUCCESS:
            return STATUS_WAIT_0;
        case KERN_ABORTED:
            continue;  /* got a signal, restart */
        case KERN_OPERATION_TIMED_OUT:
            return STATUS_TIMEOUT;
        default:
            return STATUS_INVALID_HANDLE;
        }
    }
}

static inline NTSTATUS fast_wake( RTL_CRITICAL_SECTION *crit )
{
    semaphore_t sem = get_mach_semaphore( crit );
    semaphore_signal( sem );
    return STATUS_SUCCESS;
}

static inline void close_semaphore( RTL_CRITICAL_SECTION *crit )
{
    semaphore_destroy( mach_task_self(), *(int *)&crit->LockSemaphore );
}

#else  /* __APPLE__ */

static inline NTSTATUS fast_wait( RTL_CRITICAL_SECTION *crit, int timeout )
{
    return STATUS_NOT_IMPLEMENTED;
}

static inline NTSTATUS fast_wake( RTL_CRITICAL_SECTION *crit )
{
    return STATUS_NOT_IMPLEMENTED;
}

static inline void close_semaphore( RTL_CRITICAL_SECTION *crit )
{
    NtClose( crit->LockSemaphore );
}

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
        if (!(ret = interlocked_cmpxchg_ptr( &crit->LockSemaphore, sem, 0 )))
            ret = sem;
        else
            NtClose(sem);  /* somebody beat us to it */
    }
    return ret;
}

/***********************************************************************
 *           wait_semaphore
 */
static inline NTSTATUS wait_semaphore( RTL_CRITICAL_SECTION *crit, int timeout )
{
    NTSTATUS ret;

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit->DebugInfo || ((ret = fast_wait( crit, timeout )) == STATUS_NOT_IMPLEMENTED))
    {
        HANDLE sem = get_semaphore( crit );
        LARGE_INTEGER time;
        select_op_t select_op;

        time.QuadPart = timeout * (LONGLONG)-10000000;
        select_op.wait.op = SELECT_WAIT;
        select_op.wait.handles[0] = wine_server_obj_handle( sem );
        ret = server_select( &select_op, offsetof( select_op_t, wait.handles[1] ), 0, &time );
    }
    return ret;
}

/***********************************************************************
 *           RtlInitializeCriticalSection   (NTDLL.@)
 *
 * Initialises a new critical section.
 *
 * PARAMS
 *  crit [O] Critical section to initialise
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSectionAndSpinCount(), RtlDeleteCriticalSection(),
 *  RtlEnterCriticalSection(), RtlLeaveCriticalSection(),
 *  RtlTryEnterCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
NTSTATUS WINAPI RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    return RtlInitializeCriticalSectionEx( crit, 0, 0 );
}

/***********************************************************************
 *           RtlInitializeCriticalSectionAndSpinCount   (NTDLL.@)
 *
 * Initialises a new critical section with a given spin count.
 *
 * PARAMS
 *   crit      [O] Critical section to initialise
 *   spincount [I] Spin count for crit
 * 
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * NOTES
 *  Available on NT4 SP3 or later.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlDeleteCriticalSection(),
 *  RtlEnterCriticalSection(), RtlLeaveCriticalSection(),
 *  RtlTryEnterCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    return RtlInitializeCriticalSectionEx( crit, spincount, 0 );
}

/***********************************************************************
 *           RtlInitializeCriticalSectionEx   (NTDLL.@)
 *
 * Initialises a new critical section with a given spin count and flags.
 *
 * PARAMS
 *   crit      [O] Critical section to initialise.
 *   spincount [I] Number of times to spin upon contention.
 *   flags     [I] RTL_CRITICAL_SECTION_FLAG_ flags from winnt.h.
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * NOTES
 *  Available on Vista or later.
 *
 * SEE
 *  RtlInitializeCriticalSection(), RtlDeleteCriticalSection(),
 *  RtlEnterCriticalSection(), RtlLeaveCriticalSection(),
 *  RtlTryEnterCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionEx( RTL_CRITICAL_SECTION *crit, ULONG spincount, ULONG flags )
{
    if (flags & (RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN|RTL_CRITICAL_SECTION_FLAG_STATIC_INIT))
        FIXME("(%p,%u,0x%08x) semi-stub\n", crit, spincount, flags);

    /* FIXME: if RTL_CRITICAL_SECTION_FLAG_STATIC_INIT is given, we should use
     * memory from a static pool to hold the debug info. Then heap.c could pass
     * this flag rather than initialising the process heap CS by hand. If this
     * is done, then debug info should be managed through Rtlp[Allocate|Free]DebugInfo
     * so (e.g.) MakeCriticalSectionGlobal() doesn't free it using HeapFree().
     */
    if (flags & RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO)
        crit->DebugInfo = NULL;
    else
        crit->DebugInfo = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION_DEBUG));

    if (crit->DebugInfo)
    {
        crit->DebugInfo->Type = 0;
        crit->DebugInfo->CreatorBackTraceIndex = 0;
        crit->DebugInfo->CriticalSection = crit;
        crit->DebugInfo->ProcessLocksList.Blink = &(crit->DebugInfo->ProcessLocksList);
        crit->DebugInfo->ProcessLocksList.Flink = &(crit->DebugInfo->ProcessLocksList);
        crit->DebugInfo->EntryCount = 0;
        crit->DebugInfo->ContentionCount = 0;
        memset( crit->DebugInfo->Spare, 0, sizeof(crit->DebugInfo->Spare) );
    }
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount & ~0x80000000;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           RtlSetCriticalSectionSpinCount   (NTDLL.@)
 *
 * Sets the spin count of a critical section.
 *
 * PARAMS
 *   crit      [I/O] Critical section
 *   spincount [I] Spin count for crit
 *
 * RETURNS
 *  The previous spin count.
 *
 * NOTES
 *  If the system is not SMP, spincount is ignored and set to 0.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
ULONG WINAPI RtlSetCriticalSectionSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    ULONG oldspincount = crit->SpinCount;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount;
    return oldspincount;
}

/***********************************************************************
 *           RtlDeleteCriticalSection   (NTDLL.@)
 *
 * Frees the resources used by a critical section.
 *
 * PARAMS
 *  crit [I/O] Critical section to free
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS WINAPI RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    if (crit->DebugInfo)
    {
        /* only free the ones we made in here */
        if (!crit->DebugInfo->Spare[0])
        {
            RtlFreeHeap( GetProcessHeap(), 0, crit->DebugInfo );
            crit->DebugInfo = NULL;
        }
        close_semaphore( crit );
    }
    else NtClose( crit->LockSemaphore );
    crit->LockSemaphore = 0;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlpWaitForCriticalSection   (NTDLL.@)
 *
 * Waits for a busy critical section to become free.
 * 
 * PARAMS
 *  crit [I/O] Critical section to wait for
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * NOTES
 *  Use RtlEnterCriticalSection() instead of this function as it is often much
 *  faster.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS WINAPI RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    LONGLONG timeout = NtCurrentTeb()->Peb->CriticalSectionTimeout.QuadPart / -10000000;
    for (;;)
    {
        EXCEPTION_RECORD rec;
        NTSTATUS status = wait_semaphore( crit, 5 );
        timeout -= 5;

        if ( status == STATUS_TIMEOUT )
        {
            const char *name = NULL;
            if (crit->DebugInfo) name = (char *)crit->DebugInfo->Spare[0];
            if (!name) name = "?";
            ERR( "section %p %s wait timed out in thread %04x, blocked by %04x, retrying (60 sec)\n",
                 crit, debugstr_a(name), GetCurrentThreadId(), HandleToULong(crit->OwningThread) );
            status = wait_semaphore( crit, 60 );
            timeout -= 60;

            if ( status == STATUS_TIMEOUT && TRACE_ON(relay) )
            {
                ERR( "section %p %s wait timed out in thread %04x, blocked by %04x, retrying (5 min)\n",
                     crit, debugstr_a(name), GetCurrentThreadId(), HandleToULong(crit->OwningThread) );
                status = wait_semaphore( crit, 300 );
                timeout -= 300;
            }
        }
        if (status == STATUS_WAIT_0) break;

        /* Throw exception only for Wine internal locks */
        if ((!crit->DebugInfo) || (!crit->DebugInfo->Spare[0])) continue;

        /* only throw deadlock exception if configured timeout is reached */
        if (timeout > 0) continue;

        rec.ExceptionCode    = STATUS_POSSIBLE_DEADLOCK;
        rec.ExceptionFlags   = 0;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = RtlRaiseException;  /* sic */
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (ULONG_PTR)crit;
        RtlRaiseException( &rec );
    }
    if (crit->DebugInfo) crit->DebugInfo->ContentionCount++;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlpUnWaitCriticalSection   (NTDLL.@)
 *
 * Notifies other threads waiting on the busy critical section that it has
 * become free.
 * 
 * PARAMS
 *  crit [I/O] Critical section
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any error returned by NtReleaseSemaphore()
 *
 * NOTES
 *  Use RtlLeaveCriticalSection() instead of this function as it is often much
 *  faster.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS WINAPI RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    NTSTATUS ret;

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit->DebugInfo || ((ret = fast_wake( crit )) == STATUS_NOT_IMPLEMENTED))
    {
        HANDLE sem = get_semaphore( crit );
        ret = NtReleaseSemaphore( sem, 1, NULL );
    }
    if (ret) RtlRaiseStatus( ret );
    return ret;
}


/***********************************************************************
 *           RtlEnterCriticalSection   (NTDLL.@)
 *
 * Enters a critical section, waiting for it to become available if necessary.
 *
 * PARAMS
 *  crit [I/O] Critical section to enter
 *
 * RETURNS
 *  STATUS_SUCCESS. The critical section is held by the caller.
 *  
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlSetCriticalSectionSpinCount(),
 *  RtlLeaveCriticalSection(), RtlTryEnterCriticalSection()
 */
NTSTATUS WINAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (crit->SpinCount)
    {
        ULONG count;

        if (RtlTryEnterCriticalSection( crit )) return STATUS_SUCCESS;
        for (count = crit->SpinCount; count > 0; count--)
        {
            if (crit->LockCount > 0) break;  /* more than one waiter, don't bother spinning */
            if (crit->LockCount == -1)       /* try again */
            {
                if (interlocked_cmpxchg( &crit->LockCount, 0, -1 ) == -1) goto done;
            }
            small_pause();
        }
    }

    if (interlocked_inc( &crit->LockCount ))
    {
        if (crit->OwningThread == ULongToHandle(GetCurrentThreadId()))
        {
            crit->RecursionCount++;
            return STATUS_SUCCESS;
        }

        /* Now wait for it */
        RtlpWaitForCriticalSection( crit );
    }
done:
    crit->OwningThread   = ULongToHandle(GetCurrentThreadId());
    crit->RecursionCount = 1;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlTryEnterCriticalSection   (NTDLL.@)
 *
 * Tries to enter a critical section without waiting.
 *
 * PARAMS
 *  crit [I/O] Critical section to enter
 *
 * RETURNS
 *  Success: TRUE. The critical section is held by the caller.
 *  Failure: FALSE. The critical section is currently held by another thread.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlLeaveCriticalSection(), RtlSetCriticalSectionSpinCount()
 */
BOOL WINAPI RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    BOOL ret = FALSE;
    if (interlocked_cmpxchg( &crit->LockCount, 0, -1 ) == -1)
    {
        crit->OwningThread   = ULongToHandle(GetCurrentThreadId());
        crit->RecursionCount = 1;
        ret = TRUE;
    }
    else if (crit->OwningThread == ULongToHandle(GetCurrentThreadId()))
    {
        interlocked_inc( &crit->LockCount );
        crit->RecursionCount++;
        ret = TRUE;
    }
    return ret;
}


/***********************************************************************
 *           RtlIsCriticalSectionLocked   (NTDLL.@)
 *
 * Checks if the critical section is locked by any thread.
 *
 * PARAMS
 *  crit [I/O] Critical section to check.
 *
 * RETURNS
 *  Success: TRUE. The critical section is locked.
 *  Failure: FALSE. The critical section is not locked.
 */
BOOL WINAPI RtlIsCriticalSectionLocked( RTL_CRITICAL_SECTION *crit )
{
    return crit->RecursionCount != 0;
}


/***********************************************************************
 *           RtlIsCriticalSectionLockedByThread   (NTDLL.@)
 *
 * Checks if the critical section is locked by the current thread.
 *
 * PARAMS
 *  crit [I/O] Critical section to check.
 *
 * RETURNS
 *  Success: TRUE. The critical section is locked.
 *  Failure: FALSE. The critical section is not locked.
 */
BOOL WINAPI RtlIsCriticalSectionLockedByThread( RTL_CRITICAL_SECTION *crit )
{
    return crit->OwningThread == ULongToHandle(GetCurrentThreadId()) &&
           crit->RecursionCount;
}


/***********************************************************************
 *           RtlLeaveCriticalSection   (NTDLL.@)
 *
 * Leaves a critical section.
 *
 * PARAMS
 *  crit [I/O] Critical section to leave.
 *
 * RETURNS
 *  STATUS_SUCCESS.
 *
 * SEE
 *  RtlInitializeCriticalSectionEx(),
 *  RtlInitializeCriticalSection(), RtlInitializeCriticalSectionAndSpinCount(),
 *  RtlDeleteCriticalSection(), RtlEnterCriticalSection(),
 *  RtlSetCriticalSectionSpinCount(), RtlTryEnterCriticalSection()
 */
NTSTATUS WINAPI RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (--crit->RecursionCount)
    {
        if (crit->RecursionCount > 0) interlocked_dec( &crit->LockCount );
        else ERR( "section %p is not acquired\n", crit );
    }
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
