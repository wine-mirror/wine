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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <limits.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);

#define WORKER_TIMEOUT 30000 /* 30 seconds */

static LONG num_workers;
static LONG num_work_items;
static LONG num_busy_workers;

static struct list work_item_list = LIST_INIT(work_item_list);
static HANDLE work_item_event;

static RTL_CRITICAL_SECTION threadpool_cs;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &threadpool_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": threadpool_cs") }
};
static RTL_CRITICAL_SECTION threadpool_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static HANDLE compl_port = NULL;
static RTL_CRITICAL_SECTION threadpool_compl_cs;
static RTL_CRITICAL_SECTION_DEBUG critsect_compl_debug =
{
    0, 0, &threadpool_compl_cs,
    { &critsect_compl_debug.ProcessLocksList, &critsect_compl_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": threadpool_compl_cs") }
};
static RTL_CRITICAL_SECTION threadpool_compl_cs = { &critsect_compl_debug, -1, 0, 0, 0, 0 };

struct work_item
{
    struct list entry;
    PRTL_WORK_ITEM_ROUTINE function;
    PVOID context;
};

static inline LONG interlocked_inc( PLONG dest )
{
    return interlocked_xchg_add( dest, 1 ) + 1;
}

static inline LONG interlocked_dec( PLONG dest )
{
    return interlocked_xchg_add( dest, -1 ) - 1;
}

static void WINAPI worker_thread_proc(void * param)
{
    interlocked_inc(&num_workers);

    /* free the work item memory sooner to reduce memory usage */
    while (TRUE)
    {
        if (num_work_items > 0)
        {
            struct list *item;
            RtlEnterCriticalSection(&threadpool_cs);
            item = list_head(&work_item_list);
            if (item)
            {
                struct work_item *work_item_ptr = LIST_ENTRY(item, struct work_item, entry);
                struct work_item work_item;
                list_remove(&work_item_ptr->entry);
                interlocked_dec(&num_work_items);

                RtlLeaveCriticalSection(&threadpool_cs);

                work_item = *work_item_ptr;
                RtlFreeHeap(GetProcessHeap(), 0, work_item_ptr);

                TRACE("executing %p(%p)\n", work_item.function, work_item.context);

                interlocked_inc(&num_busy_workers);

                /* do the work */
                work_item.function(work_item.context);

                interlocked_dec(&num_busy_workers);
            }
            else
                RtlLeaveCriticalSection(&threadpool_cs);
        }
        else
        {
            NTSTATUS status;
            LARGE_INTEGER timeout;
            timeout.QuadPart = -(WORKER_TIMEOUT * (ULONGLONG)10000);
            status = NtWaitForSingleObject(work_item_event, FALSE, &timeout);
            if (status != STATUS_WAIT_0)
                break;
        }
    }

    interlocked_dec(&num_workers);

    RtlExitUserThread(0);

    /* never reached */
}

static NTSTATUS add_work_item_to_queue(struct work_item *work_item)
{
    NTSTATUS status;

    RtlEnterCriticalSection(&threadpool_cs);
    list_add_tail(&work_item_list, &work_item->entry);
    num_work_items++;
    RtlLeaveCriticalSection(&threadpool_cs);

    if (!work_item_event)
    {
        HANDLE sem;
        status = NtCreateSemaphore(&sem, SEMAPHORE_ALL_ACCESS, NULL, 1, LONG_MAX);
        if (interlocked_cmpxchg_ptr( &work_item_event, sem, 0 ))
            NtClose(sem);  /* somebody beat us to it */
    }
    else
        status = NtReleaseSemaphore(work_item_event, 1, NULL);

    return status;
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

    if (Flags & ~WT_EXECUTELONGFUNCTION)
        FIXME("Flags 0x%x not supported\n", Flags);

    status = add_work_item_to_queue(work_item);

    /* FIXME: tune this algorithm to not be as aggressive with creating threads
     * if WT_EXECUTELONGFUNCTION isn't specified */
    if ((status == STATUS_SUCCESS) &&
        ((num_workers == 0) || (num_workers == num_busy_workers)))
    {
        status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE,
                                    NULL, 0, 0,
                                    worker_thread_proc, NULL, &thread, NULL );
        if (status == STATUS_SUCCESS)
            NtClose( thread );

        /* NOTE: we don't care if we couldn't create the thread if there is at
         * least one other available to process the request */
        if ((num_workers > 0) && (status != STATUS_SUCCESS))
            status = STATUS_SUCCESS;
    }

    if (status != STATUS_SUCCESS)
    {
        RtlEnterCriticalSection(&threadpool_cs);

        interlocked_dec(&num_work_items);
        list_remove(&work_item->entry);
        RtlFreeHeap(GetProcessHeap(), 0, work_item);

        RtlLeaveCriticalSection(&threadpool_cs);

        return status;
    }

    return STATUS_SUCCESS;
}

