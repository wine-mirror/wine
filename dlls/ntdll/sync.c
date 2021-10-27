/*
 *	Process synchronisation
 *
 * Copyright 1996, 1997, 1998 Marcus Meissner
 * Copyright 1997, 1998, 1999 Alexandre Julliard
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2003 Eric Pouech
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

#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);
WINE_DECLARE_DEBUG_CHANNEL(relay);

/******************************************************************
 *              RtlRunOnceInitialize (NTDLL.@)
 */
void WINAPI RtlRunOnceInitialize( RTL_RUN_ONCE *once )
{
    once->Ptr = NULL;
}

/******************************************************************
 *              RtlRunOnceBeginInitialize (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceBeginInitialize( RTL_RUN_ONCE *once, ULONG flags, void **context )
{
    if (flags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
        if ((val & 3) != 2) return STATUS_UNSUCCESSFUL;
        if (context) *context = (void *)(val & ~3);
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 0:  /* first time */
            if (!InterlockedCompareExchangePointer( &once->Ptr,
                                                    (flags & RTL_RUN_ONCE_ASYNC) ? (void *)3 : (void *)1, 0 ))
                return STATUS_PENDING;
            break;

        case 1:  /* in progress, wait */
            if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
            next = val & ~3;
            if (InterlockedCompareExchangePointer( &once->Ptr, (void *)((ULONG_PTR)&next | 1),
                                                   (void *)val ) == (void *)val)
                NtWaitForKeyedEvent( 0, &next, FALSE, NULL );
            break;

        case 2:  /* done */
            if (context) *context = (void *)(val & ~3);
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            return STATUS_PENDING;
        }
    }
}

/******************************************************************
 *              RtlRunOnceComplete (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceComplete( RTL_RUN_ONCE *once, ULONG flags, void *context )
{
    if ((ULONG_PTR)context & 3) return STATUS_INVALID_PARAMETER;

    if (flags & RTL_RUN_ONCE_INIT_FAILED)
    {
        if (context) return STATUS_INVALID_PARAMETER;
        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
    }
    else context = (void *)((ULONG_PTR)context | 2);

    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 1:  /* in progress */
            if (InterlockedCompareExchangePointer( &once->Ptr, context, (void *)val ) != (void *)val) break;
            val &= ~3;
            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                NtReleaseKeyedEvent( 0, (void *)val, FALSE, NULL );
                val = next;
            }
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            if (InterlockedCompareExchangePointer( &once->Ptr, context, (void *)val ) != (void *)val) break;
            return STATUS_SUCCESS;

        default:
            return STATUS_UNSUCCESSFUL;
        }
    }
}


/***********************************************************************
 * Critical sections
 ***********************************************************************/


static void *no_debug_info_marker = (void *)(ULONG_PTR)-1;

static BOOL crit_section_has_debuginfo( const RTL_CRITICAL_SECTION *crit )
{
    return crit->DebugInfo != NULL && crit->DebugInfo != no_debug_info_marker;
}

static inline HANDLE get_semaphore( RTL_CRITICAL_SECTION *crit )
{
    HANDLE ret = crit->LockSemaphore;
    if (!ret)
    {
        HANDLE sem;
        if (NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 )) return 0;
        if (!(ret = InterlockedCompareExchangePointer( &crit->LockSemaphore, sem, 0 )))
            ret = sem;
        else
            NtClose(sem);  /* somebody beat us to it */
    }
    return ret;
}

static inline NTSTATUS wait_semaphore( RTL_CRITICAL_SECTION *crit, int timeout )
{
    NTSTATUS ret;

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit_section_has_debuginfo( crit ) ||
        ((ret = unix_funcs->fast_RtlpWaitForCriticalSection( crit, timeout )) == STATUS_NOT_IMPLEMENTED))
    {
        HANDLE sem = get_semaphore( crit );
        LARGE_INTEGER time;

        time.QuadPart = timeout * (LONGLONG)-10000000;
        ret = NtWaitForSingleObject( sem, FALSE, &time );
    }
    return ret;
}

