/*
 *    Word splitter Implementation
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


#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "indexsrv.h"
#include "initguid.h"
#include "infosoft.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(infosoft);

extern HRESULT WINAPI wb_Constructor(IUnknown*, REFIID, LPVOID *);

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown*, REFIID, LPVOID*);

typedef struct
{
    IClassFactory      IClassFactory_iface;
    LPFNCREATEINSTANCE lpfnCI;
} CFImpl;

static inline CFImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, CFImpl, IClassFactory_iface);
}

static HRESULT WINAPI infosoftcf_fnQueryInterface ( LPCLASSFACTORY iface,
        REFIID riid, LPVOID *ppvObj)
{
    CFImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%s)\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->IClassFactory_iface;
        return S_OK;
    }

    TRACE("-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI infosoftcf_fnAddRef (LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI infosoftcf_fnRelease(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI infosoftcf_fnCreateInstance( LPCLASSFACTORY iface,
        LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    CFImpl *This = impl_from_IClassFactory(iface);

    TRACE("%p->(%p,%s,%p)\n", This, pUnkOuter, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    return This->lpfnCI(pUnkOuter, riid, ppvObject);
}

static HRESULT WINAPI infosoftcf_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("%p %d\n", iface, fLock);
    return E_NOTIMPL;
}

static const IClassFactoryVtbl infosoft_cfvt =
{
    infosoftcf_fnQueryInterface,
    infosoftcf_fnAddRef,
    infosoftcf_fnRelease,
    infosoftcf_fnCreateInstance,
    infosoftcf_fnLockServer
};

static CFImpl wb_cf = { { &infosoft_cfvt }, wb_Constructor };

/***********************************************************************
 *             DllGetClassObject (INFOSOFT.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    IClassFactory   *pcf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (IsEqualIID(rclsid, &CLSID_wb_Neutral))
        pcf = &wb_cf.IClassFactory_iface;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(pcf, iid, ppv);
}
