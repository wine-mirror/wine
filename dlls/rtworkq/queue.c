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

static CRITICAL_SECTION queues_section;
static CRITICAL_SECTION_DEBUG queues_critsect_debug =
{
    0, 0, &queues_section,
    { &queues_critsect_debug.ProcessLocksList, &queues_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": queues_section") }
};
static CRITICAL_SECTION queues_section = { &queues_critsect_debug, -1, 0, 0, 0, 0 };

static LONG platform_lock;

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

static void CALLBACK standard_queue_cleanup_callback(void *object_data, void *group_data)
{
}

static void release_work_item(struct work_item *item)
{
    if (InterlockedDecrement(&item->refcount) == 0)
    {
         IRtwqAsyncResult_Release(item->result);
         heap_free(item);
    }
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
