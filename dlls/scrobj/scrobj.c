/*
 * Copyright 2017 Nikolay Sivov
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

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "olectl.h"
#include "rpcproxy.h"

#include "initguid.h"
#include "scrobj.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrobj);

static HINSTANCE scrobj_instance;

typedef enum tid_t
{
    NULL_tid,
    IGenScriptletTLib_tid,
    LAST_tid
} tid_t;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_NULL,
    &IID_IGenScriptletTLib,
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    if (typelib)
        return S_OK;

    hres = LoadRegTypeLib(&LIBID_Scriptlet, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hres))
    {
        ERR("LoadRegTypeLib failed: %08x\n", hres);
        return hres;
    }

    if (InterlockedCompareExchangePointer((void **)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (FAILED(hres = load_typelib()))
        return hres;

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if (InterlockedCompareExchangePointer((void **)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

static void release_typelib(void)
{
    unsigned i;

    if (!typelib)
        return;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

struct scriptlet_typelib
{
    IGenScriptletTLib IGenScriptletTLib_iface;
    LONG ref;

    BSTR guid;
};

static inline struct scriptlet_typelib *impl_from_IGenScriptletTLib(IGenScriptletTLib *iface)
{
    return CONTAINING_RECORD(iface, struct scriptlet_typelib, IGenScriptletTLib_iface);
}

static HRESULT WINAPI scriptlet_typelib_QueryInterface(IGenScriptletTLib *iface, REFIID riid, void **obj)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IGenScriptletTLib) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IGenScriptletTLib_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI scriptlet_typelib_AddRef(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI scriptlet_typelib_Release(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->guid);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI scriptlet_typelib_GetTypeInfoCount(IGenScriptletTLib *iface, UINT *count)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %p)\n", This, count);

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI scriptlet_typelib_GetTypeInfo(IGenScriptletTLib *iface, UINT index, LCID lcid,
    ITypeInfo **tinfo)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %u, %x, %p)\n", This, index, lcid, tinfo);

    return get_typeinfo(IGenScriptletTLib_tid, tinfo);
}

static HRESULT WINAPI scriptlet_typelib_GetIDsOfNames(IGenScriptletTLib *iface, REFIID riid, LPOLESTR *names,
    UINT cNames, LCID lcid, DISPID *dispid)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p, %s, %p, %u, %x, %p)\n", This, debugstr_guid(riid), names, cNames, lcid, dispid);

    hr = get_typeinfo(IGenScriptletTLib_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, cNames, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI scriptlet_typelib_Invoke(IGenScriptletTLib *iface, DISPID dispid, REFIID riid,
    LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *argerr)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p, %d, %s, %x, %x, %p, %p, %p, %p)\n", This, dispid, debugstr_guid(riid), lcid, flags,
        params, result, ei, argerr);

    hr = get_typeinfo(IGenScriptletTLib_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IGenScriptletTLib_iface, dispid, flags,
                params, result, ei, argerr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI scriptlet_typelib_AddURL(IGenScriptletTLib *iface, BSTR url)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(url));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Path(IGenScriptletTLib *iface, BSTR path)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Path(IGenScriptletTLib *iface, BSTR *path)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Doc(IGenScriptletTLib *iface, BSTR doc)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(doc));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Doc(IGenScriptletTLib *iface, BSTR *doc)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, doc);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Name(IGenScriptletTLib *iface, BSTR name)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Name(IGenScriptletTLib *iface, BSTR *name)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_MajorVersion(IGenScriptletTLib *iface, WORD version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %x): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_MajorVersion(IGenScriptletTLib *iface, WORD *version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_MinorVersion(IGenScriptletTLib *iface, WORD version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %x): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_MinorVersion(IGenScriptletTLib *iface, WORD *version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_Write(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_Reset(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_GUID(IGenScriptletTLib *iface, BSTR guid)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(guid));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_GUID(IGenScriptletTLib *iface, BSTR *ret)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %p)\n", This, ret);

    *ret = NULL;

    if (!This->guid)
    {
        WCHAR guidW[39];
        GUID guid;
        HRESULT hr;

        hr = CoCreateGuid(&guid);
        if (FAILED(hr))
            return hr;

        hr = StringFromGUID2(&guid, guidW, ARRAY_SIZE(guidW));
        if (FAILED(hr))
            return hr;

        if (!(This->guid = SysAllocString(guidW)))
            return E_OUTOFMEMORY;
    }

    *ret = SysAllocString(This->guid);
    return *ret ? S_OK : E_OUTOFMEMORY;
}

static const IGenScriptletTLibVtbl scriptlet_typelib_vtbl =
{
    scriptlet_typelib_QueryInterface,
    scriptlet_typelib_AddRef,
    scriptlet_typelib_Release,
    scriptlet_typelib_GetTypeInfoCount,
    scriptlet_typelib_GetTypeInfo,
    scriptlet_typelib_GetIDsOfNames,
    scriptlet_typelib_Invoke,
    scriptlet_typelib_AddURL,
    scriptlet_typelib_put_Path,
    scriptlet_typelib_get_Path,
    scriptlet_typelib_put_Doc,
    scriptlet_typelib_get_Doc,
    scriptlet_typelib_put_Name,
    scriptlet_typelib_get_Name,
    scriptlet_typelib_put_MajorVersion,
    scriptlet_typelib_get_MajorVersion,
    scriptlet_typelib_put_MinorVersion,
    scriptlet_typelib_get_MinorVersion,
    scriptlet_typelib_Write,
    scriptlet_typelib_Reset,
    scriptlet_typelib_put_GUID,
    scriptlet_typelib_get_GUID
};

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            scrobj_instance = hinst;
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            release_typelib();
            break;
    }
    return TRUE;
}

/***********************************************************************
 *      DllRegisterServer (scrobj.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(scrobj_instance);
}

/***********************************************************************
 *      DllUnregisterServer (scrobj.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(scrobj_instance);
}

static HRESULT WINAPI scriptlet_typelib_CreateInstance(IClassFactory *factory, IUnknown *outer, REFIID riid, void **obj)
{
    struct scriptlet_typelib *This;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", factory, outer, debugstr_guid(riid), obj);

    *obj = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IGenScriptletTLib_iface.lpVtbl = &scriptlet_typelib_vtbl;
    This->ref = 1;
    This->guid = NULL;

    hr = IGenScriptletTLib_QueryInterface(&This->IGenScriptletTLib_iface, riid, obj);
    IGenScriptletTLib_Release(&This->IGenScriptletTLib_iface);
    return hr;
}

static HRESULT WINAPI scrruncf_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }
    else if (IsEqualGUID(&IID_IClassFactory, riid))
    {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI scrruncf_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI scrruncf_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI scrruncf_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const struct IClassFactoryVtbl scriptlet_typelib_factory_vtbl =
{
    scrruncf_QueryInterface,
    scrruncf_AddRef,
    scrruncf_Release,
    scriptlet_typelib_CreateInstance,
    scrruncf_LockServer
};

static IClassFactory scriptlet_typelib_factory = { &scriptlet_typelib_factory_vtbl };

/***********************************************************************
 *      DllGetClassObject (scrobj.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&CLSID_TypeLib, rclsid))
    {
        TRACE("(Scriptlet.TypeLib %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&scriptlet_typelib_factory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *      DllCanUnloadNow (scrobj.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
