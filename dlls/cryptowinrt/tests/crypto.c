/*
 * Copyright (C) 2022 Mohamad Al-Jaf
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
#define WIDL_using_Windows_Storage_Streams
#include "windows.storage.streams.h"
#define WIDL_using_Windows_Security_Cryptography
#include "windows.security.cryptography.h"
#define WIDL_using_Windows_Security_Credentials
#include "windows.security.credentials.h"
#include "robuffer.h"

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
    ok_(__FILE__, line)( !wcscmp( buffer, class_name ), "got class name %s\n", debugstr_w(buffer) );
    pWindowsDeleteString( str );
}

#define check_bool_async( a, b, c, d, e ) check_bool_async_( __LINE__, a, b, c, d, e )
static void check_bool_async_( int line, IAsyncOperation_boolean *async, UINT32 expect_id, AsyncStatus expect_status,
                               HRESULT expect_hr, BOOLEAN expect_result )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;
    BOOLEAN result;

    hr = IAsyncOperation_boolean_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    if (expect_id) ok_(__FILE__, line)( async_id == expect_id, "got id %u\n", async_id );
    else trace("Skipping async_id check, got id %u\n", async_id );

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

    result = !expect_result;
    hr = IAsyncOperation_boolean_GetResults( async, &result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr || broken( hr == E_INVALIDARG ), "GetResults returned %#lx\n", hr );
        ok_(__FILE__, line)( result == expect_result, "got result %u\n", result );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

struct bool_async_handler
{
    IAsyncOperationCompletedHandler_boolean IAsyncOperationCompletedHandler_boolean_iface;
    IAsyncOperation_boolean *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct bool_async_handler *impl_from_IAsyncOperationCompletedHandler_boolean( IAsyncOperationCompletedHandler_boolean *iface )
{
    return CONTAINING_RECORD( iface, struct bool_async_handler, IAsyncOperationCompletedHandler_boolean_iface );
}

static HRESULT WINAPI bool_async_handler_QueryInterface( IAsyncOperationCompletedHandler_boolean *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_boolean ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI bool_async_handler_AddRef( IAsyncOperationCompletedHandler_boolean *iface )
{
    return 2;
}

static ULONG WINAPI bool_async_handler_Release( IAsyncOperationCompletedHandler_boolean *iface )
{
    return 1;
}

static HRESULT WINAPI bool_async_handler_Invoke( IAsyncOperationCompletedHandler_boolean *iface,
                                                 IAsyncOperation_boolean *async, AsyncStatus status )
{
    struct bool_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_boolean( iface );

    trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_booleanVtbl bool_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    bool_async_handler_QueryInterface,
    bool_async_handler_AddRef,
    bool_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<boolean> methods ***/
    bool_async_handler_Invoke,
};

static struct bool_async_handler default_bool_async_handler = {{&bool_async_handler_vtbl}};

