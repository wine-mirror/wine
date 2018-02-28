/*
 * Active Directory services LDAP Provider
 *
 * Copyright 2018 Dmitry Timoshkov
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
#include "initguid.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "iads.h"
#define SECURITY_WIN32
#include "security.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(adsldp);

static HMODULE adsldp_hinst;

typedef struct
{
    IADsADSystemInfo IADsADSystemInfo_iface;
    LONG ref;
} AD_sysinfo;

static inline AD_sysinfo *impl_from_IADsADSystemInfo(IADsADSystemInfo *iface)
{
    return CONTAINING_RECORD(iface, AD_sysinfo, IADsADSystemInfo_iface);
}

static HRESULT WINAPI sysinfo_QueryInterface(IADsADSystemInfo *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IADsADSystemInfo) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IADsADSystemInfo_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI sysinfo_AddRef(IADsADSystemInfo *iface)
{
    AD_sysinfo *sysinfo = impl_from_IADsADSystemInfo(iface);
    return InterlockedIncrement(&sysinfo->ref);
}

static ULONG WINAPI sysinfo_Release(IADsADSystemInfo *iface)
{
    AD_sysinfo *sysinfo = impl_from_IADsADSystemInfo(iface);
    LONG ref = InterlockedDecrement(&sysinfo->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        HeapFree(GetProcessHeap(), 0, sysinfo);
    }

    return ref;
}

static HRESULT WINAPI sysinfo_GetTypeInfoCount(IADsADSystemInfo *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetTypeInfo(IADsADSystemInfo *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#x,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetIDsOfNames(IADsADSystemInfo *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_Invoke(IADsADSystemInfo *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_UserName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_ComputerName(IADsADSystemInfo *iface, BSTR *retval)
{
    UINT size;
    WCHAR *name;

    TRACE("%p,%p\n", iface, retval);

    size = 0;
    GetComputerObjectNameW(NameFullyQualifiedDN, NULL, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(GetLastError());

    name = SysAllocStringLen(NULL, size);
    if (!name) return E_OUTOFMEMORY;

    if (!GetComputerObjectNameW(NameFullyQualifiedDN, name, &size))
    {
        SysFreeString(name);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *retval = name;
    return S_OK;
}

static HRESULT WINAPI sysinfo_get_SiteName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_DomainShortName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_DomainDNSName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_ForestDNSName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_PDCRoleOwner(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_SchemaRoleOwner(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_IsNativeMode(IADsADSystemInfo *iface, VARIANT_BOOL *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetAnyDCName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetDCSiteName(IADsADSystemInfo *iface, BSTR server, BSTR *retval)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(server), retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_RefreshSchemaCache(IADsADSystemInfo *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetTrees(IADsADSystemInfo *iface, VARIANT *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static const IADsADSystemInfoVtbl IADsADSystemInfo_vtbl =
{
    sysinfo_QueryInterface,
    sysinfo_AddRef,
    sysinfo_Release,
    sysinfo_GetTypeInfoCount,
    sysinfo_GetTypeInfo,
    sysinfo_GetIDsOfNames,
    sysinfo_Invoke,
    sysinfo_get_UserName,
    sysinfo_get_ComputerName,
    sysinfo_get_SiteName,
    sysinfo_get_DomainShortName,
    sysinfo_get_DomainDNSName,
    sysinfo_get_ForestDNSName,
    sysinfo_get_PDCRoleOwner,
    sysinfo_get_SchemaRoleOwner,
    sysinfo_get_IsNativeMode,
    sysinfo_GetAnyDCName,
    sysinfo_GetDCSiteName,
    sysinfo_RefreshSchemaCache,
    sysinfo_GetTrees
};

static HRESULT ADSystemInfo_create(void **obj)
{
    AD_sysinfo *sysinfo;

    sysinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*sysinfo));
    if (!sysinfo) return E_OUTOFMEMORY;

    sysinfo->IADsADSystemInfo_iface.lpVtbl = &IADsADSystemInfo_vtbl;
    sysinfo->ref = 1;
    *obj = &sysinfo->IADsADSystemInfo_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*constructor)(void **);
} ADSystemInfo_factory;

static inline ADSystemInfo_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ADSystemInfo_factory, IClassFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    ADSystemInfo_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unknown;
    HRESULT hr;

    TRACE("%p,%s,%p\n", outer, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    hr = factory->constructor((void **)&unknown);
    if (hr == S_OK)
    {
        hr = IUnknown_QueryInterface(unknown, riid, obj);
        IUnknown_Release(unknown);
    }
    return hr;
}

static HRESULT WINAPI factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("%p,%d: stub\n", iface, lock);
    return S_OK;
}

static const struct IClassFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static ADSystemInfo_factory ADSystemInfo_cf = { { &factory_vtbl }, ADSystemInfo_create };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    TRACE("%s,%s,%p\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (!clsid || !iid || !obj) return E_INVALIDARG;

    *obj = NULL;

    if (IsEqualGUID(clsid, &CLSID_ADSystemInfo))
        return IClassFactory_QueryInterface(&ADSystemInfo_cf.IClassFactory_iface, iid, obj);

    FIXME("class %s/%s is not implemented\n", debugstr_guid(clsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(adsldp_hinst);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(adsldp_hinst);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("%p,%u,%p\n", hinst, reason, reserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */

    case DLL_PROCESS_ATTACH:
        adsldp_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }

    return TRUE;
}
