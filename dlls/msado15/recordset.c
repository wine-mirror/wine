/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include <assert.h>
#include <math.h>
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "msado15_backcompat.h"
#include "oledb.h"
#include "oledberr.h"
#include "sqlucode.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

#define NO_INTERFACE ((void*)-1)

struct recordset;

struct field
{
    Field               Field_iface;
    ISupportErrorInfo   ISupportErrorInfo_iface;
    Properties          Properties_iface;
    LONG                refs;
    DBORDINAL           ordinal;
    WCHAR              *name;
    DataTypeEnum        type;
    LONG                defined_size;
    LONG                attrs;
    LONG                index;
    unsigned char       prec;
    unsigned char       scale;
    struct recordset   *recordset;
    HACCESSOR           hacc_get;
    HACCESSOR           hacc_put;

    /* Field Properties */
    VARIANT             optimize;
};

struct fields
{
    Fields              Fields_iface;
    ISupportErrorInfo   ISupportErrorInfo_iface;
    LONG                refs;
    struct field      **field;
    int                 count;
    int                 allocated;
};

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
};

struct recordset
{
    _Recordset         Recordset_iface;
    ADORecordsetConstruction ADORecordsetConstruction_iface;
    ISupportErrorInfo  ISupportErrorInfo_iface;
    LONG               refs;
    _Connection       *active_connection;
    LONG               state;
    struct fields      fields;
    CursorLocationEnum cursor_location;
    CursorTypeEnum     cursor_type;
    IRowset           *row_set;
    IRowsetLocate     *rowset_locate;
    IRowsetExactScroll *rowset_es;
    IRowsetChange     *rowset_change;
    IAccessor         *accessor;
    EditModeEnum       editmode;
    HROW               current_row;
    struct
    {
        LONG           max_size;
        LONG           alloc;
        LONG           size;
        LONG           fetched;
        int            dir;
        LONG           pos;
        HROW          *rows;
    } cache;
    VARIANT_BOOL       is_bof;
    VARIANT_BOOL       is_eof;
    ADO_LONGPTR        max_records;

    HACCESSOR          hacc_empty; /* haccessor for adding empty rows */

    HACCESSOR          bookmark_hacc;
    DBTYPE             bookmark_type;
    VARIANT            bookmark;
};

static inline struct field *impl_from_Field( Field *iface )
{
    return CONTAINING_RECORD( iface, struct field, Field_iface );
}

static inline struct field *impl_from_Properties( Properties *iface )
{
    return CONTAINING_RECORD( iface, struct field, Properties_iface );
}

static inline BOOL cache_is_empty( struct recordset *recordset )
{
    return !recordset->cache.fetched;
}

static void cache_release( struct recordset *recordset )
{
    if (cache_is_empty( recordset ))
    {
        if (recordset->current_row)
            IRowset_ReleaseRows( recordset->row_set, 1, &recordset->current_row, NULL, NULL, NULL);
        recordset->current_row = DB_NULL_HROW;
        return;
    }

    IRowset_ReleaseRows( recordset->row_set, recordset->cache.fetched,
            recordset->cache.rows, NULL, NULL, NULL );
    recordset->cache.fetched = 0;
    recordset->cache.dir = 0;
    recordset->cache.pos = 0;
    recordset->current_row = DB_NULL_HROW;
}

static HRESULT get_bookmark( struct recordset *recordset, HROW row, VARIANT *bookmark )
{
    struct bookmark_data bookmark_data = { 0 };
    SAFEARRAY *sa;
    HRESULT hr;

    hr = IRowset_GetData(recordset->row_set, row, recordset->bookmark_hacc, &bookmark_data);
    if (FAILED(hr)) return hr;

    if (recordset->bookmark_type == DBTYPE_I4)
    {
        V_VT(bookmark) = VT_R8;
        V_R8(bookmark) = bookmark_data.val.i4;
        return S_OK;
    }

    if (recordset->bookmark_type == DBTYPE_I8)
    {
        V_VT(bookmark) = VT_I8;
        V_I8(bookmark) = bookmark_data.val.i8;
        return S_OK;
    }

    sa = SafeArrayCreateVector( VT_UI1, 0, bookmark_data.len );
    if (!sa)
    {
        CoTaskMemFree( bookmark_data.val.ptr );
        return E_OUTOFMEMORY;
    }
    hr = SafeArrayLock( sa );
    if (SUCCEEDED(hr))
    {
            memcpy( sa->pvData, bookmark_data.val.ptr, bookmark_data.len );
            SafeArrayUnlock( sa );
    }
    CoTaskMemFree( bookmark_data.val.ptr );
    if (FAILED(hr)) return hr;

    V_VT(bookmark) = VT_ARRAY | VT_UI1;
    V_ARRAY(bookmark) = sa;
    return S_OK;
}

static HRESULT cache_get( struct recordset *recordset, BOOL forward )
{
    int dir = forward ? 1 : -1;
    LONG off, fetch = 0;

    if (!recordset->cache.dir)
    {
        off = 0;
        fetch = dir * recordset->cache.size;
    }
    else if (recordset->cache.dir == dir)
    {
        if (recordset->cache.pos + 1 > recordset->cache.fetched)
        {
            off = 0;
            if (recordset->bookmark_hacc) off += dir;
            fetch = dir * recordset->cache.size;
        }
    }
    else
    {
        if (recordset->cache.pos -1 <= 0)
        {
            off = dir * recordset->cache.fetched;
            fetch = dir * recordset->cache.size;
        }
    }

    if (fetch)
    {
        DBCOUNTITEM count;
        HROW row = 0;
        HRESULT hr;

        if (!cache_is_empty( recordset ))
        {
            if (SUCCEEDED(IRowset_AddRefRows(recordset->row_set, 1, &recordset->current_row, NULL, NULL)))
               row = recordset->current_row;
        }
        cache_release( recordset );
        recordset->current_row = row;

        if (recordset->bookmark_hacc)
        {
            const BYTE *data;
            BYTE byte_buf;
            DBBKMARK len;
            int int_buf;

            if (V_VT(&recordset->bookmark) == VT_R8)
            {
                if (isinf(V_R8(&recordset->bookmark)))
                {
                    data = (BYTE *)&byte_buf;
                    if (V_R8(&recordset->bookmark) < 0)
                    {
                        byte_buf = DBBMK_FIRST;
                        if (!forward) off -= 2;
                    }
                    else
                    {
                        byte_buf = DBBMK_LAST;
                        if (forward) off += 2;
                    }
                    len = sizeof(byte_buf);
                }
                else
                {
                    data = (BYTE *)&int_buf;
                    int_buf = V_R8(&recordset->bookmark);
                    len = sizeof(int_buf);
                }
            }
            else
            {
                hr = SafeArrayLock(V_ARRAY(&recordset->bookmark));
                if (FAILED(hr)) return hr;
                data = V_ARRAY(&recordset->bookmark)->pvData;
                len = V_ARRAY(&recordset->bookmark)->rgsabound[0].cElements;
            }

            hr = IRowsetLocate_GetRowsAt(recordset->rowset_locate, 0, 0, len, data,
                    off, fetch, &count, &recordset->cache.rows);
            if (V_VT(&recordset->bookmark) & VT_ARRAY)
                SafeArrayUnlock(V_ARRAY(&recordset->bookmark));

            if (hr == DB_E_BADSTARTPOSITION)
            {
                count = 0;
                hr  = S_OK;
            }
            else if (hr == DB_S_ENDOFROWSET)
            {
                VariantClear(&recordset->bookmark);
                V_VT(&recordset->bookmark) = VT_R8;
                V_R8(&recordset->bookmark) = dir * INFINITY;
            }
            else if (SUCCEEDED(hr))
            {
                VARIANT tmp;

                hr = get_bookmark(recordset, recordset->cache.rows[count - 1], &tmp);
                if (FAILED(hr))
                {
                    cache_release( recordset );
                    recordset->current_row = row;
                    return hr;
                }

                VariantClear(&recordset->bookmark);
                recordset->bookmark = tmp;
            }
        }
        else
        {
            hr = IRowset_GetNextRows(recordset->row_set, 0, off, fetch, &count, &recordset->cache.rows);
        }
        if (FAILED(hr)) return hr;

        if (recordset->current_row)
        {
            IRowset_ReleaseRows(recordset->row_set, 1, &recordset->current_row, NULL, NULL, NULL);
            recordset->current_row = 0;
        }

        if (!count)
        {
            if (!recordset->is_eof && forward)
            {
                recordset->is_eof = VARIANT_TRUE;
                if (!row) recordset->is_bof = VARIANT_TRUE;
                return S_OK;
            }
            if (!recordset->is_bof && !forward)
            {
                recordset->is_bof = VARIANT_TRUE;
                return S_OK;
            }
            return MAKE_ADO_HRESULT(adErrNoCurrentRecord);
        }


        recordset->cache.pos = 0;
        recordset->cache.fetched = count;
        recordset->cache.dir = dir;
    }

    recordset->is_bof = recordset->is_eof = VARIANT_FALSE;
    if (dir == recordset->cache.dir)
        recordset->current_row = recordset->cache.rows[recordset->cache.pos++];
    else
        recordset->current_row = recordset->cache.rows[--recordset->cache.pos];
    return S_OK;
}

static ULONG WINAPI field_AddRef( Field *iface )
{
    struct field *field = impl_from_Field( iface );
    LONG refs = InterlockedIncrement( &field->refs );
    TRACE( "%p new refcount %ld\n", field, refs );
    return refs;
}

static ULONG WINAPI field_Release( Field *iface )
{
    struct field *field = impl_from_Field( iface );
    LONG refs = InterlockedDecrement( &field->refs );
    TRACE( "%p new refcount %ld\n", field, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", field );
        free( field->name );
        free( field );
    }
    return refs;
}

