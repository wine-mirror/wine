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
#include "winrt_test.h"

#define DEFINE_ASYNC_COMPLETED_HANDLER( name, iface_type, async_type )                              \
    struct name                                                                                     \
    {                                                                                               \
        iface_type iface_type##_iface;                                                              \
        LONG refcount;                                                                              \
        BOOL invoked;                                                                               \
        HANDLE event;                                                                               \
    };                                                                                              \
                                                                                                    \
    static HRESULT WINAPI name##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                               \
        if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject ) ||           \
            IsEqualGUID( iid, &IID_##iface_type ))                                                  \
        {                                                                                           \
            IUnknown_AddRef( iface );                                                               \
            *out = iface;                                                                           \
            return S_OK;                                                                            \
        }                                                                                           \
                                                                                                    \
        trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );            \
        *out = NULL;                                                                                \
        return E_NOINTERFACE;                                                                       \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_AddRef( iface_type *iface )                                          \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        return InterlockedIncrement( &impl->refcount );                                             \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_Release( iface_type *iface )                                         \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ULONG ref = InterlockedDecrement( &impl->refcount );                                        \
        if (!ref) free( impl );                                                                     \
        return ref;                                                                                 \
    }                                                                                               \
                                                                                                    \
    static HRESULT WINAPI name##_Invoke( iface_type *iface, async_type *async, AsyncStatus status ) \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ok( !impl->invoked, "invoked twice\n" );                                                    \
        impl->invoked = TRUE;                                                                       \
        if (impl->event) SetEvent( impl->event );                                                   \
        return S_OK;                                                                                \
    }                                                                                               \
                                                                                                    \
    static iface_type##Vtbl name##_vtbl =                                                           \
    {                                                                                               \
        name##_QueryInterface,                                                                      \
        name##_AddRef,                                                                              \
        name##_Release,                                                                             \
        name##_Invoke,                                                                              \
    };                                                                                              \
                                                                                                    \
    static iface_type *name##_create( HANDLE event )                                                \
    {                                                                                               \
        struct name *impl;                                                                          \
                                                                                                    \
        if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;                                      \
        impl->iface_type##_iface.lpVtbl = &name##_vtbl;                                             \
        impl->event = event;                                                                        \
        impl->refcount = 1;                                                                         \
                                                                                                    \
        return &impl->iface_type##_iface;                                                           \
    }                                                                                               \
                                                                                                    \
    static DWORD await_##async_type( async_type *async, DWORD timeout )                             \
    {                                                                                               \
        iface_type *handler;                                                                        \
        HANDLE event;                                                                               \
        HRESULT hr;                                                                                 \
        DWORD ret;                                                                                  \
                                                                                                    \
        event = CreateEventW( NULL, FALSE, FALSE, NULL );                                           \
        ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );                          \
        handler = name##_create( event );                                                           \
        ok( !!handler, "Failed to create completion handler\n" );                                   \
        hr = async_type##_put_Completed( async, handler );                                          \
        ok( hr == S_OK, "put_Completed returned %#lx\n", hr );                                      \
        ret = WaitForSingleObject( event, timeout );                                                \
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );                                     \
        CloseHandle( event );                                                                       \
        iface_type##_Release( handler );                                                            \
                                                                                                    \
        return ret;                                                                                 \
    }

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

static void load_resource( const WCHAR *name, const WCHAR *type, const WCHAR *filename )
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", debugstr_w(filename), GetLastError() );

    res = FindResourceW( NULL, name, type );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleW( NULL ), res ) );
    WriteFile( file, ptr, SizeofResource( GetModuleHandleW( NULL ), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleW( NULL ), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

#define check_async_info( a, b, c ) check_async_info_( __LINE__, a, b, c )
static void check_async_info_( int line, void *async, AsyncStatus expect_status, HRESULT expect_hr )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IInspectable_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( !!async_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) todo_wine_if(FAILED(expect_hr)) ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );
}

DEFINE_ASYNC_COMPLETED_HANDLER( async_deployment_result_handler, IAsyncOperationWithProgressCompletedHandler_DeploymentResult_DeploymentProgress,
                                IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress )
DEFINE_ASYNC_COMPLETED_HANDLER( async_applist_handler, IAsyncOperationCompletedHandler_IVectorView_AppListEntry,
                                IAsyncOperation_IVectorView_AppListEntry )
