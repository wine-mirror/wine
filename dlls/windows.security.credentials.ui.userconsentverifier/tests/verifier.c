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
#define WIDL_using_Windows_Security_Credentials_UI
#include "windows.security.credentials.ui.h"

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

#define check_verifier_async( a, b, c, d, e ) check_verifier_async_( __LINE__, a, b, c, d, e )
static void check_verifier_async_( int line, IAsyncOperation_UserConsentVerifierAvailability *async, UINT32 expect_id, AsyncStatus expect_status,
                                   HRESULT expect_hr, UserConsentVerifierAvailability expect_result )
{
    UserConsentVerifierAvailability result;
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_UserConsentVerifierAvailability_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
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

    hr = IAsyncOperation_UserConsentVerifierAvailability_GetResults( async, &result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr || broken( hr == E_INVALIDARG ), "GetResults returned %#lx\n", hr );
        ok_(__FILE__, line)( result == expect_result || broken(result == UserConsentVerifierAvailability_Available /*Win10 1507 */), "got result %u\n", result );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

struct verifier_async_handler
{
    IAsyncOperationCompletedHandler_UserConsentVerifierAvailability IAsyncOperationCompletedHandler_UserConsentVerifierAvailability_iface;
    IAsyncOperation_UserConsentVerifierAvailability *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct verifier_async_handler *impl_from_IAsyncOperationCompletedHandler_UserConsentVerifierAvailability(
    IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *iface )
{
    return CONTAINING_RECORD( iface, struct verifier_async_handler, IAsyncOperationCompletedHandler_UserConsentVerifierAvailability_iface );
}

static HRESULT WINAPI verifier_async_handler_QueryInterface( IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_UserConsentVerifierAvailability ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI verifier_async_handler_AddRef( IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *iface )
{
    return 2;
}

static ULONG WINAPI verifier_async_handler_Release( IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *iface )
{
    return 1;
}

static HRESULT WINAPI verifier_async_handler_Invoke( IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *iface,
                                                     IAsyncOperation_UserConsentVerifierAvailability *async, AsyncStatus status )
{
    struct verifier_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_UserConsentVerifierAvailability( iface );

    trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_UserConsentVerifierAvailabilityVtbl verifier_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    verifier_async_handler_QueryInterface,
    verifier_async_handler_AddRef,
    verifier_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<UserConsentVerifierAvailability> methods ***/
    verifier_async_handler_Invoke,
};

static struct verifier_async_handler default_verifier_async_handler = {{&verifier_async_handler_vtbl}};

static void test_UserConsentVerifierStatics(void)
{
    static const WCHAR *user_consent_verifier_statics_name = L"Windows.Security.Credentials.UI.UserConsentVerifier";
    IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *tmp_handler = NULL;
    IAsyncOperation_UserConsentVerifierAvailability *verifier_async = NULL;
    IUserConsentVerifierStatics *user_consent_verifier_statics = NULL;
    struct verifier_async_handler verifier_async_handler;
    IAsyncInfo *async_info = NULL;
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;
    DWORD ret;

    hr = WindowsCreateString( user_consent_verifier_statics_name, wcslen( user_consent_verifier_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( user_consent_verifier_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );

    hr = IActivationFactory_QueryInterface( factory, &IID_IUserConsentVerifierStatics, (void **)&user_consent_verifier_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    if (!load_combase_functions()) return;

    hr = IUserConsentVerifierStatics_CheckAvailabilityAsync( user_consent_verifier_statics, &verifier_async );
    ok( hr == S_OK, "CheckAvailabilityAsync returned %#lx\n", hr );

    check_interface( verifier_async, &IID_IUnknown );
    check_interface( verifier_async, &IID_IInspectable );
    check_interface( verifier_async, &IID_IAgileObject );
    check_interface( verifier_async, &IID_IAsyncInfo );
    check_interface( verifier_async, &IID_IAsyncOperation_UserConsentVerifierAvailability );
    check_runtimeclass( verifier_async, L"Windows.Foundation.IAsyncOperation`1<Windows.Security.Credentials.UI.UserConsentVerifierAvailability>" );

    hr = IAsyncOperation_UserConsentVerifierAvailability_get_Completed( verifier_async, &tmp_handler );
    ok( hr == S_OK, "get_Completed returned %#lx\n", hr );
    ok( tmp_handler == NULL, "got handler %p\n", tmp_handler );
    verifier_async_handler = default_verifier_async_handler;
    verifier_async_handler.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    hr = IAsyncOperation_UserConsentVerifierAvailability_put_Completed( verifier_async, &verifier_async_handler.IAsyncOperationCompletedHandler_UserConsentVerifierAvailability_iface );
    ok( hr == S_OK, "put_Completed returned %#lx\n", hr );
    ret = WaitForSingleObject( verifier_async_handler.event, 1000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( verifier_async_handler.event );
    ok( ret, "CloseHandle failed, error %lu\n", GetLastError() );
    ok( verifier_async_handler.invoked, "handler not invoked\n" );
    ok( verifier_async_handler.async == verifier_async, "got async %p\n", verifier_async_handler.async );
    ok( verifier_async_handler.status == Completed, "got status %u\n", verifier_async_handler.status );
    verifier_async_handler = default_verifier_async_handler;
    hr = IAsyncOperation_UserConsentVerifierAvailability_put_Completed( verifier_async, &verifier_async_handler.IAsyncOperationCompletedHandler_UserConsentVerifierAvailability_iface );
    ok( hr == E_ILLEGAL_DELEGATE_ASSIGNMENT, "put_Completed returned %#lx\n", hr );
    ok( !verifier_async_handler.invoked, "handler invoked\n" );
    ok( verifier_async_handler.async == NULL, "got async %p\n", verifier_async_handler.async );
    ok( verifier_async_handler.status == Started, "got status %u\n", verifier_async_handler.status );

    hr = IAsyncOperation_UserConsentVerifierAvailability_QueryInterface( verifier_async, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IAsyncInfo_Cancel( async_info );
    ok( hr == S_OK, "Cancel returned %#lx\n", hr );
    check_verifier_async( verifier_async, 1, Completed, S_OK, UserConsentVerifierAvailability_DeviceNotPresent );
    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "Close returned %#lx\n", hr );
    check_verifier_async( verifier_async, 1, 4, S_OK, FALSE );

    IAsyncInfo_Release( async_info );
    IAsyncOperation_UserConsentVerifierAvailability_Release( verifier_async );
    IUserConsentVerifierStatics_Release( user_consent_verifier_statics );
    IActivationFactory_Release( factory );
}

START_TEST(verifier)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_UserConsentVerifierStatics();

    RoUninitialize();
}
