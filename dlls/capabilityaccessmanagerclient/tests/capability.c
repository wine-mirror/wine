/* Copyright 2026 Paul Gofman for CodeWeavers
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
#define WIDL_using_Windows_Security_Authorization_AppCapabilityAccess
#include "windows.security.authorization.appcapabilityaccess.h"

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

static void test_AppCapabilityStatics(void)
{
    static const WCHAR *statics_name = L"Windows.Security.Authorization.AppCapabilityAccess.AppCapability";
    static const WCHAR *nonexistent_cap = L"nonexistent";
    AppCapabilityAccessStatus access_status;
    IAppCapabilityStatics *statics;
    IActivationFactory *factory;
    IAppCapability *cap;
    HSTRING str;
    HRESULT hr;

    hr = WindowsCreateString( statics_name, wcslen( statics_name ), &str );
    ok( hr == S_OK, "got %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "class %s is not registered, skipping tests.\n", wine_dbgstr_w( statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IAppCapabilityStatics, (void **)&statics );
    ok( hr == S_OK, "got %#lx.\n", hr );

    cap = (void *)0xdeadbeef;
    hr = WindowsCreateString( nonexistent_cap, wcslen( nonexistent_cap ), &str );
    hr = IAppCapabilityStatics_Create( statics, str, &cap );
    ok( hr == S_OK, "got %#lx.\n", hr );
    WindowsDeleteString( str );

    check_interface( cap, &IID_IUnknown );
    check_interface( cap, &IID_IInspectable );
    check_interface( cap, &IID_IAgileObject );
    check_interface( cap, &IID_IAppCapability );

    access_status = 0xdeadbeef;
    hr = IAppCapability_get_CapabilityName( cap, &str );
    ok( hr == S_OK, "got %#lx.\n", hr );
    ok( !wcscmp( WindowsGetStringRawBuffer( str, NULL ), nonexistent_cap ), "got %s.\n", debugstr_hstring(str) );
    WindowsDeleteString( str );
    hr = IAppCapability_CheckAccess( cap, &access_status );
    ok( hr == S_OK, "got %#lx.\n", hr );
    ok( access_status == AppCapabilityAccessStatus_Allowed, "got %d.\n", access_status );
    IAppCapability_Release( cap );

    IAppCapabilityStatics_Release( statics );
}

START_TEST(capability)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_AppCapabilityStatics();

    RoUninitialize();
}
