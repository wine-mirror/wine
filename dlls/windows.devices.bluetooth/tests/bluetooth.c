/*
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

#include "winerror.h"
#define COBJMACROS
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Networking
#include "windows.networking.connectivity.h"
#include "windows.networking.h"
#define WIDL_using_Windows_Devices_Bluetooth_Advertisement
#define WIDL_using_Windows_Devices_Bluetooth
#include "windows.devices.bluetooth.advertisement.h"
#include "windows.devices.bluetooth.rfcomm.h"
#include "windows.devices.bluetooth.h"

#include "wine/test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    IUnknown_Release( unk );
}

struct inspectable_async_handler
{
    IAsyncOperationCompletedHandler_IInspectable iface;
    const GUID *iid;
    IAsyncOperation_IInspectable *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
    LONG ref;
};

static inline struct inspectable_async_handler *impl_from_IAsyncOperationCompletedHandler_IInspectable( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct inspectable_async_handler, iface );
}

static HRESULT WINAPI inspectable_async_handler_QueryInterface( IAsyncOperationCompletedHandler_IInspectable *iface, REFIID iid, void **out )
{
    struct inspectable_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable( iface );
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject ) || IsEqualGUID( iid, impl->iid ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1)
        trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI inspectable_async_handler_AddRef( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    struct inspectable_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI inspectable_async_handler_Release( IAsyncOperationCompletedHandler_IInspectable *iface )
{
    struct inspectable_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI inspectable_async_handler_Invoke( IAsyncOperationCompletedHandler_IInspectable *iface, IAsyncOperation_IInspectable *async,
                                                        AsyncStatus status )
{
    struct inspectable_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_IInspectable( iface );
    ok( !impl->invoked, "invoked twice\n" );
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );
    return S_OK;
}

static const IAsyncOperationCompletedHandler_IInspectableVtbl inspectable_async_handler_vtbl =
{
    /* IUnknown */
    inspectable_async_handler_QueryInterface,
    inspectable_async_handler_AddRef,
    inspectable_async_handler_Release,
    /* IAsyncOperationCompletedHandler<IInspectable> */
    inspectable_async_handler_Invoke
};

static WINAPI IAsyncOperationCompletedHandler_IInspectable *inspectable_async_handler_create( HANDLE event, const GUID *iid )
{
    struct inspectable_async_handler *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;
    impl->iface.lpVtbl = &inspectable_async_handler_vtbl;
    impl->iid = iid;
    impl->event = event;
    impl->ref = 1;
    return &impl->iface;
}

struct inspectable_event_handler
{
    ITypedEventHandler_IInspectable_IInspectable iface;
    const GUID *iid;
    void (*callback)( IInspectable *, IInspectable *, void * );
    void *data;
    LONG ref;
};

static inline struct inspectable_event_handler *impl_from_ITypedEventHandler_IInspectable_IInspectable( ITypedEventHandler_IInspectable_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct inspectable_event_handler, iface );
}

