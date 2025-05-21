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
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#define DBINITCONSTANTS
#include "initguid.h"
#include "ocidl.h"
#include "objbase.h"
#include "msdasc.h"
#include "olectl.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct connection;

struct connection_point
{
    IConnectionPoint   IConnectionPoint_iface;
    struct connection *conn;
    const IID         *riid;
    IUnknown          **sinks;
    ULONG              sinks_size;
};

struct connection
{
    _Connection               Connection_iface;
    ISupportErrorInfo         ISupportErrorInfo_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    ADOConnectionConstruction15 ADOConnectionConstruction15_iface;
    LONG                      refs;
    ObjectStateEnum           state;
    LONG                      timeout;
    WCHAR                    *datasource;
    WCHAR                    *provider;
    ConnectModeEnum           mode;
    CursorLocationEnum        location;
    IUnknown                 *session;
    struct connection_point   cp_connev;
};

static inline struct connection *impl_from_Connection( _Connection *iface )
{
    return CONTAINING_RECORD( iface, struct connection, Connection_iface );
}

static inline struct connection *impl_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return CONTAINING_RECORD( iface, struct connection, ISupportErrorInfo_iface );
}

static inline struct connection *impl_from_IConnectionPointContainer( IConnectionPointContainer *iface )
{
    return CONTAINING_RECORD( iface, struct connection, IConnectionPointContainer_iface );
}

static inline struct connection *impl_from_ADOConnectionConstruction15( ADOConnectionConstruction15 *iface )
{
    return CONTAINING_RECORD( iface, struct connection, ADOConnectionConstruction15_iface );
}

static inline struct connection_point *impl_from_IConnectionPoint( IConnectionPoint *iface )
{
    return CONTAINING_RECORD( iface, struct connection_point, IConnectionPoint_iface );
}

static ULONG WINAPI connection_AddRef( _Connection *iface )
{
    struct connection *connection = impl_from_Connection( iface );
    return InterlockedIncrement( &connection->refs );
}

static ULONG WINAPI connection_Release( _Connection *iface )
{
    struct connection *connection = impl_from_Connection( iface );
    LONG refs = InterlockedDecrement( &connection->refs );
    ULONG i;
    if (!refs)
    {
        TRACE( "destroying %p\n", connection );
        for (i = 0; i < connection->cp_connev.sinks_size; ++i)
        {
            if (connection->cp_connev.sinks[i])
                IUnknown_Release( connection->cp_connev.sinks[i] );
        }
        if (connection->session) IUnknown_Release( connection->session );
        free( connection->cp_connev.sinks );
        free( connection->provider );
        free( connection->datasource );
        free( connection );
    }
    return refs;
}

static HRESULT WINAPI connection_QueryInterface( _Connection *iface, REFIID riid, void **obj )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %s, %p\n", connection, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualGUID( riid, &IID__Connection ) || IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else if(IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *obj = &connection->ISupportErrorInfo_iface;
    }
    else if (IsEqualGUID( riid, &IID_IConnectionPointContainer ))
    {
        *obj = &connection->IConnectionPointContainer_iface;
    }
    else if (IsEqualGUID( riid, &IID_ADOConnectionConstruction15 ))
    {
        *obj = &connection->ADOConnectionConstruction15_iface;
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
    connection_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI connection_GetTypeInfoCount( _Connection *iface, UINT *count )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %p\n", connection, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI connection_GetTypeInfo( _Connection *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %u, %lu, %p\n", connection, index, lcid, info );
    return get_typeinfo(Connection_tid, info);
}

