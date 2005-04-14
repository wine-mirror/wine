/*
 *    MSHTML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003 Mike McCormack
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "docobj.h"

#include "mshtml.h"
#include "mshtml_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#include "initguid.h"

DEFINE_GUID( CLSID_MozillaBrowser, 0x1339B54C,0x3453,0x11D2,0x93,0xB9,0x00,0x00,0x00,0x00,0x00,0x00);

typedef HRESULT (WINAPI *fnGetClassObject)(REFCLSID rclsid, REFIID iid, LPVOID *ppv);
typedef BOOL (WINAPI *fnCanUnloadNow)();

static HMODULE hMozCtl;

HINSTANCE hInst;

/* convert a guid to a wide character string */
static void MSHTML_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

static BOOL MSHTML_GetMozctlPath( LPWSTR szPath, DWORD sz )
{
    DWORD r, type;
    BOOL ret = FALSE;
    HKEY hkey;
    static const WCHAR szPre[] = {
        'S','o','f','t','w','a','r','e','\\',
        'C','l','a','s','s','e','s','\\',
        'C','L','S','I','D','\\',0 };
    static const WCHAR szPost[] = {
        '\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2',0 };
    WCHAR szRegPath[(sizeof(szPre)+sizeof(szPost))/sizeof(WCHAR)+40];

    strcpyW( szRegPath, szPre );
    MSHTML_guid2wstr( &CLSID_MozillaBrowser, &szRegPath[strlenW(szRegPath)] );
    strcatW( szRegPath, szPost );

    TRACE("key = %s\n", debugstr_w( szRegPath ) );

    r = RegOpenKeyW( HKEY_LOCAL_MACHINE, szRegPath, &hkey );
    if( r != ERROR_SUCCESS )
        return FALSE;

    r = RegQueryValueExW( hkey, NULL, NULL, &type, (LPBYTE)szPath, &sz );
    ret = ( r == ERROR_SUCCESS ) && ( type == REG_SZ );
    RegCloseKey( hkey );

    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    WCHAR szPath[MAX_PATH];

    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            if(MSHTML_GetMozctlPath(szPath, sizeof szPath)) {
                hMozCtl = LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                if(!hMozCtl)
                    ERR("Can't load the Mozilla ActiveX control\n");
            }else {
                MESSAGE("You need to install the Mozilla ActiveX control to\n");
                MESSAGE("use Wine's builtin MSHTML dll.\n");
            }
            hInst = hInstDLL;
	    break;
	case DLL_PROCESS_DETACH:
            if(hMozCtl)
                FreeLibrary( hMozCtl );
	    break;
    }
    return TRUE;
}

/***********************************************************
 *    ClassFactory implementation
 */
typedef HRESULT (*CreateInstanceFunc)(IUnknown*,REFIID,void**);
typedef struct {
    const IClassFactoryVtbl *lpVtbl;
    ULONG ref;
    CreateInstanceFunc fnCreateInstance;
} ClassFactory;

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFGUID riid, void **ppvObject)
{
    if(IsEqualGUID(&IID_IClassFactory, riid) || IsEqualGUID(&IID_IUnknown, riid)) {
        IClassFactory_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    WARN("not supported iid %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppvObject)
{
    ClassFactory *This = (ClassFactory*)iface;
    return This->fnCreateInstance(pUnkOuter, riid, ppvObject);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%x) stub\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl HTMLClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT ClassFactory_Create(REFIID riid, void **ppv, CreateInstanceFunc fnCreateInstance)
{
    ClassFactory *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ClassFactory));
    HRESULT hres;

    ret->lpVtbl = &HTMLClassFactoryVtbl;
    ret->ref = 0;
    ret->fnCreateInstance = fnCreateInstance;

    hres = IClassFactory_QueryInterface((IClassFactory*)ret, riid, ppv);
    if(FAILED(hres)) {
        HeapFree(GetProcessHeap(), 0, ret);
        *ppv = NULL;
    }
    return hres;
}

HRESULT WINAPI MSHTML_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hres;
    fnGetClassObject pGetClassObject;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv );

    if(hMozCtl && IsEqualGUID(&CLSID_HTMLDocument, rclsid)) {
        pGetClassObject = (fnGetClassObject) GetProcAddress(hMozCtl, "DllGetClassObject");
        if(pGetClassObject) {
            hres = pGetClassObject(&CLSID_MozillaBrowser, riid, ppv);
            if(SUCCEEDED(hres)) {
                TRACE("returning Mozilla ActiveX Control hres = %08lx  *ppv = %p\n", hres, *ppv);
                return hres;
            }
        }
    }

    if(IsEqualGUID(&CLSID_HTMLDocument, rclsid)) {
        hres = ClassFactory_Create(riid, ppv, HTMLDocument_Create);
        TRACE("hres = %08lx\n", hres);
        return hres;
    }

    FIXME("Unknown class %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI MSHTML_DllCanUnloadNow(void)
{
    fnCanUnloadNow pCanUnloadNow = NULL;
    HRESULT hres;

    TRACE("()\n");

    if(hMozCtl)
        pCanUnloadNow = (fnCanUnloadNow) GetProcAddress(hMozCtl, "DllCanUnloadNow");
    if(!pCanUnloadNow)
        return S_FALSE;

    hres = pCanUnloadNow();

    TRACE("hres = %08lx\n", hres);

    return hres;
}

/* appears to have the same prototype as WinMain */
INT WINAPI RunHTMLApplication( HINSTANCE hinst, HINSTANCE hPrevInst,
                               LPCSTR szCmdLine, INT nCmdShow )
{
    FIXME("%p %p %s %d\n", hinst, hPrevInst, debugstr_a(szCmdLine), nCmdShow );
    return 0;
}

/***********************************************************************
 *          DllInstall (MSHTML.@)
 */
HRESULT WINAPI MSHTML_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("stub %d %s: returning S_OK\n", bInstall, debugstr_w(cmdline));
    return S_OK;
}

/***********************************************************************
 *          DllRegisterServer (MSHTML.@)
 */
HRESULT WINAPI MSHTML_DllRegisterServer(void)
{
    FIXME("stub: returning S_OK\n");
    return S_OK;
}
