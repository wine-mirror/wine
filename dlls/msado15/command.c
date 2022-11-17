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

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct command
{
    _Command         Command_iface;
    LONG             ref;
    CommandTypeEnum  type;
    BSTR             text;
    _Connection     *connection;
};

static inline struct command *impl_from_Command( _Command *iface )
{
    return CONTAINING_RECORD( iface, struct command, Command_iface );
}

static HRESULT WINAPI command_QueryInterface( _Command *iface, REFIID riid, void **obj )
{
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
        TRACE( "destroying %p\n", command );
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

static HRESULT WINAPI command_CreateParameter( _Command *iface, BSTR name, DataTypeEnum type,
                                               ParameterDirectionEnum direction, ADO_LONGPTR size, VARIANT value,
                                               _Parameter **parameter )
{
    FIXME( "%p, %s, %d, %d, %Id, %p\n", iface, debugstr_w(name), type, direction, size, parameter );
    return E_NOTIMPL;
}

static HRESULT WINAPI command_get_Parameters( _Command *iface, Parameters **parameters )
{
    FIXME( "%p, %p\n", iface, parameters );
    return E_NOTIMPL;
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

HRESULT Command_create( void **obj )
{
    struct command *command;

    if (!(command = malloc( sizeof(*command) ))) return E_OUTOFMEMORY;
    command->Command_iface.lpVtbl = &command_vtbl;
    command->type = adCmdUnknown;
    command->text = NULL;
    command->connection = NULL;
    command->ref = 1;

    *obj = &command->Command_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
