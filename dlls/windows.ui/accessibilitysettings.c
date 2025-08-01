/* WinRT Windows.UI.ViewManagement.AccessibilitySettings Implementation
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

struct accessibilitysettings
{
    IAccessibilitySettings IAccessibilitySettings_iface;
    LONG ref;
};

static inline struct accessibilitysettings *impl_from_IAccessibilitySettings(IAccessibilitySettings *iface)
{
    return CONTAINING_RECORD(iface, struct accessibilitysettings, IAccessibilitySettings_iface);
}

static HRESULT WINAPI accessibilitysettings_QueryInterface(IAccessibilitySettings *iface,
                                                           REFIID iid, void **out)
{
    struct accessibilitysettings *impl = impl_from_IAccessibilitySettings(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_IAccessibilitySettings))
    {
        *out = &impl->IAccessibilitySettings_iface;
        IAccessibilitySettings_AddRef(&impl->IAccessibilitySettings_iface);
        return S_OK;
    }

    *out = NULL;
    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI accessibilitysettings_AddRef(IAccessibilitySettings *iface)
{
    struct accessibilitysettings *impl = impl_from_IAccessibilitySettings(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI accessibilitysettings_Release(IAccessibilitySettings *iface)
{
    struct accessibilitysettings *impl = impl_from_IAccessibilitySettings(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(impl);
    return ref;
}

static HRESULT WINAPI accessibilitysettings_GetIids(IAccessibilitySettings *iface, ULONG *iid_count,
                                                    IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_GetRuntimeClassName(IAccessibilitySettings *iface,
                                                                HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_GetTrustLevel(IAccessibilitySettings *iface,
                                                          TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_get_HighContrast(IAccessibilitySettings *iface,
                                                             boolean *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_get_HighContrastScheme(IAccessibilitySettings *iface,
                                                                   HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_add_HighContrastChanged(IAccessibilitySettings *iface,
                                                                    ITypedEventHandler_AccessibilitySettings_IInspectable *handler,
                                                                    EventRegistrationToken *cookie)
{
    FIXME("iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessibilitysettings_remove_HighContrastChanged(IAccessibilitySettings *iface,
                                                                       EventRegistrationToken cookie)
{
    FIXME("iface %p, cookie %I64x stub!\n", iface, cookie.value);
    return E_NOTIMPL;
}

static const struct IAccessibilitySettingsVtbl accessibilitysettings_vtbl =
{
    accessibilitysettings_QueryInterface,
    accessibilitysettings_AddRef,
    accessibilitysettings_Release,
    /* IInspectable methods */
    accessibilitysettings_GetIids,
    accessibilitysettings_GetRuntimeClassName,
    accessibilitysettings_GetTrustLevel,
    /* IAccessibilitySettings methods */
    accessibilitysettings_get_HighContrast,
    accessibilitysettings_get_HighContrastScheme,
    accessibilitysettings_add_HighContrastChanged,
    accessibilitysettings_remove_HighContrastChanged,
};

struct factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct factory *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct factory, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    struct factory *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef(*out);
        return S_OK;
    }

    *out = NULL;
    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct factory *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct factory *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI factory_GetIids(IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
{
    struct accessibilitysettings *impl;

    TRACE("iface %p, instance %p.\n", iface, instance);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IAccessibilitySettings_iface.lpVtbl = &accessibilitysettings_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IAccessibilitySettings_iface;
    return S_OK;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

static struct factory factory =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *accessibilitysettings_factory = &factory.IActivationFactory_iface;
