/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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

#include "initguid.h"
#include "wpcapi.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpc);

static HRESULT WINAPI WindowsParentalControls_QueryInterface(IWindowsParentalControls *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWindowsParentalControlsCore)) {
        TRACE("(IID_IWindowsParentalControlsCore %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWindowsParentalControls)) {
        TRACE("(IID_IWindowsParentalControls %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("unsupported iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WindowsParentalControls_AddRef(IWindowsParentalControls *iface)
{
    return 2;
}

static ULONG WINAPI WindowsParentalControls_Release(IWindowsParentalControls *iface)
{
    return 1;
}

static HRESULT WINAPI WindowsParentalControls_GetVisibility(IWindowsParentalControls *iface, WPCFLAG_VISIBILITY *visibility)
{
    FIXME("(%p)\n", visibility);
    return E_NOTIMPL;
}

static HRESULT WINAPI WindowsParentalControls_GetUserSettings(IWindowsParentalControls *iface, const WCHAR *sid, IWPCSettings **settings)
{
    FIXME("(%s %p)\n", debugstr_w(sid), settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI WindowsParentalControls_GetWebSettings(IWindowsParentalControls *iface, const WCHAR *sid, IWPCWebSettings **settings)
{
    FIXME("(%s %p)\n", debugstr_w(sid), settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI WindowsParentalControls_GetWebFilterInfo(IWindowsParentalControls *iface, GUID *id, WCHAR **name)
{
    FIXME("(%p %p)\n", id, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI WindowsParentalControls_GetGamesSettings(IWindowsParentalControls *iface, const WCHAR *sid, IWPCGamesSettings **settings)
{
    FIXME("(%s %p)\n", debugstr_w(sid), settings);
    return E_NOTIMPL;
}

static const IWindowsParentalControlsVtbl WindowsParentalControlsVtbl = {
    WindowsParentalControls_QueryInterface,
    WindowsParentalControls_AddRef,
    WindowsParentalControls_Release,
    WindowsParentalControls_GetVisibility,
    WindowsParentalControls_GetUserSettings,
    WindowsParentalControls_GetWebSettings,
    WindowsParentalControls_GetWebFilterInfo,
    WindowsParentalControls_GetGamesSettings
};

static HRESULT WINAPI WindowsParentalControls_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    static IWindowsParentalControls wpc = { &WindowsParentalControlsVtbl };

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IWindowsParentalControls_QueryInterface(&wpc, riid, ppv);
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

static const IClassFactoryVtbl WPCFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WindowsParentalControls_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory WPCFactory = { &WPCFactoryVtbl };

/***********************************************************************
 *		DllGetClassObject	(wpc.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&CLSID_WindowsParentalControls, rclsid)) {
        TRACE("(CLSID_WindowsParentalControls %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WPCFactory, riid, ppv);
    }

    FIXME("Unknown object %s (iface %s)\n", debugstr_guid(rclsid), debugstr_guid(riid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
