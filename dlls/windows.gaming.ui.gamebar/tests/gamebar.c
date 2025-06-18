/*
 * Copyright 2022 Paul Gofman for CodeWeavers
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
#include "winerror.h"
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Gaming_UI
#include "windows.foundation.h"
#include "windows.gaming.ui.h"

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

static void test_GameBarStatics(void)
{
    static const WCHAR *gamebar_statics_name = L"Windows.Gaming.UI.GameBar";
    IGameBarStatics *gamebar_statics;
    IActivationFactory *factory;
    BOOLEAN value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( gamebar_statics_name, wcslen( gamebar_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( gamebar_statics_name ));
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IGameBarStatics, (void **)&gamebar_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IGameBarStatics_get_Visible( gamebar_statics, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );

    value = TRUE;
    hr = IGameBarStatics_get_Visible( gamebar_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !value, "got %d.\n", value );

    hr = IGameBarStatics_get_IsInputRedirected( gamebar_statics, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );

    value = TRUE;
    hr = IGameBarStatics_get_IsInputRedirected( gamebar_statics, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !value, "got %d.\n", value );

    ref = IGameBarStatics_Release( gamebar_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(gamebar)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_GameBarStatics();

    RoUninitialize();
}
