/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#include "initguid.h"
#include "dxcore.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxcore);

struct dxcore_adapter_factory
{
    IDXCoreAdapterFactory IDXCoreAdapterFactory_iface;
    LONG refcount;
};

static inline struct dxcore_adapter_factory *impl_from_IDXCoreAdapterFactory(IDXCoreAdapterFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dxcore_adapter_factory, IDXCoreAdapterFactory_iface);
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_QueryInterface(IDXCoreAdapterFactory *iface, REFIID riid, void **out)
{
    struct dxcore_adapter_factory *factory = impl_from_IDXCoreAdapterFactory(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDXCoreAdapterFactory)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &factory->IDXCoreAdapterFactory_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_factory_AddRef(IDXCoreAdapterFactory *iface)
{
    struct dxcore_adapter_factory *factory = impl_from_IDXCoreAdapterFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_factory_Release(IDXCoreAdapterFactory *iface)
{
    struct dxcore_adapter_factory *factory = impl_from_IDXCoreAdapterFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(factory);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_CreateAdapterList(IDXCoreAdapterFactory *iface, uint32_t num_attributes,
        const GUID *filter_attributes, REFIID riid, void **out)
{
    FIXME("iface %p, num_attributes %u, filter_attributes %p, riid %s, out %p stub!\n", iface, num_attributes, filter_attributes,
            debugstr_guid(riid), out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_GetAdapterByLuid(IDXCoreAdapterFactory *iface, REFLUID adapter_luid,
        REFIID riid, void **out)
{
    FIXME("iface %p, adapter_luid %p, riid %s, out %p stub!\n", iface, adapter_luid, debugstr_guid(riid), out);
    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_factory_IsNotificationTypeSupported(IDXCoreAdapterFactory *iface, DXCoreNotificationType type)
{
    FIXME("iface %p, type %u stub!\n", iface, type);
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_RegisterEventNotification(IDXCoreAdapterFactory *iface, IUnknown *dxcore_object,
        DXCoreNotificationType type, PFN_DXCORE_NOTIFICATION_CALLBACK callback, void *callback_context, uint32_t *event_cookie)
{
    FIXME("iface %p, dxcore_object %p, type %u, callback %p, callback_context %p, event_cookie %p stub!\n", iface, dxcore_object, type, callback,
            callback_context, event_cookie);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_UnregisterEventNotification(IDXCoreAdapterFactory *iface, uint32_t event_cookie)
{
    FIXME("iface %p, event_cookie %u stub!\n", iface, event_cookie);
    return E_NOTIMPL;
}

static const struct IDXCoreAdapterFactoryVtbl dxcore_adapter_factory_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_factory_QueryInterface,
    dxcore_adapter_factory_AddRef,
    dxcore_adapter_factory_Release,
    /* IDXCoreAdapterFactory methods */
    dxcore_adapter_factory_CreateAdapterList,
    dxcore_adapter_factory_GetAdapterByLuid,
    dxcore_adapter_factory_IsNotificationTypeSupported,
    dxcore_adapter_factory_RegisterEventNotification,
    dxcore_adapter_factory_UnregisterEventNotification,
};

HRESULT STDMETHODCALLTYPE DXCoreCreateAdapterFactory(REFIID riid, void **out)
{
    static struct dxcore_adapter_factory *factory = NULL;

    TRACE("riid %s, out %p\n", debugstr_guid(riid), out);

    if (!out)
        return E_POINTER;

    if (!factory)
    {
        if (!(factory = calloc(1, sizeof(*factory))))
        {
            *out = NULL;
            return E_OUTOFMEMORY;
        }

        factory->IDXCoreAdapterFactory_iface.lpVtbl = &dxcore_adapter_factory_vtbl;
        factory->refcount = 0;
    }

    TRACE("created IDXCoreAdapterFactory %p.\n", *out);
    return IDXCoreAdapterFactory_QueryInterface(&factory->IDXCoreAdapterFactory_iface, riid, out);
}
