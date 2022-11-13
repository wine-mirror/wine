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
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_ClosedCaptioning
#include "windows.media.closedcaptioning.h"

#include "wine/test.h"

#define check_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL supported )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr, expected_hr;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_( __FILE__, line )( hr == expected_hr || broken(hr == E_NOINTERFACE ), "Got hr %#lx, expected %#lx.\n", hr, expected_hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_CaptionStatics(void)
{
    static const WCHAR *caption_properties_name = L"Windows.Media.ClosedCaptioning.ClosedCaptionProperties";
    IClosedCaptionPropertiesStatics *caption_statics;
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( caption_properties_name, wcslen( caption_properties_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( caption_properties_name ));
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IClosedCaptionPropertiesStatics, (void **)&caption_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    ref = IClosedCaptionPropertiesStatics_Release( caption_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(captions)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_CaptionStatics();

    RoUninitialize();
}
