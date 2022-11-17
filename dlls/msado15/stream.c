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
#include "msdasc.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct stream
{
    _Stream            Stream_iface;
    LONG               refs;
    ObjectStateEnum    state;
    ConnectModeEnum    mode;
    StreamTypeEnum     type;
    LineSeparatorEnum  sep;
    WCHAR             *charset;
    LONG               size;
    LONG               allocated;
    LONG               pos;
    BYTE              *buf;
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
        free( stream->charset );
        free( stream->buf );
        free( stream );
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
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI stream_GetTypeInfo( _Stream *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %u, %lu, %p\n", stream, index, lcid, info );
    return get_typeinfo(Stream_tid, info);
}

static HRESULT WINAPI stream_GetIDsOfNames( _Stream *iface, REFIID riid, LPOLESTR *names, UINT count,
                                            LCID lcid, DISPID *dispid )
{
    struct stream *stream = impl_from_Stream( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %lu, %p\n", stream, debugstr_guid(riid), names, count, lcid, dispid );

    hr = get_typeinfo(Stream_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI stream_Invoke( _Stream *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct stream *stream = impl_from_Stream( iface );
    HRESULT hr;
    ITypeInfo *typeinfo;

    TRACE( "%p, %ld, %s, %ld, %d, %p, %p, %p, %p\n", stream, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );

    hr = get_typeinfo(Stream_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &stream->Stream_iface, member, flags, params,
                               result, excep_info, arg_err);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI stream_get_Size( _Stream *iface, ADO_LONGPTR *size )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, size );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    *size = stream->size;
    return S_OK;
}

static HRESULT WINAPI stream_get_EOS( _Stream *iface, VARIANT_BOOL *eos )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, eos );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    *eos = (stream->pos >= stream->size) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI stream_get_Position( _Stream *iface, ADO_LONGPTR *pos )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, pos );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    *pos = stream->pos;
    return S_OK;
}

static HRESULT resize_buffer( struct stream *stream, LONG size )
{
    if (stream->allocated < size)
    {
        BYTE *tmp;
        LONG new_size = max( size, stream->allocated * 2 );
        if (!(tmp = realloc( stream->buf, new_size ))) return E_OUTOFMEMORY;
        memset( tmp + stream->allocated, 0, new_size - stream->allocated );
        stream->buf = tmp;
        stream->allocated = new_size;
    }
    stream->size = size;
    return S_OK;
}

static HRESULT WINAPI stream_put_Position( _Stream *iface, ADO_LONGPTR pos )
{
    struct stream *stream = impl_from_Stream( iface );
    HRESULT hr;

    TRACE( "%p, %Id\n", stream, pos );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (pos < 0) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if ((hr = resize_buffer( stream, stream->pos )) != S_OK) return hr;
    stream->pos = pos;
    return S_OK;
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
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, sep );

    *sep = stream->sep;
    return S_OK;
}

static HRESULT WINAPI stream_put_LineSeparator( _Stream *iface, LineSeparatorEnum sep )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %d\n", stream, sep );

    stream->sep = sep;
    return S_OK;
}

static HRESULT WINAPI stream_get_State( _Stream *iface, ObjectStateEnum *state )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, state );

    *state = stream->state;
    return S_OK;
}

static HRESULT WINAPI stream_get_Mode( _Stream *iface, ConnectModeEnum *mode )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %p\n", stream, mode );

    *mode = stream->mode;
    return S_OK;
}

static HRESULT WINAPI stream_put_Mode( _Stream *iface, ConnectModeEnum mode )
{
    struct stream *stream = impl_from_Stream( iface );
    TRACE( "%p, %u\n", stream, mode );

    stream->mode = mode;
    return S_OK;
}

static HRESULT WINAPI stream_get_Charset( _Stream *iface, BSTR *charset )
{
    struct stream *stream = impl_from_Stream( iface );
    const WCHAR *src = stream->charset ? stream->charset : L"Unicode";
    BSTR dst;

    TRACE( "%p, %p\n", stream, charset );

    if (!(dst = SysAllocString( src ))) return E_OUTOFMEMORY;
    *charset = dst;
    return S_OK;
}

static HRESULT WINAPI stream_put_Charset( _Stream *iface, BSTR charset )
{
    struct stream *stream = impl_from_Stream( iface );
    WCHAR *str;

    TRACE( "%p, %s\n", stream, debugstr_w(charset) );

    if (!(str = wcsdup( charset ))) return E_OUTOFMEMORY;
    free( stream->charset );
    stream->charset = str;
    return S_OK;
}

static HRESULT create_byte_array( BYTE *data, LONG len, VARIANT *ret )
{
    SAFEARRAY *vector;
    LONG i;
    HRESULT hr;

    if (!len)
    {
        V_VT( ret ) = VT_NULL;
        return S_OK;
    }
    if (!(vector = SafeArrayCreateVector( VT_UI1, 0, len ))) return E_OUTOFMEMORY;
    for (i = 0; i < len; i++)
    {
        if ((hr = SafeArrayPutElement( vector, &i, &data[i] )) != S_OK)
        {
            SafeArrayDestroy( vector );
            return hr;
        }
    }

    V_VT( ret ) = VT_ARRAY | VT_UI1;
    V_ARRAY( ret ) = vector;
    return S_OK;
}

