/*
 * Speech API (SAPI) token implementation.
 *
 * Copyright (C) 2017 Huw Davies
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

#include "config.h"
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "sapiddk.h"
#include "sperror.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct data_key
{
    ISpRegDataKey ISpRegDataKey_iface;
    LONG ref;
};

struct data_key *impl_from_ISpRegDataKey( ISpRegDataKey *iface )
{
    return CONTAINING_RECORD( iface, struct data_key, ISpRegDataKey_iface );
}

static HRESULT WINAPI data_key_QueryInterface( ISpRegDataKey *iface, REFIID iid, void **obj )
{
    struct data_key *This = impl_from_ISpRegDataKey( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_ISpDataKey ) ||
        IsEqualIID( iid, &IID_ISpRegDataKey ))
    {
        ISpRegDataKey_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI data_key_AddRef( ISpRegDataKey *iface )
{
    struct data_key *This = impl_from_ISpRegDataKey( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %u\n", This, ref );
    return ref;
}

static ULONG WINAPI data_key_Release( ISpRegDataKey *iface )
{
    struct data_key *This = impl_from_ISpRegDataKey( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %u\n", This, ref );

    if (!ref)
    {
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI data_key_SetData( ISpRegDataKey *iface, LPCWSTR name,
                                        ULONG size, const BYTE *data )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_GetData( ISpRegDataKey *iface, LPCWSTR name,
                                        ULONG *size, BYTE *data )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_SetStringValue( ISpRegDataKey *iface,
                                               LPCWSTR name, LPCWSTR value )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_GetStringValue( ISpRegDataKey *iface,
                                               LPCWSTR name, LPWSTR *value )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_SetDWORD( ISpRegDataKey *iface,
                                         LPCWSTR name, DWORD value )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_GetDWORD( ISpRegDataKey *iface,
                                         LPCWSTR name, DWORD *pdwValue )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_OpenKey( ISpRegDataKey *iface,
                                        LPCWSTR name, ISpDataKey **sub_key )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_CreateKey( ISpRegDataKey *iface,
                                          LPCWSTR name, ISpDataKey **sub_key )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_DeleteKey( ISpRegDataKey *iface, LPCWSTR name )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_DeleteValue( ISpRegDataKey *iface, LPCWSTR name )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_EnumKeys( ISpRegDataKey *iface,
                                         ULONG index, LPWSTR *sub_key )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_EnumValues( ISpRegDataKey *iface,
                                           ULONG index, LPWSTR *value )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_key_SetKey( ISpRegDataKey *iface,
                                       HKEY key, BOOL read_only )
{
    FIXME( "stub\n" );
    return E_NOTIMPL;
}

const struct ISpRegDataKeyVtbl data_key_vtbl =
{
    data_key_QueryInterface,
    data_key_AddRef,
    data_key_Release,
    data_key_SetData,
    data_key_GetData,
    data_key_SetStringValue,
    data_key_GetStringValue,
    data_key_SetDWORD,
    data_key_GetDWORD,
    data_key_OpenKey,
    data_key_CreateKey,
    data_key_DeleteKey,
    data_key_DeleteValue,
    data_key_EnumKeys,
    data_key_EnumValues,
    data_key_SetKey
};

HRESULT data_key_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct data_key *This = heap_alloc( sizeof(*This) );
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpRegDataKey_iface.lpVtbl = &data_key_vtbl;
    This->ref = 1;

    hr = ISpRegDataKey_QueryInterface( &This->ISpRegDataKey_iface, iid, obj );

    ISpRegDataKey_Release( &This->ISpRegDataKey_iface );
    return hr;
}