/***********************************************************************
 * iocp_poller - get completion events and run callbacks
 */
static DWORD CALLBACK iocp_poller(LPVOID Arg)
{
    while( TRUE )
    {
        PRTL_OVERLAPPED_COMPLETION_ROUTINE callback;
        LPVOID overlapped;
        IO_STATUS_BLOCK iosb;
        NTSTATUS res = NtRemoveIoCompletion( compl_port, (PULONG_PTR)&callback, (PULONG_PTR)&overlapped, &iosb, NULL );
        if (res)
        {
            ERR("NtRemoveIoCompletion failed: 0x%x\n", res);
        }
        else
        {
            DWORD transferred = 0;
            DWORD err = 0;

            if (iosb.u.Status == STATUS_SUCCESS)
                transferred = iosb.Information;
            else
                err = RtlNtStatusToDosError(iosb.u.Status);

            callback( err, transferred, overlapped );
        }
    }
}

/***********************************************************************
 *              RtlSetIoCompletionCallback  (NTDLL.@)
 *
 * Binds a handle to a thread pool's completion port, and possibly
 * starts a non-I/O thread to monitor this port and call functions back.
 *
 * PARAMS
 *  FileHandle [I] Handle to bind to a completion port.
 *  Function   [I] Callback function to call on I/O completions.
 *  Flags      [I] Not used.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 */
