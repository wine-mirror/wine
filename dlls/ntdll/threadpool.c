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

#include <assert.h>
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

/* threadpool_cs must be held while modifying the following elements */
static struct list work_item_list = LIST_INIT(work_item_list);
static LONG num_workers;
static LONG num_busy_workers;
static LONG num_items_processed;

static RTL_CONDITION_VARIABLE threadpool_cond = RTL_CONDITION_VARIABLE_INIT;

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
    struct list *item;
    struct work_item *work_item_ptr, work_item;
    LARGE_INTEGER timeout;
    timeout.QuadPart = -(WORKER_TIMEOUT * (ULONGLONG)10000);

    RtlEnterCriticalSection( &threadpool_cs );
    num_workers++;

    for (;;)
    {
        if ((item = list_head( &work_item_list )))
        {
            work_item_ptr = LIST_ENTRY( item, struct work_item, entry );
            list_remove( &work_item_ptr->entry );
            num_busy_workers++;
            num_items_processed++;
            RtlLeaveCriticalSection( &threadpool_cs );

            /* copy item to stack and do the work */
            work_item = *work_item_ptr;
            RtlFreeHeap( GetProcessHeap(), 0, work_item_ptr );
            TRACE("executing %p(%p)\n", work_item.function, work_item.context);
            work_item.function( work_item.context );

            RtlEnterCriticalSection( &threadpool_cs );
            num_busy_workers--;
        }
        else if (RtlSleepConditionVariableCS( &threadpool_cond, &threadpool_cs, &timeout ) != STATUS_SUCCESS)
            break;
    }

    num_workers--;
    RtlLeaveCriticalSection( &threadpool_cs );
    RtlExitUserThread( 0 );

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
    LONG items_processed;
    struct work_item *work_item = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(struct work_item));

    if (!work_item)
        return STATUS_NO_MEMORY;

    work_item->function = Function;
    work_item->context = Context;

    if (Flags & ~WT_EXECUTELONGFUNCTION)
        FIXME("Flags 0x%x not supported\n", Flags);

    RtlEnterCriticalSection( &threadpool_cs );
    list_add_tail( &work_item_list, &work_item->entry );
    status = (num_workers > num_busy_workers) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    items_processed = num_items_processed;
    RtlLeaveCriticalSection( &threadpool_cs );

    /* FIXME: tune this algorithm to not be as aggressive with creating threads
     * if WT_EXECUTELONGFUNCTION isn't specified */
    if (status == STATUS_SUCCESS)
        RtlWakeConditionVariable( &threadpool_cond );
    else
    {
        status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, NULL, 0, 0,
                                      worker_thread_proc, NULL, &thread, NULL );

        /* NOTE: we don't care if we couldn't create the thread if there is at
         * least one other available to process the request */
        if (status == STATUS_SUCCESS)
            NtClose( thread );
        else
        {
            RtlEnterCriticalSection( &threadpool_cs );
            if (num_workers > 0 || num_items_processed != items_processed)
                status = STATUS_SUCCESS;
            else
                list_remove( &work_item->entry );
            RtlLeaveCriticalSection( &threadpool_cs );

            if (status != STATUS_SUCCESS)
                RtlFreeHeap( GetProcessHeap(), 0, work_item );
        }
    }

    return status;
}

/***********************************************************************
 * iocp_poller - get completion events and run callbacks
 */
