/*
 * Copyright 2019-2020 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION

#include "initguid.h"
#include "rtworkq.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define FIRST_USER_QUEUE_HANDLE 5
#define MAX_USER_QUEUE_HANDLES 124

#define WAIT_ITEM_KEY_MASK      (0x82000000)
#define SCHEDULED_ITEM_KEY_MASK (0x80000000)

static LONG next_item_key;

static RTWQWORKITEM_KEY get_item_key(DWORD mask, DWORD key)
{
    return ((RTWQWORKITEM_KEY)mask << 32) | key;
}

static RTWQWORKITEM_KEY generate_item_key(DWORD mask)
{
    return get_item_key(mask, InterlockedIncrement(&next_item_key));
}

struct queue_handle
{
    void *obj;
    LONG refcount;
    WORD generation;
};

static struct queue_handle user_queues[MAX_USER_QUEUE_HANDLES];
static struct queue_handle *next_free_user_queue;

static CRITICAL_SECTION queues_section;
static CRITICAL_SECTION_DEBUG queues_critsect_debug =
{
    0, 0, &queues_section,
    { &queues_critsect_debug.ProcessLocksList, &queues_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": queues_section") }
};
static CRITICAL_SECTION queues_section = { &queues_critsect_debug, -1, 0, 0, 0, 0 };

static LONG platform_lock;

static struct queue_handle *get_queue_obj(DWORD handle)
{
    unsigned int idx = HIWORD(handle) - FIRST_USER_QUEUE_HANDLE;

    if (idx < MAX_USER_QUEUE_HANDLES && user_queues[idx].refcount)
    {
        if (LOWORD(handle) == user_queues[idx].generation)
            return &user_queues[idx];
    }

    return NULL;
}

/* Should be kept in sync with corresponding MFASYNC_CALLBACK_ constants. */
enum rtwq_callback_queue_id
{
    RTWQ_CALLBACK_QUEUE_UNDEFINED     = 0x00000000,
    RTWQ_CALLBACK_QUEUE_STANDARD      = 0x00000001,
    RTWQ_CALLBACK_QUEUE_RT            = 0x00000002,
    RTWQ_CALLBACK_QUEUE_IO            = 0x00000003,
    RTWQ_CALLBACK_QUEUE_TIMER         = 0x00000004,
    RTWQ_CALLBACK_QUEUE_MULTITHREADED = 0x00000005,
    RTWQ_CALLBACK_QUEUE_LONG_FUNCTION = 0x00000007,
    RTWQ_CALLBACK_QUEUE_PRIVATE_MASK  = 0xffff0000,
    RTWQ_CALLBACK_QUEUE_ALL           = 0xffffffff,
};

enum system_queue_index
{
    SYS_QUEUE_STANDARD = 0,
    SYS_QUEUE_RT,
    SYS_QUEUE_IO,
    SYS_QUEUE_TIMER,
    SYS_QUEUE_MULTITHREADED,
    SYS_QUEUE_DO_NOT_USE,
    SYS_QUEUE_LONG_FUNCTION,
    SYS_QUEUE_COUNT,
};

struct work_item
{
    struct list entry;
    LONG refcount;
    IRtwqAsyncResult *result;
    struct queue *queue;
    RTWQWORKITEM_KEY key;
    union
    {
        TP_WAIT *wait_object;
        TP_TIMER *timer_object;
    } u;
};

static const TP_CALLBACK_PRIORITY priorities[] =
{
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
};

struct queue
{
    TP_POOL *pool;
    TP_CALLBACK_ENVIRON_V3 envs[ARRAY_SIZE(priorities)];
    CRITICAL_SECTION cs;
    struct list pending_items;
};

static struct queue system_queues[SYS_QUEUE_COUNT];

static struct queue *get_system_queue(DWORD queue_id)
{
    switch (queue_id)
    {
        case RTWQ_CALLBACK_QUEUE_STANDARD:
        case RTWQ_CALLBACK_QUEUE_RT:
        case RTWQ_CALLBACK_QUEUE_IO:
        case RTWQ_CALLBACK_QUEUE_TIMER:
        case RTWQ_CALLBACK_QUEUE_MULTITHREADED:
        case RTWQ_CALLBACK_QUEUE_LONG_FUNCTION:
            return &system_queues[queue_id - 1];
        default:
            return NULL;
    }
}

static void CALLBACK standard_queue_cleanup_callback(void *object_data, void *group_data)
{
}

static struct work_item * alloc_work_item(struct queue *queue, IRtwqAsyncResult *result)
{
    struct work_item *item;

    item = heap_alloc_zero(sizeof(*item));
    item->result = result;
    IRtwqAsyncResult_AddRef(item->result);
    item->refcount = 1;
    item->queue = queue;
    list_init(&item->entry);

