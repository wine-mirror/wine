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
    session->refs = 1;

    hr = IUnknown_QueryInterface(&session->session_iface, riid, unk);
    IUnknown_Release(&session->session_iface);
    return hr;
}
