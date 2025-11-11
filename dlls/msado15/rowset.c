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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct rowset
{
    IRowsetExactScroll IRowsetExactScroll_iface;
    IColumnsInfo IColumnsInfo_iface;
    IRowsetChange IRowsetChange_iface;
    IAccessor IAccessor_iface;
    IRowsetInfo IRowsetInfo_iface;
    LONG refs;

    int columns_cnt;
    DBCOLUMNINFO *columns;
    OLECHAR *columns_buf;

    BOOL backward_fetch;
    int index;
    int row_cnt;
};

struct accessor
{
    LONG refs;
};

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
        TRACE("destroying %p\n", rowset);

        CoTaskMemFree(rowset->columns);
        CoTaskMemFree(rowset->columns_buf);
        free(rowset);
    }
    return refs;
}

static HRESULT WINAPI rowset_AddRefRows(IRowsetExactScroll *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %p, %p, %p\n", rowset, count, rows, ref_counts, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetData(IRowsetExactScroll *iface, HROW row, HACCESSOR accessor, void *data)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Id, %p\n", rowset, row, accessor, data);
    return E_NOTIMPL;
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

    FIXME("%p, %Id\n", rowset, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_Compare(IRowsetExactScroll *iface, HCHAPTER hReserved, DBBKMARK cbBookmark1,
        const BYTE *pBookmark1, DBBKMARK cbBookmark2, const BYTE *pBookmark2, DBCOMPARE *pComparison)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Iu, %p, %Iu, %p, %p\n", rowset, hReserved, cbBookmark1,
            pBookmark1, cbBookmark2, pBookmark2, pComparison);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetRowsAt(IRowsetExactScroll *iface, HWATCHREGION hReserved1,
        HCHAPTER hReserved2, DBBKMARK cbBookmark, const BYTE *pBookmark, DBROWOFFSET lRowsOffset,
        DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
    struct rowset *rowset = impl_from_IRowsetExactScroll(iface);

    FIXME("%p, %Id, %Id, %Iu, %p, %Id, %Id, %p, %p\n", rowset, hReserved1, hReserved2,
            cbBookmark, pBookmark, lRowsOffset, cRows, pcRowsObtained, prghRows);
    return E_NOTIMPL;
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

static HRESULT WINAPI rowset_change_SetData(IRowsetChange *iface, HROW row, HACCESSOR accessor, void *data)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);

    FIXME("%p, %Id, %Id, %p\n", rowset, row, accessor, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_change_InsertRow(IRowsetChange *iface, HCHAPTER reserved,
        HACCESSOR accessor, void *data, HROW *row)
{
    struct rowset *rowset = impl_from_IRowsetChange(iface);

    TRACE("%p, %Iu, %Id, %p, %p\n", rowset, reserved, accessor, data, row);

    if (data)
    {
        FIXME("setting data not implemented\n");
        return E_NOTIMPL;
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

    TRACE("%p, %lx, %Iu, %p %Id, %p %p\n", rowset, dwAccessorFlags, cBindings,
            rgBindings, cbRowSize, phAccessor, rgStatus);

    if (!phAccessor) return E_INVALIDARG;
    *phAccessor = 0;

    if (cBindings || cbRowSize)
    {
        FIXME("accessing data not implemented\n");
        return E_NOTIMPL;
    }
    if (dwAccessorFlags != DBACCESSOR_ROWDATA)
    {
        FIXME("unsupported flags %lx\n", dwAccessorFlags);
        return E_NOTIMPL;
    }

    accessor = calloc(1, sizeof(*accessor));
    if (!accessor) return E_OUTOFMEMORY;
    accessor->refs = 1;

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
    ULONG i, j, c = 0;
    DBPROP *prop;

    TRACE("%p, %lu, %p, %p, %p\n", rowset, propidsets_count, propidsets, count, propsets);

    for (i = 0; i <propidsets_count; i++)
    {
        if (!IsEqualGUID(&DBPROPSET_ROWSET, &propidsets[i].guidPropertySet))
        {
            FIXME("property set: %s\n", wine_dbgstr_guid(&propidsets[i].guidPropertySet));
            return E_NOTIMPL;
        }

        for (j = 0; j < propidsets[i].cPropertyIDs; j++)
        {
            if (propidsets[i].rgPropertyIDs[j] != DBPROP_BOOKMARKS)
            {
                FIXME("DBPROPSET_ROWSET property %lu\n", propidsets[i].rgPropertyIDs[j]);
                return E_NOTIMPL;
            }
            c++;
        }
    }
    if (c != 1) return E_NOTIMPL;

    prop = CoTaskMemAlloc(sizeof(*prop));
    if (!prop) return E_OUTOFMEMORY;
    *propsets = CoTaskMemAlloc(sizeof(**propsets));
    if (!*propsets)
    {
        CoTaskMemFree(prop);
        return E_OUTOFMEMORY;
    }

    *count = 1;
    (*propsets)->rgProperties = prop;
    (*propsets)->cProperties = 1;
    (*propsets)->guidPropertySet = DBPROPSET_ROWSET;
    memset(prop, 0, sizeof(*prop));
    prop->dwPropertyID = DBPROP_BOOKMARKS;
    V_VT(&prop->vValue) = VT_BOOL;
    V_BOOL(&prop->vValue) = VARIANT_TRUE;
    return S_OK;
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

    rowset = calloc(1, sizeof(*rowset));
    if (!rowset) return E_OUTOFMEMORY;

    rowset->IRowsetExactScroll_iface.lpVtbl = &rowset_vtbl;
    rowset->IColumnsInfo_iface.lpVtbl = &columns_info_vtbl;
    rowset->IRowsetChange_iface.lpVtbl = &rowset_change_vtbl;
    rowset->IAccessor_iface.lpVtbl = &accessor_vtbl;
    rowset->IRowsetInfo_iface.lpVtbl = &rowset_info_vtbl;
    rowset->refs = 1;

    rowset->columns_cnt = count;
    hr = copy_column_info(&rowset->columns, info, count, &rowset->columns_buf);
    if (FAILED(hr))
    {
        IRowsetExactScroll_Release(&rowset->IRowsetExactScroll_iface);
        return E_OUTOFMEMORY;
    }

    *ret = (IUnknown *)&rowset->IRowsetExactScroll_iface;
    return S_OK;
}
