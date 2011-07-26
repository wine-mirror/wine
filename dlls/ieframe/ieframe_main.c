/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "ieframe.h"

#include "shlguid.h"
#include "isguids.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

LONG module_ref = 0;

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl InternetShortcutFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InternetShortcut_Create,
    ClassFactory_LockServer
};

static IClassFactory InternetShortcutFactory = { &InternetShortcutFactoryVtbl };

static const IClassFactoryVtbl CUrlHistoryFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    CUrlHistory_Create,
    ClassFactory_LockServer
};

static IClassFactory CUrlHistoryFactory = { &CUrlHistoryFactoryVtbl };

static const IClassFactoryVtbl TaskbarListFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    TaskbarList_Create,
    ClassFactory_LockServer
};

static IClassFactory TaskbarListFactory = { &TaskbarListFactoryVtbl };

/******************************************************************
 *              DllMain (ieframe.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
         DisableThreadLibraryCalls(hInstDLL);
        break;
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(ieframe.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(rclsid, &CLSID_InternetShortcut)) {
        TRACE("(CLSID_InternetShortcut %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&InternetShortcutFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_CUrlHistory, rclsid)) {
        TRACE("(CLSID_CUrlHistory %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&CUrlHistoryFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_TaskbarList, rclsid)) {
        TRACE("(CLSID_TaskbarList %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&TaskbarListFactory, riid, ppv);
    }


    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (ieframe.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("()\n");
    return module_ref ? S_FALSE : S_OK;
}

/***********************************************************************
 *          DllRegisterServer (ieframe.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}

/***********************************************************************
 *          DllUnregisterServer (ieframe.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}
