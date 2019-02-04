/*
 * Kernel synchronization
 *
 * Copyright (C) 2018 Zebediah Figura
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
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "ddk/wdm.h"

#include "wine/debug.h"

#include "ntoskrnl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);

enum object_type
{
    TYPE_MANUAL_EVENT = 0,
    TYPE_AUTO_EVENT = 1,
    TYPE_MUTEX = 2,
    TYPE_SEMAPHORE = 5,
    TYPE_MANUAL_TIMER = 8,
    TYPE_AUTO_TIMER = 9,
};

static CRITICAL_SECTION sync_cs;
static CRITICAL_SECTION_DEBUG sync_cs_debug =
{
    0, 0, &sync_cs,
    { &sync_cs_debug.ProcessLocksList, &sync_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": sync_cs") }
};
static CRITICAL_SECTION sync_cs = { &sync_cs_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 *           KeWaitForMultipleObjects   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeWaitForMultipleObjects(ULONG count, void *pobjs[],
    WAIT_TYPE wait_type, KWAIT_REASON reason, KPROCESSOR_MODE mode,
    BOOLEAN alertable, LARGE_INTEGER *timeout, KWAIT_BLOCK *wait_blocks)
{
    DISPATCHER_HEADER **objs = (DISPATCHER_HEADER **)pobjs;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    NTSTATUS ret;
    ULONG i;

    TRACE("count %u, objs %p, wait_type %u, reason %u, mode %d, alertable %u, timeout %p, wait_blocks %p.\n",
        count, objs, wait_type, reason, mode, alertable, timeout, wait_blocks);

    /* We co-opt DISPATCHER_HEADER.WaitListHead:
     * Blink stores a handle to the synchronization object,
     * Flink stores the number of threads currently waiting on this object. */

    EnterCriticalSection( &sync_cs );
    for (i = 0; i < count; i++)
    {
        ++*((ULONG_PTR *)&objs[i]->WaitListHead.Flink);
        if (!objs[i]->WaitListHead.Blink)
        {
            switch (objs[i]->Type)
            {
            case TYPE_MANUAL_EVENT:
                objs[i]->WaitListHead.Blink = CreateEventW( NULL, TRUE, objs[i]->SignalState, NULL );
                break;
            case TYPE_AUTO_EVENT:
                objs[i]->WaitListHead.Blink = CreateEventW( NULL, FALSE, objs[i]->SignalState, NULL );
                break;
            case TYPE_MUTEX:
                objs[i]->WaitListHead.Blink = CreateMutexW( NULL, FALSE, NULL );
                break;
            case TYPE_SEMAPHORE:
            {
                KSEMAPHORE *semaphore = CONTAINING_RECORD(objs[i], KSEMAPHORE, Header);
                objs[i]->WaitListHead.Blink = CreateSemaphoreW( NULL,
                    semaphore->Header.SignalState, semaphore->Limit, NULL );
                break;
            }
            case TYPE_MANUAL_TIMER:
            case TYPE_AUTO_TIMER:
                break;
            }
        }

        handles[i] = objs[i]->WaitListHead.Blink;
    }
    LeaveCriticalSection( &sync_cs );

    ret = NtWaitForMultipleObjects( count, handles, (wait_type == WaitAny), alertable, timeout );

    EnterCriticalSection( &sync_cs );
    for (i = 0; i < count; i++)
    {
        if (ret == i || (!ret && wait_type == WaitAll))
        {
            switch (objs[i]->Type)
            {
            case TYPE_AUTO_EVENT:
            case TYPE_AUTO_TIMER:
                objs[i]->SignalState = FALSE;
                break;
            case TYPE_MUTEX:
            case TYPE_SEMAPHORE:
                --objs[i]->SignalState;
                break;
            }
        }

        if (!--*((ULONG_PTR *)&objs[i]->WaitListHead.Flink))
        {
            switch (objs[i]->Type)
            {
            case TYPE_MANUAL_EVENT:
            case TYPE_AUTO_EVENT:
            case TYPE_SEMAPHORE:
                CloseHandle(objs[i]->WaitListHead.Blink);
                objs[i]->WaitListHead.Blink = NULL;
                break;
            case TYPE_MUTEX:
                /* Native will panic if a mutex is destroyed while held, so we
                 * don't have to worry about leaking the handle here. */
                if (objs[i]->SignalState == 1)
                {
                    CloseHandle(objs[i]->WaitListHead.Blink);
                    objs[i]->WaitListHead.Blink = NULL;
                }
                break;
            }
        }
    }
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeWaitForSingleObject   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeWaitForSingleObject( void *obj, KWAIT_REASON reason,
    KPROCESSOR_MODE mode, BOOLEAN alertable, LARGE_INTEGER *timeout )
{
    return KeWaitForMultipleObjects( 1, &obj, WaitAny, reason, mode, alertable, timeout, NULL );
}