/******************************************************************************
 *      RtlInitializeCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    return RtlInitializeCriticalSectionEx( crit, 0, 0 );
}


/******************************************************************************
 *      RtlInitializeCriticalSectionAndSpinCount   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    return RtlInitializeCriticalSectionEx( crit, spincount, 0 );
}


/******************************************************************************
 *      RtlInitializeCriticalSectionEx   (NTDLL.@)
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
        crit->DebugInfo = no_debug_info_marker;
    else
    {
        crit->DebugInfo = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION_DEBUG ));
        if (crit->DebugInfo)
        {
            crit->DebugInfo->Type = 0;
            crit->DebugInfo->CreatorBackTraceIndex = 0;
            crit->DebugInfo->CriticalSection = crit;
            crit->DebugInfo->ProcessLocksList.Blink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->ProcessLocksList.Flink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->EntryCount = 0;
            crit->DebugInfo->ContentionCount = 0;
            memset( crit->DebugInfo->Spare, 0, sizeof(crit->DebugInfo->Spare) );
        }
    }
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount & ~0x80000000;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlSetCriticalSectionSpinCount   (NTDLL.@)
 */
ULONG WINAPI RtlSetCriticalSectionSpinCount( RTL_CRITICAL_SECTION *crit, ULONG spincount )
{
    ULONG oldspincount = crit->SpinCount;
    if (NtCurrentTeb()->Peb->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount;
    return oldspincount;
}


/******************************************************************************
 *      RtlDeleteCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    if (crit_section_has_debuginfo( crit ))
    {
        /* only free the ones we made in here */
        if (!crit->DebugInfo->Spare[0])
        {
            RtlFreeHeap( GetProcessHeap(), 0, crit->DebugInfo );
            crit->DebugInfo = NULL;
        }
        if (unix_funcs->fast_RtlDeleteCriticalSection( crit ) == STATUS_NOT_IMPLEMENTED)
            NtClose( crit->LockSemaphore );
    }
    else NtClose( crit->LockSemaphore );
    crit->LockSemaphore = 0;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlpWaitForCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    LONGLONG timeout = NtCurrentTeb()->Peb->CriticalSectionTimeout.QuadPart / -10000000;

    /* Don't allow blocking on a critical section during process termination */
    if (RtlDllShutdownInProgress())
    {
        WARN( "process %s is shutting down, returning STATUS_SUCCESS\n",
              debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer) );
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        EXCEPTION_RECORD rec;
        NTSTATUS status = wait_semaphore( crit, 5 );
        timeout -= 5;

        if ( status == STATUS_TIMEOUT )
        {
            const char *name = NULL;
            if (crit_section_has_debuginfo( crit )) name = (char *)crit->DebugInfo->Spare[0];
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
        if (!crit_section_has_debuginfo( crit ) || !crit->DebugInfo->Spare[0]) continue;

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
    if (crit_section_has_debuginfo( crit )) crit->DebugInfo->ContentionCount++;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlpUnWaitCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    NTSTATUS ret;

    /* debug info is cleared by MakeCriticalSectionGlobal */
    if (!crit_section_has_debuginfo( crit ) ||
        ((ret = unix_funcs->fast_RtlpUnWaitCriticalSection( crit )) == STATUS_NOT_IMPLEMENTED))
    {
        HANDLE sem = get_semaphore( crit );
        ret = NtReleaseSemaphore( sem, 1, NULL );
    }
    if (ret) RtlRaiseStatus( ret );
    return ret;
}


static inline void small_pause(void)
{
#ifdef __i386__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/******************************************************************************
 *      RtlEnterCriticalSection   (NTDLL.@)
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
                if (InterlockedCompareExchange( &crit->LockCount, 0, -1 ) == -1) goto done;
            }
            small_pause();
        }
    }

    if (InterlockedIncrement( &crit->LockCount ))
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


/******************************************************************************
 *      RtlTryEnterCriticalSection   (NTDLL.@)
 */
BOOL WINAPI RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    BOOL ret = FALSE;
    if (InterlockedCompareExchange( &crit->LockCount, 0, -1 ) == -1)
    {
        crit->OwningThread   = ULongToHandle(GetCurrentThreadId());
        crit->RecursionCount = 1;
        ret = TRUE;
    }
    else if (crit->OwningThread == ULongToHandle(GetCurrentThreadId()))
    {
        InterlockedIncrement( &crit->LockCount );
        crit->RecursionCount++;
        ret = TRUE;
    }
    return ret;
}


