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
#include "advpub.h"

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
                TRACE("Not found Mozilla ActiveX Control. HTML rendering will be disabled.");
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

/***********************************************************************
 *          RunHTMLApplication (MSHTML.@)
 *
 * Appears to have the same prototype as WinMain.
 */
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

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CAnchorBrowsePropertyPage, 0x3050F3BB, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CBackgroundPropertyPage, 0x3050F232, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CCDAnchorPropertyPage, 0x3050F1FC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CCDGenericPropertyPage, 0x3050F17F, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CDocBrowsePropertyPage, 0x3050F3B4, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CDwnBindInfo, 0x3050F3C2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CHiFiUses, 0x5AAF51B3, 0xB1F0, 0x11D1, 0xB6,0xAB, 0x00,0xA0,0xC9,0x08,0x33,0xE9);
DEFINE_GUID(CLSID_CHtmlComponentConstructor, 0x3050F4F8, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CImageBrowsePropertyPage, 0x3050F3B3, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CInlineStylePropertyPage, 0x3050F296, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CPeerHandler, 0x5AAF51B2, 0xB1F0, 0x11D1, 0xB6,0xAB, 0x00,0xA0,0xC9,0x08,0x33,0xE9);
DEFINE_GUID(CLSID_CRecalcEngine, 0x3050F499, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CSvrOMUses, 0x3050F4F0, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CrSource, 0x65014010, 0x9F62, 0x11D1, 0xA6,0x51, 0x00,0x60,0x08,0x11,0xD5,0xCE);
DEFINE_GUID(CLSID_ExternalFrameworkSite, 0x3050F163, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTADocument, 0x3050F5C8, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLLoadOptions, 0x18845040, 0x0FA5, 0x11D1, 0xBA,0x19, 0x00,0xC0,0x4F,0xD9,0x12,0xD0);
DEFINE_GUID(CLSID_HTMLPluginDocument, 0x25336921, 0x03F9, 0x11CF, 0x8F,0xD0, 0x00,0xAA,0x00,0x68,0x6F,0x13);
DEFINE_GUID(CLSID_HTMLPopup, 0x3050F667, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLPopupDoc, 0x3050F67D, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLServerDoc, 0x3050F4E7, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLWindowProxy, 0x3050F391, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_IImageDecodeFilter, 0x607FD4E8, 0x0A03, 0x11D1, 0xAB,0x1D, 0x00,0xC0,0x4F,0xC9,0xB3,0x04);
DEFINE_GUID(CLSID_IImgCtx, 0x3050F3D6, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_IntDitherer, 0x05F6FE1A, 0xECEF, 0x11D0, 0xAA,0xE7, 0x00,0xC0,0x4F,0xC9,0xB3,0x04);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MHTMLDocument, 0x3050F3D9, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_Scriptlet, 0xAE24FDAE, 0x03C6, 0x11D1, 0x8B,0x76, 0x00,0x80,0xC7,0x44,0xF3,0x89);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);
DEFINE_GUID(CLSID_TridentAPI, 0x429AF92C, 0xA51F, 0x11D2, 0x86,0x1E, 0x00,0xC0,0x4F,0xA3,0x5C,0x89);

#define INF_SET_CLSID(clsid) \
    pse[i].pszName = "CLSID_" #clsid; \
    clsids[i++] = &CLSID_ ## clsid;

static HRESULT register_server(BOOL do_register)
{
    HRESULT hres;
    HMODULE hAdvpack;
    typeof(RegInstall) *pRegInstall;
    STRTABLE strtable;
    STRENTRY pse[34];
    static CLSID const *clsids[34];
    int i = 0;
    
    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%x)\n", do_register);
    
    INF_SET_CLSID(AboutProtocol);
    INF_SET_CLSID(CAnchorBrowsePropertyPage);
    INF_SET_CLSID(CBackgroundPropertyPage);
    INF_SET_CLSID(CCDAnchorPropertyPage);
    INF_SET_CLSID(CCDGenericPropertyPage);
    INF_SET_CLSID(CDocBrowsePropertyPage);
    INF_SET_CLSID(CDwnBindInfo);
    INF_SET_CLSID(CHiFiUses);
    INF_SET_CLSID(CHtmlComponentConstructor);
    INF_SET_CLSID(CImageBrowsePropertyPage);
    INF_SET_CLSID(CInlineStylePropertyPage);
    INF_SET_CLSID(CPeerHandler);
    INF_SET_CLSID(CRecalcEngine);
    INF_SET_CLSID(CSvrOMUses);
    INF_SET_CLSID(CrSource);
    INF_SET_CLSID(ExternalFrameworkSite);
    INF_SET_CLSID(HTADocument);
    INF_SET_CLSID(HTMLDocument);
    INF_SET_CLSID(HTMLLoadOptions);
    INF_SET_CLSID(HTMLPluginDocument);
    INF_SET_CLSID(HTMLPopup);
    INF_SET_CLSID(HTMLPopupDoc);
    INF_SET_CLSID(HTMLServerDoc);
    INF_SET_CLSID(HTMLWindowProxy);
    INF_SET_CLSID(IImageDecodeFilter);
    INF_SET_CLSID(IImgCtx);
    INF_SET_CLSID(IntDitherer);
    INF_SET_CLSID(JSProtocol);
    INF_SET_CLSID(MHTMLDocument);
    INF_SET_CLSID(MailtoProtocol);
    INF_SET_CLSID(ResProtocol);
    INF_SET_CLSID(Scriptlet);
    INF_SET_CLSID(SysimageProtocol);
    INF_SET_CLSID(TridentAPI);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (typeof(RegInstall)*)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID

/***********************************************************************
 *          DllRegisterServer (MSHTML.@)
 */
HRESULT WINAPI MSHTML_DllRegisterServer(void)
{
    return register_server(TRUE);
}

/***********************************************************************
 *          DllUnregisterServer (MSHTML.@)
 */
HRESULT WINAPI MSHTML_DllUnregisterServer(void)
{
    return register_server(FALSE);
}
