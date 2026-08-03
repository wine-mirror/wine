/* WinRT Windows.Graphics.Display.DisplayInformation implementation
 *
 * Copyright 2024 Onni Kukkonen
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
#include <assert.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

struct displayinfo_impl
{
    IActivationFactory IActivationFactory_iface;
    IDisplayInformation IDisplayInformation_iface;
    IDisplayInformationStatics IDisplayInformationStatics_iface;
    LONG ref;
};

static inline struct displayinfo_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct displayinfo_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct displayinfo_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IDisplayInformationStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IDisplayInformationStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct displayinfo_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct displayinfo_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
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

static inline struct displayinfo_impl *impl_from_IDisplayInformationStatics(IDisplayInformationStatics *iface)
{
    return CONTAINING_RECORD(iface, struct displayinfo_impl, IDisplayInformationStatics_iface);
}

static HRESULT WINAPI displayinfo_statics_QueryInterface(
        IDisplayInformationStatics *iface, REFIID iid, void **out)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformationStatics(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IDisplayInformationStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IDisplayInformationStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI displayinfo_statics_AddRef(IDisplayInformationStatics *iface)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformationStatics(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI displayinfo_statics_Release(IDisplayInformationStatics *iface)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformationStatics(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI displayinfo_statics_GetIids(
        IDisplayInformationStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_statics_GetRuntimeClassName(
        IDisplayInformationStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_statics_GetTrustLevel(
        IDisplayInformationStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_statics_GetForCurrentView(
        IDisplayInformationStatics *iface, IDisplayInformation** current)
{
    struct displayinfo_impl *impl = impl_from_IDisplayInformationStatics(iface);

    FIXME("iface %p, current %p stub!\n", iface, current);
    *current = &impl->IDisplayInformation_iface;
    return S_OK;
}

static HRESULT WINAPI displayinfo_statics_get_AutoRotationPreferences(
        IDisplayInformationStatics *iface, DisplayOrientations* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_statics_set_AutoRotationPreferences(
        IDisplayInformationStatics *iface, DisplayOrientations value)
{
    FIXME("iface %p, value %d stub!\n", iface, value);
    return S_OK;
}

static HRESULT WINAPI displayinfo_statics_add_DisplayContentsInvalidated(
        IDisplayInformationStatics *iface, ITypedEventHandler_DisplayInformation_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI displayinfo_statics_remove_DisplayContentsInvalidated(
        IDisplayInformationStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static const struct IDisplayInformationStaticsVtbl displayinfo_statics_impl_vtbl =
{
    displayinfo_statics_QueryInterface,
    displayinfo_statics_AddRef,
    displayinfo_statics_Release,
    /* IInspectable methods */
    displayinfo_statics_GetIids,
    displayinfo_statics_GetRuntimeClassName,
    displayinfo_statics_GetTrustLevel,
    /* IDisplayInformationStatics methods */
    displayinfo_statics_GetForCurrentView,
    displayinfo_statics_get_AutoRotationPreferences,
    displayinfo_statics_set_AutoRotationPreferences,
    displayinfo_statics_add_DisplayContentsInvalidated,
    displayinfo_statics_remove_DisplayContentsInvalidated,
};

static inline struct displayinfo_impl *impl_from_IDisplayInformation(IDisplayInformation *iface)
{
    return CONTAINING_RECORD(iface, struct displayinfo_impl, IDisplayInformation_iface);
}

static HRESULT WINAPI displayinfo_QueryInterface(
        IDisplayInformation *iface, REFIID iid, void **out)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformation(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IDisplayInformation))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IDisplayInformation_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI displayinfo_AddRef(IDisplayInformation *iface)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformation(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI displayinfo_Release(IDisplayInformation *iface)
{
    struct displayinfo_impl *factory = impl_from_IDisplayInformation(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI displayinfo_GetIids(
        IDisplayInformation *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_GetRuntimeClassName(
        IDisplayInformation *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_GetTrustLevel(
        IDisplayInformation *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_get_CurrentOrientation(
        IDisplayInformation *iface, DisplayOrientations* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = DisplayOrientations_Landscape;
    return S_OK;
}

static HRESULT WINAPI displayinfo_get_NativeOrientation(
        IDisplayInformation *iface, DisplayOrientations* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = DisplayOrientations_Landscape;
    return S_OK;
}

static HRESULT WINAPI displayinfo_add_OrientationChanged(
        IDisplayInformation *iface, ITypedEventHandler_DisplayInformation_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI displayinfo_remove_OrientationChanged(
        IDisplayInformation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI displayinfo_get_ResolutionScale(
        IDisplayInformation *iface, ResolutionScale* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_get_LogicalDpi(
        IDisplayInformation *iface, FLOAT* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 96.0f;
    return S_OK;
}

static HRESULT WINAPI displayinfo_get_RawDpiX(
        IDisplayInformation *iface, FLOAT* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0.0f;
    return S_OK;
}

static HRESULT WINAPI displayinfo_get_RawDpiY(
        IDisplayInformation *iface, FLOAT* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0.0f;
    return S_OK;
}

static HRESULT WINAPI displayinfo_add_DpiChanged(
        IDisplayInformation *iface, ITypedEventHandler_DisplayInformation_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI displayinfo_remove_DpiChanged(
        IDisplayInformation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI displayinfo_get_StereoEnabled(
        IDisplayInformation *iface, boolean* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_add_StereoEnabledChanged(
        IDisplayInformation *iface, ITypedEventHandler_DisplayInformation_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI displayinfo_remove_StereoEnabledChanged(
        IDisplayInformation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI displayinfo_GetColorProfileAsync(
        IDisplayInformation *iface, IAsyncOperation_IRandomAccessStream** asyncInfo)
{
    FIXME("iface %p, asyncInfo %p stub!\n", iface, asyncInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI displayinfo_add_ColorProfileChanged(
        IDisplayInformation *iface, ITypedEventHandler_DisplayInformation_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI displayinfo_remove_ColorProfileChanged(
        IDisplayInformation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}


static const struct IDisplayInformationVtbl displayinfo_impl_vtbl =
{
    displayinfo_QueryInterface,
    displayinfo_AddRef,
    displayinfo_Release,
    /* IInspectable methods */
    displayinfo_GetIids,
    displayinfo_GetRuntimeClassName,
    displayinfo_GetTrustLevel,
    /* IDisplayInformation methods */
    displayinfo_get_CurrentOrientation,
    displayinfo_get_NativeOrientation,
    displayinfo_add_OrientationChanged,
    displayinfo_remove_OrientationChanged,
    displayinfo_get_ResolutionScale,
    displayinfo_get_LogicalDpi,
    displayinfo_get_RawDpiX,
    displayinfo_get_RawDpiY,
    displayinfo_add_DpiChanged,
    displayinfo_remove_DpiChanged,
    displayinfo_get_StereoEnabled,
    displayinfo_add_StereoEnabledChanged,
    displayinfo_remove_StereoEnabledChanged,
    displayinfo_GetColorProfileAsync,
    displayinfo_add_ColorProfileChanged,
    displayinfo_remove_ColorProfileChanged,
};

static struct displayinfo_impl displayinfo_impl_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .IDisplayInformationStatics_iface.lpVtbl = &displayinfo_statics_impl_vtbl,
    .IDisplayInformation_iface.lpVtbl = &displayinfo_impl_vtbl,
    .ref = 1,
};

IActivationFactory *display_factory = &displayinfo_impl_global.IActivationFactory_iface;
