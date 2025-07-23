/* WinRT Windows.ApplicationModel.DesignMode Implementation
 *
 * Copyright (C) 2025 Zhiyi Zhang for CodeWeavers
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(model);

struct design_mode_statics
{
    IActivationFactory IActivationFactory_iface;
    IDesignModeStatics IDesignModeStatics_iface;
    IDesignModeStatics2 IDesignModeStatics2_iface;
    LONG ref;
};

static inline struct design_mode_statics *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct design_mode_statics, IActivationFactory_iface);
}

static HRESULT WINAPI activation_factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    struct design_mode_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef(*out);
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IDesignModeStatics))
    {
        *out = &impl->IDesignModeStatics_iface;
        IDesignModeStatics_AddRef(*out);
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IDesignModeStatics2))
    {
        *out = &impl->IDesignModeStatics2_iface;
        IDesignModeStatics2_AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef(IActivationFactory *iface)
{
    struct design_mode_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI activation_factory_Release(IActivationFactory *iface)
{
    struct design_mode_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids(IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName(IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel(IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
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

DEFINE_IINSPECTABLE(design_mode_statics, IDesignModeStatics, struct design_mode_statics, IActivationFactory_iface)

static HRESULT WINAPI design_mode_statics_DesignModeEnabled(IDesignModeStatics *iface, boolean *value)
{
    FIXME("iface %p, value %p.\n", iface, value);

    return E_NOTIMPL;
}

static const struct IDesignModeStaticsVtbl design_mode_statics_vtbl =
{
    design_mode_statics_QueryInterface,
    design_mode_statics_AddRef,
    design_mode_statics_Release,
    /* IInspectable methods */
    design_mode_statics_GetIids,
    design_mode_statics_GetRuntimeClassName,
    design_mode_statics_GetTrustLevel,
    /* IDesignModeStatics methods */
    design_mode_statics_DesignModeEnabled,
};

DEFINE_IINSPECTABLE(design_mode_statics2, IDesignModeStatics2, struct design_mode_statics, IActivationFactory_iface)

static HRESULT WINAPI design_mode_statics2_DesignMode2Enabled(IDesignModeStatics2 *iface, boolean *value)
{
    FIXME("iface %p, value %p.\n", iface, value);

    return E_NOTIMPL;
}

static const struct IDesignModeStatics2Vtbl design_mode_statics2_vtbl =
{
    design_mode_statics2_QueryInterface,
    design_mode_statics2_AddRef,
    design_mode_statics2_Release,
    /* IInspectable methods */
    design_mode_statics2_GetIids,
    design_mode_statics2_GetRuntimeClassName,
    design_mode_statics2_GetTrustLevel,
    /* IDesignModeStatics2 methods */
    design_mode_statics2_DesignMode2Enabled,
};

static struct design_mode_statics design_mode_statics =
{
    {&activation_factory_vtbl},
    {&design_mode_statics_vtbl},
    {&design_mode_statics2_vtbl},
    1,
};

IActivationFactory *design_mode_factory = &design_mode_statics.IActivationFactory_iface;
