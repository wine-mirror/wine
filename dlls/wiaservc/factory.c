/*
 * Class factory interface for WiaServc
 *
 * Copyright (C) 2009 Damjan Jovanovic
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

#include "objbase.h"
#include "winuser.h"
#include "winreg.h"
#include "initguid.h"
#include "wia_lh.h"

#include "wiaservc_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wia);

static inline ClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactoryImpl, IClassFactory_iface);
}

static ULONG WINAPI
WIASERVC_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static HRESULT WINAPI
WIASERVC_IClassFactory_QueryInterface(LPCLASSFACTORY iface, REFIID riid,
                                      LPVOID *ppvObj)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->IClassFactory_iface;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI
WIASERVC_IClassFactory_Release(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI
WIASERVC_IClassFactory_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter,
                                      REFIID riid, LPVOID *ppvObj)
{
    HRESULT res;
    IWiaDevMgr *devmgr = NULL;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    res = wiadevmgr_Constructor(&devmgr);
    if (FAILED(res))
        return res;

    res = IWiaDevMgr_QueryInterface(devmgr, riid, ppvObj);
    IWiaDevMgr_Release(devmgr);
    return res;
}

static HRESULT WINAPI
WIASERVC_IClassFactory_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const IClassFactoryVtbl WIASERVC_IClassFactory_Vtbl =
{
    WIASERVC_IClassFactory_QueryInterface,
    WIASERVC_IClassFactory_AddRef,
    WIASERVC_IClassFactory_Release,
    WIASERVC_IClassFactory_CreateInstance,
    WIASERVC_IClassFactory_LockServer
};

ClassFactoryImpl WIASERVC_ClassFactory =
{
    { &WIASERVC_IClassFactory_Vtbl }
};