static HRESULT WINAPI stream_Read( _Stream *iface, LONG size, VARIANT *val )
{
    struct stream *stream = impl_from_Stream( iface );
    HRESULT hr;

    TRACE( "%p, %ld, %p\n", stream, size, val );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (stream->type != adTypeBinary) return MAKE_ADO_HRESULT( adErrIllegalOperation );
    if (size < adReadAll) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if (size == adReadAll) size = stream->size - stream->pos;
    else size = min( size, stream->size - stream->pos );

    if ((hr = create_byte_array( stream->buf + stream->pos, size, val )) != S_OK) return hr;
    stream->pos += size;
    return S_OK;
}

static HRESULT WINAPI stream_Open( _Stream *iface, VARIANT src, ConnectModeEnum mode, StreamOpenOptionsEnum options,
                                    BSTR username, BSTR password )
{
    struct stream *stream = impl_from_Stream( iface );
    FIXME( "%p, %s, %u, %d, %s, %p\n", stream, debugstr_variant(&src), mode, options, debugstr_w(username), password );

    if (stream->state == adStateOpen) return MAKE_ADO_HRESULT( adErrObjectOpen );

    stream->state = adStateOpen;
    return S_OK;
}

static HRESULT WINAPI stream_Close( _Stream *iface )
{
    struct stream *stream = impl_from_Stream( iface );

    TRACE( "%p\n", stream );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );

    free( stream->buf );
    stream->buf  = NULL;
    stream->size = stream->allocated = stream->pos = 0;

    stream->state = adStateClosed;
    return S_OK;
}

static HRESULT WINAPI stream_SkipLine( _Stream *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Write( _Stream *iface, VARIANT buf )
{
    struct stream *stream = impl_from_Stream( iface );
    LONG bound, i;
    HRESULT hr;

    TRACE( "%p, %s\n", stream, debugstr_variant(&buf) );

    if (stream->state == adStateClosed) return MAKE_ADO_HRESULT( adErrObjectClosed );
    if (stream->type != adTypeBinary) return MAKE_ADO_HRESULT( adErrIllegalOperation );
    if (V_VT( &buf ) != (VT_ARRAY | VT_UI1)) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if ((hr = SafeArrayGetUBound( V_ARRAY( &buf ), 1, &bound )) != S_OK) return hr;
    if ((hr = resize_buffer( stream, stream->size + bound + 1 )) != S_OK) return hr;

    for (i = 0; i <= bound; i++)
    {
        if ((hr = SafeArrayGetElement( V_ARRAY( &buf ), &i, &stream->buf[stream->pos++] )) != S_OK) return hr;
    }

    return S_OK;
}

static HRESULT WINAPI stream_SetEOS( _Stream *iface )
{
    struct stream *stream = impl_from_Stream( iface );

    TRACE( "%p\n", stream );

    return resize_buffer( stream, stream->pos );
}

static HRESULT WINAPI stream_CopyTo( _Stream *iface, _Stream *dst, ADO_LONGPTR size )
{
    FIXME( "%p, %p, %Id\n", iface, dst, size );
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
    struct stream *stream = impl_from_Stream( iface );
    BSTR str;

    TRACE( "%p, %ld, %p\n", stream, len, ret );
    if (len == adReadLine)
    {
        FIXME( "adReadLine not supported\n" );
        return E_NOTIMPL;
    }
    if (stream->charset && wcscmp( stream->charset, L"Unicode" ))
    {
        FIXME( "charset %s not supported\n", debugstr_w(stream->charset) );
        return E_NOTIMPL;
    }

    if (stream->type != adTypeText) return MAKE_ADO_HRESULT( adErrIllegalOperation );
    if (len < adReadLine) return MAKE_ADO_HRESULT( adErrInvalidArgument );

    if (len == adReadAll) len = (stream->size - stream->pos) / sizeof(WCHAR);
    else len = min( len, stream->size - stream->pos / sizeof(WCHAR) );

    if (!(str = SysAllocStringLen( NULL, len ))) return E_OUTOFMEMORY;
    memcpy( str, stream->buf + stream->pos, len * sizeof(WCHAR) );
    str[len] = 0;

    stream->pos += len * sizeof(WCHAR);

    *ret = str;
    return S_OK;
}

static HRESULT WINAPI stream_WriteText( _Stream *iface, BSTR data, StreamWriteEnum options )
{
    struct stream *stream = impl_from_Stream( iface );
    HRESULT hr;
    LONG size;

    TRACE( "%p, %s, %u\n", stream, debugstr_w(data), options );
    if (options != adWriteChar)
    {
        FIXME( "options %u not supported\n", options );
        return E_NOTIMPL;
    }
    if (stream->charset && wcscmp( stream->charset, L"Unicode" ))
    {
        FIXME( "charset %s not supported\n", debugstr_w(stream->charset) );
        return E_NOTIMPL;
    }

    if (stream->type != adTypeText) return MAKE_ADO_HRESULT( adErrIllegalOperation );

    size = (lstrlenW( data ) + 1) * sizeof(WCHAR);
    if ((hr = resize_buffer( stream, stream->size + size )) != S_OK) return hr;

    memcpy( stream->buf + stream->pos, data, size );
    stream->pos += size;

    return S_OK;
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

    if (!(stream = calloc( 1, sizeof(*stream) ))) return E_OUTOFMEMORY;
    stream->Stream_iface.lpVtbl = &stream_vtbl;
    stream->refs = 1;
    stream->type = adTypeText;
    stream->sep  = adCRLF;

    *obj = &stream->Stream_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
