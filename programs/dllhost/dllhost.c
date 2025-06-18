/*
 * Copyright 2022 Dmitry Timoshkov
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

#include <windef.h>
#include <winbase.h>
#include <objbase.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dllhost);

struct factory
{
    IClassFactory IClassFactory_iface;
    IMarshal IMarshal_iface;
    CLSID clsid;
    LONG ref;
    IClassFactory *dll_factory;
};

static inline struct factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct factory, IClassFactory_iface);
}

static inline struct factory *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, struct factory, IMarshal_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface,
    REFIID iid, void **ppv)
{
    struct factory *factory = impl_from_IClassFactory(iface);

    TRACE("(%p,%s,%p)\n", iface, wine_dbgstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(&factory->IClassFactory_iface);
        *ppv = &factory->IClassFactory_iface;
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_IMarshal))
    {
        IClassFactory_AddRef(&factory->IClassFactory_iface);
        *ppv = &factory->IMarshal_iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    struct factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("(%p)->%lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    struct factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p)->%lu\n", iface, ref);

    if (!ref)
    {
        if (factory->dll_factory) IClassFactory_Release(factory->dll_factory);
        HeapFree(GetProcessHeap(), 0, factory);
    }

    return ref;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface,
    IUnknown *punkouter, REFIID iid, void **ppv)
{
    FIXME("(%p,%p,%s,%p): stub\n", iface, punkouter, wine_dbgstr_guid(iid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("(%p,%d)\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl ClassFactory_Vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static HRESULT WINAPI marshal_QueryInterface(IMarshal *iface, REFIID iid, LPVOID *ppv)
{
    struct factory *factory = impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p)\n", iface, wine_dbgstr_guid(iid), ppv);

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, ppv);
}

static ULONG WINAPI marshal_AddRef(IMarshal *iface)
{
    struct factory *factory = impl_from_IMarshal(iface);

    TRACE("(%p)\n", iface);

    return IClassFactory_AddRef(&factory->IClassFactory_iface);
}

static ULONG WINAPI marshal_Release(IMarshal *iface)
{
    struct factory *factory = impl_from_IMarshal(iface);

    TRACE("(%p)\n", iface);

    return IClassFactory_Release(&factory->IClassFactory_iface);
}

static HRESULT WINAPI marshal_GetUnmarshalClass(IMarshal *iface, REFIID iid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *clsid)
{
    TRACE("(%p,%s,%p,%08lx,%p,%08lx,%p)\n", iface, wine_dbgstr_guid(iid), pv,
          dwDestContext, pvDestContext, mshlflags, clsid);

    *clsid = CLSID_StdMarshal;
    return S_OK;
}

static HRESULT WINAPI marshal_GetMarshalSizeMax(IMarshal *iface, REFIID iid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    FIXME("(%p,%s,%p,%08lx,%p,%08lx,%p): stub\n", iface, wine_dbgstr_guid(iid), pv,
          dwDestContext, pvDestContext, mshlflags, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID iid,
        void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    struct factory *factory = impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p,%08lx,%p,%08lx)\n", stream, wine_dbgstr_guid(iid), pv, dwDestContext, pvDestContext, mshlflags);

    return CoMarshalInterface(stream, iid, (IUnknown *)factory->dll_factory, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI marshal_UnmarshalInterface(IMarshal *iface, IStream *stream,
        REFIID iid, void **ppv)
{
    FIXME("(%p,%p,%s,%p): stub\n", iface, stream, wine_dbgstr_guid(iid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    TRACE("(%p,%p)\n", iface, stream);
    return S_OK;
}

static HRESULT WINAPI marshal_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    TRACE("(%p, %08lx)\n", iface, reserved);
    return S_OK;
}

static const IMarshalVtbl Marshal_Vtbl =
{
    marshal_QueryInterface,
    marshal_AddRef,
    marshal_Release,
    marshal_GetUnmarshalClass,
    marshal_GetMarshalSizeMax,
    marshal_MarshalInterface,
    marshal_UnmarshalInterface,
    marshal_ReleaseMarshalData,
    marshal_DisconnectObject
};

struct surrogate
{
    ISurrogate ISurrogate_iface;
    IClassFactory *factory;
    DWORD cookie;
    HANDLE event;
    LONG ref;
};

static inline struct surrogate *impl_from_ISurrogate(ISurrogate *iface)
{
    return CONTAINING_RECORD(iface, struct surrogate, ISurrogate_iface);
}

static HRESULT WINAPI surrogate_QueryInterface(ISurrogate *iface,
    REFIID iid, void **ppv)
{
    struct surrogate *surrogate = impl_from_ISurrogate(iface);

    TRACE("(%p,%s,%p)\n", iface, wine_dbgstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISurrogate))
    {
        ISurrogate_AddRef(&surrogate->ISurrogate_iface);
        *ppv = &surrogate->ISurrogate_iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI surrogate_AddRef(ISurrogate *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI surrogate_Release(ISurrogate *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI surrogate_LoadDllServer(ISurrogate *iface, const CLSID *clsid)
{
    struct surrogate *surrogate = impl_from_ISurrogate(iface);
    struct factory *factory;
    HRESULT hr;

    TRACE("(%p,%s)\n", iface, wine_dbgstr_guid(clsid));

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (!factory)
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &ClassFactory_Vtbl;
    factory->IMarshal_iface.lpVtbl = &Marshal_Vtbl;
    factory->clsid = *clsid;
    factory->ref = 1;
    factory->dll_factory = NULL;

    hr = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void **)&factory->dll_factory);
    if (SUCCEEDED(hr))
        hr = CoRegisterClassObject(clsid, (IUnknown *)&factory->IClassFactory_iface,
                                   CLSCTX_LOCAL_SERVER, REGCLS_SURROGATE, &surrogate->cookie);
    if (FAILED(hr))
        IClassFactory_Release(&factory->IClassFactory_iface);
    else
    {
        surrogate->factory = &factory->IClassFactory_iface;
        surrogate->event = CreateEventW(NULL, FALSE, FALSE, NULL);
    }

    return hr;
}

static HRESULT WINAPI surrogate_FreeSurrogate(ISurrogate *iface)
{
    struct surrogate *surrogate = impl_from_ISurrogate(iface);

    TRACE("(%p)\n", iface);

    if (surrogate->cookie)
    {
        CoRevokeClassObject(surrogate->cookie);
        surrogate->cookie = 0;
    }

    if (surrogate->factory)
    {
        IClassFactory_Release(surrogate->factory);
        surrogate->factory = NULL;
    }

    SetEvent(surrogate->event);

    return S_OK;
}

static const ISurrogateVtbl Surrogate_Vtbl =
{
    surrogate_QueryInterface,
    surrogate_AddRef,
    surrogate_Release,
    surrogate_LoadDllServer,
    surrogate_FreeSurrogate
};

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE previnst, LPWSTR cmdline, int showcmd)
{
    HRESULT hr;
    CLSID clsid;
    struct surrogate surrogate;

    TRACE("Running as %u-bit\n", (int)sizeof(void *) * 8);

    if (wcsnicmp(cmdline, L"/PROCESSID:", 11))
        return 0;

    cmdline += 11;

    surrogate.ISurrogate_iface.lpVtbl = &Surrogate_Vtbl;
    surrogate.factory = NULL;
    surrogate.cookie = 0;
    surrogate.event = NULL;
    surrogate.ref = 1;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CLSIDFromString(cmdline, &clsid);
    if (hr == S_OK)
    {
        CoRegisterSurrogate(&surrogate.ISurrogate_iface);

        hr = ISurrogate_LoadDllServer(&surrogate.ISurrogate_iface, &clsid);
        if (hr != S_OK)
        {
            ERR("Can't create instance of %s\n", wine_dbgstr_guid(&clsid));
            goto cleanup;
        }

        while (WaitForSingleObject(surrogate.event, 30000) != WAIT_OBJECT_0)
            CoFreeUnusedLibraries();
    }

cleanup:
    CoUninitialize();

    return 0;
}
