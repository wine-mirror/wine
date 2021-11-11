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
#include "oledberr.h"
#include "sqlucode.h"

#include "msdasql_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdasql);

struct msdasql_prop
{
    DBPROPID    property_id;
    DBPROPFLAGS flags;
    VARTYPE     vartype;

    LONG value;
};

static struct msdasql_prop msdasql_init_props[] =
{
    { DBPROP_ABORTPRESERVE,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_DATASOURCEINFO, VT_BOOL, VARIANT_FALSE },
    { DBPROP_BLOCKINGSTORAGEOBJECTS,          DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_BOOKMARKS,                       DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_BOOKMARKSKIPPED,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_BOOKMARKTYPE,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 1 },
    { DBPROP_CANFETCHBACKWARDS,               DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_CANHOLDROWS,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_CANSCROLLBACKWARDS,              DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_COLUMNRESTRICT,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_COMMITPRESERVE,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_DELAYSTORAGEOBJECTS,             DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IMMOBILEROWS,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_LITERALBOOKMARKS,                DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_LITERALIDENTITY,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_MAXOPENROWS,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 0 },
    { DBPROP_MAXPENDINGROWS,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4,  0 },
    { DBPROP_MAXROWS,                         DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 0 },
    { DBPROP_NOTIFICATIONPHASES,              DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 31 },
    { DBPROP_OTHERUPDATEDELETE,               DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_OWNINSERT,                       DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_OWNUPDATEDELETE,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_QUICKRESTART ,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_REENTRANTEVENTS,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_REMOVEDELETED,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_REPORTMULTIPLECHANGES,           DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ROWRESTRICT,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ROWTHREADMODEL,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 2 },
    { DBPROP_TRANSACTEDOBJECT,                DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_UPDATABILITY,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 0 },
    { DBPROP_STRONGIDENTITY,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IAccessor,                       DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IColumnsInfo,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IColumnsRowset,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IConnectionPointContainer,       DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowset,                         DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IRowsetChange,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetIdentity,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetInfo,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IRowsetLocate,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetResynch,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetUpdate,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ISupportErrorInfo,               DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_ISequentialStream,               DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_NOTIFYCOLUMNSET,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWDELETE,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWFIRSTCHANGE,            DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWINSERT,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWRESYNCH,                DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWSETRELEASE,             DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWUNDOCHANGE,             DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWUNDODELETE,             DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWUNDOINSERT,             DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_NOTIFYROWUPDATE,                 DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4,  3 },
    { DBPROP_CHANGEINSERTEDROWS,              DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_RETURNPENDINGINSERTS,            DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IConvertType,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_BOOL, VARIANT_TRUE },
    { DBPROP_NOTIFICATIONGRANULARITY,         DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 1 },
    { DBPROP_IMultipleResults,                DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ACCESSORDER,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 1 },
    { DBPROP_BOOKMARKINFO,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 0 },
    { DBPROP_UNIQUEROWS,                      DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetFind,                     DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_IRowsetScroll,                   DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_IRowsetRefresh,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_FINDCOMPAREOPS,                  DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ, VT_I4, 27 },
    { DBPROP_ORDEREDBOOKMARKS,                DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_CLIENTCURSOR,                    DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_TRUE },
    { DBPROP_ABORTPRESERVE,                   DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ACTIVESESSIONS,                  DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_ASYNCTXNCOMMIT,                  DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_CACHE_AUTHINFO,             DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_ENCRYPT_PASSWORD,           DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 0 },
    { DBPROP_AUTH_INTEGRATED,                 DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_I4, 14 },
    { DBPROP_AUTH_MASK_PASSWORD,              DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_PASSWORD,                   DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_PERSIST_ENCRYPTED,          DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO, DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_AUTH_USERID,                     DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
    { DBPROP_BLOCKINGSTORAGEOBJECTS,          DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE, VT_BOOL, VARIANT_FALSE },
};

#define SQLTYPE_TO_STR(x) case x: return #x

static const char *debugstr_sqltype(SQLSMALLINT type)
{
    switch (type)
    {
        SQLTYPE_TO_STR(SQL_DECIMAL);
        SQLTYPE_TO_STR(SQL_CHAR);
        SQLTYPE_TO_STR(SQL_VARCHAR);
        SQLTYPE_TO_STR(SQL_LONGVARCHAR);
        SQLTYPE_TO_STR(SQL_NUMERIC);
        SQLTYPE_TO_STR(SQL_GUID);
        SQLTYPE_TO_STR(SQL_TINYINT);
        SQLTYPE_TO_STR(SQL_SMALLINT);
        SQLTYPE_TO_STR(SQL_INTEGER);
        SQLTYPE_TO_STR(SQL_REAL);
        SQLTYPE_TO_STR(SQL_FLOAT);
        SQLTYPE_TO_STR(SQL_DOUBLE);
        SQLTYPE_TO_STR(SQL_BINARY);
        SQLTYPE_TO_STR(SQL_VARBINARY);
        SQLTYPE_TO_STR(SQL_LONGVARBINARY);
        SQLTYPE_TO_STR(SQL_TYPE_DATE);
        SQLTYPE_TO_STR(SQL_DATE);
        SQLTYPE_TO_STR(SQL_TIME);
        SQLTYPE_TO_STR(SQL_TYPE_TIMESTAMP);
        SQLTYPE_TO_STR(SQL_TIMESTAMP);
        SQLTYPE_TO_STR(SQL_TYPE_TIME);
        SQLTYPE_TO_STR(SQL_BIGINT);
        SQLTYPE_TO_STR(SQL_C_SBIGINT);
        SQLTYPE_TO_STR(SQL_C_SLONG);
        SQLTYPE_TO_STR(SQL_C_ULONG);
        SQLTYPE_TO_STR(SQL_WLONGVARCHAR);
        SQLTYPE_TO_STR(SQL_WCHAR);
        SQLTYPE_TO_STR(SQL_WVARCHAR);
        default:
             return "Unknown";
    }
}

static const char *debugstr_dbtype(DBTYPE type)
{
    switch(type)
    {
        SQLTYPE_TO_STR(DBTYPE_NUMERIC);
        SQLTYPE_TO_STR(DBTYPE_STR);
        SQLTYPE_TO_STR(DBTYPE_GUID);
        SQLTYPE_TO_STR(DBTYPE_I1);
        SQLTYPE_TO_STR(DBTYPE_I2);
        SQLTYPE_TO_STR(DBTYPE_UI2);
        SQLTYPE_TO_STR(DBTYPE_I4);
        SQLTYPE_TO_STR(DBTYPE_I8);
        SQLTYPE_TO_STR(DBTYPE_UI4);
        SQLTYPE_TO_STR(DBTYPE_R4);
        SQLTYPE_TO_STR(DBTYPE_R8);
        SQLTYPE_TO_STR(DBTYPE_BYTES);
        SQLTYPE_TO_STR(DBTYPE_DBDATE);
        SQLTYPE_TO_STR(DBTYPE_DBTIME);
        SQLTYPE_TO_STR(DBTYPE_DATE);
        SQLTYPE_TO_STR(DBTYPE_DBTIMESTAMP);
        SQLTYPE_TO_STR(DBTYPE_WSTR);
        default:
             return "Unknown";
    }
}

static SQLSMALLINT sqltype_to_bindtype(SQLSMALLINT type, BOOL sign)
{
    switch (type)
    {
        case SQL_DECIMAL:
            return DBTYPE_NUMERIC;
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_NUMERIC:
            return DBTYPE_STR;
        case SQL_GUID:
            return DBTYPE_GUID;
        case SQL_TINYINT:
            return DBTYPE_I1;
        case SQL_SMALLINT:
            return sign ? DBTYPE_I2 : DBTYPE_UI2;
        case SQL_INTEGER:
            return sign ? DBTYPE_I4 : DBTYPE_UI4;
        case SQL_REAL:
            return DBTYPE_R4;
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return DBTYPE_R8;
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            return DBTYPE_BYTES;
        case SQL_TYPE_DATE:
            return DBTYPE_DBDATE;
        case SQL_DATE:
            return DBTYPE_DBTIME;
        case SQL_TIME:
            return DBTYPE_DATE;
        case SQL_TYPE_TIMESTAMP:
        case SQL_TIMESTAMP:
            return DBTYPE_DBTIMESTAMP;
        case SQL_TYPE_TIME:
            return DBTYPE_DBTIME;
        case SQL_BIGINT:
        case SQL_C_SBIGINT:
            return DBTYPE_I8;
        case SQL_C_SLONG:
            return DBTYPE_I4;
        case SQL_C_ULONG:
            return DBTYPE_UI4;
        case SQL_WLONGVARCHAR:
        case SQL_WCHAR:
        case SQL_WVARCHAR:
            return DBTYPE_WSTR;
        default:
            FIXME("Unsupported type %i\n", type);
    }

    return DBTYPE_I4;
}

static BOOL is_variable_length(SQLSMALLINT type)
{
    switch(type)
    {
        case SQL_LONGVARCHAR:
        case SQL_WLONGVARCHAR:
        case SQL_LONGVARBINARY:
            return TRUE;
    }
    return FALSE;
}

static BOOL is_fixed_legnth(SQLSMALLINT type)
{
    switch(type)
    {
        case SQL_LONGVARCHAR:
        case SQL_WLONGVARCHAR:
        case SQL_WVARCHAR:
        case SQL_LONGVARBINARY:
        case SQL_VARBINARY:
            return FALSE;
    }
    return TRUE;
}

struct msdasql_session
{
    IUnknown session_iface;
    IGetDataSource IGetDataSource_iface;
    IOpenRowset    IOpenRowset_iface;
    ISessionProperties ISessionProperties_iface;
    IDBCreateCommand IDBCreateCommand_iface;
    ITransactionJoin ITransactionJoin_iface;
    ITransaction ITransaction_iface;
    LONG refs;