/***********************************************************************
 *           KeWaitForMutexObject   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeWaitForMutexObject( PRKMUTEX mutex, KWAIT_REASON reason,
    KPROCESSOR_MODE mode, BOOLEAN alertable, LARGE_INTEGER *timeout)
{
    return KeWaitForSingleObject( mutex, reason, mode, alertable, timeout );
}


/***********************************************************************
 *           KeInitializeEvent   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeEvent( PRKEVENT event, EVENT_TYPE type, BOOLEAN state )
{
    TRACE("event %p, type %u, state %u.\n", event, type, state);

    event->Header.Type = type;
    event->Header.SignalState = state;
    event->Header.WaitListHead.Blink = NULL;
    event->Header.WaitListHead.Flink = NULL;
}

/***********************************************************************
 *           KeSetEvent   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeSetEvent( PRKEVENT event, KPRIORITY increment, BOOLEAN wait )
{
    HANDLE handle;
    LONG ret;

    TRACE("event %p, increment %d, wait %u.\n", event, increment, wait);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchange( &event->Header.SignalState, TRUE );
    if ((handle = event->Header.WaitListHead.Blink))
        SetEvent( handle );
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeResetEvent   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeResetEvent( PRKEVENT event )
{
    HANDLE handle;
    LONG ret;

    TRACE("event %p.\n", event);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchange( &event->Header.SignalState, FALSE );
    if ((handle = event->Header.WaitListHead.Blink))
        ResetEvent( handle );
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeClearEvent (NTOSKRNL.EXE.@)
 */
void WINAPI KeClearEvent( PRKEVENT event )
{
    KeResetEvent( event );
}

