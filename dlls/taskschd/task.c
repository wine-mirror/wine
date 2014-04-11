/*
 * Copyright 2013 Dmitry Timoshkov
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
#include "xmllite.h"
#include "taskschd.h"
#include "winsvc.h"
#include "schrpc.h"
#include "taskschd_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    IRegistrationInfo IRegistrationInfo_iface;
    LONG ref;
    WCHAR *description, *author, *version, *date, *documentation, *uri, *source;
} registration_info;

static inline registration_info *impl_from_IRegistrationInfo(IRegistrationInfo *iface)
{
    return CONTAINING_RECORD(iface, registration_info, IRegistrationInfo_iface);
}

static ULONG WINAPI RegistrationInfo_AddRef(IRegistrationInfo *iface)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    return InterlockedIncrement(&reginfo->ref);
}

static ULONG WINAPI RegistrationInfo_Release(IRegistrationInfo *iface)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    LONG ref = InterlockedDecrement(&reginfo->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(reginfo->description);
        heap_free(reginfo->author);
        heap_free(reginfo->version);
        heap_free(reginfo->date);
        heap_free(reginfo->documentation);
        heap_free(reginfo->uri);
        heap_free(reginfo->source);
        heap_free(reginfo);
    }

    return ref;
}

static HRESULT WINAPI RegistrationInfo_QueryInterface(IRegistrationInfo *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IRegistrationInfo) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IRegistrationInfo_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI RegistrationInfo_GetTypeInfoCount(IRegistrationInfo *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_GetTypeInfo(IRegistrationInfo *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_GetIDsOfNames(IRegistrationInfo *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_Invoke(IRegistrationInfo *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                            DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_get_Description(IRegistrationInfo *iface, BSTR *description)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, description);

    if (!description) return E_POINTER;

    *description = SysAllocString(reginfo->description);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Description(IRegistrationInfo *iface, BSTR description)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(description));

    if (!description) return E_INVALIDARG;

    heap_free(reginfo->description);
    reginfo->description = heap_strdupW(description);
    /* FIXME: update XML on the server side */
    return reginfo->description ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_Author(IRegistrationInfo *iface, BSTR *author)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, author);

    if (!author) return E_POINTER;

    *author = SysAllocString(reginfo->author);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Author(IRegistrationInfo *iface, BSTR author)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(author));

    if (!author) return E_INVALIDARG;

    heap_free(reginfo->author);
    reginfo->author = heap_strdupW(author);
    /* FIXME: update XML on the server side */
    return reginfo->author ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_Version(IRegistrationInfo *iface, BSTR *version)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, version);

    if (!version) return E_POINTER;

    *version = SysAllocString(reginfo->version);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Version(IRegistrationInfo *iface, BSTR version)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(version));

    if (!version) return E_INVALIDARG;

    heap_free(reginfo->version);
    reginfo->version = heap_strdupW(version);
    /* FIXME: update XML on the server side */
    return reginfo->version ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_Date(IRegistrationInfo *iface, BSTR *date)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, date);

    if (!date) return E_POINTER;

    *date = SysAllocString(reginfo->date);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Date(IRegistrationInfo *iface, BSTR date)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(date));

    if (!date) return E_INVALIDARG;

    heap_free(reginfo->date);
    reginfo->date = heap_strdupW(date);
    /* FIXME: update XML on the server side */
    return reginfo->date ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_Documentation(IRegistrationInfo *iface, BSTR *doc)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, doc);

    if (!doc) return E_POINTER;

    *doc = SysAllocString(reginfo->documentation);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Documentation(IRegistrationInfo *iface, BSTR doc)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(doc));

    if (!doc) return E_INVALIDARG;

    heap_free(reginfo->documentation);
    reginfo->documentation = heap_strdupW(doc);
    /* FIXME: update XML on the server side */
    return reginfo->documentation ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_XmlText(IRegistrationInfo *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_put_XmlText(IRegistrationInfo *iface, BSTR xml)
{
    FIXME("%p,%p: stub\n", iface, debugstr_w(xml));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_get_URI(IRegistrationInfo *iface, BSTR *uri)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, uri);

    if (!uri) return E_POINTER;

    *uri = SysAllocString(reginfo->uri);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_URI(IRegistrationInfo *iface, BSTR uri)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(uri));

    if (!uri) return E_INVALIDARG;

    heap_free(reginfo->uri);
    reginfo->uri = heap_strdupW(uri);
    /* FIXME: update XML on the server side */
    return reginfo->uri ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegistrationInfo_get_SecurityDescriptor(IRegistrationInfo *iface, VARIANT *sddl)
{
    FIXME("%p,%p: stub\n", iface, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_put_SecurityDescriptor(IRegistrationInfo *iface, VARIANT sddl)
{
    FIXME("%p,%p: stub\n", iface, debugstr_variant(&sddl));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_get_Source(IRegistrationInfo *iface, BSTR *source)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, source);

    if (!source) return E_POINTER;

    *source = SysAllocString(reginfo->source);
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Source(IRegistrationInfo *iface, BSTR source)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, debugstr_w(source));

    if (!source) return E_INVALIDARG;

    heap_free(reginfo->source);
    reginfo->source = heap_strdupW(source);
    /* FIXME: update XML on the server side */
    return reginfo->source ? S_OK : E_OUTOFMEMORY;
}

static const IRegistrationInfoVtbl RegistrationInfo_vtbl =
{
    RegistrationInfo_QueryInterface,
    RegistrationInfo_AddRef,
    RegistrationInfo_Release,
    RegistrationInfo_GetTypeInfoCount,
    RegistrationInfo_GetTypeInfo,
    RegistrationInfo_GetIDsOfNames,
    RegistrationInfo_Invoke,
    RegistrationInfo_get_Description,
    RegistrationInfo_put_Description,
    RegistrationInfo_get_Author,
    RegistrationInfo_put_Author,
    RegistrationInfo_get_Version,
    RegistrationInfo_put_Version,
    RegistrationInfo_get_Date,
    RegistrationInfo_put_Date,
    RegistrationInfo_get_Documentation,
    RegistrationInfo_put_Documentation,
    RegistrationInfo_get_XmlText,
    RegistrationInfo_put_XmlText,
    RegistrationInfo_get_URI,
    RegistrationInfo_put_URI,
    RegistrationInfo_get_SecurityDescriptor,
    RegistrationInfo_put_SecurityDescriptor,
    RegistrationInfo_get_Source,
    RegistrationInfo_put_Source
};

static HRESULT RegistrationInfo_create(IRegistrationInfo **obj)
{
    registration_info *reginfo;

    reginfo = heap_alloc_zero(sizeof(*reginfo));
    if (!reginfo) return E_OUTOFMEMORY;

    reginfo->IRegistrationInfo_iface.lpVtbl = &RegistrationInfo_vtbl;
    reginfo->ref = 1;
    *obj = &reginfo->IRegistrationInfo_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    ITaskSettings ITaskSettings_iface;
    LONG ref;
    WCHAR *restart_interval;
    WCHAR *execution_time_limit;
    WCHAR *delete_expired_task_after;
    int restart_count;
    int priority;
    TASK_INSTANCES_POLICY policy;
    TASK_COMPATIBILITY compatibility;
    BOOL allow_on_demand_start;
    BOOL stop_if_going_on_batteries;
    BOOL disallow_start_if_on_batteries;
    BOOL allow_hard_terminate;
    BOOL start_when_available;
    BOOL run_only_if_network_available;
    BOOL enabled;
    BOOL hidden;
    BOOL run_only_if_idle;
    BOOL wake_to_run;
} TaskSettings;

