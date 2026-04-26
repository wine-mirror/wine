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
#include "oledberr.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

#define NO_INTERFACE ((void*)-1)

struct rowsetex
{
    IRowsetExactScroll IRowsetExactScroll_iface;
    IAccessor IAccessor_iface;
    IRowsetFind IRowsetFind_iface;
    LONG refs;

    IRowset *rowset;
    IRowsetLocate *rowset_loc;
    IRowsetExactScroll *rowset_es;
    IAccessor *accessor;
    IRowsetFind *rowset_find;

    DBTYPE bookmark_type;
    HACCESSOR bookmark_hacc;
};

static inline struct rowsetex *impl_from_IRowsetExactScroll(IRowsetExactScroll *iface)
{
    return CONTAINING_RECORD(iface, struct rowsetex, IRowsetExactScroll_iface);
}

static inline struct rowsetex *impl_from_IAccessor(IAccessor *iface)
{
    return CONTAINING_RECORD(iface, struct rowsetex, IAccessor_iface);
}

static inline struct rowsetex *impl_from_IRowsetFind(IRowsetFind *iface)
{
    return CONTAINING_RECORD(iface, struct rowsetex, IRowsetFind_iface);
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
    else if (IsEqualGUID(&IID_IAccessor, riid))
    {
        if (!rowset->accessor)
        {
            HRESULT hr = IRowset_QueryInterface(rowset->rowset,
                    &IID_IAccessor, (void**)&rowset->accessor);
            if (FAILED(hr)) return hr;
        }
        *obj = &rowset->IAccessor_iface;
    }
    else if (IsEqualGUID(&IID_IRowsetFind, riid))
    {
        *obj = &rowset->IRowsetFind_iface;
    }
    else if (IsEqualGUID(&IID_IColumnsInfo, riid) ||
            IsEqualGUID(&IID_IRowsetIndex, riid) ||
            IsEqualGUID(&IID_IRowsetCurrentIndex, riid))
    {
        return IRowset_QueryInterface(rowset->rowset, riid, obj);
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
        if (rowset->bookmark_hacc)
            IAccessor_ReleaseAccessor(rowset->accessor, rowset->bookmark_hacc, NULL);
        if (rowset->accessor) IAccessor_Release(rowset->accessor);
        if (rowset->rowset_find && rowset->rowset_find != NO_INTERFACE)
            IRowsetFind_Release(rowset->rowset_find);
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

static HRESULT get_bookmark(struct rowsetex *rowset, HROW hrow, DBBKMARK *size,
        BYTE **bookmark, BYTE buf[sizeof(long long)])
{
    struct bookmark_data
    {
        union
        {
            int i4;
            LONGLONG i8;
            BYTE *ptr;
        } val;
        DBLENGTH len;
        DBSTATUS status;
    } bookmark_data;
    HRESULT hr;

    if (!rowset->bookmark_hacc)
    {
        DBBINDING binding;

        memset(&binding, 0, sizeof(binding));
        binding.obValue = offsetof(struct bookmark_data, val);
        binding.obLength = offsetof(struct bookmark_data, len);
        binding.obStatus = offsetof(struct bookmark_data, status);
        binding.dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
        binding.cbMaxLen = rowset->bookmark_type == DBTYPE_I4 ? sizeof(int) : sizeof(void *);
        binding.wType = rowset->bookmark_type;

        hr = IAccessor_CreateAccessor(rowset->accessor, DBACCESSOR_ROWDATA,
                1, &binding, 0, &rowset->bookmark_hacc, NULL);
        if (FAILED(hr)) return hr;
    }

    hr = IRowset_GetData(rowset->rowset, hrow, rowset->bookmark_hacc, &bookmark_data);
    if (FAILED(hr)) return hr;

    if (rowset->bookmark_type == DBTYPE_I4)
    {
        *size = sizeof(int);
        memcpy(buf, &bookmark_data.val.i4, *size);
        *bookmark = buf;
    }
    else if (rowset->bookmark_type == DBTYPE_I8)
    {
        *size = sizeof(long long);
        memcpy(buf, &bookmark_data.val.i8, *size);
        *bookmark = buf;
    }
    else
    {
        *size = bookmark_data.len;
        *bookmark = bookmark_data.val.ptr;
    }
    return S_OK;
}

static HRESULT WINAPI rowsetex_GetExactPosition(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBBKMARK bookmark_size, const BYTE *bookmark, DBCOUNTITEM *position, DBCOUNTITEM *rows)
{
    struct rowsetex *rowset = impl_from_IRowsetExactScroll(iface);
    BYTE tmp[sizeof(long long)], *bm = tmp;
    DBCOUNTITEM got_rows, n = 0;
    HROW hrow[64], *prow = hrow;
    DBBKMARK size;
    HRESULT hr;

    TRACE("%p, %Id, %Iu, %p, %p, %p\n", rowset, chapter, bookmark_size, bookmark, position, rows);

    if (init_rowset_es(rowset))
    {
        return IRowsetExactScroll_GetExactPosition(rowset->rowset_es,
                chapter, bookmark_size, bookmark, position, rows);
    }

    if (bookmark_size || bookmark || position) return E_INVALIDARG;
    if (!rows) return S_OK;

    size = 1;
    tmp[0] = DBBMK_FIRST;
    n = 0;
    while (1)
    {
        hr = IRowsetLocate_GetRowsAt(rowset->rowset_loc, 0, chapter, size, bm,
                n ? 1 : 0, ARRAY_SIZE(hrow), &got_rows, &prow);
        if (bm != tmp) CoTaskMemFree(bm);
        if (FAILED(hr)) return hr;
        n += got_rows;

        if (hr != DB_S_ENDOFROWSET && got_rows)
        {
            hr = get_bookmark(rowset, hrow[got_rows - 1], &size, &bm, tmp);
            if (FAILED(hr))
            {
                IRowset_ReleaseRows(rowset->rowset, got_rows, hrow, NULL, NULL, NULL);
                return hr;
            }
        }
        IRowset_ReleaseRows(rowset->rowset, got_rows, hrow, NULL, NULL, NULL);
        if (hr == DB_S_ENDOFROWSET)
        {
            *rows = n;
            return S_OK;
        }
        if (!got_rows) return E_FAIL;
    }
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

static HRESULT WINAPI accessor_QueryInterface(IAccessor *iface, REFIID riid, void **obj)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, obj);
}

static ULONG WINAPI accessor_AddRef(IAccessor *iface)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG WINAPI accessor_Release(IAccessor *iface)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT WINAPI accessor_AddRefAccessor(IAccessor *iface, HACCESSOR hacc, DBREFCOUNT *refs)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);

    TRACE("%p, %Id, %p\n", rowset, hacc, refs);
    return IAccessor_AddRefAccessor(rowset->accessor, hacc, refs);
}

