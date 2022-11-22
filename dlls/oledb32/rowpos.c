/* OLE DB Row Position library
 *
 * Copyright 2013 Nikolay Sivov
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

#include "windef.h"
#include "ole2.h"
#include "olectl.h"

#include "oledb.h"
#include "oledberr.h"

#include "oledb_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

typedef struct rowpos rowpos;
typedef struct
{
    IConnectionPoint IConnectionPoint_iface;
    rowpos *container;
    IRowPositionChange **sinks;
    DWORD sinks_size;
} rowpos_cp;

struct rowpos
{
    IRowPosition IRowPosition_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG ref;

    IRowset *rowset;
    IChapteredRowset *chrst;
    HROW row;
    HCHAPTER chapter;
    DBPOSITIONFLAGS flags;
    BOOL cleared;
    rowpos_cp cp;
};

static void rowposchange_cp_destroy(rowpos_cp*);

static inline rowpos *impl_from_IRowPosition(IRowPosition *iface)
{
    return CONTAINING_RECORD(iface, rowpos, IRowPosition_iface);
}

static inline rowpos *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, rowpos, IConnectionPointContainer_iface);
}

static inline rowpos_cp *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, rowpos_cp, IConnectionPoint_iface);
}

static HRESULT rowpos_fireevent(rowpos *rp, DBREASON reason, DBEVENTPHASE phase)
{
    BOOL cant_deny = phase == DBEVENTPHASE_FAILEDTODO || phase == DBEVENTPHASE_SYNCHAFTER;
    HRESULT hr = S_OK;
    DWORD i;

    for (i = 0; i < rp->cp.sinks_size; i++)
        if (rp->cp.sinks[i])
        {
            hr = IRowPositionChange_OnRowPositionChange(rp->cp.sinks[i], reason, phase, cant_deny);
            if (phase == DBEVENTPHASE_FAILEDTODO) return DB_E_CANCELED;
            if (hr != S_OK) return hr;
        }

    return hr;
}

static void rowpos_clearposition(rowpos *rp)
{
    if (!rp->cleared)
    {
        if (rp->rowset)
            IRowset_ReleaseRows(rp->rowset, 1, &rp->row, NULL, NULL, NULL);
        if (rp->chrst)
            IChapteredRowset_ReleaseChapter(rp->chrst, rp->chapter, NULL);
    }

    rp->row = DB_NULL_HROW;
    rp->chapter = DB_NULL_HCHAPTER;
    rp->flags = DBPOSITION_NOROW;
}

static HRESULT WINAPI rowpos_QueryInterface(IRowPosition* iface, REFIID riid, void **obj)
{
    rowpos *This = impl_from_IRowPosition(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRowPosition))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IConnectionPointContainer))
    {
        *obj = &This->IConnectionPointContainer_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IRowPosition_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI rowpos_AddRef(IRowPosition* iface)
{
    rowpos *This = impl_from_IRowPosition(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%ld)\n", This, ref);
    return ref;
}

static ULONG WINAPI rowpos_Release(IRowPosition* iface)
{
    rowpos *This = impl_from_IRowPosition(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%ld)\n", This, ref);

    if (ref == 0)
    {
        if (This->rowset) IRowset_Release(This->rowset);
        if (This->chrst) IChapteredRowset_Release(This->chrst);
        rowposchange_cp_destroy(&This->cp);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI rowpos_ClearRowPosition(IRowPosition* iface)
{
    rowpos *This = impl_from_IRowPosition(iface);
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!This->rowset) return E_UNEXPECTED;

    hr = rowpos_fireevent(This, DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_OKTODO);
    if (hr != S_OK)
        return rowpos_fireevent(This, DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_FAILEDTODO);

    hr = rowpos_fireevent(This, DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_ABOUTTODO);
    if (hr != S_OK)
        return rowpos_fireevent(This, DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_FAILEDTODO);

    rowpos_clearposition(This);
    This->cleared = TRUE;
    return S_OK;
}

static HRESULT WINAPI rowpos_GetRowPosition(IRowPosition *iface, HCHAPTER *chapter,
    HROW *row, DBPOSITIONFLAGS *flags)
{
    rowpos *This = impl_from_IRowPosition(iface);

    TRACE("(%p)->(%p %p %p)\n", This, chapter, row, flags);

    *chapter = This->chapter;
    *row = This->row;
    *flags = This->flags;

    if (!This->rowset) return E_UNEXPECTED;

    return S_OK;
}

static HRESULT WINAPI rowpos_GetRowset(IRowPosition *iface, REFIID riid, IUnknown **rowset)
{
    rowpos *This = impl_from_IRowPosition(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), rowset);

    if (!This->rowset) return E_UNEXPECTED;
    return IRowset_QueryInterface(This->rowset, riid, (void**)rowset);
}

static HRESULT WINAPI rowpos_Initialize(IRowPosition *iface, IUnknown *rowset)
{
    rowpos *This = impl_from_IRowPosition(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, rowset);

    if (This->rowset) return DB_E_ALREADYINITIALIZED;

    hr = IUnknown_QueryInterface(rowset, &IID_IRowset, (void**)&This->rowset);
    if (FAILED(hr)) return hr;

    /* this one is optional */
    IUnknown_QueryInterface(rowset, &IID_IChapteredRowset, (void**)&This->chrst);
    return S_OK;
}