static inline TaskSettings *impl_from_ITaskSettings(ITaskSettings *iface)
{
    return CONTAINING_RECORD(iface, TaskSettings, ITaskSettings_iface);
}

static ULONG WINAPI TaskSettings_AddRef(ITaskSettings *iface)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);
    return InterlockedIncrement(&taskset->ref);
}

static ULONG WINAPI TaskSettings_Release(ITaskSettings *iface)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);
    LONG ref = InterlockedDecrement(&taskset->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(taskset->restart_interval);
        heap_free(taskset->execution_time_limit);
        heap_free(taskset->delete_expired_task_after);
        heap_free(taskset);
    }

    return ref;
}

static HRESULT WINAPI TaskSettings_QueryInterface(ITaskSettings *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskSettings) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskSettings_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskSettings_GetTypeInfoCount(ITaskSettings *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_GetTypeInfo(ITaskSettings *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_GetIDsOfNames(ITaskSettings *iface, REFIID riid, LPOLESTR *names,
                                                 UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_Invoke(ITaskSettings *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                          DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_AllowDemandStart(ITaskSettings *iface, VARIANT_BOOL *allow)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, allow);

    if (!allow) return E_POINTER;

    *allow = taskset->allow_on_demand_start ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_AllowDemandStart(ITaskSettings *iface, VARIANT_BOOL allow)
{
    FIXME("%p,%d: stub\n", iface, allow);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_RestartInterval(ITaskSettings *iface, BSTR *interval)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, interval);

    if (!interval) return E_POINTER;

    if (!taskset->restart_interval)
    {
        *interval = NULL;
        return S_OK;
    }

    *interval = SysAllocString(taskset->restart_interval);
    if (!*interval) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_RestartInterval(ITaskSettings *iface, BSTR interval)
{
    TRACE("%p,%s\n", iface, debugstr_w(interval));
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_RestartCount(ITaskSettings *iface, INT *count)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, count);

    if (!count) return E_POINTER;

    *count = taskset->restart_count;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_RestartCount(ITaskSettings *iface, INT count)
{
    FIXME("%p,%d: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_MultipleInstances(ITaskSettings *iface, TASK_INSTANCES_POLICY *policy)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, policy);

    if (!policy) return E_POINTER;

    *policy = taskset->policy;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_MultipleInstances(ITaskSettings *iface, TASK_INSTANCES_POLICY policy)
{
    FIXME("%p,%d: stub\n", iface, policy);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_StopIfGoingOnBatteries(ITaskSettings *iface, VARIANT_BOOL *stop)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, stop);

    if (!stop) return E_POINTER;

    *stop = taskset->stop_if_going_on_batteries ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_StopIfGoingOnBatteries(ITaskSettings *iface, VARIANT_BOOL stop)
{
    FIXME("%p,%d: stub\n", iface, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_DisallowStartIfOnBatteries(ITaskSettings *iface, VARIANT_BOOL *disallow)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, disallow);

    if (!disallow) return E_POINTER;

    *disallow = taskset->disallow_start_if_on_batteries ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_DisallowStartIfOnBatteries(ITaskSettings *iface, VARIANT_BOOL disallow)
{
    FIXME("%p,%d: stub\n", iface, disallow);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_AllowHardTerminate(ITaskSettings *iface, VARIANT_BOOL *allow)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, allow);

    if (!allow) return E_POINTER;

    *allow = taskset->allow_hard_terminate ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_AllowHardTerminate(ITaskSettings *iface, VARIANT_BOOL allow)
{
    FIXME("%p,%d: stub\n", iface, allow);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_StartWhenAvailable(ITaskSettings *iface, VARIANT_BOOL *start)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, start);

    if (!start) return E_POINTER;

    *start = taskset->start_when_available ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_StartWhenAvailable(ITaskSettings *iface, VARIANT_BOOL start)
{
    FIXME("%p,%d: stub\n", iface, start);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_XmlText(ITaskSettings *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_put_XmlText(ITaskSettings *iface, BSTR xml)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(xml));
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_RunOnlyIfNetworkAvailable(ITaskSettings *iface, VARIANT_BOOL *run)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, run);

    if (!run) return E_POINTER;

    *run = taskset->run_only_if_network_available ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_RunOnlyIfNetworkAvailable(ITaskSettings *iface, VARIANT_BOOL run)
{
    FIXME("%p,%d: stub\n", iface, run);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_ExecutionTimeLimit(ITaskSettings *iface, BSTR *limit)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, limit);

    if (!limit) return E_POINTER;

    if (!taskset->execution_time_limit)
    {
        *limit = NULL;
        return S_OK;
    }

    *limit = SysAllocString(taskset->execution_time_limit);
    if (!*limit) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_ExecutionTimeLimit(ITaskSettings *iface, BSTR limit)
{
    TRACE("%p,%s\n", iface, debugstr_w(limit));
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_Enabled(ITaskSettings *iface, VARIANT_BOOL *enabled)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, enabled);

    if (!enabled) return E_POINTER;

    *enabled = taskset->enabled ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_Enabled(ITaskSettings *iface, VARIANT_BOOL enabled)
{
    FIXME("%p,%d: stub\n", iface, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_DeleteExpiredTaskAfter(ITaskSettings *iface, BSTR *delay)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, delay);

    if (!delay) return E_POINTER;

    if (!taskset->delete_expired_task_after)
    {
        *delay = NULL;
        return S_OK;
    }

    *delay = SysAllocString(taskset->delete_expired_task_after);
    if (!*delay) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_DeleteExpiredTaskAfter(ITaskSettings *iface, BSTR delay)
{
    TRACE("%p,%s\n", iface, debugstr_w(delay));
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_Priority(ITaskSettings *iface, INT *priority)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, priority);

    if (!priority) return E_POINTER;

    *priority = taskset->priority;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_Priority(ITaskSettings *iface, INT priority)
{
    FIXME("%p,%d: stub\n", iface, priority);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_Compatibility(ITaskSettings *iface, TASK_COMPATIBILITY *level)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, level);

    if (!level) return E_POINTER;

    *level = taskset->compatibility;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_Compatibility(ITaskSettings *iface, TASK_COMPATIBILITY level)
{
    FIXME("%p,%d: stub\n", iface, level);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_Hidden(ITaskSettings *iface, VARIANT_BOOL *hidden)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, hidden);

    if (!hidden) return E_POINTER;

    *hidden = taskset->hidden ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_Hidden(ITaskSettings *iface, VARIANT_BOOL hidden)
{
    FIXME("%p,%d: stub\n", iface, hidden);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_IdleSettings(ITaskSettings *iface, IIdleSettings **settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_put_IdleSettings(ITaskSettings *iface, IIdleSettings *settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_RunOnlyIfIdle(ITaskSettings *iface, VARIANT_BOOL *run)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, run);

    if (!run) return E_POINTER;

    *run = taskset->run_only_if_idle ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_RunOnlyIfIdle(ITaskSettings *iface, VARIANT_BOOL run)
{
    FIXME("%p,%d: stub\n", iface, run);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_WakeToRun(ITaskSettings *iface, VARIANT_BOOL *wake)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, wake);

    if (!wake) return E_POINTER;

    *wake = taskset->wake_to_run ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_WakeToRun(ITaskSettings *iface, VARIANT_BOOL wake)
{
    FIXME("%p,%d: stub\n", iface, wake);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_get_NetworkSettings(ITaskSettings *iface, INetworkSettings **settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_put_NetworkSettings(ITaskSettings *iface, INetworkSettings *settings)
{
    FIXME("%p,%p: stub\n", iface, settings);
    return E_NOTIMPL;
}

static const ITaskSettingsVtbl TaskSettings_vtbl =
{
    TaskSettings_QueryInterface,
    TaskSettings_AddRef,
    TaskSettings_Release,
    TaskSettings_GetTypeInfoCount,
    TaskSettings_GetTypeInfo,
    TaskSettings_GetIDsOfNames,
    TaskSettings_Invoke,
    TaskSettings_get_AllowDemandStart,
    TaskSettings_put_AllowDemandStart,
    TaskSettings_get_RestartInterval,
    TaskSettings_put_RestartInterval,
    TaskSettings_get_RestartCount,
    TaskSettings_put_RestartCount,
    TaskSettings_get_MultipleInstances,
    TaskSettings_put_MultipleInstances,
    TaskSettings_get_StopIfGoingOnBatteries,
    TaskSettings_put_StopIfGoingOnBatteries,
    TaskSettings_get_DisallowStartIfOnBatteries,
    TaskSettings_put_DisallowStartIfOnBatteries,
    TaskSettings_get_AllowHardTerminate,
    TaskSettings_put_AllowHardTerminate,
    TaskSettings_get_StartWhenAvailable,
    TaskSettings_put_StartWhenAvailable,
    TaskSettings_get_XmlText,
    TaskSettings_put_XmlText,
    TaskSettings_get_RunOnlyIfNetworkAvailable,
    TaskSettings_put_RunOnlyIfNetworkAvailable,
    TaskSettings_get_ExecutionTimeLimit,
    TaskSettings_put_ExecutionTimeLimit,
    TaskSettings_get_Enabled,
    TaskSettings_put_Enabled,
    TaskSettings_get_DeleteExpiredTaskAfter,
    TaskSettings_put_DeleteExpiredTaskAfter,
    TaskSettings_get_Priority,
    TaskSettings_put_Priority,
    TaskSettings_get_Compatibility,
    TaskSettings_put_Compatibility,
    TaskSettings_get_Hidden,
    TaskSettings_put_Hidden,
    TaskSettings_get_IdleSettings,
    TaskSettings_put_IdleSettings,
    TaskSettings_get_RunOnlyIfIdle,
    TaskSettings_put_RunOnlyIfIdle,
    TaskSettings_get_WakeToRun,
    TaskSettings_put_WakeToRun,
    TaskSettings_get_NetworkSettings,
    TaskSettings_put_NetworkSettings
};

static HRESULT TaskSettings_create(ITaskSettings **obj)
{
    static const WCHAR exec_time_limit[] = { 'P','T','7','2','H',0 };
    TaskSettings *taskset;

    taskset = heap_alloc(sizeof(*taskset));
    if (!taskset) return E_OUTOFMEMORY;

    taskset->ITaskSettings_iface.lpVtbl = &TaskSettings_vtbl;
    taskset->ref = 1;
    /* set the defaults */
    taskset->restart_interval = NULL;
    taskset->execution_time_limit = heap_strdupW(exec_time_limit);
    taskset->delete_expired_task_after = NULL;
    taskset->restart_count = 0;
    taskset->priority = 7;
    taskset->policy = TASK_INSTANCES_IGNORE_NEW;
    taskset->compatibility = TASK_COMPATIBILITY_V2;
    taskset->allow_on_demand_start = TRUE;
    taskset->stop_if_going_on_batteries = TRUE;
    taskset->disallow_start_if_on_batteries = TRUE;
    taskset->allow_hard_terminate = TRUE;
    taskset->start_when_available = FALSE;
    taskset->run_only_if_network_available = FALSE;
    taskset->enabled = TRUE;
    taskset->hidden = FALSE;
    taskset->run_only_if_idle = FALSE;
    taskset->wake_to_run = FALSE;

    *obj = &taskset->ITaskSettings_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    ITaskDefinition ITaskDefinition_iface;
    LONG ref;
    IRegistrationInfo *reginfo;
    ITaskSettings *taskset;
    ITriggerCollection *triggers;
    IPrincipal *principal;
    IActionCollection *actions;
} TaskDefinition;

static inline TaskDefinition *impl_from_ITaskDefinition(ITaskDefinition *iface)
{
    return CONTAINING_RECORD(iface, TaskDefinition, ITaskDefinition_iface);
}

static ULONG WINAPI TaskDefinition_AddRef(ITaskDefinition *iface)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    return InterlockedIncrement(&taskdef->ref);
}

static ULONG WINAPI TaskDefinition_Release(ITaskDefinition *iface)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    LONG ref = InterlockedDecrement(&taskdef->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);

        if (taskdef->reginfo)
            IRegistrationInfo_Release(taskdef->reginfo);
        if (taskdef->taskset)
            ITaskSettings_Release(taskdef->taskset);
        if (taskdef->triggers)
            ITriggerCollection_Release(taskdef->triggers);
        if (taskdef->principal)
            IPrincipal_Release(taskdef->principal);
        if (taskdef->actions)
            IActionCollection_Release(taskdef->actions);

        heap_free(taskdef);
    }

    return ref;
}

static HRESULT WINAPI TaskDefinition_QueryInterface(ITaskDefinition *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskDefinition) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskDefinition_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskDefinition_GetTypeInfoCount(ITaskDefinition *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_GetTypeInfo(ITaskDefinition *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_GetIDsOfNames(ITaskDefinition *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_Invoke(ITaskDefinition *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                            DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_RegistrationInfo(ITaskDefinition *iface, IRegistrationInfo **info)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_POINTER;

    if (!taskdef->reginfo)
    {
        hr = RegistrationInfo_create(&taskdef->reginfo);
        if (hr != S_OK) return hr;
    }

    IRegistrationInfo_AddRef(taskdef->reginfo);
    *info = taskdef->reginfo;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_RegistrationInfo(ITaskDefinition *iface, IRegistrationInfo *info)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_POINTER;

    if (taskdef->reginfo)
        IRegistrationInfo_Release(taskdef->reginfo);

    IRegistrationInfo_AddRef(info);
    taskdef->reginfo = info;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_get_Triggers(ITaskDefinition *iface, ITriggerCollection **triggers)
{
    FIXME("%p,%p: stub\n", iface, triggers);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Triggers(ITaskDefinition *iface, ITriggerCollection *triggers)
{
    FIXME("%p,%p: stub\n", iface, triggers);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Settings(ITaskDefinition *iface, ITaskSettings **settings)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, settings);

    if (!settings) return E_POINTER;

    if (!taskdef->taskset)
    {
        hr = TaskSettings_create(&taskdef->taskset);
        if (hr != S_OK) return hr;
    }

    ITaskSettings_AddRef(taskdef->taskset);
    *settings = taskdef->taskset;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_Settings(ITaskDefinition *iface, ITaskSettings *settings)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, settings);

    if (!settings) return E_POINTER;

    if (taskdef->taskset)
        ITaskSettings_Release(taskdef->taskset);

    ITaskSettings_AddRef(settings);
    taskdef->taskset = settings;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_get_Data(ITaskDefinition *iface, BSTR *data)
{
    FIXME("%p,%p: stub\n", iface, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Data(ITaskDefinition *iface, BSTR data)
{
    FIXME("%p,%p: stub\n", iface, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Principal(ITaskDefinition *iface, IPrincipal **principal)
{
    FIXME("%p,%p: stub\n", iface, principal);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Principal(ITaskDefinition *iface, IPrincipal *principal)
{
    FIXME("%p,%p: stub\n", iface, principal);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_get_Actions(ITaskDefinition *iface, IActionCollection **actions)
{
    FIXME("%p,%p: stub\n", iface, actions);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_put_Actions(ITaskDefinition *iface, IActionCollection *actions)
{
    FIXME("%p,%p: stub\n", iface, actions);
    return E_NOTIMPL;
}

static const WCHAR Task[] = {'T','a','s','k',0};
static const WCHAR version[] = {'v','e','r','s','i','o','n',0};
static const WCHAR v1_0[] = {'1','.','0',0};
static const WCHAR v1_1[] = {'1','.','1',0};
static const WCHAR v1_2[] = {'1','.','2',0};
static const WCHAR v1_3[] = {'1','.','3',0};
static const WCHAR xmlns[] = {'x','m','l','n','s',0};
static const WCHAR task_ns[] = {'h','t','t','p',':','/','/','s','c','h','e','m','a','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','w','i','n','d','o','w','s','/','2','0','0','4','/','0','2','/','m','i','t','/','t','a','s','k',0};
static const WCHAR RegistrationInfo[] = {'R','e','g','i','s','t','r','a','t','i','o','n','I','n','f','o',0};
static const WCHAR Author[] = {'A','u','t','h','o','r',0};
static const WCHAR Description[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR Source[] = {'S','o','u','r','c','e',0};
static const WCHAR Date[] = {'D','a','t','e',0};
static const WCHAR Version[] = {'V','e','r','s','i','o','n',0};
static const WCHAR Documentation[] = {'D','o','c','u','m','e','n','t','a','t','i','o','n',0};
static const WCHAR URI[] = {'U','R','I',0};
static const WCHAR SecurityDescriptor[] = {'S','e','c','u','r','i','t','y','D','e','s','c','r','i','p','t','o','r',0};
static const WCHAR Settings[] = {'S','e','t','t','i','n','g','s',0};
static const WCHAR Triggers[] = {'T','r','i','g','g','e','r','s',0};
static const WCHAR Principals[] = {'P','r','i','n','c','i','p','a','l','s',0};
static const WCHAR Principal[] = {'P','r','i','n','c','i','p','a','l',0};
static const WCHAR id[] = {'i','d',0};
static const WCHAR UserId[] = {'U','s','e','r','I','d',0};
static const WCHAR LogonType[] = {'L','o','g','o','n','T','y','p','e',0};
static const WCHAR GroupId[] = {'G','r','o','u','p','I','d',0};
static const WCHAR DisplayName[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR HighestAvailable[] = {'H','i','g','h','e','s','t','A','v','a','i','l','a','b','l','e',0};
static const WCHAR Password[] = {'P','a','s','s','w','o','r','d',0};
static const WCHAR S4U[] = {'S','4','U',0};
static const WCHAR InteractiveToken[] = {'I','n','t','e','r','a','c','t','i','v','e','T','o','k','e','n',0};
static const WCHAR RunLevel[] = {'R','u','n','L','e','v','e','l',0};
static const WCHAR LeastPrivilege[] = {'L','e','a','s','t','P','r','i','v','i','l','e','g','e',0};
static const WCHAR Actions[] = {'A','c','t','i','o','n','s',0};
static const WCHAR Exec[] = {'E','x','e','c',0};
static const WCHAR MultipleInstancesPolicy[] = {'M','u','l','t','i','p','l','e','I','n','s','t','a','n','c','e','s','P','o','l','i','c','y',0};
static const WCHAR IgnoreNew[] = {'I','g','n','o','r','e','N','e','w',0};
static const WCHAR DisallowStartIfOnBatteries[] = {'D','i','s','a','l','l','o','w','S','t','a','r','t','I','f','O','n','B','a','t','t','e','r','i','e','s',0};
static const WCHAR AllowStartOnDemand[] = {'A','l','l','o','w','S','t','a','r','t','O','n','D','e','m','a','n','d',0};
static const WCHAR StopIfGoingOnBatteries[] = {'S','t','o','p','I','f','G','o','i','n','g','O','n','B','a','t','t','e','r','i','e','s',0};
static const WCHAR AllowHardTerminate[] = {'A','l','l','o','w','H','a','r','d','T','e','r','m','i','n','a','t','e',0};
static const WCHAR StartWhenAvailable[] = {'S','t','a','r','t','W','h','e','n','A','v','a','i','l','a','b','l','e',0};
static const WCHAR RunOnlyIfNetworkAvailable[] = {'R','u','n','O','n','l','y','I','f','N','e','t','w','o','r','k','A','v','a','i','l','a','b','l','e',0};
static const WCHAR Enabled[] = {'E','n','a','b','l','e','d',0};
static const WCHAR Hidden[] = {'H','i','d','d','e','n',0};
static const WCHAR RunOnlyIfIdle[] = {'R','u','n','O','n','l','y','I','f','I','d','l','e',0};
static const WCHAR WakeToRun[] = {'W','a','k','e','T','o','R','u','n',0};
static const WCHAR ExecutionTimeLimit[] = {'E','x','e','c','u','t','i','o','n','T','i','m','e','L','i','m','i','t',0};
static const WCHAR Priority[] = {'P','r','i','o','r','i','t','y',0};
static const WCHAR IdleSettings[] = {'I','d','l','e','S','e','t','t','i','n','g','s',0};

static int xml_indent;

static inline void push_indent(void)
{
    xml_indent += 2;
}

static inline void pop_indent(void)
{
    xml_indent -= 2;
}

static inline HRESULT write_stringW(IStream *stream, const WCHAR *str)
{
    return IStream_Write(stream, str, lstrlenW(str) * sizeof(WCHAR), NULL);
}

static void write_indent(IStream *stream)
{
    static const WCHAR spacesW[] = {' ',' ',0};
    int i;
    for (i = 0; i < xml_indent; i += 2)
        write_stringW(stream, spacesW);
}

static const WCHAR start_element[] = {'<',0};
static const WCHAR start_end_element[] = {'<','/',0};
static const WCHAR close_element[] = {'>',0};
static const WCHAR end_empty_element[] = {'/','>',0};
static const WCHAR eol[] = {'\n',0};
static const WCHAR spaceW[] = {' ',0};
static const WCHAR equalW[] = {'=',0};
static const WCHAR quoteW[] = {'"',0};

static inline HRESULT write_empty_element(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, start_element);
    write_stringW(stream, name);
    write_stringW(stream, end_empty_element);
    return write_stringW(stream, eol);
}

static inline HRESULT write_element(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, start_element);
    write_stringW(stream, name);
    write_stringW(stream, close_element);
    return write_stringW(stream, eol);
}

static inline HRESULT write_element_end(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, start_end_element);
    write_stringW(stream, name);
    write_stringW(stream, close_element);
    return write_stringW(stream, eol);
}

static inline HRESULT write_text_value(IStream *stream, const WCHAR *name, const WCHAR *value)
{
    write_indent(stream);
    write_stringW(stream, start_element);
    write_stringW(stream, name);
    write_stringW(stream, close_element);
    write_stringW(stream, value);
    write_stringW(stream, start_end_element);
    write_stringW(stream, name);
    write_stringW(stream, close_element);
    return write_stringW(stream, eol);
}

static HRESULT write_task_attributes(IStream *stream, ITaskDefinition *taskdef)
{
    HRESULT hr;
    ITaskSettings *taskset;
    TASK_COMPATIBILITY level;
    const WCHAR *compatibility;

    hr = ITaskDefinition_get_Settings(taskdef, &taskset);
    if (hr != S_OK) return hr;

    hr = ITaskSettings_get_Compatibility(taskset, &level);
    if (hr != S_OK) level = TASK_COMPATIBILITY_V2_1;

    ITaskSettings_Release(taskset);

    switch (level)
    {
    case TASK_COMPATIBILITY_AT:
        compatibility = v1_0;
        break;
    case TASK_COMPATIBILITY_V1:
        compatibility = v1_1;
        break;
    case TASK_COMPATIBILITY_V2:
        compatibility = v1_2;
        break;
    default:
        compatibility = v1_3;
        break;
    }

    write_stringW(stream, start_element);
    write_stringW(stream, Task);
    write_stringW(stream, spaceW);
    write_stringW(stream, version);
    write_stringW(stream, equalW);
    write_stringW(stream, quoteW);
    write_stringW(stream, compatibility);
    write_stringW(stream, quoteW);
    write_stringW(stream, spaceW);
    write_stringW(stream, xmlns);
    write_stringW(stream, equalW);
    write_stringW(stream, quoteW);
    write_stringW(stream, task_ns);
    write_stringW(stream, quoteW);
    write_stringW(stream, close_element);
    return write_stringW(stream, eol);
}

static HRESULT write_registration_info(IStream *stream, IRegistrationInfo *reginfo)
{
    HRESULT hr;
    BSTR bstr;
    VARIANT var;

    if (!reginfo)
        return write_empty_element(stream, RegistrationInfo);

    hr = write_element(stream, RegistrationInfo);
    if (hr != S_OK) return hr;

    push_indent();

    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Source, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Date, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Author, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Version, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Description, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, Documentation, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, URI, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_SecurityDescriptor(reginfo, &var);
    if (hr == S_OK)
    {
        if (V_VT(&var) == VT_BSTR)
        {
            hr = write_text_value(stream, SecurityDescriptor, V_BSTR(&var));
            VariantClear(&var);
            if (hr != S_OK) return hr;
        }
        else
            FIXME("SecurityInfo variant type %d is not supported\n", V_VT(&var));
    }

    pop_indent();

    return write_element_end(stream, RegistrationInfo);
}

static HRESULT write_principal(IStream *stream, IPrincipal *principal)
{
    HRESULT hr;
    BSTR bstr;
    TASK_LOGON_TYPE logon;
    TASK_RUNLEVEL_TYPE level;

    if (!principal)
        return write_empty_element(stream, Principals);

    hr = write_element(stream, Principals);
    if (hr != S_OK) return hr;

    push_indent();

    hr = IPrincipal_get_Id(principal, &bstr);
    if (hr == S_OK)
    {
        write_indent(stream);
        write_stringW(stream, start_element);
        write_stringW(stream, Principal);
        write_stringW(stream, spaceW);
        write_stringW(stream, id);
        write_stringW(stream, equalW);
        write_stringW(stream, quoteW);
        write_stringW(stream, bstr);
        write_stringW(stream, quoteW);
        write_stringW(stream, close_element);
        write_stringW(stream, eol);
        SysFreeString(bstr);
    }
    else
        write_element(stream, Principal);

    push_indent();

    hr = IPrincipal_get_GroupId(principal, &bstr);
    if (hr == S_OK)
    {
        hr = write_text_value(stream, GroupId, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IPrincipal_get_DisplayName(principal, &bstr);
    if (hr == S_OK)
    {
        hr = write_text_value(stream, DisplayName, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IPrincipal_get_UserId(principal, &bstr);
    if (hr == S_OK && lstrlenW(bstr))
    {
        hr = write_text_value(stream, UserId, bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IPrincipal_get_RunLevel(principal, &level);
    if (hr == S_OK)
    {
        const WCHAR *level_str = NULL;

        switch (level)
        {
        case TASK_RUNLEVEL_HIGHEST:
            level_str = HighestAvailable;
            break;
        case TASK_RUNLEVEL_LUA:
            level_str = LeastPrivilege;
            break;
        default:
            FIXME("Principal run level %d\n", level);
            break;
        }

        if (level_str)
        {
            hr = write_text_value(stream, RunLevel, level_str);
            if (hr != S_OK) return hr;
        }
    }
    hr = IPrincipal_get_LogonType(principal, &logon);
    if (hr == S_OK)
    {
        const WCHAR *logon_str = NULL;

        switch (logon)
        {
        case TASK_LOGON_PASSWORD:
            logon_str = Password;
            break;
        case TASK_LOGON_S4U:
            logon_str = S4U;
            break;
        case TASK_LOGON_INTERACTIVE_TOKEN:
            logon_str = InteractiveToken;
            break;
        default:
            FIXME("Principal logon type %d\n", logon);
            break;
        }

        if (logon_str)
        {
            hr = write_text_value(stream, LogonType, logon_str);
            if (hr != S_OK) return hr;
        }
    }

    pop_indent();
    write_element_end(stream, Principal);

    pop_indent();
    return write_element_end(stream, Principals);
}

static HRESULT write_settings(IStream *stream, ITaskSettings *settings)
{
    if (!settings)
        return write_empty_element(stream, Settings);

    FIXME("stub\n");
    return S_OK;
}

static HRESULT write_triggers(IStream *stream, ITriggerCollection *triggers)
{
    if (!triggers)
        return write_empty_element(stream, Triggers);

    FIXME("stub\n");
    return S_OK;
}

static HRESULT write_actions(IStream *stream, IActionCollection *actions)
{
    if (!actions)
    {
        write_element(stream, Actions);
        push_indent();
        write_empty_element(stream, Exec);
        pop_indent();
        return write_element_end(stream, Actions);
    }

    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI TaskDefinition_get_XmlText(ITaskDefinition *iface, BSTR *xml)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;
    IStream *stream;
    HGLOBAL hmem;
    void *p;

    TRACE("%p,%p\n", iface, xml);

    hmem = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, 16);
    if (!hmem) return E_OUTOFMEMORY;

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    if (hr != S_OK)
    {
        GlobalFree(hmem);
        return hr;
    }

    hr = write_task_attributes(stream, &taskdef->ITaskDefinition_iface);
    if (hr != S_OK) goto failed;

    push_indent();

    hr = write_registration_info(stream, taskdef->reginfo);
    if (hr != S_OK) goto failed;

    hr = write_triggers(stream, taskdef->triggers);
    if (hr != S_OK) goto failed;

    hr = write_principal(stream, taskdef->principal);
    if (hr != S_OK) goto failed;

    hr = write_settings(stream, taskdef->taskset);
    if (hr != S_OK) goto failed;

    hr = write_actions(stream, taskdef->actions);
    if (hr != S_OK) goto failed;

    pop_indent();

    write_element_end(stream, Task);
    IStream_Write(stream, "\0\0", 2, NULL);

    p = GlobalLock(hmem);
    *xml = SysAllocString(p);
    GlobalUnlock(hmem);

    IStream_Release(stream);

    return *xml ? S_OK : E_OUTOFMEMORY;

failed:
    IStream_Release(stream);
    return hr;
}

static HRESULT read_text_value(IXmlReader *reader, WCHAR **value)
{
    HRESULT hr;
    XmlNodeType type;

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_Text:
            hr = IXmlReader_GetValue(reader, (const WCHAR **)value, NULL);
            if (hr != S_OK) return hr;
            TRACE("%s\n", debugstr_w(*value));
            return S_OK;

        case XmlNodeType_Whitespace:
        case XmlNodeType_Comment:
            break;

        default:
            FIXME("unexpected node type %d\n", type);
            return E_FAIL;
        }
    }

    return E_FAIL;
}

static HRESULT read_variantbool_value(IXmlReader *reader, VARIANT_BOOL *vbool)
{
    static const WCHAR trueW[] = {'t','r','u','e',0};
    static const WCHAR falseW[] = {'f','a','l','s','e',0};
    HRESULT hr;
    WCHAR *value;

    hr = read_text_value(reader, &value);
    if (hr != S_OK) return hr;

    if (!lstrcmpW(value, trueW))
        *vbool = VARIANT_TRUE;
    else if (!lstrcmpW(value, falseW))
        *vbool = VARIANT_FALSE;
    else
    {
        WARN("unexpected bool value %s\n", debugstr_w(value));
        return SCHED_E_INVALIDVALUE;
    }

    return S_OK;
}

static HRESULT read_int_value(IXmlReader *reader, int *int_val)
{
    HRESULT hr;
    WCHAR *value;

    hr = read_text_value(reader, &value);
    if (hr != S_OK) return hr;

    *int_val = strtolW(value, NULL, 10);

    return S_OK;
}

static HRESULT read_triggers(IXmlReader *reader, ITaskDefinition *taskdef)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT read_principal_attributes(IXmlReader *reader, IPrincipal *principal)
{
    HRESULT hr;
    const WCHAR *name;
    const WCHAR *value;

    hr = IXmlReader_MoveToFirstAttribute(reader);

    while (hr == S_OK)
    {
        hr = IXmlReader_GetLocalName(reader, &name, NULL);
        if (hr != S_OK) break;

        hr = IXmlReader_GetValue(reader, &value, NULL);
        if (hr != S_OK) break;

        TRACE("%s=%s\n", debugstr_w(name), debugstr_w(value));

        if (!lstrcmpW(name, id))
            IPrincipal_put_Id(principal, (BSTR)value);
        else
            FIXME("unhandled Principal attribute %s\n", debugstr_w(name));

        hr = IXmlReader_MoveToNextAttribute(reader);
    }

    return S_OK;
}

static HRESULT read_principal(IXmlReader *reader, IPrincipal *principal)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;
    WCHAR *value;

    if (IXmlReader_IsEmptyElement(reader))
    {
        TRACE("Principal is empty\n");
        return S_OK;
    }

    read_principal_attributes(reader, principal);

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_EndElement:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("/%s\n", debugstr_w(name));

            if (!lstrcmpW(name, Principal))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, UserId))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IPrincipal_put_UserId(principal, value);
            }
            else if (!lstrcmpW(name, LogonType))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    TASK_LOGON_TYPE logon = TASK_LOGON_NONE;

                    if (!lstrcmpW(value, InteractiveToken))
                        logon = TASK_LOGON_INTERACTIVE_TOKEN;
                    else
                        FIXME("unhandled LogonType %s\n", debugstr_w(value));

                    IPrincipal_put_LogonType(principal, logon);
                }
            }
            else if (!lstrcmpW(name, RunLevel))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    TASK_RUNLEVEL_TYPE level = TASK_RUNLEVEL_LUA;

                    if (!lstrcmpW(value, LeastPrivilege))
                        level = TASK_RUNLEVEL_LUA;
                    else
                        FIXME("unhandled RunLevel %s\n", debugstr_w(value));

                    IPrincipal_put_RunLevel(principal, level);
                }
            }
            else
                FIXME("unhandled Principal element %s\n", debugstr_w(name));

            break;

        case XmlNodeType_Whitespace:
        case XmlNodeType_Comment:
            break;

        default:
            FIXME("unhandled Principal node type %d\n", type);
            break;
        }
    }

    WARN("Principal was not terminated\n");
    return E_FAIL;
}

static HRESULT read_principals(IXmlReader *reader, ITaskDefinition *taskdef)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;

    if (IXmlReader_IsEmptyElement(reader))
    {
        TRACE("Principals is empty\n");
        return S_OK;
    }

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_EndElement:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("/%s\n", debugstr_w(name));

            if (!lstrcmpW(name, Principals))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, Principal))
            {
                IPrincipal *principal;

                hr = ITaskDefinition_get_Principal(taskdef, &principal);
                if (hr != S_OK) return hr;
                hr = read_principal(reader, principal);
                IPrincipal_Release(principal);
            }
            else
                FIXME("unhandled Principals element %s\n", debugstr_w(name));

            break;

        case XmlNodeType_Whitespace:
        case XmlNodeType_Comment:
            break;

        default:
            FIXME("unhandled Principals node type %d\n", type);
            break;
        }
    }

    WARN("Principals was not terminated\n");
    return E_FAIL;
}

static HRESULT read_actions(IXmlReader *reader, ITaskDefinition *taskdef)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT read_idle_settings(IXmlReader *reader, ITaskSettings *taskset)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT read_settings(IXmlReader *reader, ITaskSettings *taskset)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;
    WCHAR *value;
    VARIANT_BOOL bool_val;
    int int_val;

    if (IXmlReader_IsEmptyElement(reader))
    {
        TRACE("Settings is empty\n");
        return S_OK;
    }

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_EndElement:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("/%s\n", debugstr_w(name));

            if (!lstrcmpW(name, Settings))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, MultipleInstancesPolicy))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    int_val = TASK_INSTANCES_IGNORE_NEW;

                    if (!lstrcmpW(value, IgnoreNew))
                        int_val = TASK_INSTANCES_IGNORE_NEW;
                    else
                        FIXME("unhandled MultipleInstancesPolicy %s\n", debugstr_w(value));

                    ITaskSettings_put_MultipleInstances(taskset, int_val);
                }
            }
            else if (!lstrcmpW(name, DisallowStartIfOnBatteries))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_DisallowStartIfOnBatteries(taskset, bool_val);
            }
            else if (!lstrcmpW(name, AllowStartOnDemand))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_AllowDemandStart(taskset, bool_val);
            }
            else if (!lstrcmpW(name, StopIfGoingOnBatteries))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_StopIfGoingOnBatteries(taskset, bool_val);
            }
            else if (!lstrcmpW(name, AllowHardTerminate))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_AllowHardTerminate(taskset, bool_val);
            }
            else if (!lstrcmpW(name, StartWhenAvailable))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_StartWhenAvailable(taskset, bool_val);
            }
            else if (!lstrcmpW(name, RunOnlyIfNetworkAvailable))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_RunOnlyIfNetworkAvailable(taskset, bool_val);
            }
            else if (!lstrcmpW(name, Enabled))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_Enabled(taskset, bool_val);
            }
            else if (!lstrcmpW(name, Hidden))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_Hidden(taskset, bool_val);
            }
            else if (!lstrcmpW(name, RunOnlyIfIdle))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_RunOnlyIfIdle(taskset, bool_val);
            }
            else if (!lstrcmpW(name, WakeToRun))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_WakeToRun(taskset, bool_val);
            }
            else if (!lstrcmpW(name, ExecutionTimeLimit))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    ITaskSettings_put_ExecutionTimeLimit(taskset, value);
            }
            else if (!lstrcmpW(name, Priority))
            {
                hr = read_int_value(reader, &int_val);
                if (hr == S_OK)
                    ITaskSettings_put_Priority(taskset, int_val);
            }
            else if (!lstrcmpW(name, IdleSettings))
            {
                hr = read_idle_settings(reader, taskset);
                if (hr != S_OK) return hr;
            }
            else
                FIXME("unhandled Settings element %s\n", debugstr_w(name));

            break;

        case XmlNodeType_Whitespace:
        case XmlNodeType_Comment:
            break;

        default:
            FIXME("unhandled Settings node type %d\n", type);
            break;
        }
    }

    WARN("Settings was not terminated\n");
    return SCHED_E_MALFORMEDXML;
}

