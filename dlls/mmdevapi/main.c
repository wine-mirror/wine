/*
 * Copyright 2009 Maarten Lankhorst
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

#include "config.h"

#include <stdarg.h>

#define CINTERFACE
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "initguid.h"
#include "ole2.h"
#include "mmdeviceapi.h"

#include "mmdevapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("stub\n");
    return S_FALSE;
}

typedef HRESULT (*FnCreateInstance)(REFIID riid, LPVOID *ppobj);

typedef struct {
    const IClassFactoryVtbl *lpVtbl;
    REFCLSID rclsid;
    FnCreateInstance pfnCreateInstance;
} IClassFactoryImpl;

static HRESULT WINAPI
MMCF_QueryInterface(LPCLASSFACTORY iface, REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);
    if (ppobj == NULL)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppobj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MMCF_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI MMCF_Release(LPCLASSFACTORY iface)
{
    /* static class, won't be freed */
    return 1;
}

static HRESULT WINAPI MMCF_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pOuter,
    REFIID riid,
    LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p, %p, %s, %p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_POINTER;
    }
    *ppobj = NULL;
    return This->pfnCreateInstance(riid, ppobj);
}

static HRESULT WINAPI MMCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p, %d) stub!\n", This, dolock);
    return S_OK;
}

static const IClassFactoryVtbl MMCF_Vtbl = {
    MMCF_QueryInterface,
    MMCF_AddRef,
    MMCF_Release,
    MMCF_CreateInstance,
    MMCF_LockServer
};

static IClassFactoryImpl MMDEVAPI_CF[] = {
    { &MMCF_Vtbl, &CLSID_MMDeviceEnumerator, (FnCreateInstance)MMDevEnum_Create }
};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i = 0;
    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (ppv == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualIID(riid, &IID_IClassFactory) &&
        !IsEqualIID(riid, &IID_IUnknown)) {
        WARN("no interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    for (i = 0; i < sizeof(MMDEVAPI_CF)/sizeof(MMDEVAPI_CF[0]); ++i)
    {
        if (IsEqualGUID(rclsid, MMDEVAPI_CF[i].rclsid)) {
            IUnknown_AddRef((IClassFactory*) &MMDEVAPI_CF[i]);
            *ppv = &MMDEVAPI_CF[i];
            return S_OK;
        }
        i++;
    }

    WARN("(%s, %s, %p): no class found.\n", debugstr_guid(rclsid),
         debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
