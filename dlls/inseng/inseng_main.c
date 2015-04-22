/*
 *    INSENG Implementation
 *
 * Copyright 2006 Mike McCormack
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "inseng.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inseng);

static HINSTANCE instance;

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
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static HRESULT WINAPI ActiveSetupEngCF_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    FIXME("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IClassFactoryVtbl ActiveSetupEngCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ActiveSetupEngCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ActiveSetupEngCF = { &ActiveSetupEngCFVtbl };

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        break;
    }
    return TRUE;
}

/***********************************************************************
 *             DllGetClassObject (INSENG.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    if(IsEqualGUID(rclsid, &CLSID_InstallEngine)) {
        TRACE("(CLSID_InstallEngine %s %p)\n", debugstr_guid(iid), ppv);
        return IClassFactory_QueryInterface(&ActiveSetupEngCF, iid, ppv);
    }

    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllCanUnloadNow (INSENG.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (INSENG.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (INSENG.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}

BOOL WINAPI CheckTrustEx( LPVOID a, LPVOID b, LPVOID c, LPVOID d, LPVOID e )
{
    FIXME("%p %p %p %p %p\n", a, b, c, d, e );
    return TRUE;
}

/***********************************************************************
 *  DllInstall (INSENG.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("(%s, %s): stub\n", bInstall ? "TRUE" : "FALSE", debugstr_w(cmdline));
    return S_OK;
}
