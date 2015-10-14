/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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
#include "webservices.h"

#include "wine/debug.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct
{
    ULONG size;
    BOOL  readonly;
}
writer_props[] =
{
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_DEPTH */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_ALLOW_FRAGMENT */
    { sizeof(ULONG), FALSE },       /* WS_XML_READER_PROPERTY_MAX_ATTRIBUTES */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_WRITE_DECLARATION */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_INDENT */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE */
    { sizeof(WS_CHARSET), FALSE },  /* WS_XML_WRITER_PROPERTY_CHARSET */
    { sizeof(WS_BUFFERS), FALSE },  /* WS_XML_WRITER_PROPERTY_BUFFERS */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE */
    { sizeof(WS_BYTES), FALSE },    /* WS_XML_WRITER_PROPERTY_BYTES */
    { sizeof(BOOL), TRUE },         /* WS_XML_WRITER_PROPERTY_IN_ATTRIBUTE */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE */
    { sizeof(WS_BYTES), FALSE },    /* WS_XML_WRITER_PROPERTY_INITIAL_BUFFER */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_ALLOW_INVALID_CHARACTER_REFERENCES */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_NAMESPACES */
    { sizeof(ULONG), TRUE },        /* WS_XML_WRITER_PROPERTY_BYTES_WRITTEN */
    { sizeof(ULONG), TRUE },        /* WS_XML_WRITER_PROPERTY_BYTES_TO_CLOSE */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_COMPRESS_EMPTY_ELEMENTS */
    { sizeof(BOOL), FALSE }         /* WS_XML_WRITER_PROPERTY_EMIT_UNCOMPRESSED_EMPTY_ELEMENTS */
};

struct writer
{
    ULONG                     prop_count;
    WS_XML_WRITER_PROPERTY    prop[sizeof(writer_props)/sizeof(writer_props[0])];
};

static struct writer *alloc_writer(void)
{
    static const ULONG count = sizeof(writer_props)/sizeof(writer_props[0]);
    struct writer *ret;
    ULONG i, size = sizeof(*ret) + count * sizeof(WS_XML_WRITER_PROPERTY);
    char *ptr;

    for (i = 0; i < count; i++) size += writer_props[i].size;
    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ptr = (char *)&ret->prop[count];
    for (i = 0; i < count; i++)
    {
        ret->prop[i].value = ptr;
        ret->prop[i].valueSize = writer_props[i].size;
        ptr += ret->prop[i].valueSize;
    }
    ret->prop_count = count;
    return ret;
}

static HRESULT set_writer_prop( struct writer *writer, WS_XML_WRITER_PROPERTY_ID id, const void *value,
                                ULONG size )
{
    if (id >= writer->prop_count || size != writer_props[id].size || writer_props[id].readonly)
        return E_INVALIDARG;

    memcpy( writer->prop[id].value, value, size );
    return S_OK;
}

static HRESULT get_writer_prop( struct writer *writer, WS_XML_WRITER_PROPERTY_ID id, void *buf, ULONG size )
{
    if (id >= writer->prop_count || size != writer_props[id].size)
        return E_INVALIDARG;

    memcpy( buf, writer->prop[id].value, writer->prop[id].valueSize );
    return S_OK;
}

/**************************************************************************
 *          WsCreateWriter		[webservices.@]
 */
HRESULT WINAPI WsCreateWriter( const WS_XML_WRITER_PROPERTY *properties, ULONG count,
                               WS_XML_WRITER **handle, WS_ERROR *error )
{
    struct writer *writer;
    ULONG i, max_depth = 32, max_attrs = 128, trim_size = 4096, max_size = 65536, max_ns = 32;
    WS_CHARSET charset = WS_CHARSET_UTF8;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (!(writer = alloc_writer())) return E_OUTOFMEMORY;

    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, sizeof(max_depth) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, sizeof(max_attrs) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE, &trim_size, sizeof(trim_size) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_CHARSET, &charset, sizeof(charset) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE, &max_size, sizeof(max_size) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE, &max_size, sizeof(max_size) );
    set_writer_prop( writer, WS_XML_WRITER_PROPERTY_MAX_NAMESPACES, &max_ns, sizeof(max_ns) );

    for (i = 0; i < count; i++)
    {
        hr = set_writer_prop( writer, properties[i].id, properties[i].value, properties[i].valueSize );
        if (hr != S_OK)
        {
            heap_free( writer );
            return hr;
        }
    }

    *handle = (WS_XML_WRITER *)writer;
    return S_OK;
}

/**************************************************************************
 *          WsFreeWriter		[webservices.@]
 */
void WINAPI WsFreeWriter( WS_XML_WRITER *handle )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p\n", handle );
    heap_free( writer );
}

/**************************************************************************
 *          WsGetWriterProperty		[webservices.@]
 */
HRESULT WINAPI WsGetWriterProperty( WS_XML_WRITER *handle, WS_XML_WRITER_PROPERTY_ID id,
                                    void *buf, ULONG size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    return get_writer_prop( writer, id, buf, size );
}
