/*
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
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
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef struct
{
    ITaskScheduler ITaskScheduler_iface;
    LONG ref;
    ITaskService *service;
    ITaskFolder *root;
} TaskSchedulerImpl;

typedef struct
{
    IEnumWorkItems IEnumWorkItems_iface;
    LONG ref;
} EnumWorkItemsImpl;

static inline TaskSchedulerImpl *impl_from_ITaskScheduler(ITaskScheduler *iface)
{
    return CONTAINING_RECORD(iface, TaskSchedulerImpl, ITaskScheduler_iface);
}

static inline EnumWorkItemsImpl *impl_from_IEnumWorkItems(IEnumWorkItems *iface)
{
    return CONTAINING_RECORD(iface, EnumWorkItemsImpl, IEnumWorkItems_iface);
}

static void TaskSchedulerDestructor(TaskSchedulerImpl *This)
{
    TRACE("%p\n", This);
    ITaskFolder_Release(This->root);
    ITaskService_Release(This->service);
    HeapFree(GetProcessHeap(), 0, This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI EnumWorkItems_QueryInterface(IEnumWorkItems *iface, REFIID riid, void **obj)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IEnumWorkItems) || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = &This->IEnumWorkItems_iface;
        IEnumWorkItems_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumWorkItems_AddRef(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI EnumWorkItems_Release(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
        InterlockedDecrement(&dll_ref);
    }

    return ref;
}

static HRESULT WINAPI EnumWorkItems_Next(IEnumWorkItems *iface, ULONG count, LPWSTR **names, ULONG *fetched)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, count, names, fetched);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumWorkItems_Skip(IEnumWorkItems *iface, ULONG count)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    FIXME("(%p)->(%u): stub\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumWorkItems_Reset(IEnumWorkItems *iface)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumWorkItems_Clone(IEnumWorkItems *iface, IEnumWorkItems **cloned)
{
    EnumWorkItemsImpl *This = impl_from_IEnumWorkItems(iface);
    FIXME("(%p)->(%p): stub\n", This, cloned);
    return E_NOTIMPL;
}

static const IEnumWorkItemsVtbl EnumWorkItemsVtbl = {
    EnumWorkItems_QueryInterface,
    EnumWorkItems_AddRef,
    EnumWorkItems_Release,
    EnumWorkItems_Next,
    EnumWorkItems_Skip,
    EnumWorkItems_Reset,
    EnumWorkItems_Clone
};

static HRESULT create_task_enum(IEnumWorkItems **ret)
{
    EnumWorkItemsImpl *tasks;

    *ret = NULL;

    tasks = HeapAlloc(GetProcessHeap(), 0, sizeof(*tasks));
    if (!tasks)
        return E_OUTOFMEMORY;

    tasks->IEnumWorkItems_iface.lpVtbl = &EnumWorkItemsVtbl;
    tasks->ref = 1;

    *ret = &tasks->IEnumWorkItems_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_QueryInterface(
        ITaskScheduler* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskSchedulerImpl * This = impl_from_ITaskScheduler(iface);

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskScheduler))
    {
        *ppvObject = &This->ITaskScheduler_iface;
        ITaskScheduler_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskScheduler_AddRef(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    TRACE("\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MSTASK_ITaskScheduler_Release(
        ITaskScheduler* iface)
{
    TaskSchedulerImpl * This = impl_from_ITaskScheduler(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskSchedulerDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_SetTargetComputer(
        ITaskScheduler *iface, LPCWSTR comp_name)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    VARIANT v_null, v_comp;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(comp_name));

    V_VT(&v_null) = VT_NULL;
    V_VT(&v_comp) = VT_BSTR;
    V_BSTR(&v_comp) = SysAllocString(comp_name);
    hr = ITaskService_Connect(This->service, v_comp, v_null, v_null, v_null);
    SysFreeString(V_BSTR(&v_comp));
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_GetTargetComputer(
        ITaskScheduler *iface, LPWSTR *comp_name)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);
    BSTR bstr;
    WCHAR *buffer;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, comp_name);

    if (!comp_name)
        return E_INVALIDARG;

    hr = ITaskService_get_TargetServer(This->service, &bstr);
    if (hr != S_OK) return hr;

    /* extra space for two '\' and a zero */
    buffer = CoTaskMemAlloc((SysStringLen(bstr) + 3) * sizeof(WCHAR));
    if (buffer)
    {
        buffer[0] = '\\';
        buffer[1] = '\\';
        lstrcpyW(buffer + 2, bstr);
        *comp_name = buffer;
        hr = S_OK;
    }
    else
    {
        *comp_name = NULL;
        hr = E_OUTOFMEMORY;
    }

    SysFreeString(bstr);
    return hr;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Enum(
        ITaskScheduler* iface,
        IEnumWorkItems **tasks)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);

    TRACE("(%p)->(%p)\n", This, tasks);

    if (!tasks)
        return E_INVALIDARG;

    return create_task_enum(tasks);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Activate(
        ITaskScheduler* iface,
        LPCWSTR pwszName,
        REFIID riid,
        IUnknown **ppunk)
{
    TRACE("%p, %s, %s, %p: stub\n", iface, debugstr_w(pwszName),
            debugstr_guid(riid), ppunk);
    FIXME("Partial stub always returning ERROR_FILE_NOT_FOUND\n");
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_Delete(
        ITaskScheduler* iface,
        LPCWSTR pwszName)
{
    FIXME("%p, %s: stub\n", iface, debugstr_w(pwszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_NewWorkItem(
        ITaskScheduler* iface,
        LPCWSTR task_name,
        REFCLSID rclsid,
        REFIID riid,
        IUnknown **task)
{
    TaskSchedulerImpl *This = impl_from_ITaskScheduler(iface);

    TRACE("(%p, %s, %s, %s, %p)\n", iface, debugstr_w(task_name),
            debugstr_guid(rclsid), debugstr_guid(riid), task);

    if (!IsEqualGUID(rclsid, &CLSID_CTask))
        return CLASS_E_CLASSNOTAVAILABLE;

    if (!IsEqualGUID(riid, &IID_ITask))
        return E_NOINTERFACE;

    return TaskConstructor(This->root, task_name, (ITask **)task, TRUE);
}

static HRESULT WINAPI MSTASK_ITaskScheduler_AddWorkItem(
        ITaskScheduler* iface,
        LPCWSTR pwszTaskName,
        IScheduledWorkItem *pWorkItem)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(pwszTaskName), pWorkItem);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITaskScheduler_IsOfType(
        ITaskScheduler* iface,
        LPCWSTR pwszName,
        REFIID riid)
{
    FIXME("%p, %s, %s: stub\n", iface, debugstr_w(pwszName),
            debugstr_guid(riid));
    return E_NOTIMPL;
}

static const ITaskSchedulerVtbl MSTASK_ITaskSchedulerVtbl =
{
    MSTASK_ITaskScheduler_QueryInterface,
    MSTASK_ITaskScheduler_AddRef,
    MSTASK_ITaskScheduler_Release,
    MSTASK_ITaskScheduler_SetTargetComputer,
    MSTASK_ITaskScheduler_GetTargetComputer,
    MSTASK_ITaskScheduler_Enum,
    MSTASK_ITaskScheduler_Activate,
    MSTASK_ITaskScheduler_Delete,
    MSTASK_ITaskScheduler_NewWorkItem,
    MSTASK_ITaskScheduler_AddWorkItem,
    MSTASK_ITaskScheduler_IsOfType
};

HRESULT TaskSchedulerConstructor(LPVOID *ppObj)
{
    TaskSchedulerImpl *This;
    ITaskService *service;
    ITaskFolder *root;
    VARIANT v_null;
    HRESULT hr;

    TRACE("(%p)\n", ppObj);

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK) return hr;

    V_VT(&v_null) = VT_NULL;
    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    if (hr != S_OK)
    {
        ITaskService_Release(service);
        return hr;
    }

    hr = ITaskService_GetFolder(service, NULL, &root);
    if (hr != S_OK)
    {
        ITaskService_Release(service);
        return hr;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        ITaskFolder_Release(root);
        ITaskService_Release(service);
        return E_OUTOFMEMORY;
    }

    This->ITaskScheduler_iface.lpVtbl = &MSTASK_ITaskSchedulerVtbl;
    This->service = service;
    This->root = root;
    This->ref = 1;

    *ppObj = &This->ITaskScheduler_iface;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