static HRESULT read_registration_info(IXmlReader *reader, IRegistrationInfo *info)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;
    WCHAR *value;

    if (IXmlReader_IsEmptyElement(reader))
    {
        TRACE("RegistrationInfo is empty\n");
        return S_OK;
    }

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_EndElement:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("/%s\n", debugstr_w(name));

            if (!lstrcmpW(name, RegistrationInfo))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, Author))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Author(info, value);
            }
            else if (!lstrcmpW(name, Description))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Description(info, value);
            }
            else
                FIXME("unhandled RegistrationInfo element %s\n", debugstr_w(name));

            break;

        case XmlNodeType_Whitespace:
        case XmlNodeType_Comment:
            break;

        default:
            FIXME("unhandled RegistrationInfo node type %d\n", type);
            break;
        }
    }

    WARN("RegistrationInfo was not terminated\n");
    return SCHED_E_MALFORMEDXML;
}

static HRESULT read_task_attributes(IXmlReader *reader, ITaskDefinition *taskdef)
{
    HRESULT hr;
    ITaskSettings *taskset;
    const WCHAR *name;
    const WCHAR *value;
    BOOL xmlns_ok = FALSE;

    TRACE("\n");

    hr = ITaskDefinition_get_Settings(taskdef, &taskset);
    if (hr != S_OK) return hr;

    hr = IXmlReader_MoveToFirstAttribute(reader);

    while (hr == S_OK)
    {
        hr = IXmlReader_GetLocalName(reader, &name, NULL);
        if (hr != S_OK) break;

        hr = IXmlReader_GetValue(reader, &value, NULL);
        if (hr != S_OK) break;

        TRACE("%s=%s\n", debugstr_w(name), debugstr_w(value));

        if (!lstrcmpW(name, version))
        {
            TASK_COMPATIBILITY compatibility = TASK_COMPATIBILITY_V2;

            if (!lstrcmpW(value, v1_0))
                compatibility = TASK_COMPATIBILITY_AT;
            else if (!lstrcmpW(value, v1_1))
                compatibility = TASK_COMPATIBILITY_V1;
            else if (!lstrcmpW(value, v1_2))
                compatibility = TASK_COMPATIBILITY_V2;
            else if (!lstrcmpW(value, v1_3))
                compatibility = TASK_COMPATIBILITY_V2_1;
            else
                FIXME("unknown version %s\n", debugstr_w(value));

            ITaskSettings_put_Compatibility(taskset, compatibility);
        }
        else if (!lstrcmpW(name, xmlns))
        {
            if (lstrcmpW(value, task_ns))
            {
                FIXME("unknown namespace %s\n", debugstr_w(value));
                break;
            }
            xmlns_ok = TRUE;
        }
        else
            FIXME("unhandled Task attribute %s\n", debugstr_w(name));

        hr = IXmlReader_MoveToNextAttribute(reader);
    }

    ITaskSettings_Release(taskset);
    return xmlns_ok ? S_OK : SCHED_E_NAMESPACE;
}

