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
#include <assert.h>

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

struct threadpool_factory
{
    IActivationFactory IActivationFactory_iface;
    IThreadPoolStatics IThreadPoolStatics_iface;
    LONG refcount;
};

struct async_action
{
    IAsyncAction IAsyncAction_iface;
    IAsyncInfo IAsyncInfo_iface;
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

static inline struct async_action *impl_from_IAsyncAction(IAsyncAction *iface)
{
    return CONTAINING_RECORD(iface, struct async_action, IAsyncAction_iface);
}

static inline struct async_action *impl_from_IAsyncInfo(IAsyncInfo *iface)
{
    return CONTAINING_RECORD(iface, struct async_action, IAsyncInfo_iface);
}

static HRESULT STDMETHODCALLTYPE async_action_QueryInterface(IAsyncAction *iface, REFIID iid, void **out)
{
    struct async_action *action = impl_from_IAsyncAction(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IAsyncAction)
            || IsEqualIID(iid, &IID_IInspectable)
            || IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
    }
    else if (IsEqualIID(iid, &IID_IAsyncInfo))
    {
        *out = &action->IAsyncInfo_iface;
    }
    else
    {
        *out = NULL;
        WARN("Unsupported interface %s.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE async_action_AddRef(IAsyncAction *iface)
{
    struct async_action *action = impl_from_IAsyncAction(iface);
    ULONG refcount = InterlockedIncrement(&action->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE async_action_Release(IAsyncAction *iface)
{
    struct async_action *action = impl_from_IAsyncAction(iface);
    ULONG refcount = InterlockedDecrement(&action->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(action);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE async_action_GetIids(
        IAsyncAction *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_action_GetRuntimeClassName(
        IAsyncAction *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_action_GetTrustLevel(
        IAsyncAction *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_action_put_Completed(IAsyncAction *iface, IAsyncActionCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_action_get_Completed(IAsyncAction *iface, IAsyncActionCompletedHandler **handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_action_GetResults(IAsyncAction *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const IAsyncActionVtbl async_action_vtbl =
{
    async_action_QueryInterface,
    async_action_AddRef,
    async_action_Release,
    async_action_GetIids,
    async_action_GetRuntimeClassName,
    async_action_GetTrustLevel,
    async_action_put_Completed,
    async_action_get_Completed,
    async_action_GetResults,
};

static HRESULT STDMETHODCALLTYPE async_info_QueryInterface(IAsyncInfo *iface, REFIID iid, void **out)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_QueryInterface(&action->IAsyncAction_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE async_info_AddRef(IAsyncInfo *iface)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_AddRef(&action->IAsyncAction_iface);
}

static ULONG STDMETHODCALLTYPE async_info_Release(IAsyncInfo *iface)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_Release(&action->IAsyncAction_iface);
}

static HRESULT STDMETHODCALLTYPE async_info_GetIids(IAsyncInfo *iface, ULONG *iid_count, IID **iids)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_GetIids(&action->IAsyncAction_iface, iid_count, iids);
}

static HRESULT STDMETHODCALLTYPE async_info_GetRuntimeClassName(IAsyncInfo *iface, HSTRING *class_name)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_GetRuntimeClassName(&action->IAsyncAction_iface, class_name);
}

static HRESULT STDMETHODCALLTYPE async_info_GetTrustLevel(IAsyncInfo *iface, TrustLevel *trust_level)
{
    struct async_action *action = impl_from_IAsyncInfo(iface);
    return IAsyncAction_GetTrustLevel(&action->IAsyncAction_iface, trust_level);
}

static HRESULT STDMETHODCALLTYPE async_info_get_Id(IAsyncInfo *iface, UINT32 *id)
{
    FIXME("iface %p, id %p stub!\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_info_get_Status(IAsyncInfo *iface, AsyncStatus *status)
{
    FIXME("iface %p, status %p stub!\n", iface, status);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_info_get_ErrorCode(IAsyncInfo *iface, HRESULT *error_code)
{
    FIXME("iface %p, error_code %p stub!\n", iface, error_code);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_info_Cancel(IAsyncInfo *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE async_info_Close(IAsyncInfo *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const IAsyncInfoVtbl async_info_vtbl =
{
    async_info_QueryInterface,
    async_info_AddRef,
    async_info_Release,
    async_info_GetIids,
    async_info_GetRuntimeClassName,
    async_info_GetTrustLevel,
    async_info_get_Id,
    async_info_get_Status,
    async_info_get_ErrorCode,
    async_info_Cancel,
    async_info_Close,
};

static HRESULT async_action_create(IAsyncAction **ret)
{
    struct async_action *object;

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAsyncAction_iface.lpVtbl = &async_action_vtbl;
    object->IAsyncInfo_iface.lpVtbl = &async_info_vtbl;
    object->refcount = 1;

    *ret = &object->IAsyncAction_iface;

    return S_OK;
}

struct work_item
{
    IWorkItemHandler *handler;
    IAsyncAction *action;
};

static void release_work_item(struct work_item *item)
{
    IWorkItemHandler_Release(item->handler);
    IAsyncAction_Release(item->action);
    free(item);
}

static HRESULT alloc_work_item(IWorkItemHandler *handler, struct work_item **ret)
{
    struct work_item *object;
    HRESULT hr;

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = async_action_create(&object->action)))
    {
        free(object);
        return hr;
    }

    IWorkItemHandler_AddRef((object->handler = handler));

    *ret = object;

    return S_OK;
}

static void work_item_invoke_release(struct work_item *item)
{
    IWorkItemHandler_Invoke(item->handler, item->action);
    release_work_item(item);
}

static DWORD WINAPI sliced_thread_proc(void *arg)
{
    struct work_item *item = arg;
    work_item_invoke_release(item);
    return 0;
}

struct thread_pool
{
    INIT_ONCE init_once;
    TP_CALLBACK_ENVIRON environment;
};

static struct thread_pool pools[3];

static BOOL CALLBACK pool_init_once(INIT_ONCE *init_once, void *param, void **context)
{
    struct thread_pool *pool = param;

    memset(&pool->environment, 0, sizeof(pool->environment));
    pool->environment.Version = 1;

    if (!(pool->environment.Pool = CreateThreadpool(NULL))) return FALSE;

    SetThreadpoolThreadMaximum(pool->environment.Pool, 10);

    return TRUE;
}

static void CALLBACK pool_work_callback(TP_CALLBACK_INSTANCE *instance, void *context, TP_WORK *work)
{
    struct work_item *item = context;
    work_item_invoke_release(item);
}

static HRESULT submit_threadpool_work(struct work_item *item, WorkItemPriority priority, IAsyncAction **action)
{
    struct thread_pool *pool;
    TP_WORK *work;

    assert(priority == WorkItemPriority_Low
            || priority == WorkItemPriority_Normal
            || priority == WorkItemPriority_High);

    pool = &pools[priority + 1];

    if (!InitOnceExecuteOnce(&pool->init_once, pool_init_once, pool, NULL))
        return E_FAIL;

    if (!(work = CreateThreadpoolWork(pool_work_callback, item, &pool->environment)))
        return E_FAIL;

    IAsyncAction_AddRef((*action = item->action));
    SubmitThreadpoolWork(work);

    return S_OK;
}

static HRESULT submit_standalone_thread_work(struct work_item *item, WorkItemPriority priority, IAsyncAction **action)
{
    HANDLE thread;

    if (!(thread = CreateThread(NULL, 0, sliced_thread_proc, item, priority == WorkItemPriority_Normal ?
            0 : CREATE_SUSPENDED, NULL)))
    {
        WARN("Failed to create a thread, error %ld.\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    IAsyncAction_AddRef((*action = item->action));
    if (priority != WorkItemPriority_Normal)
    {
        SetThreadPriority(thread, priority == WorkItemPriority_High ? THREAD_PRIORITY_HIGHEST : THREAD_PRIORITY_LOWEST);
        ResumeThread(thread);
    }
    CloseHandle(thread);

    return S_OK;
}

static HRESULT run_async(IWorkItemHandler *handler, WorkItemPriority priority, WorkItemOptions options,
        IAsyncAction **action)
{
    struct work_item *item;
    HRESULT hr;

    *action = NULL;

    if (!handler)
        return E_INVALIDARG;

    if (priority < WorkItemPriority_Low || priority > WorkItemPriority_High)
        return E_INVALIDARG;

    if (FAILED(hr = alloc_work_item(handler, &item)))
        return hr;

    if (options == WorkItemOptions_TimeSliced)
        hr = submit_standalone_thread_work(item, priority, action);
    else
        hr = submit_threadpool_work(item, priority, action);

    if (FAILED(hr))
        release_work_item(item);

    return hr;
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
    TRACE("iface %p, handler %p, operation %p.\n", iface, handler, operation);

    return run_async(handler, WorkItemPriority_Normal, WorkItemOptions_None, operation);
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_RunWithPriorityAsync(
        IThreadPoolStatics *iface, IWorkItemHandler *handler, WorkItemPriority priority, IAsyncAction **operation)
{
    TRACE("iface %p, handler %p, priority %d, operation %p.\n", iface, handler, priority, operation);

    return run_async(handler, priority, WorkItemOptions_None, operation);
}

static HRESULT STDMETHODCALLTYPE threadpool_statics_RunWithPriorityAndOptionsAsync(
        IThreadPoolStatics *iface, IWorkItemHandler *handler, WorkItemPriority priority,
        WorkItemOptions options, IAsyncAction **operation)
{
    TRACE("iface %p, handler %p, priority %d, options %d, operation %p.\n", iface, handler, priority, options, operation);

    return run_async(handler, priority, options, operation);
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
