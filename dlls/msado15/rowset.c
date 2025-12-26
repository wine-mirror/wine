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
#include "unknwn.h"

#include "initguid.h"
#include "msdadc.h"
#include "msdaguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct data
{
    DBSTATUS status;
    DBLENGTH length;
    BYTE data[1];
};

struct rowset
{
    IRowsetExactScroll IRowsetExactScroll_iface;
    IColumnsInfo IColumnsInfo_iface;
    IRowsetChange IRowsetChange_iface;
    IAccessor IAccessor_iface;
    IRowsetInfo IRowsetInfo_iface;
    LONG refs;

    IDataConvert *convert;

    int columns_cnt;
    DBCOLUMNINFO *columns;
    OLECHAR *columns_buf;

    BOOL backward_fetch;
    int index;
    int row_cnt;

    int rows_alloc;
    int row_size;
    int *column_off;
    BYTE *data;
};

struct accessor
{
    LONG refs;
    int bindings_count;
    DBBINDING bindings[1];
};

static void dbtype_free(DBTYPE type, void *data)
{
    if (type & DBTYPE_BYREF)
    {
        void *p = *(void **)data;

        if (p)
        {
            dbtype_free(type & ~DBTYPE_BYREF, p);
            free(p);
        }
        return;
    }
    if (type & DBTYPE_ARRAY)
    {
        SAFEARRAY *sa = *(SAFEARRAY **)data;
        SafeArrayDestroy(sa);
        return;
    }
    if (type & DBTYPE_VECTOR)
    {
        DBVECTOR *vec = *(DBVECTOR **)data;
        CoTaskMemFree(vec->ptr);
        return;
    }

    switch (type)
    {
    case DBTYPE_BSTR: SysFreeString(*(BSTR *)data); break;
    case DBTYPE_IDISPATCH: IDispatch_Release(*(IDispatch **)data); break;
    case DBTYPE_VARIANT: VariantClear((VARIANT *)data); break;
    case DBTYPE_IUNKNOWN: IUnknown_Release(*(IUnknown **)data); break;
    }
}

static int dbtype_size(DBTYPE type, DBLENGTH max_len)
{
    if (type & DBTYPE_BYREF) return sizeof(void*);
    if (type & DBTYPE_ARRAY) return sizeof(void*);
    if (type & DBTYPE_VECTOR) return sizeof(DBVECTOR);

    switch (type)
    {
    case DBTYPE_EMPTY: return 0;
    case DBTYPE_NULL: return 0;
    case DBTYPE_I2: return sizeof(short);
    case DBTYPE_I4: return sizeof(int);
    case DBTYPE_R4: return sizeof(float);
    case DBTYPE_R8: return sizeof(double);
    case DBTYPE_CY: return sizeof(CY);
    case DBTYPE_DATE: return sizeof(DATE);
    case DBTYPE_BSTR: return sizeof(BSTR);
    case DBTYPE_IDISPATCH: return sizeof(IDispatch*);
    case DBTYPE_ERROR: return sizeof(SCODE);
    case DBTYPE_BOOL: return sizeof(VARIANT_BOOL);
    case DBTYPE_VARIANT: return sizeof(VARIANT);
    case DBTYPE_IUNKNOWN: return sizeof(IUnknown*);
    case DBTYPE_DECIMAL: return sizeof(DECIMAL);
    case DBTYPE_I1: return sizeof(char);
    case DBTYPE_UI1: return sizeof(char);
    case DBTYPE_UI2: return sizeof(short);
    case DBTYPE_UI4: return sizeof(int);
    case DBTYPE_I8: return sizeof(LONGLONG);
    case DBTYPE_UI8: return sizeof(LONGLONG);
    case DBTYPE_GUID: return sizeof(GUID);
    case DBTYPE_BYTES: return sizeof(BYTE) * max_len;
    case DBTYPE_STR: return sizeof(CHAR) * max_len;
    case DBTYPE_WSTR: return sizeof(WCHAR) * max_len;
    case DBTYPE_NUMERIC: return sizeof(DB_NUMERIC);
    case DBTYPE_DBDATE: return sizeof(DBDATE);
    case DBTYPE_DBTIME: return sizeof(DBTIME);
    case DBTYPE_DBTIMESTAMP: return sizeof(DBTIMESTAMP);
    }

    FIXME("unsupported type: %x\n", type);
    return 0;
}

static inline struct rowset *impl_from_IRowsetExactScroll(IRowsetExactScroll *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IRowsetExactScroll_iface);
}

static inline struct rowset *impl_from_IColumnsInfo(IColumnsInfo *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IColumnsInfo_iface);
}

static inline struct rowset *impl_from_IRowsetChange(IRowsetChange *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IRowsetChange_iface);
}

