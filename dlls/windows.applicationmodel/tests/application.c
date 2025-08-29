/*
 * Copyright (C) 2023 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep testdll
#endif

#define COBJMACROS
#include <stddef.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "winstring.h"
#include "shlwapi.h"

#include "roapi.h"

#include "inspectable.h"
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_ApplicationModel
#define WIDL_using_Windows_Storage
#include "windows.applicationmodel.h"

#define WINE_WINRT_TEST
#include "winrt_test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_ApplicationDataStatics(void)
{
    static const WCHAR *application_data_statics_name = L"Windows.Storage.ApplicationData";
    IApplicationDataStatics *application_data_statics;
    IApplicationData *application_data;
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( application_data_statics_name, wcslen( application_data_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( application_data_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IApplicationDataStatics, (void **)&application_data_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IApplicationDataStatics_get_Current( application_data_statics, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IApplicationDataStatics_get_Current( application_data_statics, &application_data );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( application_data != NULL, "got NULL application_data %p.\n", application_data );

    ref = IApplicationData_Release( application_data );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IApplicationDataStatics_Release( application_data_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_PackageStatics(void)
{
    static const WCHAR *package_statics_name = L"Windows.ApplicationModel.Package";
    IPackageStatics *package_statics;
    IStorageFolder *storage_folder;
    IActivationFactory *factory;
    IStorageItem *storage_item;
    WCHAR buffer[MAX_PATH];
    HSTRING str, wine_str;
    IPackage *package;
    HRESULT hr;
    INT32 res;
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

    hr = IActivationFactory_QueryInterface( factory, &IID_IPackageStatics, (void **)&package_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IPackageStatics_get_Current( package_statics, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IPackageStatics_get_Current( package_statics, &package );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( package != NULL, "got NULL package %p.\n", package );

    check_interface( package, &IID_IUnknown );
    check_interface( package, &IID_IInspectable );
    check_interface( package, &IID_IAgileObject );
    check_interface( package, &IID_IPackage );

    hr = IPackage_get_InstalledLocation( package, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IPackage_get_InstalledLocation( package, &storage_folder );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( storage_folder, &IID_IUnknown );
    check_interface( storage_folder, &IID_IInspectable );
    check_interface( storage_folder, &IID_IStorageFolder );

    hr = IStorageFolder_QueryInterface( storage_folder, &IID_IStorageItem, (void **)&storage_item );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IStorageItem_get_Path( storage_item, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    GetModuleFileNameW( NULL, buffer, MAX_PATH );
    PathRemoveFileSpecW( buffer );
    hr = WindowsCreateString( buffer, wcslen(buffer), &wine_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, wine_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got string %s.\n", debugstr_hstring(str) );
    WindowsDeleteString( str );
    WindowsDeleteString( wine_str );

    ref = IStorageItem_Release( storage_item );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IStorageFolder_Release( storage_folder );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IPackage_Release( package );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IPackageStatics_Release( package_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}


static HRESULT WINAPI test_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ) ||
        IsEqualGUID( iid, &IID_IInspectable ))
    {
        IActivationFactory_AddRef(( *out = iface ));
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_factory_AddRef( IActivationFactory *iface )
{
    return 2;
}

static ULONG WINAPI test_factory_Release( IActivationFactory *iface )
{
    return 1;
}

static HRESULT WINAPI test_factory_GetIids( IActivationFactory *iface, ULONG *count, GUID **iids )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI test_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *name )
{
    static const WCHAR *nameW = L"Wine.Application.Class";
    return WindowsCreateString( nameW, wcslen( nameW ), name );
}

static HRESULT WINAPI test_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    return S_OK;
}

static HRESULT WINAPI test_factory_ActivateInstance( IActivationFactory *iface, IInspectable **out )
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static const IActivationFactoryVtbl test_factory_vtbl =
{
    /* IUnknown */
    test_factory_QueryInterface,
    test_factory_AddRef,
    test_factory_Release,
    /* IInspectable */
    test_factory_GetIids,
    test_factory_GetRuntimeClassName,
    test_factory_GetTrustLevel,
    /* IActivationFactory */
    test_factory_ActivateInstance
};

static IActivationFactory test_factory = { &test_factory_vtbl };

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

   *factory = NULL;

    if (!wcscmp( buffer, L"Wine.Application.Class" ))
        return IActivationFactory_QueryInterface( &test_factory, &IID_IActivationFactory, (void **)factory );

    return CLASS_E_CLASSNOTAVAILABLE;
}

static void test_registration( void )
{
    static const WCHAR *name = L"Wine.Application.Class";
    IActivationFactory *factory;
    const WCHAR *buf;
    HSTRING str;
    HRESULT hr;
    hr = WindowsCreateString( name, wcslen( name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IActivationFactory );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_GetRuntimeClassName( factory, &str );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    buf = WindowsGetStringRawBuffer( str, NULL );
    ok( buf && !wcscmp( buf, name ), "got str %s\n", debugstr_hstring( str ) );
    WindowsDeleteString( str );

    IActivationFactory_Release( factory );
}

int main( int argc, char const *argv[] )
{
    HRESULT hr;

    if (!winrt_test_init()) return -1;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_ApplicationDataStatics();
    test_PackageStatics();

    test_registration();

    RoUninitialize();

    winrt_test_exit();
    return 0;
}
