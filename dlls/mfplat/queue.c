/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include "mfapi.h"
#include "mferror.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

#define FIRST_USER_QUEUE_HANDLE 5
#define MAX_USER_QUEUE_HANDLES 124

struct queue
{
    void *obj;
    LONG refcount;
    WORD generation;
};

static struct queue user_queues[MAX_USER_QUEUE_HANDLES];
static struct queue *next_free_user_queue;
static struct queue *next_unused_user_queue = user_queues;
static WORD queue_generation;

static CRITICAL_SECTION user_queues_section;
static CRITICAL_SECTION_DEBUG user_queues_critsect_debug =
{
    0, 0, &user_queues_section,
    { &user_queues_critsect_debug.ProcessLocksList, &user_queues_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": user_queues_section") }
};
static CRITICAL_SECTION user_queues_section = { &user_queues_critsect_debug, -1, 0, 0, 0, 0 };

static struct queue *get_queue_obj(DWORD handle)
{
    unsigned int idx = HIWORD(handle) - FIRST_USER_QUEUE_HANDLE;

    if (idx < MAX_USER_QUEUE_HANDLES && user_queues[idx].refcount)
    {
        if (LOWORD(handle) == user_queues[idx].generation)
            return &user_queues[idx];
    }

    return NULL;
}

static HRESULT alloc_user_queue(DWORD *queue)
{
    struct queue *entry;
    unsigned int idx;

    EnterCriticalSection(&user_queues_section);

    entry = next_free_user_queue;
    if (entry)
        next_free_user_queue = entry->obj;
    else if (next_unused_user_queue < user_queues + MAX_USER_QUEUE_HANDLES)
        entry = next_unused_user_queue++;
    else
    {
        LeaveCriticalSection(&user_queues_section);
        return E_OUTOFMEMORY;
    }

    entry->refcount = 1;
    entry->obj = NULL;
    if (++queue_generation == 0xffff) queue_generation = 1;
    entry->generation = queue_generation;
    idx = entry - user_queues + FIRST_USER_QUEUE_HANDLE;
    *queue = (idx << 16) | entry->generation;
    LeaveCriticalSection(&user_queues_section);

    return S_OK;
}

static HRESULT lock_user_queue(DWORD queue)
{
    HRESULT hr = MF_E_INVALID_WORKQUEUE;
    struct queue *entry;

    if (!(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&user_queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        entry->refcount++;
        hr = S_OK;
    }
    LeaveCriticalSection(&user_queues_section);
    return hr;
}

static HRESULT unlock_user_queue(DWORD queue)
{
    HRESULT hr = MF_E_INVALID_WORKQUEUE;
    struct queue *entry;

    if (!(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK))
        return S_OK;

    EnterCriticalSection(&user_queues_section);
    entry = get_queue_obj(queue);
    if (entry && entry->refcount)
    {
        if (--entry->refcount == 0)
        {
            entry->obj = next_free_user_queue;
            next_free_user_queue = entry;
        }
        hr = S_OK;
    }
    LeaveCriticalSection(&user_queues_section);
    return hr;
}

struct async_result
{
    MFASYNCRESULT result;
    LONG refcount;
    IUnknown *object;
    IUnknown *state;
};

static struct async_result *impl_from_IMFAsyncResult(IMFAsyncResult *iface)
{
    return CONTAINING_RECORD(iface, struct async_result, result.AsyncResult);
}

static HRESULT WINAPI async_result_QueryInterface(IMFAsyncResult *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFAsyncResult) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncResult_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI async_result_AddRef(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedIncrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI async_result_Release(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);
    ULONG refcount = InterlockedDecrement(&result->refcount);

    TRACE("%p, %u.\n", iface, refcount);

    if (!refcount)
    {
        if (result->result.pCallback)
            IMFAsyncCallback_Release(result->result.pCallback);
        if (result->object)
            IUnknown_Release(result->object);
        if (result->state)
            IUnknown_Release(result->state);
        heap_free(result);

        MFUnlockPlatform();
    }

    return refcount;
}

static HRESULT WINAPI async_result_GetState(IMFAsyncResult *iface, IUnknown **state)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, state);

    if (!result->state)
        return E_POINTER;

    *state = result->state;
    IUnknown_AddRef(*state);

    return S_OK;
}

static HRESULT WINAPI async_result_GetStatus(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->result.hrStatusResult;
}

