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

static REFIID tid_ids[] = {
#define XIID(iface) &IID_ ## iface,
#define CTID(name) &CLSID_ ## name,
TID_LIST
#undef XIID
#undef CTID
};

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static HRESULT load_typelib(void)
{
    ITypeLib *tl;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_WMPLib, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hr)) {
        ERR("LoadRegTypeLib failed: %08lx\n", hr);
        return hr;
    }

    if (InterlockedCompareExchangePointer((void **)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hr;
}

HRESULT get_typeinfo(typeinfo_id tid, ITypeInfo **typeinfo)
{
    HRESULT hr;

    if (!typelib)
        hr = load_typelib();
    if (!typelib)
        return hr;

    if (!typeinfos[tid]) {
        ITypeInfo *ti;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hr)) {
            ERR("GetTypeInfoOfGuid (%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void **)(typeinfos + tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(*typeinfo);
    return S_OK;
}

static void release_typelib(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    if (typelib)
        ITypeLib_Release(typelib);
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
    TRACE("(%p %ld %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        wmp_instance = hInstDLL;
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        unregister_wmp_class();
        unregister_player_msg_class();
        release_typelib();
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
