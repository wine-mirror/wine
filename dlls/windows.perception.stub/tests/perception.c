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
#define WIDL_using_Windows_Perception_Spatial
#define WIDL_using_Windows_Perception_Spatial_Surfaces
#include "windows.perception.spatial.surfaces.h"
#define WIDL_using_Windows_Graphics_Holographic
#include "windows.graphics.holographic.h"
#include "holographicspaceinterop.h"

#include "wine/test.h"

#define check_interface( obj, iid, can_be_broken ) check_interface_( __LINE__, obj, iid, can_be_broken )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL can_be_broken )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || broken( can_be_broken && hr == E_NOINTERFACE ), "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void flush_events(void)
{
    int min_timeout = 100, diff = 200;
    DWORD time = GetTickCount() + diff;
    MSG msg;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        diff = time - GetTickCount();
    }
}

#define create_foreground_window( a ) create_foreground_window_( __FILE__, __LINE__, a, 5 )
HWND create_foreground_window_( const char *file, int line, BOOL fullscreen, UINT retries )
{
    for (;;)
    {
        HWND hwnd;
        BOOL ret;

        hwnd = CreateWindowW( L"static", NULL, WS_POPUP | (fullscreen ? 0 : WS_VISIBLE),
                              100, 100, 200, 200, NULL, NULL, NULL, NULL );
        ok_(file, line)( hwnd != NULL, "CreateWindowW failed, error %lu\n", GetLastError() );

        if (fullscreen)
        {
            HMONITOR hmonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );
            MONITORINFO mi = {.cbSize = sizeof(MONITORINFO)};

            ok_(file, line)( hmonitor != NULL, "MonitorFromWindow failed, error %lu\n", GetLastError() );
            ret = GetMonitorInfoW( hmonitor, &mi );
            ok_(file, line)( ret, "GetMonitorInfoW failed, error %lu\n", GetLastError() );
            ret = SetWindowPos( hwnd, 0, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                                mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW );
            ok_(file, line)( ret, "SetWindowPos failed, error %lu\n", GetLastError() );
        }
        flush_events();

        if (GetForegroundWindow() == hwnd) return hwnd;
        ok_(file, line)( retries > 0, "failed to create foreground window\n" );
        if (!retries--) return hwnd;

        ret = DestroyWindow( hwnd );
        ok_(file, line)( ret, "DestroyWindow failed, error %lu\n", GetLastError() );
        flush_events();
    }
}