static HRESULT WINAPI rowpos_SetRowPosition(IRowPosition *iface, HCHAPTER chapter,
    HROW row, DBPOSITIONFLAGS flags)
{
    rowpos *This = impl_from_IRowPosition(iface);
    DBREASON reason;
    HRESULT hr;

    TRACE("(%p)->(%Ix %Ix %ld)\n", This, chapter, row, flags);

    if (!This->cleared) return E_UNEXPECTED;

    hr = IRowset_AddRefRows(This->rowset, 1, &row, NULL, NULL);
    if (FAILED(hr)) return hr;

    if (This->chrst)
    {
        hr = IChapteredRowset_AddRefChapter(This->chrst, chapter, NULL);
        if (FAILED(hr))
        {
            IRowset_ReleaseRows(This->rowset, 1, &row, NULL, NULL, NULL);
            return hr;
        }
    }

    reason = This->chrst ? DBREASON_ROWPOSITION_CHAPTERCHANGED : DBREASON_ROWPOSITION_CHANGED;
    hr = rowpos_fireevent(This, reason, DBEVENTPHASE_SYNCHAFTER);
    if (hr != S_OK)
    {
        IRowset_ReleaseRows(This->rowset, 1, &row, NULL, NULL, NULL);
        if (This->chrst)
            IChapteredRowset_ReleaseChapter(This->chrst, chapter, NULL);
        return rowpos_fireevent(This, reason, DBEVENTPHASE_FAILEDTODO);
    }
    else
        rowpos_fireevent(This, reason, DBEVENTPHASE_DIDEVENT);

    /* previously set chapter and row are released with ClearRowPosition() */
    This->chapter = chapter;
    This->row = row;
    This->flags = flags;
    This->cleared = FALSE;

    return S_OK;
}

static const struct IRowPositionVtbl rowpos_vtbl =
{
    rowpos_QueryInterface,
    rowpos_AddRef,
    rowpos_Release,
    rowpos_ClearRowPosition,
    rowpos_GetRowPosition,
    rowpos_GetRowset,
    rowpos_Initialize,
    rowpos_SetRowPosition
};

static HRESULT WINAPI cpc_QueryInterface(IConnectionPointContainer *iface, REFIID riid, void **obj)
{
    rowpos *This = impl_from_IConnectionPointContainer(iface);
    return IRowPosition_QueryInterface(&This->IRowPosition_iface, riid, obj);
}

static ULONG WINAPI cpc_AddRef(IConnectionPointContainer *iface)
{
    rowpos *This = impl_from_IConnectionPointContainer(iface);
    return IRowPosition_AddRef(&This->IRowPosition_iface);
}

static ULONG WINAPI cpc_Release(IConnectionPointContainer *iface)
{
    rowpos *This = impl_from_IConnectionPointContainer(iface);
    return IRowPosition_Release(&This->IRowPosition_iface);
}

static HRESULT WINAPI cpc_EnumConnectionPoints(IConnectionPointContainer *iface, IEnumConnectionPoints **enum_points)
{
    rowpos *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p): stub\n", This, enum_points);
    return E_NOTIMPL;
}

static HRESULT WINAPI cpc_FindConnectionPoint(IConnectionPointContainer *iface, REFIID riid, IConnectionPoint **point)
{
    rowpos *This = impl_from_IConnectionPointContainer(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), point);

    if (IsEqualIID(riid, &IID_IRowPositionChange))
    {
        *point = &This->cp.IConnectionPoint_iface;
        IConnectionPoint_AddRef(*point);
        return S_OK;
    }
    else
    {
        FIXME("unsupported riid %s\n", debugstr_guid(riid));
        return CONNECT_E_NOCONNECTION;
    }
}

static const struct IConnectionPointContainerVtbl rowpos_cpc_vtbl =
{
    cpc_QueryInterface,
    cpc_AddRef,
    cpc_Release,
    cpc_EnumConnectionPoints,
    cpc_FindConnectionPoint
};

static HRESULT WINAPI rowpos_cp_QueryInterface(IConnectionPoint *iface, REFIID riid, void **obj)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_QueryInterface(&This->container->IConnectionPointContainer_iface, riid, obj);
}

