/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

struct taskbar_list
{
    ITaskbarList2 ITaskbarList2_iface;
    LONG refcount;
};

static inline struct taskbar_list *impl_from_ITaskbarList2(ITaskbarList2 *iface)
{
    return CONTAINING_RECORD(iface, struct taskbar_list, ITaskbarList2_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_QueryInterface(ITaskbarList2 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ITaskbarList) ||
        IsEqualGUID(riid, &IID_ITaskbarList2) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE taskbar_list_AddRef(ITaskbarList2 *iface)
{
    struct taskbar_list *This = impl_from_ITaskbarList2(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE taskbar_list_Release(ITaskbarList2 *iface)
{
    struct taskbar_list *This = impl_from_ITaskbarList2(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
        SHDOCVW_UnlockModule();
    }

    return refcount;
}

/* ITaskbarList methods */

static HRESULT STDMETHODCALLTYPE taskbar_list_HrInit(ITaskbarList2 *iface)
{
    TRACE("iface %p\n", iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_AddTab(ITaskbarList2 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_DeleteTab(ITaskbarList2 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_ActivateTab(ITaskbarList2 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE taskbar_list_SetActiveAlt(ITaskbarList2 *iface, HWND hwnd)
{
    FIXME("iface %p, hwnd %p stub!\n", iface, hwnd);

    return E_NOTIMPL;
}

/* ITaskbarList2 method */

static HRESULT STDMETHODCALLTYPE taskbar_list_MarkFullscreenWindow(ITaskbarList2 *iface,
                                                                   HWND hwnd,
                                                                   BOOL fullscreen)
{
    FIXME("iface %p, hwnd %p, fullscreen %s stub!\n", iface, hwnd, (fullscreen)?"true":"false");

    return E_NOTIMPL;
}

static const struct ITaskbarList2Vtbl taskbar_list_vtbl =
{
    /* IUnknown methods */
    taskbar_list_QueryInterface,
    taskbar_list_AddRef,
    taskbar_list_Release,
    /* ITaskbarList methods */
    taskbar_list_HrInit,
    taskbar_list_AddTab,
    taskbar_list_DeleteTab,
    taskbar_list_ActivateTab,
    taskbar_list_SetActiveAlt,
    /* ITaskbarList2 method */
    taskbar_list_MarkFullscreenWindow,
};

HRESULT TaskbarList_Create(IUnknown *outer, REFIID riid, void **taskbar_list)
{
    struct taskbar_list *object;
    HRESULT hr;

    TRACE("outer %p, riid %s, taskbar_list %p\n", outer, debugstr_guid(riid), taskbar_list);

    if (outer)
    {
        WARN("Aggregation not supported\n");
        *taskbar_list = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate taskbar list object memory\n");
        *taskbar_list = NULL;
        return E_OUTOFMEMORY;
    }

    object->ITaskbarList2_iface.lpVtbl = &taskbar_list_vtbl;
    object->refcount = 0;

    TRACE("Created ITaskbarList2 %p\n", object);

    hr = ITaskbarList2_QueryInterface(&object->ITaskbarList2_iface, riid, taskbar_list);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    SHDOCVW_LockModule();

    return S_OK;
}