static HRESULT WINAPI accessor_CreateAccessor(IAccessor *iface, DBACCESSORFLAGS flags,
        DBCOUNTITEM bindings_no, const DBBINDING bindings[], DBLENGTH row_size,
        HACCESSOR *hacc, DBBINDSTATUS status[])
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);

    /* FIXME: native modifies bindings and wraps returned accessor */
    TRACE("%p, %ld, %Id, %p, %Id, %p, %p\n", rowset, flags, bindings_no,
            bindings, row_size, hacc, status);
    return IAccessor_CreateAccessor(rowset->accessor, flags,
            bindings_no, bindings, row_size, hacc, status);
}

static HRESULT WINAPI accessor_GetBindings(IAccessor *iface, HACCESSOR hacc,
        DBACCESSORFLAGS *flags, DBCOUNTITEM *bindings_no, DBBINDING **bindings)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);

    TRACE("%p, %Id, %p, %p, %p\n", rowset, hacc, flags, bindings_no, bindings);
    return IAccessor_GetBindings(rowset->accessor, hacc, flags, bindings_no, bindings);
}

static HRESULT WINAPI accessor_ReleaseAccessor(IAccessor *iface, HACCESSOR hacc, DBREFCOUNT *refs)
{
    struct rowsetex *rowset = impl_from_IAccessor(iface);

    TRACE("%p, %Id, %p\n", rowset, hacc, refs);
    return IAccessor_ReleaseAccessor(rowset->accessor, hacc, refs);
}

static const struct IAccessorVtbl accessor_vtbl =
{
    accessor_QueryInterface,
    accessor_AddRef,
    accessor_Release,
    accessor_AddRefAccessor,
    accessor_CreateAccessor,
    accessor_GetBindings,
    accessor_ReleaseAccessor
};

static HRESULT WINAPI find_QueryInterface(IRowsetFind *iface, REFIID riid, void **obj)
{
    struct rowsetex *rowset = impl_from_IRowsetFind(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, obj);
}

