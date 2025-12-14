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
#include "msado15_backcompat.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

#define NO_INTERFACE ((void*)-1)

struct rowsetex
{
    IRowsetExactScroll IRowsetExactScroll_iface;
    LONG refs;

    IRowset *rowset;
    IRowsetLocate *rowset_loc;
    IRowsetExactScroll *rowset_es;
};

static inline struct rowsetex *impl_from_IRowsetExactScroll(IRowsetExactScroll *iface)
{
    return CONTAINING_RECORD(iface, struct rowsetex, IRowsetExactScroll_iface);
}

static HRESULT WINAPI rowsetex_QueryInterface(IRowsetExactScroll *iface, REFIID riid, void **obj)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %s, %p\n", rowset, debugstr_guid(riid), obj);

    *obj = NULL;
    if (IsEqualGUID(&IID_IUnknown, riid) ||
            IsEqualGUID(&IID_IRowset, riid))
    {
        *obj = &rowset->IRowsetExactScroll_iface;
    }
    else if (IsEqualGUID(&IID_IRowsetLocate, riid) ||
            IsEqualGUID(&IID_IRowsetScroll, riid) ||
            IsEqualGUID(&IID_IRowsetExactScroll, riid))
    {
        if (!rowset->rowset_loc) return E_NOINTERFACE;
        *obj = &rowset->IRowsetExactScroll_iface;
    }

    if(*obj)
    {
        IUnknown_AddRef((IUnknown*)*obj);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), obj);
    return IRowset_QueryInterface(rowset->rowset, riid, obj);
}

static ULONG WINAPI rowsetex_AddRef(IRowsetExactScroll *iface)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);
    LONG refs = InterlockedIncrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    return refs;
}

static ULONG WINAPI rowsetex_Release(IRowsetExactScroll *iface)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);
    LONG refs = InterlockedDecrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    if (!refs)
    {
        TRACE("destroying %p\n", rowset);
        if (rowset->rowset) IRowset_Release(rowset->rowset);
        if (rowset->rowset_loc) IRowsetLocate_Release(rowset->rowset_loc);
        if (rowset->rowset_es && rowset->rowset_es != NO_INTERFACE)
            IRowsetExactScroll_Release(rowset->rowset_es);
        free(rowset);
    }
    return refs;
}

static HRESULT WINAPI rowsetex_AddRefRows(IRowsetExactScroll *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %p, %p, %p\n", rowset, count, rows, ref_counts, status);
    return IRowset_AddRefRows(rowset->rowset, count, rows, ref_counts, status);
}

static HRESULT WINAPI rowsetex_GetData(IRowsetExactScroll *iface, HROW row, HACCESSOR hacc, void *data)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Id, %p\n", rowset, row, hacc, data);
    return IRowset_GetData(rowset->rowset, row, hacc, data);
}

static HRESULT WINAPI rowsetex_GetNextRows(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBROWOFFSET offset, DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Id, %Id, %p, %p\n", rowset, chapter, offset, count, obtained, rows);
    return IRowset_GetNextRows(rowset->rowset, chapter, offset, count, obtained, rows);
}

static HRESULT WINAPI rowsetex_ReleaseRows(IRowsetExactScroll *iface, DBCOUNTITEM count, const HROW rows[],
        DBROWOPTIONS options[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %p, %p, %p, %p\n", rowset, count, rows, options, ref_counts, status);
    return IRowset_ReleaseRows(rowset->rowset, count, rows, options, ref_counts, status);
}

static HRESULT WINAPI rowsetex_RestartPosition(IRowsetExactScroll *iface, HCHAPTER chapter)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id\n", rowset, chapter);
    return IRowset_RestartPosition(rowset->rowset, chapter);
}

static HRESULT WINAPI rowsetex_Compare(IRowsetExactScroll *iface, HCHAPTER chapter, DBBKMARK bookmark1_size,
        const BYTE *bookmark1, DBBKMARK bookmark2_size, const BYTE *bookmark2, DBCOMPARE *compare)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Iu, %p, %Iu, %p, %p\n", rowset, chapter, bookmark1_size,
            bookmark1, bookmark2_size, bookmark2, compare);
    return IRowsetLocate_Compare(rowset->rowset_loc, chapter, bookmark1_size,
            bookmark1, bookmark2_size, bookmark2, compare);
}

static HRESULT WINAPI rowsetex_GetRowsAt(IRowsetExactScroll *iface, HWATCHREGION watchregion,
        HCHAPTER chapter, DBBKMARK bookmark_size, const BYTE *bookmark, DBROWOFFSET offset,
        DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Id, %Iu, %p, %Id, %Id, %p, %p\n", rowset, watchregion, chapter,
            bookmark_size, bookmark, offset, count, obtained, rows);
    return IRowsetLocate_GetRowsAt(rowset->rowset_loc, watchregion, chapter,
            bookmark_size, bookmark, offset, count, obtained, rows);
}