static inline struct rowset *impl_from_IAccessor(IAccessor *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IAccessor_iface);
}

static inline struct rowset *impl_from_IRowsetInfo(IRowsetInfo *iface)
{
    return CONTAINING_RECORD(iface, struct rowset, IRowsetInfo_iface);
}

static HRESULT WINAPI rowset_QueryInterface(IRowsetExactScroll *iface, REFIID riid, void **ppv)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %s, %p\n", rowset, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IRowset, riid) ||
       IsEqualGUID(&IID_IRowsetLocate, riid) ||
       IsEqualGUID(&IID_IRowsetScroll, riid) ||
       IsEqualGUID(&IID_IRowsetExactScroll, riid))
    {
        *ppv = &rowset->IRowsetExactScroll_iface;
    }
    else if(IsEqualGUID(&IID_IColumnsInfo, riid))
    {
        *ppv = &rowset->IColumnsInfo_iface;
    }
    else if(IsEqualGUID(&IID_IRowsetChange, riid))
    {
        *ppv = &rowset->IRowsetChange_iface;
    }
    else if(IsEqualGUID(&IID_IAccessor, riid))
    {
        *ppv = &rowset->IAccessor_iface;
    }
    else if(IsEqualGUID(&IID_IRowsetInfo, riid))
    {
        *ppv = &rowset->IRowsetInfo_iface;
    }

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI rowset_AddRef(IRowsetExactScroll *iface)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);
    LONG refs = InterlockedIncrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    return refs;
}

static ULONG WINAPI rowset_Release(IRowsetExactScroll *iface)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);
    LONG refs = InterlockedDecrement(&rowset->refs);

    TRACE("%p new refcount %ld\n", rowset, refs);

    if (!refs)
    {
        int i, j;

        TRACE("destroying %p\n", rowset);

        if (rowset->convert) IDataConvert_Release(rowset->convert);

        for (i = 0; i < rowset->row_cnt; i++)
        {
            for (j = 0; j < rowset->columns_cnt; j++)
            {
                struct data *data = (struct data *)(rowset->data +
                        i * rowset->row_size + rowset->column_off[j]);
                dbtype_free(rowset->columns[j].wType, data->data);
            }
        }
        free(rowset->data);

        CoTaskMemFree(rowset->columns);
        CoTaskMemFree(rowset->columns_buf);
        free(rowset->column_off);
        free(rowset);
    }
    return refs;
}