    IUnknown *datasource;

    HDBC hdbc;
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

static inline struct msdasql_session *impl_from_ITransactionJoin( ITransactionJoin *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, ITransactionJoin_iface );
}

static inline struct msdasql_session *impl_from_ITransaction( ITransaction *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, ITransaction_iface );
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
    else if(IsEqualGUID(&IID_ITransactionJoin, riid))
    {
        TRACE("(%p)->(ITransactionJoin %p)\n", iface, ppv);
        *ppv = &session->ITransactionJoin_iface;
    }
    else if(IsEqualGUID(&IID_ITransaction, riid))
    {
        TRACE("(%p)->(ITransaction %p)\n", iface, ppv);
        *ppv = &session->ITransaction_iface;
    }
    else if(IsEqualGUID(&IID_IBindResource, riid))
    {
        TRACE("(%p)->(IID_IBindResource not support)\n", iface);
        return E_NOINTERFACE;
    }
    else if(IsEqualGUID(&IID_ICreateRow, riid))
    {
        TRACE("(%p)->(IID_ICreateRow not support)\n", iface);
        return E_NOINTERFACE;
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
    TRACE( "%p new refcount %ld\n", session, refs );
    return refs;
}

static ULONG WINAPI session_Release(IUnknown *iface)
{
    struct msdasql_session *session = impl_from_IUnknown( iface );
    LONG refs = InterlockedDecrement( &session->refs );
    TRACE( "%p new refcount %ld\n", session, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", session );

        IUnknown_Release(session->datasource);
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

    TRACE("%p, %s, %p stub\n", session, debugstr_guid(riid), datasource);

    if (!datasource)
        return E_INVALIDARG;

    return IUnknown_QueryInterface(session->datasource, riid, (void**)datasource);
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
    FIXME("%p, %p, %p %p %s, %ld %p %p stub\n", session, pUnkOuter, table, index, debugstr_guid(riid),
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
    FIXME("%p %lu %p %p %p\n", session, set_count, id_sets, count, sets);

    return E_NOTIMPL;
}

static HRESULT WINAPI properties_SetProperties(ISessionProperties *iface, ULONG count,
    DBPROPSET sets[])
{
    struct msdasql_session *session = impl_from_ISessionProperties( iface );
    FIXME("%p %lu %p\n", session, count, sets);

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
    ICommandPrepare ICommandPrepare_iface;
    ICommandWithParameters ICommandWithParameters_iface;
    LONG refs;
    WCHAR *query;
    IUnknown *session;
    HDBC hdbc;
    SQLHSTMT hstmt;

    struct msdasql_prop *properties;
    LONG prop_count;
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

static inline struct command *impl_from_ICommandPrepare( ICommandPrepare *iface )
{
    return CONTAINING_RECORD( iface, struct command, ICommandPrepare_iface );
}

static inline struct command *impl_from_ICommandWithParameters( ICommandWithParameters *iface )
{
    return CONTAINING_RECORD( iface, struct command, ICommandWithParameters_iface );
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
    else if(IsEqualGUID(&IID_ICommandPrepare, riid))
    {
         *ppv = &command->ICommandPrepare_iface;
    }
    else if(IsEqualGUID(&IID_ICommandWithParameters, riid))
    {
        *ppv = &command->ICommandWithParameters_iface;
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
    TRACE( "%p new refcount %ld\n", command, refs );
    return refs;
}

static ULONG WINAPI command_Release(ICommandText *iface)
{
    struct command *command = impl_from_ICommandText( iface );
    LONG refs = InterlockedDecrement( &command->refs );
    TRACE( "%p new refcount %ld\n", command, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", command );
        if (command->properties)
            heap_free(command->properties);
        if (command->session)
            IUnknown_Release(command->session);

        if (command->hstmt)
            SQLFreeHandle(SQL_HANDLE_STMT, command->hstmt);

        heap_free( command->query );
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

struct msdasql_rowset
{
    IRowset IRowset_iface;
    IRowsetInfo IRowsetInfo_iface;
    IColumnsInfo IColumnsInfo_iface;
    IAccessor IAccessor_iface;
    IColumnsRowset IColumnsRowset_iface;
    IUnknown *caller;
    LONG refs;
    SQLHSTMT hstmt;
};

static inline struct msdasql_rowset *impl_from_IRowset( IRowset *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_rowset, IRowset_iface );
}

static inline struct msdasql_rowset *impl_from_IRowsetInfo( IRowsetInfo *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_rowset, IRowsetInfo_iface );
}

static inline struct msdasql_rowset *rowset_impl_from_IColumnsInfo( IColumnsInfo *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_rowset, IColumnsInfo_iface );
}

static inline struct msdasql_rowset *impl_from_IAccessor ( IAccessor *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_rowset, IAccessor_iface );
}

static inline struct msdasql_rowset *impl_from_IColumnsRowset ( IColumnsRowset *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_rowset, IColumnsRowset_iface );
}

static HRESULT WINAPI msdasql_rowset_QueryInterface(IRowset *iface, REFIID riid,  void **ppv)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );

