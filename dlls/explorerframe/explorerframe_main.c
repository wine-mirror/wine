/*
 * ExplorerFrame main functions
 *
 * Copyright 2010 David Hedberg
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "advpub.h"
#include "shobjidl.h"

#include "wine/debug.h"

#include "explorerframe_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorerframe);

HINSTANCE explorerframe_hinstance;
LONG EFRAME_refCount = 0;

/*************************************************************************
 *              DllMain (ExplorerFrame.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p, 0x%x, %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        explorerframe_hinstance = hinst;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (ExplorerFrame.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("refCount is %d\n", EFRAME_refCount);
    return EFRAME_refCount ? S_FALSE : S_OK;
}

/*************************************************************************
 *              DllGetVersion (ExplorerFrame.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    TRACE("%p\n", info);
    if(info->cbSize == sizeof(DLLVERSIONINFO) ||
       info->cbSize == sizeof(DLLVERSIONINFO2))
    {
        /* Windows 7 */
        info->dwMajorVersion = 6;
        info->dwMinorVersion = 1;
        info->dwBuildNumber = 7600;
        info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;
        if(info->cbSize == sizeof(DLLVERSIONINFO2))
        {
            DLLVERSIONINFO2 *info2 = (DLLVERSIONINFO2*)info;
            info2->dwFlags = 0;
            info2->ullVersion = MAKEDLLVERULL(info->dwMajorVersion,
                                              info->dwMinorVersion,
                                              info->dwBuildNumber,
                                              16385); /* "hotfix number" */
        }
        return S_OK;
    }

    WARN("wrong DLLVERSIONINFO size from app.\n");
    return E_INVALIDARG;
}

/*************************************************************************
 * Implement the ExplorerFrame class factory
 *
 * (Taken from shdocvw/factory.c; based on implementation in
 *  ddraw/main.c)
 */

#define FACTORY(x) ((IClassFactory*) &(x)->lpClassFactoryVtbl)

typedef struct
{
    const IClassFactoryVtbl *lpClassFactoryVtbl;
    HRESULT (*cf)(IUnknown*, REFIID, void**);
    LONG ref;
} IClassFactoryImpl;

/*************************************************************************
 * EFCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI EFCF_QueryInterface(IClassFactory* iface,
                                          REFIID riid, void **ppobj)
{
    TRACE("%p (%s %p)\n", iface, debugstr_guid(riid), ppobj);

    if(!ppobj)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid))
    {
        *ppobj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Interface not supported.\n");

    *ppobj = NULL;
    return E_NOINTERFACE;
}

/*************************************************************************
 * EFCF_AddRef (IUnknown)
 */
static ULONG WINAPI EFCF_AddRef(IClassFactory *iface)
{
    EFRAME_LockModule();

    return 2; /* non-heap based object */
}

/*************************************************************************
 * EFCF_Release (IUnknown)
 */
static ULONG WINAPI EFCF_Release(IClassFactory *iface)
{
    EFRAME_UnlockModule();

    return 1; /* non-heap based object */
}

/*************************************************************************
 * EFCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI EFCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return This->cf(pOuter, riid, ppobj);
}

/*************************************************************************
 * EFCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI EFCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("%p (%d)\n", iface, dolock);

    if (dolock)
        EFRAME_LockModule();
    else
        EFRAME_UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl EFCF_Vtbl =
{
    EFCF_QueryInterface,
    EFCF_AddRef,
    EFCF_Release,
    EFCF_CreateInstance,
    EFCF_LockServer
};

/*************************************************************************
 *              DllGetClassObject (ExplorerFrame.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    static IClassFactoryImpl NSTCClassFactory = {&EFCF_Vtbl, NamespaceTreeControl_Constructor};

    TRACE("%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(&CLSID_NamespaceTreeControl, rclsid))
        return IClassFactory_QueryInterface(FACTORY(&NSTCClassFactory), riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

/*************************************************************************
 *          Register/Unregister DLL, based on shdocvw/factory.c
 */
static HRESULT reg_install(LPCSTR section, const STRTABLEA *strtable)
{
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    HMODULE hadvpack;
    HRESULT hres;

    static const WCHAR advpackW[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    hadvpack = LoadLibraryW(advpackW);
    pRegInstall = (void *)GetProcAddress(hadvpack, "RegInstall");

    hres = pRegInstall(explorerframe_hinstance, section, strtable);

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
    STRENTRYA pse[1];
    static CLSID const *clsids[1];
    unsigned int i = 0;
    HRESULT hres;

    INF_SET_CLSID(NamespaceTreeControl);

    for(i = 0; i < sizeof(pse)/sizeof(pse[0]); i++)
    {
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

/*************************************************************************
 *          DllRegisterServer (ExplorerFrame.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return register_server(TRUE);
}

/*************************************************************************
 *          DllUnregisterServer (ExplorerFrame.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return register_server(FALSE);
}
