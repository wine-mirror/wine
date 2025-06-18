/*
 *    Row and rowset servers / proxies.
 *
 * Copyright 2010 Huw Davies
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "oledb.h"

#include "row_server.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

static inline DBLENGTH db_type_size(DBTYPE type, DBLENGTH var_len)
{
    switch(type)
    {
    case DBTYPE_I1:
    case DBTYPE_UI1:
        return 1;
    case DBTYPE_I2:
    case DBTYPE_UI2:
        return 2;
    case DBTYPE_I4:
    case DBTYPE_UI4:
    case DBTYPE_R4:
        return 4;
    case DBTYPE_I8:
    case DBTYPE_UI8:
    case DBTYPE_R8:
        return 8;
    case DBTYPE_CY:
        return sizeof(CY);
    case DBTYPE_FILETIME:
        return sizeof(FILETIME);
    case DBTYPE_BSTR:
        return sizeof(BSTR);
    case DBTYPE_GUID:
        return sizeof(GUID);
    case DBTYPE_WSTR:
        return var_len;
    default:
        FIXME("Unhandled type %04x\n", type);
        return 0;
    }
}

typedef struct
{
    IWineRowServer IWineRowServer_iface;

    LONG ref;

    CLSID class;
    IMarshal *marshal;
    IUnknown *inner_unk;
} server;

static inline server *impl_from_IWineRowServer(IWineRowServer *iface)
{
    return CONTAINING_RECORD(iface, server, IWineRowServer_iface);
}

static HRESULT WINAPI server_QueryInterface(IWineRowServer *iface, REFIID riid, void **obj)
{
    server *This = impl_from_IWineRowServer(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IWineRowServer))
    {
        *obj = iface;
    }
    else
    {
        if(!IsEqualIID(riid, &IID_IMarshal)) /* We use standard marshalling */
            FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IWineRowServer_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI server_AddRef(IWineRowServer *iface)
{
    server *This = impl_from_IWineRowServer(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI server_Release(IWineRowServer *iface)
{
    server *This = impl_from_IWineRowServer(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        IMarshal_Release(This->marshal);
        if(This->inner_unk) IUnknown_Release(This->inner_unk);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI server_SetInnerUnk(IWineRowServer *iface, IUnknown *inner)
{
    server *This = impl_from_IWineRowServer(iface);

    if(This->inner_unk) IUnknown_Release(This->inner_unk);

    if(inner) IUnknown_AddRef(inner);
    This->inner_unk = inner;
    return S_OK;
}

static HRESULT WINAPI server_GetMarshal(IWineRowServer *iface, IMarshal **marshal)
{
    server *This = impl_from_IWineRowServer(iface);

    IMarshal_AddRef(This->marshal);
    *marshal = This->marshal;
    return S_OK;
}

static HRESULT WINAPI server_GetColumns(IWineRowServer* iface, DBORDINAL num_cols,
                                        wine_getcolumns_in *in_data, wine_getcolumns_out *out_data)
{
    server *This = impl_from_IWineRowServer(iface);
    HRESULT hr;
    DBORDINAL i;
    DBCOLUMNACCESS *cols;
    IRow *row;

    TRACE("(%p)->(%Id, %p, %p)\n", This, num_cols, in_data, out_data);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRow, (void**)&row);
    if(FAILED(hr)) return hr;

    cols = CoTaskMemAlloc(num_cols * sizeof(cols[0]));

    for(i = 0; i < num_cols; i++)
    {
        TRACE("%Id:\tmax_len %Id type %04x\n", i, in_data[i].max_len, in_data[i].type);
        cols[i].pData        = CoTaskMemAlloc(db_type_size(in_data[i].type, in_data[i].max_len));
        cols[i].columnid     = in_data[i].columnid;
        cols[i].cbMaxLen     = in_data[i].max_len;
        cols[i].wType        = in_data[i].type;
        cols[i].bPrecision   = in_data[i].precision;
        cols[i].bScale       = in_data[i].scale;
    }

    hr = IRow_GetColumns(row, num_cols, cols);
    IRow_Release(row);

    for(i = 0; i < num_cols; i++)
    {
        VariantInit(&out_data[i].v);
        if(cols[i].dwStatus == DBSTATUS_S_OK)
        {
            V_VT(&out_data[i].v) = in_data[i].type;
            memcpy(&V_I1(&out_data[i].v), cols[i].pData, cols[i].cbDataLen);
        }
        CoTaskMemFree(cols[i].pData);
        out_data[i].data_len = cols[i].cbDataLen;
        out_data[i].status   = cols[i].dwStatus;
    }

    CoTaskMemFree(cols);

    return hr;
}

static HRESULT WINAPI server_GetSourceRowset(IWineRowServer* iface, REFIID riid, IUnknown **ppRowset,
                                             HROW *phRow)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_Open(IWineRowServer* iface, IUnknown *pUnkOuter, DBID *pColumnID,
                                  REFGUID rguidColumnType, DWORD dwBindFlags, REFIID riid,
                                  IUnknown **ppUnk)
{
    server *This = impl_from_IWineRowServer(iface);
    IRow *row;
    HRESULT hr;
    IWineRowServer *new_server;
    IMarshal *marshal;
    IUnknown *obj;

    TRACE("(%p)->(%p, %p, %s, %08lx, %s, %p)\n", This, pUnkOuter, pColumnID, debugstr_guid(rguidColumnType),
          dwBindFlags, debugstr_guid(riid), ppUnk);

    *ppUnk = NULL;

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRow, (void**)&row);
    if(FAILED(hr)) return hr;

    if(IsEqualGUID(rguidColumnType, &DBGUID_ROWSET))
        hr = CoCreateInstance(&CLSID_wine_rowset_server, NULL, CLSCTX_INPROC_SERVER, &IID_IWineRowServer, (void**)&new_server);
    else
    {
        FIXME("Unhandled object %s\n", debugstr_guid(rguidColumnType));
        hr = E_NOTIMPL;
    }

    if(FAILED(hr))
    {
        IRow_Release(row);
        return hr;
    }

    IWineRowServer_GetMarshal(new_server, &marshal);
    hr = IRow_Open(row, (IUnknown*)marshal, pColumnID, rguidColumnType, dwBindFlags, &IID_IUnknown, &obj);
    IMarshal_Release(marshal);
    IRow_Release(row);

    if(FAILED(hr))
    {
        IWineRowServer_Release(new_server);
        return hr;
    }

    IWineRowServer_SetInnerUnk(new_server, obj);
    hr = IUnknown_QueryInterface(obj, riid, (void**)ppUnk);
    IUnknown_Release(obj);

    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_SetColumns(IWineRowServer* iface, DBORDINAL num_cols,
                                        wine_setcolumns_in *in_data, DBSTATUS *status)
{
    server *This = impl_from_IWineRowServer(iface);
    HRESULT hr;
    DBORDINAL i;
    DBCOLUMNACCESS *cols;
    IRowChange *row_change;

    TRACE("(%p)->(%Id, %p, %p)\n", This, num_cols, in_data, status);
    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowChange, (void**)&row_change);
    if(FAILED(hr)) return hr;

    cols = CoTaskMemAlloc(num_cols * sizeof(cols[0]));

    for(i = 0; i < num_cols; i++)
    {
        TRACE("%Id:\ttype %04x\n", i, in_data[i].type);
        cols[i].pData        = CoTaskMemAlloc(db_type_size(in_data[i].type, in_data[i].max_len));
        memcpy(cols[i].pData, &V_I1(&in_data[i].v), db_type_size(in_data[i].type, in_data[i].max_len));
        cols[i].columnid     = in_data[i].columnid;
        cols[i].cbDataLen    = in_data[i].data_len;
        cols[i].dwStatus     = in_data[i].status;
        cols[i].cbMaxLen     = in_data[i].max_len;
        cols[i].wType        = in_data[i].type;
        cols[i].bPrecision   = in_data[i].precision;
        cols[i].bScale       = in_data[i].scale;
    }

    hr = IRowChange_SetColumns(row_change, num_cols, cols);
    IRowChange_Release(row_change);

    for(i = 0; i < num_cols; i++)
    {
        CoTaskMemFree(cols[i].pData);
        status[i] = cols[i].dwStatus;
    }

    CoTaskMemFree(cols);

    return hr;
}

static HRESULT WINAPI server_AddRefRows(IWineRowServer* iface, DBCOUNTITEM cRows,
                                        const HROW rghRows[], DBREFCOUNT rgRefCounts[],
                                        DBROWSTATUS rgRowStatus[])
{
    server *This = impl_from_IWineRowServer(iface);
    IRowset *rowset;
    HRESULT hr;

    TRACE("(%p)->(%Id, %p, %p, %p)\n", This, cRows, rghRows, rgRefCounts, rgRowStatus);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowset, (void**)&rowset);
    if(FAILED(hr)) return hr;

    hr = IRowset_AddRefRows(rowset, cRows, rghRows, rgRefCounts, rgRowStatus);

    IRowset_Release(rowset);
    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_GetData(IWineRowServer* iface, HROW hRow,
                                     HACCESSOR hAccessor, BYTE *pData, DWORD size)
{
    server *This = impl_from_IWineRowServer(iface);
    IRowset *rowset;
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %08Ix, %p, %ld)\n", This, hRow, hAccessor, pData, size);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowset, (void**)&rowset);
    if(FAILED(hr)) return hr;

    hr = IRowset_GetData(rowset, hRow, hAccessor, pData);

    IRowset_Release(rowset);
    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_GetNextRows(IWineRowServer* iface, HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
                                         DBROWCOUNT cRows, DBCOUNTITEM *pcRowObtained, HROW **prghRows)
{
    server *This = impl_from_IWineRowServer(iface);
    IRowset *rowset;
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %Id, %Id, %p, %p)\n", This, hReserved, lRowsOffset, cRows, pcRowObtained, prghRows);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowset, (void**)&rowset);
    if(FAILED(hr)) return hr;

    *prghRows = NULL;

    hr = IRowset_GetNextRows(rowset, hReserved, lRowsOffset, cRows, pcRowObtained, prghRows);
    IRowset_Release(rowset);
    TRACE("returning %08lx, got %Id rows\n", hr, *pcRowObtained);
    return hr;
}

static HRESULT WINAPI server_ReleaseRows(IWineRowServer* iface, DBCOUNTITEM cRows, const HROW rghRows[],
                                         DBROWOPTIONS rgRowOptions[], DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    server *This = impl_from_IWineRowServer(iface);
    IRowset *rowset;
    HRESULT hr;

    TRACE("(%p)->(%Id, %p, %p, %p, %p)\n", This, cRows, rghRows, rgRowOptions, rgRefCounts, rgRowStatus);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowset, (void**)&rowset);
    if(FAILED(hr)) return hr;

    hr = IRowset_ReleaseRows(rowset, cRows, rghRows, rgRowOptions, rgRefCounts, rgRowStatus);
    IRowset_Release(rowset);

    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_RestartPosition(IWineRowServer* iface, HCHAPTER hReserved)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p)->(%08Ix): stub\n", This, hReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_Compare(IWineRowServer *iface, HCHAPTER hReserved, DBBKMARK cbBookmark1,
                                     const BYTE *pBookmark1, DBBKMARK cbBookmark2, const BYTE *pBookmark2,
                                     DBCOMPARE *pComparison)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_GetRowsAt(IWineRowServer *iface, HWATCHREGION hReserved1, HCHAPTER hReserved2,
                                       DBBKMARK cbBookmark, const BYTE *pBookmark, DBROWOFFSET lRowsOffset,
                                       DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
    server *This = impl_from_IWineRowServer(iface);
    IRowsetLocate *rowsetlocate;
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %08Ix, %Id, %p, %Id, %Id, %p, %p\n", This, hReserved1, hReserved2, cbBookmark, pBookmark, lRowsOffset, cRows,
          pcRowsObtained, prghRows);

    *prghRows = NULL;

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowsetLocate, (void**)&rowsetlocate);
    if(FAILED(hr)) return hr;

    hr = IRowsetLocate_GetRowsAt(rowsetlocate, hReserved1, hReserved2, cbBookmark, pBookmark, lRowsOffset,
                                 cRows, pcRowsObtained, prghRows);
    IRowsetLocate_Release(rowsetlocate);

    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_GetRowsByBookmark(IWineRowServer *iface, HCHAPTER hReserved, DBCOUNTITEM cRows,
                                               const DBBKMARK rgcbBookmarks[], const BYTE * rgpBookmarks[],
                                               HROW rghRows[], DBROWSTATUS rgRowStatus[])
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_Hash(IWineRowServer *iface, HCHAPTER hReserved, DBBKMARK cBookmarks,
                                  const DBBKMARK rgcbBookmarks[], const BYTE * rgpBookmarks[],
                                  DBHASHVALUE rgHashedValues[], DBROWSTATUS rgBookmarkStatus[])
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_GetProperties(IWineRowServer* iface, ULONG cPropertyIDSets,
                                           const DBPROPIDSET *rgPropertyIDSets, ULONG *pcPropertySets,
                                           DBPROPSET **prgPropertySets)
{
    server *This = impl_from_IWineRowServer(iface);
    IRowsetInfo *rowsetinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IRowsetInfo, (void**)&rowsetinfo);
    if(FAILED(hr)) return hr;

    hr = IRowsetInfo_GetProperties(rowsetinfo, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
    IRowsetInfo_Release(rowsetinfo);

    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_GetReferencedRowset(IWineRowServer* iface, DBORDINAL iOrdinal,
                                                 REFIID riid, IUnknown **ppReferencedRowset)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_GetSpecification(IWineRowServer* iface, REFIID riid,
                                               IUnknown **ppSpecification)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_AddRefAccessor(IWineRowServer* iface, HACCESSOR hAccessor,
                                            DBREFCOUNT *pcRefCount)
{
    server *This = impl_from_IWineRowServer(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI server_CreateAccessor(IWineRowServer* iface, DBACCESSORFLAGS dwAccessorFlags,
                                            DBCOUNTITEM cBindings, const DBBINDING *rgBindings, DBLENGTH cbRowSize,
                                            HACCESSOR *phAccessor, DBBINDSTATUS *rgStatus)
{
    server *This = impl_from_IWineRowServer(iface);
    HRESULT hr;
    IAccessor *accessor;

    TRACE("(%p)->(%08lx, %Id, %p, %Id, %p, %p)\n", This, dwAccessorFlags, cBindings, rgBindings, cbRowSize, phAccessor, rgStatus);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IAccessor, (void**)&accessor);
    if(FAILED(hr)) return hr;

    hr = IAccessor_CreateAccessor(accessor, dwAccessorFlags, cBindings, rgBindings, cbRowSize, phAccessor, rgStatus);
    IAccessor_Release(accessor);

    TRACE("returning %08lx, accessor %08Ix\n", hr, *phAccessor);
    return hr;
}

static HRESULT WINAPI server_GetBindings(IWineRowServer* iface, HACCESSOR hAccessor,
                                         DBACCESSORFLAGS *pdwAccessorFlags, DBCOUNTITEM *pcBindings,
                                         DBBINDING **prgBindings)
{
    server *This = impl_from_IWineRowServer(iface);
    HRESULT hr;
    IAccessor *accessor;

    TRACE("(%p)->(%08Ix, %p, %p, %p)\n", This, hAccessor, pdwAccessorFlags, pcBindings, prgBindings);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IAccessor, (void**)&accessor);
    if(FAILED(hr)) return hr;

    hr = IAccessor_GetBindings(accessor, hAccessor, pdwAccessorFlags, pcBindings, prgBindings);
    IAccessor_Release(accessor);

    TRACE("returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI server_ReleaseAccessor(IWineRowServer* iface, HACCESSOR hAccessor,
                                             DBREFCOUNT *pcRefCount)
{
    server *This = impl_from_IWineRowServer(iface);
    HRESULT hr;
    IAccessor *accessor;

    TRACE("(%p)->(%08Ix, %p)\n", This, hAccessor, pcRefCount);

    hr = IUnknown_QueryInterface(This->inner_unk, &IID_IAccessor, (void**)&accessor);
    if(FAILED(hr)) return hr;

    hr = IAccessor_ReleaseAccessor(accessor, hAccessor, pcRefCount);
    IAccessor_Release(accessor);

    return hr;
}

static const IWineRowServerVtbl server_vtbl =
{
    server_QueryInterface,
    server_AddRef,
    server_Release,
    server_SetInnerUnk,
    server_GetMarshal,
    server_GetColumns,
    server_GetSourceRowset,
    server_Open,
    server_SetColumns,
    server_AddRefRows,
    server_GetData,
    server_GetNextRows,
    server_ReleaseRows,
    server_RestartPosition,
    server_Compare,
    server_GetRowsAt,
    server_GetRowsByBookmark,
    server_Hash,
    server_GetProperties,
    server_GetReferencedRowset,
    server_GetSpecification,
    server_AddRefAccessor,
    server_CreateAccessor,
    server_GetBindings,
    server_ReleaseAccessor
};

static HRESULT create_server(IUnknown *outer, const CLSID *class, void **obj)
{
    server *server;
    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(class), obj);

    *obj = NULL;

    server = HeapAlloc(GetProcessHeap(), 0, sizeof(*server));
    if(!server) return E_OUTOFMEMORY;

    server->IWineRowServer_iface.lpVtbl = &server_vtbl;
    server->ref = 1;
    server->class = *class;
    server->inner_unk = NULL;
    if(IsEqualGUID(class, &CLSID_wine_row_server))
        create_row_marshal((IUnknown*)&server->IWineRowServer_iface, (void**)&server->marshal);
    else if(IsEqualGUID(class, &CLSID_wine_rowset_server))
        create_rowset_marshal((IUnknown*)&server->IWineRowServer_iface, (void**)&server->marshal);
    else
        ERR("create_server called with class %s\n", debugstr_guid(class));

    *obj = server;
    return S_OK;
}

HRESULT create_row_server(IUnknown *outer, void **obj)
{
    return create_server(outer, &CLSID_wine_row_server, obj);
}

HRESULT create_rowset_server(IUnknown *outer, void **obj)
{
    return create_server(outer, &CLSID_wine_rowset_server, obj);
}

typedef struct
{
    IRow IRow_iface;
    IRowChange IRowChange_iface;

    LONG ref;

    IWineRowServer *server;
} row_proxy;

static inline row_proxy *impl_from_IRow(IRow *iface)
{
    return CONTAINING_RECORD(iface, row_proxy, IRow_iface);
}

static inline row_proxy *impl_from_IRowChange(IRowChange *iface)
{
    return CONTAINING_RECORD(iface, row_proxy, IRowChange_iface);
}

static HRESULT WINAPI row_QueryInterface(IRow *iface, REFIID iid, void **obj)
{
    row_proxy *This = impl_from_IRow(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), obj);

    if(IsEqualIID(iid, &IID_IUnknown) ||
       IsEqualIID(iid, &IID_IRow))
    {
        *obj = &This->IRow_iface;
    }
    else if(IsEqualIID(iid, &IID_IRowChange))
    {
        *obj = &This->IRowChange_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IRow_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI row_AddRef(IRow *iface)
{
    row_proxy *This = impl_from_IRow(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI row_Release(IRow *iface)
{
    row_proxy *This = impl_from_IRow(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        if(This->server) IWineRowServer_Release(This->server);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI row_GetColumns(IRow* iface, DBORDINAL cColumns, DBCOLUMNACCESS rgColumns[])
{
    row_proxy *This = impl_from_IRow(iface);
    DBORDINAL i;
    wine_getcolumns_in *in_data;
    wine_getcolumns_out *out_data;
    HRESULT hr;

    TRACE("(%p)->(%Id, %p)\n", This, cColumns, rgColumns);

    in_data = CoTaskMemAlloc(cColumns * sizeof(in_data[0]));
    out_data = CoTaskMemAlloc(cColumns * sizeof(out_data[0]));

    for(i = 0; i < cColumns; i++)
    {
        TRACE("%Id:\tdata %p data_len %Id status %08lx max_len %Id type %04x\n", i, rgColumns[i].pData,
              rgColumns[i].cbDataLen, rgColumns[i].dwStatus, rgColumns[i].cbMaxLen, rgColumns[i].wType);
        in_data[i].columnid     = rgColumns[i].columnid;
        in_data[i].max_len      = rgColumns[i].cbMaxLen;
        in_data[i].type         = rgColumns[i].wType;
        in_data[i].precision    = rgColumns[i].bPrecision;
        in_data[i].scale        = rgColumns[i].bScale;
    }

    hr = IWineRowServer_GetColumns(This->server, cColumns, in_data, out_data);

    for(i = 0; i < cColumns; i++)
    {
        rgColumns[i].cbDataLen = out_data[i].data_len;
        rgColumns[i].dwStatus  = out_data[i].status;
        if(rgColumns[i].dwStatus == DBSTATUS_S_OK)
            memcpy(rgColumns[i].pData, &V_I1(&out_data[i].v), out_data[i].data_len);
    }

    CoTaskMemFree(out_data);
    CoTaskMemFree(in_data);
    return hr;
}

static HRESULT WINAPI row_GetSourceRowset(IRow* iface, REFIID riid, IUnknown **ppRowset,
                                          HROW *phRow)
{
    row_proxy *This = impl_from_IRow(iface);

    FIXME("(%p)->(%s, %p, %p): stub\n", This, debugstr_guid(riid), ppRowset, phRow);

    return E_NOTIMPL;
}

static HRESULT WINAPI row_Open(IRow* iface, IUnknown *pUnkOuter,
                               DBID *pColumnID, REFGUID rguidColumnType,
                               DWORD dwBindFlags, REFIID riid, IUnknown **ppUnk)
{
    row_proxy *This = impl_from_IRow(iface);

    TRACE("(%p)->(%p, %p, %s, %08lx, %s, %p)\n", This, pUnkOuter, pColumnID, debugstr_guid(rguidColumnType),
          dwBindFlags, debugstr_guid(riid), ppUnk);
    if(pUnkOuter)
    {
        FIXME("Aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    return IWineRowServer_Open(This->server, pUnkOuter, pColumnID, rguidColumnType, dwBindFlags, riid, ppUnk);
}

static const IRowVtbl row_vtbl =
{
    row_QueryInterface,
    row_AddRef,
    row_Release,
    row_GetColumns,
    row_GetSourceRowset,
    row_Open
};

static HRESULT WINAPI row_change_QueryInterface(IRowChange *iface, REFIID iid, void **obj)
{
    row_proxy *This = impl_from_IRowChange(iface);
    return IRow_QueryInterface(&This->IRow_iface, iid, obj);
}

static ULONG WINAPI row_change_AddRef(IRowChange *iface)
{
    row_proxy *This = impl_from_IRowChange(iface);
    return IRow_AddRef(&This->IRow_iface);
}

static ULONG WINAPI row_change_Release(IRowChange *iface)
{
    row_proxy *This = impl_from_IRowChange(iface);
    return IRow_Release(&This->IRow_iface);
}

static HRESULT WINAPI row_change_SetColumns(IRowChange *iface, DBORDINAL cColumns,
                                            DBCOLUMNACCESS rgColumns[])
{
    row_proxy *This = impl_from_IRowChange(iface);
    HRESULT hr;
    wine_setcolumns_in *in_data;
    DBSTATUS *status;
    DBORDINAL i;

    TRACE("(%p)->(%Id, %p)\n", This, cColumns, rgColumns);

    in_data = CoTaskMemAlloc(cColumns * sizeof(in_data[0]));
    status = CoTaskMemAlloc(cColumns * sizeof(status[0]));

    for(i = 0; i < cColumns; i++)
    {
        TRACE("%Id: wtype %04x max %08Ix len %08Ix\n", i, rgColumns[i].wType, rgColumns[i].cbMaxLen, rgColumns[i].cbDataLen);
        V_VT(&in_data[i].v) = rgColumns[i].wType;
        memcpy(&V_I1(&in_data[i].v), rgColumns[i].pData, db_type_size(rgColumns[i].wType, rgColumns[i].cbDataLen));
        in_data[i].columnid = rgColumns[i].columnid;
        in_data[i].data_len = rgColumns[i].cbDataLen;
        in_data[i].status = rgColumns[i].dwStatus;
        in_data[i].max_len = rgColumns[i].cbMaxLen;
        in_data[i].type = rgColumns[i].wType;
        in_data[i].precision = rgColumns[i].bPrecision;
        in_data[i].scale = rgColumns[i].bScale;
    }

    hr = IWineRowServer_SetColumns(This->server, cColumns, in_data, status);

    for(i = 0; i < cColumns; i++)
        rgColumns[i].dwStatus = status[i];

    CoTaskMemFree(status);
    CoTaskMemFree(in_data);

    return hr;
}

static const IRowChangeVtbl row_change_vtbl =
{
    row_change_QueryInterface,
    row_change_AddRef,
    row_change_Release,
    row_change_SetColumns
};

static HRESULT create_row_proxy(IWineRowServer *server, IUnknown **obj)
{
    row_proxy *proxy;

    TRACE("(%p, %p)\n", server, obj);
    *obj = NULL;

    proxy = HeapAlloc(GetProcessHeap(), 0, sizeof(*proxy));
    if(!proxy) return E_OUTOFMEMORY;

    proxy->IRow_iface.lpVtbl = &row_vtbl;
    proxy->IRowChange_iface.lpVtbl = &row_change_vtbl;
    proxy->ref = 1;
    IWineRowServer_AddRef(server);
    proxy->server = server;

    *obj = (IUnknown*)&proxy->IRow_iface;
    TRACE("returning %p\n", *obj);
    return S_OK;
}

typedef struct
{
    IRowsetLocate IRowsetLocate_iface;
    IRowsetInfo IRowsetInfo_iface;
    IAccessor IAccessor_iface;

    LONG ref;

    IWineRowServer *server;
} rowset_proxy;

static inline rowset_proxy *impl_from_IRowsetLocate(IRowsetLocate *iface)
{
    return CONTAINING_RECORD(iface, rowset_proxy, IRowsetLocate_iface);
}

static inline rowset_proxy *impl_from_IRowsetInfo(IRowsetInfo *iface)
{
    return CONTAINING_RECORD(iface, rowset_proxy, IRowsetInfo_iface);
}

static inline rowset_proxy *impl_from_IAccessor(IAccessor *iface)
{
    return CONTAINING_RECORD(iface, rowset_proxy, IAccessor_iface);
}

static HRESULT WINAPI rowsetlocate_QueryInterface(IRowsetLocate *iface, REFIID iid, void **obj)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), obj);

    *obj = NULL;

    if(IsEqualIID(iid, &IID_IUnknown) ||
       IsEqualIID(iid, &IID_IRowset) ||
       IsEqualIID(iid, &IID_IRowsetLocate))
    {
        *obj = &This->IRowsetLocate_iface;
    }
    else if(IsEqualIID(iid, &IID_IRowsetInfo))
    {
        *obj = &This->IRowsetInfo_iface;
    }
    else if(IsEqualIID(iid, &IID_IAccessor))
    {
        *obj = &This->IAccessor_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IRowsetLocate_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI rowsetlocate_AddRef(IRowsetLocate *iface)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI rowsetlocate_Release(IRowsetLocate *iface)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        if(This->server) IWineRowServer_Release(This->server);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI rowsetlocate_AddRefRows(IRowsetLocate *iface, DBCOUNTITEM cRows, const HROW rghRows[],
                                              DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    HRESULT hr;
    DBREFCOUNT *refs = rgRefCounts;
    DBSTATUS *stats = rgRowStatus;

    TRACE("(%p)->(%Id, %p, %p, %p)\n", This, cRows, rghRows, rgRefCounts, rgRowStatus);

    if(!refs) refs = CoTaskMemAlloc(cRows * sizeof(refs[0]));
    if(!stats) stats = CoTaskMemAlloc(cRows * sizeof(stats[0]));

    hr = IWineRowServer_AddRefRows(This->server, cRows, rghRows, refs, stats);

    if(refs != rgRefCounts) CoTaskMemFree(refs);
    if(stats != rgRowStatus) CoTaskMemFree(stats);

    return hr;
}

static HRESULT WINAPI rowsetlocate_GetData(IRowsetLocate *iface, HROW hRow, HACCESSOR hAccessor, void *pData)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    HRESULT hr;
    IAccessor *accessor;
    DBACCESSORFLAGS flags;
    DBCOUNTITEM count, i;
    DBBINDING *bindings;
    DWORD max_len = 0;

    TRACE("(%p)->(%Ix, %Ix, %p)\n", This, hRow, hAccessor, pData);

    hr = IRowsetLocate_QueryInterface(iface, &IID_IAccessor, (void**)&accessor);
    if(FAILED(hr)) return hr;

    hr = IAccessor_GetBindings(accessor, hAccessor, &flags, &count, &bindings);
    IAccessor_Release(accessor);
    if(FAILED(hr)) return hr;

    TRACE("got %Id bindings\n", count);
    for(i = 0; i < count; i++)
    {
        TRACE("%Id\tord %Id offs: val %Id len %Id stat %Id, part %lx, max len %Id type %04x\n",
              i, bindings[i].iOrdinal, bindings[i].obValue, bindings[i].obLength, bindings[i].obStatus,
              bindings[i].dwPart, bindings[i].cbMaxLen, bindings[i].wType);
        if(bindings[i].dwPart & DBPART_LENGTH && bindings[i].obLength >= max_len)
            max_len = bindings[i].obLength + sizeof(DBLENGTH);
        if(bindings[i].dwPart & DBPART_STATUS && bindings[i].obStatus >= max_len)
            max_len = bindings[i].obStatus + sizeof(DWORD);
        if(bindings[i].dwPart & DBPART_VALUE && bindings[i].obValue >= max_len)
            max_len = bindings[i].obValue + db_type_size(bindings[i].wType, bindings[i].cbMaxLen);

    }
    TRACE("max_len %ld\n", max_len);

    CoTaskMemFree(bindings);

    hr = IWineRowServer_GetData(This->server, hRow, hAccessor, pData, max_len);

    return hr;
}

static HRESULT WINAPI rowsetlocate_GetNextRows(IRowsetLocate *iface, HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
                                               DBROWCOUNT cRows, DBCOUNTITEM *pcRowObtained, HROW **prghRows)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    HRESULT hr;
    HROW *rows = NULL;

    TRACE("(%p)->(%08Ix, %Id, %Id, %p, %p)\n", This, hReserved, lRowsOffset, cRows, pcRowObtained, prghRows);

    hr = IWineRowServer_GetNextRows(This->server, hReserved, lRowsOffset, cRows, pcRowObtained, &rows);
    if(*prghRows)
    {
        memcpy(*prghRows, rows, *pcRowObtained * sizeof(rows[0]));
        CoTaskMemFree(rows);
    }
    else
        *prghRows = rows;

    return hr;
}

static HRESULT WINAPI rowsetlocate_ReleaseRows(IRowsetLocate *iface, DBCOUNTITEM cRows, const HROW rghRows[],
                                               DBROWOPTIONS rgRowOptions[], DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    HRESULT hr;
    DBROWOPTIONS *options = rgRowOptions;
    DBREFCOUNT *refs = rgRefCounts;
    DBROWSTATUS *status = rgRowStatus;

    TRACE("(%p)->(%Id, %p, %p, %p, %p)\n", This, cRows, rghRows, rgRowOptions, rgRefCounts, rgRowStatus);

    if(!options)
    {
        options = CoTaskMemAlloc(cRows * sizeof(options[0]));
        memset(options, 0, cRows * sizeof(options[0]));
    }
    if(!refs) refs = CoTaskMemAlloc(cRows * sizeof(refs[0]));
    if(!status) status = CoTaskMemAlloc(cRows * sizeof(status[0]));

    hr = IWineRowServer_ReleaseRows(This->server, cRows, rghRows, options, refs, status);

    if(status != rgRowStatus) CoTaskMemFree(status);
    if(refs != rgRefCounts) CoTaskMemFree(refs);
    if(options != rgRowOptions) CoTaskMemFree(options);

    return hr;
}

static HRESULT WINAPI rowsetlocate_RestartPosition(IRowsetLocate* iface, HCHAPTER hReserved)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);

    FIXME("(%p)->(%Ix): stub\n", This, hReserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetlocate_Compare(IRowsetLocate *iface, HCHAPTER hReserved, DBBKMARK cbBookmark1, const BYTE *pBookmark1,
                                           DBBKMARK cbBookmark2, const BYTE *pBookmark2, DBCOMPARE *pComparison)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetlocate_GetRowsAt(IRowsetLocate *iface, HWATCHREGION hReserved1, HCHAPTER hReserved2, DBBKMARK cbBookmark,
                                             const BYTE *pBookmark, DBROWOFFSET lRowsOffset, DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained,
                                             HROW **prghRows)
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    HRESULT hr;
    HROW *rows = NULL;

    TRACE("(%p)->(%08Ix, %08Ix, %Id, %p, %Id, %Id, %p, %p\n", This, hReserved1, hReserved2, cbBookmark, pBookmark, lRowsOffset, cRows,
          pcRowsObtained, prghRows);

    hr = IWineRowServer_GetRowsAt(This->server, hReserved1, hReserved2, cbBookmark, pBookmark, lRowsOffset, cRows, pcRowsObtained, &rows);

    if(*prghRows)
    {
        memcpy(*prghRows, rows, *pcRowsObtained * sizeof(rows[0]));
        CoTaskMemFree(rows);
    }
    else
        *prghRows = rows;

    return hr;
}

static HRESULT WINAPI rowsetlocate_GetRowsByBookmark(IRowsetLocate *iface, HCHAPTER hReserved, DBCOUNTITEM cRows, const DBBKMARK rgcbBookmarks[],
                                                     const BYTE * rgpBookmarks[], HROW rghRows[], DBROWSTATUS rgRowStatus[])
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetlocate_Hash(IRowsetLocate *iface, HCHAPTER hReserved, DBBKMARK cBookmarks, const DBBKMARK rgcbBookmarks[],
                                        const BYTE * rgpBookmarks[], DBHASHVALUE rgHashedValues[], DBROWSTATUS rgBookmarkStatus[])
{
    rowset_proxy *This = impl_from_IRowsetLocate(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IRowsetLocateVtbl rowsetlocate_vtbl =
{
    rowsetlocate_QueryInterface,
    rowsetlocate_AddRef,
    rowsetlocate_Release,
    rowsetlocate_AddRefRows,
    rowsetlocate_GetData,
    rowsetlocate_GetNextRows,
    rowsetlocate_ReleaseRows,
    rowsetlocate_RestartPosition,
    rowsetlocate_Compare,
    rowsetlocate_GetRowsAt,
    rowsetlocate_GetRowsByBookmark,
    rowsetlocate_Hash
};

static HRESULT WINAPI rowsetinfo_QueryInterface(IRowsetInfo *iface, REFIID iid, void **obj)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    return IRowsetLocate_QueryInterface(&This->IRowsetLocate_iface, iid, obj);
}

static ULONG WINAPI rowsetinfo_AddRef(IRowsetInfo *iface)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    return IRowsetLocate_AddRef(&This->IRowsetLocate_iface);
}

static ULONG WINAPI rowsetinfo_Release(IRowsetInfo *iface)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    return IRowsetLocate_Release(&This->IRowsetLocate_iface);
}

static HRESULT WINAPI rowsetinfo_GetProperties(IRowsetInfo *iface, const ULONG cPropertyIDSets,
                                               const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets,
                                               DBPROPSET **prgPropertySets)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    HRESULT hr;

    TRACE("(%p)->(%ld, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    hr = IWineRowServer_GetProperties(This->server, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    return hr;
}

static HRESULT WINAPI rowsetinfo_GetReferencedRowset(IRowsetInfo *iface, DBORDINAL iOrdinal, REFIID riid,
                                                     IUnknown **ppReferencedRowset)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowsetinfo_GetSpecification(IRowsetInfo *iface, REFIID riid, IUnknown **ppSpecification)
{
    rowset_proxy *This = impl_from_IRowsetInfo(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IRowsetInfoVtbl rowsetinfo_vtbl =
{
    rowsetinfo_QueryInterface,
    rowsetinfo_AddRef,
    rowsetinfo_Release,
    rowsetinfo_GetProperties,
    rowsetinfo_GetReferencedRowset,
    rowsetinfo_GetSpecification
};

static HRESULT WINAPI accessor_QueryInterface(IAccessor *iface, REFIID iid, void **obj)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    return IRowsetLocate_QueryInterface(&This->IRowsetLocate_iface, iid, obj);
}

static ULONG WINAPI accessor_AddRef(IAccessor *iface)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    return IRowsetLocate_AddRef(&This->IRowsetLocate_iface);
}

static ULONG WINAPI accessor_Release(IAccessor *iface)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    return IRowsetLocate_Release(&This->IRowsetLocate_iface);
}

static HRESULT WINAPI accessor_AddRefAccessor(IAccessor *iface, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI accessor_CreateAccessor(IAccessor *iface, DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
                                              const DBBINDING rgBindings[], DBLENGTH cbRowSize, HACCESSOR *phAccessor,
                                              DBBINDSTATUS rgStatus[])
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    HRESULT hr;
    DBBINDSTATUS *status;

    TRACE("(%p)->(%08lx, %Id, %p, %Id, %p, %p)\n", This, dwAccessorFlags, cBindings, rgBindings, cbRowSize, phAccessor, rgStatus);

    if(!rgStatus) status = CoTaskMemAlloc(cBindings * sizeof(status[0]));
    else status = rgStatus;

    hr = IWineRowServer_CreateAccessor(This->server, dwAccessorFlags, cBindings, rgBindings, cbRowSize, phAccessor, status);

    if(!rgStatus) CoTaskMemFree(status);

    return hr;
}

static HRESULT WINAPI accessor_GetBindings(IAccessor *iface, HACCESSOR hAccessor, DBACCESSORFLAGS *pdwAccessorFlags,
                                           DBCOUNTITEM *pcBindings, DBBINDING **prgBindings)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %p, %p, %p)\n", This, hAccessor, pdwAccessorFlags, pcBindings, prgBindings);

    hr = IWineRowServer_GetBindings(This->server, hAccessor, pdwAccessorFlags, pcBindings, prgBindings);

    return hr;
}