static DWORD CALLBACK iocp_poller(LPVOID Arg)
{
    HANDLE cport = Arg;

    while( TRUE )
    {
        PRTL_OVERLAPPED_COMPLETION_ROUTINE callback;
        LPVOID overlapped;
        IO_STATUS_BLOCK iosb;
        NTSTATUS res = NtRemoveIoCompletion( cport, (PULONG_PTR)&callback, (PULONG_PTR)&overlapped, &iosb, NULL );
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
    return 0;
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
                res = RtlQueueWorkItem( iocp_poller, cport, WT_EXECUTEDEFAULT );
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
    BOOLEAN alertable = (wait_work_item->Flags & WT_EXECUTEINIOTHREAD) != 0;
    HANDLE handles[2] = { wait_work_item->Object, wait_work_item->CancelEvent };
    LARGE_INTEGER timeout;
    HANDLE completion_event;

    TRACE("\n");

    while (TRUE)
    {
        status = NtWaitForMultipleObjects( 2, handles, TRUE, alertable,
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

    status = NtCreateEvent( &wait_work_item->CancelEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE );
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap( GetProcessHeap(), 0, wait_work_item );
        return status;
    }

    Flags = Flags & (WT_EXECUTEINIOTHREAD | WT_EXECUTEINPERSISTENTTHREAD |
                     WT_EXECUTELONGFUNCTION | WT_TRANSFER_IMPERSONATION);
    status = RtlQueueWorkItem( wait_thread_proc, wait_work_item, Flags );
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
                status = NtCreateEvent( &CompletionEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE );
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


/************************** Timer Queue Impl **************************/

struct timer_queue;
struct queue_timer
{
    struct timer_queue *q;
    struct list entry;
    ULONG runcount;             /* number of callbacks pending execution */
    RTL_WAITORTIMERCALLBACKFUNC callback;
    PVOID param;
    DWORD period;
    ULONG flags;
    ULONGLONG expire;
    BOOL destroy;      /* timer should be deleted; once set, never unset */
    HANDLE event;      /* removal event */
};

struct timer_queue
{
    DWORD magic;
    RTL_CRITICAL_SECTION cs;
    struct list timers;          /* sorted by expiration time */
    BOOL quit;         /* queue should be deleted; once set, never unset */
    HANDLE event;
    HANDLE thread;
};

#define EXPIRE_NEVER (~(ULONGLONG) 0)
#define TIMER_QUEUE_MAGIC 0x516d6954  /* TimQ */

static void queue_remove_timer(struct queue_timer *t)
{
    /* We MUST hold the queue cs while calling this function.  This ensures
       that we cannot queue another callback for this timer.  The runcount
       being zero makes sure we don't have any already queued.  */
    struct timer_queue *q = t->q;

    assert(t->runcount == 0);
    assert(t->destroy);

    list_remove(&t->entry);
    if (t->event)
        NtSetEvent(t->event, NULL);
    RtlFreeHeap(GetProcessHeap(), 0, t);

    if (q->quit && list_empty(&q->timers))
        NtSetEvent(q->event, NULL);
}

static void timer_cleanup_callback(struct queue_timer *t)
{
    struct timer_queue *q = t->q;
    RtlEnterCriticalSection(&q->cs);

    assert(0 < t->runcount);
    --t->runcount;

    if (t->destroy && t->runcount == 0)
        queue_remove_timer(t);

    RtlLeaveCriticalSection(&q->cs);
}

static DWORD WINAPI timer_callback_wrapper(LPVOID p)
{
    struct queue_timer *t = p;
    t->callback(t->param, TRUE);
    timer_cleanup_callback(t);
    return 0;
}

static inline ULONGLONG queue_current_time(void)
{
    LARGE_INTEGER now, freq;
    NtQueryPerformanceCounter(&now, &freq);
    return now.QuadPart * 1000 / freq.QuadPart;
}

static void queue_add_timer(struct queue_timer *t, ULONGLONG time,
                            BOOL set_event)
{
    /* We MUST hold the queue cs while calling this function.  */
    struct timer_queue *q = t->q;
    struct list *ptr = &q->timers;

    assert(!q->quit || (t->destroy && time == EXPIRE_NEVER));

    if (time != EXPIRE_NEVER)
        LIST_FOR_EACH(ptr, &q->timers)
        {
            struct queue_timer *cur = LIST_ENTRY(ptr, struct queue_timer, entry);
            if (time < cur->expire)
                break;
        }
    list_add_before(ptr, &t->entry);

    t->expire = time;

    /* If we insert at the head of the list, we need to expire sooner
       than expected.  */
    if (set_event && &t->entry == list_head(&q->timers))
        NtSetEvent(q->event, NULL);
}

static inline void queue_move_timer(struct queue_timer *t, ULONGLONG time,
                                    BOOL set_event)
{
    /* We MUST hold the queue cs while calling this function.  */
    list_remove(&t->entry);
    queue_add_timer(t, time, set_event);
}

static void queue_timer_expire(struct timer_queue *q)
{
    struct queue_timer *t = NULL;

    RtlEnterCriticalSection(&q->cs);
    if (list_head(&q->timers))
    {
        ULONGLONG now, next;
        t = LIST_ENTRY(list_head(&q->timers), struct queue_timer, entry);
        if (!t->destroy && t->expire <= ((now = queue_current_time())))
        {
            ++t->runcount;
            if (t->period)
            {
                next = t->expire + t->period;
                /* avoid trigger cascade if overloaded / hibernated */
                if (next < now)
                    next = now + t->period;
            }
            else
                next = EXPIRE_NEVER;
            queue_move_timer(t, next, FALSE);
        }
        else
            t = NULL;
    }
    RtlLeaveCriticalSection(&q->cs);

    if (t)
    {
        if (t->flags & WT_EXECUTEINTIMERTHREAD)
            timer_callback_wrapper(t);
        else
        {
            ULONG flags
                = (t->flags
                   & (WT_EXECUTEINIOTHREAD | WT_EXECUTEINPERSISTENTTHREAD
                      | WT_EXECUTELONGFUNCTION | WT_TRANSFER_IMPERSONATION));
            NTSTATUS status = RtlQueueWorkItem(timer_callback_wrapper, t, flags);
            if (status != STATUS_SUCCESS)
                timer_cleanup_callback(t);
        }
    }
}

static ULONG queue_get_timeout(struct timer_queue *q)
{
    struct queue_timer *t;
    ULONG timeout = INFINITE;

    RtlEnterCriticalSection(&q->cs);
    if (list_head(&q->timers))
    {
        t = LIST_ENTRY(list_head(&q->timers), struct queue_timer, entry);
        assert(!t->destroy || t->expire == EXPIRE_NEVER);

        if (t->expire != EXPIRE_NEVER)
        {
            ULONGLONG time = queue_current_time();
            timeout = t->expire < time ? 0 : t->expire - time;
        }
    }
    RtlLeaveCriticalSection(&q->cs);

    return timeout;
}

static void WINAPI timer_queue_thread_proc(LPVOID p)
{
    struct timer_queue *q = p;
    ULONG timeout_ms;

    timeout_ms = INFINITE;
    for (;;)
    {
        LARGE_INTEGER timeout;
        NTSTATUS status;
        BOOL done = FALSE;

        status = NtWaitForSingleObject(
            q->event, FALSE, get_nt_timeout(&timeout, timeout_ms));

        if (status == STATUS_WAIT_0)
        {
            /* There are two possible ways to trigger the event.  Either
               we are quitting and the last timer got removed, or a new
               timer got put at the head of the list so we need to adjust
               our timeout.  */
            RtlEnterCriticalSection(&q->cs);
            if (q->quit && list_empty(&q->timers))
                done = TRUE;
            RtlLeaveCriticalSection(&q->cs);
        }
        else if (status == STATUS_TIMEOUT)
            queue_timer_expire(q);

        if (done)
            break;

        timeout_ms = queue_get_timeout(q);
    }

    NtClose(q->event);
    RtlDeleteCriticalSection(&q->cs);
    q->magic = 0;
    RtlFreeHeap(GetProcessHeap(), 0, q);
}

static void queue_destroy_timer(struct queue_timer *t)
{
    /* We MUST hold the queue cs while calling this function.  */
    t->destroy = TRUE;
    if (t->runcount == 0)
        /* Ensure a timer is promptly removed.  If callbacks are pending,
           it will be removed after the last one finishes by the callback
           cleanup wrapper.  */
        queue_remove_timer(t);
    else
        /* Make sure no destroyed timer masks an active timer at the head
           of the sorted list.  */
        queue_move_timer(t, EXPIRE_NEVER, FALSE);
}

/***********************************************************************
 *              RtlCreateTimerQueue   (NTDLL.@)
 *
 * Creates a timer queue object and returns a handle to it.
 *
 * PARAMS
 *  NewTimerQueue [O] The newly created queue.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlCreateTimerQueue(PHANDLE NewTimerQueue)
{
    NTSTATUS status;
    struct timer_queue *q = RtlAllocateHeap(GetProcessHeap(), 0, sizeof *q);
    if (!q)
        return STATUS_NO_MEMORY;

    RtlInitializeCriticalSection(&q->cs);
    list_init(&q->timers);
    q->quit = FALSE;
    q->magic = TIMER_QUEUE_MAGIC;
    status = NtCreateEvent(&q->event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (status != STATUS_SUCCESS)
    {
        RtlFreeHeap(GetProcessHeap(), 0, q);
        return status;
    }
    status = RtlCreateUserThread(GetCurrentProcess(), NULL, FALSE, NULL, 0, 0,
                                 timer_queue_thread_proc, q, &q->thread, NULL);
    if (status != STATUS_SUCCESS)
    {
        NtClose(q->event);
        RtlFreeHeap(GetProcessHeap(), 0, q);
        return status;
    }

    *NewTimerQueue = q;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlDeleteTimerQueueEx   (NTDLL.@)
 *
 * Deletes a timer queue object.
 *
 * PARAMS
 *  TimerQueue      [I] The timer queue to destroy.
 *  CompletionEvent [I] If NULL, return immediately.  If INVALID_HANDLE_VALUE,
 *                      wait until all timers are finished firing before
 *                      returning.  Otherwise, return immediately and set the
 *                      event when all timers are done.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS if synchronous, STATUS_PENDING if not.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeleteTimerQueueEx(HANDLE TimerQueue, HANDLE CompletionEvent)
{
    struct timer_queue *q = TimerQueue;
    struct queue_timer *t, *temp;
    HANDLE thread;
    NTSTATUS status;

    if (!q || q->magic != TIMER_QUEUE_MAGIC)
        return STATUS_INVALID_HANDLE;

    thread = q->thread;

    RtlEnterCriticalSection(&q->cs);
    q->quit = TRUE;
    if (list_head(&q->timers))
        /* When the last timer is removed, it will signal the timer thread to
           exit...  */
        LIST_FOR_EACH_ENTRY_SAFE(t, temp, &q->timers, struct queue_timer, entry)
            queue_destroy_timer(t);
    else
        /* However if we have none, we must do it ourselves.  */
        NtSetEvent(q->event, NULL);
    RtlLeaveCriticalSection(&q->cs);

    if (CompletionEvent == INVALID_HANDLE_VALUE)
    {
        NtWaitForSingleObject(thread, FALSE, NULL);
        status = STATUS_SUCCESS;
    }
    else
    {
        if (CompletionEvent)
        {
            FIXME("asynchronous return on completion event unimplemented\n");
            NtWaitForSingleObject(thread, FALSE, NULL);
            NtSetEvent(CompletionEvent, NULL);
        }
        status = STATUS_PENDING;
    }

    NtClose(thread);
    return status;
}

static struct timer_queue *default_timer_queue;

static struct timer_queue *get_timer_queue(HANDLE TimerQueue)
{
    if (TimerQueue)
        return TimerQueue;
    else
    {
        if (!default_timer_queue)
        {
            HANDLE q;
            NTSTATUS status = RtlCreateTimerQueue(&q);
            if (status == STATUS_SUCCESS)
            {
                PVOID p = interlocked_cmpxchg_ptr(
                    (void **) &default_timer_queue, q, NULL);
                if (p)
                    /* Got beat to the punch.  */
                    RtlDeleteTimerQueueEx(q, NULL);
            }
        }
        return default_timer_queue;
    }
}

/***********************************************************************
 *              RtlCreateTimer   (NTDLL.@)
 *
 * Creates a new timer associated with the given queue.
 *
 * PARAMS
 *  NewTimer   [O] The newly created timer.
 *  TimerQueue [I] The queue to hold the timer.
 *  Callback   [I] The callback to fire.
 *  Parameter  [I] The argument for the callback.
 *  DueTime    [I] The delay, in milliseconds, before first firing the
 *                 timer.
 *  Period     [I] The period, in milliseconds, at which to fire the timer
 *                 after the first callback.  If zero, the timer will only
 *                 fire once.  It still needs to be deleted with
 *                 RtlDeleteTimer.
 * Flags       [I] Flags controlling the execution of the callback.  In
 *                 addition to the WT_* thread pool flags (see
 *                 RtlQueueWorkItem), WT_EXECUTEINTIMERTHREAD and
 *                 WT_EXECUTEONLYONCE are supported.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlCreateTimer(PHANDLE NewTimer, HANDLE TimerQueue,
                               RTL_WAITORTIMERCALLBACKFUNC Callback,
                               PVOID Parameter, DWORD DueTime, DWORD Period,
                               ULONG Flags)
{
    NTSTATUS status;
    struct queue_timer *t;
    struct timer_queue *q = get_timer_queue(TimerQueue);

    if (!q) return STATUS_NO_MEMORY;
    if (q->magic != TIMER_QUEUE_MAGIC) return STATUS_INVALID_HANDLE;

    t = RtlAllocateHeap(GetProcessHeap(), 0, sizeof *t);
    if (!t)
        return STATUS_NO_MEMORY;

    t->q = q;
    t->runcount = 0;
    t->callback = Callback;
    t->param = Parameter;
    t->period = Period;
    t->flags = Flags;
    t->destroy = FALSE;
    t->event = NULL;

    status = STATUS_SUCCESS;
    RtlEnterCriticalSection(&q->cs);
    if (q->quit)
        status = STATUS_INVALID_HANDLE;
    else
        queue_add_timer(t, queue_current_time() + DueTime, TRUE);
    RtlLeaveCriticalSection(&q->cs);

    if (status == STATUS_SUCCESS)
        *NewTimer = t;
    else
        RtlFreeHeap(GetProcessHeap(), 0, t);

    return status;
}

/***********************************************************************
 *              RtlUpdateTimer   (NTDLL.@)
 *
 * Changes the time at which a timer expires.
 *
 * PARAMS
 *  TimerQueue [I] The queue that holds the timer.
 *  Timer      [I] The timer to update.
 *  DueTime    [I] The delay, in milliseconds, before next firing the timer.
 *  Period     [I] The period, in milliseconds, at which to fire the timer
 *                 after the first callback.  If zero, the timer will not
 *                 refire once.  It still needs to be deleted with
 *                 RtlDeleteTimer.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlUpdateTimer(HANDLE TimerQueue, HANDLE Timer,
                               DWORD DueTime, DWORD Period)
{
    struct queue_timer *t = Timer;
    struct timer_queue *q = t->q;

    RtlEnterCriticalSection(&q->cs);
    /* Can't change a timer if it was once-only or destroyed.  */
    if (t->expire != EXPIRE_NEVER)
    {
        t->period = Period;
        queue_move_timer(t, queue_current_time() + DueTime, TRUE);
    }
    RtlLeaveCriticalSection(&q->cs);

    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlDeleteTimer   (NTDLL.@)
 *
 * Cancels a timer-queue timer.
 *
 * PARAMS
 *  TimerQueue      [I] The queue that holds the timer.
 *  Timer           [I] The timer to update.
 *  CompletionEvent [I] If NULL, return immediately.  If INVALID_HANDLE_VALUE,
 *                      wait until the timer is finished firing all pending
 *                      callbacks before returning.  Otherwise, return
 *                      immediately and set the timer is done.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS if the timer is done, STATUS_PENDING if not,
             or if the completion event is NULL.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS WINAPI RtlDeleteTimer(HANDLE TimerQueue, HANDLE Timer,
                               HANDLE CompletionEvent)
{
    struct queue_timer *t = Timer;
    struct timer_queue *q;
    NTSTATUS status = STATUS_PENDING;
    HANDLE event = NULL;

    if (!Timer)
        return STATUS_INVALID_PARAMETER_1;
    q = t->q;
    if (CompletionEvent == INVALID_HANDLE_VALUE)
    {
        status = NtCreateEvent(&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        if (status == STATUS_SUCCESS)
            status = STATUS_PENDING;
    }
    else if (CompletionEvent)
        event = CompletionEvent;

    RtlEnterCriticalSection(&q->cs);
    t->event = event;
    if (t->runcount == 0 && event)
        status = STATUS_SUCCESS;
    queue_destroy_timer(t);
    RtlLeaveCriticalSection(&q->cs);

    if (CompletionEvent == INVALID_HANDLE_VALUE && event)
    {
        if (status == STATUS_PENDING)
        {
            NtWaitForSingleObject(event, FALSE, NULL);
            status = STATUS_SUCCESS;
        }
        NtClose(event);
    }

    return status;
}