/******************************************************************************
 *      RtlIsCriticalSectionLocked   (NTDLL.@)
 */
BOOL WINAPI RtlIsCriticalSectionLocked( RTL_CRITICAL_SECTION *crit )
{
    return crit->RecursionCount != 0;
}


/******************************************************************************
 *      RtlIsCriticalSectionLockedByThread   (NTDLL.@)
 */
BOOL WINAPI RtlIsCriticalSectionLockedByThread( RTL_CRITICAL_SECTION *crit )
{
    return crit->OwningThread == ULongToHandle(GetCurrentThreadId()) &&
           crit->RecursionCount;
}


/******************************************************************************
 *      RtlLeaveCriticalSection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *crit )
{
    if (--crit->RecursionCount)
    {
        if (crit->RecursionCount > 0) InterlockedDecrement( &crit->LockCount );
        else ERR( "section %p is not acquired\n", crit );
    }
    else
    {
        crit->OwningThread = 0;
        if (InterlockedDecrement( &crit->LockCount ) >= 0)
        {
            /* someone is waiting */
            RtlpUnWaitCriticalSection( crit );
        }
    }
    return STATUS_SUCCESS;
}

/******************************************************************
 *              RtlRunOnceExecuteOnce (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceExecuteOnce( RTL_RUN_ONCE *once, PRTL_RUN_ONCE_INIT_FN func,
                                    void *param, void **context )
{
    DWORD ret = RtlRunOnceBeginInitialize( once, 0, context );

    if (ret != STATUS_PENDING) return ret;

    if (!func( once, param, context ))
    {
        RtlRunOnceComplete( once, RTL_RUN_ONCE_INIT_FAILED, NULL );
        return STATUS_UNSUCCESSFUL;
    }

    return RtlRunOnceComplete( once, 0, context ? *context : NULL );
}


/* SRW locks implementation
 *
 * The memory layout used by the lock is:
 *
 * 32 31            16               0
 *  ________________ ________________
 * | X| #exclusive  |    #shared     |
 *  ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 * Since there is no space left for a separate counter of shared access
 * threads inside the locked section the #shared field is used for multiple
 * purposes. The following table lists all possible states the lock can be
 * in, notation: [X, #exclusive, #shared]:
 *
 * [0,   0,   N] -> locked by N shared access threads, if N=0 it's unlocked
 * [0, >=1, >=1] -> threads are requesting exclusive locks, but there are
 * still shared access threads inside. #shared should not be incremented
 * anymore!
 * [1, >=1, >=0] -> lock is owned by an exclusive thread and the #shared
 * counter can be used again to count the number of threads waiting in the
 * queue for shared access.
 *
 * the following states are invalid and will never occur:
 * [0, >=1,   0], [1,   0, >=0]
 *
 * The main problem arising from the fact that we have no separate counter
 * of shared access threads inside the locked section is that in the state
 * [0, >=1, >=1] above we cannot add additional waiting threads to the
 * shared access queue - it wouldn't be possible to distinguish waiting
 * threads and those that are still inside. To solve this problem the lock
 * uses the following approach: a thread that isn't able to allocate a
 * shared lock just uses the exclusive queue instead. As soon as the thread
 * is woken up it is in the state [1, >=1, >=0]. In this state it's again
 * possible to use the shared access queue. The thread atomically moves
 * itself to the shared access queue and releases the exclusive lock, so
 * that the "real" exclusive access threads have a chance. As soon as they
 * are all ready the shared access threads are processed.
 */

#define SRWLOCK_MASK_IN_EXCLUSIVE     0x80000000
#define SRWLOCK_MASK_EXCLUSIVE_QUEUE  0x7fff0000
#define SRWLOCK_MASK_SHARED_QUEUE     0x0000ffff
#define SRWLOCK_RES_EXCLUSIVE         0x00010000
#define SRWLOCK_RES_SHARED            0x00000001