static HRESULT WINAPI accessor_ReleaseAccessor(IAccessor *iface, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
    rowset_proxy *This = impl_from_IAccessor(iface);
    HRESULT hr;
    DBREFCOUNT ref;

    TRACE("(%p)->(%08Ix, %p)\n", This, hAccessor, pcRefCount);

    hr = IWineRowServer_ReleaseAccessor(This->server, hAccessor, &ref);
    if(pcRefCount) *pcRefCount = ref;
    return hr;
}

static const IAccessorVtbl accessor_vtbl =
{
    accessor_QueryInterface,
    accessor_AddRef,
    accessor_Release,
    accessor_AddRefAccessor,
    accessor_CreateAccessor,
    accessor_GetBindings,
    accessor_ReleaseAccessor
};

static HRESULT create_rowset_proxy(IWineRowServer *server, IUnknown **obj)
{
    rowset_proxy *proxy;

    TRACE("(%p, %p)\n", server, obj);
    *obj = NULL;

    proxy = HeapAlloc(GetProcessHeap(), 0, sizeof(*proxy));
    if(!proxy) return E_OUTOFMEMORY;

    proxy->IRowsetLocate_iface.lpVtbl = &rowsetlocate_vtbl;
    proxy->IRowsetInfo_iface.lpVtbl = &rowsetinfo_vtbl;
    proxy->IAccessor_iface.lpVtbl = &accessor_vtbl;
    proxy->ref = 1;
    IWineRowServer_AddRef(server);
    proxy->server = server;

    *obj = (IUnknown *)&proxy->IRowsetLocate_iface;
    TRACE("returning %p\n", *obj);
    return S_OK;
}

