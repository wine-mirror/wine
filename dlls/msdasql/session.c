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
    LONG refs;
};

static inline struct msdasql_session *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_session, session_iface );
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

HRESULT create_db_session(REFIID riid, void **unk)
{
    struct msdasql_session *session;
    HRESULT hr;

    session = heap_alloc(sizeof(*session));
    if (!session)
        return E_OUTOFMEMORY;

    session->session_iface.lpVtbl = &unkfactoryVtbl;
    session->refs = 1;

    hr = IUnknown_QueryInterface(&session->session_iface, riid, unk);
    IUnknown_Release(&session->session_iface);
    return hr;
}
