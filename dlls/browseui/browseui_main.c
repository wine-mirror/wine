/*
 * browseui - Internet Explorer / Windows Explorer standard UI
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
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

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "rpcproxy.h"

#include "browseui.h"

#include "initguid.h"
DEFINE_GUID(CLSID_CompCatCacheDaemon, 0x8C7461EF, 0x2b13, 0x11d2, 0xbe, 0x35, 0x30, 0x78, 0x30, 0x2c, 0x20, 0x30);

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

LONG BROWSEUI_refCount = 0;

HINSTANCE BROWSEUI_hinstance = 0;

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    REFCLSID clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_ACLMulti, ACLMulti_Constructor},
    {&CLSID_ProgressDialog, ProgressDialog_Constructor},
    {&CLSID_CompCatCacheDaemon, CompCatCacheDaemon_Constructor},
    {&CLSID_ACListISF, ACLShellSource_Constructor},
    {NULL, NULL}
};

typedef struct tagClassFactory
{
    IClassFactory IClassFactory_iface;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;

static inline ClassFactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactory, IClassFactory_iface);
}

static void ClassFactory_Destructor(ClassFactory *This)
{
    TRACE("Destroying class factory %p\n", This);
    free(This);
    InterlockedDecrement(&BROWSEUI_refCount);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown)) {
        IClassFactory_AddRef(iface);
        *ppvOut = iface;
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    ULONG ret = InterlockedDecrement(&This->ref);

    if (ret == 0)
        ClassFactory_Destructor(This);
    return ret;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID iid, LPVOID *ppvOut)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    HRESULT ret;
    IUnknown *obj;

    TRACE("(%p, %p, %s, %p)\n", iface, punkOuter, debugstr_guid(iid), ppvOut);
    ret = This->ctor(punkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = IUnknown_QueryInterface(obj, iid, ppvOut);
    IUnknown_Release(obj);
    return ret;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    ClassFactory *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%x)\n", This, fLock);

    if(fLock)
        InterlockedIncrement(&BROWSEUI_refCount);
    else
        InterlockedDecrement(&BROWSEUI_refCount);

    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    /* IUnknown */
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,

    /* IClassFactory*/
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    ClassFactory *This = malloc(sizeof(*This));
    This->IClassFactory_iface.lpVtbl = &ClassFactoryVtbl;
    This->ref = 1;
    This->ctor = ctor;
    *ppvOut = &This->IClassFactory_iface;
    TRACE("Created class factory %p\n", This);
    InterlockedIncrement(&BROWSEUI_refCount);
    return S_OK;
}

/*************************************************************************
 * BROWSEUI DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            BROWSEUI_hinstance = hinst;
            break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (BROWSEUI.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return BROWSEUI_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetVersion (BROWSEUI.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if(info->cbSize == sizeof(DLLVERSIONINFO) ||
       info->cbSize == sizeof(DLLVERSIONINFO2))
    {
        /* this is what IE6 on Windows 98 reports */
        info->dwMajorVersion = 6;
        info->dwMinorVersion = 0;
        info->dwBuildNumber = 2600;
        info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;
        if(info->cbSize == sizeof(DLLVERSIONINFO2))
        {
            DLLVERSIONINFO2 *info2 = (DLLVERSIONINFO2*) info;
            info2->dwFlags = 0;
            info2->ullVersion = MAKEDLLVERULL(info->dwMajorVersion,
                                              info->dwMinorVersion,
                                              info->dwBuildNumber,
                                              0); /* FIXME: correct hotfix number */
        }
        return S_OK;
    }

    WARN("wrong DLLVERSIONINFO size from app.\n");
    return E_INVALIDARG;
}

/***********************************************************************
 *              DllGetClassObject (BROWSEUI.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppvOut)
{
    int i;

    *ppvOut = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IClassFactory))
        return E_NOINTERFACE;

    for (i = 0; ClassesTable[i].clsid != NULL; i++)
        if (IsEqualCLSID(ClassesTable[i].clsid, clsid)) {
            return ClassFactory_Constructor(ClassesTable[i].ctor, ppvOut);
        }
    FIXME("CLSID %s not supported\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *  DllInstall (BROWSEUI.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("(%s, %s): stub\n", bInstall ? "TRUE" : "FALSE", debugstr_w(cmdline));
    return S_OK;
}
