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
#include "taskschd_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    IRegisteredTask IRegisteredTask_iface;
    LONG ref;
    WCHAR *path;
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
        heap_free(regtask->path);
        heap_free(regtask);
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
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetIDsOfNames(IRegisteredTask *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_Invoke(IRegisteredTask *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Name(IRegisteredTask *iface, BSTR *name)
{
    FIXME("%p,%p: stub\n", iface, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Path(IRegisteredTask *iface, BSTR *path)
{
    FIXME("%p,%p: stub\n", iface, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_State(IRegisteredTask *iface, TASK_STATE *state)
{
    FIXME("%p,%p: stub\n", iface, state);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Enabled(IRegisteredTask *iface, VARIANT_BOOL *enabled)
{
    FIXME("%p,%p: stub\n", iface, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_put_Enabled(IRegisteredTask *iface, VARIANT_BOOL enabled)
{
    FIXME("%p,%d: stub\n", iface, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_Run(IRegisteredTask *iface, VARIANT params, IRunningTask **task)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_variant(&params), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_RunEx(IRegisteredTask *iface, VARIANT params, LONG flags,
                                    LONG session_id, BSTR user, IRunningTask **task)
{
    FIXME("%p,%s,%x,%x,%s,%p: stub\n", iface, debugstr_variant(&params), flags, session_id, debugstr_w(user), task);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetInstances(IRegisteredTask *iface, LONG flags, IRunningTaskCollection **tasks)
{
    FIXME("%p,%x,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_LastRunTime(IRegisteredTask *iface, DATE *date)
{
    FIXME("%p,%p: stub\n", iface, date);
    return E_NOTIMPL;
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
    FIXME("%p,%p: stub\n", iface, task);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_get_Xml(IRegisteredTask *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_GetSecurityDescriptor(IRegisteredTask *iface, LONG info, BSTR *sddl)
{
    FIXME("%p,%x,%p: stub\n", iface, info, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_SetSecurityDescriptor(IRegisteredTask *iface, BSTR sddl, LONG flags)
{
    FIXME("%p,%s,%x: stub\n", iface, debugstr_w(sddl), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI regtask_Stop(IRegisteredTask *iface, LONG flags)
{
    FIXME("%p,%x: stub\n", iface, flags);
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

HRESULT RegisteredTask_create(const WCHAR *path, IRegisteredTask **obj)
{
    RegisteredTask *regtask;

    regtask = heap_alloc(sizeof(*regtask));
    if (!regtask) return E_OUTOFMEMORY;

    regtask->IRegisteredTask_iface.lpVtbl = &RegisteredTask_vtbl;
    regtask->path = heap_strdupW(path);
    regtask->ref = 1;
    *obj = &regtask->IRegisteredTask_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}
