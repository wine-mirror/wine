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
#define WIDL_using_Windows_Media
#include "windows.media.h"
#include "systemmediatransportcontrolsinterop.h"

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

static void check_interface_GetForWindow( void *media_control_interop_statics, HWND window, const IID *iid )
{
    ISystemMediaTransportControls *media_control_statics = NULL;
    HRESULT hr;

    hr = ISystemMediaTransportControlsInterop_GetForWindow( media_control_interop_statics, window, iid, (void **)&media_control_statics );
    ok( hr == S_OK || broken(hr == 0x80070578) /* Win8 */, "got hr %#lx.\n", hr );
    if (media_control_statics) ISystemMediaTransportControls_Release( media_control_statics );
}

static void test_MediaControlStatics(void)
{
    static const WCHAR *media_control_statics_name = L"Windows.Media.SystemMediaTransportControls";
    ISystemMediaTransportControlsInterop *media_control_interop_statics;
    ISystemMediaTransportControls *media_control_statics = NULL;
    IActivationFactory *factory;
    HWND window = NULL;
    BOOLEAN value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( media_control_statics_name, wcslen( media_control_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( media_control_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISystemMediaTransportControlsInterop, (void **)&media_control_interop_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = ISystemMediaTransportControlsInterop_GetForWindow( media_control_interop_statics, NULL, &IID_ISystemMediaTransportControls, (void **)&media_control_statics );
    ok( hr == E_POINTER || broken(hr == 0x80070578) /* Win8 */, "got hr %#lx.\n", hr );

    window = CreateWindowExA( 0, "static", 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, GetModuleHandleA( NULL ), 0 );
    ok( window != NULL, "Failed to create a window\n" );
    hr = ISystemMediaTransportControlsInterop_GetForWindow( media_control_interop_statics, window, &IID_ISystemMediaTransportControlsInterop, (void **)&media_control_statics );
    ok( hr == E_NOINTERFACE || broken(hr == 0x80070578) /* Win8 */, "got hr %#lx.\n", hr );

    check_interface_GetForWindow( media_control_interop_statics, window, &IID_IUnknown );
    check_interface_GetForWindow( media_control_interop_statics, window, &IID_IInspectable );
    check_interface_GetForWindow( media_control_interop_statics, window, &IID_IAgileObject );

    hr = ISystemMediaTransportControlsInterop_GetForWindow( media_control_interop_statics, window, &IID_ISystemMediaTransportControls, (void **)&media_control_statics );
    ok( hr == S_OK || broken(hr == 0x80070578) /* Win8 */, "got hr %#lx.\n", hr );
    if (!media_control_statics) goto done;

    hr = ISystemMediaTransportControls_put_PlaybackStatus( media_control_statics, -1 );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_PlaybackStatus( media_control_statics, 5 );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_PlaybackStatus( media_control_statics, MediaPlaybackStatus_Closed );
    ok( hr == S_OK || broken(hr == S_FALSE) /* Win10 1507,1607 */, "got hr %#lx.\n", hr );

    hr = ISystemMediaTransportControls_put_IsPlayEnabled( media_control_statics, FALSE );
    ok( hr == S_OK || broken(hr == S_FALSE) /* Win10 1507,1607 */, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_IsPauseEnabled( media_control_statics, FALSE );
    ok( hr == S_OK || broken(hr == S_FALSE) /* Win10 1507,1607 */, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_IsPreviousEnabled( media_control_statics, FALSE );
    ok( hr == S_OK || broken(hr == S_FALSE) /* Win10 1507,1607 */, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_IsNextEnabled( media_control_statics, FALSE );
    ok( hr == S_OK || broken(hr == S_FALSE) /* Win10 1507,1607 */, "got hr %#lx.\n", hr );
    hr = ISystemMediaTransportControls_put_IsEnabled( media_control_statics, FALSE );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = TRUE;
    hr = ISystemMediaTransportControls_get_IsPlayEnabled( media_control_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got value %d.\n", value );

    value = TRUE;
    hr = ISystemMediaTransportControls_get_IsPauseEnabled( media_control_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got value %d.\n", value );

    value = TRUE;
    hr = ISystemMediaTransportControls_get_IsPreviousEnabled( media_control_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got value %d.\n", value );

    value = TRUE;
    hr = ISystemMediaTransportControls_get_IsNextEnabled( media_control_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got value %d.\n", value );

    value = TRUE;
    hr = ISystemMediaTransportControls_get_IsEnabled( media_control_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got value %d.\n", value );

    ref = ISystemMediaTransportControls_Release( media_control_statics );
    ok( ref == 1 || broken(ref == 3) /* Win10 1507 */ || broken(ref == 2) /* Win10 1607 */, "got ref %ld.\n", ref );
done:
    DestroyWindow( window );
    ref = ISystemMediaTransportControlsInterop_Release( media_control_interop_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(mediacontrol)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_MediaControlStatics();

    RoUninitialize();
}