DEFINE_ASYNC_COMPLETED_HANDLER( async_boolean_handler, IAsyncOperationCompletedHandler_boolean, IAsyncOperation_boolean )

static HRESULT uri_create( const WCHAR *uri, IUriRuntimeClass **out )
{
    static const WCHAR *statics_name = RuntimeClass_Windows_Foundation_Uri;
    IUriRuntimeClassFactory *uri_factory;
    IActivationFactory *activation;
    HSTRING str;
    HRESULT hr;

    hr = WindowsCreateString( statics_name, wcslen( statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&activation );
    WindowsDeleteString( str );

    hr = IActivationFactory_QueryInterface( activation, &IID_IUriRuntimeClassFactory, (void **)&uri_factory );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IActivationFactory_Release( activation );

    hr = WindowsCreateString( uri, wcslen( uri ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IUriRuntimeClassFactory_CreateUri( uri_factory, str, out );
    WindowsDeleteString( str );

    IUriRuntimeClassFactory_Release( uri_factory );
    return hr;
}

static HRESULT test_register_package( IPackageManager *manager, IPackage **out )
{
    static const WCHAR app_model_unlock[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock";
    IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress *operation;
    DWORD size, old_value = 0, new_value = 1;
    WCHAR temp[MAX_PATH], path[MAX_PATH + 7];
    IStorageFolder *storage_folder;
    HSTRING str, name, publisher;
    IIterable_Package *packages;
    IStorageItem *storage_item;
    IDeploymentResult *result;
    IIterator_Package *iter;
    IUriRuntimeClass *uri;
    HRESULT hr, async_hr;
    IPackage *package;
    const WCHAR *buf;
    UINT res, len;
    HKEY hkey;

    GetTempPathW( ARRAY_SIZE(temp), temp );
    GetTempFileNameW( temp, L"winetest-winrt", 0, temp );
    DeleteFileW( temp );
    CreateDirectoryW( temp, NULL );

    swprintf( path, MAX_PATH, L"%s\\%s", temp, L"application.exe" );
    load_resource( L"application.exe", L"TESTDLL", path );
    swprintf( path, MAX_PATH, L"%s\\%s", temp, L"appxmanifest.xml" );
    load_resource( L"appxmanifest.xml", (const WCHAR *)RT_RCDATA, path );

    swprintf( path, MAX_PATH, L"file://%s\\%s", temp, L"appxmanifest.xml" );
    hr = uri_create( path, &uri );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    res = RegGetValueW( HKEY_LOCAL_MACHINE, app_model_unlock, L"AllowDevelopmentWithoutDevLicense", RRF_RT_DWORD, NULL,
                        (BYTE *)&old_value, &size );
    if (res) old_value = 0xdeadbeef;

    res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, app_model_unlock, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL );
    ok( !res, "RegCreateKeyExW failed, res %#x\n", res );
    res = RegSetValueExW( hkey, L"AllowDevelopmentWithoutDevLicense", 0, REG_DWORD, (const BYTE *)&new_value, sizeof(new_value) );
    ok( !res, "RegCreateKeyExW failed, res %#x\n", res );

    /* DeploymentOptions_DevelopmentMode is invalid for AddPackageAsync */
    hr = IPackageManager_AddPackageAsync( manager, uri, NULL, DeploymentOptions_DevelopmentMode, &operation );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    hr = IPackageManager_RegisterPackageAsync( manager, uri, NULL, DeploymentOptions_DevelopmentMode, &operation );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( operation != NULL, "got operation %p\n", operation );
    IUriRuntimeClass_Release( uri );

    res = await_IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress( operation, 5000 );
    ok( res == 0, "await_IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress returned %#x\n", res );

    hr = IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress_GetResults( operation, &result );
    ok( hr == S_OK, "GetResults returned %#lx\n", hr );
    IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress_Release( operation );

    if (old_value != 0xdeadbeef)
    {
        res = RegSetValueExW( hkey, L"AllowDevelopmentWithoutDevLicense", 0, REG_DWORD, (const BYTE *)&old_value, sizeof(old_value) );
        ok( !res, "RegCreateKeyExW failed, res %#x\n", res );
    }

    RegCloseKey( hkey );

    hr = IDeploymentResult_get_ExtendedErrorCode( result, &async_hr );
    ok( hr == S_OK, "get_ExtendedErrorCode returned %#lx\n", hr );
    ok( async_hr == S_OK || broken( async_hr == 0x80073cff ) /* 32-bit missing license */ ||
        broken( async_hr == 0x80080204 ) /* W8 */ || broken( async_hr == 0x8004264b ) /* w1064v1507 */,
        "got error %#lx\n", async_hr );

    if (FAILED(async_hr))
    {
        win_skip("Failed to register package, skipping tests\n");
        hr = IDeploymentResult_get_ErrorText( result, &str );
        ok( hr == S_OK, "get_ExtendedErrorCode returned %#lx\n", hr );
        trace( "got error %s\n", debugstr_hstring(str) );
        IDeploymentResult_Release( result );
        return async_hr;
    }

    IDeploymentResult_Release( result );

    hr = WindowsCreateString( L"WineTest", wcslen( L"WineTest" ), &name );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( L"CN=Wine, O=The Wine Project, C=US", wcslen( L"CN=Wine, O=The Wine Project, C=US" ), &publisher );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IPackageManager_FindPackagesByNamePublisher( manager, name, publisher, &packages );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( name );
    WindowsDeleteString( publisher );

    hr = IIterable_Package_First( packages, &iter );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IIterable_Package_Release( packages );

    hr = IIterator_Package_get_Current( iter, &package );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IIterator_Package_Release( iter );

    hr = IPackage_get_InstalledLocation( package, &storage_folder );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IStorageFolder_QueryInterface( storage_folder, &IID_IStorageItem, (void **)&storage_item );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IStorageFolder_Release( storage_folder );

    hr = IStorageItem_get_Path( storage_item, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IStorageItem_Release( storage_item );

    buf = WindowsGetStringRawBuffer( str, &len );
    ok( !wcscmp( temp, buf ), "got path %s.\n", debugstr_hstring( str ) );
    WindowsDeleteString( str );

    *out = package;
    return S_OK;
}

static void test_remove_package( IPackageManager *manager, IPackage *package )
{
    IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress *operation;
    IDeploymentResult *result;
    HRESULT hr, async_hr;
    IPackageId *id;
    HSTRING name;
    UINT res;

    hr = IPackage_get_Id( package, &id );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IPackageId_get_FullName( id, &name );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IPackageId_Release( id );

    hr = IPackageManager_RemovePackageAsync( manager, name, &operation );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( operation != NULL, "got operation %p\n", operation );

    res = await_IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress( operation, 5000 );
    ok( res == 0, "await_IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress returned %#x\n", res );
    check_async_info( operation, Completed, S_OK );

    hr = IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress_GetResults( operation, &result );
    ok( hr == S_OK, "GetResults returned %#lx\n", hr );
    hr = IDeploymentResult_get_ExtendedErrorCode( result, &async_hr );
    ok( hr == S_OK, "get_ExtendedErrorCode returned %#lx\n", hr );
    ok( async_hr == S_OK, "got error %#lx\n", async_hr );
    IDeploymentResult_Release( result );

    IAsyncOperationWithProgress_DeploymentResult_DeploymentProgress_Release( operation );
}

static void test_execute_package( IPackage *package )
{
    IAsyncOperation_IVectorView_AppListEntry *async_list;
    IAsyncOperation_boolean *async_launch;
    IVectorView_AppListEntry *list;
    IAppListEntry *app_entry;
    IPackage3 *package3;
    boolean launched;
    HRESULT hr;
    UINT res;

    if (!winrt_test_init()) return;

    hr = IPackage_QueryInterface( package, &IID_IPackage3, (void **)&package3 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IPackage3_GetAppListEntriesAsync( package3, &async_list );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res = await_IAsyncOperation_IVectorView_AppListEntry( async_list, 5000 );
    ok( res == 0, "await_IAsyncOperation_IVectorView_AppListEntry returned %#x.\n", res );
    check_async_info( async_list, Completed, S_OK );
    hr = IAsyncOperation_IVectorView_AppListEntry_GetResults( async_list, &list );
    ok( hr == S_OK, "GetResults returned %#lx\n", hr );
    IAsyncOperation_IVectorView_AppListEntry_Release( async_list );

    hr = IVectorView_AppListEntry_GetAt( list, 0, &app_entry );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IVectorView_AppListEntry_Release( list );

    IPackage3_Release( package3 );


    hr = IAppListEntry_LaunchAsync( app_entry, &async_launch );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res = await_IAsyncOperation_boolean( async_launch, 5000 );
    ok( res == 0, "await_IAsyncOperation_boolean returned %#x.\n", res );
    check_async_info( async_launch, Completed, S_OK );

    hr = IAsyncOperation_boolean_GetResults( async_launch, &launched );
    ok( hr == S_OK, "GetResults returned %#lx\n", hr );
    ok( launched == TRUE, "got launched %u.\n", launched );

    IAppListEntry_Release( app_entry );

    winrt_test_exit();
}

static void test_PackageManager(void)
{
    static const WCHAR *statics_name = RuntimeClass_Windows_Management_Deployment_PackageManager;
    IStorageFolder *storage_folder;
    IStorageItem *storage_item;
    IActivationFactory *factory;
    IIterable_Package *packages;
    IInspectable *inspectable;
    IPackageManager2 *manager2;
    IPackageManager *manager;
    IIterator_Package *iter;
    IPackage *package;
    HSTRING str, str2;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( statics_name, wcslen( statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );

    hr = IActivationFactory_ActivateInstance( factory, &inspectable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( inspectable, &IID_IUnknown );
    check_interface( inspectable, &IID_IInspectable );
    check_interface( inspectable, &IID_IAgileObject );

    hr = IInspectable_QueryInterface( inspectable, &IID_IPackageManager, (void **)&manager );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IPackageManager_FindPackagesByNamePublisher( manager, NULL, NULL, &packages );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( L"Wine", wcslen( L"Wine" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IPackageManager_FindPackagesByNamePublisher( manager, str, NULL, &packages );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( L"The Wine Project", wcslen( L"The Wine Project" ), &str2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IPackageManager_FindPackagesByNamePublisher( manager, NULL, str2, &packages );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IPackageManager_FindPackagesByNamePublisher( manager, str, str2, &packages );
    todo_wine
    ok( hr == S_OK || broken(hr == E_ACCESSDENIED) /* Requires admin privileges */, "got hr %#lx.\n", hr );
    if (hr == S_OK) IIterable_Package_Release( packages );
    WindowsDeleteString( str );
    WindowsDeleteString( str2 );

    hr = IPackageManager_QueryInterface( manager, &IID_IPackageManager2, (void **)&manager2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IPackageManager2_FindPackagesWithPackageTypes( manager2, 1, &packages );
    todo_wine
    ok( hr == S_OK || broken(hr == E_ACCESSDENIED) /* Requires admin privileges */, "got hr %#lx.\n", hr );
    if (hr == S_OK) IIterable_Package_Release( packages );

    ref = IPackageManager2_Release( manager2 );
    ok( ref == 2, "got ref %ld.\n", ref );

    hr = IPackageManager_FindPackages( manager, &packages );
    todo_wine
    ok( hr == S_OK || broken(hr == E_ACCESSDENIED) /* w8adm */, "got hr %#lx.\n", hr );
    if (hr != S_OK)
    {
        todo_wine
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

    if (SUCCEEDED(test_register_package( manager, &package )))
    {
        test_execute_package( package );
        test_remove_package( manager, package );
        IPackage_Release( package );
    }

skip_tests:
    ref = IPackageManager_Release( manager );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IInspectable_Release( inspectable );
    ok( !ref, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_PackageStatics(void)
{
    static const WCHAR *package_statics_name = L"Windows.ApplicationModel.Package";
    IPackageStatics *package_statics;
    IActivationFactory *factory;
    IPackage *package;
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

    hr = IActivationFactory_QueryInterface( factory, &IID_IPackageStatics, (void **)&package_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IPackageStatics_get_Current( package_statics, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IPackageStatics_get_Current( package_statics, &package );
    todo_wine ok( hr == 0x80073d54, "got hr %#lx.\n", hr );
    todo_wine ok( !package, "got package %p.\n", package );
    if (package) IPackage_Release( package );

    ref = IPackageStatics_Release( package_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
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
