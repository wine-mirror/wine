/* WinRT Windows.UI.ViewManagement.ApplicationView implementation
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(twinapi);

struct factory
{
    IActivationFactory IActivationFactory_iface;
    IApplicationViewStatics IApplicationViewStatics_iface;
    IApplicationViewStatics2 IApplicationViewStatics2_iface;
    LONG ref;
};

static inline struct factory *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct factory, IActivationFactory_iface);
}

static HRESULT WINAPI activation_factory_QueryInterface(IActivationFactory *iface, REFIID iid,
                                                        void **out)
{
    struct factory *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IActivationFactory_AddRef(&impl->IActivationFactory_iface);
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IApplicationViewStatics))
    {
        IApplicationViewStatics_AddRef(&impl->IApplicationViewStatics_iface);
        *out = &impl->IApplicationViewStatics_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IApplicationViewStatics2))
    {
        IApplicationViewStatics2_AddRef(&impl->IApplicationViewStatics2_iface);
        *out = &impl->IApplicationViewStatics2_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef(IActivationFactory *iface)
{
    struct factory *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI activation_factory_Release(IActivationFactory *iface)
{
    struct factory *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids(IActivationFactory *iface, ULONG *iid_count,
                                                 IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName(IActivationFactory *iface,
                                                             HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel(IActivationFactory *iface,
                                                       TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance(IActivationFactory *iface,
                                                          IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE(statics, IApplicationViewStatics, struct factory, IActivationFactory_iface)

static HRESULT WINAPI statics_Value(IApplicationViewStatics *iface, ApplicationViewState *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_TryUnsnap(IApplicationViewStatics *iface, boolean *success)
{
    FIXME("iface %p, success %p stub!\n", iface, success);
    return E_NOTIMPL;
}

static const struct IApplicationViewStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IApplicationViewStatics methods */
    statics_Value,
    statics_TryUnsnap,
};

DEFINE_IINSPECTABLE(statics2, IApplicationViewStatics2, struct factory, IActivationFactory_iface)

static HRESULT WINAPI statics2_GetForCurrentView(IApplicationViewStatics2 *iface, IApplicationView **current)
{
    FIXME("iface %p, current %p stub!\n", iface, current);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics2_get_TerminateAppOnFinalViewClose(IApplicationViewStatics2 *iface, boolean *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics2_put_TerminateAppOnFinalViewClose(IApplicationViewStatics2 *iface, boolean value)
{
    FIXME("iface %p, value %d stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IApplicationViewStatics2Vtbl statics2_vtbl =
{
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* IApplicationViewStatics2 methods */
    statics2_GetForCurrentView,
    statics2_get_TerminateAppOnFinalViewClose,
    statics2_put_TerminateAppOnFinalViewClose
};

static struct factory factory =
{
    {&activation_factory_vtbl},
    {&statics_vtbl},
    {&statics2_vtbl},
    1,
};

IActivationFactory *application_view_factory = &factory.IActivationFactory_iface;