NTSTATUS WINAPI RtlSetIoCompletionCallback(HANDLE FileHandle, PRTL_OVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags)
{
    IO_STATUS_BLOCK iosb;
    FILE_COMPLETION_INFORMATION info;

    if (Flags) FIXME("Unknown value Flags=0x%x\n", Flags);

    if (!compl_port)
    {
        NTSTATUS res = STATUS_SUCCESS;

        RtlEnterCriticalSection(&threadpool_compl_cs);
        if (!compl_port)
        {
            HANDLE cport;

            res = NtCreateIoCompletion( &cport, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
            if (!res)
            {
                /* FIXME native can start additional threads in case of e.g. hung callback function. */
                res = RtlQueueWorkItem( iocp_poller, NULL, WT_EXECUTEDEFAULT );
                if (!res)
                    compl_port = cport;
                else
                    NtClose( cport );
            }
        }
        RtlLeaveCriticalSection(&threadpool_compl_cs);
        if (res) return res;
    }

    info.CompletionPort = compl_port;
    info.CompletionKey = (ULONG_PTR)Function;

    return NtSetInformationFile( FileHandle, &iosb, &info, sizeof(info), FileCompletionInformation );
}

static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

struct wait_work_item
{
    HANDLE Object;
    HANDLE CancelEvent;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG Milliseconds;
    ULONG Flags;
    HANDLE CompletionEvent;
    LONG DeleteCount;
    BOOLEAN CallbackInProgress;
};

static void delete_wait_work_item(struct wait_work_item *wait_work_item)
{
    NtClose( wait_work_item->CancelEvent );
    RtlFreeHeap( GetProcessHeap(), 0, wait_work_item );
}

static DWORD CALLBACK wait_thread_proc(LPVOID Arg)
{
    struct wait_work_item *wait_work_item = Arg;
    NTSTATUS status;
    BOOLEAN alertable = (wait_work_item->Flags & WT_EXECUTEINIOTHREAD) ? TRUE : FALSE;
    HANDLE handles[2] = { wait_work_item->Object, wait_work_item->CancelEvent };
    LARGE_INTEGER timeout;
    HANDLE completion_event;

    TRACE("\n");

    while (TRUE)
    {
        status = NtWaitForMultipleObjects( 2, handles, FALSE, alertable,
                                           get_nt_timeout( &timeout, wait_work_item->Milliseconds ) );
        if (status == STATUS_WAIT_0 || status == STATUS_TIMEOUT)
        {
            BOOLEAN TimerOrWaitFired;

            if (status == STATUS_WAIT_0)
            {
                TRACE( "object %p signaled, calling callback %p with context %p\n",
                    wait_work_item->Object, wait_work_item->Callback,
                    wait_work_item->Context );
                TimerOrWaitFired = FALSE;
            }
            else
            {
                TRACE( "wait for object %p timed out, calling callback %p with context %p\n",
                    wait_work_item->Object, wait_work_item->Callback,
                    wait_work_item->Context );
                TimerOrWaitFired = TRUE;
            }
            wait_work_item->CallbackInProgress = TRUE;
            wait_work_item->Callback( wait_work_item->Context, TimerOrWaitFired );
            wait_work_item->CallbackInProgress = FALSE;

            if (wait_work_item->Flags & WT_EXECUTEONLYONCE)
                break;
        }
        else
            break;
    }

    completion_event = wait_work_item->CompletionEvent;
    if (completion_event) NtSetEvent( completion_event, NULL );

    if (interlocked_inc( &wait_work_item->DeleteCount ) == 2 )
        delete_wait_work_item( wait_work_item );

    return 0;
}

/***********************************************************************
 *              RtlRegisterWait   (NTDLL.@)
 *
 * Registers a wait for a handle to become signaled.
 *
 * PARAMS
 *  NewWaitObject [I] Handle to the new wait object. Use RtlDeregisterWait() to free it.
 *  Object   [I] Object to wait to become signaled.
 *  Callback [I] Callback function to execute when the wait times out or the handle is signaled.
 *  Context  [I] Context to pass to the callback function when it is executed.
 *  Milliseconds [I] Number of milliseconds to wait before timing out.
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
NTSTATUS WINAPI RtlRegisterWait(PHANDLE NewWaitObject, HANDLE Object,
                                RTL_WAITORTIMERCALLBACKFUNC Callback,
                                PVOID Context, ULONG Milliseconds, ULONG Flags)
{
    struct wait_work_item *wait_work_item;
    NTSTATUS status;

    TRACE( "(%p, %p, %p, %p, %d, 0x%x)\n", NewWaitObject, Object, Callback, Context, Milliseconds, Flags );

    wait_work_item = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*wait_work_item) );
    if (!wait_work_item)
        return STATUS_NO_MEMORY;

    wait_work_item->Object = Object;
    wait_work_item->Callback = Callback;
    wait_work_item->Context = Context;
    wait_work_item->Milliseconds = Milliseconds;
    wait_work_item->Flags = Flags;
    wait_work_item->CallbackInProgress = FALSE;
    wait_work_item->DeleteCount = 0;
    wait_work_item->CompletionEvent = NULL;

    status = NtCreateEvent( &wait_work_item->CancelEvent, EVENT_ALL_ACCESS, NULL, TRUE, FALSE );
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap( GetProcessHeap(), 0, wait_work_item );
        return status;
    }

    status = RtlQueueWorkItem( wait_thread_proc, wait_work_item, Flags & ~WT_EXECUTEONLYONCE );
    if (status != STATUS_SUCCESS)
    {
        delete_wait_work_item( wait_work_item );
        return status;
    }

    *NewWaitObject = wait_work_item;
    return status;
}

/***********************************************************************
 *              RtlDeregisterWaitEx   (NTDLL.@)
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeregisterWaitEx(HANDLE WaitHandle, HANDLE CompletionEvent)
{
    struct wait_work_item *wait_work_item = WaitHandle;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "(%p)\n", WaitHandle );

    NtSetEvent( wait_work_item->CancelEvent, NULL );
    if (wait_work_item->CallbackInProgress)
    {
        if (CompletionEvent != NULL)
        {
            if (CompletionEvent == INVALID_HANDLE_VALUE)
            {
                status = NtCreateEvent( &CompletionEvent, EVENT_ALL_ACCESS, NULL, TRUE, FALSE );
                if (status != STATUS_SUCCESS)
                    return status;
                interlocked_xchg_ptr( &wait_work_item->CompletionEvent, CompletionEvent );
                if (wait_work_item->CallbackInProgress)
                    NtWaitForSingleObject( CompletionEvent, FALSE, NULL );
                NtClose( CompletionEvent );
            }
            else
            {
                interlocked_xchg_ptr( &wait_work_item->CompletionEvent, CompletionEvent );
                if (wait_work_item->CallbackInProgress)
                    status = STATUS_PENDING;
            }
        }
        else
            status = STATUS_PENDING;
    }

    if (interlocked_inc( &wait_work_item->DeleteCount ) == 2 )
    {
        status = STATUS_SUCCESS;
        delete_wait_work_item( wait_work_item );
    }

    return status;
}

/***********************************************************************
 *              RtlDeregisterWait   (NTDLL.@)
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeregisterWait(HANDLE WaitHandle)
{
    return RtlDeregisterWaitEx(WaitHandle, NULL);
}