    TRACE( "%p, %s, %p\n", rowset, debugstr_guid(riid), ppv );

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IRowset, riid))
    {
        *ppv = &rowset->IRowset_iface;
    }
    else if (IsEqualGUID(&IID_IRowsetInfo, riid))
    {
        *ppv = &rowset->IRowsetInfo_iface;
    }
    else if (IsEqualGUID(&IID_IColumnsInfo, riid))
    {
        *ppv = &rowset->IColumnsInfo_iface;
    }
    else if(IsEqualGUID(&IID_IAccessor, riid))
    {
         *ppv = &rowset->IAccessor_iface;
    }
    else if(IsEqualGUID(&IID_IColumnsRowset, riid))
    {
         *ppv = &rowset->IColumnsRowset_iface;
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

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI msdasql_rowset_AddRef(IRowset *iface)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    LONG refs = InterlockedIncrement( &rowset->refs );
    TRACE( "%p new refcount %ld\n", rowset, refs );
    return refs;
}

static ULONG WINAPI msdasql_rowset_Release(IRowset *iface)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    LONG refs = InterlockedDecrement( &rowset->refs );
    TRACE( "%p new refcount %ld\n", rowset, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", rowset );

        SQLFreeHandle(SQL_HANDLE_STMT, rowset->hstmt);

        if (rowset->caller)
            IUnknown_Release(rowset->caller);

        heap_free( rowset );
    }
    return refs;
}

static HRESULT WINAPI msdasql_rowset_AddRefRows(IRowset *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    FIXME("%p, %Id, %p, %p, %p\n", rowset, count, rows, ref_counts, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI msdasql_rowset_GetData(IRowset *iface, HROW row, HACCESSOR accessor, void *data)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    FIXME("%p, %Id, %Id, %p\n", rowset, row, accessor, data);
    return E_NOTIMPL;
}

static HRESULT WINAPI msdasql_rowset_GetNextRows(IRowset *iface, HCHAPTER reserved, DBROWOFFSET offset,
        DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    FIXME("%p, %Id, %Id, %Id, %p, %p\n", rowset, reserved, offset, count, obtained, rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI msdasql_rowset_ReleaseRows(IRowset *iface, DBCOUNTITEM count,
        const HROW rows[], DBROWOPTIONS options[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );

    FIXME("%p, %Id, %p, %p, %p, %p\n", rowset, count, rows, options, ref_counts, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI msdasql_rowset_RestartPosition(IRowset *iface, HCHAPTER reserved)
{
    struct msdasql_rowset *rowset = impl_from_IRowset( iface );
    FIXME("%p, %Id\n", rowset, reserved);
    return E_NOTIMPL;
}

static const struct IRowsetVtbl msdasql_rowset_vtbl =
{
    msdasql_rowset_QueryInterface,
    msdasql_rowset_AddRef,
    msdasql_rowset_Release,
    msdasql_rowset_AddRefRows,
    msdasql_rowset_GetData,
    msdasql_rowset_GetNextRows,
    msdasql_rowset_ReleaseRows,
    msdasql_rowset_RestartPosition
};

static HRESULT WINAPI rowset_info_QueryInterface(IRowsetInfo *iface, REFIID riid, void **ppv)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, ppv);
}

static ULONG WINAPI rowset_info_AddRef(IRowsetInfo *iface)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG WINAPI rowset_info_Release(IRowsetInfo *iface)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI rowset_info_GetProperties(IRowsetInfo *iface, const ULONG count,
        const DBPROPIDSET propertyidsets[], ULONG *out_count, DBPROPSET **propertysets)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );
    HRESULT hr;
    ICommandProperties *props;

    TRACE("%p, %lu, %p, %p, %p\n", rowset, count, propertyidsets, out_count, propertysets);

    hr = IUnknown_QueryInterface(rowset->caller, &IID_ICommandProperties, (void**)&props);
    if (FAILED(hr))
        return hr;

    hr = ICommandProperties_GetProperties(props, count, propertyidsets, out_count, propertysets);
    ICommandProperties_Release(props);

    return hr;
}

static HRESULT WINAPI rowset_info_GetReferencedRowset(IRowsetInfo *iface, DBORDINAL ordinal,
        REFIID riid, IUnknown **unk)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );
    FIXME("%p, %Id, %s, %p\n", rowset, ordinal, debugstr_guid(riid), unk);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_info_GetSpecification(IRowsetInfo *iface, REFIID riid,
        IUnknown **specification)
{
    struct msdasql_rowset *rowset = impl_from_IRowsetInfo( iface );

    TRACE("%p, %s, %p\n", rowset, debugstr_guid(riid), specification);

    if (!specification)
        return E_INVALIDARG;

    if (!rowset->caller)
        return S_FALSE;

    return IUnknown_QueryInterface(rowset->caller, riid, (void**)specification);
}

