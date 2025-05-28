/*
 * Copyright 2014 Dmitry Timoshkov
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
#include "winreg.h"
#include "objbase.h"
#include "taskschd.h"
#include "schrpc.h"
#include "taskschd_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    IRegisteredTask IRegisteredTask_iface;
    LONG ref;
    WCHAR *path;
    ITaskDefinition *taskdef;
} RegisteredTask;

static inline RegisteredTask *impl_from_IRegisteredTask(IRegisteredTask *iface)
{
    return CONTAINING_RECORD(iface, RegisteredTask, IRegisteredTask_iface);
}

static ULONG WINAPI regtask_AddRef(IRegisteredTask *iface)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);
    return InterlockedIncrement(&regtask->ref);
}

static ULONG WINAPI regtask_Release(IRegisteredTask *iface)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);
    LONG ref = InterlockedDecrement(&regtask->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        ITaskDefinition_Release(regtask->taskdef);
        free(regtask->path);
        free(regtask);
    }

    return ref;
}

static HRESULT WINAPI regtask_QueryInterface(IRegisteredTask *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IRegisteredTask) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IRegisteredTask_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI regtask_GetTypeInfoCount(IRegisteredTask *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetTypeInfo(IRegisteredTask *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetIDsOfNames(IRegisteredTask *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_Invoke(IRegisteredTask *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Name(IRegisteredTask *iface, BSTR *name)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);
    const WCHAR *p_name;

    TRACE("%p,%p\n", iface, name);

    if (!name) return E_POINTER;

    p_name = wcsrchr(regtask->path, '\\');
    if (!p_name)
        p_name = regtask->path;
    else
        if (p_name[1] != 0) p_name++;

    *name = SysAllocString(p_name);
    if (!*name) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI regtask_get_Path(IRegisteredTask *iface, BSTR *path)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);

    TRACE("%p,%p\n", iface, path);

    if (!path) return E_POINTER;

    *path = SysAllocString(regtask->path);
    if (!*path) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI regtask_get_State(IRegisteredTask *iface, TASK_STATE *state)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);
    DWORD enabled;

    TRACE("%p,%p\n", iface, state);

    if (!state) return E_POINTER;

    return SchRpcGetTaskInfo(regtask->path, SCH_FLAG_STATE, &enabled, (DWORD *)state);
}

static HRESULT WINAPI regtask_get_Enabled(IRegisteredTask *iface, VARIANT_BOOL *v_enabled)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);
    DWORD enabled, state;
    HRESULT hr;

    TRACE("%p,%p\n", iface, v_enabled);

    if (!v_enabled) return E_POINTER;

    hr = SchRpcGetTaskInfo(regtask->path, 0, &enabled, &state);
    if (hr == S_OK)
        *v_enabled = enabled ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;
}

static HRESULT WINAPI regtask_put_Enabled(IRegisteredTask *iface, VARIANT_BOOL enabled)
{
    FIXME("%p,%d: stub\n", iface, enabled);
    return S_OK;
}

static HRESULT WINAPI regtask_Run(IRegisteredTask *iface, VARIANT params, IRunningTask **task)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_variant(&params), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_RunEx(IRegisteredTask *iface, VARIANT params, LONG flags,
                                    LONG session_id, BSTR user, IRunningTask **task)
{
    FIXME("%p,%s,%lx,%lx,%s,%p: stub\n", iface, debugstr_variant(&params), flags, session_id, debugstr_w(user), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetInstances(IRegisteredTask *iface, LONG flags, IRunningTaskCollection **tasks)
{
    FIXME("%p,%lx,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_LastRunTime(IRegisteredTask *iface, DATE *date)
{
    FIXME("%p,%p: stub\n", iface, date);
    return SCHED_S_TASK_HAS_NOT_RUN;
}

static HRESULT WINAPI regtask_get_LastTaskResult(IRegisteredTask *iface, LONG *result)
{
    FIXME("%p,%p: stub\n", iface, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_NumberOfMissedRuns(IRegisteredTask *iface, LONG *runs)
{
    FIXME("%p,%p: stub\n", iface, runs);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_NextRunTime(IRegisteredTask *iface, DATE *date)
{
    FIXME("%p,%p: stub\n", iface, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Definition(IRegisteredTask *iface, ITaskDefinition **task)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);

    TRACE("%p,%p\n", iface, task);

    if (!task) return E_POINTER;

    ITaskDefinition_AddRef(regtask->taskdef);
    *task = regtask->taskdef;

    return S_OK;
}

static HRESULT WINAPI regtask_get_Xml(IRegisteredTask *iface, BSTR *xml)
{
    RegisteredTask *regtask = impl_from_IRegisteredTask(iface);

    TRACE("%p,%p\n", iface, xml);

    if (!xml) return E_POINTER;

    return ITaskDefinition_get_XmlText(regtask->taskdef, xml);
}

static HRESULT WINAPI regtask_GetSecurityDescriptor(IRegisteredTask *iface, LONG info, BSTR *sddl)
{
    FIXME("%p,%lx,%p: stub\n", iface, info, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_SetSecurityDescriptor(IRegisteredTask *iface, BSTR sddl, LONG flags)
{
    FIXME("%p,%s,%lx: stub\n", iface, debugstr_w(sddl), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_Stop(IRegisteredTask *iface, LONG flags)
{
    FIXME("%p,%lx: stub\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetRunTimes(IRegisteredTask *iface, const LPSYSTEMTIME start, const LPSYSTEMTIME end,
                                          DWORD *count, LPSYSTEMTIME *time)
{
    FIXME("%p,%p.%p,%p,%p: stub\n", iface, start, end, count, time);
    return E_NOTIMPL;
}

static const IRegisteredTaskVtbl RegisteredTask_vtbl =
{
    regtask_QueryInterface,
    regtask_AddRef,
    regtask_Release,
    regtask_GetTypeInfoCount,
    regtask_GetTypeInfo,
    regtask_GetIDsOfNames,
    regtask_Invoke,
    regtask_get_Name,
    regtask_get_Path,
    regtask_get_State,
    regtask_get_Enabled,
    regtask_put_Enabled,
    regtask_Run,
    regtask_RunEx,
    regtask_GetInstances,
    regtask_get_LastRunTime,
    regtask_get_LastTaskResult,
    regtask_get_NumberOfMissedRuns,
    regtask_get_NextRunTime,
    regtask_get_Definition,
    regtask_get_Xml,
    regtask_GetSecurityDescriptor,
    regtask_SetSecurityDescriptor,
    regtask_Stop,
    regtask_GetRunTimes
};

HRESULT RegisteredTask_create(const WCHAR *path, const WCHAR *name, ITaskDefinition *definition, LONG flags,
                              TASK_LOGON_TYPE logon, IRegisteredTask **obj, BOOL create)
{
    WCHAR *full_name;
    RegisteredTask *regtask;
    HRESULT hr;

    if (!name)
    {
        if (!create) return E_INVALIDARG;

        /* NULL task name is allowed only in the root folder */
        if (path[0] != '\\' || path[1])
            return E_INVALIDARG;

        full_name = NULL;
    }
    else
    {
        full_name = get_full_path(path, name);
        if (!full_name) return E_OUTOFMEMORY;
    }

    regtask = malloc(sizeof(*regtask));
    if (!regtask)
    {
        free(full_name);
        return E_OUTOFMEMORY;
    }

    if (create)
    {
        WCHAR *actual_path = NULL;
        TASK_XML_ERROR_INFO *error_info = NULL;
        BSTR xml = NULL;

        hr = ITaskDefinition_get_XmlText(definition, &xml);
        if (hr != S_OK || (hr = SchRpcRegisterTask(full_name, xml, flags, NULL, logon, 0, NULL, &actual_path, &error_info)) != S_OK)
        {
            free(full_name);
            free(regtask);
            SysFreeString(xml);
            return hr;
        }
        SysFreeString(xml);

        free(full_name);
        full_name = wcsdup(actual_path);
        MIDL_user_free(actual_path);
    }
    else
    {
        DWORD count = 0;
        WCHAR *xml = NULL;

        hr = SchRpcRetrieveTask(full_name, L"", &count, &xml);
        if (hr != S_OK || (hr = ITaskDefinition_put_XmlText(definition, xml)) != S_OK)
        {
            free(full_name);
            free(regtask);
            MIDL_user_free(xml);
            return hr;
        }
        MIDL_user_free(xml);
    }

    regtask->IRegisteredTask_iface.lpVtbl = &RegisteredTask_vtbl;
    regtask->path = full_name;
    regtask->ref = 1;
    regtask->taskdef = definition;
    *obj = &regtask->IRegisteredTask_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    IRegisteredTaskCollection IRegisteredTaskCollection_iface;
    LONG ref;
    WCHAR *path;
} RegisteredTaskCollection;

