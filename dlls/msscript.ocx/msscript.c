/*
 * Copyright 2015 Jacek Caban for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "msscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msscript);

static HINSTANCE msscript_instance;

HRESULT WINAPI ScriptControl_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    FIXME("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

/******************************************************************
 *              DllMain (msscript.ocx.@)
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", instance, reason, lpv);

    switch(reason) {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        msscript_instance = instance;
        DisableThreadLibraryCalls(instance);
        break;
    }

    return TRUE;
}

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

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
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

static const IClassFactoryVtbl ScriptControlFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ScriptControl_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ScriptControlFactory = { &ScriptControlFactoryVtbl };

/***********************************************************************
 *		DllGetClassObject	(msscript.ocx.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_ScriptControl, rclsid)) {
        TRACE("(CLSID_ScriptControl %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&ScriptControlFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (msscript.ocx.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer (msscript.ocx.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(msscript_instance);
}

/***********************************************************************
 *          DllUnregisterServer (msscript.ocx.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(msscript_instance);
}
