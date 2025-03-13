/*
 * Copyright (C) 2025 Mohamad Al-Jaf
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

#define WIDL_using_Windows_System_Profile
#include "windows.system.profile.h"

#include "wine/test.h"

#define check_interface( obj, iid, supported ) check_interface_( __LINE__, obj, iid, supported )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL supported )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || (!supported && hr == E_NOINTERFACE), "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_SystemIdentification_Statics(void)
{
    static const WCHAR *system_id_statics_name = L"Windows.System.Profile.SystemIdentification";
    ISystemIdentificationStatics *system_id_statics = (void *)0xdeadbeef;
    ISystemIdentificationInfo *system_id_info = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    HSTRING str = NULL;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( system_id_statics_name, wcslen( system_id_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( system_id_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISystemIdentificationStatics, (void **)&system_id_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = ISystemIdentificationStatics_GetSystemIdForPublisher( system_id_statics, NULL );
    todo_wine
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = ISystemIdentificationStatics_GetSystemIdForPublisher( system_id_statics, &system_id_info );
    todo_wine
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
    {
        check_interface( system_id_info, &IID_IAgileObject, FALSE );

        ref = ISystemIdentificationInfo_Release( system_id_info );
        ok( ref == 0, "got ref %ld.\n", ref );
    }

    ref = ISystemIdentificationStatics_Release( system_id_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(systemid)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_SystemIdentification_Statics();

    RoUninitialize();
}