/***********************************************************************
 *           KeInitializeSemaphore   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeSemaphore( PRKSEMAPHORE semaphore, LONG count, LONG limit )
{
    TRACE("semaphore %p, count %d, limit %d.\n", semaphore, count, limit);

    semaphore->Header.Type = TYPE_SEMAPHORE;
    semaphore->Header.SignalState = count;
    semaphore->Header.WaitListHead.Blink = NULL;
    semaphore->Header.WaitListHead.Flink = NULL;
    semaphore->Limit = limit;
}

/***********************************************************************
 *           KeReleaseSemaphore   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeReleaseSemaphore( PRKSEMAPHORE semaphore, KPRIORITY increment,
                                LONG count, BOOLEAN wait )
{
    HANDLE handle;
    LONG ret;

    TRACE("semaphore %p, increment %d, count %d, wait %u.\n",
        semaphore, increment, count, wait);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchangeAdd( &semaphore->Header.SignalState, count );
    if ((handle = semaphore->Header.WaitListHead.Blink))
        ReleaseSemaphore( handle, count, NULL );
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeInitializeMutex   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeMutex( PRKMUTEX mutex, ULONG level )
{
    TRACE("mutex %p, level %u.\n", mutex, level);

    mutex->Header.Type = TYPE_MUTEX;
    mutex->Header.SignalState = 1;
    mutex->Header.WaitListHead.Blink = NULL;
    mutex->Header.WaitListHead.Flink = NULL;
}

/***********************************************************************
 *           KeReleaseMutex   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeReleaseMutex( PRKMUTEX mutex, BOOLEAN wait )
{
    LONG ret;

    TRACE("mutex %p, wait %u.\n", mutex, wait);

    EnterCriticalSection( &sync_cs );
    ret = mutex->Header.SignalState++;
    if (!ret && !mutex->Header.WaitListHead.Flink)
    {
        CloseHandle( mutex->Header.WaitListHead.Blink );
        mutex->Header.WaitListHead.Blink = NULL;
    }
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeInitializeTimerEx   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeTimerEx( KTIMER *timer, TIMER_TYPE type )
{
    TRACE("timer %p, type %u.\n", timer, type);

    RtlZeroMemory(timer, sizeof(KTIMER));
    timer->Header.Type = (type == NotificationTimer) ? TYPE_MANUAL_TIMER : TYPE_AUTO_TIMER;
    timer->Header.SignalState = FALSE;
    timer->Header.Inserted = FALSE;
    timer->Header.WaitListHead.Blink = NULL;
    timer->Header.WaitListHead.Flink = NULL;
}

/***********************************************************************
 *           KeInitializeTimer   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeTimer( KTIMER *timer )
{
    KeInitializeTimerEx(timer, NotificationTimer);
}

/***********************************************************************
 *           KeSetTimerEx (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI KeSetTimerEx( KTIMER *timer, LARGE_INTEGER duetime, LONG period, KDPC *dpc )
{
    BOOL ret;

    TRACE("timer %p, duetime %s, period %d, dpc %p.\n",
        timer, wine_dbgstr_longlong(duetime.QuadPart), period, dpc);

    if (dpc)
    {
        FIXME("Unhandled DPC %p.\n", dpc);
        return FALSE;
    }

    EnterCriticalSection( &sync_cs );

    ret = timer->Header.Inserted;
    timer->Header.Inserted = TRUE;
    timer->Header.WaitListHead.Blink = CreateWaitableTimerW( NULL, timer->Header.Type == TYPE_MANUAL_TIMER, NULL );
    SetWaitableTimer( timer->Header.WaitListHead.Blink, &duetime, period, NULL, NULL, FALSE );

    LeaveCriticalSection( &sync_cs );

    return ret;
}

BOOLEAN WINAPI KeCancelTimer( KTIMER *timer )
{
    BOOL ret;

    TRACE("timer %p.\n", timer);

    EnterCriticalSection( &sync_cs );
    ret = timer->Header.Inserted;
    timer->Header.Inserted = FALSE;
    CloseHandle(timer->Header.WaitListHead.Blink);
    timer->Header.WaitListHead.Blink = NULL;
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeDelayExecutionThread  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeDelayExecutionThread( KPROCESSOR_MODE mode, BOOLEAN alertable, LARGE_INTEGER *timeout )
{
    TRACE("mode %d, alertable %u, timeout %p.\n", mode, alertable, timeout);
    return NtDelayExecution( alertable, timeout );
}

/***********************************************************************
 *           KeInitializeSpinLock   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeSpinLock( KSPIN_LOCK *lock )
{
    TRACE("lock %p.\n", lock);
    *lock = 0;
}

static inline void small_pause(void)
{
#ifdef __x86_64__
    __asm__ __volatile__( "rep;nop" : : : "memory" );
#else
    __asm__ __volatile__( "" : : : "memory" );
#endif
}

/***********************************************************************
 *           KeAcquireSpinLockAtDpcLevel (NTOSKRNL.EXE.@)
 */
void WINAPI KeAcquireSpinLockAtDpcLevel( KSPIN_LOCK *lock )
{
    TRACE("lock %p.\n", lock);
    while (!InterlockedCompareExchangePointer( (void **)lock, (void *)1, (void *)0 ))
        small_pause();
}

/***********************************************************************
 *           KeReleaseSpinLockFromDpcLevel (NTOSKRNL.EXE.@)
 */
void WINAPI KeReleaseSpinLockFromDpcLevel( KSPIN_LOCK *lock )
{
    TRACE("lock %p.\n", lock);
    InterlockedExchangePointer( (void **)lock, 0 );
}

#define QUEUED_SPINLOCK_OWNED   0x2

