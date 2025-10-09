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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct {
    IDailyTrigger IDailyTrigger_iface;
    LONG ref;
    short interval;
    WCHAR *start_boundary;
    WCHAR *end_boundary;
    BOOL enabled;
} DailyTrigger;

static inline DailyTrigger *impl_from_IDailyTrigger(IDailyTrigger *iface)
{
    return CONTAINING_RECORD(iface, DailyTrigger, IDailyTrigger_iface);
}

static HRESULT WINAPI DailyTrigger_QueryInterface(IDailyTrigger *iface, REFIID riid, void **ppv)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IDispatch, riid) ||
       IsEqualGUID(&IID_ITrigger, riid) ||
       IsEqualGUID(&IID_IDailyTrigger, riid))
    {
        *ppv = &This->IDailyTrigger_iface;
    }
    else
    {
        FIXME("unsupported riid %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DailyTrigger_AddRef(IDailyTrigger *iface)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI DailyTrigger_Release(IDailyTrigger *iface)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
    {
        TRACE("destroying %p\n", iface);
        free(This->start_boundary);
        free(This->end_boundary);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI DailyTrigger_GetTypeInfoCount(IDailyTrigger *iface, UINT *count)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_GetTypeInfo(IDailyTrigger *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_GetIDsOfNames(IDailyTrigger *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_Invoke(IDailyTrigger *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%ld %s %lx %x %p %p %p %p)\n", This, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_get_Type(IDailyTrigger *iface, TASK_TRIGGER_TYPE2 *type)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_get_Id(IDailyTrigger *iface, BSTR *id)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_put_Id(IDailyTrigger *iface, BSTR id)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(id));
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_get_Repetition(IDailyTrigger *iface, IRepetitionPattern **repeat)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, repeat);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_put_Repetition(IDailyTrigger *iface, IRepetitionPattern *repeat)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, repeat);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_get_ExecutionTimeLimit(IDailyTrigger *iface, BSTR *limit)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_put_ExecutionTimeLimit(IDailyTrigger *iface, BSTR limit)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(limit));
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_get_StartBoundary(IDailyTrigger *iface, BSTR *start)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%p)\n", This, start);

    if (!start) return E_POINTER;

    if (!This->start_boundary) *start = NULL;
    else if (!(*start = SysAllocString(This->start_boundary))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI DailyTrigger_put_StartBoundary(IDailyTrigger *iface, BSTR start)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    WCHAR *str = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(start));

    if (start && !(str = wcsdup(start))) return E_OUTOFMEMORY;
    free(This->start_boundary);
    This->start_boundary = str;

    return S_OK;
}

static HRESULT WINAPI DailyTrigger_get_EndBoundary(IDailyTrigger *iface, BSTR *end)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%p)\n", This, end);

    if (!end) return E_POINTER;

    if (!This->end_boundary) *end = NULL;
    else if (!(*end = SysAllocString(This->end_boundary))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI DailyTrigger_put_EndBoundary(IDailyTrigger *iface, BSTR end)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    WCHAR *str = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(end));

    if (end && !(str = wcsdup(end))) return E_OUTOFMEMORY;
    free(This->end_boundary);
    This->end_boundary = str;

    return S_OK;
}

static HRESULT WINAPI DailyTrigger_get_Enabled(IDailyTrigger *iface, VARIANT_BOOL *enabled)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%p)\n", This, enabled);

    if (!enabled) return E_POINTER;

    *enabled = This->enabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI DailyTrigger_put_Enabled(IDailyTrigger *iface, VARIANT_BOOL enabled)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%x)\n", This, enabled);

    This->enabled = !!enabled;
    return S_OK;
}

static HRESULT WINAPI DailyTrigger_get_DaysInterval(IDailyTrigger *iface, short *days)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%p)\n", This, days);

    *days = This->interval;
    return S_OK;
}

static HRESULT WINAPI DailyTrigger_put_DaysInterval(IDailyTrigger *iface, short days)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);

    TRACE("(%p)->(%d)\n", This, days);

    if(days <= 0)
        return E_INVALIDARG;

    This->interval = days;
    return S_OK;
}

static HRESULT WINAPI DailyTrigger_get_RandomDelay(IDailyTrigger *iface, BSTR *pRandomDelay)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%p)\n", This, pRandomDelay);
    return E_NOTIMPL;
}

static HRESULT WINAPI DailyTrigger_put_RandomDelay(IDailyTrigger *iface, BSTR randomDelay)
{
    DailyTrigger *This = impl_from_IDailyTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(randomDelay));
    return E_NOTIMPL;
}

static const IDailyTriggerVtbl DailyTrigger_vtbl = {
    DailyTrigger_QueryInterface,
    DailyTrigger_AddRef,
    DailyTrigger_Release,
    DailyTrigger_GetTypeInfoCount,
    DailyTrigger_GetTypeInfo,
    DailyTrigger_GetIDsOfNames,
    DailyTrigger_Invoke,
    DailyTrigger_get_Type,
    DailyTrigger_get_Id,
    DailyTrigger_put_Id,
    DailyTrigger_get_Repetition,
    DailyTrigger_put_Repetition,
    DailyTrigger_get_ExecutionTimeLimit,
    DailyTrigger_put_ExecutionTimeLimit,
    DailyTrigger_get_StartBoundary,
    DailyTrigger_put_StartBoundary,
    DailyTrigger_get_EndBoundary,
    DailyTrigger_put_EndBoundary,
    DailyTrigger_get_Enabled,
    DailyTrigger_put_Enabled,
    DailyTrigger_get_DaysInterval,
    DailyTrigger_put_DaysInterval,
    DailyTrigger_get_RandomDelay,
    DailyTrigger_put_RandomDelay
};

static HRESULT DailyTrigger_create(ITrigger **trigger)
{
    DailyTrigger *daily_trigger;

    daily_trigger = malloc(sizeof(*daily_trigger));
    if (!daily_trigger)
        return E_OUTOFMEMORY;

    daily_trigger->IDailyTrigger_iface.lpVtbl = &DailyTrigger_vtbl;
    daily_trigger->ref = 1;
    daily_trigger->interval = 1;
    daily_trigger->start_boundary = NULL;
    daily_trigger->end_boundary = NULL;
    daily_trigger->enabled = TRUE;

    *trigger = (ITrigger*)&daily_trigger->IDailyTrigger_iface;
    return S_OK;
}

typedef struct {
    IRegistrationTrigger IRegistrationTrigger_iface;
    BOOL enabled;
    LONG ref;
} RegistrationTrigger;

static inline RegistrationTrigger *impl_from_IRegistrationTrigger(IRegistrationTrigger *iface)
{
    return CONTAINING_RECORD(iface, RegistrationTrigger, IRegistrationTrigger_iface);
}