static HRESULT WINAPI rowset_AddRefRows(IRowsetExactScroll *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %p, %p, %p\n", rowset, count, rows, ref_counts, status);
    if (ref_counts || status)
    {
        FIXME("unhandled parameters\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI rowset_GetData(IRowsetExactScroll *iface, HROW row, HACCESSOR hacc, void *data)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);
    struct accessor *accessor = (struct accessor *)hacc;
    BOOL succ = FALSE, err = FALSE;
    DBSTATUS status;
    DBLENGTH len;
    HRESULT hr;
    int i;

    TRACE("%p, %Id, %Id, %p\n", rowset, row, hacc, data);

    if (!accessor->bindings_count) return DB_E_BADACCESSORTYPE;
    if (row < 1 || row > rowset->row_cnt) return DB_E_BADROWHANDLE;
    for (i = 0; i < accessor->bindings_count; i++)
    {
        if (accessor->bindings[i].dwMemOwner != DBMEMOWNER_CLIENTOWNED)
        {
            FIXME("dwMemOwner = %lx\n", accessor->bindings[i].dwMemOwner);
            return E_NOTIMPL;
        }
    }

    for (i = 0; i < accessor->bindings_count; i++)
    {
        int col = accessor->bindings[i].iOrdinal;
        struct data *val = (struct data *)(rowset->data +
                (row - 1) * rowset->row_size + rowset->column_off[col]);

        status = DBSTATUS_S_OK;
        len = 0;

        if (!(accessor->bindings[i].dwPart & DBPART_VALUE))
            status = DBSTATUS_E_BADACCESSOR;
        else if (accessor->bindings[i].wType == DBTYPE_VARIANT && val->status == DBSTATUS_E_UNAVAILABLE)
        {
            if (accessor->bindings[i].cbMaxLen < sizeof(VARIANT))
                status = DBSTATUS_E_BADACCESSOR;
            else
                VariantInit((VARIANT *)val->data);
        }
        else
        {
            hr = IDataConvert_DataConvert(rowset->convert, rowset->columns[col].wType,
                    accessor->bindings[i].wType, val->length, &len,
                    val->data, (BYTE *)data + accessor->bindings[i].obValue,
                    accessor->bindings[i].cbMaxLen, val->status, &status,
                    accessor->bindings[i].bPrecision, accessor->bindings[i].bScale, 0);
            if (FAILED(hr))
                WARN("data conversion failed\n");
        }

        if (accessor->bindings[i].dwPart & DBPART_LENGTH)
            memcpy((BYTE *)data + accessor->bindings[i].obLength, &len, sizeof(len));
        if (accessor->bindings[i].dwPart & DBPART_STATUS)
            memcpy((BYTE *)data + accessor->bindings[i].obStatus, &status, sizeof(status));

        if (status == DBSTATUS_S_OK || status == DBSTATUS_S_ISNULL ||
                status == DBSTATUS_S_TRUNCATED || status == DBSTATUS_S_DEFAULT)
            succ = TRUE;
        else err = TRUE;
    }

    if (!succ) return DB_E_ERRORSOCCURRED;
    return err ? DB_S_ERRORSOCCURRED : S_OK;
}

static HRESULT WINAPI rowset_GetNextRows(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBROWOFFSET offset, DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll( iface );
    int i, row_count;

    TRACE("%p, %Id, %Id, %Id, %p, %p\n", rowset, chapter, offset, count, obtained, rows);

    if (!obtained || !rows) return E_INVALIDARG;
    *obtained = 0;

    if (chapter != DB_NULL_HCHAPTER)
    {
        FIXME("chapter = %Id\n", chapter);
        return E_NOTIMPL;
    }

    if (rowset->backward_fetch) offset = -offset;
    if (rowset->index + offset < 0 || rowset->index + offset > rowset->row_cnt)
        return DB_E_BADSTARTPOSITION;

    if (count > 0) row_count = min(rowset->row_cnt - rowset->index - offset, count);
    else row_count = min(rowset->index + offset, -count);

    /* don't apply offset if no rows were fetched */
    if (!row_count) return count ? DB_S_ENDOFROWSET : S_OK;

    if (!*rows)
    {
        *rows = CoTaskMemAlloc(sizeof(**rows) * row_count);
        if (!*rows) return E_OUTOFMEMORY;
    }

    rowset->index += offset;
    for (i = 0; i < row_count; i++)
    {
        if (count < 0) rowset->index--;
        (*rows)[i] = rowset->index + 1;
        if (count > 0) rowset->index++;
    }

    *obtained = row_count;
    rowset->backward_fetch = (count < 0);
    return row_count == count || row_count == -count ? S_OK : DB_S_ENDOFROWSET;
}

static HRESULT WINAPI rowset_ReleaseRows(IRowsetExactScroll *iface, DBCOUNTITEM count, const HROW rows[],
        DBROWOPTIONS options[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %p, %p, %p, %p\n", rowset, count, rows, options, ref_counts, status);

    if (options || ref_counts || status)
    {
        FIXME("unhandled parameters\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI rowset_RestartPosition(IRowsetExactScroll *iface, HCHAPTER reserved)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id\n", rowset, reserved);

    rowset->index = 0;
    return S_OK;
}

static HRESULT WINAPI rowset_Compare(IRowsetExactScroll *iface, HCHAPTER hReserved, DBBKMARK cbBookmark1,
        const BYTE *pBookmark1, DBBKMARK cbBookmark2, const BYTE *pBookmark2, DBCOMPARE *pComparison)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Iu, %p, %Iu, %p, %p\n", rowset, hReserved, cbBookmark1,
            pBookmark1, cbBookmark2, pBookmark2, pComparison);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetRowsAt(IRowsetExactScroll *iface, HWATCHREGION watchregion,
        HCHAPTER chapter, DBBKMARK bookmark_size, const BYTE *bookmark, DBROWOFFSET offset,
        DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);
    int idx = -1, i, row_count;

    TRACE("%p, %Id, %Id, %Iu, %p, %Id, %Id, %p, %p\n", rowset, watchregion, chapter,
            bookmark_size, bookmark, offset, count, obtained, rows);

    if (!bookmark_size || !bookmark || !obtained || !rows)
        return E_INVALIDARG;
    *obtained = 0;

    if (watchregion || chapter)
    {
        FIXME("unsupported arguments\n");
        return E_NOTIMPL;
    }

    if (bookmark_size == 1 && bookmark[0] == DBBMK_FIRST)
        idx = 0;
    else if (bookmark_size == 1 && bookmark[0] == DBBMK_LAST)
        idx = rowset->row_cnt ? rowset->row_cnt - 1 : 0;
    else if (bookmark_size == sizeof(int))
        idx = *(int*)bookmark - 1;

    if (idx < 0 || (idx && idx >= rowset->row_cnt)) return DB_E_BADBOOKMARK;

    idx += offset;
    if (idx < 0 || (idx && idx >= rowset->row_cnt)) return DB_E_BADSTARTPOSITION;

    if (count > 0) row_count = min(rowset->row_cnt - idx, count);
    else if (!rowset->row_cnt) row_count = 0;
    else row_count = min(idx + 1, -count);
    if (!row_count) return count ? DB_S_ENDOFROWSET : S_OK;

    if (!*rows)
    {
        *rows = CoTaskMemAlloc(sizeof(**rows) * row_count);
        if (!*rows) return E_OUTOFMEMORY;
    }

    for (i = 0; i < row_count; i++)
    {
        (*rows)[i] = idx + 1;
        idx += (count > 0 ? 1 : -1);
    }

    *obtained = row_count;
    return row_count == count || row_count == -count ? S_OK : DB_S_ENDOFROWSET;
}

static HRESULT WINAPI rowset_GetRowsByBookmark(IRowsetExactScroll *iface, HCHAPTER hReserved,
        DBCOUNTITEM cRows, const DBBKMARK rgcbBookmarks[], const BYTE * rgpBookmarks[],
        HROW rghRows[], DBROWSTATUS rgRowStatus[])
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Id, %p, %p, %p, %p\n", rowset, hReserved, cRows,
            rgcbBookmarks, rgpBookmarks, rghRows, rgRowStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_Hash(IRowsetExactScroll *iface, HCHAPTER hReserved,
        DBBKMARK cBookmarks, const DBBKMARK rgcbBookmarks[], const BYTE * rgpBookmarks[],
        DBHASHVALUE rgHashedValues[], DBROWSTATUS rgBookmarkStatus[])
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Iu, %p, %p, %p, %p\n", rowset, hReserved, cBookmarks,
            rgcbBookmarks, rgpBookmarks, rgHashedValues, rgBookmarkStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetApproximatePosition(IRowsetExactScroll *iface, HCHAPTER reserved,
        DBBKMARK cnt, const BYTE *bookmarks, DBCOUNTITEM *position, DBCOUNTITEM *rows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Iu, %p, %p, %p\n", rowset, reserved, cnt, bookmarks, position, rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetRowsAtRatio(IRowsetExactScroll *iface, HWATCHREGION reserved1, HCHAPTER reserved2,
        DBCOUNTITEM numerator, DBCOUNTITEM Denominator, DBROWCOUNT rows_cnt, DBCOUNTITEM *obtained, HROW **rows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Id, %Iu, %Iu, %Id, %p, %p\n", rowset, reserved1, reserved2,
            numerator, Denominator, rows_cnt, obtained, rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetExactPosition(IRowsetExactScroll *iface, HCHAPTER chapter,
        DBBKMARK bookmark_cnt, const BYTE *bookmarks, DBCOUNTITEM *position, DBCOUNTITEM *rows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    TRACE("%p, %Id, %Iu, %p, %p, %p\n", rowset, chapter, bookmark_cnt, bookmarks, position, rows);

    if (position) FIXME("not setting position\n");
    if (rows) *rows = rowset->row_cnt;
    return S_OK;
}

static const struct IRowsetExactScrollVtbl rowset_vtbl =
{
    rowset_QueryInterface,
    rowset_AddRef,
    rowset_Release,
    rowset_AddRefRows,
    rowset_GetData,
    rowset_GetNextRows,
    rowset_ReleaseRows,
    rowset_RestartPosition,
    rowset_Compare,
    rowset_GetRowsAt,
    rowset_GetRowsByBookmark,
    rowset_Hash,
    rowset_GetApproximatePosition,
    rowset_GetRowsAtRatio,
    rowset_GetExactPosition
};

static HRESULT WINAPI columns_info_QueryInterface(IColumnsInfo *iface, REFIID riid, void **out)
{
    struct rowset *rowset = impl_from_IColumnsInfo(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, out);
}

static ULONG WINAPI columns_info_AddRef(IColumnsInfo *iface)
{
    struct rowset *rowset = impl_from_IColumnsInfo(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG  WINAPI columns_info_Release(IColumnsInfo *iface)
{
    struct rowset *rowset = impl_from_IColumnsInfo(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT copy_column_info(DBCOLUMNINFO **dest, const DBCOLUMNINFO *src, int count, OLECHAR **buf)
{
    size_t len;
    int i;

    *dest = CoTaskMemAlloc(sizeof(**dest) * count);
    if (!*dest)
        return E_OUTOFMEMORY;
    memcpy(*dest, src, sizeof(**dest) * count);

    len = 0;
    for (i = 0; i < count; i++)
    {
        if (src[i].pwszName) len += wcslen(src[i].pwszName) + 1;
        if (src[i].columnid.eKind == DBKIND_GUID_NAME || src[i].columnid.eKind == DBKIND_NAME ||
                src[i].columnid.eKind == DBKIND_PGUID_NAME)
            len += wcslen(src[i].columnid.uName.pwszName) + 1;
    }
    *buf = CoTaskMemAlloc(sizeof(**buf) * len);
    if (!*buf)
    {
        CoTaskMemFree(*dest);
        *dest = NULL;
        return E_OUTOFMEMORY;
    }
    len = 0;
    for (i = 0; i < count; i++)
    {
        if (src[i].pwszName)
        {
            wcscpy(*buf + len, src[i].pwszName);
            (*dest)[i].pwszName = *buf + len;
            len += wcslen(src[i].pwszName) + 1;
        }
        if (src[i].columnid.eKind == DBKIND_GUID_NAME || src[i].columnid.eKind == DBKIND_NAME ||
                src[i].columnid.eKind == DBKIND_PGUID_NAME)
        {
            wcscpy(*buf + len, src[i].columnid.uName.pwszName);
            (*dest)[i].columnid.uName.pwszName = *buf + len;
            len += wcslen(src[i].columnid.uName.pwszName) + 1;
        }
    }
    return S_OK;
}

static HRESULT WINAPI columns_info_GetColumnInfo(IColumnsInfo *iface,
        DBORDINAL *columns, DBCOLUMNINFO **colinfo, OLECHAR **strbuf)
{
    struct rowset *rowset = impl_from_IColumnsInfo(iface);

    TRACE("%p, %p, %p, %p\n", rowset, columns, colinfo, strbuf);

    if (!columns || !colinfo || !strbuf)
        return E_INVALIDARG;

    *columns = rowset->columns_cnt;
    return copy_column_info(colinfo, rowset->columns, rowset->columns_cnt, strbuf);
}

static HRESULT WINAPI columns_info_MapColumnIDs(IColumnsInfo *iface,
        DBORDINAL column_ids, const DBID *dbids, DBORDINAL *columns)
{
    struct rowset *rowset = impl_from_IColumnsInfo(iface);

    FIXME("%p, %Iu, %p, %p\n", rowset, column_ids, dbids, columns);
    return E_NOTIMPL;
}

static struct IColumnsInfoVtbl columns_info_vtbl =
{
    columns_info_QueryInterface,
    columns_info_AddRef,
    columns_info_Release,
    columns_info_GetColumnInfo,
    columns_info_MapColumnIDs
};

static HRESULT WINAPI rowset_change_QueryInterface(IRowsetChange *iface, REFIID riid, void **ppv)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, ppv);
}

static ULONG WINAPI rowset_change_AddRef(IRowsetChange *iface)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG WINAPI rowset_change_Release(IRowsetChange *iface)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT WINAPI rowset_change_DeleteRows(IRowsetChange *iface, HCHAPTER reserved,
        DBCOUNTITEM count, const HROW rows[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);

    FIXME("%p, %Iu, %Iu, %p, %p\n", rowset, reserved, count, rows, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_change_SetData(IRowsetChange *iface, HROW row, HACCESSOR hacc, void *data)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);
    struct accessor *accessor = (struct accessor *)hacc;
    BOOL err = FALSE, succ = FALSE;
    DBSTATUS status;
    DBLENGTH len;
    HRESULT hr;
    int i;

    TRACE("%p, %Id, %Id, %p\n", rowset, row, hacc, data);

    if (!accessor->bindings_count) return DB_E_BADACCESSORTYPE;
    if (row < 1 || row > rowset->row_cnt) return DB_E_BADROWHANDLE;

    for (i = 0; i < accessor->bindings_count; i++)
    {
        int col = accessor->bindings[i].iOrdinal;
        struct data *val = (struct data *)(rowset->data +
                (row - 1) * rowset->row_size + rowset->column_off[col]);
        DBSTATUS src_status = DBSTATUS_S_OK;
        DBLENGTH src_len = 0;

        status = DBSTATUS_S_OK;
        len = 0;

        if (accessor->bindings[i].dwPart & DBPART_LENGTH)
            src_len = *(DBLENGTH *)((BYTE *)data + accessor->bindings[i].obLength);
        if (accessor->bindings[i].dwPart & DBPART_STATUS)
            src_status = *(DBSTATUS *)((BYTE *)data + accessor->bindings[i].obStatus);

        if (!(accessor->bindings[i].dwPart & DBPART_VALUE))
            status = DBSTATUS_E_BADACCESSOR;
        else
        {
            hr = IDataConvert_DataConvert(rowset->convert, accessor->bindings[i].wType,
                    rowset->columns[col].wType, src_len, &len,
                    (BYTE *)data + accessor->bindings[i].obValue, val->data,
                    rowset->columns[col].ulColumnSize, src_status, &status,
                    rowset->columns[col].bPrecision, rowset->columns[col].bScale, 0);
            if (FAILED(hr))
                WARN("data conversion failed\n");
        }

        if (accessor->bindings[i].dwPart & DBPART_LENGTH)
            memcpy((BYTE *)data + accessor->bindings[i].obLength, &len, sizeof(len));
        if (accessor->bindings[i].dwPart & DBPART_STATUS)
            memcpy((BYTE *)data + accessor->bindings[i].obStatus, &status, sizeof(status));

        if (status == DBSTATUS_S_OK || status == DBSTATUS_S_ISNULL ||
                status == DBSTATUS_S_TRUNCATED || status == DBSTATUS_S_DEFAULT)
        {
            val->status = status;
            val->length = len;
            succ = TRUE;
        }
        else err = TRUE;
    }

    if (!succ) return DB_E_ERRORSOCCURRED;
    return err ? DB_S_ERRORSOCCURRED : S_OK;
}

static HRESULT WINAPI rowset_change_InsertRow(IRowsetChange *iface, HCHAPTER reserved,
        HACCESSOR accessor, void *data, HROW *row)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);
    struct data *val;
    HRESULT hr;
    int i;

    TRACE("%p, %Iu, %Id, %p, %p\n", rowset, reserved, accessor, data, row);

    if (rowset->row_cnt == rowset->rows_alloc)
    {
        int rows_alloc = max(8, max(rowset->row_cnt + 1, rowset->rows_alloc * 2));
        BYTE *tmp;

        tmp = realloc(rowset->data, rows_alloc * rowset->row_size);
        if (!tmp) return E_OUTOFMEMORY;

        memset(tmp + rowset->rows_alloc * rowset->row_size, 0,
                (rows_alloc - rowset->rows_alloc) * rowset->row_size);
        rowset->data = tmp;
        rowset->rows_alloc = rows_alloc;
    }

    val = (struct data *)(rowset->data + rowset->row_cnt * rowset->row_size);
    *(int*)val->data = rowset->row_cnt + 1;

    for (i = 1; i < rowset->columns_cnt; i++)
    {
        /* TODO: handle default values */
        val = (struct data *)(rowset->data + rowset->row_cnt * rowset->row_size + rowset->column_off[i]);
        val->status = DBSTATUS_E_UNAVAILABLE;
    }

    if (data)
    {
        hr = rowset_change_SetData(iface, rowset->row_cnt + 1, accessor, data);
        if (FAILED(hr)) return hr;
    }

    rowset->row_cnt++;
    if (row) *row = rowset->row_cnt;
    return S_OK;
}

static struct IRowsetChangeVtbl rowset_change_vtbl =
{
    rowset_change_QueryInterface,
    rowset_change_AddRef,
    rowset_change_Release,
    rowset_change_DeleteRows,
    rowset_change_SetData,
    rowset_change_InsertRow
};

static HRESULT WINAPI accessor_QueryInterface(IAccessor *iface, REFIID riid, void **ppv)
{
    struct rowset *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, ppv);
}

static ULONG WINAPI accessor_AddRef(IAccessor *iface)
{
    struct rowset *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG WINAPI accessor_Release(IAccessor *iface)
{
    struct rowset *rowset = impl_from_IAccessor(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT WINAPI accessor_AddRefAccessor(IAccessor *iface, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
    struct accessor *accessor = (struct accessor *)hAccessor;
    struct rowset *rowset = impl_from_IAccessor(iface);
    LONG ref;

    TRACE("%p, %Id, %p\n", rowset, hAccessor, pcRefCount);

    if (!hAccessor) return DB_E_BADACCESSORHANDLE;

    ref = InterlockedIncrement(&accessor->refs);
    if (pcRefCount) *pcRefCount = ref;
    return S_OK;
}

static HRESULT WINAPI accessor_CreateAccessor(IAccessor *iface, DBACCESSORFLAGS dwAccessorFlags,
        DBCOUNTITEM cBindings, const DBBINDING rgBindings[], DBLENGTH cbRowSize,
        HACCESSOR *phAccessor, DBBINDSTATUS rgStatus[])
{
    struct rowset *rowset = impl_from_IAccessor(iface);
    struct accessor *accessor;
    HRESULT hr, ret = S_OK;
    int i;

    TRACE("%p, %lx, %Iu, %p %Id, %p %p\n", rowset, dwAccessorFlags, cBindings,
            rgBindings, cbRowSize, phAccessor, rgStatus);

    if (!phAccessor) return E_INVALIDARG;
    *phAccessor = 0;

    if (cbRowSize)
    {
        FIXME("cbRowSize not handled\n");
        return E_NOTIMPL;
    }
    if (dwAccessorFlags != DBACCESSOR_ROWDATA)
    {
        FIXME("unsupported flags %lx\n", dwAccessorFlags);
        return E_NOTIMPL;
    }

    for (i = 0; i < cBindings; i++)
    {
        if (rgBindings[i].iOrdinal >= rowset->columns_cnt)
        {
            if (rgStatus) rgStatus[i] = DBBINDSTATUS_BADORDINAL;
            if (ret == S_OK) ret = DBSTATUS_E_BADACCESSOR;
            continue;
        }

        if (rgBindings[i].wType != rowset->columns[rgBindings[i].iOrdinal].wType)
        {
            if (!rowset->convert)
            {
               hr = CoCreateInstance(&CLSID_OLEDB_CONVERSIONLIBRARY, NULL, CLSCTX_INPROC_SERVER,
                       &IID_IDataConvert, (void **)&rowset->convert);
               if (FAILED(hr)) return hr;
            }

            hr = IDataConvert_CanConvert(rowset->convert,
                    rowset->columns[rgBindings[i].iOrdinal].wType, rgBindings[i].wType);
            if (FAILED(hr)) return hr;
            if (hr != S_OK)
            {
                if (rgStatus) rgStatus[i] = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
                if (ret == S_OK) ret = DBSTATUS_E_BADACCESSOR;
                continue;
            }
        }

        if (rgStatus) rgStatus[i] = DBBINDSTATUS_OK;
    }
    if (ret != S_OK) return ret;

    accessor = calloc(1, offsetof(struct accessor, bindings[cBindings]));
    if (!accessor) return E_OUTOFMEMORY;
    accessor->refs = 1;
    accessor->bindings_count = cBindings;
    memcpy(accessor->bindings, rgBindings, sizeof(rgBindings[0]) * cBindings);

    *phAccessor = (HACCESSOR)accessor;
    return S_OK;
}

static HRESULT WINAPI accessor_GetBindings(IAccessor *iface, HACCESSOR hAccessor,
        DBACCESSORFLAGS *pdwAccessorFlags, DBCOUNTITEM *pcBindings, DBBINDING **prgBindings)
{
    struct rowset *rowset = impl_from_IAccessor(iface);

    FIXME("%p, %Id, %p %p %p\n", rowset, hAccessor, pdwAccessorFlags, pcBindings, prgBindings);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessor_ReleaseAccessor(IAccessor *iface,
        HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
    struct accessor *accessor = (struct accessor *)hAccessor;
    struct rowset *rowset = impl_from_IAccessor(iface);
    LONG ref;

    TRACE("%p, %Id, %p\n", rowset, hAccessor, pcRefCount);

    if (!hAccessor) return DB_E_BADACCESSORHANDLE;

    ref = InterlockedDecrement(&accessor->refs);
    if (!ref)
        free(accessor);

    if (pcRefCount) *pcRefCount = ref;
    return S_OK;
}

static struct IAccessorVtbl accessor_vtbl =
{
    accessor_QueryInterface,
    accessor_AddRef,
    accessor_Release,
    accessor_AddRefAccessor,
    accessor_CreateAccessor,
    accessor_GetBindings,
    accessor_ReleaseAccessor
};

static HRESULT WINAPI rowset_info_QueryInterface(IRowsetInfo *iface, REFIID riid, void **ppv)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);
    return IRowsetExactScroll_QueryInterface(&rowset->IRowsetExactScroll_iface, riid, ppv);
}

static ULONG WINAPI rowset_info_AddRef(IRowsetInfo *iface)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);
    return IRowsetExactScroll_AddRef(&rowset->IRowsetExactScroll_iface);
}

static ULONG WINAPI rowset_info_Release(IRowsetInfo *iface)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);
    return IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
}

