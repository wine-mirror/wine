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
#include "weakreference.h"

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

struct device_watcher_added_handler_data
{
    LONG devices_added;
};

static void test_DeviceInformation_obj( int line, IDeviceInformation *info );
static void device_watcher_added_callback( IInspectable *arg1, IInspectable *arg2, void *param )
{
    struct device_watcher_added_handler_data *data = param;
    IDeviceInformation *device_info;
    HRESULT hr;

    check_interface( arg1, &IID_IDeviceWatcher, TRUE );
    hr = IInspectable_QueryInterface( arg2, &IID_IDeviceInformation, (void **)&device_info );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    InterlockedIncrement( &data->devices_added );
    test_DeviceInformation_obj( __LINE__, device_info );
    IDeviceInformation_Release( device_info );
}

static ITypedEventHandler_DeviceWatcher_DeviceInformation *device_watcher_added_handler_create( struct device_watcher_added_handler_data *data )
{
    return (ITypedEventHandler_DeviceWatcher_DeviceInformation *)inspectable_event_handler_create( &IID_ITypedEventHandler_DeviceWatcher_DeviceInformation,
                                                                                                   device_watcher_added_callback, data );
}

struct device_watcher_once_handler_data
{
    HANDLE event;
    LONG invoked;
};

static void device_watcher_once_callback( IInspectable *arg1, IInspectable *arg2, void *param )
{
    struct device_watcher_once_handler_data *data = param;
    BOOL ret;

    check_interface( arg1, &IID_IDeviceWatcher, TRUE );
    ok( !arg2, "got arg2 %p\n", arg2 );

    ok( InterlockedIncrement( &data->invoked ), "event handler invoked more than once\n" );
    ret = SetEvent( data->event );
    ok( ret, "SetEvent failed, last error %lu \n", GetLastError() );
}

