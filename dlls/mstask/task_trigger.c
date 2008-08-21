/*
 * Copyright (C) 2008 Google (Roy Shea)
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

#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static HRESULT WINAPI MSTASK_ITaskTrigger_QueryInterface(
        ITaskTrigger* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskTrigger))
    {
        *ppvObject = &This->lpVtbl;
        ITaskTrigger_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskTrigger_AddRef(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITaskTrigger_Release(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
        InterlockedDecrement(&dll_ref);
    }
    return ref;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_SetTrigger(
        ITaskTrigger* iface,
        const PTASK_TRIGGER pTrigger)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTrigger(
        ITaskTrigger* iface,
        PTASK_TRIGGER pTrigger)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTriggerString(
        ITaskTrigger* iface,
        LPWSTR *ppwszTrigger)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const ITaskTriggerVtbl MSTASK_ITaskTriggerVtbl =
{
    MSTASK_ITaskTrigger_QueryInterface,
    MSTASK_ITaskTrigger_AddRef,
    MSTASK_ITaskTrigger_Release,
    MSTASK_ITaskTrigger_SetTrigger,
    MSTASK_ITaskTrigger_GetTrigger,
    MSTASK_ITaskTrigger_GetTriggerString
};

HRESULT TaskTriggerConstructor(LPVOID *ppObj)
{
    TaskTriggerImpl *This;
    TRACE("(%p)\n", ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &MSTASK_ITaskTriggerVtbl;
    This->ref = 1;

    *ppObj = &This->lpVtbl;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
