/*
 * Protected Storage (pstores)
 *
 * Copyright 2004 Mike McCormack for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winuser.h"
#include "winnls.h"
#include "initguid.h"
#include "ole2.h"
#include "shlwapi.h"

#include "pstore.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pstores);

typedef struct
{
    IPStore IPStore_iface;
    LONG ref;
} PStore_impl;


static const WCHAR szPStoresKey[] = {
    'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','W','i','n','e','\\',
    'p','s','t','o','r','e','s',0
};

static inline PStore_impl *impl_from_IPStore(IPStore *iface)
{
    return CONTAINING_RECORD(iface, PStore_impl, IPStore_iface);
}

/* convert a guid to a wide character string */
static void IPStore_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

static LONG IPStore_OpenRoot( PST_KEY Key, HKEY *hkey )
{
    switch( Key )
    {
    case PST_KEY_CURRENT_USER:
        return RegCreateKeyW( HKEY_CURRENT_USER, szPStoresKey, hkey );
    case PST_KEY_LOCAL_MACHINE:
        return RegCreateKeyW( HKEY_LOCAL_MACHINE, szPStoresKey, hkey );
    }
    return ERROR_INVALID_PARAMETER;
}

/**************************************************************************
 *  IPStore->QueryInterface
 */
static HRESULT WINAPI PStore_fnQueryInterface(
        IPStore* iface,
        REFIID riid,
        LPVOID *ppvObj)
{
    PStore_impl *This = impl_from_IPStore(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IPStore) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &This->IPStore_iface;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * IPStore->AddRef
 */
static ULONG WINAPI PStore_fnAddRef(IPStore* iface)
{
    PStore_impl *This = impl_from_IPStore(iface);

    TRACE("%p %lu\n", This, This->ref);

    return InterlockedIncrement( &This->ref );
}

/******************************************************************************
 * IPStore->Release
 */
static ULONG WINAPI PStore_fnRelease(IPStore* iface)
{
    PStore_impl *This = impl_from_IPStore(iface);
    LONG ref;

    TRACE("%p %lu\n", This, This->ref);

    ref = InterlockedDecrement( &This->ref );
    if( !ref )
        HeapFree( GetProcessHeap(), 0, This );

    return ref;
}

/******************************************************************************
 * IPStore->GetInfo
 */
static HRESULT WINAPI PStore_fnGetInfo( IPStore* iface, PPST_PROVIDERINFO* ppProperties)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->GetProvParam
 */
static HRESULT WINAPI PStore_fnGetProvParam( IPStore* iface,
    DWORD dwParam, DWORD* pcbData, BYTE** ppbData, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->SetProvParam
 */
static HRESULT WINAPI PStore_fnSetProvParam( IPStore* This,
    DWORD dwParam, DWORD cbData, BYTE* pbData, DWORD* dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->CreateType
 */
static HRESULT WINAPI PStore_fnCreateType( IPStore* This,
    PST_KEY Key, const GUID* pType, PPST_TYPEINFO pInfo, DWORD dwFlags)
{
    LONG r;
    HKEY hkey, hkeytype;
    WCHAR szGuid[40];
    HRESULT hres = E_FAIL;
    DWORD dwCreated = 0;

    TRACE("%p %08lx %s %p(%ld,%s) %08lx\n", This, Key, debugstr_guid(pType),
          pInfo, pInfo->cbSize, debugstr_w(pInfo->szDisplayName), dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return hres;

    IPStore_guid2wstr( pType, szGuid );
    r = RegCreateKeyExW( hkey, szGuid, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hkeytype, &dwCreated );
    if( r == ERROR_SUCCESS )
    {
        if( dwCreated == REG_CREATED_NEW_KEY )
        {
            r = RegSetValueW( hkeytype, NULL, REG_SZ, 
                   pInfo->szDisplayName, lstrlenW( pInfo->szDisplayName ) );
            if( r == ERROR_SUCCESS )
                hres = PST_E_OK;
            RegCloseKey( hkeytype );
        }
        else
            hres = PST_E_TYPE_EXISTS;
    }
    RegCloseKey( hkey );

    return hres;
}

/******************************************************************************
 * IPStore->GetTypeInfo
 */
static HRESULT WINAPI PStore_fnGetTypeInfo( IPStore* This,
    PST_KEY Key, const GUID* pType, PPST_TYPEINFO** ppInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->DeleteType
 */
static HRESULT WINAPI PStore_fnDeleteType( IPStore* This,
    PST_KEY Key, const GUID* pType, DWORD dwFlags)
{
    LONG r;
    HKEY hkey;
    WCHAR szGuid[40];
    HRESULT hres = E_FAIL;

    TRACE("%p %d %s %08lx\n", This, Key, debugstr_guid(pType), dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return hres;
    
    IPStore_guid2wstr( pType, szGuid );
    r = SHDeleteKeyW( hkey, szGuid );
    if( r == ERROR_SUCCESS )
        hres = PST_E_OK;
    RegCloseKey( hkey );
    
    return hres;
}

/******************************************************************************
 * IPStore->CreateSubtype
 */
static HRESULT WINAPI PStore_fnCreateSubtype( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO pInfo, PPST_ACCESSRULESET pRules, DWORD dwFlags)
{
    LONG r;
    HKEY hkey, hkeysubtype;
    WCHAR szGuid[80];
    HRESULT hres = E_FAIL;
    DWORD dwCreated = 0;

    TRACE("%p %08lx %s %s %p %p %08lx\n", This, Key, debugstr_guid(pType),
           debugstr_guid(pSubtype), pInfo, pRules, dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return E_FAIL;
    
    IPStore_guid2wstr( pType, szGuid );
    szGuid[38] = '\\';
    IPStore_guid2wstr( pSubtype, &szGuid[39] );
    r = RegCreateKeyExW( hkey, szGuid, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hkeysubtype, &dwCreated );
    if( r == ERROR_SUCCESS )
    {
        if( dwCreated == REG_CREATED_NEW_KEY )
        {
            r = RegSetValueW( hkeysubtype, NULL, REG_SZ, 
                       pInfo->szDisplayName, lstrlenW( pInfo->szDisplayName ) );
            if( r == ERROR_SUCCESS )
                hres = S_OK;
            RegCloseKey( hkeysubtype );
        }
        else
            hres = PST_E_TYPE_EXISTS;
    }
    RegCloseKey( hkey );

    return hres;
}

/******************************************************************************
 * IPStore->GetSubtypeInfo
 */
static HRESULT WINAPI PStore_fnGetSubtypeInfo( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO** ppInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->DeleteSubtype
 */
static HRESULT WINAPI PStore_fnDeleteSubtype( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype, DWORD dwFlags)
{
    LONG r;
    HKEY hkey, hkeytype;
    WCHAR szGuid[40];
    HRESULT hres = E_FAIL;

    TRACE("%p %lu %s %s %08lx\n", This, Key,
          debugstr_guid(pType), debugstr_guid(pSubtype), dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return hres;
    
    IPStore_guid2wstr( pType, szGuid );
    r = RegOpenKeyW( hkey, szGuid, &hkeytype );
    if( r == ERROR_SUCCESS )
    {
        IPStore_guid2wstr( pSubtype, szGuid );
        r = SHDeleteKeyW( hkeytype, szGuid );
        if( r == ERROR_SUCCESS )
            hres = PST_E_OK;
        RegCloseKey( hkeytype );
    }
    RegCloseKey( hkey );

    return hres;
}

/******************************************************************************
 * IPStore->ReadAccessRuleset
 */
static HRESULT WINAPI PStore_fnReadAccessRuleset( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype, PPST_TYPEINFO pInfo,
    PPST_ACCESSRULESET** ppRules, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->WriteAccessRuleSet
 */
static HRESULT WINAPI PStore_fnWriteAccessRuleset( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO pInfo, PPST_ACCESSRULESET pRules, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->EnumTypes
 */
static HRESULT WINAPI PStore_fnEnumTypes( IPStore* This, PST_KEY Key,
    DWORD dwFlags, IEnumPStoreTypes** ppenum)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->EnumSubtypes
 */
static HRESULT WINAPI PStore_fnEnumSubtypes( IPStore* This, PST_KEY Key,
    const GUID* pType, DWORD dwFlags, IEnumPStoreTypes** ppenum)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->DeleteItem
 */
static HRESULT WINAPI PStore_fnDeleteItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubType, LPCWSTR szItemName,
    PPST_PROMPTINFO pPromptInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_FAIL;
}

/******************************************************************************
 * IPStore->ReadItem
 */
static HRESULT WINAPI PStore_fnReadItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    DWORD *cbData, BYTE** pbData, PPST_PROMPTINFO pPromptInfo, DWORD dwFlags)
{
    LONG r;
    HKEY hkey, hkeysubtype;
    WCHAR szGuid[80];
    DWORD type;

    TRACE("%p %08lx %s %s %s %p %p %p %08lx\n", This, Key,
        debugstr_guid(pItemType), debugstr_guid(pItemSubtype), 
        debugstr_w(szItemName), cbData, pbData, pPromptInfo, dwFlags);

    *pbData = NULL;
    *cbData = 0;

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return E_FAIL;

    IPStore_guid2wstr( pItemType, szGuid );
    szGuid[38] = '\\';
    IPStore_guid2wstr( pItemSubtype, &szGuid[39] );
    r = RegOpenKeyW( hkey, szGuid, &hkeysubtype );
    if( r == ERROR_SUCCESS )
    {
        type = 0;
        r = RegQueryValueExW( hkeysubtype, szItemName, NULL, &type,
                              NULL, cbData );
        if( ( r == ERROR_SUCCESS ) && ( type == REG_BINARY ) )
        {
            *pbData = CoTaskMemAlloc( *cbData );
            r = RegQueryValueExW( hkeysubtype, szItemName, NULL, &type,
                                  *pbData, cbData );
        }
        RegCloseKey( hkeysubtype );
    }
    
    RegCloseKey( hkey );

    return ( r == ERROR_SUCCESS ) ? S_OK : E_FAIL;
}

/******************************************************************************
 * IPStore->WriteItem
 */
static HRESULT WINAPI PStore_fnWriteItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    DWORD cbData, BYTE* ppbData, PPST_PROMPTINFO pPromptInfo,
    DWORD dwDefaultConfirmationStyle, DWORD dwFlags)
{
    LONG r;
    HKEY hkey, hkeysubtype;
    WCHAR szGuid[80];

    TRACE("%p %08lx %s %s %s %ld %p %p %08lx\n", This, Key,
        debugstr_guid(pItemType), debugstr_guid(pItemSubtype), 
        debugstr_w(szItemName), cbData, ppbData, pPromptInfo, dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return E_FAIL;

    IPStore_guid2wstr( pItemType, szGuid );
    szGuid[38] = '\\';
    IPStore_guid2wstr( pItemSubtype, &szGuid[39] );
    r = RegOpenKeyW( hkey, szGuid, &hkeysubtype );
    if( r == ERROR_SUCCESS )
    {
        r = RegSetValueExW( hkeysubtype, szItemName, 0, REG_BINARY, 
                            ppbData, cbData );
        RegCloseKey( hkeysubtype );
    }
    
    RegCloseKey( hkey );

    return ( r == ERROR_SUCCESS ) ? S_OK : E_FAIL;
}

/******************************************************************************
 * IPStore->OpenItem
 */
static HRESULT WINAPI PStore_fnOpenItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    PST_ACCESSMODE ModeFlags, PPST_PROMPTINFO pPromptInfo, DWORD dwFlags )
{
    LONG r;
    HKEY hkey, hkeysubtype;
    WCHAR szGuid[80];

    TRACE("%p %08lx %s %s %s %08lx %p %08lx\n", This, Key,
           debugstr_guid(pItemType), debugstr_guid(pItemSubtype),
           debugstr_w(szItemName), ModeFlags, pPromptInfo, dwFlags);

    r = IPStore_OpenRoot( Key, &hkey );
    if( r )
        return E_FAIL;

    IPStore_guid2wstr( pItemType, szGuid );
    szGuid[38] = '\\';
    IPStore_guid2wstr( pItemSubtype, &szGuid[39] );
    r = RegOpenKeyW( hkey, szGuid, &hkeysubtype );
    if( r == ERROR_SUCCESS )
    {
        DWORD type;

        r = RegQueryValueExW( hkeysubtype, szItemName, NULL, &type, 
                              NULL, NULL );
        if( ( r == ERROR_SUCCESS ) && ( type != REG_BINARY ) )
            r = ERROR_INVALID_DATA;

        RegCloseKey( hkeysubtype );
    }
    
    RegCloseKey( hkey );

    return ( r == ERROR_SUCCESS ) ? S_OK : E_FAIL;
}

/******************************************************************************
 * IPStore->CloseItem
 */
static HRESULT WINAPI PStore_fnCloseItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR* szItemName,
    DWORD dwFlags)
{
    TRACE("\n");
    return S_OK;
}

/******************************************************************************
 * IPStore->EnumItems
 */
static HRESULT WINAPI PStore_fnEnumItems( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, DWORD dwFlags,
    IEnumPStoreItems** ppenum)
{
    FIXME("\n");
    return E_FAIL;
}


static const IPStoreVtbl pstores_vtbl =
{
    PStore_fnQueryInterface,
    PStore_fnAddRef,
    PStore_fnRelease,
    PStore_fnGetInfo,
    PStore_fnGetProvParam,
    PStore_fnSetProvParam,
    PStore_fnCreateType,
    PStore_fnGetTypeInfo,
    PStore_fnDeleteType,
    PStore_fnCreateSubtype,
    PStore_fnGetSubtypeInfo,
    PStore_fnDeleteSubtype,
    PStore_fnReadAccessRuleset,
    PStore_fnWriteAccessRuleset,
    PStore_fnEnumTypes,
    PStore_fnEnumSubtypes,
    PStore_fnDeleteItem,
    PStore_fnReadItem,
    PStore_fnWriteItem,
    PStore_fnOpenItem,
    PStore_fnCloseItem,
    PStore_fnEnumItems
};

HRESULT WINAPI PStoreCreateInstance( IPStore** ppProvider,
            PST_PROVIDERID* pProviderID, void* pReserved, DWORD dwFlags)
{
    PStore_impl *ips;

    TRACE("%p %s %p %08lx\n", ppProvider, debugstr_guid(pProviderID), pReserved, dwFlags);

    ips = HeapAlloc( GetProcessHeap(), 0, sizeof (PStore_impl) );
    if( !ips )
        return E_OUTOFMEMORY;

    ips->IPStore_iface.lpVtbl = &pstores_vtbl;
    ips->ref = 1;

    *ppProvider = &ips->IPStore_iface;

    return S_OK;
}

/***********************************************************************
 *             DllGetClassObject (PSTOREC.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("(%s,%s,%p) stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
