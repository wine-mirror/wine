/*
 * Copyright 2026 Vibhav Pant for CodeWeavers
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
#include <initguid.h>

#include <propvarutil.h>
#include <propkey.h>
#include <roapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wincrypt.h>
#include <winstring.h>
#include <winternl.h>

#include <notificationactivationcallback.h>
#define WIDL_using_Windows_Foundation
#include <windows.foundation.h>
#define WIDL_using_Windows_Data_Xml_Dom
#include <windows.data.xml.dom.h>
#define WIDL_using_Windows_UI_Notifications
#include <windows.ui.notifications.h>

#include <wine/test.h>

static HRESULT (WINAPI *pSetCurrentProcessExplicitAppUserModelID)( const WCHAR * );

#define check_interface(iface, iid, exists) check_interface_( __LINE__, iface, iid, exists )
static void check_interface_( int line, void *iface, const IID *iid, BOOL exists )
{
    HRESULT hr, exp = exists ? S_OK : E_NOINTERFACE;
    IUnknown *obj = iface, *out;

    hr = IUnknown_QueryInterface( obj, iid, (void **)&out );
    ok_( __FILE__, line )( hr == exp, "got hr %#lx != %#lx\n", hr, exp );
    if (hr == S_OK) IUnknown_Release( out );
}

struct notification_callback
{
    INotificationActivationCallback iface;
    const WCHAR *aumid;
    LONG ref;
};

static inline struct notification_callback *impl_from_INotificationActivationCallback( INotificationActivationCallback *iface )
{
    return CONTAINING_RECORD( iface, struct notification_callback, iface );
}

static HRESULT WINAPI callback_QueryInterface( INotificationActivationCallback *iface, const IID *iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_INotificationActivationCallback ))
    {
        *out = iface;
        INotificationActivationCallback_AddRef( iface );
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI callback_AddRef( INotificationActivationCallback *iface )
{
    struct notification_callback *impl = impl_from_INotificationActivationCallback( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI callback_Release( INotificationActivationCallback *iface )
{
    struct notification_callback *impl = impl_from_INotificationActivationCallback( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI callback_Activate( INotificationActivationCallback *iface, const WCHAR *aumid, const WCHAR *args,
                                         const NOTIFICATION_USER_INPUT_DATA *data, ULONG count )
{
    struct notification_callback *impl = impl_from_INotificationActivationCallback( iface );

    ok( aumid && !wcscmp( aumid, impl->aumid ), "got aumid %s\n", debugstr_w( aumid ) );
    return S_OK;
}

static const INotificationActivationCallbackVtbl notification_callback_vtbl =
{
    /* IUnknown */
    callback_QueryInterface,
    callback_AddRef,
    callback_Release,
    /* INotificationActivationCallback */
    callback_Activate
};

struct notification_callback_factory
{
    IClassFactory iface;
    const WCHAR *aumid;
    LONG ref;
};

static inline struct notification_callback_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct notification_callback_factory, iface );
}

