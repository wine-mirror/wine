/*
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
#define WIDL_using_Windows_Devices_Radios
#include "windows.devices.radios.h"

#include "wine/test.h"

static HRESULT (WINAPI *pWindowsDeleteString)( HSTRING str );
static const WCHAR* (WINAPI *pWindowsGetStringRawBuffer)( HSTRING, UINT32* );

static BOOL load_combase_functions(void)
{
    HMODULE combase = GetModuleHandleW( L"combase.dll" );

#define LOAD_FUNC(m, f) if (!(p ## f = (void *)GetProcAddress( m, #f ))) goto failed;
    LOAD_FUNC( combase, WindowsDeleteString );
    LOAD_FUNC( combase, WindowsGetStringRawBuffer );
#undef LOAD_FUNC

    return TRUE;

failed:
    win_skip("Failed to load combase.dll functions, skipping tests\n");
    return FALSE;
}

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

#define check_runtimeclass( a, b ) check_runtimeclass_( __LINE__, (IInspectable *)a, b )
static void check_runtimeclass_( int line, IInspectable *inspectable, const WCHAR *class_name )
{
    const WCHAR *buffer;
    UINT32 length;
    HSTRING str;
    HRESULT hr;

    hr = IInspectable_GetRuntimeClassName( inspectable, &str );
    ok_(__FILE__, line)( hr == S_OK, "GetRuntimeClassName returned %#lx\n", hr );
    buffer = pWindowsGetStringRawBuffer( str, &length );
    todo_wine
    ok_(__FILE__, line)( !wcscmp( buffer, class_name ), "got class name %s\n", debugstr_w(buffer) );
    pWindowsDeleteString( str );
}

#define check_vectorview_radio_async( a, b, c, d ) check_vectorview_radio_async_( __LINE__, a, b, c, d )
static void check_vectorview_radio_async_( int line, IAsyncOperation_IVectorView_Radio *async, UINT32 expect_id, AsyncStatus expect_status, HRESULT expect_hr )
{
    IVectorView_Radio *result;
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_IVectorView_Radio_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
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
    ok_(__FILE__, line)( async_status == expect_status || broken( async_status == Error ), "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4)
        todo_wine_if(FAILED(expect_hr)) ok_(__FILE__, line)( async_hr == expect_hr || broken( async_hr == E_INVALIDARG), "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );

    hr = IAsyncOperation_IVectorView_Radio_GetResults( async, &result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr, "GetResults returned %#lx\n", hr );
        ok_(__FILE__, line)( result != NULL, "failed to get result\n" );

        if (result)
        {
            UINT32 size = 0xdeadbeef;

            hr = IVectorView_Radio_get_Size( result, &size );
            ok_(__FILE__, line)( hr == S_OK, "IVectorView_Radio_get_Size returned %#lx\n", hr );
            ok_(__FILE__, line)( size != 0xdeadbeef, "failed to get size\n" );
        }
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

struct vectorview_radio_async_handler
{
    IAsyncOperationCompletedHandler_IVectorView_Radio IAsyncOperationCompletedHandler_IVectorView_Radio_iface;
    IAsyncOperation_IVectorView_Radio *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct vectorview_radio_async_handler *impl_from_IAsyncOperationCompletedHandler_IVectorView_Radio(
    IAsyncOperationCompletedHandler_IVectorView_Radio *iface )
{
    return CONTAINING_RECORD( iface, struct vectorview_radio_async_handler, IAsyncOperationCompletedHandler_IVectorView_Radio_iface );
}

static HRESULT WINAPI vectorview_radio_async_handler_QueryInterface( IAsyncOperationCompletedHandler_IVectorView_Radio *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_IVectorView_Radio ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI vectorview_radio_async_handler_AddRef( IAsyncOperationCompletedHandler_IVectorView_Radio *iface )
{
    return 2;
}

static ULONG WINAPI vectorview_radio_async_handler_Release( IAsyncOperationCompletedHandler_IVectorView_Radio *iface )
{
    return 1;
}

static HRESULT WINAPI vectorview_radio_async_handler_Invoke( IAsyncOperationCompletedHandler_IVectorView_Radio *iface,
                                                             IAsyncOperation_IVectorView_Radio *async, AsyncStatus status )
{
    struct vectorview_radio_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_IVectorView_Radio( iface );

    trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_IVectorView_RadioVtbl vectorview_radio_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    vectorview_radio_async_handler_QueryInterface,
    vectorview_radio_async_handler_AddRef,
    vectorview_radio_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<IAsyncOperationCompletedHandler_IVectorView_Radio> methods ***/
    vectorview_radio_async_handler_Invoke,
};