static HRESULT read_task(IXmlReader *reader, ITaskDefinition *taskdef)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;

    if (IXmlReader_IsEmptyElement(reader))
    {
        TRACE("Task is empty\n");
        return S_OK;
    }

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_EndElement:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("/%s\n", debugstr_w(name));

            if (!lstrcmpW(name, Task))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, RegistrationInfo))
            {
                IRegistrationInfo *info;

                hr = ITaskDefinition_get_RegistrationInfo(taskdef, &info);
                if (hr != S_OK) return hr;
                hr = read_registration_info(reader, info);
                IRegistrationInfo_Release(info);
            }
            else if (!lstrcmpW(name, Settings))
            {
                ITaskSettings *taskset;

                hr = ITaskDefinition_get_Settings(taskdef, &taskset);
                if (hr != S_OK) return hr;
                hr = read_settings(reader, taskset);
                ITaskSettings_Release(taskset);
            }
            else if (!lstrcmpW(name, Triggers))
                hr = read_triggers(reader, taskdef);
            else if (!lstrcmpW(name, Principals))
                hr = read_principals(reader, taskdef);
            else if (!lstrcmpW(name, Actions))
                hr = read_actions(reader, taskdef);
            else
                FIXME("unhandled Task element %s\n", debugstr_w(name));

            if (hr != S_OK) return hr;
            break;

        case XmlNodeType_Comment:
        case XmlNodeType_Whitespace:
            break;

        default:
            FIXME("unhandled Task node type %d\n", type);
            break;
        }
    }

    WARN("Task was not terminated\n");
    return SCHED_E_MALFORMEDXML;
}

