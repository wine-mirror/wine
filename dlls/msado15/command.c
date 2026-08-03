/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "msdasc.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct param_item
{
    struct list entry;

    IDispatch *param;
};

struct command
{
    _Command         Command_iface;
    ADOCommandConstruction ADOCommandConstruction_iface;
    Parameters       Parameters_iface;
    LONG             ref;
    CommandTypeEnum  type;
    BSTR             text;
    _Connection     *connection;

    struct list parameters;
};

struct parameter
{
    _Parameter Parameter_iface;
    LONG ref;

    BSTR name;
    DataTypeEnum datatype;
    ParameterDirectionEnum direction;
    ADO_LONGPTR size;
    VARIANT value;
};

static inline struct parameter *impl_from_Parameter( _Parameter *iface )
{
    return CONTAINING_RECORD( iface, struct parameter, Parameter_iface );
}

static inline struct command *impl_from_Command( _Command *iface )
{
    return CONTAINING_RECORD( iface, struct command, Command_iface );
}

static inline struct command *impl_from_ADOCommandConstruction( ADOCommandConstruction *iface )
{
    return CONTAINING_RECORD( iface, struct command, ADOCommandConstruction_iface );
}

static inline struct command *impl_from_Parameters( Parameters *iface )
{
    return CONTAINING_RECORD( iface, struct command, Parameters_iface );
}

static HRESULT WINAPI parameters_QueryInterface(Parameters *iface, REFIID riid, void **obj)
{
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)    ||
        IsEqualIID(riid, &IID_IDispatch)   ||
        IsEqualIID(riid, &IID__Collection) ||
        IsEqualIID(riid, &IID__DynaCollection)  ||
        IsEqualIID(riid, &IID_Parameters))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }

    Parameters_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI parameters_AddRef(Parameters *iface)
{
    struct command *command = impl_from_Parameters( iface );
    return _Command_AddRef(&command->Command_iface);
}

static ULONG WINAPI parameters_Release(Parameters *iface)
{
    struct command *command = impl_from_Parameters( iface );
    return _Command_Release(&command->Command_iface);
}