static HRESULT WINAPI rowsetex_GetRowsByBookmark(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBCOUNTITEM rows_cnt, const DBBKMARK bookmark_size[], const BYTE *bookmark[],
        HROW row[], DBROWSTATUS status[])
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Id, %p, %p, %p, %p\n", rowset, chapter, rows_cnt,
            bookmark_size, bookmark, row, status);
    return IRowsetLocate_GetRowsByBookmark(rowset->rowset_loc, chapter, rows_cnt,
            bookmark_size, bookmark, row, status);
}

static HRESULT WINAPI rowsetex_Hash(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBBKMARK bookmark_cnt, const DBBKMARK bookmark_size[], const BYTE *bookmark[],
        DBHASHVALUE hash[], DBROWSTATUS status[])
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Iu, %p, %p, %p, %p\n", rowset, chapter, bookmark_cnt,
            bookmark_size, bookmark, hash, status);
    return IRowsetLocate_GetRowsByBookmark(rowset->rowset_loc, chapter, bookmark_cnt,
            bookmark_size, bookmark, hash, status);
}

static BOOL init_rowset_es(struct rowsetex *rowset)
{
    if (!rowset->rowset_es)
    {
        HRESULT hr = IRowset_QueryInterface(rowset->rowset,
                &IID_IRowsetExactScroll, (void**)&rowset->rowset_es);
        if (FAILED(hr))
            rowset->rowset_es = NO_INTERFACE;
    }
    return rowset->rowset_es != NO_INTERFACE;
}

static HRESULT WINAPI rowsetex_GetApproximatePosition(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBBKMARK bookmark_size, const BYTE *bookmark, DBCOUNTITEM *position, DBCOUNTITEM *rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Iu, %p, %p, %p\n", rowset, chapter, bookmark_size, bookmark, position, rows);

    if (init_rowset_es(rowset))
    {
        return IRowsetExactScroll_GetApproximatePosition(rowset->rowset_es,
                chapter, bookmark_size, bookmark, position, rows);
    }

    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetex_GetRowsAtRatio(IRowsetExactScroll *iface, HWATCHREGION watch_region, HCHAPTER chapter,
        DBCOUNTITEM numerator, DBCOUNTITEM denominator, DBROWCOUNT row_cnt, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Id, %Iu, %Iu, %Id, %p, %p\n", rowset, watch_region, chapter,
            numerator, denominator, row_cnt, obtained, rows);

    if (init_rowset_es(rowset))
    {
        return IRowsetExactScroll_GetRowsAtRatio(rowset->rowset_es, watch_region,
                chapter, numerator, denominator, row_cnt, obtained, rows);
    }

    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetex_GetExactPosition(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBBKMARK bookmark_size, const BYTE *bookmark, DBCOUNTITEM *position, DBCOUNTITEM *rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Iu, %p, %p, %p\n", rowset, chapter, bookmark_size, bookmark, position, rows);

    if (init_rowset_es(rowset))
    {
        return IRowsetExactScroll_GetExactPosition(rowset->rowset_es,
                chapter, bookmark_size, bookmark, position, rows);
    }

    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IRowsetExactScrollVtbl rowsetex_vtbl =
{
    rowsetex_QueryInterface,
    rowsetex_AddRef,
    rowsetex_Release,
    rowsetex_AddRefRows,
    rowsetex_GetData,
    rowsetex_GetNextRows,
    rowsetex_ReleaseRows,
    rowsetex_RestartPosition,
    rowsetex_Compare,
    rowsetex_GetRowsAt,
    rowsetex_GetRowsByBookmark,
    rowsetex_Hash,
    rowsetex_GetApproximatePosition,
    rowsetex_GetRowsAtRatio,
    rowsetex_GetExactPosition
};

HRESULT create_rowsetex(IUnknown *rowset, IUnknown **ret)
{
    struct rowsetex *rowsetex;
    HRESULT hr;

    rowsetex = calloc(1, sizeof(*rowsetex));
    if (!rowsetex) return E_OUTOFMEMORY;

    rowsetex->IRowsetExactScroll_iface.lpVtbl = &rowsetex_vtbl;
    rowsetex->refs = 1;

    hr = IUnknown_QueryInterface(rowset, &IID_IRowset, (void **)&rowsetex->rowset);
    if (FAILED(hr))
    {
        IRowsetExactScroll_Release(&rowsetex->IRowsetExactScroll_iface);
        return MAKE_ADO_HRESULT(adErrFeatureNotAvailable);
    }

    hr = IUnknown_QueryInterface(rowset, &IID_IRowsetLocate, (void **)&rowsetex->rowset_loc);
    if (FAILED(hr))
        rowsetex->rowset_loc = NULL;

    *ret = (IUnknown *)&rowsetex->IRowsetExactScroll_iface;
    return S_OK;
}
