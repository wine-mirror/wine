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
#include "windows.foundation.h"
#define WIDL_using_Windows_Graphics_Capture
#include "windows.graphics.capture.h"

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

static void test_GraphicsCaptureSessionStatics(void)
{
    static const WCHAR *session_statics_name = L"Windows.Graphics.Capture.GraphicsCaptureSession";
    IGraphicsCaptureSessionStatics *session_statics;
    IActivationFactory *factory;
    BOOLEAN res;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( session_statics_name, wcslen( session_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( session_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IGraphicsCaptureSessionStatics, (void **)&session_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    res = 2;
    hr = IGraphicsCaptureSessionStatics_IsSupported( session_statics, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( res == TRUE, "got %d.\n", res );

    ref = IGraphicsCaptureSessionStatics_Release( session_statics );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 0, "got ref %ld.\n", ref );
}

START_TEST(graphicscapture)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_GraphicsCaptureSessionStatics();

    RoUninitialize();
}