static inline RegisteredTaskCollection *impl_from_IRegisteredTaskCollection(IRegisteredTaskCollection *iface)
{
    return CONTAINING_RECORD(iface, RegisteredTaskCollection, IRegisteredTaskCollection_iface);
}

static ULONG WINAPI regtasks_AddRef(IRegisteredTaskCollection *iface)
{
    RegisteredTaskCollection *regtasks = impl_from_IRegisteredTaskCollection(iface);
    return InterlockedIncrement(&regtasks->ref);
}

static ULONG WINAPI regtasks_Release(IRegisteredTaskCollection *iface)
{
    RegisteredTaskCollection *regtasks = impl_from_IRegisteredTaskCollection(iface);
    LONG ref = InterlockedDecrement(&regtasks->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free(regtasks->path);
        free(regtasks);
    }

    return ref;
}

static HRESULT WINAPI regtasks_QueryInterface(IRegisteredTaskCollection *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IRegisteredTaskCollection) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IRegisteredTaskCollection_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI regtasks_GetTypeInfoCount(IRegisteredTaskCollection *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtasks_GetTypeInfo(IRegisteredTaskCollection *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtasks_GetIDsOfNames(IRegisteredTaskCollection *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtasks_Invoke(IRegisteredTaskCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtasks_get_Count(IRegisteredTaskCollection *iface, LONG *count)
{
    HRESULT hr;
    RegisteredTaskCollection *reg_tasks = NULL;
    TASK_NAMES task_names = NULL;
    DWORD start_index = 0, num_tasks;

    reg_tasks = impl_from_IRegisteredTaskCollection(iface);

    if (!count)
        return E_POINTER;

    hr = SchRpcEnumTasks(reg_tasks->path, 0, &start_index, 0, &num_tasks, &task_names);
    if (FAILED(hr))
    {
        *count = 0;
        return hr;
    }

    *count = num_tasks;

    for (DWORD i = 0; i < num_tasks; i++)
        MIDL_user_free(task_names[i]);
    MIDL_user_free(task_names);

    return S_OK;
}

static HRESULT WINAPI regtasks_get_Item(IRegisteredTaskCollection *iface, VARIANT index, IRegisteredTask **regtask)
{
    HRESULT hr;
    VARIANT converted_index;
    TASK_NAMES task_names = NULL;
    ITaskDefinition *definition = NULL;
    RegisteredTaskCollection *collection = NULL;
    DWORD start_index = 0, num_tasks = 0;

    collection = impl_from_IRegisteredTaskCollection(iface);
    if (!regtask)
        return E_POINTER;
    *regtask = NULL;

    VariantInit(&converted_index);
    hr = VariantChangeType(&converted_index, &index, 0, VT_UI4);
    if (FAILED(hr))
        return hr;

    start_index = V_UI4(&index);
    if (start_index == 0)
    {
        VariantClear(&converted_index);
        return E_INVALIDARG;
    }
    start_index -= 1;

    hr = SchRpcEnumTasks(collection->path, 0, &start_index, 1, &num_tasks, &task_names);
    if (FAILED(hr))
    {
        VariantClear(&converted_index);
        return hr;
    }
    if (!task_names)
    {
        VariantClear(&converted_index);
        return E_INVALIDARG;
    }

    hr = TaskDefinition_create(&definition);
    if (FAILED(hr))
    {
        if(hr != E_OUTOFMEMORY)
            ITaskDefinition_Release(definition);
        VariantClear(&converted_index);
        return hr;
    }

    hr = RegisteredTask_create(collection->path, task_names[0], definition, TASK_VALIDATE_ONLY, TASK_LOGON_INTERACTIVE_TOKEN, regtask, FALSE);

    VariantClear(&converted_index);
    MIDL_user_free(task_names[0]);
    MIDL_user_free(task_names);
    ITaskDefinition_Release(definition);
    return hr;
}

static HRESULT WINAPI regtasks_get__NewEnum(IRegisteredTaskCollection *iface, IUnknown **penum)
{
    FIXME("%p,%p: stub\n", iface, penum);
    return E_NOTIMPL;
}

static const IRegisteredTaskCollectionVtbl RegisteredTaskCollection_vtbl =
{
    regtasks_QueryInterface,
    regtasks_AddRef,
    regtasks_Release,
    regtasks_GetTypeInfoCount,
    regtasks_GetTypeInfo,
    regtasks_GetIDsOfNames,
    regtasks_Invoke,
    regtasks_get_Count,
    regtasks_get_Item,
    regtasks_get__NewEnum
};

HRESULT RegisteredTaskCollection_create(const WCHAR *path, IRegisteredTaskCollection **obj)
{
    RegisteredTaskCollection *regtasks;

    regtasks = malloc(sizeof(*regtasks));
    if (!regtasks) return E_OUTOFMEMORY;

    regtasks->IRegisteredTaskCollection_iface.lpVtbl = &RegisteredTaskCollection_vtbl;
    regtasks->ref = 1;
    regtasks->path = wcsdup(path);
    *obj = &regtasks->IRegisteredTaskCollection_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
