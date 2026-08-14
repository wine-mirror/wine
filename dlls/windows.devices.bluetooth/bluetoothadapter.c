/* WinRT Windows.Devices.Bluetooth BluetoothAdapter Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
 * Copyright (C) 2026 Vibhav Pant
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bluetooth);

struct bluetoothadapter_statics
{
    IActivationFactory IActivationFactory_iface;
    IBluetoothAdapterStatics IBluetoothAdapterStatics_iface;
    LONG ref;
};

static inline struct bluetoothadapter_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct bluetoothadapter_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct bluetoothadapter_statics *impl = impl_from_IActivationFactory( iface );

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
    struct bluetoothadapter_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct bluetoothadapter_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( bluetoothadapter_statics, IBluetoothAdapterStatics, struct bluetoothadapter_statics, IActivationFactory_iface )

static HRESULT WINAPI bluetoothadapter_statics_GetDeviceSelector( IBluetoothAdapterStatics *iface, HSTRING *result )
{
    static const WCHAR *default_res = L"System.Devices.InterfaceClassGuid:=\"{92383B0E-F90E-4AC9-8D44-8C2D0D0EBDA2}\" "
                                      L"AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";

    TRACE( "iface %p, result %p.\n", iface, result );

    if (!result) return E_POINTER;
    return WindowsCreateString( default_res, wcslen(default_res), result );
}

static HRESULT bluetoothadapter_get_adapter_async( IUnknown *invoker, IUnknown *params, PROPVARIANT *result, BOOL called_async );

static HRESULT WINAPI bluetoothadapter_statics_FromIdAsync( IBluetoothAdapterStatics *iface, HSTRING id, IAsyncOperation_BluetoothAdapter **operation )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Foundation_PropertyValue;
    IPropertyValueStatics *statics;
    IInspectable *id_propval;
    HSTRING_HEADER hdr;
    HSTRING str;
    HRESULT hr;

    TRACE( "iface %p, id %s, operation %p\n", iface, debugstr_hstring(id), operation );

    if (FAILED((hr = WindowsCreateStringReference( class_name, wcslen( class_name ), &hdr, &str )))) return hr;
    if (FAILED((hr = RoGetActivationFactory( str, &IID_IPropertyValueStatics, (void **)&statics )))) return hr;

    hr = IPropertyValueStatics_CreateString( statics, id, &id_propval );
    IPropertyValueStatics_Release( statics );
    if (FAILED(hr)) return hr;

    hr = async_operation_inspectable_create( &IID_IAsyncOperation_BluetoothAdapter,
                                             (IUnknown *)iface,
                                             (IUnknown *)id_propval,
                                             bluetoothadapter_get_adapter_async,
                                             (IAsyncOperation_IInspectable **)operation );
    IInspectable_Release( id_propval );
    return hr;
}

static HRESULT WINAPI bluetoothadapter_statics_GetDefaultAsync( IBluetoothAdapterStatics *iface, IAsyncOperation_BluetoothAdapter **operation )
{
    TRACE( "iface %p, operation %p\n", iface, operation );
    return async_operation_inspectable_create( &IID_IAsyncOperation_BluetoothAdapter,
                                               (IUnknown *)iface,
                                               NULL,
                                               bluetoothadapter_get_adapter_async,
                                               (IAsyncOperation_IInspectable **)operation );
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

static struct bluetoothadapter_statics bluetoothadapter_statics =
{
    {&factory_vtbl},
    {&bluetoothadapter_statics_vtbl},
    1,
};

IActivationFactory *bluetoothadapter_factory = &bluetoothadapter_statics.IActivationFactory_iface;

struct bluetoothadapter
{
    IBluetoothAdapter IBluetoothAdapter_iface;
    HSTRING id;
    HANDLE radio;
    LONG ref;
};

static inline struct bluetoothadapter *impl_from_IBluetoothAdapter( IBluetoothAdapter *iface )
{
    return CONTAINING_RECORD( iface, struct bluetoothadapter, IBluetoothAdapter_iface );
}

static WINAPI HRESULT bluetoothadapter_QueryInterface( IBluetoothAdapter *iface, REFIID iid, void **out )
{
    TRACE( "iface %p, iid %s, out %p\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IBluetoothAdapter ))
    {
        IBluetoothAdapter_AddRef(( *out = iface ));
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static WINAPI ULONG bluetoothadapter_AddRef( IBluetoothAdapter *iface )
{
    struct bluetoothadapter *impl = impl_from_IBluetoothAdapter( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static WINAPI ULONG bluetoothadapter_Release( IBluetoothAdapter *iface )
{
    struct bluetoothadapter *impl = impl_from_IBluetoothAdapter( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        WindowsDeleteString( impl->id );
        CloseHandle( impl->radio );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI bluetoothadapter_GetIids( IBluetoothAdapter *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_GetRuntimeClassName( IBluetoothAdapter *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_GetTrustLevel( IBluetoothAdapter *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_get_DeviceId( IBluetoothAdapter *iface, HSTRING *id )
{
    struct bluetoothadapter *impl = impl_from_IBluetoothAdapter( iface );
    TRACE( "iface %p, id %p\n", iface, id );
    return WindowsDuplicateString( impl->id, id );
}

static HRESULT WINAPI bluetoothadpter_get_BluetoothAddress( IBluetoothAdapter *iface, UINT64 *addr )
{
    struct bluetoothadapter *impl = impl_from_IBluetoothAdapter( iface );
    BLUETOOTH_RADIO_INFO info = {0};
    DWORD ret;

    TRACE( "iface %p, addr %p\n", iface, addr );

    info.dwSize = sizeof( info );
    ret = BluetoothGetRadioInfo( impl->radio, &info );
    if (!ret)
        *addr = info.address.ullLong;
    return HRESULT_FROM_WIN32( ret );
}

static HRESULT WINAPI bluetoothadapter_get_IsClassicSupported( IBluetoothAdapter *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_get_IsLowEnergySupported( IBluetoothAdapter *iface, boolean *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_get_IsPeripheralRoleSupported( IBluetoothAdapter *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_get_IsCentralRoleSupported( IBluetoothAdapter *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_get_IsAdvertisementOffloadSupported( IBluetoothAdapter *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI bluetoothadapter_GetRadioAsync( IBluetoothAdapter *iface, IAsyncOperation_Radio **async_op )
{
    FIXME("iface %p, async_op %p stub!\n", iface, async_op );
    return E_NOTIMPL;
}

static const IBluetoothAdapterVtbl bluetoothadapter_vtbl =
{
    /* IUnknown */
    bluetoothadapter_QueryInterface,
    bluetoothadapter_AddRef,
    bluetoothadapter_Release,
    /* IInspectable */
    bluetoothadapter_GetIids,
    bluetoothadapter_GetRuntimeClassName,
    bluetoothadapter_GetTrustLevel,
    /* IBluetoothAdapter */
    bluetoothadapter_get_DeviceId,
    bluetoothadpter_get_BluetoothAddress,
    bluetoothadapter_get_IsClassicSupported,
    bluetoothadapter_get_IsLowEnergySupported,
    bluetoothadapter_get_IsPeripheralRoleSupported,
    bluetoothadapter_get_IsCentralRoleSupported,
    bluetoothadapter_get_IsAdvertisementOffloadSupported,
    bluetoothadapter_GetRadioAsync,
};

