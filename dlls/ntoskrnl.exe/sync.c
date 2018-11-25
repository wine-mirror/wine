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
