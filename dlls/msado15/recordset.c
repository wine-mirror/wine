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
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "msado15_backcompat.h"
#include "oledb.h"

#include "wine/debug.h"
#include "wine/heap.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct fields;
struct recordset
{
    _Recordset         Recordset_iface;
    ADORecordsetConstruction ADORecordsetConstruction_iface;
    ISupportErrorInfo  ISupportErrorInfo_iface;
    LONG               refs;
    LONG               state;
    struct fields     *fields;
    LONG               count;
    LONG               allocated;
    LONG               index;
    VARIANT           *data;
    CursorLocationEnum cursor_location;
    CursorTypeEnum     cursor_type;
    IRowset           *row_set;
};

struct fields
{
    Fields              Fields_iface;
    ISupportErrorInfo   ISupportErrorInfo_iface;
    LONG                refs;
    Field             **field;
    ULONG               count;
    ULONG               allocated;
    struct recordset   *recordset;
};

struct field
{
    Field               Field_iface;
    ISupportErrorInfo   ISupportErrorInfo_iface;
    Properties          Properties_iface;
    LONG                refs;
    WCHAR              *name;
    DataTypeEnum        type;
    LONG                defined_size;
    LONG                attrs;
    LONG                index;
    struct recordset   *recordset;
};

static inline struct field *impl_from_Field( Field *iface )
{
    return CONTAINING_RECORD( iface, struct field, Field_iface );
}

static inline struct field *impl_from_Properties( Properties *iface )
{
    return CONTAINING_RECORD( iface, struct field, Properties_iface );
}

static ULONG WINAPI field_AddRef( Field *iface )
{
    struct field *field = impl_from_Field( iface );
    LONG refs = InterlockedIncrement( &field->refs );
    TRACE( "%p new refcount %d\n", field, refs );
    return refs;
}

static ULONG WINAPI field_Release( Field *iface )
{
    struct field *field = impl_from_Field( iface );
    LONG refs = InterlockedDecrement( &field->refs );
    TRACE( "%p new refcount %d\n", field, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", field );
        heap_free( field->name );
        heap_free( field );
    }
    return refs;
}

static HRESULT WINAPI field_QueryInterface( Field *iface, REFIID riid, void **obj )
{
    struct field *field = impl_from_Field( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_Field ) || IsEqualGUID( riid, &IID_IDispatch ) ||
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
    TRACE( "%p, %u, %u, %p\n", field, index, lcid, info );
    return get_typeinfo(Field_tid, info);
}