static HRESULT create_bluetoothadapter( const WCHAR *path, IBluetoothAdapter **out )
{
    struct bluetoothadapter *impl;
    HRESULT hr;

    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;
    impl->IBluetoothAdapter_iface.lpVtbl = &bluetoothadapter_vtbl;
    impl->radio = CreateFileW( path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );
    if (impl->radio == INVALID_HANDLE_VALUE)
    {
        free( impl );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    if (FAILED((hr = WindowsCreateString( path, wcslen( path ), &impl->id ))))
    {
        CloseHandle( impl->radio );
        free( impl );
        return hr;
    }
    impl->ref = 1;
    *out = &impl->IBluetoothAdapter_iface;
    return S_OK;
}

static HRESULT bluetoothadapter_get_adapter_async( IUnknown *invoker, IUnknown *params, PROPVARIANT *result, BOOL called_async )
{
    char buffer[sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA_W ) + MAX_PATH * sizeof( WCHAR )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    SP_DEVICE_INTERFACE_DATA iface_data;
    const WCHAR *adapter_id = NULL;
    HSTRING adapter_id_hstr = NULL;
    UINT32 adapter_id_size = 0;
    HRESULT hr = S_OK;
    HDEVINFO devinfo;
    DWORD idx = 0;

    if (!called_async) return STATUS_PENDING;
    if (params)
    {
        IPropertyValue *id_propval;

        if (FAILED((hr = IUnknown_QueryInterface( params, &IID_IPropertyValue, (void **)&id_propval )))) return hr;
        hr = IPropertyValue_GetString( id_propval, &adapter_id_hstr );
        IPropertyValue_Release( id_propval );
        if (FAILED(hr)) return hr;
        adapter_id = WindowsGetStringRawBuffer( adapter_id_hstr, &adapter_id_size );
        adapter_id_size *= sizeof( WCHAR );
    }

    iface_detail->cbSize = sizeof( *iface_detail );
    iface_data.cbSize = sizeof( iface_data );

    /* Windows.Devices.Bluetooth uses the GUID_BLUETOOTH_RADIO_INTERFACE interface class guid for radio devices. */
    devinfo = SetupDiGetClassDevsW( &GUID_BLUETOOTH_RADIO_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    if (devinfo == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32( GetLastError() );

    while (SetupDiEnumDeviceInterfaces( devinfo, NULL, &GUID_BLUETOOTH_RADIO_INTERFACE, idx++, &iface_data ))
    {
        IBluetoothAdapter *adapter = NULL;
        DWORD path_size;

        if (!SetupDiGetDeviceInterfaceDetailW( devinfo, &iface_data, iface_detail, sizeof( buffer ), &path_size, NULL ))
            continue;

        if (adapter_id)
        {
            path_size -= (offsetof( SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath ) + sizeof( WCHAR ));
            if (adapter_id_size != path_size || wcsicmp( iface_detail->DevicePath, adapter_id )) continue;
        }
        if (SUCCEEDED((hr = create_bluetoothadapter( iface_detail->DevicePath, &adapter ))))
        {
            result->vt = VT_UNKNOWN;
            result->punkVal = (IUnknown *)adapter;
        }
        break;
    }

    WindowsDeleteString( adapter_id_hstr );
    SetupDiDestroyDeviceInfoList( devinfo );
    return hr;
}
