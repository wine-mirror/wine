/*
 * Copyright 2022 Julian Klemann for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Enumeration
#include "windows.devices.enumeration.h"

#include "wine/test.h"

#define IDeviceInformationStatics2_CreateWatcher IDeviceInformationStatics2_CreateWatcherWithKindAqsFilterAndAdditionalProperties
#define check_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp, FALSE )
#define check_optional_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp, TRUE )
static void check_interface_(unsigned int line, void *obj, const IID *iid, BOOL supported, BOOL optional)
{
    IUnknown *iface = obj;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr || broken(hr == E_NOINTERFACE && optional), "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

struct device_watcher_handler
{
    ITypedEventHandler_DeviceWatcher_IInspectable ITypedEventHandler_DeviceWatcher_IInspectable_iface;
    LONG ref;

    unsigned int test_deviceinformation : 1;
    LONG devices_added;
    HANDLE event;
    BOOL invoked;
    IInspectable *args;
};

static inline struct device_watcher_handler *impl_from_ITypedEventHandler_DeviceWatcher_IInspectable(
        ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct device_watcher_handler, ITypedEventHandler_DeviceWatcher_IInspectable_iface );
}

static HRESULT WINAPI device_watcher_handler_QueryInterface(
        ITypedEventHandler_DeviceWatcher_IInspectable *iface, REFIID iid, void **out )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_ITypedEventHandler_DeviceWatcher_IInspectable ) ||
        (impl->test_deviceinformation && IsEqualGUID( iid, &IID_ITypedEventHandler_DeviceWatcher_DeviceInformation )))
    {
        IUnknown_AddRef( &impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface );
        *out = &impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface;
        return S_OK;
    }

    trace( "%s not implemented, returning E_NO_INTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_watcher_handler_AddRef( ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    return ref;
}

static ULONG WINAPI device_watcher_handler_Release( ITypedEventHandler_DeviceWatcher_IInspectable *iface )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    return ref;
}

static void test_DeviceInformation_obj( int line, IDeviceInformation *info );
static HRESULT WINAPI device_watcher_handler_Invoke( ITypedEventHandler_DeviceWatcher_IInspectable *iface,
                                                     IDeviceWatcher *sender, IInspectable *args )
{
    struct device_watcher_handler *impl = impl_from_ITypedEventHandler_DeviceWatcher_IInspectable( iface );

    impl->invoked = TRUE;
    impl->args = args;

    if (impl->test_deviceinformation)
    {
        IDeviceInformation *info;
        HRESULT hr;

        hr = IInspectable_QueryInterface( args, &IID_IDeviceInformation, (void *)&info );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        test_DeviceInformation_obj( __LINE__, info );
        InterlockedIncrement( &impl->devices_added );
        IDeviceInformation_Release( info );
    }

    SetEvent( impl->event );

    return S_OK;
}

static const ITypedEventHandler_DeviceWatcher_IInspectableVtbl device_watcher_handler_vtbl =
{
    device_watcher_handler_QueryInterface,
    device_watcher_handler_AddRef,
    device_watcher_handler_Release,
    /* ITypedEventHandler<DeviceWatcher*,IInspectable*> methods */
    device_watcher_handler_Invoke,
};

static void device_watcher_handler_create( struct device_watcher_handler *impl )
{
    impl->ITypedEventHandler_DeviceWatcher_IInspectable_iface.lpVtbl = &device_watcher_handler_vtbl;
    impl->invoked = FALSE;
    impl->ref = 1;
}

struct device_information_collection_async_handler
{
    IAsyncOperationCompletedHandler_DeviceInformationCollection iface;

    IAsyncOperation_DeviceInformationCollection *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
    LONG ref;
};

static inline struct device_information_collection_async_handler *impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    return CONTAINING_RECORD( iface, struct device_information_collection_async_handler, iface );
}

static HRESULT WINAPI device_information_collection_async_handler_QueryInterface( IAsyncOperationCompletedHandler_DeviceInformationCollection *iface,
                                                                                  REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_DeviceInformationCollection ))
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