/***********************************************************************
 *           KeAcquireInStackQueuedSpinLockAtDpcLevel  (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( KeAcquireInStackQueuedSpinLockAtDpcLevel )
void WINAPI DECLSPEC_HIDDEN __regs_KeAcquireInStackQueuedSpinLockAtDpcLevel( KSPIN_LOCK *lock, KLOCK_QUEUE_HANDLE *queue )
#else
void WINAPI KeAcquireInStackQueuedSpinLockAtDpcLevel( KSPIN_LOCK *lock, KLOCK_QUEUE_HANDLE *queue )
#endif
{
    KSPIN_LOCK_QUEUE *tail;

    TRACE("lock %p, queue %p.\n", lock, queue);

    queue->LockQueue.Next = NULL;

    if (!(tail = InterlockedExchangePointer( (void **)lock, &queue->LockQueue )))
        queue->LockQueue.Lock = (KSPIN_LOCK *)((ULONG_PTR)lock | QUEUED_SPINLOCK_OWNED);
    else
    {
        queue->LockQueue.Lock = lock;
        InterlockedExchangePointer( (void **)&tail->Next, &queue->LockQueue );

        while (!((ULONG_PTR)InterlockedCompareExchangePointer( (void **)&queue->LockQueue.Lock, 0, 0 )
                 & QUEUED_SPINLOCK_OWNED))
        {
            small_pause();
        }
    }
}

/***********************************************************************
 *           KeReleaseInStackQueuedSpinLockFromDpcLevel  (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KeReleaseInStackQueuedSpinLockFromDpcLevel )
void WINAPI DECLSPEC_HIDDEN __regs_KeReleaseInStackQueuedSpinLockFromDpcLevel( KLOCK_QUEUE_HANDLE *queue )
#else
void WINAPI KeReleaseInStackQueuedSpinLockFromDpcLevel( KLOCK_QUEUE_HANDLE *queue )
#endif
{
    KSPIN_LOCK *lock = (KSPIN_LOCK *)((ULONG_PTR)queue->LockQueue.Lock & ~QUEUED_SPINLOCK_OWNED);
    KSPIN_LOCK_QUEUE *next;

    TRACE("lock %p, queue %p.\n", lock, queue);

    queue->LockQueue.Lock = NULL;

    if (!(next = queue->LockQueue.Next))
    {
        /* If we are truly the last in the queue, the lock will point to us. */
        if (InterlockedCompareExchangePointer( (void **)lock, NULL, &queue->LockQueue ) == queue)
            return;

        /* Otherwise, someone just queued themselves, but hasn't yet set
         * themselves as successor. Spin waiting for them to do so. */
        while (!(next = queue->LockQueue.Next))
            small_pause();
    }

    InterlockedExchangePointer( (void **)&next->Lock, (KSPIN_LOCK *)((ULONG_PTR)lock | QUEUED_SPINLOCK_OWNED) );
}

#ifndef __i386__
/***********************************************************************
 *           KeReleaseSpinLock (NTOSKRNL.EXE.@)
 */
void WINAPI KeReleaseSpinLock( KSPIN_LOCK *lock, KIRQL irql )
{
    TRACE("lock %p, irql %u.\n", lock, irql);
    KeReleaseSpinLockFromDpcLevel( lock );
}

/***********************************************************************
 *           KeAcquireSpinLockRaiseToDpc (NTOSKRNL.EXE.@)
 */
KIRQL WINAPI KeAcquireSpinLockRaiseToDpc( KSPIN_LOCK *lock )
{
    TRACE("lock %p.\n", lock);
    KeAcquireSpinLockAtDpcLevel( lock );
    return 0;
}

/***********************************************************************
 *           KeAcquireInStackQueuedSpinLock (NTOSKRNL.EXE.@)
 */
void WINAPI KeAcquireInStackQueuedSpinLock( KSPIN_LOCK *lock, KLOCK_QUEUE_HANDLE *queue )
{
    TRACE("lock %p, queue %p.\n", lock, queue);
    KeAcquireInStackQueuedSpinLockAtDpcLevel( lock, queue );
}

/***********************************************************************
 *           KeReleaseInStackQueuedSpinLock (NTOSKRNL.EXE.@)
 */
void WINAPI KeReleaseInStackQueuedSpinLock( KLOCK_QUEUE_HANDLE *queue )
{
    TRACE("queue %p.\n", queue);
    KeReleaseInStackQueuedSpinLockFromDpcLevel( queue );
}
#endif

static KSPIN_LOCK cancel_lock;

/***********************************************************************
 *           IoAcquireCancelSpinLock  (NTOSKRNL.EXE.@)
 */
void WINAPI IoAcquireCancelSpinLock( KIRQL *irql )
{
    TRACE("irql %p.\n", irql);
    KeAcquireSpinLock( &cancel_lock, irql );
}

/***********************************************************************
 *           IoReleaseCancelSpinLock  (NTOSKRNL.EXE.@)
 */
void WINAPI IoReleaseCancelSpinLock( KIRQL irql )
{
    TRACE("irql %u.\n", irql);
    KeReleaseSpinLock( &cancel_lock, irql );
}

