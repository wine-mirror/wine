/*
 * Copyright 2020 Alistair Leslie-Hughes
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "msdasc.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "msdasql.h"

#include "msdasql_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdasql);

struct msdasql_session
{
    IUnknown session_iface;
    IGetDataSource IGetDataSource_iface;
    IOpenRowset    IOpenRowset_iface;
    ISessionProperties ISessionProperties_iface;
    IDBCreateCommand IDBCreateCommand_iface;
    LONG refs;
};

static inline struct msdasql_session *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, session_iface );
}

static inline struct msdasql_session *impl_from_IGetDataSource( IGetDataSource *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, IGetDataSource_iface );
}

static inline struct msdasql_session *impl_from_IOpenRowset( IOpenRowset *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, IOpenRowset_iface );
}

static inline struct msdasql_session *impl_from_ISessionProperties( ISessionProperties *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, ISessionProperties_iface );
}

static inline struct msdasql_session *impl_from_IDBCreateCommand( IDBCreateCommand *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, IDBCreateCommand_iface );
}

static HRESULT WINAPI session_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct msdasql_session *session = impl_from_IUnknown( iface );

    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), ppv );
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = &session->session_iface;
    }
    else if(IsEqualGUID(&IID_IGetDataSource, riid))
    {
        TRACE("(%p)->(IID_IGetDataSource %p)\n", iface, ppv);
        *ppv = &session->IGetDataSource_iface;
    }
    else if(IsEqualGUID(&IID_IOpenRowset, riid))
    {
        TRACE("(%p)->(IID_IOpenRowset %p)\n", iface, ppv);
        *ppv = &session->IOpenRowset_iface;
    }
    else if(IsEqualGUID(&IID_ISessionProperties, riid))
    {
        TRACE("(%p)->(IID_ISessionProperties %p)\n", iface, ppv);
        *ppv = &session->ISessionProperties_iface;
    }
    else if(IsEqualGUID(&IID_IDBCreateCommand, riid))
    {
        TRACE("(%p)->(IDBCreateCommand_iface %p)\n", iface, ppv);
        *ppv = &session->IDBCreateCommand_iface;
    }

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI session_AddRef(IUnknown *iface)
{
    struct msdasql_session *session = impl_from_IUnknown( iface );
    LONG refs = InterlockedIncrement( &session->refs );
    TRACE( "%p new refcount %d\n", session, refs );
    return refs;
}

static ULONG WINAPI session_Release(IUnknown *iface)
{
    struct msdasql_session *session = impl_from_IUnknown( iface );
    LONG refs = InterlockedDecrement( &session->refs );
    TRACE( "%p new refcount %d\n", session, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", session );
        heap_free( session );
    }
    return refs;
}

static const IUnknownVtbl unkfactoryVtbl =
{
    session_QueryInterface,
    session_AddRef,
    session_Release,
};


HRESULT WINAPI datasource_QueryInterface(IGetDataSource *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_IGetDataSource( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

ULONG WINAPI datasource_AddRef(IGetDataSource *iface)
{
    struct msdasql_session *session = impl_from_IGetDataSource( iface );
    return IUnknown_AddRef(&session->session_iface);
}

ULONG WINAPI datasource_Release(IGetDataSource *iface)
{
    struct msdasql_session *session = impl_from_IGetDataSource( iface );
    return IUnknown_Release(&session->session_iface);
}

HRESULT WINAPI datasource_GetDataSource(IGetDataSource *iface, REFIID riid, IUnknown **datasource)
{
    struct msdasql_session *session = impl_from_IGetDataSource( iface );
    FIXME("%p, %s, %p stub\n", session, debugstr_guid(riid), datasource);

    return E_NOTIMPL;
}

static const IGetDataSourceVtbl datasourceVtbl =
{
    datasource_QueryInterface,
    datasource_AddRef,
    datasource_Release,
    datasource_GetDataSource
};

HRESULT WINAPI openrowset_QueryInterface(IOpenRowset *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_IOpenRowset( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

ULONG WINAPI openrowset_AddRef(IOpenRowset *iface)
{
    struct msdasql_session *session = impl_from_IOpenRowset( iface );
    return IUnknown_AddRef(&session->session_iface);
}

ULONG WINAPI openrowset_Release(IOpenRowset *iface)
{
    struct msdasql_session *session = impl_from_IOpenRowset( iface );
    return IUnknown_Release(&session->session_iface);
}

HRESULT WINAPI openrowset_OpenRowset(IOpenRowset *iface, IUnknown *pUnkOuter, DBID *table,
            DBID *index, REFIID riid, ULONG count, DBPROPSET propertysets[], IUnknown **rowset)
{
    struct msdasql_session *session = impl_from_IOpenRowset( iface );
    FIXME("%p, %p, %p %p %s, %d %p %p stub\n", session, pUnkOuter, table, index, debugstr_guid(riid),
            count, propertysets, rowset);

    return E_NOTIMPL;
}

static const IOpenRowsetVtbl openrowsetVtbl =
{
    openrowset_QueryInterface,
    openrowset_AddRef,
    openrowset_Release,
    openrowset_OpenRowset
};

static HRESULT WINAPI properties_QueryInterface(ISessionProperties *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

static ULONG WINAPI properties_AddRef(ISessionProperties *iface)
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    return IUnknown_AddRef(&session->session_iface);
}

static ULONG WINAPI properties_Release(ISessionProperties *iface)
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    return IUnknown_Release(&session->session_iface);
}


static HRESULT WINAPI properties_GetProperties(ISessionProperties *iface, ULONG set_count,
    const DBPROPIDSET id_sets[], ULONG *count, DBPROPSET **sets)
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    FIXME("%p %d %p %p %p\n", session, set_count, id_sets, count, sets);

    return E_NOTIMPL;
}

static HRESULT WINAPI properties_SetProperties(ISessionProperties *iface, ULONG count,
    DBPROPSET sets[])
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    FIXME("%p %d %p\n", session, count, sets);

    return S_OK;
}

static const ISessionPropertiesVtbl propertiesVtbl =
{
    properties_QueryInterface,
    properties_AddRef,
    properties_Release,
    properties_GetProperties,
    properties_SetProperties
};

static HRESULT WINAPI createcommand_QueryInterface(IDBCreateCommand *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_IDBCreateCommand( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

static ULONG WINAPI createcommand_AddRef(IDBCreateCommand *iface)
{
    struct msdasql_session *session = impl_from_IDBCreateCommand( iface );
    return IUnknown_AddRef(&session->session_iface);
}

static ULONG WINAPI createcommand_Release(IDBCreateCommand *iface)
{
    struct msdasql_session *session = impl_from_IDBCreateCommand( iface );
    return IUnknown_Release(&session->session_iface);
}

struct command
{
    ICommandText ICommandText_iface;
    ICommandProperties ICommandProperties_iface;
    IColumnsInfo IColumnsInfo_iface;
    IConvertType IConvertType_iface;
    LONG refs;
};

static inline struct command *impl_from_ICommandText( ICommandText *iface )
{
    return CONTAINING_RECORD( iface, struct command, ICommandText_iface );
}

static inline struct command *impl_from_ICommandProperties( ICommandProperties *iface )
{
    return CONTAINING_RECORD( iface, struct command, ICommandProperties_iface );
}

static inline struct command *impl_from_IColumnsInfo( IColumnsInfo *iface )
{
    return CONTAINING_RECORD( iface, struct command, IColumnsInfo_iface );
}

static inline struct command *impl_from_IConvertType( IConvertType *iface )
{
    return CONTAINING_RECORD( iface, struct command, IConvertType_iface );
}

static HRESULT WINAPI command_QueryInterface(ICommandText *iface, REFIID riid, void **ppv)
{
    struct command *command = impl_from_ICommandText( iface );

    TRACE( "%p, %s, %p\n", command, debugstr_guid(riid), ppv );
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_ICommand, riid) ||
       IsEqualGUID(&IID_ICommandText, riid))
    {
        *ppv = &command->ICommandText_iface;
    }
    else if(IsEqualGUID(&IID_ICommandProperties, riid))
    {
         *ppv = &command->ICommandProperties_iface;
    }
    else if(IsEqualGUID(&IID_IColumnsInfo, riid))
    {
         *ppv = &command->IColumnsInfo_iface;
    }
    else if(IsEqualGUID(&IID_IConvertType, riid))
    {
         *ppv = &command->IConvertType_iface;
    }

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IMultipleResults, riid))
    {
        TRACE("IID_IMultipleResults not supported\n");
        return E_NOINTERFACE;
    }
    else if(IsEqualGUID(&IID_ICommandStream, riid))
    {
        TRACE("ICommandStream not support\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualGUID(&IID_IRowsetChange, riid))
    {
        TRACE("IID_IRowsetChange not supported\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualGUID(&IID_IRowsetUpdate, riid))
    {
        TRACE("IID_IRowsetUpdate not supported\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualGUID(&IID_IRowsetLocate, riid))
    {
        TRACE("IID_IRowsetLocate not supported\n");
        return E_NOINTERFACE;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI command_AddRef(ICommandText *iface)
{
    struct command *command = impl_from_ICommandText( iface );
    LONG refs = InterlockedIncrement( &command->refs );
    TRACE( "%p new refcount %d\n", command, refs );
    return refs;
}

static ULONG WINAPI command_Release(ICommandText *iface)
{
    struct command *command = impl_from_ICommandText( iface );
    LONG refs = InterlockedDecrement( &command->refs );
    TRACE( "%p new refcount %d\n", command, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", command );
        heap_free( command );
    }
    return refs;
}

static HRESULT WINAPI command_Cancel(ICommandText *iface)
{
    struct command *command = impl_from_ICommandText( iface );
    FIXME("%p\n", command);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_Execute(ICommandText *iface, IUnknown *outer, REFIID riid,
        DBPARAMS *params, DBROWCOUNT *affected, IUnknown **rowset)
{
    struct command *command = impl_from_ICommandText( iface );
    FIXME("%p, %p, %s, %p %p %p\n", command, outer, debugstr_guid(riid), params, affected, rowset);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_GetDBSession(ICommandText *iface, REFIID riid, IUnknown **session)
{
    struct command *command = impl_from_ICommandText( iface );
    FIXME("%p, %s, %p\n", command, debugstr_guid(riid), session);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_GetCommandText(ICommandText *iface, GUID *dialect, LPOLESTR *commandstr)
{
    struct command *command = impl_from_ICommandText( iface );
    FIXME("%p, %p, %p\n", command, dialect, commandstr);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_SetCommandText(ICommandText *iface, REFGUID dialect, LPCOLESTR commandstr)
{
    struct command *command = impl_from_ICommandText( iface );
    FIXME("%p, %s, %s\n", command, debugstr_guid(dialect), debugstr_w(commandstr));
    return S_OK;
}

static const ICommandTextVtbl commandVtbl =
{
    command_QueryInterface,
    command_AddRef,
    command_Release,
    command_Cancel,
    command_Execute,
    command_GetDBSession,
    command_GetCommandText,
    command_SetCommandText
};

static HRESULT WINAPI command_prop_QueryInterface(ICommandProperties *iface, REFIID riid, void **out)
{
    struct command *command = impl_from_ICommandProperties( iface );
    return ICommandText_QueryInterface(&command->ICommandText_iface, riid, out);
}

static ULONG WINAPI command_prop_AddRef(ICommandProperties *iface)
{
    struct command *command = impl_from_ICommandProperties( iface );
    return ICommandText_AddRef(&command->ICommandText_iface);
}

static ULONG WINAPI command_prop_Release(ICommandProperties *iface)
{
    struct command *command = impl_from_ICommandProperties( iface );
    return ICommandText_Release(&command->ICommandText_iface);
}

static HRESULT WINAPI command_prop_GetProperties(ICommandProperties *iface, ULONG count,
        const DBPROPIDSET propertyidsets[], ULONG *sets_cnt, DBPROPSET **propertyset)
{
    struct command *command = impl_from_ICommandProperties( iface );
    FIXME("%p %d %p %p %p\n", command, count, propertyidsets, sets_cnt, propertyset);
    return E_NOTIMPL;
}

static HRESULT WINAPI command_prop_SetProperties(ICommandProperties *iface, ULONG count,
        DBPROPSET propertyset[])
{
    struct command *command = impl_from_ICommandProperties( iface );
    FIXME("%p %p\n", command, propertyset);
    return S_OK;
}

static const ICommandPropertiesVtbl commonpropsVtbl =
{
    command_prop_QueryInterface,
    command_prop_AddRef,
    command_prop_Release,
    command_prop_GetProperties,
    command_prop_SetProperties
};

static HRESULT WINAPI colsinfo_QueryInterface(IColumnsInfo *iface, REFIID riid, void **out)
{
    struct command *command = impl_from_IColumnsInfo( iface );
    return ICommandText_QueryInterface(&command->ICommandText_iface, riid, out);
}

static ULONG WINAPI colsinfo_AddRef(IColumnsInfo *iface)
{
    struct command *command = impl_from_IColumnsInfo( iface );
    return ICommandText_AddRef(&command->ICommandText_iface);
}

static ULONG  WINAPI colsinfo_Release(IColumnsInfo *iface)
{
    struct command *command = impl_from_IColumnsInfo( iface );
    return ICommandText_Release(&command->ICommandText_iface);
}

static HRESULT WINAPI colsinfo_GetColumnInfo(IColumnsInfo *iface, DBORDINAL *columns,
        DBCOLUMNINFO **colinfo, OLECHAR **stringsbuffer)
{
    struct command *command = impl_from_IColumnsInfo( iface );
    FIXME("%p, %p, %p, %p\n", command, columns, colinfo, stringsbuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI colsinfo_MapColumnIDs(IColumnsInfo *iface, DBORDINAL column_ids,
        const DBID *dbids, DBORDINAL *columns)
{
    struct command *command = impl_from_IColumnsInfo( iface );
    FIXME("%p, %lu, %p, %p\n", command, column_ids, dbids, columns);
    return E_NOTIMPL;
}

static struct IColumnsInfoVtbl columninfoVtbl =
{
    colsinfo_QueryInterface,
    colsinfo_AddRef,
    colsinfo_Release,
    colsinfo_GetColumnInfo,
    colsinfo_MapColumnIDs
};

static HRESULT WINAPI converttype_QueryInterface(IConvertType *iface, REFIID riid, void **out)
{
    struct command *command = impl_from_IConvertType( iface );
    return ICommandText_QueryInterface(&command->ICommandText_iface, riid, out);
}

static ULONG WINAPI converttype_AddRef(IConvertType *iface)
{
    struct command *command = impl_from_IConvertType( iface );
    return ICommandText_AddRef(&command->ICommandText_iface);
}

static ULONG WINAPI converttype_Release(IConvertType *iface)
{
    struct command *command = impl_from_IConvertType( iface );
    return ICommandText_Release(&command->ICommandText_iface);
}

static HRESULT WINAPI converttype_CanConvert(IConvertType *iface, DBTYPE from, DBTYPE to, DBCONVERTFLAGS flags)
{
    struct command *command = impl_from_IConvertType( iface );
    FIXME("%p, %u, %d, 0x%08x\n", command, from, to, flags);
    return E_NOTIMPL;
}

static struct IConvertTypeVtbl converttypeVtbl =
{
    converttype_QueryInterface,
    converttype_AddRef,
    converttype_Release,
    converttype_CanConvert
};

static HRESULT WINAPI createcommand_CreateCommand(IDBCreateCommand *iface, IUnknown *outer, REFIID riid,
                                            IUnknown **out)
{
    struct msdasql_session *session = impl_from_IDBCreateCommand( iface );
    struct command *command;
    HRESULT hr;

    TRACE("%p, %p, %s, %p\n", session, outer, debugstr_guid(riid), out);

    if (outer)
        FIXME("Outer not currently supported\n");

    command = heap_alloc(sizeof(*command));
    if (!command)
        return E_OUTOFMEMORY;

    command->ICommandText_iface.lpVtbl = &commandVtbl;
    command->ICommandProperties_iface.lpVtbl = &commonpropsVtbl;
    command->IColumnsInfo_iface.lpVtbl = &columninfoVtbl;
    command->IConvertType_iface.lpVtbl = &converttypeVtbl;
    command->refs = 1;

    hr = ICommandText_QueryInterface(&command->ICommandText_iface, riid, (void**)out);
    ICommandText_Release(&command->ICommandText_iface);
    return hr;
}

static const IDBCreateCommandVtbl createcommandVtbl =
{
    createcommand_QueryInterface,
    createcommand_AddRef,
    createcommand_Release,
    createcommand_CreateCommand
};

HRESULT create_db_session(REFIID riid, void **unk)
{
    struct msdasql_session *session;
    HRESULT hr;

    session = heap_alloc(sizeof(*session));
    if (!session)
        return E_OUTOFMEMORY;

    session->session_iface.lpVtbl = &unkfactoryVtbl;
    session->IGetDataSource_iface.lpVtbl = &datasourceVtbl;
    session->IOpenRowset_iface.lpVtbl = &openrowsetVtbl;
    session->ISessionProperties_iface.lpVtbl = &propertiesVtbl;
    session->IDBCreateCommand_iface.lpVtbl = &createcommandVtbl;
    session->refs = 1;

    hr = IUnknown_QueryInterface(&session->session_iface, riid, unk);
    IUnknown_Release(&session->session_iface);
    return hr;
}