static HRESULT WINAPI factory_QueryInterface( IClassFactory *iface, const IID *iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IClassFactory ))
    {
        *out = iface;
        IClassFactory_AddRef( iface );
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IClassFactory *iface )
{
    struct notification_callback_factory *impl = impl_from_IClassFactory( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI factory_Release( IClassFactory *iface )
{
    struct notification_callback_factory *impl = impl_from_IClassFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI factory_CreateInstance( IClassFactory *iface, IUnknown *outer, const IID *iid, void **out )
{
    struct notification_callback_factory *impl = impl_from_IClassFactory( iface );
    struct notification_callback *cb_impl;
    HRESULT hr;

    if (outer) return CLASS_E_NOAGGREGATION;
    if (!((cb_impl = calloc( 1, sizeof(*cb_impl) )))) return E_OUTOFMEMORY;

    cb_impl->iface.lpVtbl = &notification_callback_vtbl;
    cb_impl->aumid = impl->aumid;
    cb_impl->ref = 1;
    hr = INotificationActivationCallback_QueryInterface( &cb_impl->iface, iid, out );
    INotificationActivationCallback_Release( &cb_impl->iface );
    return hr;
}

static HRESULT WINAPI factory_LockServer( IClassFactory *iface, BOOL lock )
{
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    /* IUnknown */
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IClassFactory */
    factory_CreateInstance,
    factory_LockServer,
};

static void string_to_guid( int line, const WCHAR *str, GUID *out )
{
    HCRYPTPROV hprov;
    HCRYPTHASH hash;
    BYTE buf[32];
    DWORD len = ARRAY_SIZE( buf );
    BOOL ret;

    ret = CryptAcquireContextW( &hprov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT );
    ok_( __FILE__, line )( ret, "CryptAcquireContextW failed: %lu\n", GetLastError() );
    ret = CryptCreateHash( hprov, CALG_SHA_256, 0, 0, &hash );
    ok_( __FILE__, line )( ret, "CryptCreateHash failed: %lu\n", GetLastError() );
    ret = CryptHashData( hash, (const BYTE *)str, wcslen( str ), 0 );
    ok_( __FILE__, line )( ret, "CryptHashData failed: %lu\n", GetLastError() );
    ret = CryptGetHashParam( hash, HP_HASHVAL, buf, &len, 0 );
    ok_( __FILE__, line )( ret, "CryptGetHashParam failed: %lu\n", GetLastError() );
    CryptDestroyHash( hash );
    CryptReleaseContext( hprov, 0 );
    memcpy( out, buf, sizeof( *out ) );
}

static WCHAR *aumid_get_clsid_reg_key( int line, const WCHAR *aumid )
{
    static const WCHAR *fmt = L"Software\\Classes\\CLSID\\%s\\LocalServer32";
    WCHAR *key, clsid_str[39];
    CLSID clsid;
    int size;

    string_to_guid( line, aumid, &clsid );
    StringFromGUID2( &clsid, clsid_str, ARRAY_SIZE( clsid_str ) );
    size = _scwprintf( fmt, clsid_str ) + 1;
    key = calloc( size, sizeof(WCHAR) );
    ok_( __FILE__, line )( !!key, "got key %p\n", key );
    _snwprintf( key, size, fmt, clsid_str );
    return key;
}

static WCHAR *aumid_get_reg_key( int line, const WCHAR *aumid )
{
    static const WCHAR *fmt = L"Software\\Classes\\AppUserModelId\\%s";
    WCHAR *key;
    int size;

    size = _scwprintf( fmt, aumid ) + 1;
    key = calloc( size, sizeof(WCHAR) );
    ok_( __FILE__, line )( !!key, "got key %p\n", key );
    _snwprintf( key, size, fmt, aumid );
    return key;
}

static WCHAR *aumid_get_shortcut_path( int line, const WCHAR *aumid )
{
    static const WCHAR *fmt = L"%s\\%s.lnk";
    WCHAR *programs_path, *lnk_path;
    HRESULT hr;
    int size;

    hr = SHGetKnownFolderPath(&FOLDERID_Programs, 0, NULL, &programs_path);
    ok_( __FILE__, line )( hr == S_OK, "got hr %#lx\n", hr );
    size = _scwprintf( fmt, programs_path, aumid ) + 1;
    lnk_path = calloc( size, sizeof(WCHAR) );
    ok_( __FILE__, line )( !!lnk_path, "got lnk_path %p\n", lnk_path );
    _snwprintf( lnk_path, size, fmt, programs_path, aumid );
    CoTaskMemFree( programs_path );
    return lnk_path;
}

static void aumid_register_notification_callback( const WCHAR *aumid, DWORD *registration )
{
    WCHAR path[MAX_PATH + 1], *launch, *key, clsid_str[39], *shortcut_path;
    struct notification_callback_factory *factory_impl;
    IPropertyStore *store;
    IPersistFile *file;
    IShellLinkW *link;
    PROPVARIANT prop;
    CLSID clsid;
    HRESULT hr;
    size_t len;
    DWORD ret;

    if (!pSetCurrentProcessExplicitAppUserModelID)
    {
        win_skip( "SetCurrentProcessExplicitAppUserModelID is not available\n" );
        return;
    }

    hr = pSetCurrentProcessExplicitAppUserModelID( aumid );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    factory_impl = calloc( 1, sizeof(*factory_impl) );
    ok( !!factory_impl, "got impl %p\n", factory_impl );
    factory_impl->iface.lpVtbl = &class_factory_vtbl;
    factory_impl->aumid = aumid;
    factory_impl->ref = 1;

    string_to_guid( __LINE__, aumid, &clsid );
    hr = CoRegisterClassObject( &clsid, (IUnknown *)&factory_impl->iface, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, registration );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    IClassFactory_Release( &factory_impl->iface );

    GetModuleFileNameW( NULL, path, ARRAY_SIZE( path ) );
    len = _snwprintf( NULL, 0, L"\"%s\" wpnapps -toast", path );
    launch = calloc( len + 1, sizeof( WCHAR ) );
    ok( !!launch, "got launch %p\n", launch );
    _snwprintf( launch, len, L"\"%s\" wpnapps -toast", path );

    hr = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&link );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IShellLinkW_SetPath( link, path );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IShellLinkW_QueryInterface( link, &IID_IPropertyStore, (void **)&store );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    IShellLinkW_Release( link );
    V_VT(&prop) = VT_LPWSTR;
    prop.pwszVal = (WCHAR *)aumid;
    hr = IPropertyStore_SetValue( store, &PKEY_AppUserModel_ID, &prop );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    V_VT(&prop) = VT_CLSID;
    prop.puuid = &clsid;
    hr = IPropertyStore_SetValue( store, &PKEY_AppUserModel_ToastActivatorCLSID, &prop );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    hr = IPropertyStore_QueryInterface( store, &IID_IPersistFile, (void **)&file );
    ok( hr == S_OK, "got hr %#lx\n", hr);
    IPropertyStore_Release( store );

    shortcut_path = aumid_get_shortcut_path( __LINE__, aumid );
    hr = IPersistFile_Save( file, shortcut_path, TRUE );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    free( shortcut_path );

    key = aumid_get_clsid_reg_key( __LINE__, aumid );
    ret = RegSetKeyValueW( HKEY_CURRENT_USER, key, NULL, REG_SZ, launch, (wcslen( launch ) + 1) * sizeof(WCHAR) );
    ok( !ret, "got ret %lu\n", ret );
    free( key );
    free( launch );

    StringFromGUID2( &clsid, clsid_str, ARRAY_SIZE( clsid_str ) );
    key = aumid_get_reg_key( __LINE__, aumid );
    ret = RegSetKeyValueW( HKEY_CURRENT_USER, key, L"CustomActivator", REG_SZ, clsid_str, ( wcslen( clsid_str ) + 1 ) * sizeof( WCHAR ) );
    ok( !ret, "got ret %lu\n", ret );
    ret = RegSetKeyValueW( HKEY_CURRENT_USER, key, L"DisplayName", REG_SZ, aumid, (wcslen( aumid ) + 1) * sizeof(WCHAR) );
    free( key );
}

static void aumid_cleanup_notification_callback( const WCHAR *aumid, DWORD registration )
{
    CLSID clsid;
    LSTATUS ret = 0;
    WCHAR *key, *path;
    HRESULT hr;
    BOOL success;

    if (!pSetCurrentProcessExplicitAppUserModelID)
    {
        win_skip( "SetCurrentProcessExplicitAppUserModelID is not available\n" );
        return;
    }

    string_to_guid( __LINE__, aumid, &clsid );
    hr = CoRevokeClassObject( registration );
    ok( hr == S_OK, "got hr %#lx\n", hr );

    path = aumid_get_shortcut_path( __LINE__, aumid );
    success = DeleteFileW( path );
    ok( success, "DeleteFileW failed: %lu\n", GetLastError() );

    key = aumid_get_clsid_reg_key( __LINE__, aumid );
    ret = RegDeleteTreeW( HKEY_CURRENT_USER, key );
    ok( !ret, "got ret %lu\n", ret );
    free( key );

    key = aumid_get_reg_key( __LINE__, aumid );
    ret = RegDeleteTreeW( HKEY_CURRENT_USER, key );
    ok( !ret, "got ret %lu\n", ret );
    free( key );
}

static void test_ToastNotificationManager( const WCHAR *aumid )
{
    static const WCHAR *class_name_manager = RuntimeClass_Windows_UI_Notifications_ToastNotificationManager;
    static const WCHAR *class_name_notif = RuntimeClass_Windows_UI_Notifications_ToastNotification;
    static const WCHAR *notification_text = L"\u25B2";

    IToastNotificationManagerStatics *statics;
    IToastNotificationFactory *notif_factory;
    IActivationFactory *factory;
    IToastNotification *notif;
    IToastNotifier *notifier;
    IXmlNodeSerializer *ser;
    IXmlDocument *notif_xml;
    HSTRING_HEADER str_hdr;
    IXmlNodeList *nodes;
    IXmlNode *node;
    UINT32 len = 0;
    HSTRING str;
    HRESULT hr;

    if (!pSetCurrentProcessExplicitAppUserModelID)
    {
        win_skip( "SetCurrentProcessExplicitAppUserModelID is not available\n" );
        return;
    }

    WindowsCreateStringReference( class_name_manager, wcslen( class_name_manager ), &str_hdr, &str );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    todo_wine ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        todo_wine win_skip( "%s runtimeclass not registered\n", debugstr_w( class_name_manager ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IToastNotificationManagerStatics, (void **)&statics );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IActivationFactory_Release( factory );

    WindowsCreateStringReference( class_name_notif, wcslen( class_name_notif ), &str_hdr, &str );
    hr = RoGetActivationFactory( str, &IID_IToastNotificationFactory, (void **)&notif_factory );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        skip( "%s runtimeclass not registered\n", debugstr_w( class_name_manager ) );
        return;
    }

    WindowsCreateStringReference( aumid, wcslen( aumid ), &str_hdr, &str );
    hr = IToastNotificationManagerStatics_CreateToastNotifierWithId( statics, str, &notifier );
    todo_wine ok(hr == S_OK, "got hr %#lx\n", hr );

    todo_wine check_interface( notifier, &IID_IUnknown, TRUE );
    todo_wine check_interface( notifier, &IID_IInspectable, TRUE );
    check_interface( notifier, &IID_IAgileObject, FALSE );

    hr = IToastNotificationManagerStatics_GetTemplateContent( statics, ToastTemplateType_ToastText01, &notif_xml );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );

    todo_wine check_interface( notif_xml, &IID_IUnknown, TRUE );
    todo_wine check_interface( notif_xml, &IID_IInspectable, TRUE );
    todo_wine check_interface( notif_xml, &IID_IAgileObject, TRUE );

    WindowsCreateStringReference( L"text", wcslen( L"text" ), &str_hdr, &str );
    hr = IXmlDocument_GetElementsByTagName( notif_xml, str, &nodes );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );

    hr = IXmlNodeList_get_Length( nodes, &len );
    todo_wine ok( len == 1, "got len %I32u\n", len );
    hr = IXmlNodeList_Item( nodes, 0, &node );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IXmlNodeList_Release( nodes );

    hr = IXmlNode_get_NodeName( node, &str );
    todo_wine ok( !wcscmp( WindowsGetStringRawBuffer( str, NULL ), L"text" ), "got str %s\n", debugstr_hstring( str ) );
    hr = IXmlNode_QueryInterface( node, &IID_IXmlNodeSerializer, (void **)&ser );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IXmlNode_Release( node );

    WindowsCreateStringReference( notification_text, wcslen( notification_text ), &str_hdr, &str );
    hr = IXmlNodeSerializer_put_InnerText( ser, str );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IXmlNodeSerializer_Release( ser );

    hr = IToastNotificationFactory_CreateToastNotification( notif_factory, notif_xml, &notif );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IXmlDocument_Release( notif_xml );
    hr = IToastNotifier_Show( notifier, notif );
    todo_wine ok( hr == S_OK, "got hr %#lx\n", hr );
    IToastNotification_Release( notif );
    IToastNotifier_Release( notifier );

    IToastNotificationFactory_Release( notif_factory );
    IToastNotificationManagerStatics_Release( statics );
}

START_TEST(wpnapps)
{
    static const WCHAR *aumid = L"WineHQ.Wine.Test.WpnApps";
    DWORD registration;
    HMODULE hshcore;
    HRESULT hr;
    int argc;

    CommandLineToArgvW(GetCommandLineW(), &argc);

    hshcore = LoadLibraryA( "shcore.dll" );
    if (!hshcore)
    {
        win_skip( "shcore.dll is not available\n" );
        return;
    }

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    pSetCurrentProcessExplicitAppUserModelID = (void *)GetProcAddress( hshcore, "SetCurrentProcessExplicitAppUserModelID" );

    aumid_register_notification_callback( aumid, &registration );
    test_ToastNotificationManager( aumid );
    aumid_cleanup_notification_callback( aumid, registration );

    RoUninitialize();
}