#ifdef __i386__
DEFINE_FASTCALL2_ENTRYPOINT( ExfInterlockedRemoveHeadList )
PLIST_ENTRY WINAPI DECLSPEC_HIDDEN __regs_ExfInterlockedRemoveHeadList( LIST_ENTRY *list, KSPIN_LOCK *lock )
{
    return ExInterlockedRemoveHeadList( list, lock );
}
#endif

/***********************************************************************
 *           ExInterlockedRemoveHeadList  (NTOSKRNL.EXE.@)
 */
LIST_ENTRY * WINAPI ExInterlockedRemoveHeadList( LIST_ENTRY *list, KSPIN_LOCK *lock )
{
    LIST_ENTRY *ret;
    KIRQL irql;

    TRACE("list %p, lock %p.\n", list, lock);

    KeAcquireSpinLock( lock, &irql );
    ret = RemoveHeadList( list );
    KeReleaseSpinLock( lock, irql );

    return ret;
}

#ifndef _WIN64

/***********************************************************************
 *           InterlockedPopEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( NTOSKRNL_InterlockedPopEntrySList )
PSLIST_ENTRY WINAPI DECLSPEC_HIDDEN __regs_NTOSKRNL_InterlockedPopEntrySList( PSLIST_HEADER list )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_InterlockedPopEntrySList( PSLIST_HEADER list )
#endif
{
    return RtlInterlockedPopEntrySList( list );
}


/***********************************************************************
 *           InterlockedPushEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( NTOSKRNL_InterlockedPushEntrySList )
PSLIST_ENTRY WINAPI DECLSPEC_HIDDEN __regs_NTOSKRNL_InterlockedPushEntrySList( PSLIST_HEADER list,
                                                                               PSLIST_ENTRY entry )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_InterlockedPushEntrySList( PSLIST_HEADER list, PSLIST_ENTRY entry )
#endif
{
    return RtlInterlockedPushEntrySList( list, entry );
}

#ifdef __i386__

/***********************************************************************
 *           ExInterlockedPopEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( NTOSKRNL_ExInterlockedPopEntrySList )
PSLIST_ENTRY WINAPI __regs_NTOSKRNL_ExInterlockedPopEntrySList( PSLIST_HEADER list, PKSPIN_LOCK lock )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_ExInterlockedPopEntrySList( PSLIST_HEADER list, PKSPIN_LOCK lock )
#endif
{
    return RtlInterlockedPopEntrySList( list );
}

/***********************************************************************
 *           ExInterlockedPushEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL3_ENTRYPOINT
DEFINE_FASTCALL3_ENTRYPOINT( NTOSKRNL_ExInterlockedPushEntrySList )
PSLIST_ENTRY WINAPI DECLSPEC_HIDDEN __regs_NTOSKRNL_ExInterlockedPushEntrySList( PSLIST_HEADER list,
                                                                                 PSLIST_ENTRY entry,
                                                                                 PKSPIN_LOCK lock )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_ExInterlockedPushEntrySList( PSLIST_HEADER list, PSLIST_ENTRY entry, PKSPIN_LOCK lock )
#endif
{
    return RtlInterlockedPushEntrySList( list, entry );
}

#endif /* __i386__ */
#endif /* _WIN64 */

/***********************************************************************
 *           ExAcquireFastMutexUnsafe  (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT(ExAcquireFastMutexUnsafe)
void WINAPI __regs_ExAcquireFastMutexUnsafe( FAST_MUTEX *mutex )
#else
void WINAPI ExAcquireFastMutexUnsafe( FAST_MUTEX *mutex )
#endif
{
    LONG count;

    TRACE("mutex %p.\n", mutex);

    count = InterlockedDecrement( &mutex->Count );
    if (count < 0)
        KeWaitForSingleObject( &mutex->Event, Executive, KernelMode, FALSE, NULL );
}

/***********************************************************************
 *           ExReleaseFastMutexUnsafe  (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT(ExReleaseFastMutexUnsafe)
void WINAPI __regs_ExReleaseFastMutexUnsafe( FAST_MUTEX *mutex )
#else
void WINAPI ExReleaseFastMutexUnsafe( FAST_MUTEX *mutex )
#endif
{
    LONG count;

    TRACE("mutex %p.\n", mutex);

    count = InterlockedIncrement( &mutex->Count );
    if (count < 1)
        KeSetEvent( &mutex->Event, IO_NO_INCREMENT, FALSE );
}
