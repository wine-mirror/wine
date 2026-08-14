/* WinRT Windows.Devices.Geolocation.Geolocator Implementation
 *
 * Copyright 2023 Fabian Maurer
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

#include "initguid.h"
#include "private.h"
#include "weakref.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(geolocator);

struct geolocator_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

struct geolocator
{
    IGeolocator IGeolocator_iface;
    struct weak_reference_source weak_reference_source;
};

static inline struct geolocator *impl_from_IGeolocator(IGeolocator *iface)
{
    return CONTAINING_RECORD(iface, struct geolocator, IGeolocator_iface);
}

static HRESULT WINAPI geolocator_QueryInterface(IGeolocator *iface, REFIID iid, void **out)
{
    struct geolocator *impl = impl_from_IGeolocator(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IGeolocator))
    {
        *out = &impl->IGeolocator_iface;
        IInspectable_AddRef(*out);
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IWeakReferenceSource))
    {
        *out = &impl->weak_reference_source.IWeakReferenceSource_iface;
        IWeakReferenceSource_AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI geolocator_AddRef(IGeolocator *iface)
{
    struct geolocator *impl = impl_from_IGeolocator(iface);
    ULONG ref = weak_reference_strong_add_ref(&impl->weak_reference_source);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI geolocator_Release(IGeolocator *iface)
{
    struct geolocator *impl = impl_from_IGeolocator(iface);
    ULONG ref = weak_reference_strong_release(&impl->weak_reference_source);
    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI geolocator_GetIids(IGeolocator *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI geolocator_GetRuntimeClassName(IGeolocator *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI geolocator_GetTrustLevel(IGeolocator *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_get_DesiredAccuracy(IGeolocator *iface, PositionAccuracy *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_set_DesiredAccuracy(IGeolocator *iface, PositionAccuracy value)
{
    FIXME("iface %p, value %d stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_get_MovementThreshold(IGeolocator *iface, DOUBLE *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_set_MovementThreshold(IGeolocator *iface, DOUBLE value)
{
    FIXME("iface %p, value %f stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_get_ReportInterval(IGeolocator *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_set_ReportInterval(IGeolocator *iface, UINT32 value)
{
    FIXME("iface %p, value %u stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_LocationStatus(IGeolocator *iface, PositionStatus *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_GetGeopositionAsync(IGeolocator *iface, IAsyncOperation_Geoposition **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_GetGeopositionAsyncWithAgeAndTimeout(IGeolocator *iface, TimeSpan maximum_age, TimeSpan timeout, IAsyncOperation_Geoposition **value)
{
    FIXME("iface %p, maximum_age %#I64x, timeout %#I64x, value %p stub.\n", iface, maximum_age.Duration, timeout.Duration, value);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_add_PositionChanged(IGeolocator *iface, ITypedEventHandler_Geolocator_PositionChangedEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub.\n", iface, handler, token);
    return S_OK;
}

HRESULT WINAPI geolocator_remove_PositionChanged(IGeolocator *iface, EventRegistrationToken token)
{
   FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
   return E_NOTIMPL;
}

HRESULT WINAPI geolocator_add_StatusChanged(IGeolocator *iface, ITypedEventHandler_Geolocator_StatusChangedEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub.\n", iface, handler, token);
    return E_NOTIMPL;
}

HRESULT WINAPI geolocator_remove_StatusChanged(IGeolocator *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct IGeolocatorVtbl geolocator_vtbl =
{
    geolocator_QueryInterface,
    geolocator_AddRef,
    geolocator_Release,
    /* IInspectable methods */
    geolocator_GetIids,
    geolocator_GetRuntimeClassName,
    geolocator_GetTrustLevel,
    /* IGeolocator methods */
    geolocator_get_DesiredAccuracy,
    geolocator_set_DesiredAccuracy,
    geolocator_get_MovementThreshold,
    geolocator_set_MovementThreshold,
    geolocator_get_ReportInterval,
    geolocator_set_ReportInterval,
    geolocator_LocationStatus,
    geolocator_GetGeopositionAsync,
    geolocator_GetGeopositionAsyncWithAgeAndTimeout,
    geolocator_add_PositionChanged,
    geolocator_remove_PositionChanged,
    geolocator_add_StatusChanged,
    geolocator_remove_StatusChanged,
};

static inline struct geolocator_statics *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct geolocator_statics, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    struct geolocator_statics *impl = impl_from_IActivationFactory(iface);

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

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct geolocator_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct geolocator_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);
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

static const struct IActivationFactoryVtbl factory_vtbl;

static HRESULT WINAPI factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
{
    struct geolocator *impl;
    HRESULT hr;

    TRACE("iface %p, instance %p.\n", iface, instance);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IGeolocator_iface.lpVtbl = &geolocator_vtbl;
    if (FAILED(hr = weak_reference_source_init( &impl->weak_reference_source,
                                                (IUnknown *)&impl->IGeolocator_iface )))
    {
        *instance = NULL;
        free( impl );
        return hr;
    }

    *instance = (IInspectable *)&impl->IGeolocator_iface;
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

static struct geolocator_statics geolocator_statics =
{
    {&factory_vtbl},
    1,
};

static IActivationFactory *geolocator_factory = &geolocator_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    const WCHAR *name = WindowsGetStringRawBuffer(classid, NULL);

    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);

    *factory = NULL;

    if (!wcscmp(name, RuntimeClass_Windows_Devices_Geolocation_Geolocator))
        IActivationFactory_QueryInterface(geolocator_factory, &IID_IActivationFactory, (void **)factory);

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