#ifdef WORDS_BIGENDIAN
#define srwlock_key_exclusive(lock)   ((void *)(((ULONG_PTR)&lock->Ptr + 1) & ~1))
#define srwlock_key_shared(lock)      ((void *)(((ULONG_PTR)&lock->Ptr + 3) & ~1))
#else
#define srwlock_key_exclusive(lock)   ((void *)(((ULONG_PTR)&lock->Ptr + 3) & ~1))
#define srwlock_key_shared(lock)      ((void *)(((ULONG_PTR)&lock->Ptr + 1) & ~1))
#endif

static inline void srwlock_check_invalid( unsigned int val )
{
    /* Throw exception if it's impossible to acquire/release this lock. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) == SRWLOCK_MASK_EXCLUSIVE_QUEUE ||
            (val & SRWLOCK_MASK_SHARED_QUEUE) == SRWLOCK_MASK_SHARED_QUEUE)
        RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
}

static inline unsigned int srwlock_lock_exclusive( unsigned int *dest, int incr )
{
    unsigned int val, tmp;
    /* Atomically modifies the value of *dest by adding incr. If the shared
     * queue is empty and there are threads waiting for exclusive access, then
     * sets the mark SRWLOCK_MASK_IN_EXCLUSIVE to signal other threads that
     * they are allowed again to use the shared queue counter. */
    for (val = *dest;; val = tmp)
    {
        tmp = val + incr;
        srwlock_check_invalid( tmp );
        if ((tmp & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(tmp & SRWLOCK_MASK_SHARED_QUEUE))
            tmp |= SRWLOCK_MASK_IN_EXCLUSIVE;
        if ((tmp = InterlockedCompareExchange( (int *)dest, tmp, val )) == val)
            break;
    }
    return val;
}

static inline unsigned int srwlock_unlock_exclusive( unsigned int *dest, int incr )
{
    unsigned int val, tmp;
    /* Atomically modifies the value of *dest by adding incr. If the queue of
     * threads waiting for exclusive access is empty, then remove the
     * SRWLOCK_MASK_IN_EXCLUSIVE flag (only the shared queue counter will
     * remain). */
    for (val = *dest;; val = tmp)
    {
        tmp = val + incr;
        srwlock_check_invalid( tmp );
        if (!(tmp & SRWLOCK_MASK_EXCLUSIVE_QUEUE))
            tmp &= SRWLOCK_MASK_SHARED_QUEUE;
        if ((tmp = InterlockedCompareExchange( (int *)dest, tmp, val )) == val)
            break;
    }
    return val;
}

static inline void srwlock_leave_exclusive( RTL_SRWLOCK *lock, unsigned int val )
{
    /* Used when a thread leaves an exclusive section. If there are other
     * exclusive access threads they are processed first, followed by
     * the shared waiters. */
    if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
        NtReleaseKeyedEvent( 0, srwlock_key_exclusive(lock), FALSE, NULL );
    else
    {
        val &= SRWLOCK_MASK_SHARED_QUEUE; /* remove SRWLOCK_MASK_IN_EXCLUSIVE */
        while (val--)
            NtReleaseKeyedEvent( 0, srwlock_key_shared(lock), FALSE, NULL );
    }
}

static inline void srwlock_leave_shared( RTL_SRWLOCK *lock, unsigned int val )
{
    /* Wake up one exclusive thread as soon as the last shared access thread
     * has left. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_SHARED_QUEUE))
        NtReleaseKeyedEvent( 0, srwlock_key_exclusive(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlInitializeSRWLock (NTDLL.@)
 *
 * NOTES
 *  Please note that SRWLocks do not keep track of the owner of a lock.
 *  It doesn't make any difference which thread for example unlocks an
 *  SRWLock (see corresponding tests). This implementation uses two
 *  keyed events (one for the exclusive waiters and one for the shared
 *  waiters) and is limited to 2^15-1 waiting threads.
 */
void WINAPI RtlInitializeSRWLock( RTL_SRWLOCK *lock )
{
    lock->Ptr = NULL;
}

/***********************************************************************
 *              RtlAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Unlike RtlAcquireResourceExclusive this function doesn't allow
 *  nested calls from the same thread. "Upgrading" a shared access lock
 *  to an exclusive access lock also doesn't seem to be supported.
 */
void WINAPI RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    if (unix_funcs->fast_RtlAcquireSRWLockExclusive( lock ) != STATUS_NOT_IMPLEMENTED)
        return;

    if (srwlock_lock_exclusive( (unsigned int *)&lock->Ptr, SRWLOCK_RES_EXCLUSIVE ))
        NtWaitForKeyedEvent( 0, srwlock_key_exclusive(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlAcquireSRWLockShared (NTDLL.@)
 *
 * NOTES
 *   Do not call this function recursively - it will only succeed when
 *   there are no threads waiting for an exclusive lock!
 */
void WINAPI RtlAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    unsigned int val, tmp;

    if (unix_funcs->fast_RtlAcquireSRWLockShared( lock ) != STATUS_NOT_IMPLEMENTED)
        return;

    /* Acquires a shared lock. If it's currently not possible to add elements to
     * the shared queue, then request exclusive access instead. */
    for (val = *(unsigned int *)&lock->Ptr;; val = tmp)
    {
        if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_IN_EXCLUSIVE))
            tmp = val + SRWLOCK_RES_EXCLUSIVE;
        else
            tmp = val + SRWLOCK_RES_SHARED;
        if ((tmp = InterlockedCompareExchange( (int *)&lock->Ptr, tmp, val )) == val)
            break;
    }

    /* Drop exclusive access again and instead requeue for shared access. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_IN_EXCLUSIVE))
    {
        NtWaitForKeyedEvent( 0, srwlock_key_exclusive(lock), FALSE, NULL );
        val = srwlock_unlock_exclusive( (unsigned int *)&lock->Ptr, (SRWLOCK_RES_SHARED
                                        - SRWLOCK_RES_EXCLUSIVE) ) - SRWLOCK_RES_EXCLUSIVE;
        srwlock_leave_exclusive( lock, val );
    }

    if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
        NtWaitForKeyedEvent( 0, srwlock_key_shared(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlReleaseSRWLockExclusive (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock )
{
    if (unix_funcs->fast_RtlReleaseSRWLockExclusive( lock ) != STATUS_NOT_IMPLEMENTED)
        return;

    srwlock_leave_exclusive( lock, srwlock_unlock_exclusive( (unsigned int *)&lock->Ptr,
                             - SRWLOCK_RES_EXCLUSIVE ) - SRWLOCK_RES_EXCLUSIVE );
}

/***********************************************************************
 *              RtlReleaseSRWLockShared (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockShared( RTL_SRWLOCK *lock )
{
    if (unix_funcs->fast_RtlReleaseSRWLockShared( lock ) != STATUS_NOT_IMPLEMENTED)
        return;

    srwlock_leave_shared( lock, srwlock_lock_exclusive( (unsigned int *)&lock->Ptr,
                          - SRWLOCK_RES_SHARED ) - SRWLOCK_RES_SHARED );
}

/***********************************************************************
 *              RtlTryAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Similarly to AcquireSRWLockExclusive, recursive calls are not allowed
 *  and will fail with a FALSE return value.
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    NTSTATUS ret;

    if ((ret = unix_funcs->fast_RtlTryAcquireSRWLockExclusive( lock )) != STATUS_NOT_IMPLEMENTED)
        return (ret == STATUS_SUCCESS);

    return InterlockedCompareExchange( (int *)&lock->Ptr, SRWLOCK_MASK_IN_EXCLUSIVE |
                                       SRWLOCK_RES_EXCLUSIVE, 0 ) == 0;
}

/***********************************************************************
 *              RtlTryAcquireSRWLockShared (NTDLL.@)
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    unsigned int val, tmp;
    NTSTATUS ret;

    if ((ret = unix_funcs->fast_RtlTryAcquireSRWLockShared( lock )) != STATUS_NOT_IMPLEMENTED)
        return (ret == STATUS_SUCCESS);

    for (val = *(unsigned int *)&lock->Ptr;; val = tmp)
    {
        if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
            return FALSE;
        if ((tmp = InterlockedCompareExchange( (int *)&lock->Ptr, val + SRWLOCK_RES_SHARED, val )) == val)
            break;
    }
    return TRUE;
}

/***********************************************************************
 *           RtlInitializeConditionVariable   (NTDLL.@)
 *
 * Initializes the condition variable with NULL.
 *
 * PARAMS
 *  variable [O] condition variable
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlInitializeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    variable->Ptr = NULL;
}

/***********************************************************************
 *           RtlWakeConditionVariable   (NTDLL.@)
 *
 * Wakes up one thread waiting on the condition variable.
 *
 * PARAMS
 *  variable [I/O] condition variable to wake up.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  The calling thread does not have to own any lock in order to call
 *  this function.
 */
void WINAPI RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    if (unix_funcs->fast_RtlWakeConditionVariable( variable, 1 ) == STATUS_NOT_IMPLEMENTED)
    {
        InterlockedIncrement( (int *)&variable->Ptr );
        RtlWakeAddressSingle( variable );
    }
}

/***********************************************************************
 *           RtlWakeAllConditionVariable   (NTDLL.@)
 *
 * See WakeConditionVariable, wakes up all waiting threads.
 */
void WINAPI RtlWakeAllConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    if (unix_funcs->fast_RtlWakeConditionVariable( variable, INT_MAX ) == STATUS_NOT_IMPLEMENTED)
    {
        InterlockedIncrement( (int *)&variable->Ptr );
        RtlWakeAddressAll( variable );
    }
}