    return item;
}

static void release_work_item(struct work_item *item)
{
    if (InterlockedDecrement(&item->refcount) == 0)
    {
         IRtwqAsyncResult_Release(item->result);
         heap_free(item);
    }
}

static struct work_item *grab_work_item(struct work_item *item)
{
    InterlockedIncrement(&item->refcount);
    return item;
}

static void init_work_queue(RTWQ_WORKQUEUE_TYPE queue_type, struct queue *queue)
{
    TP_CALLBACK_ENVIRON_V3 env;
    unsigned int max_thread, i;

    queue->pool = CreateThreadpool(NULL);

    memset(&env, 0, sizeof(env));
    env.Version = 3;
    env.Size = sizeof(env);
    env.Pool = queue->pool;
    env.CleanupGroup = CreateThreadpoolCleanupGroup();
    env.CleanupGroupCancelCallback = standard_queue_cleanup_callback;
    env.CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    for (i = 0; i < ARRAY_SIZE(queue->envs); ++i)
    {
        queue->envs[i] = env;
        queue->envs[i].CallbackPriority = priorities[i];
    }
    list_init(&queue->pending_items);
    InitializeCriticalSection(&queue->cs);

    max_thread = (queue_type == RTWQ_STANDARD_WORKQUEUE || queue_type == RTWQ_WINDOW_WORKQUEUE) ? 1 : 4;

    SetThreadpoolThreadMinimum(queue->pool, 1);
    SetThreadpoolThreadMaximum(queue->pool, max_thread);

    if (queue_type == RTWQ_WINDOW_WORKQUEUE)
        FIXME("RTWQ_WINDOW_WORKQUEUE is not supported.\n");
}

static HRESULT grab_queue(DWORD queue_id, struct queue **ret)
{
    struct queue *queue = get_system_queue(queue_id);
    RTWQ_WORKQUEUE_TYPE queue_type;
    struct queue_handle *entry;

    *ret = NULL;

    if (!system_queues[SYS_QUEUE_STANDARD].pool)
        return RTWQ_E_SHUTDOWN;

    if (queue && queue->pool)
    {
        *ret = queue;
        return S_OK;
    }
    else if (queue)
    {
        EnterCriticalSection(&queues_section);
        switch (queue_id)
        {
            case RTWQ_CALLBACK_QUEUE_IO:
            case RTWQ_CALLBACK_QUEUE_MULTITHREADED:
            case RTWQ_CALLBACK_QUEUE_LONG_FUNCTION:
                queue_type = RTWQ_MULTITHREADED_WORKQUEUE;
                break;
            default:
                queue_type = RTWQ_STANDARD_WORKQUEUE;
        }
        init_work_queue(queue_type, queue);
        LeaveCriticalSection(&queues_section);
        *ret = queue;
        return S_OK;
    }

    /* Handles user queues. */
    if ((entry = get_queue_obj(queue_id)))
        *ret = entry->obj;

    return *ret ? S_OK : RTWQ_E_INVALID_WORKQUEUE;
}

