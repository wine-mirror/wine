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
#include "wine/wined3d.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxcore);

struct dxcore_adapter
{
    IDXCoreAdapter IDXCoreAdapter_iface;
    LONG refcount;

    struct wined3d_adapter_identifier identifier;
};

static inline struct dxcore_adapter *impl_from_IDXCoreAdapter(IDXCoreAdapter *iface)
{
    return CONTAINING_RECORD(iface, struct dxcore_adapter, IDXCoreAdapter_iface);
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_QueryInterface(IDXCoreAdapter *iface, REFIID riid, void **out)
{
    struct dxcore_adapter *adapter = impl_from_IDXCoreAdapter(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDXCoreAdapter)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &adapter->IDXCoreAdapter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_AddRef(IDXCoreAdapter *iface)
{
    struct dxcore_adapter *adapter = impl_from_IDXCoreAdapter(iface);
    ULONG refcount = InterlockedIncrement(&adapter->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_Release(IDXCoreAdapter *iface)
{
    struct dxcore_adapter *adapter = impl_from_IDXCoreAdapter(iface);
    ULONG refcount = InterlockedDecrement(&adapter->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(adapter);

    return refcount;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_IsValid(IDXCoreAdapter *iface)
{
    FIXME("iface %p stub!\n", iface);
    return FALSE;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_IsAttributeSupported(IDXCoreAdapter *iface, REFGUID attribute)
{
    FIXME("iface %p, attribute %s stub!\n", iface, debugstr_guid(attribute));
    return FALSE;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_IsPropertySupported(IDXCoreAdapter *iface, DXCoreAdapterProperty property)
{
    FIXME("iface %p, property %u stub!\n", iface, property);
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_GetProperty(IDXCoreAdapter *iface, DXCoreAdapterProperty property,
        size_t buffer_size, void *buffer)
{
    struct dxcore_adapter *adapter = impl_from_IDXCoreAdapter(iface);

    TRACE("iface %p, property %u, buffer_size %Iu, buffer %p\n", iface, property, buffer_size, buffer);

    if (!buffer)
        return E_POINTER;

    switch (property)
    {
        case HardwareID:
        {
            struct DXCoreHardwareID *hardware_id = buffer;

            if (buffer_size != sizeof(DXCoreHardwareID))
                return E_INVALIDARG;

            hardware_id->vendorID = adapter->identifier.vendor_id;
            hardware_id->deviceID = adapter->identifier.device_id;
            hardware_id->subSysID = adapter->identifier.subsystem_id;
            hardware_id->revision = adapter->identifier.revision;
            break;
        }
        default:
        {
            FIXME("property %u not implemented.\n", property);
            return DXGI_ERROR_INVALID_CALL;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_GetPropertySize(IDXCoreAdapter *iface, DXCoreAdapterProperty property,
        size_t *buffer_size)
{
    FIXME("iface %p, property %u, buffer_size %p stub!\n", iface, property, buffer_size);
    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_IsQueryStateSupported(IDXCoreAdapter *iface, DXCoreAdapterState property)
{
    FIXME("iface %p, property %u stub!\n", iface, property);
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_QueryState(IDXCoreAdapter *iface, DXCoreAdapterState state, size_t state_details_size,
        const void *state_details, size_t buffer_size, void *buffer)
{
    FIXME("iface %p, state %u, state_details_size %Iu, state_details %p, buffer_size %Iu, buffer %p stub!\n",
            iface, state, state_details_size, state_details, buffer_size, buffer);
    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_IsSetStateSupported(IDXCoreAdapter *iface, DXCoreAdapterState property)
{
    FIXME("iface %p, property %u stub!\n", iface, property);
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_SetState(IDXCoreAdapter *iface, DXCoreAdapterState state, size_t state_details_size,
        const void *state_details, size_t buffer_size, const void *buffer)
{
    FIXME("iface %p, state %u, state_details_size %Iu, state_details %p, buffer_size %Iu, buffer %p stub!\n",
            iface, state, state_details_size, state_details, buffer_size, buffer);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_GetFactory(IDXCoreAdapter *iface, REFIID riid, void **out)
{
    FIXME("iface %p, riid %s, out %p stub!\n", iface, debugstr_guid(riid), out);
    return E_NOTIMPL;
}

static const struct IDXCoreAdapterVtbl dxcore_adapter_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_QueryInterface,
    dxcore_adapter_AddRef,
    dxcore_adapter_Release,
    /* IDXCoreAdapter methods */
    dxcore_adapter_IsValid,
    dxcore_adapter_IsAttributeSupported,
    dxcore_adapter_IsPropertySupported,
    dxcore_adapter_GetProperty,
    dxcore_adapter_GetPropertySize,
    dxcore_adapter_IsQueryStateSupported,
    dxcore_adapter_QueryState,
    dxcore_adapter_IsSetStateSupported,
    dxcore_adapter_SetState,
    dxcore_adapter_GetFactory,
};

struct dxcore_adapter_list
{
    IDXCoreAdapterList IDXCoreAdapterList_iface;
    LONG refcount;

    struct dxcore_adapter **adapters;
    uint32_t adapter_count;
};

static inline struct dxcore_adapter_list *impl_from_IDXCoreAdapterList(IDXCoreAdapterList *iface)
{
    return CONTAINING_RECORD(iface, struct dxcore_adapter_list, IDXCoreAdapterList_iface);
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_list_QueryInterface(IDXCoreAdapterList *iface, REFIID riid, void **out)
{
    struct dxcore_adapter_list *list = impl_from_IDXCoreAdapterList(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDXCoreAdapterList)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &list->IDXCoreAdapterList_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_list_AddRef(IDXCoreAdapterList *iface)
{
    struct dxcore_adapter_list *list = impl_from_IDXCoreAdapterList(iface);
    ULONG refcount = InterlockedIncrement(&list->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxcore_adapter_list_Release(IDXCoreAdapterList *iface)
{
    struct dxcore_adapter_list *list = impl_from_IDXCoreAdapterList(iface);
    ULONG refcount = InterlockedDecrement(&list->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (UINT i = 0; i < list->adapter_count; i++)
        {
            if (list->adapters[i])
                IDXCoreAdapter_Release(&list->adapters[i]->IDXCoreAdapter_iface);
        }
        free(list->adapters);
        free(list);
    }
    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_list_GetAdapter(IDXCoreAdapterList *iface, uint32_t index,
        REFIID riid, void **out)
{
    struct dxcore_adapter_list *list = impl_from_IDXCoreAdapterList(iface);
    IDXCoreAdapter *adapter;

    TRACE("iface %p, index %u, riid %s, out %p\n", iface, index, debugstr_guid(riid), out);

    if (!out)
        return E_POINTER;

    if (index >= list->adapter_count)
    {
        *out = NULL;
        return E_INVALIDARG;
    }

    adapter = &list->adapters[index]->IDXCoreAdapter_iface;
    TRACE("returning IDXCoreAdapter %p for index %u.\n", adapter, index);
    return IDXCoreAdapter_QueryInterface(adapter, riid, out);
}

static uint32_t STDMETHODCALLTYPE dxcore_adapter_list_GetAdapterCount(IDXCoreAdapterList *iface)
{
    struct dxcore_adapter_list *list = impl_from_IDXCoreAdapterList(iface);

    TRACE("iface %p\n", iface);

    return list->adapter_count;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_list_IsStale(IDXCoreAdapterList *iface)
{
    FIXME("iface %p stub!\n", iface);
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_list_GetFactory(IDXCoreAdapterList *iface, REFIID riid, void **out)
{
    FIXME("iface %p, riid %s, out %p stub!\n", iface, debugstr_guid(riid), out);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_list_Sort(IDXCoreAdapterList *iface, uint32_t num_preferences,
        const DXCoreAdapterPreference *preferences)
{
    FIXME("iface %p, num_preferences %u, preferences %p stub!\n", iface, num_preferences, preferences);
    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxcore_adapter_list_IsAdapterPreferenceSupported(IDXCoreAdapterList *iface,
        DXCoreAdapterPreference preference)
{
    FIXME("iface %p, preference %u stub!\n", iface, preference);
    return FALSE;
}

static const struct IDXCoreAdapterListVtbl dxcore_adapter_list_vtbl =
{
    /* IUnknown methods */
    dxcore_adapter_list_QueryInterface,
    dxcore_adapter_list_AddRef,
    dxcore_adapter_list_Release,
    /* IDXCoreAdapterList methods */
    dxcore_adapter_list_GetAdapter,
    dxcore_adapter_list_GetAdapterCount,
    dxcore_adapter_list_IsStale,
    dxcore_adapter_list_GetFactory,
    dxcore_adapter_list_Sort,
    dxcore_adapter_list_IsAdapterPreferenceSupported,
};

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

static HRESULT get_adapters(struct dxcore_adapter_list *list)
{
    struct wined3d *wined3d = wined3d_create(0);
    HRESULT hr = S_OK;

    if (!wined3d)
        return E_FAIL;

    if (!(list->adapter_count = wined3d_get_adapter_count(wined3d)))
        goto done;

    if (!(list->adapters = calloc(list->adapter_count, sizeof(*list->adapters))))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    for (UINT i = 0; i < list->adapter_count; i++)
    {
        struct dxcore_adapter *dxcore_adapter = calloc(1, sizeof(*dxcore_adapter));
        const struct wined3d_adapter *wined3d_adapter;

        if (!dxcore_adapter)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        wined3d_adapter = wined3d_get_adapter(wined3d, i);
        if (FAILED(hr = wined3d_adapter_get_identifier(wined3d_adapter, 0, &dxcore_adapter->identifier)))
        {
            free(dxcore_adapter);
            goto done;
        }

        dxcore_adapter->IDXCoreAdapter_iface.lpVtbl = &dxcore_adapter_vtbl;
        dxcore_adapter->refcount = 1;

        list->adapters[i] = dxcore_adapter;
    }

done:
    wined3d_decref(wined3d);
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxcore_adapter_factory_CreateAdapterList(IDXCoreAdapterFactory *iface, uint32_t num_attributes,
        const GUID *filter_attributes, REFIID riid, void **out)
{
    struct dxcore_adapter_list *list;
    HRESULT hr;

    FIXME("iface %p, num_attributes %u, filter_attributes %p, riid %s, out %p semi-stub!\n", iface, num_attributes, filter_attributes,
            debugstr_guid(riid), out);

    if (!out)
        return E_POINTER;

    *out = NULL;

    if (!num_attributes || !filter_attributes)
        return E_INVALIDARG;

    if (!(list = calloc(1, sizeof(*list))))
        return E_OUTOFMEMORY;

    list->IDXCoreAdapterList_iface.lpVtbl = &dxcore_adapter_list_vtbl;
    list->refcount = 1;
    if (FAILED(hr = get_adapters(list)))
    {
        IDXCoreAdapterList_Release(&list->IDXCoreAdapterList_iface);
        return hr;
    }

    hr = IDXCoreAdapterList_QueryInterface(&list->IDXCoreAdapterList_iface, riid, out);
    IDXCoreAdapterList_Release(&list->IDXCoreAdapterList_iface);
    TRACE("created IDXCoreAdapterList %p.\n", *out);
    return hr;
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
