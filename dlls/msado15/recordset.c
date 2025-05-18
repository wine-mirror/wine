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
#include "sqlucode.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct fields
{
    Fields              Fields_iface;
    ISupportErrorInfo   ISupportErrorInfo_iface;
    LONG                refs;
    Field             **field;
    ULONG               count;
    ULONG               allocated;
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
    LONG               count;
    LONG               allocated;
    LONG               index;
    VARIANT           *data;
    CursorLocationEnum cursor_location;
    CursorTypeEnum     cursor_type;
    IRowset           *row_set;
    EditModeEnum      editmode;
    LONG               cache_size;
    ADO_LONGPTR        max_records;
    VARIANT            filter;

    DBTYPE            *columntypes;
    HACCESSOR         *haccessors;
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

    /* Field Properties */
    VARIANT             optimize;
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

static LONG get_column_count( struct recordset *recordset )
{
    return recordset->fields.count;
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

    if (field->recordset->editmode == adEditNone)
        field->recordset->editmode = adEditInProgress;

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

static HRESULT Field_create( const WCHAR *name, LONG index, struct recordset *recordset, Field **obj )
{
    struct field *field;

    if (!(field = calloc( 1, sizeof(*field) ))) return E_OUTOFMEMORY;
    field->Field_iface.lpVtbl = &field_vtbl;
    field->ISupportErrorInfo_iface.lpVtbl = &field_supporterrorinfo_vtbl;
    field->Properties_iface.lpVtbl = &field_properties_vtbl;
    field->refs = 1;
    if (!(field->name = wcsdup( name )))
    {
        free( field );
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

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = malloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static HRESULT WINAPI fields_get_Item( Fields *iface, VARIANT index, Field **obj )
{
    struct fields *fields = impl_from_Fields( iface );
    HRESULT hr;
    ULONG i = 0;

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
        if (!(tmp = realloc( fields->field, new_size * sizeof(*tmp) ))) return FALSE;
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

    if ((hr = Field_create( name, fields->count, fields_get_recordset(fields), &field )) != S_OK) return hr;
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

static HRESULT WINAPI fields__Append( Fields *iface, BSTR name, DataTypeEnum type, ADO_LONGPTR size, FieldAttributeEnum attr )
{
    struct fields *fields = impl_from_Fields( iface );

    TRACE( "%p, %s, %u, %Id, %d\n", fields, debugstr_w(name), type, size, attr );

    return append_field( fields, name, type, size, attr, NULL );
}

static HRESULT WINAPI fields_Delete( Fields *iface, VARIANT index )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&index) );
    return E_NOTIMPL;
}

static HRESULT WINAPI fields_Append( Fields *iface, BSTR name, DataTypeEnum type, ADO_LONGPTR size, FieldAttributeEnum attr,
                                     VARIANT value )
{
    struct fields *fields = impl_from_Fields( iface );

    TRACE( "%p, %s, %u, %Id, %d, %s\n", fields, debugstr_w(name), type, size, attr, debugstr_variant(&value) );

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
            TRACE("Adding Column %Iu, pwszName: %s, pTypeInfo %p, iOrdinal %Iu, dwFlags 0x%08lx, "
                  "ulColumnSize %Iu, wType %d, bPrecision %d, bScale %d\n",
                  i, debugstr_w(colinfo[i].pwszName), colinfo[i].pTypeInfo, colinfo[i].iOrdinal,
                  colinfo[i].dwFlags, colinfo[i].ulColumnSize, colinfo[i].wType,
                  colinfo[i].bPrecision, colinfo[i].bScale);

            hr = append_field(fields, colinfo[i].pwszName, colinfo[i].wType, colinfo[i].ulColumnSize,
                     colinfo[i].dwFlags, NULL);
            if (FAILED(hr))
            {
                ERR("Failed to add Field name - 0x%08lx\n", hr);
                return;
            }
        }

        CoTaskMemFree(colinfo);
        CoTaskMemFree(stringsbuffer);
    }

    IColumnsInfo_Release(columninfo);
}