static HRESULT WINAPI async_result_SetStatus(IMFAsyncResult *iface, HRESULT status)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %#x.\n", iface, status);

    result->result.hrStatusResult = status;

    return S_OK;
}

static HRESULT WINAPI async_result_GetObject(IMFAsyncResult *iface, IUnknown **object)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!result->object)
        return E_POINTER;

    *object = result->object;
    IUnknown_AddRef(*object);

    return S_OK;
}

static IUnknown * WINAPI async_result_GetStateNoAddRef(IMFAsyncResult *iface)
{
    struct async_result *result = impl_from_IMFAsyncResult(iface);

    TRACE("%p.\n", iface);

    return result->state;
}

static const IMFAsyncResultVtbl async_result_vtbl =
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

HRESULT WINAPI MFCreateAsyncResult(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **out)
{
    struct async_result *result;

    TRACE("%p, %p, %p, %p.\n", object, callback, state, out);

    if (!out)
        return E_INVALIDARG;

    result = heap_alloc_zero(sizeof(*result));
    if (!result)
        return E_OUTOFMEMORY;

    MFLockPlatform();

    result->result.AsyncResult.lpVtbl = &async_result_vtbl;
    result->refcount = 1;
    result->object = object;
    if (result->object)
        IUnknown_AddRef(result->object);
    result->result.pCallback = callback;
    if (result->result.pCallback)
        IMFAsyncCallback_AddRef(result->result.pCallback);
    result->state = state;
    if (result->state)
        IUnknown_AddRef(result->state);

    *out = &result->result.AsyncResult;

    return S_OK;
}

/***********************************************************************
 *      MFAllocateWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFAllocateWorkQueue(DWORD *queue)
{
    TRACE("%p.\n", queue);

    return alloc_user_queue(queue);
}

/***********************************************************************
 *      MFLockWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFLockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return lock_user_queue(queue);
}

/***********************************************************************
 *      MFUnlockWorkQueue (mfplat.@)
 */
HRESULT WINAPI MFUnlockWorkQueue(DWORD queue)
{
    TRACE("%#x.\n", queue);

    return unlock_user_queue(queue);
}

/***********************************************************************
 *      MFPutWorkItem (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItem(DWORD queue, IMFAsyncCallback *callback, IUnknown *state)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%#x, %p, %p.\n", queue, callback, state);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = MFPutWorkItemEx(queue, result);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFPutWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result)
{
    FIXME("%#x, %p\n", queue, result);

    return E_NOTIMPL;
}

/***********************************************************************
 *      MFInvokeCallback (mfplat.@)
 */
HRESULT WINAPI MFInvokeCallback(IMFAsyncResult *result)
{
    MFASYNCRESULT *result_data = (MFASYNCRESULT *)result;
    DWORD queue = MFASYNC_CALLBACK_QUEUE_STANDARD, flags;
    HRESULT hr;

    TRACE("%p.\n", result);

    if (FAILED(IMFAsyncCallback_GetParameters(result_data->pCallback, &flags, &queue)))
        queue = MFASYNC_CALLBACK_QUEUE_STANDARD;

    if (FAILED(MFLockWorkQueue(queue)))
        queue = MFASYNC_CALLBACK_QUEUE_STANDARD;

    hr = MFPutWorkItemEx(queue, result);

    MFUnlockWorkQueue(queue);

    return hr;
}

static HRESULT schedule_work_item(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key)
{
    FIXME("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return E_NOTIMPL;
}

/***********************************************************************
 *      MFScheduleWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItemEx(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key)
{
    TRACE("%p, %s, %p.\n", result, wine_dbgstr_longlong(timeout), key);

    return schedule_work_item(result, timeout, key);
}

/***********************************************************************
 *      MFScheduleWorkItemEx (mfplat.@)
 */
HRESULT WINAPI MFScheduleWorkItem(IMFAsyncCallback *callback, IUnknown *state, INT64 timeout, MFWORKITEM_KEY *key)
{
    IMFAsyncResult *result;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", callback, state, wine_dbgstr_longlong(timeout), key);

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &result)))
        return hr;

    hr = schedule_work_item(result, timeout, key);

    IMFAsyncResult_Release(result);

    return hr;
}

/***********************************************************************
 *      MFCancelWorkItem (mfplat.@)
 */
HRESULT WINAPI MFCancelWorkItem(MFWORKITEM_KEY key)
{
    FIXME("%s.\n", wine_dbgstr_longlong(key));

    return E_NOTIMPL;
}