static HRESULT create_proxy(IWineRowServer *server, const CLSID *class, IUnknown **obj)
{
    *obj = NULL;

    if(IsEqualGUID(class, &CLSID_wine_row_proxy))
        return create_row_proxy(server, obj);
    else if(IsEqualGUID(class, &CLSID_wine_rowset_proxy))
        return create_rowset_proxy(server, obj);
    else
        FIXME("Unhandled proxy class %s\n", debugstr_guid(class));
    return E_NOTIMPL;
}

/* Marshal impl */

typedef struct
{
    IMarshal IMarshal_iface;

    LONG ref;
    CLSID unmarshal_class;
    IUnknown *outer;
} marshal;

static inline marshal *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, marshal, IMarshal_iface);
}

static HRESULT WINAPI marshal_QueryInterface(IMarshal *iface, REFIID iid, void **obj)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), obj);

    if(IsEqualIID(iid, &IID_IUnknown) ||
       IsEqualIID(iid, &IID_IMarshal))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IMarshal_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI marshal_AddRef(IMarshal *iface)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI marshal_Release(IMarshal *iface)
{
    marshal *This = impl_from_IMarshal(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI marshal_GetUnmarshalClass(IMarshal *iface, REFIID iid, void *obj,
                                                DWORD dwDestContext, void *pvDestContext,
                                                DWORD mshlflags, CLSID *clsid)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%s, %p, %08lx, %p, %08lx, %p)\n", This, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags, clsid);

    *clsid = This->unmarshal_class;
    return S_OK;
}

