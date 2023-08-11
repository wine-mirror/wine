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
#define WIDL_using_Windows_ApplicationModel
#define WIDL_using_Windows_ApplicationModel_Core
#define WIDL_using_Windows_Management_Deployment
#define WIDL_using_Windows_Storage
#include "windows.applicationmodel.h"
#include "windows.management.deployment.h"

#include "wine/test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || broken( hr == E_NOINTERFACE ) , "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_PackageManager(void)
{
    static const WCHAR *statics_name = RuntimeClass_Windows_Management_Deployment_PackageManager;
    IStorageFolder *storage_folder;
    IStorageItem *storage_item;
    IActivationFactory *factory;
    IIterable_Package *packages;
    IPackageManager *manager;
    IIterator_Package *iter;
    IPackage *package;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( statics_name, wcslen( statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    todo_wine
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        todo_wine
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_ActivateInstance( factory, (IInspectable **)&manager );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( manager, &IID_IUnknown );
    check_interface( manager, &IID_IInspectable );
    check_interface( manager, &IID_IAgileObject );
    check_interface( manager, &IID_IPackageManager );

    hr = IPackageManager_FindPackages( manager, &packages );
    ok( hr == S_OK || broken(hr == E_ACCESSDENIED) /* w8adm */, "got hr %#lx.\n", hr );
    if (broken(hr == E_ACCESSDENIED))
    {
        win_skip("Unable to list packages, skipping package manager tests\n");
        goto skip_tests;
    }

    hr = IIterable_Package_First( packages, &iter );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IIterator_Package_get_Current( iter, &package );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IPackage_get_InstalledLocation( package, &storage_folder );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IStorageFolder_QueryInterface( storage_folder, &IID_IStorageItem, (void **)&storage_item );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IStorageItem_get_Path( storage_item, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    ref = IStorageItem_Release( storage_item );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IStorageFolder_Release( storage_folder );
    ok( !ref, "got ref %ld.\n", ref );
    IPackage_Release( package );
    ref = IIterator_Package_Release( iter );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IIterable_Package_Release( packages );
    ok( !ref, "got ref %ld.\n", ref );

skip_tests:
    ref = IPackageManager_Release( manager );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_PackageStatics(void)
{
    static const WCHAR *package_statics_name = L"Windows.ApplicationModel.Package";
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( package_statics_name, wcslen( package_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( package_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(model)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_PackageManager();
    test_PackageStatics();

    RoUninitialize();
}
