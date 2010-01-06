/*
 *    Word splitter Implementation
 *
 * Copyright 2006 Mike McCormack
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "indexsrv.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(infosoft);

DEFINE_GUID(CLSID_wb_Neutral,0x369647e0,0x17b0,0x11ce,0x99,0x50,0x00,0xaa,0x00,0x4b,0xbb,0x1f);

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern HRESULT WINAPI wb_Constructor(IUnknown*, REFIID, LPVOID *);

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown*, REFIID, LPVOID*);

typedef struct
{
    const IClassFactoryVtbl *lpVtbl;
    LPFNCREATEINSTANCE      lpfnCI;
} CFImpl;

static HRESULT WINAPI infosoftcf_fnQueryInterface ( LPCLASSFACTORY iface,
        REFIID riid, LPVOID *ppvObj)
{
    CFImpl *This = (CFImpl *)iface;

    TRACE("(%p)->(%s)\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppvObj = This;
        return S_OK;
    }

    TRACE("-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI infosoftcf_fnAddRef (LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI infosoftcf_fnRelease(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI infosoftcf_fnCreateInstance( LPCLASSFACTORY iface,
        LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    CFImpl *This = (CFImpl *)iface;

    TRACE("%p->(%p,%s,%p)\n", This, pUnkOuter, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    return This->lpfnCI(pUnkOuter, riid, ppvObject);
}

static HRESULT WINAPI infosoftcf_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("%p %d\n", iface, fLock);
    return E_NOTIMPL;
}

static const IClassFactoryVtbl infosoft_cfvt =
{
    infosoftcf_fnQueryInterface,
    infosoftcf_fnAddRef,
    infosoftcf_fnRelease,
    infosoftcf_fnCreateInstance,
    infosoftcf_fnLockServer
};

static CFImpl wb_cf = { &infosoft_cfvt, wb_Constructor };

/***********************************************************************
 *             DllGetClassObject (INFOSOFT.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    IClassFactory   *pcf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (IsEqualIID(rclsid, &CLSID_wb_Neutral))
        pcf = (IClassFactory*) &wb_cf;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(pcf, iid, ppv);
}


HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

static HRESULT add_key_val( LPCSTR key, LPCSTR valname, LPCSTR value )
{
    HKEY hkey;

    if (RegCreateKeyA( HKEY_CLASSES_ROOT, key, &hkey ) != ERROR_SUCCESS) return E_FAIL;
    RegSetValueA( hkey, valname, REG_SZ, value, strlen( value ) + 1 );
    RegCloseKey( hkey );
    return S_OK;
}

static HRESULT add_wordbreaker_clsid( LPCSTR lang, const CLSID *id)
{
    CHAR key[100], val[50];

    strcpy(key, "CLSID\\");
    sprintf(key+6, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            id->Data1, id->Data2, id->Data3,
            id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
            id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    sprintf(val, "%s Word Breaker", lang);
    add_key_val( key, NULL, val );
    strcat(key, "\\InProcServer32");
    add_key_val( key, NULL, "infosoft.dll" );
    add_key_val( key, "ThreadingModel", "Both" );
    return S_OK;
}

#define ADD_BREAKER(name)  add_wordbreaker_clsid( #name, &CLSID_wb_##name )

static HRESULT add_content_index_keys(void)
{
    ADD_BREAKER(Neutral); /* in query.dll on Windows */
    return S_OK;
}

/***********************************************************************
 *             DllRegisterServer (INFOSOFT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    add_content_index_keys();
    return S_OK;
}