static HRESULT WINAPI rowset_info_GetProperties(IRowsetInfo *iface, const ULONG propidsets_count,
        const DBPROPIDSET propidsets[], ULONG *count, DBPROPSET **propsets)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);
    BOOL prop_fail = FALSE, prop_succ = FALSE;
    int i, j;

    TRACE("%p, %lu, %p, %p, %p\n", rowset, propidsets_count, propidsets, count, propsets);

    *count = 0;
    *propsets = CoTaskMemAlloc(sizeof(**propsets) * propidsets_count);
    for (i = 0; i < propidsets_count; i++)
    {
        size_t size = sizeof(*(*propsets)[i].rgProperties) * propidsets[i].cPropertyIDs;
        DBPROPSTATUS *status;
        VARIANT *v;

        (*propsets)[i].rgProperties = CoTaskMemAlloc(size);
        memset((*propsets)[i].rgProperties, 0, size);
        (*propsets)[i].cProperties = propidsets[i].cPropertyIDs;
        (*propsets)[i].guidPropertySet = propidsets[i].guidPropertySet;

        for (j = 0; j < propidsets[i].cPropertyIDs; j++)
        {
            (*propsets)[i].rgProperties[j].dwPropertyID = propidsets[i].rgPropertyIDs[j];
            status = &(*propsets)[i].rgProperties[j].dwStatus;
            v = &(*propsets)[i].rgProperties[j].vValue;

            if (IsEqualGUID(&propidsets[i].guidPropertySet, &DBPROPSET_ROWSET))
            {
                switch (propidsets[i].rgPropertyIDs[j])
                {
                case DBPROP_OTHERUPDATEDELETE:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_FALSE;
                    prop_succ = TRUE;
                    break;
                case DBPROP_OTHERINSERT:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_FALSE;
                    break;
                case DBPROP_CANHOLDROWS:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_CANSCROLLBACKWARDS:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_UPDATABILITY:
                    V_VT(v) = VT_I4;
                    V_I4(v) = DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_DELETE | DBPROPVAL_UP_INSERT;
                    break;
                case DBPROP_IRowsetLocate:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_IRowsetScroll:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_IRowsetUpdate:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_BOOKMARKSKIPPED:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                case DBPROP_LOCKMODE:
                    *status = DBPROPSTATUS_NOTSUPPORTED;
                    prop_fail = TRUE;
                    break;
                case DBPROP_IRowsetCurrentIndex:
                    *status = DBPROPSTATUS_NOTSUPPORTED;
                    prop_fail = TRUE;
                    break;
                case DBPROP_REMOVEDELETED:
                    V_VT(v) = VT_BOOL;
                    V_BOOL(v) = VARIANT_TRUE;
                    break;
                default:
                    FIXME("unsupported DBPROPSET_ROWSET property %ld\n",
                            propidsets[i].rgPropertyIDs[j]);
                    *status = DBPROPSTATUS_NOTSUPPORTED;
                    prop_fail = TRUE;
                    break;
                }
            }
            else
            {
                FIXME("unsupported property %s %ld\n",
                        wine_dbgstr_guid(&propidsets[i].guidPropertySet),
                        propidsets[i].rgPropertyIDs[j]);
                *status = DBPROPSTATUS_NOTSUPPORTED;
                prop_fail = TRUE;
            }
        }
    }

    *count = propidsets_count;
    if (!prop_succ) return DB_E_ERRORSOCCURRED;
    return prop_fail ? DB_S_ERRORSOCCURRED : S_OK;
}

