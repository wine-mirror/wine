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

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG platform_lock;

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