static HRESULT WINAPI inspectable_event_handler_QueryInterface( ITypedEventHandler_IInspectable_IInspectable *iface, REFIID iid, void **out )
{
    struct inspectable_event_handler *impl = impl_from_ITypedEventHandler_IInspectable_IInspectable( iface );

    if (winetest_debug > 1) trace( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject) || IsEqualGUID( iid, impl->iid ))
    {
        ITypedEventHandler_IInspectable_IInspectable_AddRef((*out = &impl->iface.lpVtbl));
        return S_OK;
    }

    *out = NULL;
    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI inspectable_event_handler_AddRef( ITypedEventHandler_IInspectable_IInspectable *iface )
{
    struct inspectable_event_handler *impl = impl_from_ITypedEventHandler_IInspectable_IInspectable( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI inspectable_event_handler_Release( ITypedEventHandler_IInspectable_IInspectable *iface )
{
    struct inspectable_event_handler *impl = impl_from_ITypedEventHandler_IInspectable_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI inspectable_event_handler_Invoke( ITypedEventHandler_IInspectable_IInspectable *iface, IInspectable *arg1, IInspectable *arg2 )
{
    struct inspectable_event_handler *impl = impl_from_ITypedEventHandler_IInspectable_IInspectable( iface );

    if (winetest_debug > 1) trace( "(%p, %p, %p)\n", iface, arg1, arg2 );
    impl->callback( arg1, arg2, impl->data );
    return S_OK;
}

static const ITypedEventHandler_IInspectable_IInspectableVtbl inspectable_event_handler_vtbl = {
    /* IUnknown */
    inspectable_event_handler_QueryInterface,
    inspectable_event_handler_AddRef,
    inspectable_event_handler_Release,
    /* ITypedEventHandler<IInspectable *, IInspectable *> */
    inspectable_event_handler_Invoke
};

static ITypedEventHandler_IInspectable_IInspectable *inspectable_event_handler_create( REFIID iid, void (*callback)( IInspectable *, IInspectable *, void * ),
                                                                                       void *data )
{
    struct inspectable_event_handler *handler;

    if (!(handler = calloc( 1, sizeof( *handler )))) return NULL;
    handler->iface.lpVtbl = &inspectable_event_handler_vtbl;
    handler->iid = iid;
    handler->callback = callback;
    handler->data = data;
    handler->ref = 1;
    return &handler->iface;
}

static void await_bluetoothledevice( int line, IAsyncOperation_BluetoothLEDevice *async )
{
    IAsyncOperationCompletedHandler_IInspectable *handler;
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(__FILE__, line)( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    handler = inspectable_async_handler_create( event, &IID_IAsyncOperationCompletedHandler_BluetoothLEDevice );
    ok_( __FILE__, line )( !!handler, "inspectable_async_handler_create failed\n" );
    hr = IAsyncOperation_BluetoothLEDevice_put_Completed( async, (IAsyncOperationCompletedHandler_BluetoothLEDevice *)handler );
    ok_(__FILE__, line)( hr == S_OK, "put_Completed returned %#lx\n", hr );
    IAsyncOperationCompletedHandler_IInspectable_Release( handler );

    ret = WaitForSingleObject( event, 5000 );
    ok_(__FILE__, line)( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( event );
    ok_(__FILE__, line)( ret, "CloseHandle failed, error %lu\n", GetLastError() );
}

static void check_bluetoothledevice_async( int line, IAsyncOperation_BluetoothLEDevice *async,
                                           UINT32 expect_id, AsyncStatus expect_status,
                                           HRESULT expect_hr, IBluetoothLEDevice **result )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_BluetoothLEDevice_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( async_id == expect_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) todo_wine_if( FAILED(expect_hr))
    ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );

    hr = IAsyncOperation_BluetoothLEDevice_GetResults( async, result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if( FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr, "GetResults returned %#lx\n", hr );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

static void test_BluetoothAdapterStatics(void)
{
    static const WCHAR *default_res = L"System.Devices.InterfaceClassGuid:=\"{92383B0E-F90E-4AC9-8D44-8C2D0D0EBDA2}\" "
                                      L"AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";
    static const WCHAR *bluetoothadapter_statics_name = L"Windows.Devices.Bluetooth.BluetoothAdapter";
    IBluetoothAdapterStatics *bluetoothadapter_statics;
    IActivationFactory *factory;
    HSTRING str, default_str;
    HRESULT hr;
    INT32 res;
    LONG ref;

    hr = WindowsCreateString( bluetoothadapter_statics_name, wcslen( bluetoothadapter_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( bluetoothadapter_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IBluetoothAdapterStatics, (void **)&bluetoothadapter_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IBluetoothAdapterStatics_GetDeviceSelector( bluetoothadapter_statics, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IBluetoothAdapterStatics_GetDeviceSelector( bluetoothadapter_statics, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( default_res, wcslen(default_res), &default_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, default_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got unexpected string %s.\n", debugstr_hstring(str) );

    WindowsDeleteString( str );
    WindowsDeleteString( default_str );
    ref = IBluetoothAdapterStatics_Release( bluetoothadapter_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_BluetoothDeviceStatics( void )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Devices_Bluetooth_BluetoothDevice;
    IBluetoothDeviceStatics *statics;
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void *)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr != S_OK)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }
    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IBluetoothDeviceStatics, (void **)&statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IActivationFactory_Release( factory );
    if (FAILED( hr ))
    {
        skip( "BluetoothDeviceStatics not available.\n" );
        return;
    }

    str = NULL;
    hr = IBluetoothDeviceStatics_GetDeviceSelector( statics, &str );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !WindowsIsStringEmpty( str ), "got empty device selector string.\n" );
    WindowsDeleteString( str );

    IBluetoothDeviceStatics_Release( statics );
}

static void test_BluetoothLEDeviceStatics( void )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Devices_Bluetooth_BluetoothLEDevice;
    IAsyncOperation_BluetoothLEDevice *async_op;
    IBluetoothLEDeviceStatics *statics;
    IActivationFactory *factory;
    IBluetoothLEDevice *device = NULL;
    HSTRING str;
    HRESULT hr;

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void *)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }
    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IBluetoothLEDeviceStatics, (void **)&statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IActivationFactory_Release( factory );

    str = NULL;
    hr = IBluetoothLEDeviceStatics_GetDeviceSelector( statics, &str );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !WindowsIsStringEmpty( str ), "got empty device selector string.\n" );
    WindowsDeleteString( str );

    /* Use an invalid Bluetooth address. */
    hr = IBluetoothLEDeviceStatics_FromBluetoothAddressAsync( statics, 0, &async_op );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );

    if (hr == S_OK)
    {
        await_bluetoothledevice( __LINE__, async_op );
        check_bluetoothledevice_async( __LINE__, async_op, 1, Completed, S_OK, &device );
        ok( !device, "got device %p != NULL\n", device );
        if (device) IBluetoothLEDevice_Release( device );
    }

    IBluetoothLEDeviceStatics_Release( statics );
}

/* See https://www.bluetooth.com/specifications/assigned-numbers, 2.3, Common Data Types */
#define BLE_AD_TYPE_FLAGS            0x01
#define BLE_AD_TYPE_SHORT_LOCAL_NAME 0x08
#define BLE_AD_TYPE_FULL_LOCAL_NAME  0x09
#define BLE_AD_TYPE_MFG_SPECIFIC     0xFF

#define test_advertisement_has_data_type( a, t ) test_advertisement_has_data_type_( __LINE__, (a), (t) )
static BOOL test_advertisement_has_data_type_( int line, IBluetoothLEAdvertisement *adv, BYTE type )
{
    IVectorView_BluetoothLEAdvertisementDataSection *section = NULL;
    HRESULT hr;

    hr = IBluetoothLEAdvertisement_GetSectionsByType( adv, type, &section );
    todo_wine ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok_(__FILE__, line)( !!section, "got section %p.\n", section );
    if (SUCCEEDED( hr ))
    {
        UINT32 len = 0;

        hr = IVectorView_BluetoothLEAdvertisementDataSection_get_Size( section, &len );
        IVectorView_BluetoothLEAdvertisementDataSection_Release( section );
        ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
        return !!len;
    }

    return FALSE;
}

static const char *debugstr_bluetooth_addr( INT64 addr )
{
    const BYTE *bytes = (BYTE *)&addr;
    return wine_dbg_sprintf( "%02X:%02X:%02X:%02X:%02X:%02X", bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0] );
}

struct adv_watcher_received_data
{
    LONG *stopped;
    DWORD received;
};

static void test_watcher_advertisement_received( IInspectable *arg1, IInspectable *arg2, void *user_data )
{
    IVector_BluetoothLEAdvertisementDataSection *data_sections = NULL;
    IReference_BluetoothLEAdvertisementFlags *flags_ref = NULL;
    struct adv_watcher_received_data *data = user_data;
    IBluetoothLEAdvertisementReceivedEventArgs *event;
    IVector_BluetoothLEManufacturerData *mfgs = NULL;
    BluetoothLEAdvertisementType adv_type = 6;
    IBluetoothLEAdvertisement *adv = NULL;
    IVector_GUID *uuids = NULL;
    FILETIME timestamp = {0};
    UINT32 len, mfgs_len = 0;
    SYSTEMTIME st = {0};
    HSTRING name = NULL;
    INT16 signal = 0;
    UINT64 addr = 0;
    WCHAR buf[256];
    BOOL success;
    HRESULT hr;

    ok( !ReadNoFence( data->stopped ), "handler called after watcher was stopped.\n" );

    data->received += 1;
    winetest_push_context( "advertisement %lu", data->received );
    check_interface( arg1, &IID_IBluetoothLEAdvertisementWatcher );

    hr = IInspectable_QueryInterface( arg2, &IID_IBluetoothLEAdvertisementReceivedEventArgs, (void **)&event );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !!event, "got event %p.\n", event );

    hr = IBluetoothLEAdvertisementReceivedEventArgs_get_RawSignalStrengthInDBm( event, &signal );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (winetest_debug > 1)
        trace( "raw signal strength: %d dBm.\n", signal );

    hr = IBluetoothLEAdvertisementReceivedEventArgs_get_BluetoothAddress( event, &addr );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( addr, "got addr %012I64x\n", addr );
    if (winetest_debug > 1)
        trace( "address: %I64X (%s)\n", addr, debugstr_bluetooth_addr( addr ) );

    hr = IBluetoothLEAdvertisementReceivedEventArgs_get_AdvertisementType( event, &adv_type );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( adv_type <= BluetoothLEAdvertisementType_Extended, "got adv_type %d\n", adv_type );
    if (winetest_debug > 1)
        trace( "advertisement type: %d\n", adv_type );

    hr = IBluetoothLEAdvertisementReceivedEventArgs_get_Timestamp( event, (DateTime *)&timestamp );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( ((DateTime *)&timestamp)->UniversalTime, "got timestamp %I64u.\n", ((DateTime *)&timestamp)->UniversalTime );
    success = FileTimeToSystemTime( &timestamp, &st );
    ok( success, "FileTimeToSystemTime failed: %#lx.\n", GetLastError() );
    success = SystemTimeToTzSpecificLocalTime( NULL, &st, &st );
    ok( success, "SystemTimeToTzSpecificLocalTime failed: %#lx.\n", GetLastError() );
    buf[0] = 0;
    GetTimeFormatEx( NULL, TIME_FORCE24HOURFORMAT, &st, NULL, buf, ARRAY_SIZE( buf ) );
    if (winetest_debug > 1)
        trace("timestamp: %s\n", debugstr_w( buf ));

    hr = IBluetoothLEAdvertisementReceivedEventArgs_get_Advertisement( event, &adv );
    IBluetoothLEAdvertisementReceivedEventArgs_Release( event );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (FAILED( hr ))
    {
        winetest_pop_context();
        return;
    }

    flags_ref = NULL;
    hr = IBluetoothLEAdvertisement_get_Flags( adv, &flags_ref );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (flags_ref)
    {
        BluetoothLEAdvertisementFlags flags = 0;

        hr = IReference_BluetoothLEAdvertisementFlags_get_Value( flags_ref, &flags );
        IReference_BluetoothLEAdvertisementFlags_Release( flags_ref );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        if (winetest_debug > 1)
            trace( "flags: %#x\n", flags );
        if (flags)
            todo_wine ok( test_advertisement_has_data_type( adv, BLE_AD_TYPE_FLAGS ), "missing data section for type %#x\n", BLE_AD_TYPE_FLAGS );
    }

    hr = IBluetoothLEAdvertisement_get_LocalName( adv, &name );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (winetest_debug > 1)
        trace( "name: %s\n", debugstr_hstring( name ) );
    if (!WindowsIsStringEmpty( name ))
        todo_wine ok( test_advertisement_has_data_type( adv, BLE_AD_TYPE_SHORT_LOCAL_NAME ) ||
                      test_advertisement_has_data_type( adv, BLE_AD_TYPE_FULL_LOCAL_NAME ),
                      "missing data sections for type %#x and %#x\n", BLE_AD_TYPE_SHORT_LOCAL_NAME, BLE_AD_TYPE_FULL_LOCAL_NAME );

    hr = IBluetoothLEAdvertisement_get_ServiceUuids( adv, &uuids );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !!uuids, "got uuids %p.\n", uuids );
    if (SUCCEEDED( hr ))
    {
        UINT32 copied = 0, i;
        GUID *buf;

        len = 0;
        hr = IVector_GUID_get_Size( uuids, &len );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        buf = calloc( len, sizeof( *buf ) );
        hr = IVector_GUID_GetMany( uuids, 0, len, buf, &copied );
        IVector_GUID_Release( uuids );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( copied == len, "got copied %u\n", copied );

        if (winetest_debug > 1)
            for (i = 0; i < copied; i++)
                trace( "uuids[%u]: %s\n", i, debugstr_guid( &buf[i] ) );
        free( buf );
    }

    hr = IBluetoothLEAdvertisement_get_ManufacturerData( adv, &mfgs );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !!mfgs, "got mfgs %p.\n", mfgs );
    if (SUCCEEDED( hr ))
    {
        hr = IVector_BluetoothLEManufacturerData_get_Size( mfgs, &mfgs_len );
        IVector_BluetoothLEManufacturerData_Release( mfgs );
        todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
        if (winetest_debug > 1)
            trace( "manufacturer data sections: %u\n", mfgs_len );
        if (mfgs_len)
            todo_wine ok( test_advertisement_has_data_type( adv, BLE_AD_TYPE_MFG_SPECIFIC ), "missing data section for type %#x\n", BLE_AD_TYPE_MFG_SPECIFIC );
    }

    hr = IBluetoothLEAdvertisement_get_DataSections( adv, &data_sections );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !!data_sections, "got data_sections %p.\n", data_sections );
    if (SUCCEEDED( hr ))
    {
        UINT32 exp_len = mfgs_len;

        if (!WindowsIsStringEmpty( name ))
            exp_len++;
        len = 0;
        /* All the above properties are derived from the raw data sections, so we can determine how many data sections we can at least expect. */
        hr = IVector_BluetoothLEAdvertisementDataSection_get_Size( data_sections, &len );
        IVector_BluetoothLEAdvertisementDataSection_Release( data_sections );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( len >= exp_len, "got len %u, should be >= %u\n", len, exp_len );
    }
    winetest_pop_context();
    WindowsDeleteString( name );
    IBluetoothLEAdvertisement_Release( adv );
}