/***********************************************************************
 *           RtlSleepConditionVariableCS   (NTDLL.@)
 *
 * Atomically releases the critical section and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the critical section again and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  crit      [I/O] critical section to leave temporarily
 *  timeout   [I]   timeout
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 */
NTSTATUS WINAPI RtlSleepConditionVariableCS( RTL_CONDITION_VARIABLE *variable, RTL_CRITICAL_SECTION *crit,
                                             const LARGE_INTEGER *timeout )
{
    const void *value = variable->Ptr;
    NTSTATUS status;

    RtlLeaveCriticalSection( crit );
    if ((status = unix_funcs->fast_wait_cv( variable, value, timeout )) == STATUS_NOT_IMPLEMENTED)
        status = RtlWaitOnAddress( &variable->Ptr, &value, sizeof(value), timeout );
    RtlEnterCriticalSection( crit );
    return status;
}

/***********************************************************************
 *           RtlSleepConditionVariableSRW   (NTDLL.@)
 *
 * Atomically releases the SRWLock and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the SRWLock again with the same access rights and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  lock      [I/O] SRWLock to leave temporarily
 *  timeout   [I]   timeout
 *  flags     [I]   type of the current lock (exclusive / shared)
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 *
 * NOTES
 *  the behaviour is undefined if the thread doesn't own the lock.
 */
