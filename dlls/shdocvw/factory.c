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
    TRACE("\n");

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid)
       || IsEqualGUID(&CLSID_WebBrowser_V1, rclsid)
       || IsEqualGUID(&CLSID_InternetShortcut, rclsid)
       || IsEqualGUID(&CLSID_CUrlHistory, rclsid)
       || IsEqualGUID(&CLSID_TaskbarList, rclsid))
        return get_ieframe_object(rclsid, riid, ppv);

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
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

static DWORD register_iexplore(BOOL doregister)
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

/******************************************************************
 *		IEWinMain            (SHDOCVW.101)
 *
 * Only returns on error.
 */
DWORD WINAPI IEWinMain(LPSTR szCommandLine, int nShowWindow)
{
    DWORD (WINAPI *pIEWinMain)(LPSTR,int);

    TRACE("%s %d\n", debugstr_a(szCommandLine), nShowWindow);

    if(*szCommandLine == '-' || *szCommandLine == '/') {
        if(!strcasecmp(szCommandLine+1, "regserver"))
            return register_iexplore(TRUE);
        if(!strcasecmp(szCommandLine+1, "unregserver"))
            return register_iexplore(FALSE);
    }

    pIEWinMain = (void*)GetProcAddress(get_ieframe_instance(), MAKEINTRESOURCEA(101));
    if(!pIEWinMain)
        ExitProcess(1);

    return pIEWinMain(szCommandLine, nShowWindow);
}
