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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "ddk/wdm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);

enum object_type
{
    TYPE_MANUAL_EVENT = 0,
    TYPE_AUTO_EVENT = 1,
    TYPE_MUTEX = 2,
    TYPE_SEMAPHORE = 5,
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
            case TYPE_SEMAPHORE:
            {
                KSEMAPHORE *semaphore = CONTAINING_RECORD(objs[i], KSEMAPHORE, Header);
                objs[i]->WaitListHead.Blink = CreateSemaphoreW( NULL,
                    semaphore->Header.SignalState, semaphore->Limit, NULL );
                break;
            }
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
                objs[i]->SignalState = FALSE;
                break;
            case TYPE_SEMAPHORE:
                --objs[i]->SignalState;
                break;
            }
        }

        if (!--*((ULONG_PTR *)&objs[i]->WaitListHead.Flink))
        {
            CloseHandle(objs[i]->WaitListHead.Blink);
            objs[i]->WaitListHead.Blink = NULL;
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
    HANDLE handle = event->Header.WaitListHead.Blink;
    LONG ret;

    TRACE("event %p, increment %d, wait %u.\n", event, increment, wait);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchange( &event->Header.SignalState, TRUE );
    if (handle)
        SetEvent( handle );
    LeaveCriticalSection( &sync_cs );

    return ret;
}

/***********************************************************************
 *           KeResetEvent   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeResetEvent( PRKEVENT event )
{
    HANDLE handle = event->Header.WaitListHead.Blink;
    LONG ret;

    TRACE("event %p.\n", event);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchange( &event->Header.SignalState, FALSE );
    if (handle)
        ResetEvent( handle );
    LeaveCriticalSection( &sync_cs );

    return ret;
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
    HANDLE handle = semaphore->Header.WaitListHead.Blink;
    LONG ret;

    TRACE("semaphore %p, increment %d, count %d, wait %u.\n",
        semaphore, increment, count, wait);

    EnterCriticalSection( &sync_cs );
    ret = InterlockedExchangeAdd( &semaphore->Header.SignalState, count );
    if (handle)
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