static HRESULT WINAPI parameters_GetTypeInfoCount(Parameters *iface, UINT *count)
{
    struct command *command = impl_from_Parameters( iface );
    TRACE( "%p, %p\n", command, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI parameters_GetTypeInfo(Parameters *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    struct command *command = impl_from_Parameters( iface );
    TRACE( "%p, %u, %lu, %p\n", command, index, lcid, info );
    return get_typeinfo(Parameters_tid, info);
}

static HRESULT WINAPI parameters_GetIDsOfNames(Parameters *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid)
{
    struct command *command = impl_from_Parameters( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", command, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Parameters_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parameters_Invoke(Parameters *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err)
{
    struct command *command = impl_from_Parameters( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", command, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Parameters_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &command->Parameters_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parameters_get_Count(Parameters *iface, LONG *count)
{
    struct command *command = impl_from_Parameters( iface );
    TRACE( "%p, %p\n", iface, count);
    *count = list_count(&command->parameters);
    return S_OK;
}

static HRESULT WINAPI parameters__NewEnum(Parameters *iface, IUnknown **object)
{
    FIXME( "%p, %p\n", iface, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameters_Refresh(Parameters *iface)
{
    FIXME( "%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameters_Append(Parameters *iface, IDispatch *object)
{
    struct command *command = impl_from_Parameters( iface );
    struct param_item *param;

    TRACE( "%p, %p\n", iface, object);

    if (!(param = calloc(1, sizeof(*param))))
        return E_OUTOFMEMORY;
    param->param = object;
    IDispatch_AddRef(object);

    list_add_tail(&command->parameters, &param->entry);
    return S_OK;
}

static HRESULT WINAPI parameters_Delete(Parameters *iface, VARIANT index)
{
    struct command *command = impl_from_Parameters( iface );
    struct param_item *para, *para2;
    int cnt = 0;

    TRACE( "%p, %s\n", iface, debugstr_variant(&index));

    LIST_FOR_EACH_ENTRY_SAFE(para, para2, &command->parameters, struct param_item, entry)
    {
        if(cnt == V_I4(&index))
        {
            list_remove(&para->entry);
            if (para->param)
                IDispatch_Release(para->param);
            free(para);
            break;
        }
    }

    return S_OK;
}

static HRESULT WINAPI parameters_get_Item(Parameters *iface, VARIANT index, _Parameter **object)
{
    FIXME( "%p, %s, %p\n", iface, debugstr_variant(&index), object);
    return E_NOTIMPL;
}

static const struct ParametersVtbl parameters_vtbl =
{
    parameters_QueryInterface,
    parameters_AddRef,
    parameters_Release,
    parameters_GetTypeInfoCount,
    parameters_GetTypeInfo,
    parameters_GetIDsOfNames,
    parameters_Invoke,
    parameters_get_Count,
    parameters__NewEnum,
    parameters_Refresh,
    parameters_Append,
    parameters_Delete,
    parameters_get_Item
};

static HRESULT WINAPI command_QueryInterface( _Command *iface, REFIID riid, void **obj )
{
    struct command *command = impl_from_Command( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)   ||
        IsEqualIID(riid, &IID_IDispatch)  ||
        IsEqualIID(riid, &IID__ADO)       ||
        IsEqualIID(riid, &IID_Command15)  ||
        IsEqualIID(riid, &IID_Command25)  ||
        IsEqualIID(riid, &IID__Command))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_ADOCommandConstruction))
    {
        *obj = &command->ADOCommandConstruction_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }

    _Command_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI command_AddRef( _Command *iface )
{
    struct command *command = impl_from_Command( iface );
    return InterlockedIncrement( &command->ref );
}

static ULONG WINAPI command_Release( _Command *iface )
{
    struct command *command = impl_from_Command( iface );
    LONG ref = InterlockedDecrement( &command->ref );
    if (!ref)
    {
        struct param_item *para, *para2;
        TRACE( "destroying %p\n", command );

        LIST_FOR_EACH_ENTRY_SAFE(para, para2, &command->parameters, struct param_item, entry)
        {
            list_remove(&para->entry);
            if (para->param)
                IDispatch_Release(para->param);
            free(para);
        }

        if (command->connection) _Connection_Release(command->connection);
        free( command->text );
        free( command );
    }
    return ref;
}

static HRESULT WINAPI command_GetTypeInfoCount( _Command *iface, UINT *count )
{
    struct command *command = impl_from_Command( iface );
    TRACE( "%p, %p\n", command, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI command_GetTypeInfo( _Command *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct command *command = impl_from_Command( iface );
    TRACE( "%p, %u, %lu, %p\n", command, index, lcid, info );
    return get_typeinfo(Command_tid, info);
}

static HRESULT WINAPI command_GetIDsOfNames( _Command *iface, REFIID riid, LPOLESTR *names, UINT count,
                                             LCID lcid, DISPID *dispid )
{
    struct command *command = impl_from_Command( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", command, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Command_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI command_Invoke( _Command *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                      DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct command *command = impl_from_Command( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", command, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Command_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &command->Command_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI command_get_Properties( _Command *iface, Properties **props )
{
    FIXME( "%p, %p\n", iface, props );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_ActiveConnection( _Command *iface, _Connection **connection )
{
    struct command *command = impl_from_Command( iface );
    TRACE( "%p, %p\n", iface, connection );

    *connection = command->connection;
    if (command->connection) _Connection_AddRef(command->connection);
    return S_OK;
}

static HRESULT WINAPI command_putref_ActiveConnection( _Command *iface, _Connection *connection )
{
    struct command *command = impl_from_Command( iface );
    TRACE( "%p, %p\n", iface, connection );

    if (command->connection) _Connection_Release(command->connection);
    command->connection = connection;
    if (command->connection) _Connection_AddRef(command->connection);
    return S_OK;
}

static HRESULT WINAPI command_put_ActiveConnection( _Command *iface, VARIANT connection )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&connection) );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_CommandText( _Command *iface, BSTR *text )
{
    struct command *command = impl_from_Command( iface );
    BSTR cmd_text = NULL;

    TRACE( "%p, %p\n", command, text );

    if (command->text && !(cmd_text = SysAllocString( command->text ))) return E_OUTOFMEMORY;
    *text = cmd_text;
    return S_OK;
}

static HRESULT WINAPI command_put_CommandText( _Command *iface, BSTR text )
{
    struct command *command = impl_from_Command( iface );
    WCHAR *source = NULL;

    TRACE( "%p, %s\n", command, debugstr_w( text ) );

    if (text && !(source = wcsdup( text ))) return E_OUTOFMEMORY;
    free( command->text );
    command->text = source;
    return S_OK;
}

static HRESULT WINAPI command_get_CommandTimeout( _Command *iface, LONG *timeout )
{
    FIXME( "%p, %p\n", iface, timeout );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_put_CommandTimeout( _Command *iface, LONG timeout )
{
    FIXME( "%p, %ld\n", iface, timeout );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_Prepared( _Command *iface, VARIANT_BOOL *prepared )
{
    FIXME( "%p, %p\n", iface, prepared );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_put_Prepared( _Command *iface, VARIANT_BOOL prepared )
{
    FIXME( "%p, %d\n", iface, prepared );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_Execute( _Command *iface, VARIANT *affected, VARIANT *parameters,
                                       LONG options, _Recordset **recordset )
{
    FIXME( "%p, %p, %p, %ld, %p\n", iface, affected, parameters, options, recordset );
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_QueryInterface(_Parameter *iface, REFIID riid, void **obj)
{
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)   ||
        IsEqualIID(riid, &IID_IDispatch)  ||
        IsEqualIID(riid, &IID__Parameter))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }

    _Parameter_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI parameter_AddRef(_Parameter *iface)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    return InterlockedIncrement( &parameter->ref );
}

static ULONG WINAPI parameter_Release(_Parameter *iface)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    LONG ref = InterlockedDecrement( &parameter->ref );
    if (!ref)
    {
        SysFreeString(parameter->name);
        VariantClear(&parameter->value);

        free( parameter );
    }

    return ref;
}

static HRESULT WINAPI parameter_GetTypeInfoCount(_Parameter *iface, UINT *count)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE( "%p, %p\n", parameter, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI parameter_GetTypeInfo(_Parameter *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE( "%p, %u, %lu, %p\n", parameter, index, lcid, info );
    return get_typeinfo(Parameter_tid, info);
}

static HRESULT WINAPI parameter_GetIDsOfNames(_Parameter *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", parameter, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Parameter_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parameter_Invoke(_Parameter *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                        DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", parameter, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Parameter_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &parameter->Parameter_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parameter_get_Properties(_Parameter *iface, Properties **object)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %p\n", parameter, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_get_Name(_Parameter *iface, BSTR *str)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %p\n", parameter, str);
    *str = SysAllocString(parameter->name);
    return S_OK;
}

static HRESULT WINAPI parameter_put_Name(_Parameter *iface, BSTR str)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %s\n", parameter, debugstr_w(str));

    SysFreeString(parameter->name);
    parameter->name = SysAllocString(str);
    return S_OK;
}

static HRESULT WINAPI parameter_get_Value(_Parameter *iface, VARIANT *val)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %p\n", parameter, val);

    return VariantCopy(val, &parameter->value);
}

static HRESULT WINAPI parameter_put_Value(_Parameter *iface, VARIANT val)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %s\n", parameter, debugstr_variant(&val));

    VariantClear(&parameter->value);
    return VariantCopy(&parameter->value, &val);
}

static HRESULT WINAPI parameter_get_Type(_Parameter *iface, DataTypeEnum *data_type)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %p\n", parameter, data_type);
    *data_type = parameter->datatype;
    return S_OK;
}

static HRESULT WINAPI parameter_put_Type(_Parameter *iface, DataTypeEnum data_type)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %d\n", parameter, data_type);
    parameter->datatype = data_type;
    return S_OK;
}

static HRESULT WINAPI parameter_put_Direction(_Parameter *iface, ParameterDirectionEnum direction)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %d\n", parameter, direction);
    parameter->direction = direction;
    return S_OK;
}

static HRESULT WINAPI parameter_get_Direction(_Parameter *iface, ParameterDirectionEnum *direction)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %p\n", parameter, direction);
    *direction = parameter->direction;
    return S_OK;
}

static HRESULT WINAPI parameter_put_Precision(_Parameter *iface, unsigned char precision)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %d\n", parameter, precision);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_get_Precision(_Parameter *iface, unsigned char *precision)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %p\n", parameter, precision);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_put_NumericScale(_Parameter *iface, unsigned char scale)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %d\n", parameter, scale);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_get_NumericScale(_Parameter *iface, unsigned char *scale)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %p\n", parameter, scale);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_put_Size(_Parameter *iface, ADO_LONGPTR size)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %Id\n", parameter, size);
    parameter->size = size;
    return S_OK;
}

static HRESULT WINAPI parameter_get_Size(_Parameter *iface, ADO_LONGPTR *size)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    TRACE("%p, %p\n", parameter, size);
    *size = parameter->size;
    return S_OK;
}

static HRESULT WINAPI parameter_AppendChunk(_Parameter *iface, VARIANT val)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %s\n", parameter, debugstr_variant(&val));
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_get_Attributes(_Parameter *iface, LONG *attrs)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %p\n", parameter, attrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI parameter_put_Attributes(_Parameter *iface, LONG attrs)
{
    struct parameter *parameter = impl_from_Parameter( iface );
    FIXME("%p, %ld\n", parameter, attrs);
    return E_NOTIMPL;
}

static const _ParameterVtbl parameter_vtbl =
{
    parameter_QueryInterface,
    parameter_AddRef,
    parameter_Release,
    parameter_GetTypeInfoCount,
    parameter_GetTypeInfo,
    parameter_GetIDsOfNames,
    parameter_Invoke,
    parameter_get_Properties,
    parameter_get_Name,
    parameter_put_Name,
    parameter_get_Value,
    parameter_put_Value,
    parameter_get_Type,
    parameter_put_Type,
    parameter_put_Direction,
    parameter_get_Direction,
    parameter_put_Precision,
    parameter_get_Precision,
    parameter_put_NumericScale,
    parameter_get_NumericScale,
    parameter_put_Size,
    parameter_get_Size,
    parameter_AppendChunk,
    parameter_get_Attributes,
    parameter_put_Attributes
};

static HRESULT WINAPI command_CreateParameter( _Command *iface, BSTR name, DataTypeEnum type,
                                               ParameterDirectionEnum direction, ADO_LONGPTR size, VARIANT value,
                                               _Parameter **obj )
{
    struct parameter *parameter;
    TRACE( "%p, %s, %d, %d, %Id, %s, %p\n", iface, debugstr_w(name), type, direction, size,
                    debugstr_variant(&value), obj );

    if (!(parameter = malloc( sizeof(*parameter) ))) return E_OUTOFMEMORY;
    parameter->Parameter_iface.lpVtbl = &parameter_vtbl;
    parameter->ref = 1;
    parameter->name = SysAllocString(name);
    parameter->datatype = type;
    parameter->direction = direction;
    parameter->size = size;
    VariantCopy(&parameter->value, &value);

    *obj = &parameter->Parameter_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

static HRESULT WINAPI command_get_Parameters( _Command *iface, Parameters **parameters )
{
    struct command *command = impl_from_Command( iface );

    TRACE( "%p, %p\n", iface, parameters );

    if (!parameters)
        return E_INVALIDARG;

    *parameters = &command->Parameters_iface;
    Parameters_AddRef(*parameters);

    return S_OK;
}

static HRESULT WINAPI command_put_CommandType( _Command *iface, CommandTypeEnum type )
{
    struct command *command = impl_from_Command( iface );

    TRACE( "%p, %d\n", iface, type );

    switch (type)
    {
    case adCmdUnspecified:
    case adCmdUnknown:
    case adCmdText:
    case adCmdTable:
    case adCmdStoredProc:
    case adCmdFile:
    case adCmdTableDirect:
        command->type = type;
        return S_OK;
    }

    return MAKE_ADO_HRESULT( adErrInvalidArgument );
}

static HRESULT WINAPI command_get_CommandType( _Command *iface, CommandTypeEnum *type )
{
    struct command *command = impl_from_Command( iface );

    TRACE( "%p, %p\n", iface, type );

    *type = command->type;
    return S_OK;
}

static HRESULT WINAPI command_get_Name(_Command *iface, BSTR *name)
{
    FIXME( "%p, %p\n", iface, name );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_put_Name( _Command *iface, BSTR name )
{
    FIXME( "%p, %s\n", iface, debugstr_w(name) );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_State( _Command *iface, LONG *state )
{
    FIXME( "%p, %p\n", iface, state );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_Cancel( _Command *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_putref_CommandStream( _Command *iface, IUnknown *stream )
{
    FIXME( "%p, %p\n", iface, stream );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_CommandStream( _Command *iface, VARIANT *stream )
{
    FIXME( "%p, %p\n", iface, stream );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_put_Dialect( _Command *iface, BSTR dialect )
{
    FIXME( "%p, %s\n", iface, debugstr_w(dialect) );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_Dialect( _Command *iface, BSTR *dialect )
{
    FIXME( "%p, %p\n", iface, dialect );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_put_NamedParameters( _Command *iface, VARIANT_BOOL parameters )
{
    FIXME( "%p, %d\n", iface, parameters );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_NamedParameters( _Command *iface, VARIANT_BOOL *parameters )
{
    FIXME( "%p, %p\n", iface, parameters );
    return E_NOTIMPL;
}

static const struct _CommandVtbl command_vtbl =
{
    command_QueryInterface,
    command_AddRef,
    command_Release,
    command_GetTypeInfoCount,
    command_GetTypeInfo,
    command_GetIDsOfNames,
    command_Invoke,
    command_get_Properties,
    command_get_ActiveConnection,
    command_putref_ActiveConnection,
    command_put_ActiveConnection,
    command_get_CommandText,
    command_put_CommandText,
    command_get_CommandTimeout,
    command_put_CommandTimeout,
    command_get_Prepared,
    command_put_Prepared,
    command_Execute,
    command_CreateParameter,
    command_get_Parameters,
    command_put_CommandType,
    command_get_CommandType,
    command_get_Name,
    command_put_Name,
    command_get_State,
    command_Cancel,
    command_putref_CommandStream,
    command_get_CommandStream,
    command_put_Dialect,
    command_get_Dialect,
    command_put_NamedParameters,
    command_get_NamedParameters
};

static HRESULT WINAPI construction_QueryInterface(ADOCommandConstruction *iface, REFIID riid, void **obj)
{
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)    ||
        IsEqualIID(riid, &IID_ADOCommandConstruction))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }

    ADOCommandConstruction_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI construction_AddRef(ADOCommandConstruction *iface)
{
    struct command *command = impl_from_ADOCommandConstruction( iface );
    return _Command_AddRef(&command->Command_iface);
}

static ULONG WINAPI construction_Release(ADOCommandConstruction *iface)
{
    struct command *command = impl_from_ADOCommandConstruction( iface );
    return _Command_Release(&command->Command_iface);
}

static HRESULT WINAPI construction_get_OLEDBCommand(ADOCommandConstruction *iface, IUnknown **command)
{
    FIXME("%p, %p\n", iface, command);
    return E_NOTIMPL;
}

static HRESULT WINAPI construction_put_OLEDBCommand(ADOCommandConstruction *iface, IUnknown *command)
{
    FIXME("%p, %p\n", iface, command);
    return E_NOTIMPL;
}

static ADOCommandConstructionVtbl construct_vtbl =
{
    construction_QueryInterface,
    construction_AddRef,
    construction_Release,
    construction_get_OLEDBCommand,
    construction_put_OLEDBCommand
};

HRESULT Command_create( void **obj )
{
    struct command *command;

    if (!(command = malloc( sizeof(*command) ))) return E_OUTOFMEMORY;
    command->Command_iface.lpVtbl = &command_vtbl;
    command->ADOCommandConstruction_iface.lpVtbl = &construct_vtbl;
    command->Parameters_iface.lpVtbl = &parameters_vtbl;
    command->type = adCmdUnknown;
    command->text = NULL;
    command->connection = NULL;
    list_init(&command->parameters);
    command->ref = 1;

    *obj = &command->Command_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
