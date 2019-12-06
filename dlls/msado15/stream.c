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
#include "objbase.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"
#include "wine/heap.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct stream
{
    _Stream         Stream_iface;
    LONG            refs;
    StreamTypeEnum  type;
};

static inline struct stream *impl_from_Stream( _Stream *iface )
{
    return CONTAINING_RECORD( iface, struct stream, Stream_iface );
}

static ULONG WINAPI stream_AddRef( _Stream *iface )
{
    struct stream *stream = impl_from_Stream( iface );
    return InterlockedIncrement( &stream->refs );
}

static ULONG WINAPI stream_Release( _Stream *iface )
{
    struct stream *stream = impl_from_Stream( iface );
    LONG refs = InterlockedDecrement( &stream->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", stream );
        heap_free( stream );
    }
    return refs;
}

static HRESULT WINAPI stream_QueryInterface( _Stream *iface, REFIID riid, void **obj )
{
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID__Stream ) || IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    stream_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI stream_GetTypeInfoCount( _Stream *iface, UINT *count )
{
    FIXME( "%p, %p\n", iface, count );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_GetTypeInfo( _Stream *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    FIXME( "%p, %u, %u, %p\n", iface, index, lcid, info );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_GetIDsOfNames( _Stream *iface, REFIID riid, LPOLESTR *names, UINT count,
                                            LCID lcid, DISPID *dispid )
{
    FIXME( "%p, %s, %p, %u, %u, %p\n", iface, debugstr_guid(riid), names, count, lcid, dispid );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Invoke( _Stream *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    FIXME( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", iface, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_Size( _Stream *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_EOS( _Stream *iface, VARIANT_BOOL *eos )
{
    FIXME( "%p, %p\n", iface, eos );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_Position( _Stream *iface, LONG *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_put_Position( _Stream *iface, LONG pos )
{
    FIXME( "%p, %d\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_Type( _Stream *iface, StreamTypeEnum *type )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, type );

    *type = stream->type;
    return S_OK;
}

static HRESULT WINAPI stream_put_Type( _Stream *iface, StreamTypeEnum type )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %u\n", stream, type );

    stream->type = type;
    return S_OK;
}

static HRESULT WINAPI stream_get_LineSeparator( _Stream *iface, LineSeparatorEnum *sep )
{
    FIXME( "%p, %p\n", iface, sep );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_put_LineSeparator( _Stream *iface, LineSeparatorEnum sep )
{
    FIXME( "%p, %d\n", iface, sep );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_State( _Stream *iface, ObjectStateEnum *state )
{
    FIXME( "%p, %p\n", iface, state );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_Mode( _Stream *iface, ConnectModeEnum *mode )
{
    FIXME( "%p, %p\n", iface, mode );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_put_Mode( _Stream *iface, ConnectModeEnum mode )
{
    FIXME( "%p, %u\n", iface, mode );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_get_Charset( _Stream *iface, BSTR *charset )
{
    FIXME( "%p, %p\n", iface, charset );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_put_Charset( _Stream *iface, BSTR charset )
{
    FIXME( "%p, %s\n", iface, debugstr_w(charset) );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Read( _Stream *iface, LONG size, VARIANT *val )
{
    FIXME( "%p, %d, %p\n", iface, size, val );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Open( _Stream *iface, VARIANT src, ConnectModeEnum mode, StreamOpenOptionsEnum options,
                                    BSTR username, BSTR password )
{
    FIXME( "%p, %s, %u, %d, %s, %p\n", iface, debugstr_variant(&src), mode, options, debugstr_w(username), password );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Close( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_SkipLine( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Write( _Stream *iface, VARIANT buf )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&buf) );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_SetEOS( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CopyTo( _Stream *iface, _Stream *dst, LONG size )
{
    FIXME( "%p, %p, %d\n", iface, dst, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Flush( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_SaveToFile( _Stream *iface, BSTR filename, SaveOptionsEnum options )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_w(filename), options );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_LoadFromFile( _Stream *iface, BSTR filename )
{
    FIXME( "%p, %s\n", iface, debugstr_w(filename) );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_ReadText( _Stream *iface, LONG len, BSTR *ret )
{
    FIXME( "%p, %d, %p\n", iface, len, ret );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_WriteText( _Stream *iface, BSTR data, StreamWriteEnum options )
{
    FIXME( "%p, %p, %u\n", iface, debugstr_w(data), options );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Cancel( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static const struct _StreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_GetTypeInfoCount,
    stream_GetTypeInfo,
    stream_GetIDsOfNames,
    stream_Invoke,
    stream_get_Size,
    stream_get_EOS,
    stream_get_Position,
    stream_put_Position,
    stream_get_Type,
    stream_put_Type,
    stream_get_LineSeparator,
    stream_put_LineSeparator,
    stream_get_State,
    stream_get_Mode,
    stream_put_Mode,
    stream_get_Charset,
    stream_put_Charset,
    stream_Read,
    stream_Open,
    stream_Close,
    stream_SkipLine,
    stream_Write,
    stream_SetEOS,
    stream_CopyTo,
    stream_Flush,
    stream_SaveToFile,
    stream_LoadFromFile,
    stream_ReadText,
    stream_WriteText,
    stream_Cancel
};

HRESULT Stream_create( void **obj )
{
    struct stream *stream;

    if (!(stream = heap_alloc_zero( sizeof(*stream) ))) return E_OUTOFMEMORY;
    stream->Stream_iface.lpVtbl = &stream_vtbl;
    stream->refs = 1;
    stream->type = adTypeText;

    *obj = &stream->Stream_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
