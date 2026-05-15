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
#include "roapi.h"
#include "setupapi.h"
#include "initguid.h"
#include "devpkey.h"
#include "bthledef.h"
#include "ddk/bthguid.h"

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

struct ble_device
{
    IBluetoothLEDevice IBluetoothLEDevice_iface;
    HSTRING id;
    UINT64 addr;
    LONG ref;
};

static inline struct ble_device *impl_from_IBluetoothLEDevice( IBluetoothLEDevice *iface )
{
    return CONTAINING_RECORD( iface, struct ble_device, IBluetoothLEDevice_iface );
}

static HRESULT WINAPI ble_device_QueryInterface( IBluetoothLEDevice *iface, REFIID iid, void **out )
{
    struct ble_device *impl = impl_from_IBluetoothLEDevice( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IBluetoothLEDevice ))
    {
        IBluetoothLEDevice_AddRef( (*out = &impl->IBluetoothLEDevice_iface) );
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI ble_device_AddRef( IBluetoothLEDevice *iface )
{
     struct ble_device *impl = impl_from_IBluetoothLEDevice( iface );
     TRACE( "(%p)\n", iface );
     return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI ble_device_Release( IBluetoothLEDevice *iface )
{
    struct ble_device *impl = impl_from_IBluetoothLEDevice( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );

    if (!ref)
    {
        WindowsDeleteString( impl->id );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI ble_device_GetIids( IBluetoothLEDevice *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_GetRuntimeClassName( IBluetoothLEDevice *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_GetTrustLevel( IBluetoothLEDevice *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_get_DeviceId( IBluetoothLEDevice *iface, HSTRING *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_get_Name( IBluetoothLEDevice *iface, HSTRING *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_get_GattServices( IBluetoothLEDevice *iface, IVectorView_GattDeviceService **services )
{
    FIXME( "(%p, %p): stub!\n", iface, services );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_get_ConnectionStatus( IBluetoothLEDevice *iface, BluetoothConnectionStatus *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_get_BluetoothAddress( IBluetoothLEDevice *iface, UINT64 *value )
{
    struct ble_device *impl = impl_from_IBluetoothLEDevice( iface );
    TRACE( "(%p, %p)\n", iface, value );
    *value = impl->addr;
    return S_OK;
}

static HRESULT WINAPI ble_device_GetGattService( IBluetoothLEDevice *iface, GUID uuid, IGattDeviceService **service )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_guid( &uuid ), service );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_add_NameChanged( IBluetoothLEDevice *iface, ITypedEventHandler_BluetoothLEDevice_IInspectable *handler,
                                                  EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_remove_NameChanged( IBluetoothLEDevice *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64d): stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_add_GattServicesChanged( IBluetoothLEDevice *iface, ITypedEventHandler_BluetoothLEDevice_IInspectable *handler,
                                                          EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_remove_GattServicesChanged( IBluetoothLEDevice *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64d): stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_add_ConnectionStatusChanged( IBluetoothLEDevice *iface, ITypedEventHandler_BluetoothLEDevice_IInspectable *handler,
                                                              EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI ble_device_remove_ConnectionStatusChanged( IBluetoothLEDevice *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64d): stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static const IBluetoothLEDeviceVtbl ble_device_vtbl = {
    /* IUnknown */
    ble_device_QueryInterface,
    ble_device_AddRef,
    ble_device_Release,
    /* IInspectable */
    ble_device_GetIids,
    ble_device_GetRuntimeClassName,
    ble_device_GetTrustLevel,
    /* IBluetoothLEDevice */
    ble_device_get_DeviceId,
    ble_device_get_Name,
    ble_device_get_GattServices,
    ble_device_get_ConnectionStatus,
    ble_device_get_BluetoothAddress,
    ble_device_GetGattService,
    ble_device_add_NameChanged,
    ble_device_remove_NameChanged,
    ble_device_add_GattServicesChanged,
    ble_device_remove_GattServicesChanged,
    ble_device_add_ConnectionStatusChanged,
    ble_device_remove_ConnectionStatusChanged,
};

static HRESULT ble_device_create( IBluetoothLEDevice **device, const WCHAR *id, UINT64 addr )
{
    struct ble_device *impl;
    HRESULT hr;

    TRACE( "(%p, %s, %I64x)\n", device, debugstr_w( id ), addr );

    if (!(impl = calloc( 1, sizeof( *impl ))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = WindowsCreateString( id, wcslen( id ), &impl->id )))
    {
        free( impl );
        return hr;
    }
    impl->ref = 1;
    impl->addr = addr;
    impl->IBluetoothLEDevice_iface.lpVtbl = &ble_device_vtbl;
    *device = &impl->IBluetoothLEDevice_iface;
    return S_OK;
}

static HRESULT bluetoothledevice_get_device_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async )
{
    char buffer[sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA_W ) + MAX_PATH * sizeof( WCHAR )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    SP_DEVICE_INTERFACE_DATA iface_data = { .cbSize = sizeof( iface_data ) };
    IBluetoothLEDevice *device;
    BOOL found = FALSE;
    HDEVINFO devinfo;
    DWORD idx = 0;
    UINT64 addr;
    HRESULT hr;

    if (!called_async) return STATUS_PENDING;
    if (FAILED(hr = IPropertyValue_GetUInt64( (IPropertyValue *)param, &addr )))
        return hr;

    devinfo = SetupDiGetClassDevsW( &GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    if (devinfo == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32( GetLastError() );
    iface_detail->cbSize = sizeof( *iface_detail );
    result->vt = VT_NULL;
    while (SetupDiEnumDeviceInterfaces( devinfo, NULL, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, idx++, &iface_data ))
    {
        SP_DEVINFO_DATA devinfo_data = { .cbSize = sizeof( devinfo_data ) };
        WCHAR addr_str[13];
        DEVPROPTYPE type;
        UINT64 addr2 = 0;

        if (!SetupDiGetDeviceInterfaceDetailW( devinfo, &iface_data, iface_detail, sizeof( buffer ), NULL, &devinfo_data ))
            continue;
        if (!SetupDiGetDevicePropertyW( devinfo, &devinfo_data, &DEVPKEY_Bluetooth_DeviceAddress, &type, (BYTE *)addr_str, sizeof( addr_str ), NULL, 0 ))
            continue;
        if (swscanf( addr_str, L"%I64x", &addr2 ) != 1)
            continue;
        if (addr == addr2)
        {
            found = TRUE;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList( devinfo );
    if (!found)
        return S_OK;
    if (FAILED(hr = ble_device_create( &device, iface_detail->DevicePath, addr )))
        return hr;
    result->vt = VT_UNKNOWN;
    result->punkVal = (IUnknown *)device;
    return S_OK;
}

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

static HRESULT WINAPI bluetoothledevice_statics_FromBluetoothAddressAsync( IBluetoothLEDeviceStatics *iface,
                                                                           UINT64 addr,
                                                                           IAsyncOperation_BluetoothLEDevice **async_op )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Foundation_PropertyValue;
    IPropertyValueStatics *statics;
    IPropertyValue *addr_val;
    HSTRING_HEADER hdr;
    HSTRING str;
    HRESULT hr;

    TRACE( "(%p, %#I64x, %p)\n", iface, addr, async_op );

    if (FAILED(hr = WindowsCreateStringReference( class_name, wcslen( class_name ), &hdr, &str ))) return hr;
    if (FAILED(hr = RoGetActivationFactory( str, &IID_IPropertyValueStatics, (void **)&statics ))) return hr;
    if (FAILED(hr = IPropertyValueStatics_CreateUInt64( statics, addr, (IInspectable **)&addr_val )))
    {
        IPropertyValueStatics_Release( statics );
        return hr;
    }
    IPropertyValueStatics_Release( statics );

    return async_operation_inspectable_create( &IID_IAsyncOperation_BluetoothLEDevice, (IUnknown *)iface, (IUnknown *)addr_val, bluetoothledevice_get_device_async,
                                               (IAsyncOperation_IInspectable **)async_op );
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
