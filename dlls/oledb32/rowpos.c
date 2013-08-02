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

#include "oledb.h"
#include "oledberr.h"

#include "oledb_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

typedef struct
{
    IRowPosition IRowPosition_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG ref;

    IRowset *rowset;
} rowpos;

static inline rowpos *impl_from_IRowPosition(IRowPosition *iface)
{
    return CONTAINING_RECORD(iface, rowpos, IRowPosition_iface);
}

static inline rowpos *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, rowpos, IConnectionPointContainer_iface);
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
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI rowpos_Release(IRowPosition* iface)
{
    rowpos *This = impl_from_IRowPosition(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        if (This->rowset) IRowset_Release(This->rowset);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI rowpos_ClearRowPosition(IRowPosition* iface)
{
    rowpos *This = impl_from_IRowPosition(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowpos_GetRowPosition(IRowPosition *iface, HCHAPTER *chapter,
    HROW *row, DBPOSITIONFLAGS *flags)
{
    rowpos *This = impl_from_IRowPosition(iface);
    FIXME("(%p)->(%p %p %p): stub\n", This, chapter, row, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowpos_GetRowset(IRowPosition *iface, REFIID riid, IUnknown **rowset)
{
    rowpos *This = impl_from_IRowPosition(iface);
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), rowset);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowpos_Initialize(IRowPosition *iface, IUnknown *rowset)
{
    rowpos *This = impl_from_IRowPosition(iface);

    TRACE("(%p)->(%p)\n", This, rowset);

    if (This->rowset) return DB_E_ALREADYINITIALIZED;

    return IUnknown_QueryInterface(rowset, &IID_IRowset, (void**)&This->rowset);
}

static HRESULT WINAPI rowpos_SetRowPosition(IRowPosition *iface, HCHAPTER chapter,
    HROW row, DBPOSITIONFLAGS flags)
{
    rowpos *This = impl_from_IRowPosition(iface);
    FIXME("(%p)->(%lx %lx %d): stub\n", This, chapter, row, flags);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), point);
    return E_NOTIMPL;
}

static const struct IConnectionPointContainerVtbl rowpos_cpc_vtbl =
{
    cpc_QueryInterface,
    cpc_AddRef,
    cpc_Release,
    cpc_EnumConnectionPoints,
    cpc_FindConnectionPoint
};

HRESULT create_oledb_rowpos(IUnknown *outer, void **obj)
{
    rowpos *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IRowPosition_iface.lpVtbl = &rowpos_vtbl;
    This->IConnectionPointContainer_iface.lpVtbl = &rowpos_cpc_vtbl;
    This->ref = 1;
    This->rowset = NULL;

    *obj = &This->IRowPosition_iface;

    return S_OK;
}