struct IRowsetInfoVtbl rowset_info_vtbl =
{
    rowset_info_QueryInterface,
    rowset_info_AddRef,
    rowset_info_Release,
    rowset_info_GetProperties,
    rowset_info_GetReferencedRowset,
    rowset_info_GetSpecification
};

static HRESULT WINAPI rowset_colsinfo_QueryInterface(IColumnsInfo *iface, REFIID riid, void **out)
{
    struct msdasql_rowset *rowset = rowset_impl_from_IColumnsInfo( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, out);
}

static ULONG WINAPI rowset_colsinfo_AddRef(IColumnsInfo *iface)
{
    struct msdasql_rowset *rowset = rowset_impl_from_IColumnsInfo( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG  WINAPI rowset_colsinfo_Release(IColumnsInfo *iface)
{
    struct msdasql_rowset *rowset = rowset_impl_from_IColumnsInfo( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI rowset_colsinfo_GetColumnInfo(IColumnsInfo *iface, DBORDINAL *columns,
        DBCOLUMNINFO **colinfo, OLECHAR **stringsbuffer)
{
#define MAX_COLUMN_LEN 128

    struct msdasql_rowset *rowset = rowset_impl_from_IColumnsInfo( iface );
    DBCOLUMNINFO *dbcolumn;
    RETCODE ret;
    SQLSMALLINT colcnt;
    int i;
    OLECHAR *ptr;

    TRACE("%p, %p, %p, %p\n", rowset, columns, colinfo, stringsbuffer);

    if (!columns || !colinfo || !stringsbuffer)
        return E_INVALIDARG;

    SQLNumResultCols(rowset->hstmt, &colcnt);
    TRACE("SQLNumResultCols %d\n", colcnt);

    *columns = colcnt;

    ptr = *stringsbuffer = CoTaskMemAlloc(colcnt * MAX_COLUMN_LEN * sizeof(WCHAR));
    if (!ptr)
        return E_OUTOFMEMORY;

    dbcolumn = CoTaskMemAlloc(colcnt * sizeof(DBCOLUMNINFO));
    if (!dbcolumn)
    {
        CoTaskMemFree(ptr);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < colcnt; i++)
    {
        SQLWCHAR      columnname[MAX_COLUMN_LEN];
        SQLSMALLINT   ColumnNameLen;
        SQLSMALLINT   ColumnDataType;
        SQLULEN       ColumnDataSize;
        SQLSMALLINT   ColumnDataDigits;
        SQLSMALLINT   ColumnDataNullable;

        ret = SQLDescribeColW(rowset->hstmt, i+1, columnname, MAX_COLUMN_LEN, &ColumnNameLen, &ColumnDataType,
                    &ColumnDataSize, &ColumnDataDigits, &ColumnDataNullable);
        if (SQL_SUCCEEDED(ret))
        {
            SQLLEN  length;

            TRACE("%d: Column Name : %s, Column Name Len : %i, SQL Data Type : %i, Data Size : %i, DecimalDigits : %i, Nullable %i\n",
                 i, debugstr_w(columnname), (int)ColumnNameLen, (int)ColumnDataType, (int)ColumnDataSize, (int)ColumnDataDigits,
                 (int)ColumnDataNullable);
            lstrcpyW(ptr, columnname);

            dbcolumn[i].pwszName = ptr;
            dbcolumn[i].pTypeInfo = NULL;
            dbcolumn[i].iOrdinal = i+1;

            ret = SQLColAttribute(rowset->hstmt, i+1, SQL_DESC_UNSIGNED, NULL, 0, NULL, &length);
            if (!SQL_SUCCEEDED(ret))
            {
                CoTaskMemFree(ptr);
                CoTaskMemFree(dbcolumn);
                ERR("Failed to get column %d attribute\n", i+1);
                return E_FAIL;
            }

            dbcolumn[i].wType = sqltype_to_bindtype(ColumnDataType, length == SQL_FALSE);
            TRACE("SQLType %s -> %s\n", debugstr_sqltype(ColumnDataType), debugstr_dbtype(dbcolumn[i].wType));

            dbcolumn[i].dwFlags = DBCOLUMNFLAGS_WRITE;

            ret = SQLColAttribute(rowset->hstmt, i+1, SQL_DESC_LENGTH, NULL, 0, NULL, &length);
            if (!SQL_SUCCEEDED(ret))
            {
                CoTaskMemFree(ptr);
                CoTaskMemFree(dbcolumn);
                ERR("Failed to get column %d length (%d)\n", i+1, ret);
                return E_FAIL;
            }
            dbcolumn[i].ulColumnSize = length;

            if (dbcolumn[i].ulColumnSize > 1024 && is_variable_length(ColumnDataType))
                dbcolumn[i].dwFlags |= DBCOLUMNFLAGS_MAYDEFER | DBCOLUMNFLAGS_ISLONG;

            if (ColumnDataNullable)
                dbcolumn[i].dwFlags |= DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_MAYBENULL;

            if (is_fixed_legnth(ColumnDataType))
                dbcolumn[i].dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;

            ret = SQLColAttribute(rowset->hstmt, i+1, SQL_DESC_SCALE, NULL, 0, NULL, &length);
            if (!SQL_SUCCEEDED(ret))
            {
                CoTaskMemFree(ptr);
                CoTaskMemFree(dbcolumn);
                ERR("Failed to get column %d scale (%d)\n", i+1, ret);
                return E_FAIL;
            }
            if (length == 0)
                length = 255;
            dbcolumn[i].bScale = length;

            ret = SQLColAttribute(rowset->hstmt, i+1, SQL_DESC_PRECISION, NULL, 0, NULL, &length);
            if (!SQL_SUCCEEDED(ret))
            {
                CoTaskMemFree(ptr);
                CoTaskMemFree(dbcolumn);
                ERR("Failed to get column %d precision (%d)\n", i+1, ret);
                return E_FAIL;
            }
            if (length == 0)
                length = 255;
            dbcolumn[i].bPrecision= length;

            dbcolumn[i].columnid.eKind = DBKIND_NAME;
            dbcolumn[i].columnid.uName.pwszName = ptr;

            ptr += ColumnNameLen + 1;
        }
        else
        {
            CoTaskMemFree(ptr);
            CoTaskMemFree(dbcolumn);
            ERR("Failed to get column %d description (%d)\n", i+1, ret);
            return E_FAIL;
        }
    }

    *colinfo = dbcolumn;
#undef MAX_COLUMN_LEN
    return S_OK;
}

static HRESULT WINAPI rowset_colsinfo_MapColumnIDs(IColumnsInfo *iface, DBORDINAL column_ids,
        const DBID *dbids, DBORDINAL *columns)
{
    struct msdasql_rowset *rowset = rowset_impl_from_IColumnsInfo( iface );
    FIXME("%p, %Id, %p, %p\n", rowset, column_ids, dbids, columns);
    return E_NOTIMPL;
}

static struct IColumnsInfoVtbl rowset_columninfo_vtbll =
{
    rowset_colsinfo_QueryInterface,
    rowset_colsinfo_AddRef,
    rowset_colsinfo_Release,
    rowset_colsinfo_GetColumnInfo,
    rowset_colsinfo_MapColumnIDs
};

static HRESULT WINAPI rowset_accessor_QueryInterface(IAccessor *iface, REFIID riid, void **out)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, out);
}

static ULONG WINAPI rowset_accessor_AddRef(IAccessor *iface)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG  WINAPI rowset_accessor_Release(IAccessor *iface)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI rowset_accessor_AddRefAccessor(IAccessor *iface, HACCESSOR accessor, DBREFCOUNT *count)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    FIXME("%p, %Id, %p\n", rowset, accessor, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_accessor_CreateAccessor(IAccessor *iface, DBACCESSORFLAGS flags,
        DBCOUNTITEM count, const DBBINDING bindings[], DBLENGTH row_size, HACCESSOR *accessor,
        DBBINDSTATUS status[])
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    FIXME("%p, 0x%08lx, %Id, %p, %Id, %p, %p\n", rowset, flags, count, bindings, row_size, accessor, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_accessor_GetBindings(IAccessor *iface, HACCESSOR accessor,
        DBACCESSORFLAGS *flags, DBCOUNTITEM *count, DBBINDING **bindings)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    FIXME("%p, %Id, %p, %p, %p\n", rowset, accessor, flags, count, bindings);
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_accessor_ReleaseAccessor(IAccessor *iface, HACCESSOR accessor, DBREFCOUNT *count)
{
    struct msdasql_rowset *rowset = impl_from_IAccessor( iface );
    FIXME("%p, %Id, %p\n", rowset, accessor, count);
    return E_NOTIMPL;
}

struct IAccessorVtbl accessor_vtbl =
{
    rowset_accessor_QueryInterface,
    rowset_accessor_AddRef,
    rowset_accessor_Release,
    rowset_accessor_AddRefAccessor,
    rowset_accessor_CreateAccessor,
    rowset_accessor_GetBindings,
    rowset_accessor_ReleaseAccessor
};

static HRESULT WINAPI column_rs_QueryInterface(IColumnsRowset *iface, REFIID riid, void **out)
{
    struct msdasql_rowset *rowset = impl_from_IColumnsRowset( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, out);
}

static ULONG WINAPI column_rs_AddRef(IColumnsRowset *iface)
{
    struct msdasql_rowset *rowset = impl_from_IColumnsRowset( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG WINAPI column_rs_Release(IColumnsRowset *iface)
{
    struct msdasql_rowset *rowset = impl_from_IColumnsRowset( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI column_rs_GetAvailableColumns(IColumnsRowset *iface, DBORDINAL *count, DBID **columns)
{
    struct msdasql_rowset *rowset = impl_from_IColumnsRowset( iface );

    TRACE("%p, %p, %p\n", rowset, count, columns);

    if (!count || !columns)
        return E_INVALIDARG;

    *count = 0;
    *columns = NULL;

    return S_OK;
}

static HRESULT WINAPI column_rs_GetColumnsRowset(IColumnsRowset *iface, IUnknown *outer, DBORDINAL count,
        const DBID columns[], REFIID riid, ULONG property_cnt, DBPROPSET property_sets[], IUnknown **unk_rs)
{
    struct msdasql_rowset *rowset = impl_from_IColumnsRowset( iface );
    FIXME("(%p)->(%p, %Id, %p, %s, %lu, %p, %p): stub\n", rowset, outer, count, columns, debugstr_guid(riid),
          property_cnt, property_sets, unk_rs);
    return E_NOTIMPL;
}

struct IColumnsRowsetVtbl columnrs_rs_vtbl =
{
    column_rs_QueryInterface,
    column_rs_AddRef,
    column_rs_Release,
    column_rs_GetAvailableColumns,
    column_rs_GetColumnsRowset
};

static HRESULT WINAPI command_Execute(ICommandText *iface, IUnknown *outer, REFIID riid,
        DBPARAMS *params, DBROWCOUNT *affected, IUnknown **rowset)
{
    struct command *command = impl_from_ICommandText( iface );
    struct msdasql_rowset *msrowset;
    HRESULT hr = S_OK;
    RETCODE ret;
    SQLHSTMT hstmt = command->hstmt;
    SQLLEN results = -1;

    TRACE("%p, %p, %s, %p %p %p\n", command, outer, debugstr_guid(riid), params, affected, rowset);

    if (!hstmt)
        SQLAllocHandle(SQL_HANDLE_STMT, command->hdbc, &hstmt);

    ret = SQLExecDirectW(hstmt, command->query, SQL_NTS);
    if (ret != SQL_SUCCESS)
    {
        dump_sql_diag_records(SQL_HANDLE_STMT, hstmt);
        return E_FAIL;
    }

    *rowset = NULL;
    if (!wcsnicmp( command->query, L"select ", 7 ))
    {
        msrowset = heap_alloc(sizeof(*msrowset));
        if (!msrowset)
            return E_OUTOFMEMORY;

        command->hstmt = NULL;

        msrowset->IRowset_iface.lpVtbl = &msdasql_rowset_vtbl;
        msrowset->IRowsetInfo_iface.lpVtbl = &rowset_info_vtbl;
        msrowset->IColumnsInfo_iface.lpVtbl = &rowset_columninfo_vtbll;
        msrowset->IAccessor_iface.lpVtbl = &accessor_vtbl;
        msrowset->IColumnsRowset_iface.lpVtbl = &columnrs_rs_vtbl;
        msrowset->refs = 1;
        ICommandText_QueryInterface(iface, &IID_IUnknown, (void**)&msrowset->caller);
        msrowset->hstmt = hstmt;

        hr = IRowset_QueryInterface(&msrowset->IRowset_iface, riid, (void**)rowset);
        IRowset_Release(&msrowset->IRowset_iface);
    }
    else
    {
        ret = SQLRowCount(hstmt, &results);
        if (ret != SQL_SUCCESS)
            ERR("SQLRowCount failed (%d)\n", ret);

        SQLFreeStmt(hstmt, SQL_CLOSE);
    }

    if (affected)
        *affected = results;

    return hr;
}

static HRESULT WINAPI command_GetDBSession(ICommandText *iface, REFIID riid, IUnknown **session)
{
    struct command *command = impl_from_ICommandText( iface );

    TRACE("%p, %s, %p\n", command, debugstr_guid(riid), session);

    if (!session)
        return E_INVALIDARG;

    *session = NULL;

    if (!command->session)
        return S_FALSE;

    return IUnknown_QueryInterface(command->session, riid, (void**)session);
}

static HRESULT WINAPI command_GetCommandText(ICommandText *iface, GUID *dialect, LPOLESTR *commandstr)
{
    struct command *command = impl_from_ICommandText( iface );
    HRESULT hr = S_OK;
    TRACE("%p, %p, %p\n", command, dialect, commandstr);

    if (!command->query)
        return DB_E_NOCOMMAND;

    if (dialect && !IsEqualGUID(&DBGUID_DEFAULT, dialect))
    {
        *dialect = DBGUID_DEFAULT;
        hr = DB_S_DIALECTIGNORED;
    }

    *commandstr = heap_alloc((lstrlenW(command->query)+1)*sizeof(WCHAR));
    wcscpy(*commandstr, command->query);
    return hr;
}

static HRESULT WINAPI command_SetCommandText(ICommandText *iface, REFGUID dialect, LPCOLESTR commandstr)
{
    struct command *command = impl_from_ICommandText( iface );
    TRACE("%p, %s, %s\n", command, debugstr_guid(dialect), debugstr_w(commandstr));

    if (!IsEqualGUID(&DBGUID_DEFAULT, dialect))
        FIXME("Currently non Default Dialect isn't supported\n");

    heap_free(command->query);

    if (commandstr)
    {
        command->query = heap_alloc((lstrlenW(commandstr)+1)*sizeof(WCHAR));
        if (!command->query)
            return E_OUTOFMEMORY;

        wcscpy(command->query, commandstr);
    }
    else
        command->query = NULL;
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

static ULONG get_property_count(DWORD flag, struct msdasql_prop *properties, int prop_count)
{
    int i, count = 0;

    for(i=0; i < prop_count; i++)
    {
        if (properties[i].flags & flag)
            count++;
    }

    return count;
}

static HRESULT WINAPI command_prop_GetProperties(ICommandProperties *iface, ULONG count,
        const DBPROPIDSET propertyidsets[], ULONG *sets_cnt, DBPROPSET **propertyset)
{
    struct command *command = impl_from_ICommandProperties( iface );
    DBPROPSET *propset = NULL;
    int i, j, k;

    TRACE("%p %ld %p %p %p\n", command, count, propertyidsets, sets_cnt, propertyset);

    /* All Properties */
    if (count == 0)
    {
        int idx;
        propset = CoTaskMemAlloc(2 * sizeof(DBPROPSET));
        if (!propset)
            return E_OUTOFMEMORY;

        propset[0].guidPropertySet = DBPROPSET_ROWSET;
        propset[0].cProperties = get_property_count(DBPROPFLAGS_ROWSET, command->properties, command->prop_count);
        propset[0].rgProperties = CoTaskMemAlloc(propset[0].cProperties * sizeof(DBPROP));
        if (!propset[0].rgProperties)
        {
            CoTaskMemFree(propset);
            return E_OUTOFMEMORY;
        }

        idx = 0;
        for (j=0; j < command->prop_count; j++)
        {
            if (!(command->properties[j].flags & DBPROPFLAGS_ROWSET))
                continue;
            propset[0].rgProperties[idx].dwPropertyID = command->properties[j].property_id;

            V_VT(&propset[0].rgProperties[idx].vValue) = command->properties[j].vartype;
            if (command->properties[j].vartype == VT_BOOL)
            {
                V_BOOL(&propset[0].rgProperties[idx].vValue) = command->properties[j].value;
            }
            else if (command->properties[j].vartype == VT_I4)
            {
                V_I4(&propset[0].rgProperties[idx].vValue) = command->properties[j].value;
            }
            else
                ERR("Unknown variant type %d\n", command->properties[j].vartype);

            idx++;
        }

        propset[1].guidPropertySet = DBPROPSET_PROVIDERROWSET;
        propset[1].cProperties = get_property_count(DBPROPFLAGS_DATASOURCEINFO, command->properties, command->prop_count);
        propset[1].rgProperties = CoTaskMemAlloc(propset[1].cProperties * sizeof(DBPROP));
        if (!propset[1].rgProperties)
        {
            CoTaskMemFree(propset[0].rgProperties);
            CoTaskMemFree(propset);
            return E_OUTOFMEMORY;
        }

        idx = 0;
        for (j=0; j < command->prop_count; j++)
        {
            if (!(command->properties[j].flags & DBPROPFLAGS_DATASOURCEINFO))
                continue;
            propset[1].rgProperties[idx].dwPropertyID = command->properties[j].property_id;

            V_VT(&propset[1].rgProperties[idx].vValue) = command->properties[j].vartype;
            if (command->properties[j].vartype == VT_BOOL)
            {
                V_BOOL(&propset[1].rgProperties[idx].vValue) = command->properties[j].value;
            }
            else if (command->properties[j].vartype == VT_I4)
            {
                V_I4(&propset[1].rgProperties[idx].vValue) = command->properties[j].value;
            }
            else
                ERR("Unknown variant type %d\n", command->properties[j].vartype);

            idx++;
        }

        *sets_cnt = 2;
    }
    else
    {
        propset = CoTaskMemAlloc(count * sizeof(DBPROPSET));
        if (!propset)
            return E_OUTOFMEMORY;

        for (i=0; i < count; i++)
        {
            TRACE("Property id %d (count %ld, set %s)\n", i, propertyidsets[i].cPropertyIDs,
                    debugstr_guid(&propertyidsets[i].guidPropertySet));

            propset[i].cProperties = propertyidsets[i].cPropertyIDs;
            propset[i].rgProperties = CoTaskMemAlloc(propset[i].cProperties * sizeof(DBPROP));

            for (j=0; j < propset[i].cProperties; j++)
            {
                propset[i].rgProperties[j].dwPropertyID = propertyidsets[i].rgPropertyIDs[j];

                for(k = 0; k < command->prop_count; k++)
                {
                    if (command->properties[k].property_id == propertyidsets[i].rgPropertyIDs[j])
                    {
                        V_VT(&propset[i].rgProperties[i].vValue) = command->properties[j].vartype;
                        if (command->properties[j].vartype == VT_BOOL)
                        {
                            V_BOOL(&propset[i].rgProperties[i].vValue) = command->properties[j].value;
                        }
                        else if (command->properties[j].vartype == VT_I4)
                        {
                            V_I4(&propset[i].rgProperties[i].vValue) = command->properties[j].value;
                        }
                        else
                            ERR("Unknown variant type %d\n", command->properties[j].vartype);
                        break;
                    }
                }
            }
        }

        *sets_cnt = count;
    }

    *propertyset = propset;

    return S_OK;
}

static HRESULT WINAPI command_prop_SetProperties(ICommandProperties *iface, ULONG count,
        DBPROPSET propertyset[])
{
    struct command *command = impl_from_ICommandProperties( iface );
    int i, j, k;

    TRACE("%p %lu, %p\n", command, count, propertyset);

    for(i=0; i < count; i++)
    {
        TRACE("set %s, count %ld\n", debugstr_guid(&propertyset[i].guidPropertySet), propertyset[i].cProperties);
        for(j=0; j < propertyset[i].cProperties; j++)
        {
            for(k=0; k < command->prop_count; k++)
            {
                if (command->properties[k].property_id == propertyset[i].rgProperties[j].dwPropertyID)
                {
                    TRACE("Found property 0x%08lx\n", command->properties[k].property_id);
                    if (command->properties[k].flags & DBPROPFLAGS_WRITE)
                    {
                        if (command->properties[k].vartype == VT_BOOL)
                        {
                            command->properties[k].value = V_BOOL(&propertyset[i].rgProperties[j].vValue);
                        }
                        else if (command->properties[k].vartype == VT_I4)
                        {
                            command->properties[k].value = V_I4(&propertyset[i].rgProperties[j].vValue);
                        }
                        else
                            ERR("Unknown variant type %d\n", command->properties[j].vartype);
                    }
                    else
                        WARN("Attempting to set Readonly property\n");

                    break;
                }
            }
        }
    }
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
    FIXME("%p, %Iu, %p, %p\n", command, column_ids, dbids, columns);
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
    FIXME("%p, %u, %d, 0x%08lx\n", command, from, to, flags);
    return E_NOTIMPL;
}

static struct IConvertTypeVtbl converttypeVtbl =
{
    converttype_QueryInterface,
    converttype_AddRef,
    converttype_Release,
    converttype_CanConvert
};

static HRESULT WINAPI commandprepare_QueryInterface(ICommandPrepare *iface, REFIID riid, void **out)
{
    struct command *command = impl_from_ICommandPrepare( iface );
    return ICommandText_QueryInterface(&command->ICommandText_iface, riid, out);
}

static ULONG WINAPI commandprepare_AddRef(ICommandPrepare *iface)
{
    struct command *command = impl_from_ICommandPrepare( iface );
    return ICommandText_AddRef(&command->ICommandText_iface);
}

static ULONG WINAPI commandprepare_Release(ICommandPrepare *iface)
{
    struct command *command = impl_from_ICommandPrepare( iface );
    return ICommandText_Release(&command->ICommandText_iface);
}

static HRESULT WINAPI commandprepare_Prepare(ICommandPrepare *iface, ULONG runs)
{
    struct command *command = impl_from_ICommandPrepare( iface );
    RETCODE ret;

    TRACE("%p, %lu\n", command, runs);

    if (!command->query)
        return DB_E_NOCOMMAND;

    if (command->hstmt)
        SQLFreeHandle(SQL_HANDLE_STMT, command->hstmt);

    SQLAllocHandle(SQL_HANDLE_STMT, command->hdbc, &command->hstmt);

    ret = SQLPrepareW(command->hstmt, command->query, SQL_NTS);
    if (ret != SQL_SUCCESS)
    {
        dump_sql_diag_records(SQL_HANDLE_STMT, command->hstmt);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI commandprepare_Unprepare(ICommandPrepare *iface)
{
    struct command *command = impl_from_ICommandPrepare( iface );
    TRACE("%p\n", command);
    return S_OK;
}

struct ICommandPrepareVtbl commandprepareVtbl =
{
    commandprepare_QueryInterface,
    commandprepare_AddRef,
    commandprepare_Release,
    commandprepare_Prepare,
    commandprepare_Unprepare
};

static HRESULT WINAPI cmd_with_params_QueryInterface(ICommandWithParameters *iface, REFIID riid, void **out)
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    return ICommandText_QueryInterface(&command->ICommandText_iface, riid, out);
}

static ULONG WINAPI cmd_with_params_AddRef(ICommandWithParameters *iface)
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    return ICommandText_AddRef(&command->ICommandText_iface);
}

static ULONG WINAPI cmd_with_params_Release(ICommandWithParameters *iface)
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    return ICommandText_Release(&command->ICommandText_iface);
}

static HRESULT WINAPI cmd_with_params_GetParameterInfo(ICommandWithParameters *iface, DB_UPARAMS *uparams,
        DBPARAMINFO **info, OLECHAR **buffer)
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    FIXME("%p, %p, %p, %p\n", command, uparams, info, buffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI cmd_with_params_MapParameterNames(ICommandWithParameters *iface, DB_UPARAMS uparams,
        LPCWSTR names[], DB_LPARAMS ordinals[])
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    FIXME("%p, %Iu, %p, %p\n", command, uparams, names, ordinals);
    return E_NOTIMPL;
}

static HRESULT WINAPI cmd_with_params_SetParameterInfo(ICommandWithParameters *iface, DB_UPARAMS uparams,
        const DB_UPARAMS ordinals[], const DBPARAMBINDINFO bindinfo[])
{
    struct command *command = impl_from_ICommandWithParameters( iface );
    FIXME("%p, %Iu, %p, %p\n", command, uparams, ordinals, bindinfo);
    return E_NOTIMPL;
}

struct ICommandWithParametersVtbl command_with_params_vtbl =
{
    cmd_with_params_QueryInterface,
    cmd_with_params_AddRef,
    cmd_with_params_Release,
    cmd_with_params_GetParameterInfo,
    cmd_with_params_MapParameterNames,
    cmd_with_params_SetParameterInfo
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
    command->ICommandPrepare_iface.lpVtbl = &commandprepareVtbl;
    command->ICommandWithParameters_iface.lpVtbl = &command_with_params_vtbl;
    command->refs = 1;
    command->query = NULL;
    command->hdbc = session->hdbc;
    command->hstmt = NULL;

    command->prop_count = ARRAY_SIZE(msdasql_init_props);
    command->properties = heap_alloc(sizeof(msdasql_init_props));
    memcpy(command->properties, msdasql_init_props, sizeof(msdasql_init_props));

    IUnknown_QueryInterface(&session->session_iface, &IID_IUnknown, (void**)&command->session);

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

static HRESULT WINAPI transjoin_QueryInterface(ITransactionJoin *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_ITransactionJoin( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

static ULONG WINAPI transjoin_AddRef(ITransactionJoin *iface)
{
    struct msdasql_session *session = impl_from_ITransactionJoin( iface );
    return IUnknown_AddRef(&session->session_iface);
}

static ULONG WINAPI transjoin_Release(ITransactionJoin *iface)
{
    struct msdasql_session *session = impl_from_ITransactionJoin( iface );
    return IUnknown_Release(&session->session_iface);
}

static HRESULT WINAPI transjoin_GetOptionsObject(ITransactionJoin *iface, ITransactionOptions **options)
{
    struct msdasql_session *session = impl_from_ITransactionJoin( iface );

    FIXME("%p, %p\n", session, options);

    return E_NOTIMPL;
}

static HRESULT WINAPI transjoin_JoinTransaction(ITransactionJoin *iface, IUnknown *unk, ISOLEVEL level,
    ULONG flags, ITransactionOptions *options)
{
    struct msdasql_session *session = impl_from_ITransactionJoin( iface );

    FIXME("%p, %p, %lu, 0x%08lx, %p\n", session, unk, level, flags, options);

    return E_NOTIMPL;
}

static const ITransactionJoinVtbl transactionjoinVtbl =
{
    transjoin_QueryInterface,
    transjoin_AddRef,
    transjoin_Release,
    transjoin_GetOptionsObject,
    transjoin_JoinTransaction
};

static HRESULT WINAPI transaction_QueryInterface(ITransaction *iface, REFIID riid, void **out)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );
    return IUnknown_QueryInterface(&session->session_iface, riid, out);
}

static ULONG WINAPI transaction_AddRef(ITransaction *iface)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );
    return IUnknown_AddRef(&session->session_iface);
}

static ULONG WINAPI transaction_Release(ITransaction *iface)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );
    return IUnknown_Release(&session->session_iface);
}

static HRESULT WINAPI transaction_Commit(ITransaction *iface, BOOL retaining, DWORD tc, DWORD rm)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );

    FIXME("%p, %d, %ld, %ld\n", session, retaining, tc, rm);

    return E_NOTIMPL;
}

static HRESULT WINAPI transaction_Abort(ITransaction *iface, BOID *reason, BOOL retaining, BOOL async)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );

    FIXME("%p, %p, %d, %d\n", session, reason, retaining, async);

    return E_NOTIMPL;
}

static HRESULT WINAPI transaction_GetTransactionInfo(ITransaction *iface, XACTTRANSINFO *info)
{
    struct msdasql_session *session = impl_from_ITransaction( iface );

    FIXME("%p, %p\n", session, info);

    return E_NOTIMPL;
}

static const ITransactionVtbl transactionVtbl =
{
    transaction_QueryInterface,
    transaction_AddRef,
    transaction_Release,
    transaction_Commit,
    transaction_Abort,
    transaction_GetTransactionInfo
};

HRESULT create_db_session(REFIID riid, IUnknown *datasource, HDBC hdbc, void **unk)
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
    session->ITransactionJoin_iface.lpVtbl = &transactionjoinVtbl;
    session->ITransaction_iface.lpVtbl = &transactionVtbl;

    IUnknown_QueryInterface(datasource, &IID_IUnknown, (void**)&session->datasource);
    session->refs = 1;
    session->hdbc = hdbc;

    hr = IUnknown_QueryInterface(&session->session_iface, riid, unk);
    IUnknown_Release(&session->session_iface);
    return hr;
}
