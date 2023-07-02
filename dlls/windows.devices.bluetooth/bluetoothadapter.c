/* WinRT Windows.Devices.Bluetooth BluetoothAdapter Implementation
 *
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

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bluetooth);

struct bluetoothadapter
{
    IActivationFactory IActivationFactory_iface;
    IBluetoothAdapterStatics IBluetoothAdapterStatics_iface;
    LONG ref;
};

static inline struct bluetoothadapter *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct bluetoothadapter, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct bluetoothadapter *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IBluetoothAdapterStatics ))
    {
        *out = &impl->IBluetoothAdapterStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct bluetoothadapter *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct bluetoothadapter *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( bluetoothadapter_statics, IBluetoothAdapterStatics, struct bluetoothadapter, IActivationFactory_iface )

static HRESULT WINAPI bluetoothadapter_statics_GetDeviceSelector( IBluetoothAdapterStatics *iface, HSTRING *result )
{
    static const WCHAR *default_res = L"System.Devices.InterfaceClassGuid:=\"{92383B0E-F90E-4AC9-8D44-8C2D0D0EBDA2}\" "
                                      L"AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";

    TRACE( "iface %p, result %p.\n", iface, result );

    if (!result) return E_POINTER;
    return WindowsCreateString( default_res, wcslen(default_res), result );
}

static HRESULT WINAPI bluetoothadapter_statics_FromIdAsync( IBluetoothAdapterStatics *iface, HSTRING id, IAsyncOperation_BluetoothAdapter **operation )
{
    FIXME( "iface %p, id %s, operation %p stub!\n", iface, debugstr_hstring(id), operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_statics_GetDefaultAsync( IBluetoothAdapterStatics *iface, IAsyncOperation_BluetoothAdapter **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static const struct IBluetoothAdapterStaticsVtbl bluetoothadapter_statics_vtbl =
{
    bluetoothadapter_statics_QueryInterface,
    bluetoothadapter_statics_AddRef,
    bluetoothadapter_statics_Release,
    /* IInspectable methods */
    bluetoothadapter_statics_GetIids,
    bluetoothadapter_statics_GetRuntimeClassName,
    bluetoothadapter_statics_GetTrustLevel,
    /* IBluetoothAdapterStatics methods */
    bluetoothadapter_statics_GetDeviceSelector,
    bluetoothadapter_statics_FromIdAsync,
    bluetoothadapter_statics_GetDefaultAsync,
};

static struct bluetoothadapter bluetoothadapter_statics =
{
    {&factory_vtbl},
    {&bluetoothadapter_statics_vtbl},
    1,
};

IActivationFactory *bluetoothadapter_factory = &bluetoothadapter_statics.IActivationFactory_iface;
