/*
 * Copyright 2025 Piotr Caban for CodeWeavers
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
#include "oledb.h"
#include "unknwn.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct rowset
{
    IRowset IRowset_iface;
    LONG refs;
};

static inline struct rowset *impl_from_IRowset(IRowset *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IRowset_iface);
}

static HRESULT WINAPI rowset_QueryInterface(IRowset *iface, REFIID riid, void **ppv)
{
    struct rowset *rowset = impl_from_IRowset(iface);

    TRACE("%p, %s, %p\n", rowset, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IRowset, riid))
    {
        *ppv = &rowset->IRowset_iface;
    }

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI rowset_AddRef(IRowset *iface)
{
    struct rowset *rowset = impl_from_IRowset(iface);
    LONG refs = InterlockedIncrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    return refs;
}

static ULONG WINAPI rowset_Release(IRowset *iface)
{
    struct rowset *rowset = impl_from_IRowset(iface);
    LONG refs = InterlockedDecrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    if (!refs)
    {
        TRACE("destroying %p\n", rowset);

        free(rowset);
    }
    return refs;
}

static HRESULT WINAPI rowset_AddRefRows(IRowset *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowset(iface);

    FIXME("%p, %Id, %p, %p, %p\n", rowset, count, rows, ref_counts, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetData(IRowset *iface, HROW row, HACCESSOR accessor, void *data)
{
    struct rowset *rowset = impl_from_IRowset(iface);

    FIXME("%p, %Id, %Id, %p\n", rowset, row, accessor, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetNextRows(IRowset *iface, HCHAPTER reserved,
        DBROWOFFSET offset, DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowset *rowset = impl_from_IRowset( iface );

    FIXME("%p, %Id, %Id, %Id, %p, %p\n", rowset, reserved, offset, count, obtained, rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_ReleaseRows(IRowset *iface, DBCOUNTITEM count, const HROW rows[],
        DBROWOPTIONS options[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowset(iface);

    FIXME("%p, %Id, %p, %p, %p, %p\n", rowset, count, rows, options, ref_counts, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_RestartPosition(IRowset *iface, HCHAPTER reserved)
{
    struct rowset *rowset = impl_from_IRowset(iface);

    FIXME("%p, %Id\n", rowset, reserved);
    return E_NOTIMPL;
}

static const struct IRowsetVtbl rowset_vtbl =
{
    rowset_QueryInterface,
    rowset_AddRef,
    rowset_Release,
    rowset_AddRefRows,
    rowset_GetData,
    rowset_GetNextRows,
    rowset_ReleaseRows,
    rowset_RestartPosition
};

HRESULT create_mem_rowset(IUnknown **ret)
{
    struct rowset *rowset;

    rowset = malloc(sizeof(*rowset));
    if (!rowset) return E_OUTOFMEMORY;

    rowset->IRowset_iface.lpVtbl = &rowset_vtbl;
    rowset->refs = 1;

    *ret = (IUnknown *)&rowset->IRowset_iface;
    return S_OK;
}