static ULONG WINAPI find_AddRef(IRowsetFind *iface)
{
    struct rowsetex *rowset = impl_from_IRowsetFind(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG WINAPI find_Release(IRowsetFind *iface)
{
    struct rowsetex *rowset = impl_from_IRowsetFind(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT int_compare(DBCOMPAREOP compare_op, long long x, long long y)
{
    switch (compare_op)
    {
    case DBCOMPAREOPS_LT:
        return x < y ? S_OK : S_FALSE;
    case DBCOMPAREOPS_LE:
        return x <= y ? S_OK : S_FALSE;
    case DBCOMPAREOPS_EQ:
        return x == y ? S_OK : S_FALSE;
    case DBCOMPAREOPS_GE:
        return x >= y ? S_OK : S_FALSE;
    case DBCOMPAREOPS_GT:
        return x > y ? S_OK : S_FALSE;
    case DBCOMPAREOPS_NE:
        return x != y ? S_OK : S_FALSE;
    default:
        return S_FALSE;
    }
}

static HRESULT str_compare(DBCOMPAREOP compare_op, BSTR x, BSTR y)
{
    HRESULT hr;

    if (compare_op == DBCOMPAREOPS_BEGINSWITH)
    {
        unsigned int x_len = SysStringLen(x);
        unsigned int y_len = SysStringLen(y);

        if (x_len < y_len) return S_FALSE;
        hr = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, x, y_len, y, y_len);
        return hr == CSTR_EQUAL ? S_OK : S_FALSE;
    }

    if (compare_op == DBCOMPAREOPS_CONTAINS)
    {
        unsigned int x_len = SysStringLen(x);
        unsigned int y_len = SysStringLen(y);
        int i;

        if (x_len < y_len) return S_FALSE;
        for (i = 0; i <= x_len - y_len; i++)
        {
            hr = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, x + i, y_len, y, y_len);
            if (hr == CSTR_EQUAL) return S_OK;
        }
        return S_FALSE;
    }

    hr = VarBstrCmp(x, y, LOCALE_USER_DEFAULT, NORM_IGNORECASE);
    switch (compare_op)
    {
    case DBCOMPAREOPS_LT:
        return hr == VARCMP_LT ? S_OK : S_FALSE;
    case DBCOMPAREOPS_LE:
        return hr != VARCMP_GT ? S_OK : S_FALSE;
    case DBCOMPAREOPS_EQ:
        return hr == VARCMP_EQ ? S_OK : S_FALSE;
    case DBCOMPAREOPS_GE:
        return hr != VARCMP_LT ? S_OK : S_FALSE;
    case DBCOMPAREOPS_GT:
        return hr == VARCMP_GT ? S_OK : S_FALSE;
    case DBCOMPAREOPS_NE:
        return hr != VARCMP_EQ ? S_OK : S_FALSE;
    }
    return S_FALSE;
}

static HRESULT WINAPI find_FindNextRow(IRowsetFind *iface, HCHAPTER chapter, HACCESSOR hacc,
        void *find_value, DBCOMPAREOP compare_op, DBBKMARK bookmark_size, const BYTE *bookmark,
        DBROWOFFSET offset, DBROWCOUNT rows, DBCOUNTITEM *obtained, HROW **hrows)
{
    BYTE tmp[sizeof(long long)], *bm = (BYTE *)bookmark, *data_buf = NULL;
    struct rowsetex *rowset = impl_from_IRowsetFind(iface);
    DWORD status = DBSTATUS_S_OK;
    DBBINDING *bindings = NULL;
    DBCOUNTITEM bindings_no;
    HROW row, *prow = &row;
    DBACCESSORFLAGS flags;
    BOOL free_val = FALSE;
    DBCOUNTITEM count = 0;
    size_t data_size;
    VARIANT conv;
    HRESULT hr;

    TRACE("%p, %Id, %Id, %p, %ld, %Id, %p, %Id, %Id, %p, %p\n", rowset, chapter, hacc,
            find_value, compare_op, bookmark_size, bookmark, offset, rows, obtained, hrows);

    if (!rowset->rowset_find)
    {
        HRESULT hr = IRowset_QueryInterface(rowset->rowset,
                &IID_IRowsetFind, (void**)&rowset->rowset_find);
        if (FAILED(hr))
            rowset->rowset_find = NO_INTERFACE;
    }

    if (rowset->rowset_find != NO_INTERFACE)
    {
        return IRowsetFind_FindNextRow(rowset->rowset_find, chapter, hacc, find_value,
                compare_op, bookmark_size, bookmark, offset, rows, obtained, hrows);
    }

    if (!obtained || !hrows) return E_INVALIDARG;
    if (rows != -1 && rows != 1)
    {
        FIXME("rows = %Id\n", rows);
        return E_NOTIMPL;
    }

    if (!hacc || !rowset->accessor)
        return DB_E_BADACCESSORHANDLE;
    /* TODO: Use custom HACCESSOR instead of calling IAccessor::GetBinding. */
    hr = IAccessor_GetBindings(rowset->accessor, hacc, &flags, &bindings_no, &bindings);
    if (FAILED(hr)) return hr;
    VariantInit(&conv);
    if (bindings_no != 1 || !(flags & DBACCESSOR_ROWDATA) || !(bindings->dwPart & DBPART_VALUE))
    {
        hr = DB_E_BADACCESSORTYPE;
        goto done;
    }

    if (bindings->dwPart & DBPART_STATUS)
        status = *(DWORD *)((BYTE *)find_value + bindings->obStatus);
    if (status != DBSTATUS_S_OK)
    {
        FIXME("unhandled pattern status: %ld\n", status);
        hr = E_NOTIMPL;
        goto done;
    }

    data_size = bindings->obValue + bindings->cbMaxLen;
    if (bindings->dwPart & DBPART_LENGTH && bindings->obLength + sizeof(DBLENGTH) > data_size)
        data_size = bindings->obLength + sizeof(DBLENGTH);
    if (bindings->dwPart & DBPART_STATUS && bindings->obStatus + sizeof(DWORD) > data_size)
        data_size = bindings->obStatus + sizeof(DWORD);
    data_buf = malloc(data_size);
    if (!data_buf)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    /* Non DBBMK_FIRST bookmark is ignored if IRowsetLocate is not implemented */
    if (!rowset->rowset_loc && bookmark_size == 1 && *bookmark == DBBMK_FIRST)
    {
        hr = IRowset_RestartPosition(rowset->rowset, chapter);
        if (FAILED(hr)) goto done;
    }
    if (!rowset->rowset_loc) bookmark_size = 0;

    compare_op &= ~(DBCOMPAREOPS_CASESENSITIVE | DBCOMPAREOPS_CASEINSENSITIVE);

    while (1)
    {
        BYTE *x = data_buf + bindings->obValue;
        BYTE *y = (BYTE *)find_value + bindings->obValue;
        DBTYPE type = bindings->wType;

        if (bookmark_size)
        {
            hr = IRowsetLocate_GetRowsAt(rowset->rowset_loc, 0, chapter,
                    bookmark_size, bm, offset, rows, &count, &prow);
            if (bm != bookmark && bm != tmp)
            {
                CoTaskMemFree(bm);
                bm = NULL;
            }
        }
        else
        {
            hr = IRowset_GetNextRows(rowset->rowset, chapter, offset, rows, &count, &prow);
        }
        if (FAILED(hr)) goto done;
        if (!count) break;
        offset = bookmark_size ? 1 : 0;

        hr = IRowset_GetData(rowset->rowset, row, hacc, data_buf);
        if (FAILED(hr)) goto done;
        free_val = TRUE;
        if (compare_op == DBCOMPAREOPS_IGNORE) break;

        if (bindings->dwPart & DBPART_STATUS)
            status = *(DWORD *)(data_buf + bindings->obStatus);
        if (status == DBSTATUS_S_ISNULL)
            type = DBTYPE_NULL;
        else if (status != DBSTATUS_S_OK)
        {
            FIXME("unhandled status: %ld\n", status);
            hr = E_NOTIMPL;
            goto done;
        }

        if (type == DBTYPE_VARIANT)
        {
            VARIANT *xv = (VARIANT *)x;
            VARIANT *yv = (VARIANT *)y;

            type = V_VT(xv);
            if (type != V_VT(yv))
            {
                if (type != V_VT(&conv))
                {
                    VariantClear(&conv);
                    hr = VariantChangeType(&conv, yv, 0, type);
                    if (FAILED(hr)) goto done;
                }
                yv = &conv;
            }

            x = &V_UI1(xv);
            y = &V_UI1(yv);
        }

        switch (type)
        {
        case DBTYPE_I1:
            hr = int_compare(compare_op, *(char *)x, *(char *)y);
            break;
        case DBTYPE_I2:
            hr = int_compare(compare_op, *(short *)x, *(short *)y);
            break;
        case DBTYPE_I4:
            hr = int_compare(compare_op, *(int *)x, *(int *)y);
            break;
        case DBTYPE_I8:
            hr = int_compare(compare_op, *(long long*)x, *(long long*)y);
            break;
        case DBTYPE_BSTR:
            hr = str_compare(compare_op, *(BSTR *)x, *(BSTR *)y);
            break;
        default:
            FIXME("unhandled data type: %d\n", type);
            hr = E_NOTIMPL;
            break;
        }

        if (hr != S_FALSE) break;

        if (bookmark_size)
        {
            hr = get_bookmark(rowset, row, &bookmark_size, &bm, tmp);
            if (FAILED(hr)) goto done;
        }

        if (bindings->dwMemOwner == DBMEMOWNER_CLIENTOWNED)
            dbtype_free(bindings->wType, data_buf + bindings->obValue);
        free_val = FALSE;
        IRowset_ReleaseRows(rowset->rowset, count, prow, NULL, NULL, NULL);
        count = 0;
    }

done:
    if (free_val && bindings->dwMemOwner == DBMEMOWNER_CLIENTOWNED)
        dbtype_free(bindings->wType, data_buf + bindings->obValue);
    if (bm != bookmark && bm != tmp) CoTaskMemFree(bm);
    VariantClear(&conv);
    CoTaskMemFree(bindings);
    free(data_buf);

    if (SUCCEEDED(hr))
    {
        if (count && !*hrows)
        {
            *hrows = CoTaskMemAlloc(sizeof(**hrows));
            if (*hrows) hr = E_OUTOFMEMORY;
        }
        if (count && *hrows)
        {
            IRowset_AddRefRows(rowset->rowset, 1, &row, NULL, NULL);
            *hrows[0] = row;
        }
    }
    if (count) IRowset_ReleaseRows(rowset->rowset, count, prow, NULL, NULL, NULL);
    if (FAILED(hr)) return hr;

    *obtained = count;
    return count ? hr : DB_S_ENDOFROWSET;
}

static const struct IRowsetFindVtbl find_vtbl =
{
    find_QueryInterface,
    find_AddRef,
    find_Release,
    find_FindNextRow
};

HRESULT create_rowsetex(IUnknown *rowset, IUnknown **ret)
{
    struct rowsetex *rowsetex;
    HRESULT hr;

    rowsetex = calloc(1, sizeof(*rowsetex));
    if (!rowsetex) return E_OUTOFMEMORY;

    rowsetex->IRowsetExactScroll_iface.lpVtbl = &rowsetex_vtbl;
    rowsetex->IAccessor_iface.lpVtbl = &accessor_vtbl;
    rowsetex->IRowsetFind_iface.lpVtbl = &find_vtbl;
    rowsetex->refs = 1;

    hr = IUnknown_QueryInterface(rowset, &IID_IRowset, (void **)&rowsetex->rowset);
    if (FAILED(hr))
    {
        IRowsetExactScroll_Release(&rowsetex->IRowsetExactScroll_iface);
        return MAKE_ADO_HRESULT(adErrFeatureNotAvailable);
    }

    hr = IUnknown_QueryInterface(rowset, &IID_IRowsetLocate, (void **)&rowsetex->rowset_loc);
    if (SUCCEEDED(hr))
    {
        DBCOLUMNINFO *colinfo = NULL;
        OLECHAR *strbuf = NULL;
        DBORDINAL i, columns;
        IColumnsInfo *info;

        hr = IUnknown_QueryInterface(rowset, &IID_IAccessor, (void **)&rowsetex->accessor);
        if (FAILED(hr))
        {
            IRowsetExactScroll_Release(&rowsetex->IRowsetExactScroll_iface);
            return MAKE_ADO_HRESULT(adErrFeatureNotAvailable);
        }

        hr = IUnknown_QueryInterface(rowset, &IID_IColumnsInfo, (void **)&info);
        if (FAILED(hr))
        {
            IRowsetExactScroll_Release(&rowsetex->IRowsetExactScroll_iface);
            return MAKE_ADO_HRESULT(adErrFeatureNotAvailable);
        }

        hr = IColumnsInfo_GetColumnInfo(info, &columns, &colinfo, &strbuf);
        IColumnsInfo_Release(info);
        if (FAILED(hr))
        {
            IRowsetExactScroll_Release(&rowsetex->IRowsetExactScroll_iface);
            return MAKE_ADO_HRESULT(adErrFeatureNotAvailable);
        }

        for (i = 0; i < columns; i++)
        {
            if (colinfo[i].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK)
            {
                if (colinfo[i].ulColumnSize == sizeof(int))
                    rowsetex->bookmark_type = DBTYPE_I4;
                else if (colinfo[i].ulColumnSize == sizeof(INT_PTR))
                    rowsetex->bookmark_type = DBTYPE_I8;
                else
                    rowsetex->bookmark_type = DBTYPE_BYREF | DBTYPE_BYTES;
            }
        }
        CoTaskMemFree(strbuf);
        CoTaskMemFree(colinfo);
    }

    *ret = (IUnknown *)&rowsetex->IRowsetExactScroll_iface;
    return S_OK;
}