static void fields_init( struct recordset *recordset )
{
    memset( &recordset->fields, 0, sizeof(recordset->fields) );
    recordset->fields.Fields_iface.lpVtbl = &fields_vtbl;
    recordset->fields.ISupportErrorInfo_iface.lpVtbl = &fields_supporterrorinfo_vtbl;
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
    ULONG row, col, col_count;
    ULONG i;
    IAccessor *accessor;

    if (recordset->haccessors)
        IRowset_QueryInterface(recordset->row_set, &IID_IAccessor, (void**)&accessor);

    if ( recordset->row_set ) IRowset_Release( recordset->row_set );
    recordset->row_set = NULL;

    VariantClear( &recordset->filter );

    col_count = get_column_count( recordset );

    free(recordset->columntypes);

    for (i = 0; i < col_count; i++)
    {
        struct field *field = impl_from_Field( recordset->fields.field[i] );
        field->recordset = NULL;
        Field_Release(&field->Field_iface);

        if (recordset->haccessors)
            IAccessor_ReleaseAccessor(accessor, recordset->haccessors[i], NULL);
    }

    if (recordset->haccessors)
    {
        IAccessor_Release(accessor);
        free(recordset->haccessors);
        recordset->haccessors = NULL;
    }
    recordset->fields.count = 0;

    for (row = 0; row < recordset->count; row++)
        for (col = 0; col < col_count; col++) VariantClear( &recordset->data[row * col_count + col] );

    recordset->count = recordset->allocated = recordset->index = 0;
    free( recordset->data );
    recordset->data = NULL;
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
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, size );

    *size = recordset->cache_size;
    return S_OK;
}