static HRESULT WINAPI RegistrationTrigger_QueryInterface(IRegistrationTrigger *iface, REFIID riid, void **ppv)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IDispatch, riid) ||
       IsEqualGUID(&IID_ITrigger, riid) ||
       IsEqualGUID(&IID_IRegistrationTrigger, riid))
    {
        *ppv = &This->IRegistrationTrigger_iface;
    }
    else
    {
        FIXME("unsupported riid %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI RegistrationTrigger_AddRef(IRegistrationTrigger *iface)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI RegistrationTrigger_Release(IRegistrationTrigger *iface)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
    {
        TRACE("destroying %p\n", iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI RegistrationTrigger_GetTypeInfoCount(IRegistrationTrigger *iface, UINT *count)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_GetTypeInfo(IRegistrationTrigger *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_GetIDsOfNames(IRegistrationTrigger *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_Invoke(IRegistrationTrigger *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%ld %s %lx %x %p %p %p %p)\n", This, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_Type(IRegistrationTrigger *iface, TASK_TRIGGER_TYPE2 *type)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_Id(IRegistrationTrigger *iface, BSTR *id)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_Id(IRegistrationTrigger *iface, BSTR id)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(id));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_Repetition(IRegistrationTrigger *iface, IRepetitionPattern **repeat)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, repeat);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_Repetition(IRegistrationTrigger *iface, IRepetitionPattern *repeat)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, repeat);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_ExecutionTimeLimit(IRegistrationTrigger *iface, BSTR *limit)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_ExecutionTimeLimit(IRegistrationTrigger *iface, BSTR limit)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(limit));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_StartBoundary(IRegistrationTrigger *iface, BSTR *start)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, start);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_StartBoundary(IRegistrationTrigger *iface, BSTR start)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(start));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_EndBoundary(IRegistrationTrigger *iface, BSTR *end)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, end);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_EndBoundary(IRegistrationTrigger *iface, BSTR end)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(end));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_get_Enabled(IRegistrationTrigger *iface, VARIANT_BOOL *enabled)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);

    TRACE("(%p)->(%p)\n", This, enabled);

    if (!enabled) return E_POINTER;

    *enabled = This->enabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI RegistrationTrigger_put_Enabled(IRegistrationTrigger *iface, VARIANT_BOOL enabled)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);

    TRACE("(%p)->(%x)\n", This, enabled);

    This->enabled = !!enabled;
    return S_OK;
}

static HRESULT WINAPI RegistrationTrigger_get_Delay(IRegistrationTrigger *iface, BSTR *pDelay)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%p)\n", This, pDelay);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationTrigger_put_Delay(IRegistrationTrigger *iface, BSTR delay)
{
    RegistrationTrigger *This = impl_from_IRegistrationTrigger(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(delay));
    return E_NOTIMPL;
}

static const IRegistrationTriggerVtbl RegistrationTrigger_vtbl = {
    RegistrationTrigger_QueryInterface,
    RegistrationTrigger_AddRef,
    RegistrationTrigger_Release,
    RegistrationTrigger_GetTypeInfoCount,
    RegistrationTrigger_GetTypeInfo,
    RegistrationTrigger_GetIDsOfNames,
    RegistrationTrigger_Invoke,
    RegistrationTrigger_get_Type,
    RegistrationTrigger_get_Id,
    RegistrationTrigger_put_Id,
    RegistrationTrigger_get_Repetition,
    RegistrationTrigger_put_Repetition,
    RegistrationTrigger_get_ExecutionTimeLimit,
    RegistrationTrigger_put_ExecutionTimeLimit,
    RegistrationTrigger_get_StartBoundary,
    RegistrationTrigger_put_StartBoundary,
    RegistrationTrigger_get_EndBoundary,
    RegistrationTrigger_put_EndBoundary,
    RegistrationTrigger_get_Enabled,
    RegistrationTrigger_put_Enabled,
    RegistrationTrigger_get_Delay,
    RegistrationTrigger_put_Delay
};

static HRESULT RegistrationTrigger_create(ITrigger **trigger)
{
    RegistrationTrigger *registration_trigger;

    registration_trigger = malloc(sizeof(*registration_trigger));
    if (!registration_trigger)
        return E_OUTOFMEMORY;

    registration_trigger->IRegistrationTrigger_iface.lpVtbl = &RegistrationTrigger_vtbl;
    registration_trigger->ref = 1;
    registration_trigger->enabled = TRUE;

    *trigger = (ITrigger*)&registration_trigger->IRegistrationTrigger_iface;
    return S_OK;
}

typedef struct
{
    ITriggerCollection ITriggerCollection_iface;
    LONG ref;
} trigger_collection;

static inline trigger_collection *impl_from_ITriggerCollection(ITriggerCollection *iface)
{
    return CONTAINING_RECORD(iface, trigger_collection, ITriggerCollection_iface);
}

static HRESULT WINAPI TriggerCollection_QueryInterface(ITriggerCollection *iface, REFIID riid, void **ppv)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IDispatch, riid) ||
       IsEqualGUID(&IID_ITriggerCollection, riid)) {
        *ppv = &This->ITriggerCollection_iface;
    }else {
        FIXME("unimplemented interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TriggerCollection_AddRef(ITriggerCollection *iface)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI TriggerCollection_Release(ITriggerCollection *iface)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI TriggerCollection_GetTypeInfoCount(ITriggerCollection *iface, UINT *count)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%p)\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_GetTypeInfo(ITriggerCollection *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_GetIDsOfNames(ITriggerCollection *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_Invoke(ITriggerCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                               DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%ld %s %lx %x %p %p %p %p)\n", This, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_get_Count(ITriggerCollection *iface, LONG *count)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%p)\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_get_Item(ITriggerCollection *iface, LONG index, ITrigger **trigger)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, trigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_get__NewEnum(ITriggerCollection *iface, IUnknown **penum)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%p)\n", This, penum);
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_Create(ITriggerCollection *iface, TASK_TRIGGER_TYPE2 type, ITrigger **trigger)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);

    TRACE("(%p)->(%d %p)\n", This, type, trigger);

    switch(type) {
    case TASK_TRIGGER_DAILY:
        return DailyTrigger_create(trigger);
    case TASK_TRIGGER_REGISTRATION:
        return RegistrationTrigger_create(trigger);
    default:
        FIXME("Unimplemented type %d\n", type);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI TriggerCollection_Remove(ITriggerCollection *iface, VARIANT index)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&index));
    return E_NOTIMPL;
}