static void test_CryptobufferStatics(void)
{
    static const WCHAR *cryptobuffer_statics_name = L"Windows.Security.Cryptography.CryptographicBuffer";
    static const WCHAR *buffer_name = L"Windows.Storage.Streams.Buffer";
    ICryptographicBufferStatics *cryptobuffer_statics;
    IActivationFactory *factory, *factory2;
    IBufferByteAccess *buffer_access;
    IBufferFactory *buffer_factory;
    const WCHAR *base64_str;
    IBuffer *buffer;
    HSTRING str;
    byte *data;
    UINT32 len;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( cryptobuffer_statics_name, wcslen( cryptobuffer_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( cryptobuffer_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_ICryptographicBufferStatics, (void **)&cryptobuffer_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    /* NULL buffer */
    hr = ICryptographicBufferStatics_EncodeToBase64String( cryptobuffer_statics, NULL, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    base64_str = WindowsGetStringRawBuffer( str, &len );
    ok( !wcscmp( L"", base64_str ) && !len, "got str %s, len %d.\n", wine_dbgstr_w( base64_str ), len );
    WindowsDeleteString( str );

    hr = WindowsCreateString( buffer_name, wcslen( buffer_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IActivationFactory_QueryInterface( factory2, &IID_IBufferFactory, (void **)&buffer_factory );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    /* Empty buffer */
    hr = IBufferFactory_Create( buffer_factory, 0, &buffer );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = ICryptographicBufferStatics_EncodeToBase64String( cryptobuffer_statics, buffer, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    base64_str = WindowsGetStringRawBuffer( str, &len );
    ok( !wcscmp( L"", base64_str ) && !len, "got str %s, len %d.\n", wine_dbgstr_w( base64_str ), len );
    WindowsDeleteString( str );
    IBuffer_Release( buffer );

    /* Test with contents */
    hr = IBufferFactory_Create( buffer_factory, 16, &buffer );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IBuffer_QueryInterface( buffer, &IID_IBufferByteAccess, (void **)&buffer_access );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IBufferByteAccess_Buffer( buffer_access, &data );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    for (int i = 0; i < 16; ++i)
        data[i] = i;
    hr = IBuffer_put_Length( buffer, 16 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IBufferByteAccess_Release( buffer_access );

    hr = ICryptographicBufferStatics_EncodeToBase64String( cryptobuffer_statics, buffer, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    base64_str = WindowsGetStringRawBuffer( str, &len );
    ok( !wcscmp( L"AAECAwQFBgcICQoLDA0ODw==", base64_str ), "got str %s, len %d.\n", wine_dbgstr_w( base64_str ), len );
    WindowsDeleteString( str );
    IBuffer_Release( buffer );

    IBufferFactory_Release( buffer_factory );
    IActivationFactory_Release( factory2 );

    ref = ICryptographicBufferStatics_Release( cryptobuffer_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_Credentials_Statics(void)
{
    static const WCHAR *credentials_statics_name = L"Windows.Security.Credentials.KeyCredentialManager";
    IAsyncOperationCompletedHandler_boolean *tmp_handler;
    IKeyCredentialManagerStatics *credentials_statics;
    struct bool_async_handler bool_async_handler;
    IAsyncOperation_boolean *bool_async;
    IActivationFactory *factory;
    IAsyncInfo *async_info;
    HSTRING str;
    HRESULT hr;
    DWORD ret;

    hr = WindowsCreateString( credentials_statics_name, wcslen( credentials_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( credentials_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IKeyCredentialManagerStatics, (void **)&credentials_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    if (!load_combase_functions()) return;

    hr = IKeyCredentialManagerStatics_IsSupportedAsync( credentials_statics, &bool_async );
    ok( hr == S_OK, "IsSupportedAsync returned %#lx\n", hr );

    check_interface( bool_async, &IID_IUnknown );
    check_interface( bool_async, &IID_IInspectable );
    check_interface( bool_async, &IID_IAgileObject );
    check_interface( bool_async, &IID_IAsyncInfo );
    check_interface( bool_async, &IID_IAsyncOperation_boolean );
    check_runtimeclass( bool_async, L"Windows.Foundation.IAsyncOperation`1<Boolean>" );

    hr = IAsyncOperation_boolean_get_Completed( bool_async, &tmp_handler );
    ok( hr == S_OK, "get_Completed returned %#lx\n", hr );
    ok( tmp_handler == NULL, "got handler %p\n", tmp_handler );
    bool_async_handler = default_bool_async_handler;
    bool_async_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    hr = IAsyncOperation_boolean_put_Completed( bool_async, &bool_async_handler.IAsyncOperationCompletedHandler_boolean_iface );
    ok( hr == S_OK, "put_Completed returned %#lx\n", hr );
    ret = WaitForSingleObject( bool_async_handler.event, 1000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( bool_async_handler.event );
    ok( ret, "CloseHandle failed, error %lu\n", GetLastError() );
    ok( bool_async_handler.invoked, "handler not invoked\n" );
    ok( bool_async_handler.async == bool_async, "got async %p\n", bool_async_handler.async );
    ok( bool_async_handler.status == Completed || broken( bool_async_handler.status == Error ), "got status %u\n", bool_async_handler.status );
    bool_async_handler = default_bool_async_handler;
    hr = IAsyncOperation_boolean_put_Completed( bool_async, &bool_async_handler.IAsyncOperationCompletedHandler_boolean_iface );
    ok( hr == E_ILLEGAL_DELEGATE_ASSIGNMENT, "put_Completed returned %#lx\n", hr );
    ok( !bool_async_handler.invoked, "handler invoked\n" );
    ok( bool_async_handler.async == NULL, "got async %p\n", bool_async_handler.async );
    ok( bool_async_handler.status == Started, "got status %u\n", bool_async_handler.status );

    hr = IAsyncOperation_boolean_QueryInterface( bool_async, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IAsyncInfo_Cancel( async_info );
    ok( hr == S_OK, "Cancel returned %#lx\n", hr );
    check_bool_async( bool_async, 0, Completed, S_OK, FALSE );
    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "Close returned %#lx\n", hr );
    check_bool_async( bool_async, 0, 4, S_OK, FALSE );
    IAsyncInfo_Release( async_info );

    IAsyncOperation_boolean_Release( bool_async );

    IKeyCredentialManagerStatics_Release( credentials_statics );
    IActivationFactory_Release( factory );
}

START_TEST(crypto)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_CryptobufferStatics();
    test_Credentials_Statics();

    RoUninitialize();
}