static HRESULT WINAPI recordset_put_CacheSize( _Recordset *iface, LONG size )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %ld\n", iface, size );

    recordset->cache_size = size;
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

    TRACE( "%p, %p\n", recordset, eof );

    *eof = (!recordset->count || recordset->index >= recordset->count) ? VARIANT_TRUE : VARIANT_FALSE;
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
        if (!(tmp = realloc( recordset->data, count * row_size ))) return FALSE;
        memset( (BYTE*)tmp + recordset->allocated * row_size, 0, (count - recordset->allocated) * row_size );
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
    if (V_VT(&field_list) != VT_ERROR)
        FIXME( "ignoring field list and values\n" );

    if (recordset->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (!resize_recordset( recordset, recordset->count + 1 )) return E_OUTOFMEMORY;
    recordset->index = recordset->count - 1;
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

#define ROUND_SIZE(size) (((size) + sizeof(void *) - 1) & ~(sizeof(void *) - 1))

DEFINE_GUID(DBPROPSET_ROWSET,    0xc8b522be, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

static HRESULT create_bindings(IUnknown *rowset, struct recordset *recordset, DBBINDING **bind, DBBYTEOFFSET *size)
{
    HRESULT hr;
    IColumnsInfo *columninfo;
    IAccessor *accessor;
    DBORDINAL columns;
    DBCOLUMNINFO *colinfo;
    OLECHAR *stringsbuffer;
    DBBINDING *bindings;
    DBBYTEOFFSET offset;

    *size = 0;

    hr = IUnknown_QueryInterface(rowset, &IID_IColumnsInfo, (void**)&columninfo);
    if (FAILED(hr))
        return hr;

    hr = IUnknown_QueryInterface(rowset, &IID_IAccessor, (void**)&accessor);
    if (FAILED(hr))
    {
        IColumnsInfo_Release(columninfo);
        return hr;
    }

    hr = IColumnsInfo_GetColumnInfo(columninfo, &columns, &colinfo, &stringsbuffer);
    if (SUCCEEDED(hr))
    {
        ULONG i;
        DBOBJECT *dbobj;
        offset = 1;

        recordset->columntypes = malloc(sizeof(DBTYPE) * columns);
        recordset->haccessors = calloc(1, sizeof(HACCESSOR) * columns );

        /* Do one allocation for the bindings and append the DBOBJECTS to the end.
         * This is to save on multiple allocations vs a little bit of extra memory.
         */
        bindings = CoTaskMemAlloc( (sizeof(DBBINDING) + sizeof(DBOBJECT)) * columns);
        dbobj = (DBOBJECT *)((char*)bindings + (sizeof(DBBINDING) * columns));

        for (i=0; i < columns; i++)
        {
            TRACE("Column %lu, pwszName: %s, pTypeInfo %p, iOrdinal %Iu, dwFlags 0x%08lx, "
                  "ulColumnSize %Iu, wType %d, bPrecision %d, bScale %d\n",
                  i, debugstr_w(colinfo[i].pwszName), colinfo[i].pTypeInfo, colinfo[i].iOrdinal,
                  colinfo[i].dwFlags, colinfo[i].ulColumnSize, colinfo[i].wType,
                  colinfo[i].bPrecision, colinfo[i].bScale);

            hr = append_field(&recordset->fields, colinfo[i].pwszName, colinfo[i].wType, colinfo[i].ulColumnSize,
                     colinfo[i].dwFlags, NULL);

            bindings[i].iOrdinal = colinfo[i].iOrdinal;
            bindings[i].obValue = offset;
            bindings[i].pTypeInfo = NULL;
            /* Always assigned the pObject even if it's not used. */
            bindings[i].pObject = &dbobj[i];
            bindings[i].pObject->dwFlags = 0;
            bindings[i].pObject->iid = IID_ISequentialStream;
            bindings[i].pBindExt = NULL;
            bindings[i].dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
            bindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
            bindings[i].eParamIO = 0;

            recordset->columntypes[i] = colinfo[i].wType;
            if (colinfo[i].dwFlags & DBCOLUMNFLAGS_ISLONG)
            {
                colinfo[i].wType = DBTYPE_IUNKNOWN;

                bindings[i].cbMaxLen = (colinfo[i].ulColumnSize + 1) * sizeof(WCHAR);
                offset += sizeof(ISequentialStream*);
            }
            else if(colinfo[i].wType == DBTYPE_WSTR)
            {
                /* ulColumnSize is the number of characters in the string not the actual buffer size */
                bindings[i].cbMaxLen = colinfo[i].ulColumnSize * sizeof(WCHAR);
                offset += bindings[i].cbMaxLen;
            }
            else
            {
                bindings[i].cbMaxLen = colinfo[i].ulColumnSize;
                offset += bindings[i].cbMaxLen;
            }

            bindings[i].dwFlags = 0;
            bindings[i].wType = colinfo[i].wType;
            bindings[i].bPrecision = colinfo[i].bPrecision;
            bindings[i].bScale = colinfo[i].bScale;
        }

        offset = ROUND_SIZE(offset);
        for (i=0; i < columns; i++)
        {
            bindings[i].obLength = offset;
            bindings[i].obStatus = offset + sizeof(DBBYTEOFFSET);

            offset += sizeof(DBBYTEOFFSET) + sizeof(DBBYTEOFFSET);

            hr = IAccessor_CreateAccessor(accessor, DBACCESSOR_ROWDATA, 1, &bindings[i], 0, &recordset->haccessors[i], NULL);
            if (FAILED(hr))
                FIXME("IAccessor_CreateAccessor Failed 0x%0lx\n", hr);
        }

        *size = offset;
        *bind = bindings;

        CoTaskMemFree(colinfo);
        CoTaskMemFree(stringsbuffer);
    }

    IAccessor_Release(accessor);

    IColumnsInfo_Release(columninfo);

    return hr;
}

static HRESULT load_all_recordset_data(struct recordset *recordset, IUnknown *rowset, DBBINDING *bindings,
        DBBYTEOFFSET datasize)
{
    IRowset *rowset2;
    DBORDINAL columns;
    HRESULT hr;
    DBCOUNTITEM obtained;
    HROW *row = NULL;
    int datarow = 0, datacol;
    char *data;

    columns = get_column_count(recordset);

    hr = IUnknown_QueryInterface(rowset, &IID_IRowset, (void**)&rowset2);
    if (FAILED(hr))
    {
        WARN("Failed to get IRowset interface (0x%08lx)\n", hr);
        return hr;
    }

    hr = IRowset_GetNextRows(rowset2, 0, 0, 1, &obtained, &row);
    if (hr != S_OK)
    {
        recordset->count = 0;
        recordset->index = -1;
        IRowset_Release(rowset2);
        return FAILED(hr) ? hr : S_OK;
    }
    recordset->index = 0;

    data = malloc (datasize);
    if (!data)
    {
        ERR("Failed to allocate row data (%Iu)\n", datasize);
        IRowset_Release(rowset2);
        return E_OUTOFMEMORY;
    }

    do
    {
        VARIANT *v;

        if (!resize_recordset(recordset, datarow+1))
        {
            IRowset_ReleaseRows(rowset2, 1, row, NULL, NULL, NULL);
            free(data);
            IRowset_Release(rowset2);
            WARN("Failed to resize recordset\n");
            return E_OUTOFMEMORY;
        }

        for (datacol = 0; datacol < columns; datacol++)
        {
            hr = IRowset_GetData(rowset2, *row, recordset->haccessors[datacol], data);
            if (FAILED(hr))
            {
                ERR("GetData Failed on Column %d (0x%08lx), status %Id\n", datacol, hr,
                        *(DBBYTEOFFSET*)(data + bindings[datacol].obStatus));
                break;
            }

            v = &recordset->data[datarow * columns + datacol];
            VariantInit(v);

            if ( *(DBBYTEOFFSET*)(data + bindings[datacol].obStatus) == DBSTATUS_S_ISNULL)
            {
                V_VT(v) = VT_NULL;
                continue;
            }

            /* For most cases DBTYPE_* = VT_* type */
            V_VT(v) = bindings[datacol].wType;
            switch(bindings[datacol].wType)
            {
                case DBTYPE_IUNKNOWN:
                {
                    ISequentialStream *seq;
                    char unkdata[2048];
                    ULONG size = 4096, dataRead = 0, total = 0;
                    char *buffer = malloc(size), *p = buffer;
                    HRESULT hr2;

                    /*
                     * Cast directly to the object we specified in our bindings. As this object
                     *  is referenced counted in some case and will error in GetData if the object
                     *  hasn't been released.
                     */
                    seq = *(ISequentialStream**)(data + bindings[datacol].obValue);
                    TRACE("Reading DBTYPE_IUNKNOWN %p\n", seq);

                    do
                    {
                        dataRead = 0;
                        hr2 = ISequentialStream_Read(seq, unkdata, sizeof(unkdata), &dataRead);
                        if (FAILED(hr2) || !dataRead) break;

                        total += dataRead;
                        memcpy(p, unkdata, dataRead);
                        p += dataRead;
                        if (total == size)
                        {
                            size *= 2;  /* Double buffer */
                            buffer = realloc(buffer, size);
                            p = buffer + total;
                        }
                    } while(hr2 == S_OK);

                    if (recordset->columntypes[datacol] == DBTYPE_WSTR)
                    {
                        V_VT(v) = VT_BSTR;
                        V_BSTR(v) = SysAllocStringLen( (WCHAR*)buffer, total / sizeof(WCHAR) );
                    }
                    else if (recordset->columntypes[datacol] == DBTYPE_BYTES)
                    {
                        SAFEARRAYBOUND sab;

                        sab.lLbound = 0;
                        sab.cElements = total;

                        V_VT(v) = (VT_ARRAY|VT_UI1);
                        V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

                        memcpy( (BYTE*)V_ARRAY(v)->pvData, buffer, total);
                    }
                    else
                    {
                        FIXME("Unsupported conversion (%d)\n", recordset->columntypes[datacol]);
                        V_VT(v) = VT_NULL;
                    }

                    free(buffer);
                    ISequentialStream_Release(seq);

                    break;
                }
                case DBTYPE_R4:
                    V_R4(v) = *(float*)(data + bindings[datacol].obValue);
                    break;
                case DBTYPE_R8:
                    V_R8(v) = *(DOUBLE*)(data + bindings[datacol].obValue);
                    break;
                case DBTYPE_I8:
                    V_VT(v) = VT_I8;
                    V_I8(v) = *(LONGLONG*)(data + bindings[datacol].obValue);
                    break;
                case DBTYPE_I4:
                    V_I4(v) = *(LONG*)(data + bindings[datacol].obValue);
                    break;
                case DBTYPE_STR:
                {
                    WCHAR *str = heap_strdupAtoW( (char*)(data + bindings[datacol].obValue) );

                    V_VT(v) = VT_BSTR;
                    V_BSTR(v) = SysAllocString(str);
                    free(str);
                    break;
                }
                case DBTYPE_WSTR:
                {
                    V_VT(v) = VT_BSTR;
                    V_BSTR(v) = SysAllocString( (WCHAR*)(data + bindings[datacol].obValue) );
                    break;
                }
                case DBTYPE_DBTIMESTAMP:
                {
                    SYSTEMTIME st;
                    DBTIMESTAMP *ts = (DBTIMESTAMP *)(data + bindings[datacol].obValue);
                    DATE d;

                    V_VT(v) = VT_DATE;

                    st.wYear = ts->year;
                    st.wMonth = ts->month;
                    st.wDay = ts->day;
                    st.wHour = ts->hour;
                    st.wMinute = ts->minute;
                    st.wSecond = ts->second;
                    st.wMilliseconds = ts->fraction/1000000;
                    st.wDayOfWeek = 0;
                    hr = (SystemTimeToVariantTime(&st, &d) ? S_OK : E_FAIL);

                    V_DATE(v) = d;
                    break;
                }
                default:
                    V_VT(v) = VT_I2;
                    V_I2(v) = 0;
                    FIXME("Unknown Type %d\n", bindings[datacol].wType);
            }
        }

        datarow++;

        hr = IRowset_ReleaseRows(rowset2, 1, row, NULL, NULL, NULL);
        if (FAILED(hr))
            ERR("Failed to ReleaseRows 0x%08lx\n", hr);

        hr = IRowset_GetNextRows(rowset2, 0, 0, 1, &obtained, &row);
    } while(hr == S_OK);

    free(data);
    IRowset_Release(rowset2);

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
    DBBINDING *bindings;
    DBBYTEOFFSET datasize;

    TRACE( "%p, %s, %s, %d, %d, %ld\n", recordset, debugstr_variant(&source), debugstr_variant(&active_connection),
           cursor_type, lock_type, options );

    if (recordset->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    if (get_column_count( recordset ))
    {
        recordset->state = adStateOpen;
        return S_OK;
    }

    if (V_VT(&active_connection) != VT_ERROR || V_ERROR(&active_connection) != DISP_E_PARAMNOTFOUND)
    {
        hr = _Recordset_put_ActiveConnection( iface, active_connection );
        if (FAILED(hr))
            return hr;
    }
    else if (!recordset->active_connection)
        return MAKE_ADO_HRESULT( adErrInvalidConnection );

    hr = _Connection_QueryInterface(recordset->active_connection, &IID_ADOConnectionConstruction15, (void**)&construct);
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

    hr = create_bindings(rowset, recordset, &bindings, &datasize);
    if (FAILED(hr))
    {
        WARN("Failed to load bindings (%lx)\n", hr);
        IUnknown_Release(rowset);
        return hr;
    }
    recordset->count = affected > 0 ? affected : 0;
    resize_recordset(recordset, recordset->count);

    hr = load_all_recordset_data(recordset, rowset, bindings, datasize);
    if (FAILED(hr))
    {
        WARN("Failed to load all recordset data (%lx)\n", hr);
        CoTaskMemFree(bindings);
        IUnknown_Release(rowset);
        return hr;
    }

    CoTaskMemFree(bindings);

    ADORecordsetConstruction_put_Rowset(&recordset->ADORecordsetConstruction_iface, rowset);
    recordset->cursor_type = cursor_type;
    recordset->state = adStateOpen;

    IUnknown_Release(rowset);

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
    if (recordset->index < 0) return MAKE_ADO_HRESULT( adErrNoCurrentRecord );

    *mode = recordset->editmode;
    return S_OK;
}

static HRESULT WINAPI recordset_get_Filter( _Recordset *iface, VARIANT *criteria )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %p\n", iface, criteria );

    if (!criteria) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    VariantCopy(criteria, &recordset->filter);
    return S_OK;
}

static HRESULT WINAPI recordset_put_Filter( _Recordset *iface, VARIANT criteria )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    TRACE( "%p, %s\n", recordset, debugstr_variant(&criteria) );

    if (V_VT(&criteria) != VT_I2 && V_VT(&criteria) != VT_I4 && V_VT(&criteria) != VT_BSTR)
        return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if (V_VT(&criteria) == VT_BSTR && recordset->state == adStateOpen)
    {
        FIXME("No filter performed.  Reporting no records found.\n");

        /* Set the index to signal we didn't find a record. */
        recordset->index = -1;
    }
    else
    {
        recordset->index = recordset->count ? 0 : -1; /* Reset */
    }

    VariantCopy(&recordset->filter, &criteria);
    return S_OK;
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

    if ( !get_column_count(recordset) )
        map_rowset_fields(recordset, &recordset->fields);

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
    recordset->index = -1;
    recordset->cursor_location = adUseServer;
    recordset->cursor_type = adOpenForwardOnly;
    recordset->row_set = NULL;
    recordset->editmode = adEditNone;
    recordset->cache_size = 1;
    recordset->max_records = 0;
    VariantInit( &recordset->filter );
    recordset->columntypes = NULL;
    recordset->haccessors = NULL;
    fields_init( recordset );

    *obj = &recordset->Recordset_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