NTSTATUS WINAPI RtlSleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock,
                                              const LARGE_INTEGER *timeout, ULONG flags )
{
    const void *value = variable->Ptr;
    NTSTATUS status;

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlReleaseSRWLockShared( lock );
    else
        RtlReleaseSRWLockExclusive( lock );

    if ((status = unix_funcs->fast_wait_cv( variable, value, timeout )) == STATUS_NOT_IMPLEMENTED)
        status = RtlWaitOnAddress( variable, &value, sizeof(value), timeout );

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlAcquireSRWLockShared( lock );
    else
        RtlAcquireSRWLockExclusive( lock );
    return status;
}

/***********************************************************************
 *           RtlWaitOnAddress   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWaitOnAddress( const void *addr, const void *cmp, SIZE_T size,
                                  const LARGE_INTEGER *timeout )
{
    return unix_funcs->RtlWaitOnAddress( addr, cmp, size, timeout );
}

/***********************************************************************
 *           RtlWakeAddressAll    (NTDLL.@)
 */
void WINAPI RtlWakeAddressAll( const void *addr )
{
    return unix_funcs->RtlWakeAddressAll( addr );
}

/***********************************************************************
 *           RtlWakeAddressSingle (NTDLL.@)
 */
void WINAPI RtlWakeAddressSingle( const void *addr )
{
    return unix_funcs->RtlWakeAddressSingle( addr );
}
