/*              DirectShow Timeline object (QEDIT.DLL)
 *
 * Copyright 2016 Alex Henrie
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

typedef struct {
    IUnknown IUnknown_inner;
    IAMTimeline IAMTimeline_iface;
    IUnknown *outer_unk;
    LONG ref;
} TimelineImpl;

static inline TimelineImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TimelineImpl, IUnknown_inner);
}

static inline TimelineImpl *impl_from_IAMTimeline(IAMTimeline *iface)
{
    return CONTAINING_RECORD(iface, TimelineImpl, IAMTimeline_iface);
}

/* Timeline inner IUnknown */

static HRESULT WINAPI Timeline_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    TimelineImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IAMTimeline))
        *ppv = &This->IAMTimeline_iface;
    else
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);

    if (!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Timeline_AddRef(IUnknown *iface)
{
    TimelineImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) new ref = %u\n", This, ref);

    return ref;
}

static ULONG WINAPI Timeline_Release(IUnknown *iface)
{
    TimelineImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) new ref = %u\n", This, ref);

    if (ref == 0)
    {
        CoTaskMemFree(This);
        return 0;
    }

    return ref;
}

static const IUnknownVtbl timeline_vtbl =
{
    Timeline_QueryInterface,
    Timeline_AddRef,
    Timeline_Release,
};

/* IAMTimeline implementation */

static HRESULT WINAPI Timeline_IAMTimeline_QueryInterface(IAMTimeline *iface, REFIID riid, void **ppv)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI Timeline_IAMTimeline_AddRef(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI Timeline_IAMTimeline_Release(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI Timeline_IAMTimeline_CreateEmptyNode(IAMTimeline *iface, IAMTimelineObj **obj,
                                                           TIMELINE_MAJOR_TYPE type)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p,%04x): not implemented!\n", This, obj, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_AddGroup(IAMTimeline *iface, IAMTimelineObj *group)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_RemGroupFromList(IAMTimeline *iface, IAMTimelineObj *group)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetGroup(IAMTimeline *iface, IAMTimelineObj **group, LONG index)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p,%d): not implemented!\n", This, group, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetGroupCount(IAMTimeline *iface, LONG *count)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_ClearAllGroups(IAMTimeline *iface)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p): not implemented!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetInsertMode(IAMTimeline *iface, LONG *mode)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetInsertMode(IAMTimeline *iface, LONG mode)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EnableTransitions(IAMTimeline *iface, BOOL enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_TransitionsEnabled(IAMTimeline *iface, BOOL *enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EnableEffects(IAMTimeline *iface, BOOL enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_EffectsEnabled(IAMTimeline *iface, BOOL *enabled)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetInterestRange(IAMTimeline *iface, REFERENCE_TIME start,
                                                            REFERENCE_TIME stop)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s,%s): not implemented!\n", This, wine_dbgstr_longlong(start),
          wine_dbgstr_longlong(stop));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDuration(IAMTimeline *iface, REFERENCE_TIME *duration)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDuration2(IAMTimeline *iface, double *duration)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultFPS(IAMTimeline *iface, double fps)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%f): not implemented!\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultFPS(IAMTimeline *iface, double *fps)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, fps);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_IsDirty(IAMTimeline *iface, BOOL *dirty)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, dirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDirtyRange(IAMTimeline *iface, REFERENCE_TIME *start,
                                                         REFERENCE_TIME *stop)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p,%p): not implemented!\n", This, start, stop);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetCountOfType(IAMTimeline *iface, LONG group, LONG *value,
                                                          LONG *value_with_comps, TIMELINE_MAJOR_TYPE type)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d,%p,%p,%04x): not implemented!\n", This, group, value, value_with_comps, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_ValidateSourceNames(IAMTimeline *iface, LONG flags, IMediaLocator *override,
                                                               LONG_PTR notify_event)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%d,%p,%lx): not implemented!\n", This, flags, override, notify_event);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultTransition(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultTransition(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultEffect(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultEffect(IAMTimeline *iface, GUID *guid)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%s): not implemented!\n", This, wine_dbgstr_guid(guid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultTransitionB(IAMTimeline *iface, BSTR guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultTransitionB(IAMTimeline *iface, BSTR *guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_SetDefaultEffectB(IAMTimeline *iface, BSTR guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static HRESULT WINAPI Timeline_IAMTimeline_GetDefaultEffectB(IAMTimeline *iface, BSTR *guidb)
{
    TimelineImpl *This = impl_from_IAMTimeline(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, guidb);
    return E_NOTIMPL;
}

static const IAMTimelineVtbl IAMTimeline_VTable =
{
    Timeline_IAMTimeline_QueryInterface,
    Timeline_IAMTimeline_AddRef,
    Timeline_IAMTimeline_Release,
    Timeline_IAMTimeline_CreateEmptyNode,
    Timeline_IAMTimeline_AddGroup,
    Timeline_IAMTimeline_RemGroupFromList,
    Timeline_IAMTimeline_GetGroup,
    Timeline_IAMTimeline_GetGroupCount,
    Timeline_IAMTimeline_ClearAllGroups,
    Timeline_IAMTimeline_GetInsertMode,
    Timeline_IAMTimeline_SetInsertMode,
    Timeline_IAMTimeline_EnableTransitions,
    Timeline_IAMTimeline_TransitionsEnabled,
    Timeline_IAMTimeline_EnableEffects,
    Timeline_IAMTimeline_EffectsEnabled,
    Timeline_IAMTimeline_SetInterestRange,
    Timeline_IAMTimeline_GetDuration,
    Timeline_IAMTimeline_GetDuration2,
    Timeline_IAMTimeline_SetDefaultFPS,
    Timeline_IAMTimeline_GetDefaultFPS,
    Timeline_IAMTimeline_IsDirty,
    Timeline_IAMTimeline_GetDirtyRange,
    Timeline_IAMTimeline_GetCountOfType,
    Timeline_IAMTimeline_ValidateSourceNames,
    Timeline_IAMTimeline_SetDefaultTransition,
    Timeline_IAMTimeline_GetDefaultTransition,
    Timeline_IAMTimeline_SetDefaultEffect,
    Timeline_IAMTimeline_GetDefaultEffect,
    Timeline_IAMTimeline_SetDefaultTransitionB,
    Timeline_IAMTimeline_GetDefaultTransitionB,
    Timeline_IAMTimeline_SetDefaultEffectB,
    Timeline_IAMTimeline_GetDefaultEffectB,
};

HRESULT AMTimeline_create(IUnknown *pUnkOuter, LPVOID *ppv)
{
    TimelineImpl* obj = NULL;

    TRACE("(%p,%p)\n", pUnkOuter, ppv);

    obj = CoTaskMemAlloc(sizeof(TimelineImpl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(TimelineImpl));

    obj->ref = 1;
    obj->IUnknown_inner.lpVtbl = &timeline_vtbl;
    obj->IAMTimeline_iface.lpVtbl = &IAMTimeline_VTable;

    if (pUnkOuter)
        obj->outer_unk = pUnkOuter;
    else
        obj->outer_unk = &obj->IUnknown_inner;

    *ppv = &obj->IUnknown_inner;
    return S_OK;
}