static ULONG WINAPI device_information_collection_async_handler_AddRef( IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    struct device_information_collection_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI device_information_collection_async_handler_Release( IAsyncOperationCompletedHandler_DeviceInformationCollection *iface )
{
    struct device_information_collection_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );
    ULONG ref;

    ref = InterlockedDecrement( &impl->ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI device_information_collection_async_handler_Invoke( IAsyncOperationCompletedHandler_DeviceInformationCollection *iface,
                                                                          IAsyncOperation_DeviceInformationCollection *async, AsyncStatus status )
{
    struct device_information_collection_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_DeviceInformationCollection( iface );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_DeviceInformationCollectionVtbl device_information_collection_async_handler_vtbl =
{
    /* IUnknown */
    device_information_collection_async_handler_QueryInterface,
    device_information_collection_async_handler_AddRef,
    device_information_collection_async_handler_Release,
    /* IAsyncOperationCompletedHandler<DeviceInformationCollection> */
    device_information_collection_async_handler_Invoke,
};

static IAsyncOperationCompletedHandler_DeviceInformationCollection *device_information_collection_async_handler_create( HANDLE event )
{
    struct device_information_collection_async_handler *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;
    impl->iface.lpVtbl = &device_information_collection_async_handler_vtbl;
    impl->event = event;
    impl->ref = 1;

    return &impl->iface;
}

#define await_device_information_collection( a ) await_device_information_collection_( __LINE__, (a) )
static void await_device_information_collection_( int line, IAsyncOperation_DeviceInformationCollection *async )
{
    IAsyncOperationCompletedHandler_DeviceInformationCollection *handler;
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(__FILE__, line)( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    handler = device_information_collection_async_handler_create( event );
    ok_(__FILE__, line)( !!handler, "device_information_collection_async_handler_create failed\n" );
    hr = IAsyncOperation_DeviceInformationCollection_put_Completed( async, handler );
    ok_(__FILE__, line)( hr == S_OK, "put_Completed returned %#lx\n", hr );
    IAsyncOperationCompletedHandler_DeviceInformationCollection_Release( handler );

    ret = WaitForSingleObject( event, 5000 );
    ok_(__FILE__, line)( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( event );
    ok_(__FILE__, line)( ret, "CloseHandle failed, error %lu\n", GetLastError() );
}

#define check_device_information_collection_async( a, b, c, d, e ) check_device_information_collection_async_( __LINE__, a, b, c, d, e )
static void check_device_information_collection_async_( int line, IAsyncOperation_DeviceInformationCollection *async,
                                                        UINT32 expect_id, AsyncStatus expect_status,
                                                        HRESULT expect_hr, IVectorView_DeviceInformation **result )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_DeviceInformationCollection_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
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

    hr = IAsyncOperation_DeviceInformationCollection_GetResults( async, result );
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

static void test_DeviceInformation_obj( int line, IDeviceInformation *info )
{
    HRESULT hr;
    HSTRING str;
    boolean bool_val;

    hr = IDeviceInformation_get_Id( info, &str );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx\n", hr );
    WindowsDeleteString( str );
    str = NULL;
    hr = IDeviceInformation_get_Name( info, &str );
    todo_wine ok_(__FILE__, line)( hr == S_OK, "got hr %#lx\n", hr );
    WindowsDeleteString( str );
    hr = IDeviceInformation_get_IsEnabled( info, &bool_val );
    todo_wine ok_(__FILE__, line)( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceInformation_get_IsDefault( info, &bool_val );
    todo_wine ok_(__FILE__, line)( hr == S_OK, "got hr %#lx\n", hr );
}

static void test_DeviceInformation( void )
{
    static const WCHAR *device_info_name = L"Windows.Devices.Enumeration.DeviceInformation";

    static struct device_watcher_handler stopped_handler, added_handler, enumerated_handler;
    EventRegistrationToken stopped_token, added_token, enumerated_token;
    IInspectable *inspectable, *inspectable2;
    IActivationFactory *factory;
    IDeviceInformationStatics2 *device_info_statics2;
    IDeviceInformationStatics *device_info_statics;
    IDeviceWatcher *device_watcher;
    DeviceWatcherStatus status = 0xdeadbeef;
    IAsyncOperation_DeviceInformationCollection *info_collection_async = NULL;
    IVectorView_DeviceInformation *info_collection = NULL;
    IDeviceInformation *info;
    UINT32 i, size;
    HSTRING str;
    HRESULT hr;
    ULONG ref;

    device_watcher_handler_create( &added_handler );
    device_watcher_handler_create( &stopped_handler );
    device_watcher_handler_create( &enumerated_handler );

    stopped_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stopped_handler.event, "failed to create event, got error %lu\n", GetLastError() );
    enumerated_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!enumerated_handler.event, "failed to create event, got error %lu\n", GetLastError() );

    hr = WindowsCreateString( device_info_name, wcslen( device_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    if ( hr == REGDB_E_CLASSNOTREG )
    {
        win_skip( "%s runtimeclass, not registered.\n", wine_dbgstr_w( device_info_name ) );
        goto done;
    }

    hr = IActivationFactory_QueryInterface( factory, &IID_IInspectable, (void **)&inspectable );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    check_interface( factory, &IID_IAgileObject, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceInformationStatics2, (void **)&device_info_statics2 );
    ok( hr == S_OK || broken( hr == E_NOINTERFACE ), "got hr %#lx\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "IDeviceInformationStatics2 not supported.\n" );
        goto skip_device_statics;
    }

    hr = IDeviceInformationStatics2_QueryInterface( device_info_statics2, &IID_IInspectable, (void **)&inspectable2 );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( inspectable == inspectable2, "got inspectable %p, inspectable2 %p\n", inspectable, inspectable2 );

    hr = IDeviceInformationStatics2_CreateWatcher( device_info_statics2, NULL, NULL, DeviceInformationKind_AssociationEndpoint, &device_watcher );
    check_interface( device_watcher, &IID_IUnknown, TRUE );
    check_interface( device_watcher, &IID_IInspectable, TRUE );
    check_interface( device_watcher, &IID_IAgileObject, TRUE );
    check_interface( device_watcher, &IID_IDeviceWatcher, TRUE );

    hr = IDeviceWatcher_add_Added( device_watcher, (void *)&added_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped( device_watcher, &stopped_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_handler.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_handler.invoked, "stopped_handler not invoked\n" );
    ok( stopped_handler.args == NULL, "stopped_handler not invoked\n" );

    IDeviceWatcher_Release( device_watcher );
    IInspectable_Release( inspectable2 );
    IDeviceInformationStatics2_Release( device_info_statics2 );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceInformationStatics, (void **)&device_info_statics );
    ok( hr == S_OK || broken( hr == E_NOINTERFACE ), "got hr %#lx\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "IDeviceInformationStatics not supported.\n" );
        goto skip_device_statics;
    }

    hr = IDeviceInformationStatics_CreateWatcherAqsFilter( device_info_statics, NULL, &device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    check_interface( device_watcher, &IID_IUnknown, TRUE );
    check_interface( device_watcher, &IID_IInspectable, TRUE );
    check_interface( device_watcher, &IID_IAgileObject, TRUE );
    check_interface( device_watcher, &IID_IDeviceWatcher, TRUE );

    hr = IDeviceWatcher_add_Added( device_watcher, (void *)&added_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped( device_watcher, &stopped_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_handler.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    todo_wine ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_handler.invoked, "stopped_handler not invoked\n" );
    ok( stopped_handler.args == NULL, "stopped_handler not invoked\n" );

    IDeviceWatcher_Release( device_watcher );

    hr = IDeviceInformationStatics_CreateWatcher( device_info_statics, &device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    if (device_watcher)
    {
        added_handler.test_deviceinformation = 1;
        hr = IDeviceWatcher_add_Added( device_watcher, (void *)&added_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &added_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_add_EnumerationCompleted( device_watcher, (void *)&enumerated_handler.ITypedEventHandler_DeviceWatcher_IInspectable_iface, &enumerated_token );
        todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );

        hr = IDeviceWatcher_Start( device_watcher );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        todo_wine ok( !WaitForSingleObject( enumerated_handler.event, 5000 ), "wait for enumerated_handler.event failed\n" );
        todo_wine ok( added_handler.devices_added > 0, "devices_added should be greater than 0\n" );
        hr = IDeviceWatcher_get_Status( device_watcher, &status );
        todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
        todo_wine ok( status == DeviceWatcherStatus_EnumerationCompleted, "got status %u\n", status );

        hr = IDeviceWatcher_Start( device_watcher );
        todo_wine ok( hr == E_ILLEGAL_METHOD_CALL, "Start returned %#lx\n", hr );

        IDeviceWatcher_Release( device_watcher );
    }

    hr = IDeviceInformationStatics_FindAllAsync( device_info_statics, &info_collection_async );
    ok( hr == S_OK, "got %#lx\n", hr );

    await_device_information_collection( info_collection_async );
    check_device_information_collection_async( info_collection_async, 1, Completed, S_OK, &info_collection );
    IAsyncOperation_DeviceInformationCollection_Release( info_collection_async );

    hr = IVectorView_DeviceInformation_get_Size( info_collection, &size );
    ok( hr == S_OK, "got %#lx\n", hr );
    for (i = 0; i < size; i++)
    {
        winetest_push_context( "info_collection %u", i );
        hr = IVectorView_DeviceInformation_GetAt( info_collection, i, &info );
        ok( hr == S_OK, "got %#lx\n", hr );
        test_DeviceInformation_obj( __LINE__, info );
        IDeviceInformation_Release( info );
        winetest_pop_context();
    }
    IVectorView_DeviceInformation_Release( info_collection );

    IDeviceInformationStatics_Release( device_info_statics );

skip_device_statics:
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %lu\n", ref );

done:
    WindowsDeleteString( str );
    CloseHandle( stopped_handler.event );
    CloseHandle( enumerated_handler.event );
}

static void test_DeviceAccessInformation( void )
{
    static const WCHAR *device_access_info_name = L"Windows.Devices.Enumeration.DeviceAccessInformation";
    static const WCHAR *device_info_name = L"Windows.Devices.Enumeration.DeviceInformation";
    IDeviceAccessInformationStatics *statics;
    IActivationFactory *factory, *factory2;
    IDeviceAccessInformation *access_info;
    enum DeviceAccessStatus access_status;
    HSTRING str;
    HRESULT hr;
    ULONG ref;

    hr = WindowsCreateString( device_access_info_name, wcslen( device_access_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    WindowsDeleteString( str );

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered.\n", wine_dbgstr_w(device_access_info_name) );
        return;
    }

    hr = WindowsCreateString( device_info_name, wcslen( device_info_name ), &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory2 );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    WindowsDeleteString( str );

    ok( factory != factory2, "Got the same factory.\n" );
    IActivationFactory_Release( factory2 );

    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IDeviceAccessInformation, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDeviceAccessInformationStatics, (void **)&statics );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceAccessInformationStatics_CreateFromDeviceClass( statics, DeviceClass_AudioCapture, &access_info );
    ok( hr == S_OK || broken( hr == RPC_E_CALL_COMPLETE ) /* broken on some Testbot machines */, "got hr %#lx\n", hr );

    if (hr == S_OK)
    {
        hr = IDeviceAccessInformation_get_CurrentStatus( access_info, &access_status );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( access_status == DeviceAccessStatus_Allowed, "got %d.\n", access_status );
        ref = IDeviceAccessInformation_Release( access_info );
        ok( !ref, "got ref %lu\n", ref );
    }

    ref = IDeviceAccessInformationStatics_Release( statics );
    ok( ref == 2, "got ref %lu\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %lu\n", ref );
}

START_TEST( devices )
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    test_DeviceInformation();
    test_DeviceAccessInformation();

    RoUninitialize();
}
