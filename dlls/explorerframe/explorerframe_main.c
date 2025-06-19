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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shobjidl.h"
#include "rpcproxy.h"

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
    TRACE("%p, 0x%lx, %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        explorerframe_hinstance = hinst;
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (ExplorerFrame.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("refCount is %ld\n", EFRAME_refCount);
    return EFRAME_refCount ? S_FALSE : S_OK;
}

/*************************************************************************
 * Implement the ExplorerFrame class factory
 */

typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*cf)(IUnknown*, REFIID, void**);
    LONG ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

/*************************************************************************
 * EFCF_QueryInterface
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
 * EFCF_AddRef
 */
static ULONG WINAPI EFCF_AddRef(IClassFactory *iface)
{
    EFRAME_LockModule();

    return 2; /* non-heap based object */
}

/*************************************************************************
 * EFCF_Release
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
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
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
    static IClassFactoryImpl NSTCClassFactory = {{&EFCF_Vtbl}, NamespaceTreeControl_Constructor};
    static IClassFactoryImpl TaskbarListFactory = {{&EFCF_Vtbl}, TaskbarList_Constructor};

    TRACE("%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(&CLSID_NamespaceTreeControl, rclsid))
        return IClassFactory_QueryInterface(&NSTCClassFactory.IClassFactory_iface, riid, ppv);

    if(IsEqualGUID(&CLSID_TaskbarList, rclsid))
        return IClassFactory_QueryInterface(&TaskbarListFactory.IClassFactory_iface, riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}
