/*
 * Copyright 2020 Nikolay Sivov
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

#include "evr.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

struct sample_allocator
{
    IMFVideoSampleAllocator IMFVideoSampleAllocator_iface;
    IMFVideoSampleAllocatorCallback IMFVideoSampleAllocatorCallback_iface;
    LONG refcount;

    IMFVideoSampleAllocatorNotify *callback;
    unsigned int free_samples;
    CRITICAL_SECTION cs;
};

static struct sample_allocator *impl_from_IMFVideoSampleAllocator(IMFVideoSampleAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocator_iface);
}

static struct sample_allocator *impl_from_IMFVideoSampleAllocatorCallback(IMFVideoSampleAllocatorCallback *iface)
{
    return CONTAINING_RECORD(iface, struct sample_allocator, IMFVideoSampleAllocatorCallback_iface);
}

static HRESULT WINAPI sample_allocator_QueryInterface(IMFVideoSampleAllocator *iface, REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &allocator->IMFVideoSampleAllocator_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorCallback))
    {
        *obj = &allocator->IMFVideoSampleAllocatorCallback_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI sample_allocator_AddRef(IMFVideoSampleAllocator *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    ULONG refcount = InterlockedIncrement(&allocator->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sample_allocator_Release(IMFVideoSampleAllocator *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocator(iface);
    ULONG refcount = InterlockedDecrement(&allocator->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (allocator->callback)
            IMFVideoSampleAllocatorNotify_Release(allocator->callback);
        DeleteCriticalSection(&allocator->cs);
        heap_free(allocator);
    }

    return refcount;
}

static HRESULT WINAPI sample_allocator_SetDirectXManager(IMFVideoSampleAllocator *iface,
        IUnknown *manager)
{
    FIXME("%p, %p.\n", iface, manager);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_allocator_UninitializeSampleAllocator(IMFVideoSampleAllocator *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_allocator_InitializeSampleAllocator(IMFVideoSampleAllocator *iface,
        DWORD sample_count, IMFMediaType *media_type)
{
    FIXME("%p, %u, %p.\n", iface, sample_count, media_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI sample_allocator_AllocateSample(IMFVideoSampleAllocator *iface, IMFSample **sample)
{
    FIXME("%p, %p.\n", iface, sample);

    return E_NOTIMPL;
}

static const IMFVideoSampleAllocatorVtbl sample_allocator_vtbl =
{
    sample_allocator_QueryInterface,
    sample_allocator_AddRef,
    sample_allocator_Release,
    sample_allocator_SetDirectXManager,
    sample_allocator_UninitializeSampleAllocator,
    sample_allocator_InitializeSampleAllocator,
    sample_allocator_AllocateSample,
};

static HRESULT WINAPI sample_allocator_callback_QueryInterface(IMFVideoSampleAllocatorCallback *iface,
        REFIID riid, void **obj)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_QueryInterface(&allocator->IMFVideoSampleAllocator_iface, riid, obj);
}

static ULONG WINAPI sample_allocator_callback_AddRef(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_AddRef(&allocator->IMFVideoSampleAllocator_iface);
}

static ULONG WINAPI sample_allocator_callback_Release(IMFVideoSampleAllocatorCallback *iface)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);
    return IMFVideoSampleAllocator_Release(&allocator->IMFVideoSampleAllocator_iface);
}

static HRESULT WINAPI sample_allocator_callback_SetCallback(IMFVideoSampleAllocatorCallback *iface,
        IMFVideoSampleAllocatorNotify *callback)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, callback);

    EnterCriticalSection(&allocator->cs);
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_Release(allocator->callback);
    allocator->callback = callback;
    if (allocator->callback)
        IMFVideoSampleAllocatorNotify_AddRef(allocator->callback);
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static HRESULT WINAPI sample_allocator_callback_GetFreeSampleCount(IMFVideoSampleAllocatorCallback *iface,
        LONG *count)
{
    struct sample_allocator *allocator = impl_from_IMFVideoSampleAllocatorCallback(iface);

    TRACE("%p, %p.\n", iface, count);

    EnterCriticalSection(&allocator->cs);
    if (count)
        *count = allocator->free_samples;
    LeaveCriticalSection(&allocator->cs);

    return S_OK;
}

static const IMFVideoSampleAllocatorCallbackVtbl sample_allocator_callback_vtbl =
{
    sample_allocator_callback_QueryInterface,
    sample_allocator_callback_AddRef,
    sample_allocator_callback_Release,
    sample_allocator_callback_SetCallback,
    sample_allocator_callback_GetFreeSampleCount,
};

HRESULT WINAPI MFCreateVideoSampleAllocator(REFIID riid, void **obj)
{
    struct sample_allocator *object;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFVideoSampleAllocator_iface.lpVtbl = &sample_allocator_vtbl;
    object->IMFVideoSampleAllocatorCallback_iface.lpVtbl = &sample_allocator_callback_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    hr = IMFVideoSampleAllocator_QueryInterface(&object->IMFVideoSampleAllocator_iface, riid, obj);
    IMFVideoSampleAllocator_Release(&object->IMFVideoSampleAllocator_iface);

    return hr;
}