static ITypedEventHandler_DeviceWatcher_IInspectable *device_watcher_once_handler_create( struct device_watcher_once_handler_data *data )
{
    return (ITypedEventHandler_DeviceWatcher_IInspectable *)inspectable_event_handler_create( &IID_ITypedEventHandler_DeviceWatcher_IInspectable,
                                                                                              device_watcher_once_callback, data );
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

#define check_device_information_collection_async( a, b, c, d, e ) check_device_information_collection_async_( __LINE__, a, TRUE, b, c, d, e )
#define check_device_information_collection_async_no_id( a, b, c, d  ) check_device_information_collection_async_( __LINE__, a, FALSE, 0, b, c, d )
static void check_device_information_collection_async_( int line, IAsyncOperation_DeviceInformationCollection *async,
                                                        BOOL test_id, UINT32 expect_id, AsyncStatus expect_status,
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
    if (test_id)
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

    ITypedEventHandler_DeviceWatcher_IInspectable *stopped_handler, *enumerated_handler;
    struct device_watcher_once_handler_data enumerated_data = {0}, stopped_data = {0};
    ITypedEventHandler_DeviceWatcher_DeviceInformation *device_added_handler;
    struct device_watcher_added_handler_data device_added_data = {0};
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
    IWeakReferenceSource *weak_src;
    IWeakReference *weak_ref;
    IDeviceWatcher *watcher;
    UINT32 i, size;
    HSTRING str;
    HRESULT hr;
    ULONG ref;

    enumerated_data.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!enumerated_data.event, "failed to create event, got error %lu\n", GetLastError() );
    stopped_data.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stopped_data.event, "failed to create event, got error %lu\n", GetLastError() );

    device_added_handler = device_watcher_added_handler_create( &device_added_data );
    stopped_handler = device_watcher_once_handler_create( &stopped_data );
    enumerated_handler = device_watcher_once_handler_create( &enumerated_data );

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

    hr = IDeviceWatcher_QueryInterface( device_watcher, &IID_IWeakReferenceSource, (void **)&weak_src );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    check_interface( weak_src, &IID_IAgileObject, TRUE );
    hr = IWeakReferenceSource_GetWeakReference( weak_src, &weak_ref );
    IWeakReferenceSource_Release( weak_src );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IWeakReference_Resolve( weak_ref, &IID_IDeviceWatcher, (IInspectable **)&watcher );
    IWeakReference_Release( weak_ref );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ref = IDeviceWatcher_Release( watcher );
    ok( ref == 1, "got ref %lu\n", ref );

    device_added_data.devices_added = 0;
    hr = IDeviceWatcher_add_Added( device_watcher, device_added_handler, &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped( device_watcher, stopped_handler, &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_data.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_data.invoked, "stopped_handler not invoked\n" );

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

    hr = IDeviceWatcher_add_Added( device_watcher, device_added_handler, &added_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_add_Stopped( device_watcher, stopped_handler, &stopped_token );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Created, "got status %u\n", status );

    stopped_data.invoked = 0;
    hr = IDeviceWatcher_Start( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Started, "got status %u\n", status );

    ref = IDeviceWatcher_AddRef( device_watcher );
    ok( ref == 2, "got ref %lu\n", ref );
    hr = IDeviceWatcher_Stop( device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !WaitForSingleObject( stopped_data.event, 1000 ), "wait for stopped_handler.event failed\n" );

    hr = IDeviceWatcher_get_Status( device_watcher, &status );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( status == DeviceWatcherStatus_Stopped, "got status %u\n", status );
    ok( stopped_data.invoked, "stopped_handler not invoked\n" );

    IDeviceWatcher_Release( device_watcher );

    hr = IDeviceInformationStatics_CreateWatcher( device_info_statics, &device_watcher );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    if (device_watcher)
    {
        hr = IDeviceWatcher_add_Added( device_watcher, device_added_handler, &added_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_add_EnumerationCompleted( device_watcher, enumerated_handler, &enumerated_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );

        hr = IDeviceWatcher_Start( device_watcher );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( !WaitForSingleObject( enumerated_data.event, 5000 ), "wait for enumerated_handler.event failed\n" );
        ok( device_added_data.devices_added > 0, "devices_added should be greater than 0\n" );
        hr = IDeviceWatcher_get_Status( device_watcher, &status );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( status == DeviceWatcherStatus_EnumerationCompleted, "got status %u\n", status );

        hr = IDeviceWatcher_Start( device_watcher );
        ok( hr == E_ILLEGAL_METHOD_CALL, "Start returned %#lx\n", hr );

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
    ITypedEventHandler_DeviceWatcher_DeviceInformation_Release( device_added_handler );
    ITypedEventHandler_DeviceWatcher_IInspectable_Release( stopped_handler );
    ITypedEventHandler_DeviceWatcher_IInspectable_Release( enumerated_handler );
    CloseHandle( stopped_data.event );
    CloseHandle( enumerated_data.event );
}

struct test_case_filter
{
    const WCHAR *filter;
    HRESULT hr;
    BOOL no_results;
    HRESULT start_hr; /* Code returned by IDeviceWatcher::Start */
};

#define test_FindAllAsyncAqsFilter( statics, test_cases, todo_hr, todo_results ) \
    test_FindAllAsyncAqsFilter_( statics, #test_cases, test_cases, ARRAY_SIZE( test_cases ), todo_hr, todo_results )

static void test_FindAllAsyncAqsFilter_( IDeviceInformationStatics *statics, const char *name, const struct test_case_filter *test_cases, SIZE_T len,
                                         BOOL todo_hr, BOOL todo_results )
{
    SIZE_T i;

    for (i = 0; i < len; i++)
    {
        IAsyncOperation_DeviceInformationCollection *devices_async;
        const struct test_case_filter *test_case = &test_cases[i];
        IVectorView_DeviceInformation *devices;
        UINT32 size;
        HSTRING str;
        HRESULT hr;

        winetest_push_context("%s[%Iu]", name, i );

        hr = WindowsCreateString( test_case->filter, wcslen( test_case->filter ), &str );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceInformationStatics_FindAllAsyncAqsFilter( statics, str, &devices_async );
        todo_wine_if( todo_hr ) ok( hr == test_case->hr, "got hr %#lx != %#lx\n", hr, test_case->hr );
        WindowsDeleteString( str );
        if (FAILED( hr ) || FAILED( test_case->hr ))
        {
            if (SUCCEEDED( hr )) IAsyncOperation_DeviceInformationCollection_Release( devices_async );
            winetest_pop_context();
            continue;
        }
        await_device_information_collection( devices_async );
        check_device_information_collection_async_no_id( devices_async, Completed, S_OK, &devices );

        hr = IVectorView_DeviceInformation_get_Size( devices, &size );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        todo_wine_if( todo_results ) ok( test_case->no_results == !size, "got size %I32u\n", size );
        IAsyncOperation_DeviceInformationCollection_Release( devices_async );
        IVectorView_DeviceInformation_Release( devices );

        winetest_pop_context();
    }
}

#define test_CreateWatcherAqsFilter( statics, test_cases, todo_hr, todo_start_hr, todo_wait, todo_results ) \
    test_CreateWatcherAqsFilter_( statics, #test_cases, test_cases, ARRAY_SIZE( test_cases ), todo_hr, todo_start_hr, todo_wait, todo_results )

static void test_CreateWatcherAqsFilter_( IDeviceInformationStatics *statics, const char *name, const struct test_case_filter *test_cases, SIZE_T len,
                                          BOOL todo_hr, BOOL todo_start_hr, BOOL todo_wait, BOOL todo_results )
{
    ITypedEventHandler_DeviceWatcher_IInspectable *enumerated_handler, *stopped_handler;
    struct device_watcher_once_handler_data enumerated_data = {0}, stopped_data = {0};
    SIZE_T i;

    enumerated_data.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!enumerated_data.event, "CreateEventW failed, last error %lu\n", GetLastError() );
    enumerated_handler = device_watcher_once_handler_create( &enumerated_data );

    stopped_data.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!stopped_data.event, "CreateEventW failed, last error %lu\n", GetLastError() );
    stopped_handler = device_watcher_once_handler_create( &stopped_data );

    for (i = 0; i < len; i++)
    {
        EventRegistrationToken added_token, enumerated_token, stopped_token;
        ITypedEventHandler_DeviceWatcher_DeviceInformation *added_handler;
        struct device_watcher_added_handler_data added_data = {0};
        const struct test_case_filter *test_case = &test_cases[i];
        IDeviceWatcher *watcher;
        HSTRING str;
        HRESULT hr;
        DWORD ret;

        winetest_push_context("%s[%Iu]", name, i );

        hr = WindowsCreateString( test_case->filter, wcslen( test_case->filter ), &str );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceInformationStatics_CreateWatcherAqsFilter( statics, str, &watcher );
        todo_wine_if( todo_hr ) ok( hr == test_case->hr, "got hr %#lx != %#lx\n", hr, test_case->hr );
        WindowsDeleteString( str );
        if (FAILED( hr ) || FAILED( test_case->hr ))
        {
            if (SUCCEEDED( hr )) IDeviceWatcher_Release( watcher );
            winetest_pop_context();
            continue;
        }

        added_handler = device_watcher_added_handler_create( &added_data );
        hr = IDeviceWatcher_add_Added( watcher, added_handler, &added_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ITypedEventHandler_DeviceWatcher_DeviceInformation_Release( added_handler );
        hr = IDeviceWatcher_add_EnumerationCompleted( watcher, enumerated_handler, &enumerated_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_add_Stopped( watcher, stopped_handler, &stopped_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_Start( watcher );
        todo_wine_if( todo_start_hr ) ok( hr == test_case->start_hr, "got hr %#lx\n", hr );
        if (FAILED( hr ) || FAILED( test_case->start_hr )) goto next;

        ret = WaitForSingleObject( enumerated_data.event, 500 );
        todo_wine_if( todo_wait ) ok( !ret, "WaitForSingleObject returned %lu\n", ret );

        hr = IDeviceWatcher_Stop( watcher );
        ret = WaitForSingleObject( stopped_data.event, 500 );
        todo_wine_if( todo_wait ) ok( !ret, "WaitForSingleObject returned %lu\n", ret );
        todo_wine_if( todo_results ) ok( test_case->no_results == !added_data.devices_added, "got devices_added %lu\n", added_data.devices_added );

    next:
        hr = IDeviceWatcher_remove_Added( watcher, added_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_remove_Stopped( watcher, stopped_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        hr = IDeviceWatcher_remove_EnumerationCompleted( watcher, enumerated_token );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        IDeviceWatcher_Release( watcher );

        winetest_pop_context();

        enumerated_data.invoked = 0;
        stopped_data.invoked = 0;
    }

    ITypedEventHandler_DeviceWatcher_IInspectable_Release( enumerated_handler );
    ITypedEventHandler_DeviceWatcher_IInspectable_Release( stopped_handler );
    CloseHandle( enumerated_data.event );
    CloseHandle( stopped_data.event );
}

static const struct test_case_filter filters_empty[] = { { L"", S_OK } };
static const struct test_case_filter filters_simple[] = {
    { L"System.Devices.InterfaceEnabled := System.StructuredQueryType.Boolean#True", S_OK },
    { L"System.Devices.InterfaceEnabled : System.StructuredQueryType.Boolean#True", S_OK },
    { L"\t\nSystem.Devices.InterfaceEnabled\n\t\n:=\r\n\tSystem.StructuredQueryType.Boolean#True  ", S_OK },
    { L"System.Devices.InterfaceEnabled :NOT System.StructuredQueryType.Boolean#False", S_OK },
    { L"System.Devices.InterfaceEnabled :<> System.StructuredQueryType.Boolean#False", S_OK },
    { L"System.Devices.InterfaceEnabled :- System.StructuredQueryType.Boolean#False", S_OK },
    { L"System.Devices.InterfaceEnabled :\u2260 System.StructuredQueryType.Boolean#False", S_OK },
    { L"System.Devices.InterfaceClassGuid :\u2260 {deadbeef-dead-beef-dead-deadbeefdead}", S_OK },
    { L"System.Devices.InterfaceEnabled :NOT []", S_OK },
    { L"System.Devices.InterfaceEnabled := System.StructuredQueryType.Boolean#True "
        L"System.Devices.DeviceInstanceId :NOT []", S_OK },
    { L"(((System.Devices.DeviceInstanceId :NOT [])))", S_OK },
};
static const struct test_case_filter filters_boolean_op[] = {
    { L"NOT System.Devices.DeviceInstanceId := []", S_OK },
    { L"(NOT (NOT (NOT System.Devices.DeviceInstanceId := [])))", S_OK },
    { L"System.Devices.DeviceInstanceId := [] OR System.Devices.DeviceInstanceId :NOT []", S_OK },
    { L"NOT ((System.Devices.DeviceInstanceId :NOT []) AND System.Devices.DeviceInstanceId := [])", S_OK },
    { L"NOT ((NOT System.Devices.InterfaceEnabled := []) AND System.Devices.DeviceInstanceId := [])", S_OK }
};
/* Propsys canonical property names are case-sensitive, but not in AQS. */
static const struct test_case_filter filters_case_insensitive[] = {
    { L"SYSTEM.DEVICES.InterfaceEnabled := System.StructuredQueryType.Boolean#True", S_OK },
    { L"system.devices.interfaceenabled : SYSTEM.STRUCTUREDQUERYTYPE.BOOLEAN#true", S_OK },
    { L"SYSTEM.DEVICES.INTERFACEENABLED :- SYSTEM.STRUCTUREDQUERYTYPE.BOOLEAN#FALSE", S_OK },
    { L"system.devices.interfaceenabled :NOT system.structuredquerytype.boolean#false", S_OK },
    { L"SYSTEM.DEVICES.INTERFACECLASSGUID :\u2260 {DEADBEEF-DEAD-BEEF-DEAD-DEADBEEFDEAD}", S_OK },
};
static const struct test_case_filter filters_precedence[] = {
    /* Gets parsed as ((DeviceInstanceId := [] AND DeviceInstanceId := []) OR DeviceInstanceId :NOT []) */
    { L"System.Devices.DeviceInstanceId := [] System.Devices.DeviceInstanceId := [] OR System.Devices.DeviceInstanceId :NOT []", S_OK },
    /* Gets parsed as ((DeviceInstanceId := [] OR DeviceInstanceId :NOT []) AND DeviceInstanceId :NOT []) */
    { L"System.Devices.DeviceInstanceId := [] OR System.Devices.DeviceInstanceId :NOT [] System.Devices.DeviceInstanceId :NOT []", S_OK }
};
/* Queries that succeed but don't return any results. */
static const struct test_case_filter filters_no_results[] = {
    /* Gets parsed as ((NOT DeviceInstanceId := []) AND DeviceInstanceId := []) */
    { L"NOT System.Devices.DeviceInstanceId := [] System.Devices.DeviceInstanceId := []", S_OK, TRUE },
    { L"System.Devices.InterfaceClassGuid := {deadbeef-dead-beef-dead-deadbeefdead}", S_OK, TRUE },
    { L"System.Devices.DeviceInstanceId := \"invalid\\device\\id\"", S_OK, TRUE },
    { L"System.Devices.InterfaceEnabled := []", S_OK, TRUE },
    { L"System.Devices.DeviceInstanceId := []", S_OK, TRUE },
    { L"System.Devices.InterfaceEnabled := System.StructuredQueryType.Boolean#True "
      L"System.Devices.InterfaceEnabled := System.StructuredQueryType.Boolean#False", S_OK, TRUE },
    { L"System.Devices.InterfaceClassGuid :< {deadbeef-dead-beef-dead-deadbeefdead} OR "
      L"System.Devices.InterfaceClassGuid :> {deadbeef-dead-beef-dead-deadbeefdead}", S_OK, TRUE }
};
static const struct test_case_filter filters_invalid_comparand_type[] = {
    { L"System.Devices.InterfaceEnabled := \"foo\"", E_INVALIDARG },
    { L"System.Devices.InterfaceEnabled := {deadbeef-dead-beef-dead-deadbeefdead}", E_INVALIDARG },
    { L"System.Devices.InterfaceClassGuid := System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfaceClassGuid :- System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfaceEnabled := System.Devices.InterfaceEnabled", E_INVALIDARG }, /* RHS is parsed as a string. */
};
static const struct test_case_filter filters_invalid_empty[] = {
    { L" ", E_INVALIDARG },
    { L"\t", E_INVALIDARG },
    { L"\n", E_INVALIDARG },
};
/* CreateWatcher* accepts whitespace-only strings, the error will be returned by IDeviceWatcher_Start instead. */
static const struct test_case_filter filters_empty_watcher[] = {
    { L" ", S_OK, FALSE, E_INVALIDARG },
    { L"\t", S_OK, FALSE, E_INVALIDARG },
    { L"\n", S_OK, FALSE, E_INVALIDARG },
};
static const struct test_case_filter filters_invalid_operator[] = {
    { L"System.Devices.InterfaceEnabled = System.StructuredQueryType.Boolean#True", E_INVALIDARG }, /* Missing colon */
    { L"System.Devices.InterfacesEnabled :not System.StructuredQueryType.Boolean#True", E_INVALIDARG }, /* Named operators are case-sensitive */
    { L"System.Devices.InterfacesEnabled System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfacesEnabled := := System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfacesEnabled :!= System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfacesEnabled :\U0001F377 System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L"System.Devices.InterfacesEnabled", E_INVALIDARG },
    { L"System.StructuredQueryType.Boolean#True", E_INVALIDARG },
};
static const struct test_case_filter filters_invalid_operand[] = {
    { L"System.Devices.InterfaceEnabled := ", E_INVALIDARG },
    { L":= System.StructuredQueryType.Boolean#True", E_INVALIDARG },
    { L":=", E_INVALIDARG },
    { L" System.StructuredQueryType.Boolean#True := System.StructuredQueryType.Boolean#True", E_INVALIDARG },
};

static void test_aqs_filters( void )
{
    static const WCHAR *class_name = RuntimeClass_Windows_Devices_Enumeration_DeviceInformation;
    IDeviceInformationStatics *statics;
    HSTRING str;
    HRESULT hr;

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok(hr == S_OK, "got hr %#lx\n", hr );
    hr = RoGetActivationFactory( str, &IID_IDeviceInformationStatics, (void **)&statics );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    WindowsDeleteString( str  );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass, not registered.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    test_FindAllAsyncAqsFilter( statics, filters_empty, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_empty, FALSE, FALSE, FALSE, FALSE );

    test_FindAllAsyncAqsFilter( statics, filters_boolean_op, TRUE, TRUE );
    test_CreateWatcherAqsFilter( statics, filters_boolean_op, FALSE, FALSE, TRUE, TRUE );

    test_FindAllAsyncAqsFilter( statics, filters_simple, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_simple, FALSE, FALSE, TRUE, TRUE );

    test_FindAllAsyncAqsFilter( statics, filters_case_insensitive, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_case_insensitive, FALSE, FALSE, TRUE, TRUE );

    test_FindAllAsyncAqsFilter( statics, filters_precedence, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_precedence, FALSE, FALSE, TRUE, TRUE );

    test_FindAllAsyncAqsFilter( statics, filters_no_results, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_no_results, FALSE, FALSE, TRUE, FALSE );

    test_FindAllAsyncAqsFilter( statics, filters_invalid_comparand_type, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_invalid_comparand_type, TRUE, FALSE, FALSE, FALSE );

    test_FindAllAsyncAqsFilter( statics, filters_invalid_empty, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_empty_watcher, FALSE, TRUE, FALSE, FALSE );

    test_FindAllAsyncAqsFilter( statics, filters_invalid_operator, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_invalid_operator, TRUE, FALSE, FALSE, FALSE );

    test_FindAllAsyncAqsFilter( statics, filters_invalid_operand, TRUE, FALSE );
    test_CreateWatcherAqsFilter( statics, filters_invalid_operand, TRUE, FALSE, FALSE, FALSE );

    IDeviceInformationStatics_Release( statics );
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
    test_aqs_filters();

    RoUninitialize();
}