static HRESULT WINAPI field_GetIDsOfNames( Field *iface, REFIID riid, LPOLESTR *names, UINT count,
                                           LCID lcid, DISPID *dispid )
{
    struct field *field = impl_from_Field( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %u, %p\n", field, debugstr_guid(riid), names, count, lcid, dispid );

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

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", field, member, debugstr_guid(riid), lcid, flags, params,
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

static HRESULT WINAPI field_get_ActualSize( Field *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_get_Attributes( Field *iface, LONG *attrs )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %p\n", field, attrs );

    *attrs = field->attrs;
    return S_OK;
}

static HRESULT WINAPI field_get_DefinedSize( Field *iface, LONG *size )
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

static LONG get_column_count( struct recordset *recordset )
{
    return recordset->fields->count;
}

static HRESULT WINAPI field_get_Value( Field *iface, VARIANT *val )
{
    struct field *field = impl_from_Field( iface );
    ULONG row = field->recordset->index, col = field->index, col_count;
    VARIANT copy;
    HRESULT hr;

    TRACE( "%p, %p\n", field, val );

    if (field->recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (field->recordset->index < 0) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    col_count = get_column_count( field->recordset );

    VariantInit( &copy );
    if ((hr = VariantCopy( &copy, &field->recordset->data[row * col_count + col] )) != S_OK) return hr;

    *val = copy;
    return S_OK;
}

static HRESULT WINAPI field_put_Value( Field *iface, VARIANT val )
{
    struct field *field = impl_from_Field( iface );
    ULONG row = field->recordset->index, col = field->index, col_count;
    VARIANT copy;
    HRESULT hr;

    TRACE( "%p, %s\n", field, debugstr_variant(&val) );

    if (field->recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (field->recordset->index < 0) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    col_count = get_column_count( field->recordset );

    VariantInit( &copy );
    if ((hr = VariantCopy( &copy, &val )) != S_OK) return hr;

    field->recordset->data[row * col_count + col] = copy;
    return S_OK;
}

static HRESULT WINAPI field_get_Precision( Field *iface, unsigned char *precision )
{
    FIXME( "%p, %p\n", iface, precision );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_get_NumericScale( Field *iface, unsigned char *scale )
{
    FIXME( "%p, %p\n", iface, scale );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_AppendChunk( Field *iface, VARIANT data )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&data) );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_GetChunk( Field *iface, LONG length, VARIANT *var )
{
    FIXME( "%p, %d, %p\n", iface, length, var );
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
    FIXME( "%p, %c\n", iface, precision );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_put_NumericScale( Field *iface, unsigned char scale )
{
    FIXME( "%p, %c\n", iface, scale );
    return E_NOTIMPL;
}

static HRESULT WINAPI field_put_Type( Field *iface, DataTypeEnum type )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %u\n", field, type );

    field->type = type;
    return S_OK;
}

static HRESULT WINAPI field_put_DefinedSize( Field *iface, LONG size )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %d\n", field, size );

    field->defined_size = size;
    return S_OK;
}

static HRESULT WINAPI field_put_Attributes( Field *iface, LONG attrs )
{
    struct field *field = impl_from_Field( iface );

    TRACE( "%p, %d\n", field, attrs );

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
    TRACE( "%p, %u, %u, %p\n", field, index, lcid, info );
    return get_typeinfo(Properties_tid, info);
}

static HRESULT WINAPI field_props_GetIDsOfNames(Properties *iface, REFIID riid, LPOLESTR *names, UINT count,
                                           LCID lcid, DISPID *dispid )
{
    struct field *field = impl_from_Properties( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %u, %p\n", field, debugstr_guid(riid), names, count, lcid, dispid );

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

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", field, member, debugstr_guid(riid), lcid, flags, params,
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

static HRESULT WINAPI field_props_get_Item(Properties *iface, VARIANT index, Property **object)
{
    struct field *field = impl_from_Properties( iface );
    FIXME( "%p, %s, %p\n", field, debugstr_variant(&index), object);
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

static HRESULT Field_create( const WCHAR *name, LONG index, struct recordset *recordset, Field **obj )
{
    struct field *field;

    if (!(field = heap_alloc_zero( sizeof(*field) ))) return E_OUTOFMEMORY;
    field->Field_iface.lpVtbl = &field_vtbl;
    field->ISupportErrorInfo_iface.lpVtbl = &field_supporterrorinfo_vtbl;
    field->Properties_iface.lpVtbl = &field_properties_vtbl;
    field->refs = 1;
    if (!(field->name = strdupW( name )))
    {
        heap_free( field );
        return E_OUTOFMEMORY;
    }
    field->index = index;
    field->recordset = recordset;

    *obj = &field->Field_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

static inline struct fields *impl_from_Fields( Fields *iface )
{
    return CONTAINING_RECORD( iface, struct fields, Fields_iface );
}

static ULONG WINAPI fields_AddRef( Fields *iface )
{
    struct fields *fields = impl_from_Fields( iface );
    LONG refs = InterlockedIncrement( &fields->refs );
    TRACE( "%p new refcount %d\n", fields, refs );
    return refs;
}

static ULONG WINAPI fields_Release( Fields *iface )
{
    struct fields *fields = impl_from_Fields( iface );
    LONG refs = InterlockedDecrement( &fields->refs );
    TRACE( "%p new refcount %d\n", fields, refs );
    if (!refs)
    {
        if (fields->recordset) _Recordset_Release( &fields->recordset->Recordset_iface );
        fields->recordset = NULL;
        WARN( "not destroying %p\n", fields );
        return InterlockedIncrement( &fields->refs );
    }
    return refs;
}

static HRESULT WINAPI fields_QueryInterface( Fields *iface, REFIID riid, void **obj )
{
    struct fields *fields = impl_from_Fields( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_Fields ) || IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
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
    TRACE( "%p, %u, %u, %p\n", fields, index, lcid, info );
    return get_typeinfo(Fields_tid, info);
}

static HRESULT WINAPI fields_GetIDsOfNames( Fields *iface, REFIID riid, LPOLESTR *names, UINT count,
                                            LCID lcid, DISPID *dispid )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %u, %p\n", fields, debugstr_guid(riid), names, count, lcid, dispid );

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

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", fields, member, debugstr_guid(riid), lcid, flags, params,
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

static HRESULT WINAPI fields_get_Count( Fields *iface, LONG *count )
{
    struct fields *fields = impl_from_Fields( iface );

    TRACE( "%p, %p\n", fields, count );

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
        BSTR name;
        BOOL match;
        HRESULT hr;

        if ((hr = Field_get_Name( fields->field[i], &name )) != S_OK) return hr;
        match = !wcsicmp( V_BSTR( index ), name );
        SysFreeString( name );
        if (match)
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
    ULONG i;

    TRACE( "%p, %s, %p\n", fields, debugstr_variant(&index), obj );

    if ((hr = map_index( fields, &index, &i )) != S_OK) return hr;

    Field_AddRef( fields->field[i] );
    *obj = fields->field[i];
    return S_OK;
}

static BOOL resize_fields( struct fields *fields, ULONG count )
{
    if (count > fields->allocated)
    {
        Field **tmp;
        ULONG new_size = max( count, fields->allocated * 2 );
        if (!(tmp = heap_realloc( fields->field, new_size * sizeof(*tmp) ))) return FALSE;
        fields->field = tmp;
        fields->allocated = new_size;
    }

    fields->count = count;
    return TRUE;
}

static HRESULT append_field( struct fields *fields, BSTR name, DataTypeEnum type, LONG size, FieldAttributeEnum attr,
                             VARIANT *value )
{
    Field *field;
    HRESULT hr;

    if ((hr = Field_create( name, fields->count, fields->recordset, &field )) != S_OK) return hr;
    Field_put_Type( field, type );
    Field_put_DefinedSize( field, size );
    if (attr != adFldUnspecified) Field_put_Attributes( field, attr );
    if (value) FIXME( "ignoring value %s\n", debugstr_variant(value) );

    if (!(resize_fields( fields, fields->count + 1 )))
    {
        Field_Release( field );
        return E_OUTOFMEMORY;
    }

    fields->field[fields->count - 1] = field;
    return S_OK;
}

static HRESULT WINAPI fields__Append( Fields *iface, BSTR name, DataTypeEnum type, LONG size, FieldAttributeEnum attr )
{
    struct fields *fields = impl_from_Fields( iface );

    TRACE( "%p, %s, %u, %d, %d\n", fields, debugstr_w(name), type, size, attr );

    return append_field( fields, name, type, size, attr, NULL );
}

static HRESULT WINAPI fields_Delete( Fields *iface, VARIANT index )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&index) );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_Append( Fields *iface, BSTR name, DataTypeEnum type, LONG size, FieldAttributeEnum attr,
                                     VARIANT value )
{
    struct fields *fields = impl_from_Fields( iface );

    TRACE( "%p, %s, %u, %d, %d, %s\n", fields, debugstr_w(name), type, size, attr, debugstr_variant(&value) );

    return append_field( fields, name, type, size, attr, &value );
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

static void map_rowset_fields(struct recordset *recordset, struct fields *fields)
{
    HRESULT hr;
    IColumnsInfo *columninfo;
    DBORDINAL columns, i;
    DBCOLUMNINFO *colinfo;
    OLECHAR *stringsbuffer;

    /* Not Finding the interface or GetColumnInfo failing just causes 0 Fields to be returned */
    hr = IRowset_QueryInterface(recordset->row_set, &IID_IColumnsInfo, (void**)&columninfo);
    if (FAILED(hr))
        return;

    hr = IColumnsInfo_GetColumnInfo(columninfo, &columns, &colinfo, &stringsbuffer);
    if (SUCCEEDED(hr))
    {
        for (i=0; i < columns; i++)
        {
            TRACE("Adding Column %lu, pwszName: %s, pTypeInfo %p, iOrdinal %lu, dwFlags 0x%08x, "
                  "ulColumnSize %lu, wType %d, bPrecision %d, bScale %d\n",
                  i, debugstr_w(colinfo[i].pwszName), colinfo[i].pTypeInfo, colinfo[i].iOrdinal,
                  colinfo[i].dwFlags, colinfo[i].ulColumnSize, colinfo[i].wType,
                  colinfo[i].bPrecision, colinfo[i].bScale);

            hr = append_field(fields, colinfo[i].pwszName, colinfo[i].wType, colinfo[i].ulColumnSize,
                     colinfo[i].dwFlags, NULL);
            if (FAILED(hr))
            {
                ERR("Failed to add Field name - 0x%08x\n", hr);
                return;
            }
        }

        CoTaskMemFree(colinfo);
        CoTaskMemFree(stringsbuffer);
    }

    IColumnsInfo_Release(columninfo);
}

static HRESULT fields_create( struct recordset *recordset, struct fields **ret )
{
    struct fields *fields;

    if (!(fields = heap_alloc_zero( sizeof(*fields) ))) return E_OUTOFMEMORY;
    fields->Fields_iface.lpVtbl = &fields_vtbl;
    fields->ISupportErrorInfo_iface.lpVtbl = &fields_supporterrorinfo_vtbl;
    fields->refs = 1;
    fields->recordset = recordset;
    _Recordset_AddRef( &fields->recordset->Recordset_iface );

    if ( recordset->row_set )
        map_rowset_fields(recordset, fields);

    *ret = fields;
    TRACE( "returning %p\n", *ret );
    return S_OK;
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
    TRACE( "%p new refcount %d\n", recordset, refs );
    return refs;
}

static void close_recordset( struct recordset *recordset )
{
    ULONG row, col, col_count;
    ULONG i;

    if ( recordset->row_set ) IRowset_Release( recordset->row_set );
    recordset->row_set = NULL;

    if (!recordset->fields) return;
    col_count = get_column_count( recordset );

    for (i = 0; i < col_count; i++)
    {
        struct field *field = impl_from_Field( recordset->fields->field[i] );
        field->recordset = NULL;
        Field_Release(&field->Field_iface);
    }
    recordset->fields->count = 0;
    Fields_Release( &recordset->fields->Fields_iface );
    recordset->fields = NULL;

    for (row = 0; row < recordset->count; row++)
        for (col = 0; col < col_count; col++) VariantClear( &recordset->data[row * col_count + col] );

    recordset->count = recordset->allocated = recordset->index = 0;
    heap_free( recordset->data );
    recordset->data = NULL;
}

static ULONG WINAPI recordset_Release( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    LONG refs = InterlockedDecrement( &recordset->refs );
    TRACE( "%p new refcount %d\n", recordset, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", recordset );
        close_recordset( recordset );
        heap_free( recordset );
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
    TRACE( "%p, %u, %u, %p\n", recordset, index, lcid, info );
    return get_typeinfo(Recordset_tid, info);
}

static HRESULT WINAPI recordset_GetIDsOfNames( _Recordset *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %u, %p\n", recordset, debugstr_guid(riid), names, count, lcid, dispid );

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

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", recordset, member, debugstr_guid(riid), lcid, flags, params,
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
    FIXME( "%p, %d\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_ActiveConnection( _Recordset *iface, IDispatch *connection )
{
    FIXME( "%p, %p\n", iface, connection );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_ActiveConnection( _Recordset *iface, VARIANT connection )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&connection) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_ActiveConnection( _Recordset *iface, VARIANT *connection )
{
    FIXME( "%p, %p\n", iface, connection );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_BOF( _Recordset *iface, VARIANT_BOOL *bof )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", recordset, bof );

    *bof = (recordset->index < 0) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Bookmark( _Recordset *iface, VARIANT *bookmark )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, bookmark );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (recordset->index < 0) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    V_VT(bookmark) = VT_I4;
    V_I4(bookmark) = recordset->index;
    return S_OK;
}

static HRESULT WINAPI recordset_put_Bookmark( _Recordset *iface, VARIANT bookmark )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %s\n", iface, debugstr_variant(&bookmark) );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (V_VT(&bookmark) != VT_I4) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    recordset->index = V_I4(&bookmark);
    return S_OK;
}

static HRESULT WINAPI recordset_get_CacheSize( _Recordset *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_CacheSize( _Recordset *iface, LONG size )
{
    FIXME( "%p, %d\n", iface, size );
    return E_NOTIMPL;
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

    TRACE( "%p, %p\n", recordset, eof );

    *eof = (!recordset->count || recordset->index >= recordset->count) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Fields( _Recordset *iface, Fields **obj )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    HRESULT hr;

    TRACE( "%p, %p\n", recordset, obj );

    if (recordset->fields)
    {
        /* yes, this adds a reference to the recordset instead of the fields object */
        _Recordset_AddRef( &recordset->Recordset_iface );
        recordset->fields->recordset = recordset;
        *obj = &recordset->fields->Fields_iface;
        return S_OK;
    }

    if ((hr = fields_create( recordset, &recordset->fields )) != S_OK) return hr;

    *obj = &recordset->fields->Fields_iface;
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

static HRESULT WINAPI recordset_get_MaxRecords( _Recordset *iface, LONG *max_records )
{
    FIXME( "%p, %p\n", iface, max_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_MaxRecords( _Recordset *iface, LONG max_records )
{
    FIXME( "%p, %d\n", iface, max_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_RecordCount( _Recordset *iface, LONG *count )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %p\n", recordset, count );

    *count = recordset->count;
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

static BOOL resize_recordset( struct recordset *recordset, ULONG row_count )
{
    ULONG row_size = get_column_count( recordset ) * sizeof(*recordset->data);

    if (row_count > recordset->allocated)
    {
        VARIANT *tmp;
        ULONG count = max( row_count, recordset->allocated * 2 );
        if (!(tmp = heap_realloc_zero( recordset->data, count * row_size ))) return FALSE;
        recordset->data = tmp;
        recordset->allocated = count;
    }

    recordset->count = row_count;
    return TRUE;
}

static HRESULT WINAPI recordset_AddNew( _Recordset *iface, VARIANT field_list, VARIANT values )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p, %s, %s\n", recordset, debugstr_variant(&field_list), debugstr_variant(&values) );
    FIXME( "ignoring field list and values\n" );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!resize_recordset( recordset, recordset->count + 1 )) return E_OUTOFMEMORY;
    recordset->index++;
    return S_OK;
}

static HRESULT WINAPI recordset_CancelUpdate( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
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
    FIXME( "%p, %d, %s, %s, %p\n", iface, rows, debugstr_variant(&start), debugstr_variant(&fields), var );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Move( _Recordset *iface, LONG num_records, VARIANT start )
{
    FIXME( "%p, %d, %s\n", iface, num_records, debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MoveNext( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    if (recordset->index < recordset->count) recordset->index++;
    return S_OK;
}

static HRESULT WINAPI recordset_MovePrevious( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    if (recordset->index >= 0) recordset->index--;
    return S_OK;
}

static HRESULT WINAPI recordset_MoveFirst( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    recordset->index = 0;
    return S_OK;
}

static HRESULT WINAPI recordset_MoveLast( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );

    TRACE( "%p\n", recordset );

    recordset->index = (recordset->count > 0) ? recordset->count - 1 : 0;
    return S_OK;
}

static HRESULT create_command_text(IUnknown *session, BSTR command, ICommandText **cmd_text)
{
    HRESULT hr;
    IOpenRowset *openrowset;
    ICommandText *command_text;
    ICommand *cmd;
    IDBCreateCommand *create_command;

    hr = IUnknown_QueryInterface(session, &IID_IOpenRowset, (void**)&openrowset);
    if (FAILED(hr))
        return hr;

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

    hr = ICommandText_SetCommandText(command_text, &DBGUID_DEFAULT, command);
    if (FAILED(hr))
    {
        ICommandText_Release(command_text);
        return hr;
    }

    *cmd_text = command_text;

    return S_OK;
}

static HRESULT WINAPI recordset_Open( _Recordset *iface, VARIANT source, VARIANT active_connection,
                                      CursorTypeEnum cursor_type, LockTypeEnum lock_type, LONG options )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    ADOConnectionConstruction15 *construct;
    IUnknown *session;
    ICommandText *command_text;
    DBROWCOUNT affected;
    IUnknown *rowset;
    HRESULT hr;

    FIXME( "%p, %s, %s, %d, %d, %d Semi-stub\n", recordset, debugstr_variant(&source), debugstr_variant(&active_connection),
           cursor_type, lock_type, options );

    if (recordset->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    if (recordset->fields)
    {
        recordset->state = adStateOpen;
        return S_OK;
    }

    if (V_VT(&active_connection) != VT_DISPATCH)
    {
        FIXME("Unsupported Active connection type %d\n", V_VT(&active_connection));
        return MAKE_ADO_HRESULT( adErrInvalidConnection );
    }

    hr = IDispatch_QueryInterface(V_DISPATCH(&active_connection), &IID_ADOConnectionConstruction15, (void**)&construct);
    if (FAILED(hr))
        return E_FAIL;

    hr = ADOConnectionConstruction15_get_Session(construct, &session);
    ADOConnectionConstruction15_Release(construct);
    if (FAILED(hr))
        return E_FAIL;

    if (V_VT(&source) != VT_BSTR)
    {
        FIXME("Unsupported source type!\n");
        IUnknown_Release(session);
        return E_FAIL;
    }

    hr = create_command_text(session, V_BSTR(&source), &command_text);
    IUnknown_Release(session);
    if (FAILED(hr))
        return hr;

    hr = ICommandText_Execute(command_text, NULL, &IID_IUnknown, NULL, &affected, &rowset);
    ICommandText_Release(command_text);
    if (FAILED(hr) || !rowset)
        return hr;

    ADORecordsetConstruction_put_Rowset(&recordset->ADORecordsetConstruction_iface, rowset);
    recordset->cursor_type = cursor_type;
    recordset->state = adStateOpen;

    IUnknown_Release(rowset);

    return hr;
}

static HRESULT WINAPI recordset_Requery( _Recordset *iface, LONG options )
{
    FIXME( "%p, %d\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xResync( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Update( _Recordset *iface, VARIANT fields, VARIANT values )
{
    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&fields), debugstr_variant(&values) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_AbsolutePage( _Recordset *iface, PositionEnum_Param *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_AbsolutePage( _Recordset *iface, PositionEnum_Param pos )
{
    FIXME( "%p, %d\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_EditMode( _Recordset *iface, EditModeEnum *mode )
{
    FIXME( "%p, %p\n", iface, mode );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Filter( _Recordset *iface, VARIANT *criteria )
{
    FIXME( "%p, %p\n", iface, criteria );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Filter( _Recordset *iface, VARIANT criteria )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&criteria) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_PageCount( _Recordset *iface, LONG *count )
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
    FIXME( "%p, %d\n", iface, size );
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
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CancelBatch( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
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
    return E_NOTIMPL;
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
    FIXME( "%p, %s, %d, %d, %s\n", iface, debugstr_w(criteria), skip_records, search_direction,
           debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Cancel( _Recordset *iface )
{
    FIXME( "%p\n", iface );
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
    FIXME( "%p, %u, %d, %s, %s, %s, %p\n", iface, string_format, num_rows, debugstr_w(column_delimiter),
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
    FIXME( "%p, %d, %p\n", iface, lock_type, obj );
    return E_NOTIMPL;
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
    TRACE( "%p %u %u %p\n", recordset, iTInfo, lcid, ppTInfo );
    return get_typeinfo(ADORecordsetConstruction_tid, ppTInfo);
}

static HRESULT WINAPI rsconstruction_GetIDsOfNames(ADORecordsetConstruction *iface, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p %s %p %u %u %p\n", recordset, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId );

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

    TRACE( "%p %d %s %d %d %p %p %p %p\n", recordset, dispIdMember, debugstr_guid(riid),
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

    hr = IRowset_QueryInterface(recordset->row_set, &IID_IUnknown, (void**)row_set);
    if ( FAILED(hr) ) return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI rsconstruction_put_Rowset(ADORecordsetConstruction *iface, IUnknown *unk)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    HRESULT hr;
    IRowset *rowset;

    TRACE( "%p, %p\n", recordset, unk );

    hr = IUnknown_QueryInterface(unk, &IID_IRowset, (void**)&rowset);
    if ( FAILED(hr) ) return E_FAIL;

    if ( recordset->row_set ) IRowset_Release( recordset->row_set );
    recordset->row_set = rowset;

    return S_OK;
}

static HRESULT WINAPI rsconstruction_get_Chapter(ADORecordsetConstruction *iface, LONG *chapter)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %p\n", recordset, chapter );
    return E_NOTIMPL;
}

static HRESULT WINAPI rsconstruction_put_Chapter(ADORecordsetConstruction *iface, LONG chapter)
{
    struct recordset *recordset = impl_from_ADORecordsetConstruction( iface );
    FIXME( "%p, %d\n", recordset, chapter );
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

    if (!(recordset = heap_alloc_zero( sizeof(*recordset) ))) return E_OUTOFMEMORY;
    recordset->Recordset_iface.lpVtbl = &recordset_vtbl;
    recordset->ISupportErrorInfo_iface.lpVtbl = &recordset_supporterrorinfo_vtbl;
    recordset->ADORecordsetConstruction_iface.lpVtbl = &rsconstruction_vtbl;
    recordset->refs = 1;
    recordset->index = -1;
    recordset->cursor_location = adUseServer;
    recordset->cursor_type = adOpenForwardOnly;
    recordset->row_set = NULL;

    *obj = &recordset->Recordset_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
