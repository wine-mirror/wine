/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#include "initguid.h"
#include "wmp_private.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmp);

HINSTANCE wmp_instance;
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

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

static const IClassFactoryVtbl WMPFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WMPFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory WMPFactory = { &WMPFactoryVtbl };

/******************************************************************
 *              DllMain (wmp.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason) {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        wmp_instance = hInstDLL;
        break;
    case DLL_PROCESS_DETACH:
        unregister_wmp_class();
        unregister_player_msg_class();
        break;
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(wmp.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_WindowsMediaPlayer, rclsid)) {
        TRACE("(CLSID_WindowsMediaPlayer %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WMPFactory, riid, ppv);
    }

    FIXME("Unknown object %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (wmp.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer (wmp.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(wmp_instance);
}

/***********************************************************************
 *          DllUnregisterServer (wmp.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(wmp_instance);
}