static HRESULT read_xml(IXmlReader *reader, ITaskDefinition *taskdef)
{
    HRESULT hr;
    XmlNodeType type;
    const WCHAR *name;

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_XmlDeclaration:
            TRACE("XmlDeclaration\n");
            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, Task))
            {
                hr = read_task_attributes(reader, taskdef);
                if (hr != S_OK) return hr;

                return read_task(reader, taskdef);
            }
            else
                FIXME("unhandled XML element %s\n", debugstr_w(name));

            break;

        case XmlNodeType_Comment:
        case XmlNodeType_Whitespace:
            break;

        default:
            FIXME("unhandled XML node type %d\n", type);
            break;
        }
    }

    WARN("Task definition was not found\n");
    return SCHED_E_MALFORMEDXML;
}

static HRESULT WINAPI TaskDefinition_put_XmlText(ITaskDefinition *iface, BSTR xml)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;
    IStream *stream;
    IXmlReader *reader;
    HGLOBAL hmem;
    void *buf;

    TRACE("%p,%s\n", iface, debugstr_w(xml));

    if (!xml) return E_INVALIDARG;

    hmem = GlobalAlloc(0, lstrlenW(xml) * sizeof(WCHAR));
    if (!hmem) return E_OUTOFMEMORY;

    buf = GlobalLock(hmem);
    memcpy(buf, xml, lstrlenW(xml) * sizeof(WCHAR));
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    if (hr != S_OK)
    {
        GlobalFree(hmem);
        return hr;
    }

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    if (hr != S_OK)
    {
        IStream_Release(stream);
        return hr;
    }

    hr = IXmlReader_SetInput(reader, (IUnknown *)stream);
    if (hr == S_OK)
    {
        if (taskdef->reginfo)
        {
            IRegistrationInfo_Release(taskdef->reginfo);
            taskdef->reginfo = NULL;
        }
        if (taskdef->taskset)
        {
            ITaskSettings_Release(taskdef->taskset);
            taskdef->taskset = NULL;
        }
        if (taskdef->triggers)
        {
            ITriggerCollection_Release(taskdef->triggers);
            taskdef->triggers = NULL;
        }
        if (taskdef->principal)
        {
            IPrincipal_Release(taskdef->principal);
            taskdef->principal = NULL;
        }
        if (taskdef->actions)
        {
            IActionCollection_Release(taskdef->actions);
            taskdef->actions = NULL;
        }

        hr = read_xml(reader, iface);
    }

    IXmlReader_Release(reader);
    IStream_Release(stream);

    return hr;
}

