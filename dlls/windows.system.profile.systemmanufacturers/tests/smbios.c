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

#define WIDL_using_Windows_System_Profile_SystemManufacturers
#include "windows.system.profile.systemmanufacturers.h"

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

static void test_Smbios_Statics(void)
{
    static const WCHAR *smbios_statics_name = L"Windows.System.Profile.SystemManufacturers.SmbiosInformation";
    ISmbiosInformationStatics *smbios_statics;
    IActivationFactory *factory;
    HSTRING str, serial;
    const WCHAR *buf;
    HRESULT hr;
    UINT32 len;
    LONG ref;

    hr = WindowsCreateString( smbios_statics_name, wcslen( smbios_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( smbios_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISmbiosInformationStatics, (void **)&smbios_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    if (0) /* Win8 Crash */
    {
        hr = ISmbiosInformationStatics_get_SerialNumber( smbios_statics, &serial );
        ok( hr == S_OK || broken(hr == E_UNEXPECTED), "got hr %#lx.\n", hr );
        if (hr == S_OK)
        {
            buf = WindowsGetStringRawBuffer( serial, &len );
            ok( buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len );
            WindowsDeleteString( serial );
        }
    }

    ref = ISmbiosInformationStatics_Release( smbios_statics );
    ok( ref == 2, "got ref %ld.\n", ref );

    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(smbios)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_Smbios_Statics();

    RoUninitialize();
}
