/*
 *
 * Copyright 2011 Alistair Leslie-Hughes
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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"

#include "wine/debug.h"

#include "initguid.h"
#include "mmc.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmc);

static HRESULT WINAPI mmcversion_QueryInterface(IMMCVersionInfo *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    if ( IsEqualGUID( riid, &IID_IMMCVersionInfo ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppv = iface;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IMMCVersionInfo_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI mmcversion_AddRef(IMMCVersionInfo *iface)
{
    return 2;
}

static ULONG WINAPI mmcversion_Release(IMMCVersionInfo *iface)
{
   return 1;
}

static HRESULT WINAPI mmcversion_GetMMCVersion(IMMCVersionInfo *iface, LONG *pVersionMajor, LONG *pVersionMinor)
{
    TRACE("(%p, %p, %p): stub\n", iface, pVersionMajor, pVersionMinor);

    if(pVersionMajor)
        *pVersionMajor = 3;

    if(pVersionMinor)
        *pVersionMinor = 0;

    return S_OK;
}

static const struct IMMCVersionInfoVtbl mmcversionVtbl =
{
    mmcversion_QueryInterface,
    mmcversion_AddRef,
    mmcversion_Release,
    mmcversion_GetMMCVersion
};

static IMMCVersionInfo mmcVersionInfo = { &mmcversionVtbl };

/***********************************************************
 *    ClassFactory implementation
 */
typedef HRESULT (*CreateInstanceFunc)(IUnknown*,REFIID,void**);

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFGUID riid, void **ppvObject)
{
    if(IsEqualGUID(&IID_IClassFactory, riid) || IsEqualGUID(&IID_IUnknown, riid)) {
        IClassFactory_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    WARN("not supported iid %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
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

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);
    return IMMCVersionInfo_QueryInterface(&mmcVersionInfo, riid, ppv);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl MMCClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory MMCVersionInfoFactory = { &MMCClassFactoryVtbl };

HRESULT WINAPI DllGetClassObject( REFCLSID riid, REFIID iid, LPVOID *ppv )
{
    TRACE("%s %s %p\n", debugstr_guid(riid), debugstr_guid(iid), ppv );

    if( IsEqualCLSID( riid, &CLSID_MMCVersionInfo ))
    {
        TRACE("(CLSID_MMCVersionInfo %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&MMCVersionInfoFactory, iid, ppv);
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
