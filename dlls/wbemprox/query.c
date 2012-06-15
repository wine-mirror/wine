/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

HRESULT create_view( const struct property *proplist, const WCHAR *class,
                     const struct expr *cond, struct view **ret )
{
    struct view *view = heap_alloc( sizeof(struct view) );

    if (!view) return E_OUTOFMEMORY;
    view->proplist = proplist;
    view->cond     = cond;
    *ret = view;
    return S_OK;
}

void destroy_view( struct view *view )
{
    heap_free( view );
}

static struct query *alloc_query(void)
{
    struct query *query;

    if (!(query = heap_alloc( sizeof(*query) ))) return NULL;
    list_init( &query->mem );
    return query;
}

void free_query( struct query *query )
{
    struct list *mem, *next;

    destroy_view( query->view );
    LIST_FOR_EACH_SAFE( mem, next, &query->mem )
    {
        heap_free( mem );
    }
    heap_free( query );
}

HRESULT exec_query( const WCHAR *str, IEnumWbemClassObject **result )
{
    HRESULT hr;
    struct query *query;

    *result = NULL;
    if (!(query = alloc_query())) return E_OUTOFMEMORY;
    hr = parse_query( str, &query->view, &query->mem );
    if (hr != S_OK)
    {
        free_query( query );
        return hr;
    }
    hr = EnumWbemClassObject_create( NULL, query, (void **)result );
    if (hr != S_OK) free_query( query );
    return hr;
}