static ULONG WINAPI rowpos_cp_AddRef(IConnectionPoint *iface)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(&This->container->IConnectionPointContainer_iface);
}

static ULONG WINAPI rowpos_cp_Release(IConnectionPoint *iface)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(&This->container->IConnectionPointContainer_iface);
}

static HRESULT WINAPI rowpos_cp_GetConnectionInterface(IConnectionPoint *iface, IID *iid)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, iid);

    if (!iid) return E_POINTER;

    *iid = IID_IRowPositionChange;
    return S_OK;
}

static HRESULT WINAPI rowpos_cp_GetConnectionPointContainer(IConnectionPoint *iface, IConnectionPointContainer **container)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, container);

    if (!container) return E_POINTER;

    *container = &This->container->IConnectionPointContainer_iface;
    IConnectionPointContainer_AddRef(*container);
    return S_OK;
}

static HRESULT WINAPI rowpos_cp_Advise(IConnectionPoint *iface, IUnknown *unksink, DWORD *cookie)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);
    IRowPositionChange *sink;
    IRowPositionChange **new_sinks;
    HRESULT hr;
    DWORD i;

    TRACE("(%p)->(%p %p)\n", This, unksink, cookie);

    if (!cookie) return E_POINTER;

    hr = IUnknown_QueryInterface(unksink, &IID_IRowPositionChange, (void**)&sink);
    if (FAILED(hr))
    {
        FIXME("sink doesn't support IRowPositionChange\n");
        return CONNECT_E_CANNOTCONNECT;
    }

    if (This->sinks)
    {
        for (i = 0; i < This->sinks_size; i++)
        {
            if (!This->sinks[i])
                break;
        }

        if (i == This->sinks_size)
        {
            new_sinks = realloc(This->sinks, 2 * This->sinks_size * sizeof(*This->sinks));
            if (!new_sinks)
                return E_OUTOFMEMORY;
            memset(new_sinks + This->sinks_size, 0, This->sinks_size * sizeof(*This->sinks));
            This->sinks = new_sinks;
            This->sinks_size *= 2;
        }
    }
    else
    {
        This->sinks = calloc(10, sizeof(*This->sinks));
        if (!This->sinks)
            return E_OUTOFMEMORY;
        This->sinks_size = 10;
        i = 0;
    }

    This->sinks[i] = sink;
    *cookie = i + 1;

    return S_OK;
}

static HRESULT WINAPI rowpos_cp_Unadvise(IConnectionPoint *iface, DWORD cookie)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%ld)\n", This, cookie);

    if (!cookie || cookie > This->sinks_size || !This->sinks[cookie-1])
        return CONNECT_E_NOCONNECTION;

    IRowPositionChange_Release(This->sinks[cookie-1]);
    This->sinks[cookie-1] = NULL;

    return S_OK;
}

static HRESULT WINAPI rowpos_cp_EnumConnections(IConnectionPoint *iface, IEnumConnections **enum_c)
{
    rowpos_cp *This = impl_from_IConnectionPoint(iface);
    FIXME("(%p)->(%p): stub\n", This, enum_c);
    return E_NOTIMPL;
}

static const struct IConnectionPointVtbl rowpos_cp_vtbl =
{
    rowpos_cp_QueryInterface,
    rowpos_cp_AddRef,
    rowpos_cp_Release,
    rowpos_cp_GetConnectionInterface,
    rowpos_cp_GetConnectionPointContainer,
    rowpos_cp_Advise,
    rowpos_cp_Unadvise,
    rowpos_cp_EnumConnections
};

static void rowposchange_cp_init(rowpos_cp *cp, rowpos *container)
{
    cp->IConnectionPoint_iface.lpVtbl = &rowpos_cp_vtbl;
    cp->container = container;
    cp->sinks = NULL;
    cp->sinks_size = 0;
}

void rowposchange_cp_destroy(rowpos_cp *cp)
{
    DWORD i;
    for (i = 0; i < cp->sinks_size; i++)
    {
        if (cp->sinks[i])
            IRowPositionChange_Release(cp->sinks[i]);
    }
    free(cp->sinks);
}

HRESULT create_oledb_rowpos(IUnknown *outer, void **obj)
{
    rowpos *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = malloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IRowPosition_iface.lpVtbl = &rowpos_vtbl;
    This->IConnectionPointContainer_iface.lpVtbl = &rowpos_cpc_vtbl;
    This->ref = 1;
    This->rowset = NULL;
    This->chrst = NULL;
    This->cleared = FALSE;
    rowpos_clearposition(This);
    rowposchange_cp_init(&This->cp, This);

    *obj = &This->IRowPosition_iface;

    return S_OK;
}