static HRESULT WINAPI field_QueryInterface( Field *iface, REFIID riid, void **obj )
{
    struct field *field = impl_from_Field( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_Field ) ||
        IsEqualGUID( riid, &IID_Field20 ) ||
        IsEqualGUID( riid, &IID__ADO ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else if (IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *obj = &field->ISupportErrorInfo_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    field_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI field_GetTypeInfoCount( Field *iface, UINT *count )
{
    struct field *field = impl_from_Field( iface );
    TRACE( "%p, %p\n", field, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI field_GetTypeInfo( Field *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct field *field = impl_from_Field( iface );
    TRACE( "%p, %u, %lu, %p\n", field, index, lcid, info );
    return get_typeinfo(Field_tid, info);
}

static HRESULT WINAPI field_GetIDsOfNames( Field *iface, REFIID riid, LPOLESTR *names, UINT count,
                                           LCID lcid, DISPID *dispid )
{
    struct field *field = impl_from_Field( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", field, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Field_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_Invoke( Field *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct field *field = impl_from_Field( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", field, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Field_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &field->Field_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_get_Properties( Field *iface, Properties **obj )
{
    struct field *field = impl_from_Field( iface );
    TRACE( "%p, %p\n", iface, obj );

    *obj = &field->Properties_iface;
    Properties_AddRef(&field->Properties_iface);
    return S_OK;
}

static HRESULT WINAPI field_get_ActualSize( Field *iface, ADO_LONGPTR *size )
{
    struct field *field = impl_from_Field( iface );
    FIXME( "%p, %p\n", field, size );
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI field_get_Attributes( Field *iface, LONG *attrs )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", field, attrs );

    *attrs = field->attrs;
    return S_OK;
}

static HRESULT WINAPI field_get_DefinedSize( Field *iface, ADO_LONGPTR *size )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", field, size );

    *size = field->defined_size;
    return S_OK;
}

static HRESULT WINAPI field_get_Name( Field *iface, BSTR *str )
{
    struct field *field = impl_from_Field( iface );
    BSTR name;

    TRACE( "%p, %p\n", field, str );

    if (!(name = SysAllocString( field->name ))) return E_OUTOFMEMORY;
    *str = name;
    return S_OK;
}

static HRESULT WINAPI field_get_Type( Field *iface, DataTypeEnum *type )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", field, type );

    *type = field->type;
    return S_OK;
}

static HRESULT WINAPI field_get_Value( Field *iface, VARIANT *val )
{
    struct field *field = impl_from_Field( iface );
    struct recordset *recordset = field->recordset;
    struct buf
    {
        VARIANT val;
        DBSTATUS status;
    } buf;
    HRESULT hr;

    TRACE( "%p, %p\n", field, val );

    if (!recordset || recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->is_eof && !recordset->is_bof && !recordset->current_row)
    {
        hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }
    if (!recordset->current_row) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    if (!recordset->accessor)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IAccessor, (void **)&recordset->accessor );
        if (FAILED(hr) || !recordset->accessor)
            recordset->accessor = NO_INTERFACE;
    }
    if (recordset->accessor == NO_INTERFACE)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    if (!field->hacc_get)
    {
        DBBINDSTATUS status = DBBINDSTATUS_OK;
        DBBINDING binding;

        memset(&binding, 0, sizeof(binding));
        binding.iOrdinal = field->ordinal;
        binding.obStatus = offsetof(struct buf, status);
        binding.dwPart = DBPART_VALUE | DBPART_STATUS;
        binding.cbMaxLen = sizeof(buf.val);
        binding.wType = DBTYPE_VARIANT;
        binding.bPrecision = field->prec;
        binding.bScale = field->scale;
        hr = IAccessor_CreateAccessor( recordset->accessor, DBACCESSOR_ROWDATA,
            1, &binding, 0, &field->hacc_get, &status );
        if (FAILED(hr)) return hr;
        if (status != DBBINDSTATUS_OK)
        {
            IAccessor_ReleaseAccessor( recordset->accessor, field->hacc_get, NULL );
            field->hacc_get = 0;
            return E_FAIL;
        }
    }

    memset(&buf, 0, sizeof(buf));
    hr = IRowset_GetData(recordset->row_set, recordset->current_row, field->hacc_get, &buf);
    if (FAILED(hr)) return hr;
    if (buf.status != DBSTATUS_S_OK) return E_FAIL;

    *val = buf.val;
    return S_OK;
}

static HRESULT WINAPI field_put_Value( Field *iface, VARIANT val )
{
    struct field *field = impl_from_Field( iface );
    struct recordset *recordset = field->recordset;
    struct buf
    {
        VARIANT val;
        DBSTATUS status;
        DBLENGTH len;
    } buf;
    HRESULT hr;

    TRACE( "%p, %s\n", field, debugstr_variant(&val) );

    if (!recordset || recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->is_eof && !recordset->is_bof && !recordset->current_row)
    {
        hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }
    if (!recordset->current_row) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    if (!recordset->rowset_change)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IRowsetChange,
                (void **)&recordset->rowset_change );
        if (FAILED(hr) || !recordset->rowset_change)
            recordset->rowset_change = NO_INTERFACE;
    }
    if (recordset->rowset_change == NO_INTERFACE)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    if (!recordset->accessor)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IAccessor, (void **)&recordset->accessor );
        if (FAILED(hr) || !recordset->accessor)
            recordset->accessor = NO_INTERFACE;
    }
    if (recordset->accessor == NO_INTERFACE)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    if (!field->hacc_put)
    {
        DBBINDSTATUS status = DBBINDSTATUS_OK;
        DBBINDING binding;

        memset(&binding, 0, sizeof(binding));
        binding.iOrdinal = field->ordinal;
        binding.obLength = offsetof(struct buf, len);
        binding.obStatus = offsetof(struct buf, status);
        binding.dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
        binding.cbMaxLen = sizeof(buf.val);
        binding.wType = DBTYPE_VARIANT;
        binding.bPrecision = field->prec;
        binding.bScale = field->scale;
        hr = IAccessor_CreateAccessor( recordset->accessor, DBACCESSOR_ROWDATA,
            1, &binding, 0, &field->hacc_put, &status );
        if (FAILED(hr)) return hr;
        if (status != DBBINDSTATUS_OK)
        {
            IAccessor_ReleaseAccessor( recordset->accessor, field->hacc_put, NULL );
            field->hacc_put = 0;
            return E_FAIL;
        }
    }

    memset(&buf, 0, sizeof(buf));
    buf.val = val;
    hr = IRowsetChange_SetData(recordset->rowset_change, recordset->current_row, field->hacc_put, &buf);
    if (FAILED(hr)) return hr;
    if (buf.status != DBSTATUS_S_OK) return E_FAIL;
    return S_OK;
}

static HRESULT WINAPI field_get_Precision( Field *iface, unsigned char *precision )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", iface, precision );

    *precision = field->prec;
    return S_OK;
}

static HRESULT WINAPI field_get_NumericScale( Field *iface, unsigned char *scale )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", iface, scale );

    *scale = field->scale;
    return S_OK;
}

static HRESULT WINAPI field_AppendChunk( Field *iface, VARIANT data )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&data) );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_GetChunk( Field *iface, LONG length, VARIANT *var )
{
    FIXME( "%p, %ld, %p\n", iface, length, var );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_get_OriginalValue( Field *iface, VARIANT *val )
{
    FIXME( "%p, %p\n", iface, val );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_get_UnderlyingValue( Field *iface, VARIANT *val )
{
    FIXME( "%p, %p\n", iface, val );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_get_DataFormat( Field *iface, IUnknown **format )
{
    FIXME( "%p, %p\n", iface, format );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_putref_DataFormat( Field *iface, IUnknown *format )
{
    FIXME( "%p, %p\n", iface, format );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_put_Precision( Field *iface, unsigned char precision )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %u\n", iface, precision );

    if (field->recordset->state != adStateClosed) return MAKE_ADO_HRESULT( adErrObjectOpen );

    field->prec = precision;
    return S_OK;
}

static HRESULT WINAPI field_put_NumericScale( Field *iface, unsigned char scale )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %u\n", iface, scale );

    if (field->recordset->state != adStateClosed) return MAKE_ADO_HRESULT( adErrObjectOpen );

    field->scale = scale;
    return S_OK;
}

static HRESULT WINAPI field_put_Type( Field *iface, DataTypeEnum type )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %u\n", field, type );

    field->type = type;
    return S_OK;
}

static HRESULT WINAPI field_put_DefinedSize( Field *iface, ADO_LONGPTR size )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %Id\n", field, size );

    field->defined_size = size;
    return S_OK;
}

static HRESULT WINAPI field_put_Attributes( Field *iface, LONG attrs )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %ld\n", field, attrs );

    field->attrs = attrs;
    return S_OK;
}

static HRESULT WINAPI field_get_Status( Field *iface, LONG *status )
{
    FIXME( "%p, %p\n", iface, status );
    return E_NOTIMPL;
}

static const struct FieldVtbl field_vtbl =
{
    field_QueryInterface,
    field_AddRef,
    field_Release,
    field_GetTypeInfoCount,
    field_GetTypeInfo,
    field_GetIDsOfNames,
    field_Invoke,
    field_get_Properties,
    field_get_ActualSize,
    field_get_Attributes,
    field_get_DefinedSize,
    field_get_Name,
    field_get_Type,
    field_get_Value,
    field_put_Value,
    field_get_Precision,
    field_get_NumericScale,
    field_AppendChunk,
    field_GetChunk,
    field_get_OriginalValue,
    field_get_UnderlyingValue,
    field_get_DataFormat,
    field_putref_DataFormat,
    field_put_Precision,
    field_put_NumericScale,
    field_put_Type,
    field_put_DefinedSize,
    field_put_Attributes,
    field_get_Status
};

static inline struct field *field_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return CONTAINING_RECORD( iface, struct field, ISupportErrorInfo_iface );
}

static HRESULT WINAPI field_supporterrorinfo_QueryInterface( ISupportErrorInfo *iface, REFIID riid, void **obj )
{
    struct field *field = field_from_ISupportErrorInfo( iface );
    return Field_QueryInterface( &field->Field_iface, riid, obj );
}

static ULONG WINAPI field_supporterrorinfo_AddRef( ISupportErrorInfo *iface )
{
    struct field *field = field_from_ISupportErrorInfo( iface );
    return Field_AddRef( &field->Field_iface );
}

