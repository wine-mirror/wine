/*
 * Thread pooling
 *
 * Copyright (c) 2006 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);

struct work_item
{
    PRTL_WORK_ITEM_ROUTINE function;
    PVOID context;
};

static void WINAPI worker_thread_proc(void * param)
{
    struct work_item *work_item_ptr = (struct work_item *)param;
    struct work_item work_item;

    /* free the work item memory sooner to reduce memory usage */
    work_item = *work_item_ptr;
    RtlFreeHeap(GetProcessHeap(), 0, work_item_ptr);

    TRACE("executing %p(%p)\n", work_item.function, work_item.context);

    /* do the work */
    work_item.function(work_item.context);

    RtlExitUserThread(0);

    /* never reached */
}

/***********************************************************************
 *              RtlQueueWorkItem   (NTDLL.@)
 *
 * Queues a work item into a thread in the thread pool.
 *
 * PARAMS
 *  Function [I] Work function to execute.
 *  Context  [I] Context to pass to the work function when it is executed.
 *  Flags    [I] Flags. See notes.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 * NOTES
 *  Flags can be one or more of the following:
 *|WT_EXECUTEDEFAULT - Executes the work item in a non-I/O worker thread.
 *|WT_EXECUTEINIOTHREAD - Executes the work item in an I/O worker thread.
 *|WT_EXECUTEINPERSISTENTTHREAD - Executes the work item in a thread that is persistent.
 *|WT_EXECUTELONGFUNCTION - Hints that the execution can take a long time.
 *|WT_TRANSFER_IMPERSONATION - Executes the function with the current access token.
 */
NTSTATUS WINAPI RtlQueueWorkItem(PRTL_WORK_ITEM_ROUTINE Function, PVOID Context, ULONG Flags)
{
    HANDLE thread;
    NTSTATUS status;
    struct work_item *work_item = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(struct work_item));

    if (!work_item)
        return STATUS_NO_MEMORY;

    work_item->function = Function;
    work_item->context = Context;

    if (Flags != WT_EXECUTEDEFAULT)
        FIXME("Flags 0x%lx not supported\n", Flags);

    /* FIXME: very crude implementation that doesn't support pooling at all */
    status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE,
                                  NULL, 0, 0,
                                  worker_thread_proc, work_item, &thread, NULL );
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap(GetProcessHeap(), 0, work_item);
        return status;
    }
    NtClose( thread );

    return STATUS_SUCCESS;
}
