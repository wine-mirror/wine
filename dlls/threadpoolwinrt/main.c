/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_System_Threading
#include "windows.system.threading.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct threadpool_factory
{
    IActivationFactory IActivationFactory_iface;
    IThreadPoolStatics IThreadPoolStatics_iface;
    LONG refcount;
};

static inline struct threadpool_factory *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct threadpool_factory, IActivationFactory_iface);
}

static inline struct threadpool_factory *impl_from_IThreadPoolStatics(IThreadPoolStatics *iface)
{
    return CONTAINING_RECORD(iface, struct threadpool_factory, IThreadPoolStatics_iface);
}

static HRESULT STDMETHODCALLTYPE threadpool_factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct threadpool_factory *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IThreadPoolStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IThreadPoolStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE threadpool_factory_AddRef(IActivationFactory *iface)
{
    struct threadpool_factory *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE threadpool_factory_Release(IActivationFactory *iface)
{
    struct threadpool_factory *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE threadpool_factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl threadpool_factory_vtbl =
{
    threadpool_factory_QueryInterface,
    threadpool_factory_AddRef,
    threadpool_factory_Release,
    /* IInspectable methods */
    threadpool_factory_GetIids,
    threadpool_factory_GetRuntimeClassName,
    threadpool_factory_GetTrustLevel,
    /* IActivationFactory methods */
    threadpool_factory_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE threadpool_statics_QueryInterface(
        IThreadPoolStatics *iface, REFIID iid, void **object)
{
    struct threadpool_factory *factory = impl_from_IThreadPoolStatics(iface);
    return IActivationFactory_QueryInterface(&factory->IActivationFactory_iface, iid, object);
}

static ULONG STDMETHODCALLTYPE threadpool_statics_AddRef(IThreadPoolStatics *iface)
{
    struct threadpool_factory *factory = impl_from_IThreadPoolStatics(iface);
    return IActivationFactory_AddRef(&factory->IActivationFactory_iface);
}

static ULONG STDMETHODCALLTYPE threadpool_statics_Release(IThreadPoolStatics *iface)
{
    struct threadpool_factory *factory = impl_from_IThreadPoolStatics(iface);
    return IActivationFactory_Release(&factory->IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_GetIids(
        IThreadPoolStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_GetRuntimeClassName(
        IThreadPoolStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_GetTrustLevel(
        IThreadPoolStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_RunAsync(
        IThreadPoolStatics *iface, IWorkItemHandler *handler, IAsyncAction **operation)
{
    FIXME("iface %p, handler %p, operation %p stub!\n", iface, handler, operation);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_RunWithPriorityAsync(
        IThreadPoolStatics *iface, IWorkItemHandler *handler, WorkItemPriority priority, IAsyncAction **operation)
{
    FIXME("iface %p, handler %p, priority %d, operation %p stub!\n", iface, handler, priority, operation);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_RunWithPriorityAndOptionsAsync(
        IThreadPoolStatics *iface, IWorkItemHandler *handler, WorkItemPriority priority,
        WorkItemOptions options, IAsyncAction **operation)
{
    FIXME("iface %p, handler %p, priority %d, options %d, operation %p stub!\n", iface, handler, priority, options, operation);
    return E_NOTIMPL;
}

static const struct IThreadPoolStaticsVtbl threadpool_statics_vtbl =
{
    threadpool_statics_QueryInterface,
    threadpool_statics_AddRef,
    threadpool_statics_Release,
    /* IInspectable methods */
    threadpool_statics_GetIids,
    threadpool_statics_GetRuntimeClassName,
    threadpool_statics_GetTrustLevel,
    /* IThreadPoolStatics methods */
    threadpool_statics_RunAsync,
    threadpool_statics_RunWithPriorityAsync,
    threadpool_statics_RunWithPriorityAndOptionsAsync,
};

static struct threadpool_factory threadpool_factory =
{
    .IActivationFactory_iface.lpVtbl = &threadpool_factory_vtbl,
    .IThreadPoolStatics_iface.lpVtbl = &threadpool_statics_vtbl,
    .refcount = 1,
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    const WCHAR *name = WindowsGetStringRawBuffer(classid, NULL);

    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);

    *factory = NULL;

    if (!wcscmp(name, RuntimeClass_Windows_System_Threading_ThreadPool))
    {
        *factory = &threadpool_factory.IActivationFactory_iface;
        IUnknown_AddRef(*factory);
    }

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