static const ITaskDefinitionVtbl TaskDefinition_vtbl =
{
    TaskDefinition_QueryInterface,
    TaskDefinition_AddRef,
    TaskDefinition_Release,
    TaskDefinition_GetTypeInfoCount,
    TaskDefinition_GetTypeInfo,
    TaskDefinition_GetIDsOfNames,
    TaskDefinition_Invoke,
    TaskDefinition_get_RegistrationInfo,
    TaskDefinition_put_RegistrationInfo,
    TaskDefinition_get_Triggers,
    TaskDefinition_put_Triggers,
    TaskDefinition_get_Settings,
    TaskDefinition_put_Settings,
    TaskDefinition_get_Data,
    TaskDefinition_put_Data,
    TaskDefinition_get_Principal,
    TaskDefinition_put_Principal,
    TaskDefinition_get_Actions,
    TaskDefinition_put_Actions,
    TaskDefinition_get_XmlText,
    TaskDefinition_put_XmlText
};

HRESULT TaskDefinition_create(ITaskDefinition **obj)
{
    TaskDefinition *taskdef;

    taskdef = heap_alloc_zero(sizeof(*taskdef));
    if (!taskdef) return E_OUTOFMEMORY;

    taskdef->ITaskDefinition_iface.lpVtbl = &TaskDefinition_vtbl;
    taskdef->ref = 1;
    *obj = &taskdef->ITaskDefinition_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    ITaskService ITaskService_iface;
    LONG ref;
    BOOL connected;
    DWORD version;
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
} TaskService;