static HRESULT WINAPI marshal_GetMarshalSizeMax(IMarshal *iface, REFIID iid, void *obj,
                                                DWORD dwDestContext, void *pvDestContext,
                                                DWORD mshlflags, DWORD *size)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%s, %p, %08lx, %p, %08lx, %p)\n", This, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags, size);

    return CoGetMarshalSizeMax(size, &IID_IWineRowServer, This->outer, dwDestContext, pvDestContext,
                               mshlflags);
}

static HRESULT WINAPI marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID iid,
                                               void *obj, DWORD dwDestContext, void *pvDestContext,
                                               DWORD mshlflags)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%p, %s, %p, %08lx, %p, %08lx)\n", This, stream, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags);

    return CoMarshalInterface(stream, &IID_IWineRowServer, This->outer, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI marshal_UnmarshalInterface(IMarshal *iface, IStream *stream,
                                                 REFIID iid, void **obj)
{
    marshal *This = impl_from_IMarshal(iface);
    HRESULT hr;
    IWineRowServer *server;
    IUnknown *proxy;

    TRACE("(%p)->(%p, %s, %p)\n", This, stream, debugstr_guid(iid), obj);
    *obj = NULL;

    hr = CoUnmarshalInterface(stream, &IID_IWineRowServer, (void**)&server);
    if(SUCCEEDED(hr))
    {
        hr = create_proxy(server, &This->unmarshal_class, &proxy);
        if(SUCCEEDED(hr))
        {
            hr = IUnknown_QueryInterface(proxy, iid, obj);
            IUnknown_Release(proxy);
        }
        IWineRowServer_Release(server);
    }

    TRACE("returning %p\n", *obj);
    return hr;
}

static HRESULT WINAPI marshal_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%p)\n", This, stream);
    return CoReleaseMarshalData(stream);
}