static HRESULT WINAPI connection_GetIDsOfNames( _Connection *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid )
{
    struct connection *connection = impl_from_Connection( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", connection, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Connection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI connection_Invoke( _Connection *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct connection *connection = impl_from_Connection( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", connection, member, debugstr_guid(riid), lcid, flags,
           params, result, excep_info, arg_err );

    hr = get_typeinfo(Connection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &connection->Connection_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI connection_get_Properties( _Connection *iface, Properties **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_ConnectionString( _Connection *iface, BSTR *str )
{
    struct connection *connection = impl_from_Connection( iface );
    BSTR source = NULL;

    TRACE( "%p, %p\n", connection, str );

    if (connection->datasource && !(source = SysAllocString( connection->datasource ))) return E_OUTOFMEMORY;
    *str = source;
    return S_OK;
}

static HRESULT WINAPI connection_put_ConnectionString( _Connection *iface, BSTR str )
{
    struct connection *connection = impl_from_Connection( iface );
    WCHAR *source = NULL;

    TRACE( "%p, %s\n", connection, debugstr_w( str && !wcsstr( str, L"Password" ) ? L"<hidden>" : str ) );

    if (str && !(source = wcsdup( str ))) return E_OUTOFMEMORY;
    free( connection->datasource );
    connection->datasource = source;
    return S_OK;
}

static HRESULT WINAPI connection_get_CommandTimeout( _Connection *iface, LONG *timeout )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %p\n", connection, timeout );
    *timeout = connection->timeout;
    return S_OK;
}

static HRESULT WINAPI connection_put_CommandTimeout( _Connection *iface, LONG timeout )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %ld\n", connection, timeout );
    connection->timeout = timeout;
    return S_OK;
}

static HRESULT WINAPI connection_get_ConnectionTimeout( _Connection *iface, LONG *timeout )
{
    FIXME( "%p, %p\n", iface, timeout );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_put_ConnectionTimeout( _Connection *iface, LONG timeout )
{
    FIXME( "%p, %ld\n", iface, timeout );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_Version( _Connection *iface, BSTR *str )
{
    FIXME( "%p, %p\n", iface, str );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_Close( _Connection *iface )
{
    struct connection *connection = impl_from_Connection( iface );

    TRACE( "%p\n", connection );

    if (connection->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    if (connection->session)
    {
        IUnknown_Release( connection->session );
        connection->session = NULL;
    }

    connection->state = adStateClosed;
    return S_OK;
}

static HRESULT WINAPI connection_Execute( _Connection *iface, BSTR command, VARIANT *records_affected,
                                          LONG options, _Recordset **record_set )
{
    struct connection *connection = impl_from_Connection( iface );
    HRESULT hr;
    _Recordset *recordset;
    VARIANT source, active;
    IDispatch *dispatch;

    FIXME( "%p, %s, %p, 0x%08lx, %p Semi-stub\n", iface, debugstr_w(command), records_affected, options, record_set );

    if (connection->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    hr = Recordset_create( (void**)&recordset);
    if (FAILED(hr))
    {
        return hr;
    }

    _Recordset_put_CursorLocation(recordset, connection->location);

    V_VT(&source) = VT_BSTR;
    V_BSTR(&source) = command;

    hr = _Connection_QueryInterface(&connection->Connection_iface, &IID_IDispatch, (void**)&dispatch);
    if (FAILED(hr))
    {
        _Recordset_Release(recordset);
        return hr;
    }

    V_VT(&active) = VT_DISPATCH;
    V_DISPATCH(&active) = dispatch;

    hr = _Recordset_Open(recordset, source, active, adOpenDynamic, adLockPessimistic, 0);
    VariantClear(&active);
    if (FAILED(hr))
    {
        _Recordset_Release(recordset);
        return hr;
    }

    if (records_affected)
    {
        ADO_LONGPTR count;
        _Recordset_get_RecordCount(recordset, &count);
        V_VT(records_affected) = VT_I4;
        V_I4(records_affected) = count;
    }

    *record_set = recordset;

    return hr;
}

static HRESULT WINAPI connection_BeginTrans( _Connection *iface, LONG *transaction_level )
{
    FIXME( "%p, %p\n", iface, transaction_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_CommitTrans( _Connection *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_RollbackTrans( _Connection *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_Open( _Connection *iface, BSTR connect_str, BSTR userid, BSTR password,
                                       LONG options )
{
    struct connection *connection = impl_from_Connection( iface );
    IDBProperties *props;
    IDBInitialize *dbinit = NULL;
    IDataInitialize *datainit;
    IDBCreateSession *session = NULL;
    HRESULT hr;

    TRACE( "%p, %s, %s, %p, %08lx\n", iface, debugstr_w(connect_str), debugstr_w(userid), password, options );

    if (connection->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );
    if (!connect_str) return E_FAIL;

    if ((hr = CoCreateInstance( &CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, &IID_IDataInitialize,
                                (void **)&datainit )) != S_OK) return hr;
    if ((hr = IDataInitialize_GetDataSource( datainit, NULL, CLSCTX_INPROC_SERVER, connect_str, &IID_IDBInitialize,
                                             (IUnknown **)&dbinit )) != S_OK) goto done;
    if ((hr = IDBInitialize_QueryInterface( dbinit, &IID_IDBProperties, (void **)&props )) != S_OK) goto done;

    /* TODO - Update username/password if required. */
    if ((userid && *userid) || (password && *password))
        FIXME("Username/password parameters currently not supported\n");

    if ((hr = IDBInitialize_Initialize( dbinit )) != S_OK) goto done;
    if ((hr = IDBInitialize_QueryInterface( dbinit, &IID_IDBCreateSession, (void **)&session )) != S_OK) goto done;
    if ((hr = IDBCreateSession_CreateSession( session, NULL, &IID_IUnknown, &connection->session )) == S_OK)
    {
        connection->state = adStateOpen;
    }
    IDBCreateSession_Release( session );

done:
    if (hr != S_OK && connection->session)
    {
        IUnknown_Release( connection->session );
        connection->session = NULL;
    }
    if (dbinit)
    {
        IDBInitialize_Uninitialize( dbinit );
        IDBInitialize_Release( dbinit );
    }
    IDataInitialize_Release( datainit );

    TRACE("ret 0x%08lx\n", hr);
    return hr;
}

static HRESULT WINAPI connection_get_Errors( _Connection *iface, Errors **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_DefaultDatabase( _Connection *iface, BSTR *str )
{
    FIXME( "%p, %p\n", iface, str );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_put_DefaultDatabase( _Connection *iface, BSTR str )
{
    FIXME( "%p, %s\n", iface, debugstr_w(str) );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_IsolationLevel( _Connection *iface, IsolationLevelEnum *level )
{
    FIXME( "%p, %p\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_put_IsolationLevel( _Connection *iface, IsolationLevelEnum level )
{
    FIXME( "%p, %d\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_Attributes( _Connection *iface, LONG *attr )
{
    FIXME( "%p, %p\n", iface, attr );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_put_Attributes( _Connection *iface, LONG attr )
{
    FIXME( "%p, %ld\n", iface, attr );
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_get_CursorLocation( _Connection *iface, CursorLocationEnum *cursor_loc )
{
    struct connection *connection = impl_from_Connection( iface );

    TRACE( "%p, %p\n", iface, cursor_loc );

    *cursor_loc = connection->location;
    return S_OK;
}

static HRESULT WINAPI connection_put_CursorLocation( _Connection *iface, CursorLocationEnum cursor_loc )
{
    struct connection *connection = impl_from_Connection( iface );

    TRACE( "%p, %u\n", iface, cursor_loc );

    connection->location = cursor_loc;
    return S_OK;
}

static HRESULT WINAPI connection_get_Mode( _Connection *iface, ConnectModeEnum *mode )
{
    struct connection *connection = impl_from_Connection( iface );

    TRACE( "%p, %p\n", iface, mode );

    *mode = connection->mode;
    return S_OK;
}

static HRESULT WINAPI connection_put_Mode( _Connection *iface, ConnectModeEnum mode )
{
    struct connection *connection = impl_from_Connection( iface );

    TRACE( "%p, %u\n", iface, mode );

    connection->mode = mode;
    return S_OK;
}

static HRESULT WINAPI connection_get_Provider( _Connection *iface, BSTR *str )
{
    struct connection *connection = impl_from_Connection( iface );
    BSTR provider = NULL;

    TRACE( "%p, %p\n", iface, str );

    if (connection->provider && !(provider = SysAllocString( connection->provider ))) return E_OUTOFMEMORY;
    *str = provider;
    return S_OK;
}

static HRESULT WINAPI connection_put_Provider( _Connection *iface, BSTR str )
{
    struct connection *connection = impl_from_Connection( iface );
    WCHAR *provider = NULL;

    TRACE( "%p, %s\n", iface, debugstr_w(str) );

    if (!str) return MAKE_ADO_HRESULT(adErrInvalidArgument);

    if (!(provider = wcsdup( str ))) return E_OUTOFMEMORY;
    free( connection->provider );
    connection->provider = provider;
    return S_OK;
}

static HRESULT WINAPI connection_get_State( _Connection *iface, LONG *state )
{
    struct connection *connection = impl_from_Connection( iface );
    TRACE( "%p, %p\n", connection, state );
    *state = connection->state;
    return S_OK;
}

static HRESULT WINAPI connection_OpenSchema( _Connection *iface, SchemaEnum schema, VARIANT restrictions,
                                             VARIANT schema_id, _Recordset **record_set )
{
    struct connection *connection = impl_from_Connection( iface );
    ADORecordsetConstruction *construct;
    IDBSchemaRowset *schema_rowset;
    _Recordset *recordset;
    ULONG restr_count;
    IUnknown *rowset;
    const GUID *guid;
    VARIANT *restr;
    HRESULT hr;

    TRACE( "%p, %d, %s, %s, %p\n", iface, schema, debugstr_variant(&restrictions),
           debugstr_variant(&schema_id), record_set );

    if (connection->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (V_VT(&schema_id) != VT_ERROR || V_ERROR(&schema_id) != DISP_E_PARAMNOTFOUND)
    {
        FIXME( "schema_id = %s\n", debugstr_variant(&schema_id) );
        return E_NOTIMPL;
    }

    if (V_VT(&restrictions) == (VT_VARIANT | VT_ARRAY))
    {
        SAFEARRAY *arr = V_ARRAY(&restrictions);
        LONG ubound, lbound;

        if (SafeArrayGetDim( arr ) != 1) return MAKE_ADO_HRESULT( adErrInvalidArgument );
        if (FAILED((hr = SafeArrayGetUBound( arr, 1, &ubound )))) return hr;
        if (FAILED((hr = SafeArrayGetLBound( arr, 1, &lbound )))) return hr;
        if (FAILED((hr = SafeArrayAccessData( arr, (void **)&restr )))) return hr;
        restr_count = ubound - lbound + 1;
    }
    else if (V_VT(&restrictions) != VT_ERROR || V_ERROR(&restrictions) != DISP_E_PARAMNOTFOUND)
    {
        FIXME( "restrictions = %s\n", debugstr_variant(&restrictions) );
        return E_NOTIMPL;
    }
    else
    {
        restr_count = 0;
        restr = NULL;
    }

    hr = IUnknown_QueryInterface( connection->session, &IID_IDBSchemaRowset, (void**)&schema_rowset );
    if (FAILED(hr))
    {
        if (restr) SafeArrayUnaccessData( V_ARRAY(&restrictions) );
        return MAKE_ADO_HRESULT( adErrFeatureNotAvailable );
    }

    switch(schema)
    {
    case adSchemaTables:
        if (restr_count > CRESTRICTIONS_DBSCHEMA_TABLES)
            hr = MAKE_ADO_HRESULT( adErrFeatureNotAvailable );
        guid = &DBSCHEMA_TABLES;
        break;
    default:
        FIXME( "unsupported schema: %d\n", schema );
        hr = E_NOTIMPL;
        break;
    }

    if (SUCCEEDED(hr))
    {
        hr = IDBSchemaRowset_GetRowset( schema_rowset, NULL, guid,
                    restr_count, restr, &IID_IRowset, 0, NULL, &rowset );
    }
    if (restr) SafeArrayUnaccessData( V_ARRAY(&restrictions) );
    IDBSchemaRowset_Release( schema_rowset );
    if (FAILED(hr)) return hr;

    hr = Recordset_create( (void **)&recordset );
    if (FAILED(hr))
    {
        IUnknown_Release( rowset );
        return hr;
    }

    hr = _Recordset_QueryInterface( recordset, &IID_ADORecordsetConstruction, (void**)&construct );
    if (SUCCEEDED(hr))
    {
        hr = ADORecordsetConstruction_put_Rowset( construct, rowset );
        ADORecordsetConstruction_Release( construct );
    }
    IUnknown_Release( rowset );
    if (FAILED(hr)) return hr;

    *record_set = recordset;
    return S_OK;
}

static HRESULT WINAPI connection_Cancel( _Connection *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static const struct _ConnectionVtbl connection_vtbl =
{
    connection_QueryInterface,
    connection_AddRef,
    connection_Release,
    connection_GetTypeInfoCount,
    connection_GetTypeInfo,
    connection_GetIDsOfNames,
    connection_Invoke,
    connection_get_Properties,
    connection_get_ConnectionString,
    connection_put_ConnectionString,
    connection_get_CommandTimeout,
    connection_put_CommandTimeout,
    connection_get_ConnectionTimeout,
    connection_put_ConnectionTimeout,
    connection_get_Version,
    connection_Close,
    connection_Execute,
    connection_BeginTrans,
    connection_CommitTrans,
    connection_RollbackTrans,
    connection_Open,
    connection_get_Errors,
    connection_get_DefaultDatabase,
    connection_put_DefaultDatabase,
    connection_get_IsolationLevel,
    connection_put_IsolationLevel,
    connection_get_Attributes,
    connection_put_Attributes,
    connection_get_CursorLocation,
    connection_put_CursorLocation,
    connection_get_Mode,
    connection_put_Mode,
    connection_get_Provider,
    connection_put_Provider,
    connection_get_State,
    connection_OpenSchema,
    connection_Cancel
};

static HRESULT WINAPI supporterror_QueryInterface( ISupportErrorInfo *iface, REFIID riid, void **obj )
{
    struct connection *connection = impl_from_ISupportErrorInfo( iface );
    return connection_QueryInterface( &connection->Connection_iface, riid, obj );
}

static ULONG WINAPI supporterror_AddRef( ISupportErrorInfo *iface )
{
    struct connection *connection = impl_from_ISupportErrorInfo( iface );
    return connection_AddRef( &connection->Connection_iface );
}

static ULONG WINAPI supporterror_Release( ISupportErrorInfo *iface )
{
    struct connection *connection = impl_from_ISupportErrorInfo( iface );
    return connection_Release( &connection->Connection_iface );
}

static HRESULT WINAPI supporterror_InterfaceSupportsErrorInfo( ISupportErrorInfo *iface, REFIID riid )
{
    struct connection *connection = impl_from_ISupportErrorInfo( iface );
    FIXME( "%p, %s\n", connection, debugstr_guid(riid) );
    return S_FALSE;
}

static const struct ISupportErrorInfoVtbl support_error_vtbl =
{
    supporterror_QueryInterface,
    supporterror_AddRef,
    supporterror_Release,
    supporterror_InterfaceSupportsErrorInfo
};

static HRESULT WINAPI connpointcontainer_QueryInterface( IConnectionPointContainer *iface,
        REFIID riid, void **obj )
{
    struct connection *connection = impl_from_IConnectionPointContainer( iface );
    return connection_QueryInterface( &connection->Connection_iface, riid, obj );
}

static ULONG WINAPI connpointcontainer_AddRef( IConnectionPointContainer *iface )
{
    struct connection *connection = impl_from_IConnectionPointContainer( iface );
    return connection_AddRef( &connection->Connection_iface );
}

static ULONG WINAPI connpointcontainer_Release( IConnectionPointContainer *iface )
{
    struct connection *connection = impl_from_IConnectionPointContainer( iface );
    return connection_Release( &connection->Connection_iface );
}

static HRESULT WINAPI connpointcontainer_EnumConnectionPoints( IConnectionPointContainer *iface,
        IEnumConnectionPoints **points )
{
    struct connection *connection = impl_from_IConnectionPointContainer( iface );
    FIXME( "%p, %p\n", connection, points );
    return E_NOTIMPL;
}

static HRESULT WINAPI connpointcontainer_FindConnectionPoint( IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **point )
{
    struct connection *connection = impl_from_IConnectionPointContainer( iface );

    TRACE( "%p, %s %p\n", connection, debugstr_guid( riid ), point );

    if (!point) return E_POINTER;

    if (IsEqualIID( riid, connection->cp_connev.riid ))
    {
        *point = &connection->cp_connev.IConnectionPoint_iface;
        IConnectionPoint_AddRef( *point );
        return S_OK;
    }

    FIXME( "unsupported connection point %s\n", debugstr_guid( riid ) );
    return CONNECT_E_NOCONNECTION;
}

static const struct IConnectionPointContainerVtbl connpointcontainer_vtbl =
{
    connpointcontainer_QueryInterface,
    connpointcontainer_AddRef,
    connpointcontainer_Release,
    connpointcontainer_EnumConnectionPoints,
    connpointcontainer_FindConnectionPoint
};

static HRESULT WINAPI connpoint_QueryInterface( IConnectionPoint *iface, REFIID riid, void **obj )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );

    if (IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IConnectionPoint, riid ))
    {
        *obj = &connpoint->IConnectionPoint_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid( riid ) );
        return E_NOINTERFACE;
    }

    connection_AddRef( &connpoint->conn->Connection_iface );
    return S_OK;
}

static ULONG WINAPI connpoint_AddRef( IConnectionPoint *iface )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    return IConnectionPointContainer_AddRef( &connpoint->conn->IConnectionPointContainer_iface );
}

static ULONG WINAPI connpoint_Release( IConnectionPoint *iface )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    return IConnectionPointContainer_Release( &connpoint->conn->IConnectionPointContainer_iface );
}

static HRESULT WINAPI connpoint_GetConnectionInterface( IConnectionPoint *iface, IID *iid )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %p\n", connpoint, iid );
    return E_NOTIMPL;
}

static HRESULT WINAPI connpoint_GetConnectionPointContainer( IConnectionPoint *iface,
        IConnectionPointContainer **container )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %p\n", connpoint, container );
    return E_NOTIMPL;
}

static HRESULT WINAPI connpoint_Advise( IConnectionPoint *iface, IUnknown *unk_sink,
        DWORD *cookie )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    IUnknown *sink, **tmp;
    ULONG new_size;
    HRESULT hr;
    DWORD i;

    TRACE( "%p, %p, %p\n", iface, unk_sink, cookie );

    if (!unk_sink || !cookie) return E_FAIL;

    if (FAILED(hr = IUnknown_QueryInterface( unk_sink, &IID_ConnectionEventsVt, (void**)&sink )))
    {
        *cookie = 0;
        return E_FAIL;
    }

    if (connpoint->sinks)
    {
        for (i = 0; i < connpoint->sinks_size; ++i)
        {
            if (!connpoint->sinks[i])
                break;
        }

        if (i == connpoint->sinks_size)
        {
            new_size = connpoint->sinks_size * 2;
            if (!(tmp = realloc( connpoint->sinks, new_size * sizeof(*connpoint->sinks) )))
                return E_OUTOFMEMORY;
            memset( tmp + connpoint->sinks_size, 0, (new_size - connpoint->sinks_size) * sizeof(*connpoint->sinks) );
            connpoint->sinks = tmp;
            connpoint->sinks_size = new_size;
        }
    }
    else
    {
        if (!(connpoint->sinks = calloc( 1, sizeof(*connpoint->sinks) ))) return E_OUTOFMEMORY;
        connpoint->sinks_size = 1;
        i = 0;
    }

    connpoint->sinks[i] = sink;
    *cookie = i + 1;
    return S_OK;
}

static HRESULT WINAPI connpoint_Unadvise( IConnectionPoint *iface, DWORD cookie )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    TRACE( "%p, %lu\n", connpoint, cookie );

    if (!cookie || cookie > connpoint->sinks_size || !connpoint->sinks || !connpoint->sinks[cookie - 1])
        return E_FAIL;

    IUnknown_Release( connpoint->sinks[cookie - 1] );
    connpoint->sinks[cookie - 1] = NULL;
    return S_OK;
}

static HRESULT WINAPI connpoint_EnumConnections( IConnectionPoint *iface,
        IEnumConnections **points )
{
    struct connection_point *connpoint = impl_from_IConnectionPoint( iface );
    FIXME( "%p, %p\n", connpoint, points );
    return E_NOTIMPL;
}

static const IConnectionPointVtbl connpoint_vtbl =
{
    connpoint_QueryInterface,
    connpoint_AddRef,
    connpoint_Release,
    connpoint_GetConnectionInterface,
    connpoint_GetConnectionPointContainer,
    connpoint_Advise,
    connpoint_Unadvise,
    connpoint_EnumConnections
};

static HRESULT WINAPI adoconstruct_QueryInterface(ADOConnectionConstruction15 *iface, REFIID riid, void **obj)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    return _Connection_QueryInterface( &connection->Connection_iface, riid, obj );
}

static ULONG WINAPI adoconstruct_AddRef(ADOConnectionConstruction15 *iface)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    return _Connection_AddRef( &connection->Connection_iface );
}

static ULONG WINAPI adoconstruct_Release(ADOConnectionConstruction15 *iface)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    return _Connection_Release( &connection->Connection_iface );
}

static HRESULT WINAPI adoconstruct_get_DSO(ADOConnectionConstruction15 *iface, IUnknown **dso)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    FIXME("%p, %p\n", connection, dso);
    return E_NOTIMPL;
}

static HRESULT WINAPI adoconstruct_get_Session(ADOConnectionConstruction15 *iface, IUnknown **session)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    TRACE("%p, %p\n", connection, session);

    *session = connection->session;
    if (*session)
        IUnknown_AddRef(*session);
    return S_OK;
}

static HRESULT WINAPI adoconstruct_WrapDSOandSession(ADOConnectionConstruction15 *iface, IUnknown *dso,
        IUnknown *session)
{
    struct connection *connection = impl_from_ADOConnectionConstruction15( iface );
    FIXME("%p, %p, %p\n", connection, dso, session);
    return E_NOTIMPL;
}

struct ADOConnectionConstruction15Vtbl ado_construct_vtbl =
{
    adoconstruct_QueryInterface,
    adoconstruct_AddRef,
    adoconstruct_Release,
    adoconstruct_get_DSO,
    adoconstruct_get_Session,
    adoconstruct_WrapDSOandSession
};

HRESULT Connection_create( void **obj )
{
    struct connection *connection;

    if (!(connection = malloc( sizeof(*connection) ))) return E_OUTOFMEMORY;
    connection->Connection_iface.lpVtbl = &connection_vtbl;
    connection->ISupportErrorInfo_iface.lpVtbl = &support_error_vtbl;
    connection->IConnectionPointContainer_iface.lpVtbl = &connpointcontainer_vtbl;
    connection->ADOConnectionConstruction15_iface.lpVtbl = &ado_construct_vtbl;
    connection->refs = 1;
    connection->state = adStateClosed;
    connection->timeout = 30;
    connection->datasource = NULL;
    if (!(connection->provider = wcsdup( L"MSDASQL" )))
    {
        free( connection );
        return E_OUTOFMEMORY;
    }
    connection->mode = adModeUnknown;
    connection->location = adUseServer;
    connection->session = NULL;

    connection->cp_connev.conn = connection;
    connection->cp_connev.riid = &DIID_ConnectionEvents;
    connection->cp_connev.IConnectionPoint_iface.lpVtbl = &connpoint_vtbl;
    connection->cp_connev.sinks = NULL;
    connection->cp_connev.sinks_size = 0;

    *obj = &connection->Connection_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