static HRESULT WINAPI TriggerCollection_Clear(ITriggerCollection *iface)
{
    trigger_collection *This = impl_from_ITriggerCollection(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const ITriggerCollectionVtbl TriggerCollection_vtbl = {
    TriggerCollection_QueryInterface,
    TriggerCollection_AddRef,
    TriggerCollection_Release,
    TriggerCollection_GetTypeInfoCount,
    TriggerCollection_GetTypeInfo,
    TriggerCollection_GetIDsOfNames,
    TriggerCollection_Invoke,
    TriggerCollection_get_Count,
    TriggerCollection_get_Item,
    TriggerCollection_get__NewEnum,
    TriggerCollection_Create,
    TriggerCollection_Remove,
    TriggerCollection_Clear
};

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
        free(reginfo->description);
        free(reginfo->author);
        free(reginfo->version);
        free(reginfo->date);
        free(reginfo->documentation);
        free(reginfo->uri);
        free(reginfo->source);
        free(reginfo);
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
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_GetIDsOfNames(IRegistrationInfo *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_Invoke(IRegistrationInfo *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                            DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_get_Description(IRegistrationInfo *iface, BSTR *description)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, description);

    if (!description) return E_POINTER;

    if (!reginfo->description) *description = NULL;
    else if (!(*description = SysAllocString(reginfo->description))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Description(IRegistrationInfo *iface, BSTR description)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(description));

    if (description && !(str = wcsdup(description))) return E_OUTOFMEMORY;
    free(reginfo->description);
    reginfo->description = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_Author(IRegistrationInfo *iface, BSTR *author)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, author);

    if (!author) return E_POINTER;

    if (!reginfo->author) *author = NULL;
    else if (!(*author = SysAllocString(reginfo->author))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Author(IRegistrationInfo *iface, BSTR author)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(author));

    if (author && !(str = wcsdup(author))) return E_OUTOFMEMORY;
    free(reginfo->author);
    reginfo->author = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_Version(IRegistrationInfo *iface, BSTR *version)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, version);

    if (!version) return E_POINTER;

    if (!reginfo->version) *version = NULL;
    else if (!(*version = SysAllocString(reginfo->version))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Version(IRegistrationInfo *iface, BSTR version)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(version));

    if (version && !(str = wcsdup(version))) return E_OUTOFMEMORY;
    free(reginfo->version);
    reginfo->version = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_Date(IRegistrationInfo *iface, BSTR *date)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, date);

    if (!date) return E_POINTER;

    if (!reginfo->date) *date = NULL;
    else if (!(*date = SysAllocString(reginfo->date))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Date(IRegistrationInfo *iface, BSTR date)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(date));

    if (date && !(str = wcsdup(date))) return E_OUTOFMEMORY;
    free(reginfo->date);
    reginfo->date = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_Documentation(IRegistrationInfo *iface, BSTR *doc)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, doc);

    if (!doc) return E_POINTER;

    if (!reginfo->documentation) *doc = NULL;
    else if (!(*doc = SysAllocString(reginfo->documentation))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Documentation(IRegistrationInfo *iface, BSTR doc)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(doc));

    if (doc && !(str = wcsdup(doc))) return E_OUTOFMEMORY;
    free(reginfo->documentation);
    reginfo->documentation = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_XmlText(IRegistrationInfo *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_put_XmlText(IRegistrationInfo *iface, BSTR xml)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(xml));
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_get_URI(IRegistrationInfo *iface, BSTR *uri)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, uri);

    if (!uri) return E_POINTER;

    if (!reginfo->uri) *uri = NULL;
    else if (!(*uri = SysAllocString(reginfo->uri))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_URI(IRegistrationInfo *iface, BSTR uri)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(uri));

    if (uri && !(str = wcsdup(uri))) return E_OUTOFMEMORY;
    free(reginfo->uri);
    reginfo->uri = str;
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_SecurityDescriptor(IRegistrationInfo *iface, VARIANT *sddl)
{
    FIXME("%p,%p: stub\n", iface, sddl);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegistrationInfo_put_SecurityDescriptor(IRegistrationInfo *iface, VARIANT sddl)
{
    FIXME("%p,%s: stub\n", iface, debugstr_variant(&sddl));
    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_get_Source(IRegistrationInfo *iface, BSTR *source)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);

    TRACE("%p,%p\n", iface, source);

    if (!source) return E_POINTER;

    if (!reginfo->source) *source = NULL;
    else if (!(*source = SysAllocString(reginfo->source))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI RegistrationInfo_put_Source(IRegistrationInfo *iface, BSTR source)
{
    registration_info *reginfo = impl_from_IRegistrationInfo(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(source));

    if (source && !(str = wcsdup(source))) return E_OUTOFMEMORY;
    free(reginfo->source);
    reginfo->source = str;
    return S_OK;
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

    reginfo = calloc(1, sizeof(*reginfo));
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
        free(taskset->restart_interval);
        free(taskset->execution_time_limit);
        free(taskset->delete_expired_task_after);
        free(taskset);
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
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_GetIDsOfNames(ITaskSettings *iface, REFIID riid, LPOLESTR *names,
                                                 UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskSettings_Invoke(ITaskSettings *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                          DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, allow);

    taskset->allow_on_demand_start = !!allow;

    return S_OK;
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

    if (!taskset->restart_interval) *interval = NULL;
    else if (!(*interval = SysAllocString(taskset->restart_interval))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_RestartInterval(ITaskSettings *iface, BSTR interval)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(interval));

    if (interval && !(str = wcsdup(interval))) return E_OUTOFMEMORY;
    free(taskset->restart_interval);
    taskset->restart_interval = str;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, count);

    taskset->restart_count = count;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, policy);

    taskset->policy = policy;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, stop);

    taskset->stop_if_going_on_batteries = !!stop;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, disallow);

    taskset->disallow_start_if_on_batteries = !!disallow;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, allow);

    taskset->allow_hard_terminate = !!allow;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, start);

    taskset->start_when_available = !!start;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, run);

    taskset->run_only_if_network_available = !!run;

    return S_OK;
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

    if (!taskset->execution_time_limit) *limit = NULL;
    else if (!(*limit = SysAllocString(taskset->execution_time_limit))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_ExecutionTimeLimit(ITaskSettings *iface, BSTR limit)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(limit));

    if (limit && !(str = wcsdup(limit))) return E_OUTOFMEMORY;
    free(taskset->execution_time_limit);
    taskset->execution_time_limit = str;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, enabled);

    taskset->enabled = !!enabled;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_get_DeleteExpiredTaskAfter(ITaskSettings *iface, BSTR *delay)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%p\n", iface, delay);

    if (!delay) return E_POINTER;

    if (!taskset->delete_expired_task_after) *delay = NULL;
    else if (!(*delay = SysAllocString(taskset->delete_expired_task_after))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskSettings_put_DeleteExpiredTaskAfter(ITaskSettings *iface, BSTR delay)
{
    TaskSettings *taskset = impl_from_ITaskSettings(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(delay));

    if (delay && !(str = wcsdup(delay))) return E_OUTOFMEMORY;
    free(taskset->delete_expired_task_after);
    taskset->delete_expired_task_after = str;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, priority);

    taskset->priority = priority;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, level);

    taskset->compatibility = level;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, hidden);

    taskset->hidden = !!hidden;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, run);

    taskset->run_only_if_idle = !!run;

    return S_OK;
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
    TaskSettings *taskset = impl_from_ITaskSettings(iface);

    TRACE("%p,%d\n", iface, wake);

    taskset->wake_to_run = !!wake;

    return S_OK;
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
    TaskSettings *taskset;

    taskset = malloc(sizeof(*taskset));
    if (!taskset) return E_OUTOFMEMORY;

    taskset->ITaskSettings_iface.lpVtbl = &TaskSettings_vtbl;
    taskset->ref = 1;
    /* set the defaults */
    taskset->restart_interval = NULL;
    taskset->execution_time_limit = wcsdup(L"PT72H");
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
    IPrincipal IPrincipal_iface;
    LONG ref;
} Principal;

static inline Principal *impl_from_IPrincipal(IPrincipal *iface)
{
    return CONTAINING_RECORD(iface, Principal, IPrincipal_iface);
}

static ULONG WINAPI Principal_AddRef(IPrincipal *iface)
{
    Principal *principal = impl_from_IPrincipal(iface);
    return InterlockedIncrement(&principal->ref);
}

