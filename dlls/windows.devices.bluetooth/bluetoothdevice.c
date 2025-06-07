/* BluetoothDevice, BluetoothLEDevice Implementation
 *
 * Copyright 2025 Vibhav Pant
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

WINE_DEFAULT_DEBUG_CHANNEL( bluetooth );

struct bluetoothdevice_statics
{
    IActivationFactory IActivationFactory_iface;
    IBluetoothDeviceStatics IBluetoothDeviceStatics_iface;
    LONG ref;
};

static inline struct bluetoothdevice_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct bluetoothdevice_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct bluetoothdevice_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "(%p, %s, %p) %ld\n", iface, debugstr_guid( iid ), out, impl->ref );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef(( *out = &impl->IActivationFactory_iface ));
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IBluetoothDeviceStatics))
    {
        IBluetoothDeviceStatics_AddRef(( *out = &impl->IBluetoothDeviceStatics_iface ));
        return S_OK;
    }
    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct bluetoothdevice_statics *impl = impl_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct bluetoothdevice_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "(%p, %p): stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory */
    factory_ActivateInstance
};

DEFINE_IINSPECTABLE( bluetoothdevice_statics, IBluetoothDeviceStatics, struct bluetoothdevice_statics, IActivationFactory_iface );

static HRESULT WINAPI bluetoothdevice_statics_FromIdAsync( IBluetoothDeviceStatics *iface, HSTRING id, IAsyncOperation_BluetoothDevice **async_op )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_hstring( id ), async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothdevice_statics_FromHostNameAsync( IBluetoothDeviceStatics *iface, IHostName *name, IAsyncOperation_BluetoothDevice **async_op )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, name, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothdevice_statics_FromBluetoothAddressAsync( IBluetoothDeviceStatics *iface,
                                                                         UINT64 address,
                                                                         IAsyncOperation_BluetoothDevice **async_op )
{
    FIXME( "(%p, %#I64x, %p): stub!\n", iface, address, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothdevice_statics_GetDeviceSelector( IBluetoothDeviceStatics *iface, HSTRING *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    *value = NULL;
    return E_NOTIMPL;
}

static const IBluetoothDeviceStaticsVtbl bluetoothdevice_statics_vtbl =
{
    bluetoothdevice_statics_QueryInterface,
    bluetoothdevice_statics_AddRef,
    bluetoothdevice_statics_Release,
    /* IInspectable */
    bluetoothdevice_statics_GetIids,
    bluetoothdevice_statics_GetRuntimeClassName,
    bluetoothdevice_statics_GetTrustLevel,
    /* IBluetoothDeviceStatics */
    bluetoothdevice_statics_FromIdAsync,
    bluetoothdevice_statics_FromHostNameAsync,
    bluetoothdevice_statics_FromBluetoothAddressAsync,
    bluetoothdevice_statics_GetDeviceSelector,
};

struct bluetoothledevice_statics
{
    IActivationFactory IActivationFactory_iface;
    IBluetoothLEDeviceStatics IBluetoothLEDeviceStatics_iface;
    LONG ref;
};

static inline struct bluetoothledevice_statics *ble_device_impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct bluetoothledevice_statics, IActivationFactory_iface );
}

static HRESULT WINAPI ble_device_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct bluetoothledevice_statics *impl = ble_device_impl_from_IActivationFactory( iface );

    TRACE( "(%p, %s, %p) %ld\n", iface, debugstr_guid( iid ), out, impl->ref );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef(( *out = &impl->IActivationFactory_iface ));
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IBluetoothLEDeviceStatics ))
    {
        IBluetoothLEDeviceStatics_AddRef(( *out = &impl->IBluetoothLEDeviceStatics_iface ));
        return S_OK;
    }
    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI ble_device_factory_AddRef( IActivationFactory *iface )
{
    struct bluetoothledevice_statics *impl = ble_device_impl_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI ble_device_factory_Release( IActivationFactory *iface )
{
    struct bluetoothledevice_statics *impl = ble_device_impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );
    return ref;
}

static HRESULT WINAPI ble_device_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "(%p, %p): stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl ble_device_factory_vtbl =
{
    ble_device_factory_QueryInterface,
    ble_device_factory_AddRef,
    ble_device_factory_Release,
    /* IInspectable */
    ble_device_factory_GetIids,
    ble_device_factory_GetRuntimeClassName,
    ble_device_factory_GetTrustLevel,
    /* IActivationFactory */
    ble_device_factory_ActivateInstance
};

DEFINE_IINSPECTABLE( bluetoothledevice_statics, IBluetoothLEDeviceStatics, struct bluetoothledevice_statics, IActivationFactory_iface );

static HRESULT WINAPI bluetoothledevice_statics_FromIdAsync( IBluetoothLEDeviceStatics *iface, HSTRING id, IAsyncOperation_BluetoothLEDevice **async_op )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_hstring( id ), async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothledevice_statics_FromBluetoothAddressAsync( IBluetoothLEDeviceStatics *iface, UINT64 addr, IAsyncOperation_BluetoothLEDevice **async_op )
{
    FIXME( "(%p, %#I64x, %p): stub!\n", iface, addr, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothledevice_statics_GetDeviceSelector( IBluetoothLEDeviceStatics *iface, HSTRING *result )
{
    FIXME( "(%p, %p): stub!\n", iface, result );
    return E_NOTIMPL;
}

static const IBluetoothLEDeviceStaticsVtbl bluetoothledevice_statics_vtbl =
{
    /* IUnknown */
    bluetoothledevice_statics_QueryInterface,
    bluetoothledevice_statics_AddRef,
    bluetoothledevice_statics_Release,
    /* IInspectable */
    bluetoothledevice_statics_GetIids,
    bluetoothledevice_statics_GetRuntimeClassName,
    bluetoothledevice_statics_GetTrustLevel,
    /* IBluetoothLEDeviceStatics */
    bluetoothledevice_statics_FromIdAsync,
    bluetoothledevice_statics_FromBluetoothAddressAsync,
    bluetoothledevice_statics_GetDeviceSelector
};


static struct bluetoothdevice_statics bluetoothdevice_statics =
{
    {&factory_vtbl},
    {&bluetoothdevice_statics_vtbl},
    1
};

IActivationFactory *bluetoothdevice_statics_factory = &bluetoothdevice_statics.IActivationFactory_iface;

static struct bluetoothledevice_statics bluetoothledevice_statics =
{
    {&ble_device_factory_vtbl},
    {&bluetoothledevice_statics_vtbl},
    1
};

IActivationFactory *bluetoothledevice_statics_factory = &bluetoothledevice_statics.IActivationFactory_iface;