struct adv_watcher_stopped_data
{
    HANDLE event;
    LONG called;
    BOOLEAN no_radio;
};

static void test_watcher_stopped( IInspectable *arg1, IInspectable *arg2, void *param )
{
    IBluetoothLEAdvertisementWatcherStoppedEventArgs *event;
    BluetoothError error = BluetoothError_OtherError;
    struct adv_watcher_stopped_data *data = param;
    HRESULT hr;

    check_interface( arg1, &IID_IBluetoothLEAdvertisementWatcher );

    hr = IInspectable_QueryInterface( arg2, &IID_IBluetoothLEAdvertisementWatcherStoppedEventArgs, (void **)&event );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IBluetoothLEAdvertisementWatcherStoppedEventArgs_get_Error( event, &error );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( error == BluetoothError_Success || error == BluetoothError_RadioNotAvailable, "got error %d.\n", error );

    if (error == BluetoothError_RadioNotAvailable)
        data->no_radio = TRUE;

    ok( InterlockedIncrement( &data->called ) == 1, "handler called more than once.\n" );
    SetEvent( data->event );
    IBluetoothLEAdvertisementWatcherStoppedEventArgs_Release( event );
}

static void test_BluetoothLEAdvertisementWatcher( void )
{
    const WCHAR *class_name = RuntimeClass_Windows_Devices_Bluetooth_Advertisement_BluetoothLEAdvertisementWatcher;
    ITypedEventHandler_IInspectable_IInspectable *received_handler, *stopped_handler;
    EventRegistrationToken received_token = {0}, stopped_token = {0};
    struct adv_watcher_received_data received_data = {0};
    struct adv_watcher_stopped_data stopped_data = {0};
    IBluetoothSignalStrengthFilter *sig_filter = NULL;
    BluetoothLEAdvertisementWatcherStatus status;
    IBluetoothLEAdvertisementWatcher *watcher;
    IReference_TimeSpan *ref_span;
    BluetoothLEScanningMode mode;
    IInspectable *inspectable;
    IReference_INT16 *int16;
    TimeSpan span;
    HSTRING str;
    HRESULT hr;
    DWORD ret;

    WindowsCreateString( class_name, wcslen( class_name ), &str );
    hr = RoActivateInstance( str, &inspectable );
    WindowsDeleteString( str );
    todo_wine ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG || hr == CLASS_E_CLASSNOTAVAILABLE)
    {
        todo_wine win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    check_interface( inspectable, &IID_IUnknown );
    check_interface( inspectable, &IID_IInspectable );
    check_interface( inspectable, &IID_IAgileObject );

    hr = IInspectable_QueryInterface( inspectable, &IID_IBluetoothLEAdvertisementWatcher, (void **)&watcher );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IInspectable_Release( inspectable );

    span.Duration = 0;
    hr = IBluetoothLEAdvertisementWatcher_get_MinSamplingInterval( watcher, &span );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( span.Duration == 1000000, "got Duration %I64d.\n", span.Duration );

    span.Duration = 0;
    hr = IBluetoothLEAdvertisementWatcher_get_MaxSamplingInterval( watcher, &span );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( span.Duration == 255000000, "got Duration %I64d.\n", span.Duration );

    span.Duration = 0;
    hr = IBluetoothLEAdvertisementWatcher_get_MinOutOfRangeTimeout( watcher, &span );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( span.Duration == 10000000, "got Duration %I64d.\n", span.Duration );

    span.Duration = 0;
    hr = IBluetoothLEAdvertisementWatcher_get_MaxOutOfRangeTimeout( watcher, &span );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( span.Duration == 600000000, "got Duration %I64d.\n", span.Duration );

    status = BluetoothLEAdvertisementWatcherStatus_Aborted;
    hr = IBluetoothLEAdvertisementWatcher_get_Status( watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( status == BluetoothLEAdvertisementWatcherStatus_Created, "got status %u.\n", status );

    mode = BluetoothLEScanningMode_None;
    hr = IBluetoothLEAdvertisementWatcher_get_ScanningMode( watcher, &mode );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( mode == BluetoothLEScanningMode_Passive, "got status %u.\n", status );

    hr = IBluetoothLEAdvertisementWatcher_get_SignalStrengthFilter( watcher, &sig_filter );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( !!sig_filter, "got sig_filter %p.\n", sig_filter );
    if (SUCCEEDED( hr ))
    {
        check_interface( sig_filter, &IID_IAgileObject );

        int16 = (IReference_INT16 *)0xdeadbeef;
        hr = IBluetoothSignalStrengthFilter_get_InRangeThresholdInDBm( sig_filter, &int16 );
        todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
        todo_wine ok( !int16, "got int16 %p.\n", int16 );

        int16 = (IReference_INT16 *)0xdeadbeef;
        hr = IBluetoothSignalStrengthFilter_get_OutOfRangeThresholdInDBm( sig_filter, &int16 );
        todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
        todo_wine ok( !int16, "got int16 %p.\n", int16 );

        ref_span = (IReference_TimeSpan *)0xdeadbeef;
        hr = IBluetoothSignalStrengthFilter_get_OutOfRangeTimeout( sig_filter, &ref_span );
        todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
        todo_wine ok( !ref_span, "got span %p.\n", ref_span );

        ref_span = (IReference_TimeSpan *)0xdeadbeef;
        hr = IBluetoothSignalStrengthFilter_get_SamplingInterval( sig_filter, &ref_span );
        todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
        todo_wine ok( !ref_span, "got span %p.\n", ref_span );

        IBluetoothSignalStrengthFilter_Release( sig_filter );
    }

    received_data.stopped = &stopped_data.called;
    received_handler = inspectable_event_handler_create( &IID_ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementReceivedEventArgs,
                                                         test_watcher_advertisement_received, &received_data );
    ok( !!received_handler, "inspectable_event_handler_create failed.\n" );
    hr = IBluetoothLEAdvertisementWatcher_add_Received( watcher,
                                                        (ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementReceivedEventArgs *)received_handler,
                                                        &received_token );
    ITypedEventHandler_IInspectable_IInspectable_Release( received_handler );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );

    stopped_data.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stopped_data.event, "CreateEventW failed: %lu\n", GetLastError() );
    stopped_handler = inspectable_event_handler_create( &IID_ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementWatcherStoppedEventArgs,
                                                        test_watcher_stopped, &stopped_data );
    ok( !!stopped_handler, "inspectable_event_handler_create failed.\n" );
    hr = IBluetoothLEAdvertisementWatcher_add_Stopped( watcher,
                                                       (ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementWatcherStoppedEventArgs *)stopped_handler,
                                                       &stopped_token );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    ITypedEventHandler_IInspectable_IInspectable_Release( stopped_handler );

    hr = IBluetoothLEAdvertisementWatcher_Start( watcher );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );

    /* The watcher stops with the error BluetoothError_RadioNotAvailable if no BLE radios are available. */
    ret = WaitForSingleObject( stopped_data.event, 100);
    if (!ret)
    {
        skip( "No Bluetooth radios, skipping.\n" );
        ok( stopped_data.no_radio, "got no_radio %d.\n", stopped_data.no_radio );
        CloseHandle( stopped_data.event );
        IBluetoothLEAdvertisementWatcher_Release( watcher );
        return;
    }

    /* Calling Start multiple times seems to not result in any errors.  */
    hr = IBluetoothLEAdvertisementWatcher_Start( watcher );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );

    /* Usually there are plenty of BLE devices advertising themselves at any given point in most places, but waiting for an extended period of time
     * allows testing the watcher with custom advertisements as well. */
    if (winetest_interactive)
        SleepEx( 10000, FALSE );

    hr = IBluetoothLEAdvertisementWatcher_Stop( watcher );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    ret = WaitForSingleObject( stopped_data.event, 500 );
    todo_wine ok( !ret, "got ret %lu.\n", ret );
    /* The watcher should not have stopped with BluetoothError_RadioNotAvailable. */
    ok( !stopped_data.no_radio, "got no_radio %d.\n", stopped_data.no_radio );

    /* Calling Stop for a stopped watcher also does not result in any errors, but also does not dispatch any more Stopped events. */
    hr = IBluetoothLEAdvertisementWatcher_Stop( watcher );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    ret = WaitForSingleObject( stopped_data.event, 100 );
    ok( ret == WAIT_TIMEOUT, "got ret %lu.\n", ret );

    hr = IBluetoothLEAdvertisementWatcher_remove_Stopped( watcher, stopped_token );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IBluetoothLEAdvertisementWatcher_remove_Received( watcher, received_token );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );

    trace( "Received %lu advertisement packets.\n", received_data.received );

    CloseHandle( stopped_data.event );
    IBluetoothLEAdvertisementWatcher_Release( watcher );
}

START_TEST(bluetooth)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_BluetoothAdapterStatics();
    test_BluetoothDeviceStatics();
    test_BluetoothLEDeviceStatics();
    test_BluetoothLEAdvertisementWatcher();

    RoUninitialize();
}
