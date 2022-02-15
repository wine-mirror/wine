/*
 * Copyright 2015, 2016 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc error_props[] =
{
    { sizeof(ULONG), TRUE },    /* WS_ERROR_PROPERTY_STRING_COUNT */
    { sizeof(ULONG), FALSE },   /* WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE */
    { sizeof(LANGID), FALSE }   /* WS_ERROR_PROPERTY_LANGID */
};

struct error
{
    ULONG            magic;
    CRITICAL_SECTION cs;
    ULONG            prop_count;
    struct prop      prop[ARRAY_SIZE( error_props )];
};

#define ERROR_MAGIC (('E' << 24) | ('R' << 16) | ('R' << 8) | 'O')

static struct error *alloc_error(void)
{
    static const ULONG count = ARRAY_SIZE( error_props );
    struct error *ret;
    ULONG size = sizeof(*ret) + prop_size( error_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;

    ret->magic      = ERROR_MAGIC;
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": error.cs");

    prop_init( error_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_error( struct error *error )
{
    error->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &error->cs );
    free( error );
}

/**************************************************************************
 *          WsCreateError		[webservices.@]
 */
HRESULT WINAPI WsCreateError( const WS_ERROR_PROPERTY *properties, ULONG count, WS_ERROR **handle )
{
    struct error *error;
    LANGID langid = GetUserDefaultUILanguage();
    HRESULT hr;
    ULONG i;

    TRACE( "%p %lu %p\n", properties, count, handle );

    if (!handle) return E_INVALIDARG;
    if (!(error = alloc_error())) return E_OUTOFMEMORY;

    prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_LANGID, &langid, sizeof(langid) );
    for (i = 0; i < count; i++)
    {
        if (properties[i].id == WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE)
        {
            free_error( error );
            return E_INVALIDARG;
        }
        hr = prop_set( error->prop, error->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_error( error );
            return hr;
        }
    }

    TRACE( "created %p\n", error );
    *handle = (WS_ERROR *)error;
    return S_OK;
}

static void reset_error( struct error *error )
{
    ULONG code = 0;
    /* FIXME: release strings added with WsAddErrorString when it's implemented, reset string count */
    prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, sizeof(code) );
}

/**************************************************************************
 *          WsFreeError		[webservices.@]
 */
void WINAPI WsFreeError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;

    TRACE( "%p\n", handle );

    if (!error) return;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return;
    }

    reset_error( error );
    error->magic = 0;

    LeaveCriticalSection( &error->cs );
    free_error( error );
}

/**************************************************************************
 *          WsResetError	[webservices.@]
 */
HRESULT WINAPI WsResetError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p\n", handle );

    if (!error) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return E_INVALIDARG;
    }

    reset_error( error );

    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsGetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, void *buf,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu\n", handle, id, buf, size );

    if (!error) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return E_INVALIDARG;
    }

    hr = prop_get( error->prop, error->prop_count, id, buf, size );

    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetErrorString		[webservices.@]
 */
HRESULT WINAPI WsGetErrorString( WS_ERROR *handle, ULONG index, WS_STRING *str )
{
    FIXME( "%p %lu %p: stub\n", handle, index, str );
    return E_NOTIMPL;
}

/**************************************************************************
 *          WsSetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsSetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, const void *value,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu\n", handle, id, value, size );

    if (!error) return E_INVALIDARG;

    EnterCriticalSection( &error->cs );

    if (error->magic != ERROR_MAGIC)
    {
        LeaveCriticalSection( &error->cs );
        return E_INVALIDARG;
    }

    if (id == WS_ERROR_PROPERTY_LANGID) hr = WS_E_INVALID_OPERATION;
    else hr = prop_set( error->prop, error->prop_count, id, value, size );

    LeaveCriticalSection( &error->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}
