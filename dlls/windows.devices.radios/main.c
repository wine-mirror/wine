/* WinRT Windows.Devices.Radios Implementation
 *
 * Copyright (C) 2026 Mohamad Al-Jaf
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(radios);

struct radio_statics
{
    IActivationFactory IActivationFactory_iface;
    IRadioStatics IRadioStatics_iface;
    LONG ref;
};

static inline struct radio_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct radio_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct radio_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IRadioStatics ))
    {
        *out = &impl->IRadioStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct radio_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct radio_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    /* IUnknown methods */
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

DEFINE_IINSPECTABLE( radio_statics, IRadioStatics, struct radio_statics, IActivationFactory_iface )

static HRESULT get_radios_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async )
{
    struct vector_iids iids =
    {
        .iterable = &IID_IIterable_Radio,
        .iterator = &IID_IIterator_Radio,
        .vector = &IID_IVector_IInspectable,
        .view = &IID_IVectorView_Radio,
    };
    IVector_IInspectable *vector;
    IVectorView_Radio *view;
    HRESULT hr;

    FIXME( "stub!\n" );

    if (FAILED(hr = vector_create( &iids, (void **)&vector ))) return hr;

    hr = IVector_IInspectable_GetView( vector, (IVectorView_IInspectable **)&view );
    IVector_IInspectable_Release( vector );
    if (FAILED(hr)) return hr;

    result->vt = VT_UNKNOWN;
    result->punkVal = (IUnknown *)view;
    return S_OK;
}

static HRESULT WINAPI radio_statics_GetRadiosAsync( IRadioStatics *iface, IAsyncOperation_IVectorView_Radio **value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    return async_operation_inspectable_create( &IID_IAsyncOperation_IVectorView_Radio, NULL, NULL, get_radios_async, (IAsyncOperation_IInspectable **)value );
}

static HRESULT WINAPI radio_statics_GetDeviceSelector( IRadioStatics *iface, HSTRING *selector )
{
    FIXME( "iface %p, selector %p stub!\n", iface, selector );
    return E_NOTIMPL;
}

static HRESULT WINAPI radio_statics_FromIdAsync( IRadioStatics *iface, HSTRING id, IAsyncOperation_Radio **value )
{
    FIXME( "iface %p, id %s, value %p stub!\n", iface, debugstr_hstring( id ), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI radio_statics_RequestAccessAsync( IRadioStatics *iface, IAsyncOperation_RadioAccessStatus **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IRadioStaticsVtbl radio_statics_vtbl =
{
    /* IUnknown methods */
    radio_statics_QueryInterface,
    radio_statics_AddRef,
    radio_statics_Release,
    /* IInspectable methods */
    radio_statics_GetIids,
    radio_statics_GetRuntimeClassName,
    radio_statics_GetTrustLevel,
    /* IRadioStatics methods */
    radio_statics_GetRadiosAsync,
    radio_statics_GetDeviceSelector,
    radio_statics_FromIdAsync,
    radio_statics_RequestAccessAsync,
};

static struct radio_statics radio_statics =
{
    {&factory_vtbl},
    {&radio_statics_vtbl},
    1,
};

static IActivationFactory *radio_factory = &radio_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Devices_Radios_Radio ))
        IActivationFactory_QueryInterface( radio_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