static HRESULT WINAPI marshal_DisconnectObject(IMarshal *iface, DWORD dwReserved)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%08lx)\n", This, dwReserved);

    return E_NOTIMPL;
}

static const IMarshalVtbl marshal_vtbl =
{
    marshal_QueryInterface,
    marshal_AddRef,
    marshal_Release,
    marshal_GetUnmarshalClass,
    marshal_GetMarshalSizeMax,
    marshal_MarshalInterface,
    marshal_UnmarshalInterface,
    marshal_ReleaseMarshalData,
    marshal_DisconnectObject
};

static HRESULT create_marshal(IUnknown *outer, const CLSID *class, void **obj)
{
    marshal *marshal;

    TRACE("(%p, %p)\n", outer, obj);
    *obj = NULL;

    marshal = HeapAlloc(GetProcessHeap(), 0, sizeof(*marshal));
    if(!marshal) return E_OUTOFMEMORY;

    marshal->unmarshal_class = *class;
    marshal->outer = outer; /* don't ref outer unk */
    marshal->IMarshal_iface.lpVtbl = &marshal_vtbl;
    marshal->ref = 1;

    *obj = &marshal->IMarshal_iface;
    TRACE("returning %p\n", *obj);
    return S_OK;
}

HRESULT create_row_marshal(IUnknown *outer, void **obj)
{
    TRACE("(%p, %p)\n", outer, obj);
    return create_marshal(outer, &CLSID_wine_row_proxy, obj);
}

HRESULT create_rowset_marshal(IUnknown *outer, void **obj)
{
    TRACE("(%p, %p)\n", outer, obj);
    return create_marshal(outer, &CLSID_wine_rowset_proxy, obj);
}
