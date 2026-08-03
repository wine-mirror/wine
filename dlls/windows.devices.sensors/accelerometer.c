/* WinRT Windows.Devices.Sensors.Accelerometer implementation
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
#include <winuser.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(accelerometer);

struct accelerometer_impl {
    IActivationFactory IActivationFactory_iface;
    IAccelerometer IAccelerometer_iface;
    IAccelerometerStatics IAccelerometerStatics_iface;
    IAccelerometerReading IAccelerometerReading_iface;
    LONG ref;
};

static inline struct accelerometer_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct accelerometer_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct accelerometer_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IAccelerometerStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IAccelerometerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct accelerometer_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct accelerometer_impl *factory = impl_from_IActivationFactory(iface);
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

static inline struct accelerometer_impl *impl_from_IAccelerometerStatics(IAccelerometerStatics *iface)
{
    return CONTAINING_RECORD(iface, struct accelerometer_impl, IAccelerometerStatics_iface);
}

static HRESULT WINAPI accelerometer_static_QueryInterface(
        IAccelerometerStatics *iface, REFIID iid, void **out)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerStatics(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAccelerometerStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IAccelerometerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI accelerometer_static_AddRef(IAccelerometerStatics *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerStatics(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI accelerometer_static_Release(IAccelerometerStatics *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerStatics(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI accelerometer_static_GetIids(
        IAccelerometerStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_static_GetRuntimeClassName(
        IAccelerometerStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_static_GetTrustLevel(
        IAccelerometerStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_static_GetDefault(
        IAccelerometerStatics *iface, IAccelerometer **result)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerStatics(iface);
    
    *result = &factory->IAccelerometer_iface;
    return S_OK;
}

static const struct IAccelerometerStaticsVtbl accelerometer_statics_vtbl =
{
    accelerometer_static_QueryInterface,
    accelerometer_static_AddRef,
    accelerometer_static_Release,
    /* IInspectable methods */
    accelerometer_static_GetIids,
    accelerometer_static_GetRuntimeClassName,
    accelerometer_static_GetTrustLevel,
    /* IAccelerometerStatics methods */
    accelerometer_static_GetDefault,
};

static inline struct accelerometer_impl *impl_from_IAccelerometer(IAccelerometer *iface)
{
    return CONTAINING_RECORD(iface, struct accelerometer_impl, IAccelerometer_iface);
}

static HRESULT WINAPI accelerometer_QueryInterface(
        IAccelerometer *iface, REFIID iid, void **out)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometer(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAccelerometer))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IAccelerometer_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI accelerometer_AddRef(IAccelerometer *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometer(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI accelerometer_Release(IAccelerometer *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometer(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI accelerometer_GetIids(
        IAccelerometer *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_GetRuntimeClassName(
        IAccelerometer *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_GetTrustLevel(
        IAccelerometer *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_GetCurrentReading(
        IAccelerometer *iface, IAccelerometerReading** value)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometer(iface);

    //FIXME("iface %p, value %p stub!\n", iface, value);
    *value = &factory->IAccelerometerReading_iface;
    return S_OK;
}

static HRESULT WINAPI accelerometer_get_MinimumReportInterval(
        IAccelerometer *iface, UINT32* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT WINAPI accelerometer_put_ReportInterval(
        IAccelerometer *iface, UINT32 value)
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return S_OK;
}

static HRESULT WINAPI accelerometer_get_ReportInterval(
        IAccelerometer *iface, UINT32* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT WINAPI accelerometer_add_ReadingChanged(
        IAccelerometer *iface, ITypedEventHandler_Accelerometer_AccelerometerReadingChangedEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI accelerometer_remove_ReadingChanged(
        IAccelerometer *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %I64u stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI accelerometer_add_Shaken(
        IAccelerometer *iface, ITypedEventHandler_Accelerometer_AccelerometerShakenEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI accelerometer_remove_Shaken(
        IAccelerometer *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %I64u stub!\n", iface, token.value);
    return S_OK;
}


static const struct IAccelerometerVtbl accelerometer_vtbl =
{
    accelerometer_QueryInterface,
    accelerometer_AddRef,
    accelerometer_Release,
    /* IInspectable methods */
    accelerometer_GetIids,
    accelerometer_GetRuntimeClassName,
    accelerometer_GetTrustLevel,
    /* IAccelerometer methods */
    accelerometer_GetCurrentReading,
    accelerometer_get_MinimumReportInterval,
    accelerometer_put_ReportInterval,
    accelerometer_get_ReportInterval,
    accelerometer_add_ReadingChanged,
    accelerometer_remove_ReadingChanged,
    accelerometer_add_Shaken,
    accelerometer_remove_Shaken,
};

static inline struct accelerometer_impl *impl_from_IAccelerometerReading(IAccelerometerReading *iface)
{
    return CONTAINING_RECORD(iface, struct accelerometer_impl, IAccelerometerReading_iface);
}

static HRESULT WINAPI accelerometer_reading_QueryInterface(
        IAccelerometerReading *iface, REFIID iid, void **out)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerReading(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAccelerometerReading))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IAccelerometerReading_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI accelerometer_reading_AddRef(IAccelerometerReading *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerReading(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI accelerometer_reading_Release(IAccelerometerReading *iface)
{
    struct accelerometer_impl *factory = impl_from_IAccelerometerReading(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI accelerometer_reading_GetIids(
        IAccelerometerReading *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_reading_GetRuntimeClassName(
        IAccelerometerReading *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_reading_GetTrustLevel(
        IAccelerometerReading *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI accelerometer_reading_get_Timestamp(
        IAccelerometerReading *iface, DateTime *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    value->UniversalTime = 0;
    return S_OK;
}

static HRESULT WINAPI accelerometer_reading_get_AccelerationX(
        IAccelerometerReading *iface, DOUBLE *value)
{
    //FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT WINAPI accelerometer_reading_get_AccelerationY(
        IAccelerometerReading *iface, DOUBLE *value)
{
   // FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT WINAPI accelerometer_reading_get_AccelerationZ(
        IAccelerometerReading *iface, DOUBLE *value)
{
    //FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}


static const struct IAccelerometerReadingVtbl accelerometer_reading_vtbl =
{
    accelerometer_reading_QueryInterface,
    accelerometer_reading_AddRef,
    accelerometer_reading_Release,
    /* IInspectable methods */
    accelerometer_reading_GetIids,
    accelerometer_reading_GetRuntimeClassName,
    accelerometer_reading_GetTrustLevel,
    /* IAccelerometerReading methods */
    accelerometer_reading_get_Timestamp,
    accelerometer_reading_get_AccelerationX,
    accelerometer_reading_get_AccelerationY,
    accelerometer_reading_get_AccelerationZ,
};

static struct accelerometer_impl accelerometer_impl_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .IAccelerometer_iface.lpVtbl = &accelerometer_vtbl,
    .IAccelerometerStatics_iface.lpVtbl = &accelerometer_statics_vtbl,
    .IAccelerometerReading_iface.lpVtbl = &accelerometer_reading_vtbl,
    .ref = 1,
};

IActivationFactory *accelerometer_factory = &accelerometer_impl_global.IActivationFactory_iface;
