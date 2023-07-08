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
#define WIDL_using_Windows_Networking
#include "windows.networking.h"

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

static void test_HostnameStatics(void)
{
    static const WCHAR *hostname_statics_name = L"Windows.Networking.HostName";
    static const WCHAR *ip = L"192.168.0.0";
    IHostNameFactory *hostnamefactory;
    IActivationFactory *factory;
    HSTRING_HEADER header;
    HSTRING str, rawname;
    IHostName *hostname;
    HRESULT hr;
    INT32 res;
    LONG ref;

    hr = WindowsCreateString( hostname_statics_name, wcslen( hostname_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( hostname_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHostNameFactory, (void **)&hostnamefactory );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCreateStringReference( ip, wcslen( ip ), &header, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IHostNameFactory_CreateHostName( hostnamefactory, NULL, &hostname );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IHostNameFactory_CreateHostName( hostnamefactory, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IHostNameFactory_CreateHostName( hostnamefactory, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IHostNameFactory_CreateHostName( hostnamefactory, str, &hostname );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( hostname != NULL, "got NULL hostname %p.\n", hostname );

    check_interface( hostname, &IID_IUnknown );
    check_interface( hostname, &IID_IInspectable );
    check_interface( hostname, &IID_IAgileObject );
    check_interface( hostname, &IID_IHostName );

    hr = IHostName_get_RawName( hostname, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IHostName_get_RawName( hostname, &rawname );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, rawname, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got unexpected string %s.\n", debugstr_hstring(rawname) );
    ok( str != rawname, "got same HSTRINGs %p, %p.\n", str, rawname );

    WindowsDeleteString( str );
    WindowsDeleteString( rawname );
    ref = IHostName_Release( hostname );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IHostNameFactory_Release( hostnamefactory );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(hostname)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_HostnameStatics();

    RoUninitialize();
}