static HRESULT lock_user_queue(DWORD queue)
{
    HRESULT hr = RTWQ_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & RTWQ_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        entry->refcount++;
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

static void shutdown_queue(struct queue *queue)
{
    struct work_item *item, *item2;

    if (!queue->pool)
        return;

    CloseThreadpoolCleanupGroupMembers(queue->envs[0].CleanupGroup, TRUE, NULL);
    CloseThreadpool(queue->pool);
    queue->pool = NULL;

    EnterCriticalSection(&queue->cs);
    LIST_FOR_EACH_ENTRY_SAFE(item, item2, &queue->pending_items, struct work_item, entry)
    {
        list_remove(&item->entry);
        release_work_item(item);
    }
    LeaveCriticalSection(&queue->cs);

    DeleteCriticalSection(&queue->cs);
}

static HRESULT unlock_user_queue(DWORD queue)
{
    HRESULT hr = RTWQ_E_INVALID_WORKQUEUE;
    struct queue_handle *entry;

    if (!(queue & RTWQ_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        if (--entry->refcount == 0)
        {
            shutdown_queue((struct queue *)entry->obj);
            heap_free(entry->obj);
            entry->obj = next_free_user_queue;
            next_free_user_queue = entry;
        }
        hr = S_OK;
    }
    LeaveCriticalSection(&queues_section);
    return hr;
}

static void CALLBACK standard_queue_worker(TP_CALLBACK_INSTANCE *instance, void *context, TP_WORK *work)
{
    struct work_item *item = context;
    RTWQASYNCRESULT *result = (RTWQASYNCRESULT *)item->result;

    TRACE("result object %p.\n", result);

    IRtwqAsyncCallback_Invoke(result->pCallback, item->result);

    release_work_item(item);
}

static HRESULT queue_submit_item(struct queue *queue, LONG priority, IRtwqAsyncResult *result)
{
    TP_CALLBACK_PRIORITY callback_priority;
    struct work_item *item;
    TP_WORK *work_object;

    if (!(item = alloc_work_item(queue, result)))
        return E_OUTOFMEMORY;

    if (priority == 0)
        callback_priority = TP_CALLBACK_PRIORITY_NORMAL;
    else if (priority < 0)
        callback_priority = TP_CALLBACK_PRIORITY_LOW;
    else
        callback_priority = TP_CALLBACK_PRIORITY_HIGH;
    work_object = CreateThreadpoolWork(standard_queue_worker, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[callback_priority]);
    SubmitThreadpoolWork(work_object);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

static HRESULT queue_put_work_item(DWORD queue_id, LONG priority, IRtwqAsyncResult *result)
{
    struct queue *queue;
    HRESULT hr;

    if (FAILED(hr = grab_queue(queue_id, &queue)))
        return hr;

    return queue_submit_item(queue, priority, result);
}

static HRESULT invoke_async_callback(IRtwqAsyncResult *result)
{
    RTWQASYNCRESULT *result_data = (RTWQASYNCRESULT *)result;
    DWORD queue = RTWQ_CALLBACK_QUEUE_STANDARD, flags;
    HRESULT hr;

    if (FAILED(IRtwqAsyncCallback_GetParameters(result_data->pCallback, &flags, &queue)))
        queue = RTWQ_CALLBACK_QUEUE_STANDARD;

    if (FAILED(lock_user_queue(queue)))
        queue = RTWQ_CALLBACK_QUEUE_STANDARD;

    hr = queue_put_work_item(queue, 0, result);

    unlock_user_queue(queue);

    return hr;
}

static void queue_release_pending_item(struct work_item *item)
{
    EnterCriticalSection(&item->queue->cs);
    if (item->key)
    {
        list_remove(&item->entry);
        item->key = 0;
        release_work_item(item);
    }
    LeaveCriticalSection(&item->queue->cs);
}

static void CALLBACK waiting_item_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void CALLBACK waiting_item_cancelable_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WAIT *wait,
        TP_WAIT_RESULT wait_result)
{
    struct work_item *item = context;

    TRACE("result object %p.\n", item->result);

    queue_release_pending_item(item);

    invoke_async_callback(item->result);

    release_work_item(item);
}

static void queue_mark_item_pending(DWORD mask, struct work_item *item, RTWQWORKITEM_KEY *key)
{
    *key = generate_item_key(mask);
    item->key = *key;

    EnterCriticalSection(&item->queue->cs);
    list_add_tail(&item->queue->pending_items, &item->entry);
    grab_work_item(item);
    LeaveCriticalSection(&item->queue->cs);
}

static HRESULT queue_submit_wait(struct queue *queue, HANDLE event, LONG priority, IRtwqAsyncResult *result,
        RTWQWORKITEM_KEY *key)
{
    PTP_WAIT_CALLBACK callback;
    struct work_item *item;

    if (!(item = alloc_work_item(queue, result)))
        return E_OUTOFMEMORY;

    if (key)
    {
        queue_mark_item_pending(WAIT_ITEM_KEY_MASK, item, key);
        callback = waiting_item_cancelable_callback;
    }
    else
        callback = waiting_item_callback;

    item->u.wait_object = CreateThreadpoolWait(callback, item,
            (TP_CALLBACK_ENVIRON *)&queue->envs[TP_CALLBACK_PRIORITY_NORMAL]);
    SetThreadpoolWait(item->u.wait_object, event, NULL);

    TRACE("dispatched %p.\n", result);

    return S_OK;
}

struct async_result
{
    RTWQASYNCRESULT result;
    LONG refcount;
    IUnknown *object;
    IUnknown *state;
};

static struct async_result *impl_from_IRtwqAsyncResult(IRtwqAsyncResult *iface)
{
    return CONTAINING_RECORD(iface, struct async_result, result.AsyncResult);
}

static HRESULT WINAPI async_result_QueryInterface(IRtwqAsyncResult *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IRtwqAsyncResult) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncResult_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI async_result_AddRef(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);
    ULONG refcount = InterlockedIncrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_result_Release(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);
    ULONG refcount = InterlockedDecrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        if (result->result.pCallback)
            IRtwqAsyncCallback_Release(result->result.pCallback);
        if (result->object)
            IUnknown_Release(result->object);
        if (result->state)
            IUnknown_Release(result->state);
        if (result->result.hEvent)
            CloseHandle(result->result.hEvent);
        heap_free(result);

        RtwqUnlockPlatform();
    }

    return refcount;
}

