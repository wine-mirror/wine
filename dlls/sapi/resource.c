/*
 * Speech API (SAPI) resource manager implementation.
 *
 * Copyright 2020 Gijs Vermeulen
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

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "sapiddk.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct resource_manager
{
    ISpResourceManager ISpResourceManager_iface;
    LONG ref;
};

static inline struct resource_manager *impl_from_ISpResourceManager(ISpResourceManager *iface)
{
    return CONTAINING_RECORD(iface, struct resource_manager, ISpResourceManager_iface);
}

static HRESULT WINAPI resource_manager_QueryInterface(ISpResourceManager *iface, REFIID iid, void **obj)
{
    struct resource_manager *This = impl_from_ISpResourceManager(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISpResourceManager))
        *obj = &This->ISpResourceManager_iface;
    else
    {
        *obj = NULL;
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI resource_manager_AddRef(ISpResourceManager *iface)
{
    struct resource_manager *This = impl_from_ISpResourceManager(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI resource_manager_Release(ISpResourceManager *iface)
{
    struct resource_manager *This = impl_from_ISpResourceManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI resource_manager_QueryService(ISpResourceManager *iface, REFGUID guid, REFIID iid,
                                                    void **obj)
{
    FIXME("(%p, %s, %s, %p): stub.\n", iface, debugstr_guid(guid), debugstr_guid(iid), obj);

    return E_NOTIMPL;
}

static HRESULT WINAPI resource_manager_SetObject(ISpResourceManager *iface, REFGUID guid, IUnknown *obj)
{
    FIXME("(%p, %s, %p): stub.\n", iface, debugstr_guid(guid), obj);

    return E_NOTIMPL;
}

static HRESULT WINAPI resource_manager_GetObject(ISpResourceManager *iface, REFGUID guid, REFCLSID clsid, REFIID iid,
                                                 BOOL release, void **obj)
{
    FIXME("(%p, %s, %s, %s, %d, %p): stub.\n", iface, debugstr_guid(guid), debugstr_guid(clsid), debugstr_guid(iid),
           release, obj);

    return E_NOTIMPL;
}

const static ISpResourceManagerVtbl resource_manager_vtbl =
{
    resource_manager_QueryInterface,
    resource_manager_AddRef,
    resource_manager_Release,
    resource_manager_QueryService,
    resource_manager_SetObject,
    resource_manager_GetObject
};

HRESULT resource_manager_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct resource_manager *This = malloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpResourceManager_iface.lpVtbl = &resource_manager_vtbl;
    This->ref = 1;

    hr = ISpResourceManager_QueryInterface(&This->ISpResourceManager_iface, iid, obj);

    ISpResourceManager_Release(&This->ISpResourceManager_iface);
    return hr;
}
