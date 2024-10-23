/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dataexchange);

struct dataexchange
{
    IActivationFactory IActivationFactory_iface;
    ICoreDragDropManagerStatics ICoreDragDropManagerStatics_iface;
    LONG ref;
};

static inline struct dataexchange *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dataexchange, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE dataexchange_QueryInterface(IActivationFactory *iface, REFIID iid,
                                                             void **out)
{
    struct dataexchange *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_ICoreDragDropManagerStatics))
    {
        ICoreDragDropManagerStatics_AddRef(&impl->ICoreDragDropManagerStatics_iface);
        *out = &impl->ICoreDragDropManagerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dataexchange_AddRef(IActivationFactory *iface)
{
    struct dataexchange *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE dataexchange_Release(IActivationFactory *iface)
{
    struct dataexchange *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE dataexchange_GetIids(IActivationFactory *iface, ULONG *iid_count,
                                                      IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dataexchange_GetRuntimeClassName(IActivationFactory *iface,
                                                                  HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dataexchange_GetTrustLevel(IActivationFactory *iface,
                                                            TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dataexchange_ActivateInstance(IActivationFactory *iface,
                                                               IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    dataexchange_QueryInterface,
    dataexchange_AddRef,
    dataexchange_Release,
    /* IInspectable methods */
    dataexchange_GetIids,
    dataexchange_GetRuntimeClassName,
    dataexchange_GetTrustLevel,
    /* IActivationFactory methods */
    dataexchange_ActivateInstance,
};

DEFINE_IINSPECTABLE(core_dragdrop_manager_statics, ICoreDragDropManagerStatics, struct dataexchange,
                    IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE core_dragdrop_manager_statics_GetForCurrentView(ICoreDragDropManagerStatics *iface,
                                                                                 ICoreDragDropManager **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct ICoreDragDropManagerStaticsVtbl core_dragdrop_manager_statics_vtbl =
{
    core_dragdrop_manager_statics_QueryInterface,
    core_dragdrop_manager_statics_AddRef,
    core_dragdrop_manager_statics_Release,
    /* IInspectable methods */
    core_dragdrop_manager_statics_GetIids,
    core_dragdrop_manager_statics_GetRuntimeClassName,
    core_dragdrop_manager_statics_GetTrustLevel,
    /* ICoreDragDropManagerStatics methods */
    core_dragdrop_manager_statics_GetForCurrentView,
};

static struct dataexchange dataexchange =
{
    {&activation_factory_vtbl},
    {&core_dragdrop_manager_statics_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &dataexchange.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
