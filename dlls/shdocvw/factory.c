/*
 * Implementation of class factory for IE Web Browser
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include <string.h>
#include <stdio.h>

#include "shdocvw.h"
#include "winreg.h"
#include "advpub.h"
#include "rpcproxy.h"
#include "isguids.h"

#include "winver.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the WebBrowser class factory
 */

typedef struct
{
    /* IUnknown fields */
    IClassFactory IClassFactory_iface;
    HRESULT (*cf)(LPUNKNOWN, REFIID, LPVOID *);
    LONG ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}


/**********************************************************************
 * WBCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI WBCF_QueryInterface(LPCLASSFACTORY iface,
                                          REFIID riid, LPVOID *ppobj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppobj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Not supported interface %s\n", debugstr_guid(riid));

    *ppobj = NULL;
    return E_NOINTERFACE;
}

/************************************************************************
 * WBCF_AddRef (IUnknown)
 */
static ULONG WINAPI WBCF_AddRef(LPCLASSFACTORY iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

/************************************************************************
 * WBCF_Release (IUnknown)
 */
static ULONG WINAPI WBCF_Release(LPCLASSFACTORY iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/************************************************************************
 * WBCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI WBCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                          REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *) iface;
    return This->cf(pOuter, riid, ppobj);
}

/************************************************************************
 * WBCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI WBCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("(%d)\n", dolock);

    if (dolock)
        SHDOCVW_LockModule();
    else
        SHDOCVW_UnlockModule();
    
    return S_OK;
}

static const IClassFactoryVtbl WBCF_Vtbl =
{
    WBCF_QueryInterface,
    WBCF_AddRef,
    WBCF_Release,
    WBCF_CreateInstance,
    WBCF_LockServer
};

static HRESULT get_ieframe_object(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HINSTANCE ieframe_instance;

    static HRESULT (WINAPI *ieframe_DllGetClassObject)(REFCLSID,REFIID,void**);

    if(!ieframe_DllGetClassObject) {
        ieframe_instance = get_ieframe_instance();
        if(!ieframe_instance)
            return CLASS_E_CLASSNOTAVAILABLE;

        ieframe_DllGetClassObject = (void*)GetProcAddress(ieframe_instance, "DllGetClassObject");
        if(!ieframe_DllGetClassObject)
            return CLASS_E_CLASSNOTAVAILABLE;
    }

    return ieframe_DllGetClassObject(rclsid, riid, ppv);
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    static IClassFactoryImpl WB1ClassFactory = {{&WBCF_Vtbl}, WebBrowserV1_Create};
    static IClassFactoryImpl WB2ClassFactory = {{&WBCF_Vtbl}, WebBrowserV2_Create};
    static IClassFactoryImpl TBLClassFactory = {{&WBCF_Vtbl}, TaskbarList_Create};

    TRACE("\n");

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid))
        return IClassFactory_QueryInterface(&WB2ClassFactory.IClassFactory_iface, riid, ppv);

    if(IsEqualGUID(&CLSID_WebBrowser_V1, rclsid))
        return IClassFactory_QueryInterface(&WB1ClassFactory.IClassFactory_iface, riid, ppv);

    if(IsEqualGUID(&CLSID_InternetShortcut, rclsid)
       || IsEqualGUID(&CLSID_CUrlHistory, rclsid))
        return get_ieframe_object(rclsid, riid, ppv);

    if(IsEqualGUID(&CLSID_TaskbarList, rclsid))
        return IClassFactory_QueryInterface(&TBLClassFactory.IClassFactory_iface, riid, ppv);

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

HRESULT register_class_object(BOOL do_reg)
{
    HRESULT hres;

    static DWORD cookie;
    static IClassFactoryImpl IEClassFactory = {{&WBCF_Vtbl}, InternetExplorer_Create};

    if(do_reg) {
        hres = CoRegisterClassObject(&CLSID_InternetExplorer,
                                     (IUnknown*)&IEClassFactory.IClassFactory_iface, CLSCTX_SERVER,
                                     REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED, &cookie);
        if (FAILED(hres)) {
            ERR("failed to register object %08x\n", hres);
            return hres;
        }

        hres = CoResumeClassObjects();
        if(SUCCEEDED(hres))
            return hres;

        ERR("failed to resume object %08x\n", hres);
    }

    return CoRevokeClassObject(cookie);
}

static HRESULT reg_install(LPCSTR section, STRTABLEA *strtable)
{
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    HMODULE hadvpack;
    HRESULT hres;

    static const WCHAR advpackW[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    hadvpack = LoadLibraryW(advpackW);
    pRegInstall = (void *)GetProcAddress(hadvpack, "RegInstall");

    hres = pRegInstall(shdocvw_hinstance, section, strtable);

    FreeLibrary(hadvpack);
    return hres;
}

#define INF_SET_CLSID(clsid)                  \
    do                                        \
    {                                         \
        static CHAR name[] = "CLSID_" #clsid; \
                                              \
        pse[i].pszName = name;                \
        clsids[i++] = &CLSID_ ## clsid;       \
    } while (0)

static HRESULT register_server(BOOL doregister)
{
    STRTABLEA strtable;
    STRENTRYA pse[3];
    static CLSID const *clsids[3];
    unsigned int i = 0;
    HRESULT hres;

    INF_SET_CLSID(Internet);
    INF_SET_CLSID(InternetExplorer);
    INF_SET_CLSID(InternetShortcut);

    for(i = 0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hres = reg_install(doregister ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID

/***********************************************************************
 *          DllRegisterServer (shdocvw.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hres;

    hres = __wine_register_resources( shdocvw_hinstance, NULL );
    if(FAILED(hres))
        return hres;

    return register_server(TRUE);
}

/***********************************************************************
 *          DllUnregisterServer (shdocvw.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hres;

    hres = register_server(FALSE);
    if(FAILED(hres))
        return hres;

    return __wine_unregister_resources( shdocvw_hinstance, NULL );
}

static BOOL check_native_ie(void)
{
    static const WCHAR cszPath[] = {'b','r','o','w','s','e','u','i','.','d','l','l',0};
    DWORD handle,size;
    BOOL ret = TRUE;

    size = GetFileVersionInfoSizeW(cszPath,&handle);
    if (size)
    {
        LPVOID buf;
        LPWSTR lpFileDescription;
        UINT dwBytes;
        static const WCHAR cszFD[] = {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o','\\','0','4','0','9','0','4','e','4','\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0};
        static const WCHAR cszWine[] = {'W','i','n','e',0};

        buf = HeapAlloc(GetProcessHeap(),0,size);
        GetFileVersionInfoW(cszPath,0,size,buf);

        if (VerQueryValueW(buf, cszFD, (LPVOID*)&lpFileDescription, &dwBytes) &&
            strstrW(lpFileDescription,cszWine))
                ret = FALSE;

        HeapFree(GetProcessHeap(), 0, buf);
    }

    return ret;
}

DWORD register_iexplore(BOOL doregister)
{
    HRESULT hres;
    if (check_native_ie())
    {
        TRACE("Native IE detected, not doing registration\n");
        return S_OK;
    }
    hres = reg_install(doregister ? "RegisterIE" : "UnregisterIE", NULL);
    return FAILED(hres);
}