static ULONG WINAPI Principal_Release(IPrincipal *iface)
{
    Principal *principal = impl_from_IPrincipal(iface);
    LONG ref = InterlockedDecrement(&principal->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free(principal);
    }

    return ref;
}

static HRESULT WINAPI Principal_QueryInterface(IPrincipal *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IPrincipal) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IPrincipal_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI Principal_GetTypeInfoCount(IPrincipal *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_GetTypeInfo(IPrincipal *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_GetIDsOfNames(IPrincipal *iface, REFIID riid, LPOLESTR *names,
                                              UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_Invoke(IPrincipal *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                       DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_get_Id(IPrincipal *iface, BSTR *id)
{
    FIXME("%p,%p: stub\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_Id(IPrincipal *iface, BSTR id)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(id));
    return S_OK;
}

static HRESULT WINAPI Principal_get_DisplayName(IPrincipal *iface, BSTR *name)
{
    FIXME("%p,%p: stub\n", iface, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_DisplayName(IPrincipal *iface, BSTR name)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_get_UserId(IPrincipal *iface, BSTR *user_id)
{
    FIXME("%p,%p: stub\n", iface, user_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_UserId(IPrincipal *iface, BSTR user_id)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(user_id));
    return S_OK;
}

static HRESULT WINAPI Principal_get_LogonType(IPrincipal *iface, TASK_LOGON_TYPE *logon_type)
{
    FIXME("%p,%p: stub\n", iface, logon_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_LogonType(IPrincipal *iface, TASK_LOGON_TYPE logon_type)
{
    FIXME("%p,%u: stub\n", iface, logon_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_get_GroupId(IPrincipal *iface, BSTR *group_id)
{
    FIXME("%p,%p: stub\n", iface, group_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_GroupId(IPrincipal *iface, BSTR group_id)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(group_id));
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_get_RunLevel(IPrincipal *iface, TASK_RUNLEVEL_TYPE *run_level)
{
    FIXME("%p,%p: stub\n", iface, run_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI Principal_put_RunLevel(IPrincipal *iface, TASK_RUNLEVEL_TYPE run_level)
{
    FIXME("%p,%u: stub\n", iface, run_level);
    return S_OK;
}

static const IPrincipalVtbl Principal_vtbl =
{
    Principal_QueryInterface,
    Principal_AddRef,
    Principal_Release,
    Principal_GetTypeInfoCount,
    Principal_GetTypeInfo,
    Principal_GetIDsOfNames,
    Principal_Invoke,
    Principal_get_Id,
    Principal_put_Id,
    Principal_get_DisplayName,
    Principal_put_DisplayName,
    Principal_get_UserId,
    Principal_put_UserId,
    Principal_get_LogonType,
    Principal_put_LogonType,
    Principal_get_GroupId,
    Principal_put_GroupId,
    Principal_get_RunLevel,
    Principal_put_RunLevel
};

static HRESULT Principal_create(IPrincipal **obj)
{
    Principal *principal;

    principal = malloc(sizeof(*principal));
    if (!principal) return E_OUTOFMEMORY;

    principal->IPrincipal_iface.lpVtbl = &Principal_vtbl;
    principal->ref = 1;

    *obj = &principal->IPrincipal_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    IExecAction IExecAction_iface;
    LONG ref;
    WCHAR *path;
    WCHAR *directory;
    WCHAR *args;
    WCHAR *id;
} ExecAction;

static inline ExecAction *impl_from_IExecAction(IExecAction *iface)
{
    return CONTAINING_RECORD(iface, ExecAction, IExecAction_iface);
}

static ULONG WINAPI ExecAction_AddRef(IExecAction *iface)
{
    ExecAction *action = impl_from_IExecAction(iface);
    return InterlockedIncrement(&action->ref);
}

static ULONG WINAPI ExecAction_Release(IExecAction *iface)
{
    ExecAction *action = impl_from_IExecAction(iface);
    LONG ref = InterlockedDecrement(&action->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free(action->path);
        free(action->directory);
        free(action->args);
        free(action->id);
        free(action);
    }

    return ref;
}

static HRESULT WINAPI ExecAction_QueryInterface(IExecAction *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IExecAction) ||
        IsEqualGUID(riid, &IID_IAction) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IExecAction_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI ExecAction_GetTypeInfoCount(IExecAction *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI ExecAction_GetTypeInfo(IExecAction *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI ExecAction_GetIDsOfNames(IExecAction *iface, REFIID riid, LPOLESTR *names,
                                               UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ExecAction_Invoke(IExecAction *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                        DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI ExecAction_get_Id(IExecAction *iface, BSTR *id)
{
    ExecAction *action = impl_from_IExecAction(iface);

    TRACE("%p,%p\n", iface, id);

    if (!id) return E_POINTER;

    if (!action->id) *id = NULL;
    else if (!(*id = SysAllocString(action->id))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI ExecAction_put_Id(IExecAction *iface, BSTR id)
{
    ExecAction *action = impl_from_IExecAction(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(id));

    if (id && !(str = wcsdup((id)))) return E_OUTOFMEMORY;
    free(action->id);
    action->id = str;

    return S_OK;
}

static HRESULT WINAPI ExecAction_get_Type(IExecAction *iface, TASK_ACTION_TYPE *type)
{
    TRACE("%p,%p\n", iface, type);

    if (!type) return E_POINTER;

    *type = TASK_ACTION_EXEC;

    return S_OK;
}

static HRESULT WINAPI ExecAction_get_Path(IExecAction *iface, BSTR *path)
{
    ExecAction *action = impl_from_IExecAction(iface);

    TRACE("%p,%p\n", iface, path);

    if (!path) return E_POINTER;

    if (!action->path) *path = NULL;
    else if (!(*path = SysAllocString(action->path))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI ExecAction_put_Path(IExecAction *iface, BSTR path)
{
    ExecAction *action = impl_from_IExecAction(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(path));

    if (path && !(str = wcsdup((path)))) return E_OUTOFMEMORY;
    free(action->path);
    action->path = str;

    return S_OK;
}

static HRESULT WINAPI ExecAction_get_Arguments(IExecAction *iface, BSTR *arguments)
{
    ExecAction *action = impl_from_IExecAction(iface);

    TRACE("%p,%p\n", iface, arguments);

    if (!arguments) return E_POINTER;

    if (!action->args) *arguments = NULL;
    else if (!(*arguments = SysAllocString(action->args))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI ExecAction_put_Arguments(IExecAction *iface, BSTR arguments)
{
    ExecAction *action = impl_from_IExecAction(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(arguments));

    if (arguments && !(str = wcsdup((arguments)))) return E_OUTOFMEMORY;
    free(action->args);
    action->args = str;

    return S_OK;
}

static HRESULT WINAPI ExecAction_get_WorkingDirectory(IExecAction *iface, BSTR *directory)
{
    ExecAction *action = impl_from_IExecAction(iface);

    TRACE("%p,%p\n", iface, directory);

    if (!directory) return E_POINTER;

    if (!action->directory) *directory = NULL;
    else if (!(*directory = SysAllocString(action->directory))) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI ExecAction_put_WorkingDirectory(IExecAction *iface, BSTR directory)
{
    ExecAction *action = impl_from_IExecAction(iface);
    WCHAR *str = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(directory));

    if (directory && !(str = wcsdup((directory)))) return E_OUTOFMEMORY;
    free(action->directory);
    action->directory = str;

    return S_OK;
}

static const IExecActionVtbl Action_vtbl =
{
    ExecAction_QueryInterface,
    ExecAction_AddRef,
    ExecAction_Release,
    ExecAction_GetTypeInfoCount,
    ExecAction_GetTypeInfo,
    ExecAction_GetIDsOfNames,
    ExecAction_Invoke,
    ExecAction_get_Id,
    ExecAction_put_Id,
    ExecAction_get_Type,
    ExecAction_get_Path,
    ExecAction_put_Path,
    ExecAction_get_Arguments,
    ExecAction_put_Arguments,
    ExecAction_get_WorkingDirectory,
    ExecAction_put_WorkingDirectory
};

static HRESULT ExecAction_create(IExecAction **obj)
{
    ExecAction *action;

    action = malloc(sizeof(*action));
    if (!action) return E_OUTOFMEMORY;

    action->IExecAction_iface.lpVtbl = &Action_vtbl;
    action->ref = 1;
    action->path = NULL;
    action->directory = NULL;
    action->args = NULL;
    action->id = NULL;

    *obj = &action->IExecAction_iface;

    TRACE("created %p\n", *obj);

    return S_OK;
}

typedef struct
{
    IActionCollection IActionCollection_iface;
    LONG ref;
} Actions;

static inline Actions *impl_from_IActionCollection(IActionCollection *iface)
{
    return CONTAINING_RECORD(iface, Actions, IActionCollection_iface);
}

static ULONG WINAPI Actions_AddRef(IActionCollection *iface)
{
    Actions *actions = impl_from_IActionCollection(iface);
    return InterlockedIncrement(&actions->ref);
}

static ULONG WINAPI Actions_Release(IActionCollection *iface)
{
    Actions *actions = impl_from_IActionCollection(iface);
    LONG ref = InterlockedDecrement(&actions->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free(actions);
    }

    return ref;
}

static HRESULT WINAPI Actions_QueryInterface(IActionCollection *iface, REFIID riid, void **obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IActionCollection) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IActionCollection_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI Actions_GetTypeInfoCount(IActionCollection *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_GetTypeInfo(IActionCollection *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_GetIDsOfNames(IActionCollection *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_Invoke(IActionCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_get_Count(IActionCollection *iface, LONG *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_get_Item(IActionCollection *iface, LONG index, IAction **action)
{
    FIXME("%p,%ld,%p: stub\n", iface, index, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_get__NewEnum(IActionCollection *iface, IUnknown **penum)
{
    FIXME("%p,%p: stub\n", iface, penum);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_get_XmlText(IActionCollection *iface, BSTR *xml)
{
    FIXME("%p,%p: stub\n", iface, xml);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_put_XmlText(IActionCollection *iface, BSTR xml)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(xml));
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_Create(IActionCollection *iface, TASK_ACTION_TYPE type, IAction **action)
{
    TRACE("%p,%u,%p\n", iface, type, action);

    switch (type)
    {
    case TASK_ACTION_EXEC:
        return ExecAction_create((IExecAction **)action);

    default:
        FIXME("unimplemented type %u\n", type);
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI Actions_Remove(IActionCollection *iface, VARIANT index)
{
    FIXME("%p,%s: stub\n", iface, debugstr_variant(&index));
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_Clear(IActionCollection *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_get_Context(IActionCollection *iface, BSTR *ctx)
{
    FIXME("%p,%p: stub\n", iface, ctx);
    return E_NOTIMPL;
}

static HRESULT WINAPI Actions_put_Context(IActionCollection *iface, BSTR ctx)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(ctx));
    return S_OK;
}

static const IActionCollectionVtbl Actions_vtbl =
{
    Actions_QueryInterface,
    Actions_AddRef,
    Actions_Release,
    Actions_GetTypeInfoCount,
    Actions_GetTypeInfo,
    Actions_GetIDsOfNames,
    Actions_Invoke,
    Actions_get_Count,
    Actions_get_Item,
    Actions_get__NewEnum,
    Actions_get_XmlText,
    Actions_put_XmlText,
    Actions_Create,
    Actions_Remove,
    Actions_Clear,
    Actions_get_Context,
    Actions_put_Context
};

static HRESULT Actions_create(IActionCollection **obj)
{
    Actions *actions;

    actions = malloc(sizeof(*actions));
    if (!actions) return E_OUTOFMEMORY;

    actions->IActionCollection_iface.lpVtbl = &Actions_vtbl;
    actions->ref = 1;

    *obj = &actions->IActionCollection_iface;

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
    BSTR data;
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
        if (taskdef->data)
            SysFreeString(taskdef->data);

        free(taskdef);
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
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_GetIDsOfNames(ITaskDefinition *iface, REFIID riid, LPOLESTR *names,
                                                   UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskDefinition_Invoke(ITaskDefinition *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                            DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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
    TaskDefinition *This = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", This, triggers);

    if (!This->triggers)
    {
        trigger_collection *collection;

        collection = malloc(sizeof(*collection));
        if (!collection) return E_OUTOFMEMORY;

        collection->ITriggerCollection_iface.lpVtbl = &TriggerCollection_vtbl;
        collection->ref = 1;
        This->triggers = &collection->ITriggerCollection_iface;
    }

    ITriggerCollection_AddRef(*triggers = This->triggers);
    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_Triggers(ITaskDefinition *iface, ITriggerCollection *triggers)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, triggers);

    if (!triggers) return E_POINTER;

    if (taskdef->triggers)
        ITriggerCollection_Release(taskdef->triggers);

    ITriggerCollection_AddRef(triggers);
    taskdef->triggers = triggers;

    return S_OK;
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
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, data);

    if (!data) return E_POINTER;

    if (!taskdef->data)
        *data = NULL;
    else
    {
        *data = SysAllocString(taskdef->data);
        if (!*data) return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_Data(ITaskDefinition *iface, BSTR data)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    BSTR copy = NULL;

    TRACE("%p,%s\n", iface, debugstr_w(data));

    if (data)
    {
        copy = SysAllocString(data);
        if (!copy) return E_OUTOFMEMORY;
    }

    if (taskdef->data)
        SysFreeString(taskdef->data);

    taskdef->data = copy;
    return S_OK;
}

static HRESULT WINAPI TaskDefinition_get_Principal(ITaskDefinition *iface, IPrincipal **principal)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, principal);

    if (!principal) return E_POINTER;

    if (!taskdef->principal)
    {
        hr = Principal_create(&taskdef->principal);
        if (hr != S_OK) return hr;
    }

    IPrincipal_AddRef(taskdef->principal);
    *principal = taskdef->principal;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_Principal(ITaskDefinition *iface, IPrincipal *principal)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, principal);

    if (!principal) return E_POINTER;

    if (taskdef->principal)
        IPrincipal_Release(taskdef->principal);

    IPrincipal_AddRef(principal);
    taskdef->principal = principal;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_get_Actions(ITaskDefinition *iface, IActionCollection **actions)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, actions);

    if (!actions) return E_POINTER;

    if (!taskdef->actions)
    {
        hr = Actions_create(&taskdef->actions);
        if (hr != S_OK) return hr;
    }

    IActionCollection_AddRef(taskdef->actions);
    *actions = taskdef->actions;

    return S_OK;
}

static HRESULT WINAPI TaskDefinition_put_Actions(ITaskDefinition *iface, IActionCollection *actions)
{
    TaskDefinition *taskdef = impl_from_ITaskDefinition(iface);

    TRACE("%p,%p\n", iface, actions);

    if (!actions) return E_POINTER;

    if (taskdef->actions)
        IActionCollection_Release(taskdef->actions);

    IActionCollection_AddRef(actions);
    taskdef->actions = actions;

    return S_OK;
}

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
    int i;
    for (i = 0; i < xml_indent; i += 2)
        write_stringW(stream, L"  ");
}

static inline HRESULT write_empty_element(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, L"<");
    write_stringW(stream, name);
    return write_stringW(stream, L"/>\n");
}

static inline HRESULT write_element(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, L"<");
    write_stringW(stream, name);
    return write_stringW(stream, L">\n");
}

static inline HRESULT write_element_end(IStream *stream, const WCHAR *name)
{
    write_indent(stream);
    write_stringW(stream, L"</");
    write_stringW(stream, name);
    return write_stringW(stream, L">\n");
}

static inline HRESULT write_text_value(IStream *stream, const WCHAR *name, const WCHAR *value)
{
    write_indent(stream);
    write_stringW(stream, L"<");
    write_stringW(stream, name);
    write_stringW(stream, L">");
    write_stringW(stream, value);
    write_stringW(stream, L"</");
    write_stringW(stream, name);
    return write_stringW(stream, L">\n");
}

static HRESULT write_bool_value(IStream *stream, const WCHAR *name, VARIANT_BOOL value)
{
    return write_text_value(stream, name, value ? L"true" : L"false");
}

static HRESULT write_int_value(IStream *stream, const WCHAR *name, int val)
{
    WCHAR s[32];

    swprintf(s, ARRAY_SIZE(s), L"%d", val);
    return write_text_value(stream, name, s);
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
        compatibility = L"1.0";
        break;
    case TASK_COMPATIBILITY_V1:
        compatibility = L"1.1";
        break;
    case TASK_COMPATIBILITY_V2:
        compatibility = L"1.2";
        break;
    default:
        compatibility = L"1.3";
        break;
    }

    write_stringW(stream, L"<Task version=\"");
    write_stringW(stream, compatibility);
    write_stringW(stream, L"\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">");
    return write_stringW(stream, L"\n");
}

static HRESULT write_registration_info(IStream *stream, IRegistrationInfo *reginfo)
{
    HRESULT hr;
    BSTR bstr;
    VARIANT var;

    if (!reginfo)
        return write_empty_element(stream, L"RegistrationInfo");

    hr = write_element(stream, L"RegistrationInfo");
    if (hr != S_OK) return hr;

    push_indent();

    hr = IRegistrationInfo_get_Source(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Source", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Date(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Date", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Author(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Author", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Version(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Version", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Description(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Description", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_Documentation(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"Documentation", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_URI(reginfo, &bstr);
    if (hr == S_OK && bstr)
    {
        hr = write_text_value(stream, L"URI", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IRegistrationInfo_get_SecurityDescriptor(reginfo, &var);
    if (hr == S_OK)
    {
        if (V_VT(&var) == VT_BSTR)
        {
            hr = write_text_value(stream, L"SecurityDescriptor", V_BSTR(&var));
            VariantClear(&var);
            if (hr != S_OK) return hr;
        }
        else
            FIXME("SecurityInfo variant type %d is not supported\n", V_VT(&var));
    }

    pop_indent();

    return write_element_end(stream, L"RegistrationInfo");
}

static HRESULT write_principal(IStream *stream, IPrincipal *principal)
{
    HRESULT hr;
    BSTR bstr;
    TASK_LOGON_TYPE logon;
    TASK_RUNLEVEL_TYPE level;

    if (!principal)
        return write_empty_element(stream, L"Principals");

    hr = write_element(stream, L"Principals");
    if (hr != S_OK) return hr;

    push_indent();

    hr = IPrincipal_get_Id(principal, &bstr);
    if (hr == S_OK)
    {
        write_indent(stream);
        write_stringW(stream, L"<Principal id=\"");
        write_stringW(stream, bstr);
        write_stringW(stream, L"\">\n");
        SysFreeString(bstr);
    }
    else
        write_element(stream, L"Principal");

    push_indent();

    hr = IPrincipal_get_GroupId(principal, &bstr);
    if (hr == S_OK)
    {
        hr = write_text_value(stream, L"GroupId", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IPrincipal_get_DisplayName(principal, &bstr);
    if (hr == S_OK)
    {
        hr = write_text_value(stream, L"DisplayName", bstr);
        SysFreeString(bstr);
        if (hr != S_OK) return hr;
    }
    hr = IPrincipal_get_UserId(principal, &bstr);
    if (hr == S_OK && lstrlenW(bstr))
    {
        hr = write_text_value(stream, L"UserId", bstr);
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
            level_str = L"HighestAvailable";
            break;
        case TASK_RUNLEVEL_LUA:
            level_str = L"LeastPrivilege";
            break;
        default:
            FIXME("Principal run level %d\n", level);
            break;
        }

        if (level_str)
        {
            hr = write_text_value(stream, L"RunLevel", level_str);
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
            logon_str = L"Password";
            break;
        case TASK_LOGON_S4U:
            logon_str = L"S4U";
            break;
        case TASK_LOGON_INTERACTIVE_TOKEN:
            logon_str = L"InteractiveToken";
            break;
        default:
            FIXME("Principal logon type %d\n", logon);
            break;
        }

        if (logon_str)
        {
            hr = write_text_value(stream, L"LogonType", logon_str);
            if (hr != S_OK) return hr;
        }
    }

    pop_indent();
    write_element_end(stream, L"Principal");

    pop_indent();
    return write_element_end(stream, L"Principals");
}

const WCHAR *string_from_instances_policy(TASK_INSTANCES_POLICY policy)
{
    switch (policy)
    {
        case TASK_INSTANCES_PARALLEL:       return L"Parallel";
        case TASK_INSTANCES_QUEUE:          return L"Queue";
        case TASK_INSTANCES_IGNORE_NEW:     return L"IgnoreNew";
        case TASK_INSTANCES_STOP_EXISTING : return L"StopExisting";
    }
    return L"<error>";
}

static HRESULT write_settings(IStream *stream, ITaskSettings *settings)
{
    INetworkSettings *network_settings;
    TASK_INSTANCES_POLICY policy;
    IIdleSettings *idle_settings;
    VARIANT_BOOL bval;
    HRESULT hr;
    INT ival;
    BSTR s;

    if (!settings)
        return write_empty_element(stream, L"Settings");

    if (FAILED(hr = write_element(stream, L"Settings")))
        return hr;

    push_indent();

#define WRITE_BOOL_OPTION(name) \
    { \
        if (FAILED(hr = ITaskSettings_get_##name(settings, &bval))) \
            return hr; \
        if (FAILED(hr = write_bool_value(stream, L ## #name, bval))) \
            return hr; \
    }


    if (FAILED(hr = ITaskSettings_get_AllowDemandStart(settings, &bval)))
        return hr;
    if (FAILED(hr = write_bool_value(stream, L"AllowStartOnDemand", bval)))
        return hr;

    if (SUCCEEDED(hr = TaskSettings_get_RestartInterval(settings, &s)) && s)
    {
        FIXME("RestartInterval not handled.\n");
        SysFreeString(s);
    }
    if (FAILED(hr = ITaskSettings_get_MultipleInstances(settings, &policy)))
        return hr;
    if (FAILED(hr = write_text_value(stream, L"MultipleInstancesPolicy", string_from_instances_policy(policy))))
        return hr;

    WRITE_BOOL_OPTION(DisallowStartIfOnBatteries);
    WRITE_BOOL_OPTION(StopIfGoingOnBatteries);
    WRITE_BOOL_OPTION(AllowHardTerminate);
    WRITE_BOOL_OPTION(StartWhenAvailable);
    WRITE_BOOL_OPTION(RunOnlyIfNetworkAvailable);
    WRITE_BOOL_OPTION(WakeToRun);
    WRITE_BOOL_OPTION(Enabled);
    WRITE_BOOL_OPTION(Hidden);

    if (SUCCEEDED(hr = TaskSettings_get_DeleteExpiredTaskAfter(settings, &s)) && s)
    {
        hr = write_text_value(stream, L"DeleteExpiredTaskAfter", s);
        SysFreeString(s);
        if (FAILED(hr))
            return hr;
    }
    if (SUCCEEDED(hr = TaskSettings_get_IdleSettings(settings, &idle_settings)))
    {
        FIXME("IdleSettings not handled.\n");
        IIdleSettings_Release(idle_settings);
    }
    if (SUCCEEDED(hr = TaskSettings_get_NetworkSettings(settings, &network_settings)))
    {
        FIXME("NetworkSettings not handled.\n");
        INetworkSettings_Release(network_settings);
    }
    if (SUCCEEDED(hr = TaskSettings_get_ExecutionTimeLimit(settings, &s)) && s)
    {
        hr = write_text_value(stream, L"ExecutionTimeLimit", s);
        SysFreeString(s);
        if (FAILED(hr))
            return hr;
    }
    if (FAILED(hr = ITaskSettings_get_Priority(settings, &ival)))
        return hr;
    if (FAILED(hr = write_int_value(stream, L"Priority", ival)))
        return hr;

    WRITE_BOOL_OPTION(RunOnlyIfIdle);
#undef WRITE_BOOL_OPTION

    pop_indent();
    write_element_end(stream, L"Settings");

    return S_OK;
}

static HRESULT write_triggers(IStream *stream, ITriggerCollection *triggers)
{
    if (!triggers)
        return write_empty_element(stream, L"Triggers");

    FIXME("stub\n");
    return S_OK;
}

static HRESULT write_actions(IStream *stream, IActionCollection *actions)
{
    if (!actions)
    {
        write_element(stream, L"Actions");
        push_indent();
        write_empty_element(stream, L"Exec");
        pop_indent();
        return write_element_end(stream, L"Actions");
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

    if (taskdef->data)
    {
        hr = write_text_value(stream, L"Data", taskdef->data);
        if (hr != S_OK) goto failed;
    }

    pop_indent();

    write_element_end(stream, L"Task");
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
    HRESULT hr;
    WCHAR *value;

    hr = read_text_value(reader, &value);
    if (hr != S_OK) return hr;

    if (!lstrcmpW(value, L"true"))
        *vbool = VARIANT_TRUE;
    else if (!lstrcmpW(value, L"false"))
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

    *int_val = wcstol(value, NULL, 10);

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

        if (!lstrcmpW(name, L"id"))
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

            if (!lstrcmpW(name, L"Principal"))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, L"UserId"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IPrincipal_put_UserId(principal, value);
            }
            else if (!lstrcmpW(name, L"LogonType"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    TASK_LOGON_TYPE logon = TASK_LOGON_NONE;

                    if (!lstrcmpW(value, L"InteractiveToken"))
                        logon = TASK_LOGON_INTERACTIVE_TOKEN;
                    else
                        FIXME("unhandled LogonType %s\n", debugstr_w(value));

                    IPrincipal_put_LogonType(principal, logon);
                }
            }
            else if (!lstrcmpW(name, L"RunLevel"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    TASK_RUNLEVEL_TYPE level = TASK_RUNLEVEL_LUA;

                    if (!lstrcmpW(value, L"LeastPrivilege"))
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

            if (!lstrcmpW(name, L"Principals"))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, L"Principal"))
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

            if (!lstrcmpW(name, L"Settings"))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, L"MultipleInstancesPolicy"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                {
                    int_val = TASK_INSTANCES_IGNORE_NEW;

                    if (!lstrcmpW(value, L"IgnoreNew"))
                        int_val = TASK_INSTANCES_IGNORE_NEW;
                    else
                        FIXME("unhandled MultipleInstancesPolicy %s\n", debugstr_w(value));

                    ITaskSettings_put_MultipleInstances(taskset, int_val);
                }
            }
            else if (!lstrcmpW(name, L"DisallowStartIfOnBatteries"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_DisallowStartIfOnBatteries(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"AllowStartOnDemand"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_AllowDemandStart(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"StopIfGoingOnBatteries"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_StopIfGoingOnBatteries(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"AllowHardTerminate"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_AllowHardTerminate(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"StartWhenAvailable"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_StartWhenAvailable(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"RunOnlyIfNetworkAvailable"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_RunOnlyIfNetworkAvailable(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"Enabled"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_Enabled(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"Hidden"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_Hidden(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"RunOnlyIfIdle"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_RunOnlyIfIdle(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"WakeToRun"))
            {
                hr = read_variantbool_value(reader, &bool_val);
                if (hr != S_OK) return hr;
                ITaskSettings_put_WakeToRun(taskset, bool_val);
            }
            else if (!lstrcmpW(name, L"ExecutionTimeLimit"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    ITaskSettings_put_ExecutionTimeLimit(taskset, value);
            }
            else if (!lstrcmpW(name, L"Priority"))
            {
                hr = read_int_value(reader, &int_val);
                if (hr == S_OK)
                    ITaskSettings_put_Priority(taskset, int_val);
            }
            else if (!lstrcmpW(name, L"IdleSettings"))
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

            if (!lstrcmpW(name, L"RegistrationInfo"))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, L"Author"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Author(info, value);
            }
            else if (!lstrcmpW(name, L"Description"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Description(info, value);
            }
            else if (!lstrcmpW(name, L"Version"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Version(info, value);
            }
            else if (!lstrcmpW(name, L"Date"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Date(info, value);
            }
            else if (!lstrcmpW(name, L"Documentation"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Documentation(info, value);
            }
            else if (!lstrcmpW(name, L"URI"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_URI(info, value);
            }
            else if (!lstrcmpW(name, L"Source"))
            {
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    IRegistrationInfo_put_Source(info, value);
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

        if (!lstrcmpW(name, L"version"))
        {
            TASK_COMPATIBILITY compatibility = TASK_COMPATIBILITY_V2;

            if (!lstrcmpW(value, L"1.0"))
                compatibility = TASK_COMPATIBILITY_AT;
            else if (!lstrcmpW(value, L"1.1"))
                compatibility = TASK_COMPATIBILITY_V1;
            else if (!lstrcmpW(value, L"1.2"))
                compatibility = TASK_COMPATIBILITY_V2;
            else if (!lstrcmpW(value, L"1.3"))
                compatibility = TASK_COMPATIBILITY_V2_1;
            else
                FIXME("unknown version %s\n", debugstr_w(value));

            ITaskSettings_put_Compatibility(taskset, compatibility);
        }
        else if (!lstrcmpW(name, L"xmlns"))
        {
            if (lstrcmpW(value, L"http://schemas.microsoft.com/windows/2004/02/mit/task"))
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

            if (!lstrcmpW(name, L"Task"))
                return S_OK;

            break;

        case XmlNodeType_Element:
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            if (hr != S_OK) return hr;

            TRACE("Element: %s\n", debugstr_w(name));

            if (!lstrcmpW(name, L"RegistrationInfo"))
            {
                IRegistrationInfo *info;

                hr = ITaskDefinition_get_RegistrationInfo(taskdef, &info);
                if (hr != S_OK) return hr;
                hr = read_registration_info(reader, info);
                IRegistrationInfo_Release(info);
            }
            else if (!lstrcmpW(name, L"Settings"))
            {
                ITaskSettings *taskset;

                hr = ITaskDefinition_get_Settings(taskdef, &taskset);
                if (hr != S_OK) return hr;
                hr = read_settings(reader, taskset);
                ITaskSettings_Release(taskset);
            }
            else if (!lstrcmpW(name, L"Triggers"))
                hr = read_triggers(reader, taskdef);
            else if (!lstrcmpW(name, L"Principals"))
                hr = read_principals(reader, taskdef);
            else if (!lstrcmpW(name, L"Actions"))
                hr = read_actions(reader, taskdef);
            else if (!lstrcmpW(name, L"Data"))
            {
                WCHAR *value;
                hr = read_text_value(reader, &value);
                if (hr == S_OK)
                    ITaskDefinition_put_Data(taskdef, value);
            }
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

            if (!lstrcmpW(name, L"Task"))
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
        if (taskdef->data)
        {
            SysFreeString(taskdef->data);
            taskdef->data = NULL;
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

    taskdef = calloc(1, sizeof(*taskdef));
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
    WCHAR user_name[256];
    WCHAR domain_name[256];
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
        free(task_svc);
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
    FIXME("%p,%u,%lu,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_GetIDsOfNames(ITaskService *iface, REFIID riid, LPOLESTR *names,
                                                UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_Invoke(ITaskService *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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
    FIXME("%p,%lx,%p: stub\n", iface, flags, tasks);
    return E_NOTIMPL;
}

static HRESULT WINAPI TaskService_NewTask(ITaskService *iface, DWORD flags, ITaskDefinition **definition)
{
    TRACE("%p,%lx,%p\n", iface, flags, definition);

    if (!definition) return E_POINTER;

    if (flags)
        FIXME("unsupported flags %lx\n", flags);

    return TaskDefinition_create(definition);
}

static inline BOOL is_variant_null(const VARIANT *var)
{
    return V_VT(var) == VT_EMPTY || V_VT(var) == VT_NULL ||
          (V_VT(var) == VT_BSTR && (V_BSTR(var) == NULL || !*V_BSTR(var)));
}

static HRESULT start_schedsvc(void)
{
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    ULONGLONG start_time;
    HRESULT hr = SCHED_E_SERVICE_NOT_RUNNING;

    TRACE("Trying to start Schedule service\n");

    scm = OpenSCManagerW(NULL, NULL, 0);
    if (!scm) return SCHED_E_SERVICE_NOT_INSTALLED;

    service = OpenServiceW(scm, L"Schedule", SERVICE_START | SERVICE_QUERY_STATUS);
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
                    WARN("failed to query scheduler status (%lu)\n", GetLastError());
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
                WARN("scheduler failed to start %lu\n", status.dwCurrentState);
        }
        else
            WARN("failed to start scheduler service (%lu)\n", GetLastError());

        CloseServiceHandle(service);
    }
    else
        WARN("failed to open scheduler service (%lu)\n", GetLastError());

    CloseServiceHandle(scm);
    return hr;
}

static HRESULT WINAPI TaskService_Connect(ITaskService *iface, VARIANT server, VARIANT user, VARIANT domain, VARIANT password)
{
    static WCHAR ncalrpc[] = L"ncalrpc";
    TaskService *task_svc = impl_from_ITaskService(iface);
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR user_name[256];
    WCHAR domain_name[256];
    DWORD len;
    HRESULT hr;
    RPC_WSTR binding_str;
    extern handle_t schrpc_handle;

    TRACE("%p,%s,%s,%s,%s\n", iface, debugstr_variant(&server), debugstr_variant(&user),
          debugstr_variant(&domain), debugstr_variant(&password));

    if (!is_variant_null(&user) || !is_variant_null(&domain) || !is_variant_null(&password))
        FIXME("user/domain/password are ignored\n");

    len = ARRAY_SIZE(comp_name);
    if (!GetComputerNameW(comp_name, &len))
        return HRESULT_FROM_WIN32(GetLastError());

    len = ARRAY_SIZE(user_name);
    if (!GetUserNameW(user_name, &len))
        return HRESULT_FROM_WIN32(GetLastError());

    len = ARRAY_SIZE(domain_name);
    if(!GetEnvironmentVariableW(L"USERDOMAIN", domain_name, len))
    {
        if (!GetComputerNameExW(ComputerNameDnsHostname, domain_name, &len))
            return HRESULT_FROM_WIN32(GetLastError());
        wcsupr(domain_name);
    }

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

        if (wcsicmp(server_name, comp_name))
        {
            FIXME("connection to remote server %s is not supported\n", debugstr_w(V_BSTR(&server)));
            return HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }
    }

    hr = start_schedsvc();
    if (hr != S_OK) return hr;

    hr = RpcStringBindingComposeW(NULL, ncalrpc, NULL, NULL, NULL, &binding_str);
    if (hr != RPC_S_OK) return hr;
    hr = RpcBindingFromStringBindingW(binding_str, &schrpc_handle);
    RpcStringFreeW(&binding_str);
    if (hr != RPC_S_OK) return hr;

    /* Make sure that the connection works */
    hr = SchRpcHighestVersion(&task_svc->version);
    if (hr != S_OK) return hr;

    TRACE("server version %#lx\n", task_svc->version);

    lstrcpyW(task_svc->comp_name, comp_name);
    lstrcpyW(task_svc->user_name, user_name);
    lstrcpyW(task_svc->domain_name, domain_name);
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
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, user);

    if (!user) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    *user = SysAllocString(task_svc->user_name);
    if (!*user) return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI TaskService_get_ConnectedDomain(ITaskService *iface, BSTR *domain)
{
    TaskService *task_svc = impl_from_ITaskService(iface);

    TRACE("%p,%p\n", iface, domain);

    if (!domain) return E_POINTER;

    if (!task_svc->connected)
        return HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);

    *domain = SysAllocString(task_svc->domain_name);
    if (!*domain) return E_OUTOFMEMORY;

    return S_OK;
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

    task_svc = malloc(sizeof(*task_svc));
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
    return malloc(n);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    free(p);
}