static HRESULT WINAPI rowset_info_GetReferencedRowset(IRowsetInfo *iface,
        DBORDINAL iOrdinal, REFIID riid, IUnknown **ppReferencedRowset)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);

    FIXME("%p, %Iu, %s, %p\n", rowset, iOrdinal, wine_dbgstr_guid(riid), ppReferencedRowset);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_info_GetSpecification(IRowsetInfo *iface,
        REFIID riid, IUnknown **ppSpecification)
{
    struct rowset *rowset = impl_from_IRowsetInfo(iface);

    FIXME("%p, %s, %p\n", rowset, wine_dbgstr_guid(riid), ppSpecification);
    return E_NOTIMPL;
}

static struct IRowsetInfoVtbl rowset_info_vtbl =
{
    rowset_info_QueryInterface,
    rowset_info_AddRef,
    rowset_info_Release,
    rowset_info_GetProperties,
    rowset_info_GetReferencedRowset,
    rowset_info_GetSpecification
};

HRESULT create_mem_rowset(int count, const DBCOLUMNINFO *info, IUnknown **ret)
{
    struct rowset *rowset;
    HRESULT hr;
    int i;

    rowset = calloc(1, sizeof(*rowset));
    if (!rowset) return E_OUTOFMEMORY;

    rowset->IRowsetExactScroll_iface.lpVtbl = &rowset_vtbl;
    rowset->IColumnsInfo_iface.lpVtbl = &columns_info_vtbl;
    rowset->IRowsetChange_iface.lpVtbl = &rowset_change_vtbl;
    rowset->IAccessor_iface.lpVtbl = &accessor_vtbl;
    rowset->IRowsetInfo_iface.lpVtbl = &rowset_info_vtbl;
    rowset->refs = 1;

    rowset->column_off = malloc(sizeof(*rowset->column_off) * count);
    if (!rowset->column_off)
    {
        IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
        return E_OUTOFMEMORY;
    }

    rowset->columns_cnt = count;
    hr = copy_column_info(&rowset->columns, info, count, &rowset->columns_buf);
    if (FAILED(hr))
    {
        IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < count; i++)
    {
        rowset->column_off[i] = rowset->row_size;
        rowset->row_size += offsetof(struct data,
                data[dbtype_size(info[i].wType, info[i].ulColumnSize)]);
    }

    *ret = (IUnknown *)&rowset->IRowsetExactScroll_iface;
    return S_OK;
}