static struct vectorview_radio_async_handler default_vectorview_radio_async_handler = {{&vectorview_radio_async_handler_vtbl}};

static void test_RadioStatics(void)
{
    IAsyncOperationCompletedHandler_IVectorView_Radio *tmp_handler = (void *)0xdeadbeef;
    static const WCHAR *radio_statics_name = L"Windows.Devices.Radios.Radio";
    struct vectorview_radio_async_handler vectorview_radio_async_handler;
    IAsyncOperation_IVectorView_Radio *operation = (void *)0xdeadbeef;
    IRadioStatics *radio_statics = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IAsyncInfo *async_info = (void *)0xdeadbeef;
    HSTRING str;
    HRESULT hr;
    DWORD ret;

    if (!load_combase_functions()) return;

    hr = WindowsCreateString( radio_statics_name, wcslen( radio_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( radio_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IRadioStatics, (void **)&radio_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IRadioStatics_GetRadiosAsync( radio_statics, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IRadioStatics_GetRadiosAsync( radio_statics, &operation );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( operation, &IID_IUnknown );
    check_interface( operation, &IID_IInspectable );
    check_interface( operation, &IID_IAgileObject );
    check_interface( operation, &IID_IAsyncInfo );
    check_interface( operation, &IID_IAsyncOperation_IVectorView_Radio );
    check_runtimeclass( operation, L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Radios.Radio>>" );

    hr = IAsyncOperation_IVectorView_Radio_get_Completed( operation, &tmp_handler );
    ok( hr == S_OK, "get_Completed returned %#lx\n", hr );
    ok( tmp_handler == NULL, "got handler %p\n", tmp_handler );
    vectorview_radio_async_handler = default_vectorview_radio_async_handler;
    vectorview_radio_async_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    hr = IAsyncOperation_IVectorView_Radio_put_Completed( operation, &vectorview_radio_async_handler.IAsyncOperationCompletedHandler_IVectorView_Radio_iface );
    ok( hr == S_OK, "put_Completed returned %#lx\n", hr );
    ret = WaitForSingleObject( vectorview_radio_async_handler.event, 1000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( vectorview_radio_async_handler.event );
    ok( ret, "CloseHandle failed, error %lu\n", GetLastError() );
    ok( vectorview_radio_async_handler.invoked, "handler not invoked\n" );
    ok( vectorview_radio_async_handler.async == operation, "got async %p\n", vectorview_radio_async_handler.async );
    ok( vectorview_radio_async_handler.status == Completed, "got status %u\n", vectorview_radio_async_handler.status );
    vectorview_radio_async_handler = default_vectorview_radio_async_handler;
    hr = IAsyncOperation_IVectorView_Radio_put_Completed( operation, &vectorview_radio_async_handler.IAsyncOperationCompletedHandler_IVectorView_Radio_iface );
    ok( hr == E_ILLEGAL_DELEGATE_ASSIGNMENT, "put_Completed returned %#lx\n", hr );
    ok( !vectorview_radio_async_handler.invoked, "handler invoked\n" );
    ok( vectorview_radio_async_handler.async == NULL, "got async %p\n", vectorview_radio_async_handler.async );
    ok( vectorview_radio_async_handler.status == Started, "got status %u\n", vectorview_radio_async_handler.status );

    hr = IAsyncOperation_IVectorView_Radio_QueryInterface( operation, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IAsyncInfo_Cancel( async_info );
    ok( hr == S_OK, "Cancel returned %#lx\n", hr );
    check_vectorview_radio_async( operation, 1, Completed, S_OK );
    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "Close returned %#lx\n", hr );
    check_vectorview_radio_async( operation, 1, 4, S_OK );

    IAsyncInfo_Release( async_info );
    IAsyncOperation_IVectorView_Radio_Release( operation );
    IRadioStatics_Release( radio_statics );
    IActivationFactory_Release( factory );
}

START_TEST(radios)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_RadioStatics();

    RoUninitialize();
}