static ULONG WINAPI field_supporterrorinfo_Release( ISupportErrorInfo *iface )
{
    struct field *field = field_from_ISupportErrorInfo( iface );
    return Field_Release( &field->Field_iface );
}

static HRESULT WINAPI field_supporterrorinfo_InterfaceSupportsErrorInfo( ISupportErrorInfo *iface, REFIID riid )
{
    struct field *field = field_from_ISupportErrorInfo( iface );
    FIXME( "%p, %s\n", field, debugstr_guid(riid) );
    return S_FALSE;
}

static const ISupportErrorInfoVtbl field_supporterrorinfo_vtbl =
{
    field_supporterrorinfo_QueryInterface,
    field_supporterrorinfo_AddRef,
    field_supporterrorinfo_Release,
    field_supporterrorinfo_InterfaceSupportsErrorInfo
};

static HRESULT WINAPI field_props_QueryInterface(Properties *iface, REFIID riid, void **ppv)
{
    struct field *field = impl_from_Properties( iface );

    if (IsEqualGUID( riid, &IID_Properties) || IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppv = &field->Properties_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    Field_AddRef(&field->Field_iface);
    return S_OK;
}

static ULONG WINAPI field_props_AddRef(Properties *iface)
{
    struct field *field = impl_from_Properties( iface );
    return Field_AddRef(&field->Field_iface);
}

static ULONG WINAPI field_props_Release(Properties *iface)
{
    struct field *field = impl_from_Properties( iface );
    return Field_Release(&field->Field_iface);
}

static HRESULT WINAPI field_props_GetTypeInfoCount(Properties *iface, UINT *count)
{
    struct field *field = impl_from_Properties( iface );
    TRACE( "%p, %p\n", field, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI field_props_GetTypeInfo(Properties *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    struct field *field = impl_from_Properties( iface );
    TRACE( "%p, %u, %lu, %p\n", field, index, lcid, info );
    return get_typeinfo(Properties_tid, info);
}

static HRESULT WINAPI field_props_GetIDsOfNames(Properties *iface, REFIID riid, LPOLESTR *names, UINT count,
                                           LCID lcid, DISPID *dispid )
{
    struct field *field = impl_from_Properties( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", field, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Properties_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_props_Invoke(Properties *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct field *field = impl_from_Properties( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", field, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Properties_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &field->Field_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_props_get_Count(Properties *iface, LONG *count)
{
    struct field *field = impl_from_Properties( iface );
    FIXME( "%p, %p\n", field, count);
    *count = 0;
    return S_OK;
}

static HRESULT WINAPI field_props__NewEnum(Properties *iface, IUnknown **object)
{
    struct field *field = impl_from_Properties( iface );
    FIXME( "%p, %p\n", field, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI field_props_Refresh(Properties *iface)
{
    struct field *field = impl_from_Properties( iface );
    FIXME( "%p\n", field);
    return E_NOTIMPL;
}


struct field_property
{
    Property Property_iface;
    LONG refs;
    VARIANT *value;
};

static inline struct field_property *impl_from_Property( Property *iface )
{
    return CONTAINING_RECORD( iface, struct field_property, Property_iface );
}

static ULONG WINAPI field_property_AddRef(Property *iface)
{
    struct field_property *property = impl_from_Property( iface );
    LONG refs = InterlockedIncrement( &property->refs );
    TRACE( "%p new refcount %ld\n", property, refs );
    return refs;
}

static ULONG WINAPI field_property_Release(Property *iface)
{
    struct field_property *property = impl_from_Property( iface );
    LONG refs = InterlockedDecrement( &property->refs );
    TRACE( "%p new refcount %ld\n", property, refs );
    if (!refs)
    {
        free( property );
    }
    return refs;
}

static HRESULT WINAPI field_property_QueryInterface(Property *iface, REFIID riid, void **obj)
{
    struct field_property *property = impl_from_Property( iface );
    TRACE( "%p, %s, %p\n", property, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_Property )
        || IsEqualGUID( riid, &IID_IDispatch )
        || IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    field_property_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI field_property_GetTypeInfoCount(Property *iface, UINT *count)
{
    struct field_property *property = impl_from_Property( iface );
    TRACE( "%p, %p\n", property, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI field_property_GetTypeInfo(Property *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    struct field_property *property = impl_from_Property( iface );
    TRACE( "%p, %u, %lu, %p\n", property, index, lcid, info );
    return get_typeinfo(Property_tid, info);
}

static HRESULT WINAPI field_property_GetIDsOfNames(Property *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                    LCID lcid, DISPID *dispid)
{
    struct field_property *property = impl_from_Property( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", property, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Property_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_property_Invoke(Property *iface, DISPID member, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err)
{
    struct field_property *property = impl_from_Property( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", property, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Property_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &property->Property_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI field_property_get_Value(Property *iface, VARIANT *val)
{
    struct field_property *property = impl_from_Property( iface );
    TRACE("%p, %p\n", property, val);
    VariantCopy(val, property->value);
    return S_OK;
}

static HRESULT WINAPI field_property_put_Value(Property *iface, VARIANT val)
{
    struct field_property *property = impl_from_Property( iface );
    TRACE("%p, %s\n", property, debugstr_variant(&val));
    VariantCopy(property->value, &val);
    return S_OK;
}

static HRESULT WINAPI field_property_get_Name(Property *iface, BSTR *str)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI field_property_get_Type(Property *iface, DataTypeEnum *type)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI field_property_get_Attributes(Property *iface, LONG *attributes)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI field_property_put_Attributes(Property *iface, LONG attributes)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static struct PropertyVtbl field_property_vtbl =
{
    field_property_QueryInterface,
    field_property_AddRef,
    field_property_Release,
    field_property_GetTypeInfoCount,
    field_property_GetTypeInfo,
    field_property_GetIDsOfNames,
    field_property_Invoke,
    field_property_get_Value,
    field_property_put_Value,
    field_property_get_Name,
    field_property_get_Type,
    field_property_get_Attributes,
    field_property_put_Attributes
};

static HRESULT WINAPI field_props_get_Item(Properties *iface, VARIANT index, Property **object)
{
    struct field *field = impl_from_Properties( iface );
    struct field_property *prop;

    TRACE( "%p, %s, %p\n", field, debugstr_variant(&index), object);

    if (V_VT(&index) == VT_BSTR)
    {
        if(!wcscmp(L"Optimize", V_BSTR(&index)))
        {
            prop = malloc (sizeof(struct field_property));
            prop->Property_iface.lpVtbl = &field_property_vtbl;
            prop->value = &field->optimize;

            *object = &prop->Property_iface;
            return S_OK;
        }
    }

    FIXME("Unsupported property %s\n", debugstr_variant(&index));

    return MAKE_ADO_HRESULT(adErrItemNotFound);
}

static struct PropertiesVtbl field_properties_vtbl =
{
    field_props_QueryInterface,
    field_props_AddRef,
    field_props_Release,
    field_props_GetTypeInfoCount,
    field_props_GetTypeInfo,
    field_props_GetIDsOfNames,
    field_props_Invoke,
    field_props_get_Count,
    field_props__NewEnum,
    field_props_Refresh,
    field_props_get_Item
};

static HRESULT Field_create( const WCHAR *name, LONG index, struct recordset *recordset, struct field **field )
{
    if (!(*field = calloc( 1, sizeof(**field) ))) return E_OUTOFMEMORY;
    (*field)->Field_iface.lpVtbl = &field_vtbl;
    (*field)->ISupportErrorInfo_iface.lpVtbl = &field_supporterrorinfo_vtbl;
    (*field)->Properties_iface.lpVtbl = &field_properties_vtbl;
    (*field)->refs = 1;
    if (!((*field)->name = wcsdup( name )))
    {
        free( *field );
        return E_OUTOFMEMORY;
    }
    (*field)->index = index;
    (*field)->recordset = recordset;

    TRACE( "returning field %p\n", *field );
    return S_OK;
}

static inline struct fields *impl_from_Fields( Fields *iface )
{
    return CONTAINING_RECORD( iface, struct fields, Fields_iface );
}

static inline struct recordset *fields_get_recordset( struct fields *fields )
{
    return CONTAINING_RECORD( fields, struct recordset, fields );
}

static ULONG WINAPI fields_AddRef( Fields *iface )
{
    struct fields *fields = impl_from_Fields( iface );
    LONG refs = InterlockedIncrement( &fields->refs );

    TRACE( "%p new refcount %ld\n", fields, refs );

    if (refs == 1) _Recordset_AddRef( &fields_get_recordset(fields)->Recordset_iface );
    return refs;
}

static ULONG WINAPI fields_Release( Fields *iface )
{
    struct fields *fields = impl_from_Fields( iface );
    LONG refs = InterlockedDecrement( &fields->refs );

    TRACE( "%p new refcount %ld\n", fields, refs );

    if (!refs) _Recordset_Release( &fields_get_recordset(fields)->Recordset_iface );
    return refs < 1 ? 1 : refs;
}

static HRESULT WINAPI fields_QueryInterface( Fields *iface, REFIID riid, void **obj )
{
    struct fields *fields = impl_from_Fields( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_Fields ) ||
        IsEqualGUID( riid, &IID_Fields20 ) ||
        IsEqualGUID( riid, &IID_Fields15 ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else if (IsEqualGUID( riid, &IID__Collection ))
    {
        TRACE( "interface _Collection cannot be queried returning NULL\n" );
        return E_NOINTERFACE;
    }
    else if (IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *obj = &fields->ISupportErrorInfo_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    fields_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fields_GetTypeInfoCount( Fields *iface, UINT *count )
{
    struct fields *fields = impl_from_Fields( iface );
    TRACE( "%p, %p\n", fields, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI fields_GetTypeInfo( Fields *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct fields *fields = impl_from_Fields( iface );
    TRACE( "%p, %u, %lu, %p\n", fields, index, lcid, info );
    return get_typeinfo(Fields_tid, info);
}

static HRESULT WINAPI fields_GetIDsOfNames( Fields *iface, REFIID riid, LPOLESTR *names, UINT count,
                                            LCID lcid, DISPID *dispid )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", fields, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Fields_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI fields_Invoke( Fields *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", fields, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Fields_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &fields->Fields_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static BOOL resize_fields( struct fields *fields, ULONG count )
{
    if (count > fields->allocated)
    {
        struct field **tmp;
        ULONG new_size = max( count, fields->allocated * 2 );
        if (!(tmp = realloc( fields->field, new_size * sizeof(*tmp) ))) return FALSE;
        fields->field = tmp;
        fields->allocated = new_size;
    }

    return TRUE;
}

static HRESULT append_field( struct fields *fields, const DBCOLUMNINFO *info )
{
    struct field *field;
    HRESULT hr;

    hr = Field_create( info->pwszName, fields->count, fields_get_recordset(fields), &field );
    if (hr != S_OK) return hr;
    field->ordinal = info->iOrdinal;
    field->type = info->wType;
    field->defined_size = info->ulColumnSize;
    if (info->dwFlags != adFldUnspecified) field->attrs = info->dwFlags;
    field->prec = info->bPrecision;
    field->scale = info->bScale;

    if (!(resize_fields( fields, fields->count + 1 )))
    {
        Field_Release( &field->Field_iface );
        return E_OUTOFMEMORY;
    }

    fields->field[fields->count++] = field;
    return S_OK;
}

static HRESULT init_fields( struct fields *fields )
{
    struct recordset *rec = fields_get_recordset( fields );
    DBCOLUMNINFO *colinfo = NULL;
    OLECHAR *strbuf = NULL;
    DBORDINAL i, columns;
    IColumnsInfo *info;
    HRESULT hr;

    if (fields->count != -1) return S_OK;

    if (!rec->row_set)
    {
        fields->count = 0;
        return S_OK;
    }

    hr = IRowset_QueryInterface( rec->row_set, &IID_IColumnsInfo, (void **)&info );
    if (FAILED(hr) || !info)
    {
        fields->count = 0;
        return S_OK;
    }

    hr = IColumnsInfo_GetColumnInfo( info, &columns, &colinfo, &strbuf );
    IColumnsInfo_Release( info );
    if (FAILED(hr)) return hr;

    if (!resize_fields( fields, columns ))
    {
        CoTaskMemFree( colinfo );
        CoTaskMemFree( strbuf );
        return E_OUTOFMEMORY;
    }

    fields->count = 0;
    for (i = 0; i < columns; i++)
    {
        TRACE("Adding Column %Iu, pwszName: %s, pTypeInfo %p, iOrdinal %Iu, dwFlags 0x%08lx, "
                "ulColumnSize %Iu, wType %d, bPrecision %d, bScale %d\n",
                i, debugstr_w(colinfo[i].pwszName), colinfo[i].pTypeInfo, colinfo[i].iOrdinal,
                colinfo[i].dwFlags, colinfo[i].ulColumnSize, colinfo[i].wType,
                colinfo[i].bPrecision, colinfo[i].bScale);

        if (!i && colinfo[i].columnid.eKind == DBKIND_GUID_PROPID &&
                IsEqualGUID(&colinfo[i].columnid.uGuid.guid, &DBCOL_SPECIALCOL)) continue;

        hr = append_field(fields, &colinfo[i]);
        if (FAILED(hr))
        {
            for (; i > 0; i--)
                Field_Release(&fields->field[i - 1]->Field_iface);
            fields->count = -1;

            CoTaskMemFree( colinfo );
            CoTaskMemFree( strbuf );

            ERR("Failed to add Field name - 0x%08lx\n", hr);
            return hr;
        }
    }

    CoTaskMemFree( colinfo );
    CoTaskMemFree( strbuf );
    return S_OK;
}

static HRESULT WINAPI fields_get_Count( Fields *iface, LONG *count )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;

    TRACE( "%p, %p\n", fields, count );

    if ((hr = init_fields( fields )) != S_OK) return hr;
    *count = fields->count;
    return S_OK;
}

static HRESULT WINAPI fields__NewEnum( Fields *iface, IUnknown **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_Refresh( Fields *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT map_index( struct fields *fields, VARIANT *index, ULONG *ret )
{
    ULONG i;

    if (V_VT( index ) != VT_BSTR)
    {
        VARIANT idx;

        VariantInit(&idx);
        if (VariantChangeType(&idx, index, 0, VT_UI4) == S_OK)
        {
            i = V_UI4 ( &idx );
            if (i < fields->count)
            {
                *ret = i;
                return S_OK;
            }
        }

        return MAKE_ADO_HRESULT(adErrItemNotFound);
    }

    for (i = 0; i < fields->count; i++)
    {
        if (!wcsicmp( V_BSTR(index), fields->field[i]->name ))
        {
            *ret = i;
            return S_OK;
        }
    }

    return MAKE_ADO_HRESULT(adErrItemNotFound);
}

static HRESULT WINAPI fields_get_Item( Fields *iface, VARIANT index, Field **obj )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;
    ULONG i = 0;

    TRACE( "%p, %s, %p\n", fields, debugstr_variant(&index), obj );

    if ((hr = init_fields( fields )) != S_OK) return hr;
    if ((hr = map_index( fields, &index, &i )) != S_OK) return hr;

    Field_AddRef( &fields->field[i]->Field_iface );
    *obj = &fields->field[i]->Field_iface;
    return S_OK;
}

static HRESULT WINAPI fields__Append( Fields *iface, BSTR name, DataTypeEnum type, ADO_LONGPTR size, FieldAttributeEnum attr )
{
    struct fields *fields = impl_from_Fields( iface );
    DBCOLUMNINFO colinfo;
    HRESULT hr;

    TRACE( "%p, %s, %u, %Id, %d\n", fields, debugstr_w(name), type, size, attr );

    if (fields_get_recordset(fields)->state != adStateClosed) return MAKE_ADO_HRESULT( adErrIllegalOperation );
    if ((hr = init_fields( fields )) != S_OK) return hr;

    memset( &colinfo, 0, sizeof(colinfo) );
    colinfo.iOrdinal = fields->count ? (*fields->field)[fields->count - 1].ordinal + 1 : 1;
    colinfo.pwszName = name;
    colinfo.wType = type;
    colinfo.ulColumnSize = size;
    colinfo.dwFlags = attr;
    return append_field( fields, &colinfo );
}

static HRESULT WINAPI fields_Delete( Fields *iface, VARIANT index )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&index) );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_Append( Fields *iface, BSTR name, DataTypeEnum type, ADO_LONGPTR size, FieldAttributeEnum attr,
                                     VARIANT value )
{
    FIXME( "ignoring value %s\n", debugstr_variant(&value) );
    return Fields__Append( iface, name, type, size, attr );
}

static HRESULT WINAPI fields_Update( Fields *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_Resync( Fields *iface, ResyncEnum resync_values )
{
    FIXME( "%p, %u\n", iface, resync_values );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_CancelUpdate( Fields *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static const struct FieldsVtbl fields_vtbl =
{
    fields_QueryInterface,
    fields_AddRef,
    fields_Release,
    fields_GetTypeInfoCount,
    fields_GetTypeInfo,
    fields_GetIDsOfNames,
    fields_Invoke,
    fields_get_Count,
    fields__NewEnum,
    fields_Refresh,
    fields_get_Item,
    fields__Append,
    fields_Delete,
    fields_Append,
    fields_Update,
    fields_Resync,
    fields_CancelUpdate
};

static inline struct fields *fields_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return CONTAINING_RECORD( iface, struct fields, ISupportErrorInfo_iface );
}

static HRESULT WINAPI fields_supporterrorinfo_QueryInterface( ISupportErrorInfo *iface, REFIID riid, void **obj )
{
    struct fields *fields = fields_from_ISupportErrorInfo( iface );
    return Fields_QueryInterface( &fields->Fields_iface, riid, obj );
}

static ULONG WINAPI fields_supporterrorinfo_AddRef( ISupportErrorInfo *iface )
{
    struct fields *fields = fields_from_ISupportErrorInfo( iface );
    return Fields_AddRef( &fields->Fields_iface );
}

static ULONG WINAPI fields_supporterrorinfo_Release( ISupportErrorInfo *iface )
{
    struct fields *fields = fields_from_ISupportErrorInfo( iface );
    return Fields_Release( &fields->Fields_iface );
}

static HRESULT WINAPI fields_supporterrorinfo_InterfaceSupportsErrorInfo( ISupportErrorInfo *iface, REFIID riid )
{
    struct fields *fields = fields_from_ISupportErrorInfo( iface );
    FIXME( "%p, %s\n", fields, debugstr_guid(riid) );
    return S_FALSE;
}

static const ISupportErrorInfoVtbl fields_supporterrorinfo_vtbl =
{
    fields_supporterrorinfo_QueryInterface,
    fields_supporterrorinfo_AddRef,
    fields_supporterrorinfo_Release,
    fields_supporterrorinfo_InterfaceSupportsErrorInfo
};

static void Fields_create( struct recordset *recordset )
{
    memset( &recordset->fields, 0, sizeof(recordset->fields) );
    recordset->fields.Fields_iface.lpVtbl = &fields_vtbl;
    recordset->fields.ISupportErrorInfo_iface.lpVtbl = &fields_supporterrorinfo_vtbl;
    recordset->fields.count = -1;
}

static inline struct recordset *impl_from_Recordset( _Recordset *iface )
{
    return CONTAINING_RECORD( iface, struct recordset, Recordset_iface );
}

static inline struct recordset *impl_from_ADORecordsetConstruction( ADORecordsetConstruction *iface )
{
    return CONTAINING_RECORD( iface, struct recordset, ADORecordsetConstruction_iface );
}

static ULONG WINAPI recordset_AddRef( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    LONG refs = InterlockedIncrement( &recordset->refs );
    TRACE( "%p new refcount %ld\n", recordset, refs );
    return refs;
}

static void close_recordset( struct recordset *recordset )
{
    int i;

    cache_release( recordset );
    recordset->is_bof = recordset->is_eof = VARIANT_FALSE;
    if ( recordset->hacc_empty )
    {
        IAccessor_ReleaseAccessor( recordset->accessor, recordset->hacc_empty, NULL );
        recordset->hacc_empty = 0;
    }

    if (recordset->bookmark_hacc)
    {
        IAccessor_ReleaseAccessor( recordset->accessor, recordset->bookmark_hacc, NULL );
        recordset->bookmark_hacc = 0;
    }
    VariantClear( &recordset->bookmark );

    if ( recordset->row_set ) IRowset_Release( recordset->row_set );
    recordset->row_set = NULL;
    if ( recordset->rowset_locate )
        IRowsetLocate_Release( recordset->rowset_locate );
    recordset->rowset_locate = NULL;
    if ( recordset->rowset_es && recordset->rowset_es != NO_INTERFACE )
        IRowsetExactScroll_Release( recordset->rowset_es );
    recordset->rowset_es = NULL;
    if ( recordset->rowset_change && recordset->rowset_change != NO_INTERFACE )
        IRowsetChange_Release( recordset->rowset_change );
    recordset->rowset_change = NULL;

    for (i = 0; i < recordset->fields.count; i++)
    {
        if (recordset->fields.field[i]->hacc_get)
        {
            IAccessor_ReleaseAccessor(recordset->accessor, recordset->fields.field[i]->hacc_get, NULL);
            recordset->fields.field[i]->hacc_get = 0;
        }
        if (recordset->fields.field[i]->hacc_put)
        {
            IAccessor_ReleaseAccessor(recordset->accessor, recordset->fields.field[i]->hacc_put, NULL);
            recordset->fields.field[i]->hacc_put = 0;
        }
        recordset->fields.field[i]->recordset = NULL;

        Field_Release(&recordset->fields.field[i]->Field_iface);
    }
    recordset->fields.count = -1;

    if (recordset->accessor && recordset->accessor != NO_INTERFACE )
        IAccessor_Release( recordset->accessor );
    recordset->accessor = NULL;
}

static ULONG WINAPI recordset_Release( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    LONG refs = InterlockedDecrement( &recordset->refs );
    TRACE( "%p new refcount %ld\n", recordset, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", recordset );
        close_recordset( recordset );
        free( recordset->fields.field );
        free( recordset->cache.rows );
        free( recordset );
    }
    return refs;
}

static HRESULT WINAPI recordset_QueryInterface( _Recordset *iface, REFIID riid, void **obj )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)    || IsEqualIID(riid, &IID_IDispatch)   ||
        IsEqualIID(riid, &IID__ADO)        || IsEqualIID(riid, &IID_Recordset15) ||
        IsEqualIID(riid, &IID_Recordset20) || IsEqualIID(riid, &IID_Recordset21) ||
        IsEqualIID(riid, &IID__Recordset))
    {
        *obj = iface;
    }
    else if (IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *obj = &recordset->ISupportErrorInfo_iface;
    }
    else if (IsEqualGUID( riid, &IID_ADORecordsetConstruction ))
    {
        *obj = &recordset->ADORecordsetConstruction_iface;
    }
    else if (IsEqualGUID( riid, &IID_IRunnableObject ))
    {
        TRACE("IID_IRunnableObject not supported returning NULL\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    recordset_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI recordset_GetTypeInfoCount( _Recordset *iface, UINT *count )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", recordset, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI recordset_GetTypeInfo( _Recordset *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %u, %lu, %p\n", recordset, index, lcid, info );
    return get_typeinfo(Recordset_tid, info);
}

static HRESULT WINAPI recordset_GetIDsOfNames( _Recordset *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", recordset, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Recordset_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI recordset_Invoke( _Recordset *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", recordset, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Recordset_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &recordset->Recordset_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI recordset_get_Properties( _Recordset *iface, Properties **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_AbsolutePosition( _Recordset *iface, PositionEnum_Param *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_AbsolutePosition( _Recordset *iface, PositionEnum_Param pos )
{
    FIXME( "%p, %Id\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_ActiveConnection( _Recordset *iface, IDispatch *connection )
{
    VARIANT v;

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = connection;
    return _Recordset_put_ActiveConnection( iface, v );
}

static HRESULT WINAPI recordset_put_ActiveConnection( _Recordset *iface, VARIANT connection )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    _Connection *conn;
    LONG state;
    HRESULT hr;

    TRACE( "%p, %s\n", iface, debugstr_variant(&connection) );

    switch( V_VT(&connection) )
    {
    case VT_BSTR:
        hr = Connection_create( (void **)&conn );
        if (FAILED(hr)) return hr;
        hr = _Connection_Open( conn, V_BSTR(&connection), NULL, NULL, adConnectUnspecified );
        if (FAILED(hr))
        {
            _Connection_Release( conn );
            return hr;
        }
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        if (!V_UNKNOWN(&connection)) return MAKE_ADO_HRESULT( adErrInvalidConnection );
        hr = IUnknown_QueryInterface( V_UNKNOWN(&connection), &IID__Connection, (void **)&conn );
        if (FAILED(hr)) return hr;
        hr = _Connection_get_State( conn, &state );
        if (SUCCEEDED(hr) && state != adStateOpen)
            hr = MAKE_ADO_HRESULT( adErrInvalidConnection );
        if (FAILED(hr))
        {
            _Connection_Release( conn );
            return hr;
        }
        break;

    default:
        FIXME( "unsupported connection %s\n", debugstr_variant(&connection) );
        return E_NOTIMPL;
    }

    if (recordset->active_connection) _Connection_Release( recordset->active_connection );
    recordset->active_connection = conn;
    return S_OK;
}

static HRESULT WINAPI recordset_get_ActiveConnection( _Recordset *iface, VARIANT *connection )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", iface, connection );

    V_VT(connection) = VT_DISPATCH;
    V_DISPATCH(connection) = (IDispatch *)recordset->active_connection;
    if (recordset->active_connection) _Connection_AddRef( recordset->active_connection );
    return S_OK;
}

static HRESULT WINAPI recordset_get_BOF( _Recordset *iface, VARIANT_BOOL *bof )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, bof );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->is_bof && !recordset->is_eof && !recordset->current_row)
    {
        hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }

    *bof = recordset->is_bof;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Bookmark( _Recordset *iface, VARIANT *bookmark )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", iface, bookmark );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (!recordset->current_row && !recordset->is_eof && !recordset->is_bof)
    {
        HRESULT hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }
    if (!recordset->current_row) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );
    if (!recordset->bookmark_hacc)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    return get_bookmark( recordset, recordset->current_row, bookmark );
}

static HRESULT WINAPI recordset_put_Bookmark( _Recordset *iface, VARIANT bookmark )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    VARIANT copy;
    HRESULT hr;

    TRACE( "%p, %s\n", iface, debugstr_variant(&bookmark) );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (V_VT(&bookmark) != VT_R8 && V_VT(&bookmark) != VT_I8 &&
            V_VT(&bookmark) != (VT_ARRAY | VT_UI1))
        return MAKE_ADO_HRESULT( adErrInvalidArgument );
    if (!recordset->bookmark_hacc)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    VariantInit( &copy );
    hr = VariantCopy( &copy, &bookmark );
    if (FAILED(hr)) return hr;

    VariantClear( &recordset->bookmark );
    recordset->bookmark = copy;
    recordset->cache.dir = 0;
    return cache_get( recordset, TRUE );
}

static HRESULT WINAPI recordset_get_CacheSize( _Recordset *iface, LONG *size )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, size );

    *size = recordset->cache.size;
    return S_OK;
}

static HRESULT WINAPI recordset_put_CacheSize( _Recordset *iface, LONG size )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %ld\n", iface, size );

    if (size < 1) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if (!recordset->cache.max_size && recordset->row_set)
    {
        DBPROPSET *propset = NULL;
        IRowsetInfo *info;
        HRESULT hr;

        recordset->cache.max_size = LONG_MAX;

        hr = IRowset_QueryInterface( recordset->row_set, &IID_IRowsetInfo, (void **)&info );
        if (SUCCEEDED( hr ))
        {
            DBPROPIDSET propidset;
            DBPROPID id[1];
            ULONG count;

            propidset.rgPropertyIDs = id;
            propidset.cPropertyIDs = ARRAY_SIZE(id);
            propidset.guidPropertySet = DBPROPSET_ROWSET;
            id[0] = DBPROP_MAXOPENROWS;
            hr = IRowsetInfo_GetProperties( info, 1, &propidset, &count, &propset );
            IRowsetInfo_Release( info );
            if (FAILED( hr )) propset = NULL;
        }
        if (propset)
        {
            if (V_VT(&propset->rgProperties[0].vValue) == VT_I4)
                recordset->cache.max_size = V_I4(&propset->rgProperties[0].vValue);
            CoTaskMemFree( propset->rgProperties );
            CoTaskMemFree( propset );
        }
    }

    if (recordset->cache.max_size && recordset->cache.max_size < size)
        return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if (size > recordset->cache.alloc)
    {
        HROW *rows = realloc( recordset->cache.rows, sizeof(*rows) * size );
        if (!rows) return E_OUTOFMEMORY;
        recordset->cache.alloc = size;
        recordset->cache.rows = rows;
    }

    recordset->cache.size = size;
    return S_OK;
}

static HRESULT WINAPI recordset_get_CursorType( _Recordset *iface, CursorTypeEnum *cursor_type )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", iface, cursor_type );

    *cursor_type = recordset->cursor_type;
    return S_OK;
}

static HRESULT WINAPI recordset_put_CursorType( _Recordset *iface, CursorTypeEnum cursor_type )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %d\n", iface, cursor_type );

    recordset->cursor_type = cursor_type;
    return S_OK;
}

static HRESULT WINAPI recordset_get_EOF( _Recordset *iface, VARIANT_BOOL *eof )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, eof );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->is_eof && !recordset->is_bof && !recordset->current_row)
    {
        hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }

    *eof = recordset->is_eof;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Fields( _Recordset *iface, Fields **obj )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", recordset, obj );

    *obj = &recordset->fields.Fields_iface;
    Fields_AddRef(*obj);
    return S_OK;
}

static HRESULT WINAPI recordset_get_LockType( _Recordset *iface, LockTypeEnum *lock_type )
{
    FIXME( "%p, %p\n", iface, lock_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_LockType( _Recordset *iface, LockTypeEnum lock_type )
{
    FIXME( "%p, %d\n", iface, lock_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_MaxRecords( _Recordset *iface, ADO_LONGPTR *max_records )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, max_records );

    *max_records = recordset->max_records;
    return S_OK;
}

static HRESULT WINAPI recordset_put_MaxRecords( _Recordset *iface, ADO_LONGPTR max_records )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %Id\n", iface, max_records );

    recordset->max_records = max_records;
    return S_OK;
}

static HRESULT WINAPI recordset_get_RecordCount( _Recordset *iface, ADO_LONGPTR *count )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    DBCOUNTITEM rows;
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, count );

    *count = -1;

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->rowset_es)
    {
        hr = IUnknown_QueryInterface( recordset->row_set, &IID_IRowsetExactScroll,
                (void**)&recordset->rowset_es );
        if (FAILED(hr) || !recordset->rowset_es)
            recordset->rowset_es = NO_INTERFACE;
    }
    if (recordset->rowset_es == NO_INTERFACE)
        return S_OK;

    hr = IRowsetExactScroll_GetExactPosition( recordset->rowset_es, 0, 0, 0, 0, &rows );
    if (SUCCEEDED(hr))
        *count = rows;
    return S_OK;
}

static HRESULT WINAPI recordset_putref_Source( _Recordset *iface, IDispatch *source )
{
    FIXME( "%p, %p\n", iface, source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Source( _Recordset *iface, BSTR source )
{
    FIXME( "%p, %s\n", iface, debugstr_w(source) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Source( _Recordset *iface, VARIANT *source )
{
    FIXME( "%p, %p\n", iface, source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_AddNew( _Recordset *iface, VARIANT field_list, VARIANT values )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    DBREFCOUNT refcount;
    HRESULT hr;

    TRACE( "%p, %s, %s\n", recordset, debugstr_variant(&field_list), debugstr_variant(&values) );
    if (V_VT(&field_list) != VT_ERROR)
        FIXME( "ignoring field list and values\n" );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->rowset_change)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IRowsetChange,
                (void **)&recordset->rowset_change );
        if (FAILED(hr) || !recordset->rowset_change)
            recordset->rowset_change = NO_INTERFACE;
    }
    if (recordset->rowset_change == NO_INTERFACE)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    if (!recordset->accessor)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IAccessor, (void **)&recordset->accessor );
        if (FAILED(hr) || !recordset->accessor)
            recordset->accessor = NO_INTERFACE;
    }
    if (recordset->accessor == NO_INTERFACE)
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );

    if (!recordset->hacc_empty)
    {
        hr = IAccessor_CreateAccessor( recordset->accessor, DBACCESSOR_ROWDATA,
                0, NULL, 0, &recordset->hacc_empty, NULL );
        if (FAILED(hr) || !recordset->hacc_empty)
            return MAKE_ADO_HRESULT( adErrNoCurrentRecord );
    }

    hr = IAccessor_AddRefAccessor( recordset->accessor, recordset->hacc_empty, &refcount );
    if (FAILED(hr))
        return MAKE_ADO_HRESULT( adErrNoCurrentRecord );
    cache_release( recordset );
    hr = IRowsetChange_InsertRow( recordset->rowset_change, 0,
            recordset->hacc_empty, NULL, &recordset->current_row );
    IAccessor_ReleaseAccessor( recordset->accessor, recordset->hacc_empty, &refcount );
    if (FAILED(hr))
        return MAKE_ADO_HRESULT( adErrNoCurrentRecord );
    recordset->is_bof = recordset->is_eof = FALSE;

    recordset->editmode = adEditAdd;
    return S_OK;
}

static HRESULT WINAPI recordset_CancelUpdate( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    FIXME( "%p\n", iface );

    if (recordset->active_connection == NULL)
        return S_OK;

    recordset->editmode = adEditNone;
    return S_OK;
}

static HRESULT WINAPI recordset_Close( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    close_recordset( recordset );
    recordset->state = adStateClosed;
    return S_OK;
}

static HRESULT WINAPI recordset_Delete( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetRows( _Recordset *iface, LONG rows, VARIANT start, VARIANT fields, VARIANT *var )
{
    FIXME( "%p, %ld, %s, %s, %p\n", iface, rows, debugstr_variant(&start), debugstr_variant(&fields), var );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Move( _Recordset *iface, ADO_LONGPTR num_records, VARIANT start )
{
    FIXME( "%p, %Id, %s\n", iface, num_records, debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MoveNext( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->current_row && !recordset->is_eof && !recordset->is_bof)
    {
        HRESULT hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }

    return cache_get( recordset, TRUE );
}

static HRESULT WINAPI recordset_MovePrevious( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->current_row && !recordset->is_eof && !recordset->is_bof)
    {
        HRESULT hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }

    return cache_get( recordset, FALSE );
}

static HRESULT WINAPI recordset_MoveFirst( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;

    TRACE( "%p\n", recordset );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (recordset->bookmark_hacc)
    {
        VariantClear( &recordset->bookmark );
        V_VT( &recordset->bookmark ) = VT_R8;
        V_R8( &recordset->bookmark ) = -INFINITY;
        recordset->cache.dir = 0;
    }
    else
    {
        cache_release( recordset );
        hr = IRowset_RestartPosition( recordset->row_set, DB_NULL_HCHAPTER );
        if (FAILED(hr)) return hr;
    }

    recordset->is_bof = recordset->is_eof = VARIANT_FALSE;
    return cache_get( recordset, TRUE );
}

static HRESULT WINAPI recordset_MoveLast( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;

    TRACE( "%p\n", recordset );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (recordset->bookmark_hacc)
    {
        VariantClear( &recordset->bookmark );
        V_VT( &recordset->bookmark ) = VT_R8;
        V_R8( &recordset->bookmark ) = INFINITY;
        recordset->cache.dir = 0;
    }
    else
    {
        cache_release( recordset );
        hr = IRowset_RestartPosition( recordset->row_set, DB_NULL_HCHAPTER );
        if (FAILED(hr)) return hr;
    }

    recordset->is_bof = recordset->is_eof = VARIANT_FALSE;
    return cache_get( recordset, FALSE );
}

static HRESULT get_rowset(struct recordset *recordset, IUnknown *session, BSTR source, IUnknown **rowset)
{
    IDBCreateCommand *create_command;
    ICommandText *command_text;
    IOpenRowset *openrowset;
    DBROWCOUNT affected;
    ICommand *cmd;
    DBID table;
    HRESULT hr;

    hr = IUnknown_QueryInterface(session, &IID_IOpenRowset, (void**)&openrowset);
    if (FAILED(hr))
        return hr;

    table.eKind = DBKIND_NAME;
    table.uName.pwszName = source;
    hr = IOpenRowset_OpenRowset(openrowset, NULL, &table, NULL, &IID_IUnknown, 0, NULL, rowset);
    if (SUCCEEDED(hr))
    {
        IOpenRowset_Release(openrowset);
        return hr;
    }

    hr = IOpenRowset_QueryInterface(openrowset, &IID_IDBCreateCommand, (void**)&create_command);
    IOpenRowset_Release(openrowset);
    if (FAILED(hr))
        return hr;

    hr = IDBCreateCommand_CreateCommand(create_command, NULL, &IID_IUnknown, (IUnknown **)&cmd);
    IDBCreateCommand_Release(create_command);
    if (FAILED(hr))
        return hr;

    hr = ICommand_QueryInterface(cmd, &IID_ICommandText, (void**)&command_text);
    ICommand_Release(cmd);
    if (FAILED(hr))
    {
        FIXME("Currently only ICommandText interface is support\n");
        return hr;
    }

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, source);
    if (FAILED(hr))
    {
        ICommandText_Release(command_text);
        return hr;
    }

    hr = ICommandText_Execute(command_text, NULL, &IID_IUnknown, NULL, &affected, rowset);
    ICommandText_Release(command_text);
    return hr;
}

static HRESULT WINAPI recordset_Open( _Recordset *iface, VARIANT source, VARIANT active_connection,
                                      CursorTypeEnum cursor_type, LockTypeEnum lock_type, LONG options )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    ADOConnectionConstruction15 *construct;
    IUnknown *session;
    IUnknown *rowset;
    HRESULT hr;

    TRACE( "%p, %s, %s, %d, %d, %ld\n", recordset, debugstr_variant(&source), debugstr_variant(&active_connection),
           cursor_type, lock_type, options );

    if (recordset->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    if (V_VT(&active_connection) != VT_ERROR || V_ERROR(&active_connection) != DISP_E_PARAMNOTFOUND)
    {
        hr = _Recordset_put_ActiveConnection( iface, active_connection );
        if (FAILED(hr))
            return hr;
    }

    if (recordset->fields.count != -1)
    {
        DBCOLUMNINFO *info;
        int i;

        if (recordset->active_connection)
        {
            FIXME("adding new table\n");
            return E_NOTIMPL;
        }

        info = calloc(recordset->fields.count + 1, sizeof(*info));
        if (!info)
            return E_OUTOFMEMORY;

        info[0].dwFlags = DBCOLUMNFLAGS_ISBOOKMARK | DBCOLUMNFLAGS_ISFIXEDLENGTH;
        info[0].ulColumnSize = sizeof(unsigned int);
        info[0].wType = DBTYPE_UI4;
        info[0].bPrecision = 10;
        info[0].bScale = 255;
        info[0].columnid.eKind = DBKIND_GUID_PROPID;
        info[0].columnid.uGuid.guid = DBCOL_SPECIALCOL;
        info[0].columnid.uName.ulPropid = 2 /* PROPID_DBBMK_BOOKMARK */;

        for (i = 1; i <= recordset->fields.count; i++)
        {
            struct field *field = recordset->fields.field[i - 1];

            info[i].pwszName = field->name;
            info[i].iOrdinal = field->ordinal;
            info[i].dwFlags = field->attrs;
            info[i].ulColumnSize = field->defined_size;
            info[i].wType = field->type;
            info[i].bPrecision = field->prec;
            info[i].bScale = field->scale;
            info[i].columnid.eKind = DBKIND_NAME;
            info[i].columnid.uName.pwszName = field->name;
        }

        hr = create_mem_rowset(recordset->fields.count + 1, info, &rowset);
        free(info);
        if (FAILED(hr))
            return hr;

        hr = ADORecordsetConstruction_put_Rowset(&recordset->ADORecordsetConstruction_iface, rowset);
        IUnknown_Release(rowset);
        return hr;
    }

    if (!recordset->active_connection)
        return MAKE_ADO_HRESULT( adErrInvalidConnection );

    if (V_VT(&source) != VT_BSTR)
    {
        FIXME("Unsupported source type!\n");
        return E_FAIL;
    }

    hr = _Connection_QueryInterface(recordset->active_connection, &IID_ADOConnectionConstruction15, (void**)&construct);
    if (FAILED(hr))
        return E_FAIL;

    hr = ADOConnectionConstruction15_get_Session(construct, &session);
    ADOConnectionConstruction15_Release(construct);
    if (FAILED(hr))
        return E_FAIL;

    hr = get_rowset(recordset, session, V_BSTR(&source), &rowset);
    IUnknown_Release(session);
    if (FAILED(hr) || !rowset)
        return hr;

    hr = ADORecordsetConstruction_put_Rowset(&recordset->ADORecordsetConstruction_iface, rowset);
    IUnknown_Release(rowset);
    recordset->cursor_type = cursor_type;
    return hr;
}

static HRESULT WINAPI recordset_Requery( _Recordset *iface, LONG options )
{
    FIXME( "%p, %ld\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xResync( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Update( _Recordset *iface, VARIANT fields, VARIANT values )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&fields), debugstr_variant(&values) );

    if (recordset->active_connection == NULL)
        return S_OK;

    recordset->editmode = adEditNone;
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_AbsolutePage( _Recordset *iface, PositionEnum_Param *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_AbsolutePage( _Recordset *iface, PositionEnum_Param pos )
{
    FIXME( "%p, %Id\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_EditMode( _Recordset *iface, EditModeEnum *mode )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, mode );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!recordset->current_row && !recordset->is_eof && !recordset->is_bof)
    {
        HRESULT hr = cache_get( recordset, TRUE );
        if (FAILED(hr)) return hr;
    }
    if (!recordset->current_row) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    *mode = recordset->editmode;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Filter( _Recordset *iface, VARIANT *criteria )
{
    FIXME( "%p, %p\n", iface, criteria );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Filter( _Recordset *iface, VARIANT criteria )
{
    TRACE( "%p, %s\n", iface, debugstr_variant(&criteria) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_PageCount( _Recordset *iface, ADO_LONGPTR *count )
{
    FIXME( "%p, %p\n", iface, count );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_PageSize( _Recordset *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_PageSize( _Recordset *iface, LONG size )
{
    FIXME( "%p, %ld\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Sort( _Recordset *iface, BSTR *criteria )
{
    FIXME( "%p, %p\n", iface, criteria );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Sort( _Recordset *iface, BSTR criteria )
{
    FIXME( "%p, %s\n", iface, debugstr_w(criteria) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Status( _Recordset *iface, LONG *status )
{
    FIXME( "%p, %p\n", iface, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_State( _Recordset *iface, LONG *state )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", recordset, state );

    *state = recordset->state;
    return S_OK;
}

static HRESULT WINAPI recordset__xClone( _Recordset *iface, _Recordset **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_UpdateBatch( _Recordset *iface, AffectEnum affect_records )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    FIXME( "%p, %u\n", iface, affect_records );

    if (recordset->active_connection == NULL)
        return S_OK;

    recordset->editmode = adEditNone;
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CancelBatch( _Recordset *iface, AffectEnum affect_records )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    FIXME( "%p, %u\n", iface, affect_records );

    if (recordset->active_connection == NULL)
        return S_OK;

    recordset->editmode = adEditNone;
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_CursorLocation( _Recordset *iface, CursorLocationEnum *cursor_loc )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", iface, cursor_loc );

    *cursor_loc = recordset->cursor_location;

    return S_OK;
}

static HRESULT WINAPI recordset_put_CursorLocation( _Recordset *iface, CursorLocationEnum cursor_loc )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %u\n", iface, cursor_loc );

    if (recordset->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    recordset->cursor_location = cursor_loc;

    return S_OK;
}

static HRESULT WINAPI recordset_NextRecordset( _Recordset *iface, VARIANT *records_affected, _Recordset **record_set )
{
    FIXME( "%p, %p, %p\n", iface, records_affected, record_set );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Supports( _Recordset *iface, CursorOptionEnum cursor_options, VARIANT_BOOL *ret )
{
    FIXME( "%p, %08x, %p\n", iface, cursor_options, ret );
    *ret = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Collect( _Recordset *iface, VARIANT index, VARIANT *var )
{
    FIXME( "%p, %s, %p\n", iface, debugstr_variant(&index), var );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Collect( _Recordset *iface, VARIANT index, VARIANT var )
{
    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&index), debugstr_variant(&var) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_MarshalOptions( _Recordset *iface, MarshalOptionsEnum *options )
{
    FIXME( "%p, %p\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_MarshalOptions( _Recordset *iface, MarshalOptionsEnum options )
{
    FIXME( "%p, %u\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Find( _Recordset *iface, BSTR criteria, LONG skip_records,
                                      SearchDirectionEnum search_direction, VARIANT start )
{
    FIXME( "%p, %s, %ld, %d, %s\n", iface, debugstr_w(criteria), skip_records, search_direction,
           debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Cancel( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    FIXME( "%p\n", iface );

    if (recordset->active_connection == NULL)
        return S_OK;

    recordset->editmode = adEditNone;
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_DataSource( _Recordset *iface, IUnknown **data_source )
{
    FIXME( "%p, %p\n", iface, data_source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_DataSource( _Recordset *iface, IUnknown *data_source )
{
    FIXME( "%p, %p\n", iface, data_source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xSave( _Recordset *iface, BSTR filename, PersistFormatEnum persist_format )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_w(filename), persist_format );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_ActiveCommand( _Recordset *iface, IDispatch **cmd )
{
    FIXME( "%p, %p\n", iface, cmd );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_StayInSync( _Recordset *iface, VARIANT_BOOL stay_in_sync )
{
    FIXME( "%p, %d\n", iface, stay_in_sync );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_StayInSync( _Recordset *iface, VARIANT_BOOL *stay_in_sync )
{
    FIXME( "%p, %p\n", iface, stay_in_sync );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetString( _Recordset *iface, StringFormatEnum string_format, LONG num_rows,
                                           BSTR column_delimiter, BSTR row_delimiter, BSTR null_expr,
                                           BSTR *ret_string )
{
    FIXME( "%p, %u, %ld, %s, %s, %s, %p\n", iface, string_format, num_rows, debugstr_w(column_delimiter),
           debugstr_w(row_delimiter), debugstr_w(null_expr), ret_string );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_DataMember( _Recordset *iface, BSTR *data_member )
{
    FIXME( "%p, %p\n", iface, data_member );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_DataMember( _Recordset *iface, BSTR data_member )
{
    FIXME( "%p, %s\n", iface, debugstr_w(data_member) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CompareBookmarks( _Recordset *iface, VARIANT bookmark1, VARIANT bookmark2, CompareEnum *compare )
{
    FIXME( "%p, %s, %s, %p\n", iface, debugstr_variant(&bookmark1), debugstr_variant(&bookmark2), compare );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Clone( _Recordset *iface, LockTypeEnum lock_type, _Recordset **obj )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    FIXME( "%p, %d, %p\n", recordset, lock_type, obj );

    *obj = iface;
    recordset_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI recordset_Resync( _Recordset *iface, AffectEnum affect_records, ResyncEnum resync_values )
{
    FIXME( "%p, %u, %u\n", iface, affect_records, resync_values );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Seek( _Recordset *iface, VARIANT key_values, SeekEnum seek_option )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_variant(&key_values), seek_option );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Index( _Recordset *iface, BSTR index )
{
    FIXME( "%p, %s\n", iface, debugstr_w(index) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Index( _Recordset *iface, BSTR *index )
{
    FIXME( "%p, %p\n", iface, index );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Save( _Recordset *iface, VARIANT destination, PersistFormatEnum persist_format )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_variant(&destination), persist_format );
    return E_NOTIMPL;
}

static const struct _RecordsetVtbl recordset_vtbl =
{
    recordset_QueryInterface,
    recordset_AddRef,
    recordset_Release,
    recordset_GetTypeInfoCount,
    recordset_GetTypeInfo,
    recordset_GetIDsOfNames,
    recordset_Invoke,
    recordset_get_Properties,
    recordset_get_AbsolutePosition,
    recordset_put_AbsolutePosition,
    recordset_putref_ActiveConnection,
    recordset_put_ActiveConnection,
    recordset_get_ActiveConnection,
    recordset_get_BOF,
    recordset_get_Bookmark,
    recordset_put_Bookmark,
    recordset_get_CacheSize,
    recordset_put_CacheSize,
    recordset_get_CursorType,
    recordset_put_CursorType,
    recordset_get_EOF,
    recordset_get_Fields,
    recordset_get_LockType,
    recordset_put_LockType,
    recordset_get_MaxRecords,
    recordset_put_MaxRecords,
    recordset_get_RecordCount,
    recordset_putref_Source,
    recordset_put_Source,
    recordset_get_Source,
    recordset_AddNew,
    recordset_CancelUpdate,
    recordset_Close,
    recordset_Delete,
    recordset_GetRows,
    recordset_Move,
    recordset_MoveNext,
    recordset_MovePrevious,
    recordset_MoveFirst,
    recordset_MoveLast,
    recordset_Open,
    recordset_Requery,
    recordset__xResync,
    recordset_Update,
    recordset_get_AbsolutePage,
    recordset_put_AbsolutePage,
    recordset_get_EditMode,
    recordset_get_Filter,
    recordset_put_Filter,
    recordset_get_PageCount,
    recordset_get_PageSize,
    recordset_put_PageSize,
    recordset_get_Sort,
    recordset_put_Sort,
    recordset_get_Status,
    recordset_get_State,
    recordset__xClone,
    recordset_UpdateBatch,
    recordset_CancelBatch,
    recordset_get_CursorLocation,
    recordset_put_CursorLocation,
    recordset_NextRecordset,
    recordset_Supports,
    recordset_get_Collect,
    recordset_put_Collect,
    recordset_get_MarshalOptions,
    recordset_put_MarshalOptions,
    recordset_Find,
    recordset_Cancel,
    recordset_get_DataSource,
    recordset_putref_DataSource,
    recordset__xSave,
    recordset_get_ActiveCommand,
    recordset_put_StayInSync,
    recordset_get_StayInSync,
    recordset_GetString,
    recordset_get_DataMember,
    recordset_put_DataMember,
    recordset_CompareBookmarks,
    recordset_Clone,
    recordset_Resync,
    recordset_Seek,
    recordset_put_Index,
    recordset_get_Index,
    recordset_Save
};

static inline struct recordset *recordset_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return CONTAINING_RECORD( iface, struct recordset, ISupportErrorInfo_iface );
}

static HRESULT WINAPI recordset_supporterrorinfo_QueryInterface( ISupportErrorInfo *iface, REFIID riid, void **obj )
{
    struct recordset *recordset = recordset_from_ISupportErrorInfo( iface );
    return _Recordset_QueryInterface( &recordset->Recordset_iface, riid, obj );
}

static ULONG WINAPI recordset_supporterrorinfo_AddRef( ISupportErrorInfo *iface )
{
    struct recordset *recordset = recordset_from_ISupportErrorInfo( iface );
    return _Recordset_AddRef( &recordset->Recordset_iface );
}

static ULONG WINAPI recordset_supporterrorinfo_Release( ISupportErrorInfo *iface )
{
    struct recordset *recordset = recordset_from_ISupportErrorInfo( iface );
    return _Recordset_Release( &recordset->Recordset_iface );
}

static HRESULT WINAPI recordset_supporterrorinfo_InterfaceSupportsErrorInfo( ISupportErrorInfo *iface, REFIID riid )
{
    struct recordset *recordset = recordset_from_ISupportErrorInfo( iface );
    FIXME( "%p, %s\n", recordset, debugstr_guid(riid) );
    return S_FALSE;
}

static const ISupportErrorInfoVtbl recordset_supporterrorinfo_vtbl =
{
    recordset_supporterrorinfo_QueryInterface,
    recordset_supporterrorinfo_AddRef,
    recordset_supporterrorinfo_Release,
    recordset_supporterrorinfo_InterfaceSupportsErrorInfo
};

static HRESULT WINAPI rsconstruction_QueryInterface(ADORecordsetConstruction *iface,
    REFIID riid, void **obj)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    return _Recordset_QueryInterface( &recordset->Recordset_iface, riid, obj );
}

static ULONG WINAPI rsconstruction_AddRef(ADORecordsetConstruction *iface)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    return _Recordset_AddRef( &recordset->Recordset_iface );
}

static ULONG WINAPI rsconstruction_Release(ADORecordsetConstruction *iface)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    return _Recordset_Release( &recordset->Recordset_iface );
}

static HRESULT WINAPI rsconstruction_GetTypeInfoCount(ADORecordsetConstruction *iface, UINT *pctinfo)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    TRACE( "%p, %p\n", recordset, pctinfo );
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI rsconstruction_GetTypeInfo(ADORecordsetConstruction *iface, UINT iTInfo,
    LCID lcid, ITypeInfo **ppTInfo)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    TRACE( "%p %u %lu %p\n", recordset, iTInfo, lcid, ppTInfo );
    return get_typeinfo(ADORecordsetConstruction_tid, ppTInfo);
}

static HRESULT WINAPI rsconstruction_GetIDsOfNames(ADORecordsetConstruction *iface, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p %s %p %u %lu %p\n", recordset, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId );

    hr = get_typeinfo(ADORecordsetConstruction_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI rsconstruction_Invoke(ADORecordsetConstruction *iface, DISPID dispIdMember,
    REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p %ld %s %ld %d %p %p %p %p\n", recordset, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );

    hr = get_typeinfo(ADORecordsetConstruction_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &recordset->ADORecordsetConstruction_iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI rsconstruction_get_Rowset(ADORecordsetConstruction *iface, IUnknown **row_set)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, row_set );

    if (!recordset->row_set)
    {
        *row_set = NULL;
        return S_OK;
    }

    hr = IRowset_QueryInterface(recordset->row_set, &IID_IUnknown, (void**)row_set);
    if ( FAILED(hr) ) return E_FAIL;
    return S_OK;
}

static void init_bookmark( struct recordset *recordset )
{
    DBCOLUMNINFO *colinfo = NULL;
    OLECHAR *strbuf = NULL;
    DBORDINAL i, columns;
    IColumnsInfo *info;
    HRESULT hr;

    hr = IRowset_QueryInterface( recordset->row_set, &IID_IColumnsInfo, (void **)&info );
    if (FAILED(hr) || !info) return;

    if (!recordset->accessor)
    {
        hr = IRowset_QueryInterface( recordset->row_set, &IID_IAccessor, (void **)&recordset->accessor );
        if (FAILED(hr) || !recordset->accessor)
            recordset->accessor = NO_INTERFACE;
    }
    if (recordset->accessor == NO_INTERFACE) return;

    hr = IColumnsInfo_GetColumnInfo( info, &columns, &colinfo, &strbuf );
    IColumnsInfo_Release( info );
    if (FAILED(hr)) return;
    CoTaskMemFree( strbuf );

    for (i = 0; i < columns; i++)
    {
        if (colinfo[i].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK)
        {
            DBBINDING binding;

            if (colinfo[i].ulColumnSize == sizeof(int))
                recordset->bookmark_type = DBTYPE_I4;
            else if (colinfo[i].ulColumnSize == sizeof(INT_PTR))
                recordset->bookmark_type = DBTYPE_I8;
            else
                recordset->bookmark_type = DBTYPE_BYREF | DBTYPE_BYTES;

            memset(&binding, 0, sizeof(binding));
            binding.iOrdinal = colinfo[i].iOrdinal;
            binding.obValue = offsetof( struct bookmark_data, val );
            binding.obLength = offsetof( struct bookmark_data, len );
            binding.obStatus = offsetof( struct bookmark_data, status );
            binding.dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
            binding.cbMaxLen = recordset->bookmark_type == DBTYPE_I4 ? sizeof(int) : sizeof(void *);
            binding.wType = recordset->bookmark_type;
            binding.bPrecision = colinfo[i].bPrecision;
            binding.bScale = colinfo[i].bScale;
            hr = IAccessor_CreateAccessor( recordset->accessor, DBACCESSOR_ROWDATA,
                    1, &binding, 0, &recordset->bookmark_hacc, NULL );
            if (FAILED(hr)) return;

            V_VT(&recordset->bookmark) = VT_R8;
            V_R8(&recordset->bookmark) = -INFINITY;
            return;
        }
    }

    CoTaskMemFree( colinfo );
}

static HRESULT WINAPI rsconstruction_put_Rowset(ADORecordsetConstruction *iface, IUnknown *unk)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    IRowset *rowset;
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, unk );

    if (recordset->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    hr = IUnknown_QueryInterface( unk, &IID_IRowset, (void**)&rowset );
    if ( FAILED(hr) ) return E_FAIL;
    recordset->row_set = rowset;
    recordset->state = adStateOpen;

    hr = IRowset_QueryInterface( rowset, &IID_IRowsetLocate, (void**)&recordset->rowset_locate );
    if (SUCCEEDED(hr)) init_bookmark( recordset );
    return S_OK;
}

static HRESULT WINAPI rsconstruction_get_Chapter(ADORecordsetConstruction *iface, ADO_LONGPTR *chapter)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %p\n", recordset, chapter );
    return E_NOTIMPL;
}

static HRESULT WINAPI rsconstruction_put_Chapter(ADORecordsetConstruction *iface, ADO_LONGPTR chapter)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %Id\n", recordset, chapter );
    return E_NOTIMPL;
}

static HRESULT WINAPI rsconstruction_get_RowPosition(ADORecordsetConstruction *iface, IUnknown **row_pos)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %p\n", recordset, row_pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI rsconstruction_put_RowPosition(ADORecordsetConstruction *iface, IUnknown *row_pos)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %p\n", recordset, row_pos );
    return E_NOTIMPL;
}

static const ADORecordsetConstructionVtbl rsconstruction_vtbl =
{
    rsconstruction_QueryInterface,
    rsconstruction_AddRef,
    rsconstruction_Release,
    rsconstruction_GetTypeInfoCount,
    rsconstruction_GetTypeInfo,
    rsconstruction_GetIDsOfNames,
    rsconstruction_Invoke,
    rsconstruction_get_Rowset,
    rsconstruction_put_Rowset,
    rsconstruction_get_Chapter,
    rsconstruction_put_Chapter,
    rsconstruction_get_RowPosition,
    rsconstruction_put_RowPosition
};

HRESULT Recordset_create( void **obj )
{
    struct recordset *recordset;

    if (!(recordset = calloc( 1, sizeof(*recordset) ))) return E_OUTOFMEMORY;
    recordset->Recordset_iface.lpVtbl = &recordset_vtbl;
    recordset->ISupportErrorInfo_iface.lpVtbl = &recordset_supporterrorinfo_vtbl;
    recordset->ADORecordsetConstruction_iface.lpVtbl = &rsconstruction_vtbl;
    recordset->active_connection = NULL;
    recordset->refs = 1;
    recordset->cursor_location = adUseServer;
    recordset->cursor_type = adOpenForwardOnly;
    recordset->row_set = NULL;
    recordset->editmode = adEditNone;
    recordset->cache.alloc = 1;
    recordset->cache.size = 1;
    recordset->cache.rows = malloc( sizeof(*recordset->cache.rows) );
    recordset->max_records = 0;
    Fields_create( recordset );

    *obj = &recordset->Recordset_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