static inline TaskService *impl_from_ITaskService(ITaskService *iface)
{
    return CONTAINING_RECORD(iface, TaskService, ITaskService_iface);
}

static ULONG WINAPI TaskService_AddRef(ITaskService *iface)
{
    TaskService *task_svc = impl_from_ITaskService(iface);
    return InterlockedIncrement(&task_svc->ref);
}

static ULONG WINAPI TaskService_Release(ITaskService *iface)
{
    TaskService *task_svc = impl_from_ITaskService(iface);
    LONG ref = InterlockedDecrement(&task_svc->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(task_svc);
    }

    return ref;
}

static HRESULT WINAPI TaskService_QueryInterface(ITaskService *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_ITaskService) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        ITaskService_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI TaskService_GetTypeInfoCount(ITaskService *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetTypeInfo(ITaskService *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%u,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetIDsOfNames(ITaskService *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_Invoke(ITaskService *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetFolder(ITaskService *iface, BSTR path, ITaskFolder **folder)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_w(path), folder);

    if (!folder) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    return TaskFolder_create(path, NULL, folder, FALSE);
}

static HRESULT WINAPI TaskService_GetRunningTasks(ITaskService *iface, LONG flags, IRunningTaskCollection **tasks)
{
    FIXME("%p,%x,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_NewTask(ITaskService *iface, DWORD flags, ITaskDefinition **definition)
{
    TRACE("%p,%x,%p\n", iface, flags, definition);

    if (!definition) return E_POINTER;

    if (flags)
        FIXME("unsupported flags %x\n", flags);

    return TaskDefinition_create(definition);
}

static inline BOOL is_variant_null(const VARIANT *var)
{
    return V_VT(var) == VT_EMPTY || V_VT(var) == VT_NULL ||
          (V_VT(var) == VT_BSTR && (V_BSTR(var) == NULL || !*V_BSTR(var)));
}

static HRESULT start_schedsvc(void)
{
    static const WCHAR scheduleW[] = { 'S','c','h','e','d','u','l','e',0 };
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    ULONGLONG start_time;
    HRESULT hr = SCHED_E_SERVICE_NOT_RUNNING;

    TRACE("Trying to start %s service\n", debugstr_w(scheduleW));

    scm = OpenSCManagerW(NULL, NULL, 0);
    if (!scm) return SCHED_E_SERVICE_NOT_INSTALLED;

    service = OpenServiceW(scm, scheduleW, SERVICE_START | SERVICE_QUERY_STATUS);
    if (service)
    {
        if (StartServiceW(service, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
        {
            start_time = GetTickCount64();
            do
            {
                DWORD dummy;

                if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(status), &dummy))
                {
                    WARN("failed to query scheduler status (%u)\n", GetLastError());
                    break;
                }

                if (status.dwCurrentState == SERVICE_RUNNING)
                {
                    hr = S_OK;
                    break;
                }

                if (GetTickCount64() - start_time > 30000) break;
                Sleep(1000);

            } while (status.dwCurrentState == SERVICE_START_PENDING);

            if (status.dwCurrentState != SERVICE_RUNNING)
                WARN("scheduler failed to start %u\n", status.dwCurrentState);
        }
        else
            WARN("failed to start scheduler service (%u)\n", GetLastError());

        CloseServiceHandle(service);
    }
    else
        WARN("failed to open scheduler service (%u)\n", GetLastError());

    CloseServiceHandle(scm);
    return hr;
}

static HRESULT WINAPI TaskService_Connect(ITaskService *iface, VARIANT server, VARIANT user, VARIANT domain, VARIANT password)
{
    static WCHAR ncalrpc[] = { 'n','c','a','l','r','p','c',0 };
    TaskService *task_svc = impl_from_ITaskService(iface);
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len;
    HRESULT hr;
    RPC_WSTR binding_str;
    extern handle_t rpc_handle;

    TRACE("%p,%s,%s,%s,%s\n", iface, debugstr_variant(&server), debugstr_variant(&user),
          debugstr_variant(&domain), debugstr_variant(&password));

    if (!is_variant_null(&user) || !is_variant_null(&domain) || !is_variant_null(&password))
        FIXME("user/domain/password are ignored\n");

    len = sizeof(comp_name)/sizeof(comp_name[0]);
    if (!GetComputerNameW(comp_name, &len))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!is_variant_null(&server))
    {
        const WCHAR *server_name;

        if (V_VT(&server) != VT_BSTR)
        {
            FIXME("server variant type %d is not supported\n", V_VT(&server));
            return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }

        /* skip UNC prefix if any */
        server_name = V_BSTR(&server);
        if (server_name[0] == '\\' && server_name[1] == '\\')
            server_name += 2;

        if (strcmpiW(server_name, comp_name))
        {
            FIXME("connection to remote server %s is not supported\n", debugstr_w(V_BSTR(&server)));
            return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }
    }

    hr = start_schedsvc();
    if (hr != S_OK) return hr;

    hr = RpcStringBindingComposeW(NULL, ncalrpc, NULL, NULL, NULL, &binding_str);
    if (hr != RPC_S_OK) return hr;
    hr = RpcBindingFromStringBindingW(binding_str, &rpc_handle);
    if (hr != RPC_S_OK) return hr;
    RpcStringFreeW(&binding_str);

    /* Make sure that the connection works */
    hr = SchRpcHighestVersion(&task_svc->version);
    if (hr != S_OK) return hr;

    TRACE("server version %#x\n", task_svc->version);

    strcpyW(task_svc->comp_name, comp_name);
    task_svc->connected = TRUE;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_Connected(ITaskService *iface, VARIANT_BOOL *connected)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, connected);

    if (!connected) return E_POINTER;

    *connected = task_svc->connected ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_TargetServer(ITaskService *iface, BSTR *server)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, server);

    if (!server) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    *server = SysAllocString(task_svc->comp_name);
    if (!*server) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_ConnectedUser(ITaskService *iface, BSTR *user)
{
    FIXME("%p,%p: stub\n", iface, user);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_get_ConnectedDomain(ITaskService *iface, BSTR *domain)
{
    FIXME("%p,%p: stub\n", iface, domain);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_get_HighestVersion(ITaskService *iface, DWORD *version)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, version);

    if (!version) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    *version = task_svc->version;

    return S_OK;
}

static const ITaskServiceVtbl TaskService_vtbl =
{
    TaskService_QueryInterface,
    TaskService_AddRef,
    TaskService_Release,
    TaskService_GetTypeInfoCount,
    TaskService_GetTypeInfo,
    TaskService_GetIDsOfNames,
    TaskService_Invoke,
    TaskService_GetFolder,
    TaskService_GetRunningTasks,
    TaskService_NewTask,
    TaskService_Connect,
    TaskService_get_Connected,
    TaskService_get_TargetServer,
    TaskService_get_ConnectedUser,
    TaskService_get_ConnectedDomain,
    TaskService_get_HighestVersion
};

HRESULT TaskService_create(void **obj)
{
    TaskService *task_svc;

    task_svc = heap_alloc(sizeof(*task_svc));
    if (!task_svc) return E_OUTOFMEMORY;

    task_svc->ITaskService_iface.lpVtbl = &TaskService_vtbl;
    task_svc->ref = 1;
    task_svc->connected = FALSE;
    *obj = &task_svc->ITaskService_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

void __RPC_FAR *__RPC_USER MIDL_user_allocate(SIZE_T n)
{
    return HeapAlloc(GetProcessHeap(), 0, n);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}