static void test_ObserverStatics(void)
{
    static const WCHAR *observer_statics_name = L"Windows.Perception.Spatial.Surfaces.SpatialSurfaceObserver";
    ISpatialSurfaceObserverStatics2 *observer_statics2;
    IActivationFactory *factory;
    BOOLEAN value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( observer_statics_name, wcslen( observer_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( observer_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_ISpatialSurfaceObserverStatics, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISpatialSurfaceObserverStatics2, (void **)&observer_statics2 );
    if (hr == E_NOINTERFACE) /* win1607 */
    {
        win_skip( "ISpatialSurfaceObserverStatics2 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = TRUE;
    hr = ISpatialSurfaceObserverStatics2_IsSupported( observer_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !value, "got %d.\n", value );

    ref = ISpatialSurfaceObserverStatics2_Release( observer_statics2 );
    ok( ref == 2, "got ref %ld.\n", ref );
done:
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_HolographicSpaceStatics(void)
{
    static const WCHAR *holographicspace_statics_name = L"Windows.Graphics.Holographic.HolographicSpace";
    IHolographicSpaceStatics2 *holographicspace_statics2;
    IHolographicSpaceStatics3 *holographicspace_statics3;
    IHolographicSpaceInterop *holographic_space_interop;
    IHolographicSpace *holographic_space;
    HolographicAdapterId adapter_id;
    IActivationFactory *factory;
    BOOLEAN value;
    HWND window;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( holographicspace_statics_name, wcslen( holographicspace_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( holographicspace_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IHolographicSpaceInterop, TRUE /* broken on Testbot Win1607 */ );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHolographicSpaceStatics2, (void **)&holographicspace_statics2 );
    if (hr == E_NOINTERFACE) /* win1607 */
    {
        win_skip( "IHolographicSpaceStatics2 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = 2;
    hr = IHolographicSpaceStatics2_get_IsSupported( holographicspace_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( value == TRUE, "got %d.\n", value );

    value = 2;
    hr = IHolographicSpaceStatics2_get_IsAvailable( holographicspace_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got %d.\n", value );

    ref = IHolographicSpaceStatics2_Release( holographicspace_statics2 );
    ok( ref == 2, "got ref %ld.\n", ref );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHolographicSpaceStatics3, (void **)&holographicspace_statics3 );
    if (hr == E_NOINTERFACE) /* win1703 */
    {
        win_skip( "IHolographicSpaceStatics3 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = 2;
    hr = IHolographicSpaceStatics3_get_IsConfigured( holographicspace_statics3, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got %d.\n", value );

    ref = IHolographicSpaceStatics3_Release( holographicspace_statics3 );
    ok( ref == 2, "got ref %ld.\n", ref );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHolographicSpaceInterop, (void **)&holographic_space_interop );
    if (hr == E_NOINTERFACE) /* win1703 */
    {
        win_skip( "IHolographicSpaceInterop is not supported, skipping tests.\n" );
        goto done;
    }
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    holographic_space = (void *)0xdeadbeef;
    hr = IHolographicSpaceInterop_CreateForWindow( holographic_space_interop, NULL, &IID_IHolographicSpaceInterop, (void **)&holographic_space );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    todo_wine
    ok( holographic_space == (void *)0xdeadbeef, "got holographic_space %p.\n", holographic_space );
    hr = IHolographicSpaceInterop_CreateForWindow( holographic_space_interop, (void *)0xdeadbeef, &IID_IHolographicSpaceInterop, (void **)&holographic_space );
    ok( IsWindowVisible( (void *)0xdeadbeef ) == FALSE, "got IsWindowVisible %d\n", IsWindowVisible( (void *)0xdeadbeef ) );
    ok( hr == E_INVALIDARG || broken(hr == E_ACCESSDENIED) /* w1064v1709 */ || broken(hr == E_NOINTERFACE) /* w11 */, "got hr %#lx.\n", hr );
    ok( holographic_space == NULL, "got holographic_space %p.\n", holographic_space );
    if (hr == E_ACCESSDENIED) goto cleanup;

    window = create_foreground_window( FALSE );
    ok( IsWindowVisible( window ) == TRUE, "got IsWindowVisible %d\n", IsWindowVisible( window ) );

    holographic_space = (void *)0xdeadbeef;
    hr = IHolographicSpaceInterop_CreateForWindow( holographic_space_interop, window, &IID_IHolographicSpace, (void **)&holographic_space );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( holographic_space == NULL, "got holographic_space %p.\n", holographic_space );

    ShowWindow( window, SW_HIDE );
    ok( IsWindowVisible( window ) == FALSE, "got IsWindowVisible %d\n", IsWindowVisible( window ) );
    hr = IHolographicSpaceInterop_CreateForWindow( holographic_space_interop, window, &IID_IHolographicSpace, (void **)&holographic_space );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( IsWindowVisible( window ) == FALSE, "got IsWindowVisible %d\n", IsWindowVisible( window ) );

    check_interface( holographic_space, &IID_IUnknown, FALSE );
    check_interface( holographic_space, &IID_IInspectable, FALSE );
    check_interface( holographic_space, &IID_IAgileObject, FALSE );

    hr = IHolographicSpace_get_PrimaryAdapterId( holographic_space, &adapter_id );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( adapter_id.LowPart == 0, "got adapter_id.LowPart %u.\n", adapter_id.LowPart );
    ok( adapter_id.HighPart == 0, "got adapter_id.HighPart %u.\n", adapter_id.HighPart );

    hr = IHolographicSpace_SetDirect3D11Device( holographic_space, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    ref = IHolographicSpace_Release( holographic_space );
    ok( ref == 0, "got ref %ld.\n", ref );

    hr = IHolographicSpaceInterop_CreateForWindow( holographic_space_interop, window, &IID_IHolographicSpace, (void **)&holographic_space );
    todo_wine
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr)) IHolographicSpace_Release( holographic_space );

    DestroyWindow( window );
cleanup:
    ref = IHolographicSpaceInterop_Release( holographic_space_interop );
    ok( ref == 2, "got ref %ld.\n", ref );
done:
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

struct status_async_handler
{
    IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus_iface;
    LONG refcount;

    IAsyncOperation_SpatialPerceptionAccessStatus *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct status_async_handler *impl_from_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus(
        IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *iface )
{
    return CONTAINING_RECORD( iface, struct status_async_handler, IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus_iface );
}

static HRESULT WINAPI status_async_handler_QueryInterface( IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *iface,
        REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI status_async_handler_AddRef( IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *iface )
{
    struct status_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus( iface );
    return InterlockedIncrement( &impl->refcount );
}

static ULONG WINAPI status_async_handler_Release( IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *iface )
{
    struct status_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus( iface );
    ULONG ref = InterlockedDecrement( &impl->refcount );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI status_async_handler_Invoke( IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *iface,
                                                 IAsyncOperation_SpatialPerceptionAccessStatus *async, AsyncStatus status )
{
    struct status_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus( iface );

    if (winetest_debug > 1) trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatusVtbl status_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    status_async_handler_QueryInterface,
    status_async_handler_AddRef,
    status_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<boolean> methods ***/
    status_async_handler_Invoke,
};

static IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *status_async_handler_create( HANDLE event )
{
    struct status_async_handler *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;
    impl->IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus_iface.lpVtbl = &status_async_handler_vtbl;
    impl->event = event;
    impl->refcount = 1;

    return &impl->IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus_iface;
}

static void test_SpatialAnchorExporter(void)
{
    static const WCHAR *class_name = L"Windows.Perception.Spatial.SpatialAnchorExporter";
    IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus *handler;
    IAsyncOperation_SpatialPerceptionAccessStatus *access_status;
    struct status_async_handler *handler_impl;
    ISpatialAnchorExporterStatics *statics;
    SpatialPerceptionAccessStatus status;
    IActivationFactory *factory;
    AsyncStatus astatus;
    IAsyncInfo *info;
    HANDLE event;
    HSTRING str;
    HRESULT hr;
    UINT32 id;
    LONG ref;

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_ISpatialAnchorExporterStatics, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISpatialAnchorExporterStatics, (void **)&statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = ISpatialAnchorExporterStatics_RequestAccessAsync( statics, &access_status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    handler = (void *)0xdeadbeef;
    hr = IAsyncOperation_SpatialPerceptionAccessStatus_get_Completed( access_status, &handler );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !handler, "got %p.\n", handler );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    handler = status_async_handler_create( event );
    hr = IAsyncOperation_SpatialPerceptionAccessStatus_put_Completed( access_status, handler );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    handler_impl = impl_from_IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus( handler );
    if (!handler_impl->invoked)
    {
        /* Looks like it normally completes at once on Win11, while on some Win10 Testbot machines it doesn't. */
        WaitForSingleObject( event, INFINITE );
    }
    CloseHandle( event );
    ok( handler_impl->invoked, "handler not invoked.\n" );
    ok( handler_impl->async == access_status, "got %p, %p.\n", handler_impl->async, access_status );
    ok( handler_impl->status == Completed, "got %d.\n", handler_impl->status );
    IAsyncOperationCompletedHandler_SpatialPerceptionAccessStatus_Release( handler );

    hr = IAsyncOperation_SpatialPerceptionAccessStatus_GetResults( access_status, &status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( status == SpatialPerceptionAccessStatus_DeniedBySystem, "got %d.\n", status );

    hr = IAsyncOperation_SpatialPerceptionAccessStatus_QueryInterface( access_status, &IID_IAsyncInfo, (void **)&info );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IAsyncInfo_get_Id( info, &id );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( id == 1, "got %u.\n", id );
    hr = IAsyncInfo_get_Status( info, &astatus );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( astatus == Completed, "got %u.\n", id );
    IAsyncInfo_Release( info );

    IAsyncOperation_SpatialPerceptionAccessStatus_Release( access_status );

    ISpatialAnchorExporterStatics_Release( statics );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(perception)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_ObserverStatics();
    test_HolographicSpaceStatics();
    test_SpatialAnchorExporter();

    RoUninitialize();
}