static HRESULT WINAPI async_result_GetState(IRtwqAsyncResult *iface, IUnknown **state)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %p.\n", iface, state);

    if (!result->state)
        return E_POINTER;

    *state = result->state;
    IUnknown_AddRef(*state);

    return S_OK;
}

static HRESULT WINAPI async_result_GetStatus(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->result.hrStatusResult;
}

static HRESULT WINAPI async_result_SetStatus(IRtwqAsyncResult *iface, HRESULT status)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %#x.\n", iface, status);

    result->result.hrStatusResult = status;

    return S_OK;
}

static HRESULT WINAPI async_result_GetObject(IRtwqAsyncResult *iface, IUnknown **object)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!result->object)
        return E_POINTER;

    *object = result->object;
    IUnknown_AddRef(*object);

    return S_OK;
}

static IUnknown * WINAPI async_result_GetStateNoAddRef(IRtwqAsyncResult *iface)
{
    struct async_result *result = impl_from_IRtwqAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->state;
}

static const IRtwqAsyncResultVtbl async_result_vtbl =
{
    async_result_QueryInterface,
    async_result_AddRef,
    async_result_Release,
    async_result_GetState,
    async_result_GetStatus,
    async_result_SetStatus,
    async_result_GetObject,
    async_result_GetStateNoAddRef,
};

static HRESULT create_async_result(IUnknown *object, IRtwqAsyncCallback *callback, IUnknown *state, IRtwqAsyncResult **out)
{
    struct async_result *result;

    if (!out)
        return E_INVALIDARG;

    result = heap_alloc_zero(sizeof(*result));
    if (!result)
        return E_OUTOFMEMORY;

    RtwqLockPlatform();

    result->result.AsyncResult.lpVtbl = &async_result_vtbl;
    result->refcount = 1;
    result->object = object;
    if (result->object)
        IUnknown_AddRef(result->object);
    result->result.pCallback = callback;
    if (result->result.pCallback)
        IRtwqAsyncCallback_AddRef(result->result.pCallback);
    result->state = state;
    if (result->state)
        IUnknown_AddRef(result->state);

    *out = &result->result.AsyncResult;

    TRACE("Created async result object %p.\n", *out);

    return S_OK;
}

HRESULT WINAPI RtwqCreateAsyncResult(IUnknown *object, IRtwqAsyncCallback *callback, IUnknown *state,
        IRtwqAsyncResult **out)
{
    TRACE("%p, %p, %p, %p.\n", object, callback, state, out);

    return create_async_result(object, callback, state, out);
}

HRESULT WINAPI RtwqLockPlatform(void)
{
    InterlockedIncrement(&platform_lock);

    return S_OK;
}

HRESULT WINAPI RtwqUnlockPlatform(void)
{
    InterlockedDecrement(&platform_lock);

    return S_OK;
}

static void init_system_queues(void)
{
    /* Always initialize standard queue, keep the rest lazy. */

    EnterCriticalSection(&queues_section);

    if (system_queues[SYS_QUEUE_STANDARD].pool)
    {
        LeaveCriticalSection(&queues_section);
        return;
    }

    init_work_queue(RTWQ_STANDARD_WORKQUEUE, &system_queues[SYS_QUEUE_STANDARD]);

    LeaveCriticalSection(&queues_section);
}

HRESULT WINAPI RtwqStartup(void)
{
    if (InterlockedIncrement(&platform_lock) == 1)
    {
        init_system_queues();
    }

    return S_OK;
}

static void shutdown_system_queues(void)
{
    unsigned int i;

    EnterCriticalSection(&queues_section);

    for (i = 0; i < ARRAY_SIZE(system_queues); ++i)
    {
        shutdown_queue(&system_queues[i]);
    }

    LeaveCriticalSection(&queues_section);
}

HRESULT WINAPI RtwqShutdown(void)
{
    if (platform_lock <= 0)
        return S_OK;

    if (InterlockedExchangeAdd(&platform_lock, -1) == 1)
    {
        shutdown_system_queues();
    }

    return S_OK;
}

HRESULT WINAPI RtwqPutWaitingWorkItem(HANDLE event, LONG priority, IRtwqAsyncResult *result, RTWQWORKITEM_KEY *key)
{
    struct queue *queue;
    HRESULT hr;

    TRACE("%p, %d, %p, %p.\n", event, priority, result, key);

    if (FAILED(hr = grab_queue(RTWQ_CALLBACK_QUEUE_TIMER, &queue)))
        return hr;

    hr = queue_submit_wait(queue, event, priority, result, key);

    return hr;
}

HRESULT WINAPI RtwqLockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return lock_user_queue(queue);
}

HRESULT WINAPI RtwqUnlockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return unlock_user_queue(queue);
}
