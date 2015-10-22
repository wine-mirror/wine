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

#include <stdio.h>
#include "windows.h"
#include "webservices.h"
#include "wine/test.h"

static HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING encoding;
    WS_XML_WRITER_BUFFER_OUTPUT output;

    encoding.encoding.encodingType = WS_XML_WRITER_ENCODING_TYPE_TEXT;
    encoding.charSet               = WS_CHARSET_UTF8;

    output.output.outputType = WS_XML_WRITER_OUTPUT_TYPE_BUFFER;

    return WsSetOutput( writer, &encoding.encoding, &output.output, NULL, 0, NULL );
}

static void test_WsCreateWriter(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_WRITER_PROPERTY prop;
    ULONG size, max_depth, max_attrs, indent, trim_size, max_size, max_ns;
    BOOL allow_fragment, write_decl, in_attr;
    WS_CHARSET charset;
    WS_BUFFERS buffers;
    WS_BYTES bytes;

    hr = WsCreateWriter( NULL, 0, NULL, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    writer = NULL;
    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );
    ok( writer != NULL, "writer not set\n" );

    /* can't retrieve properties before output is set */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    ok( max_depth == 0xdeadbeef, "max_depth set\n" );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    /* check some defaults */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 32, "got %u\n", max_depth );

    allow_fragment = TRUE;
    size = sizeof(allow_fragment);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_ALLOW_FRAGMENT, &allow_fragment, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !allow_fragment, "got %d\n", allow_fragment );

    max_attrs = 0xdeadbeef;
    size = sizeof(max_attrs);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_attrs == 128, "got %u\n", max_attrs );

    write_decl = TRUE;
    size = sizeof(write_decl);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_WRITE_DECLARATION, &write_decl, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !write_decl, "got %d\n", write_decl );

    indent = 0xdeadbeef;
    size = sizeof(indent);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_INDENT, &indent, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !indent, "got %u\n", indent );

    trim_size = 0xdeadbeef;
    size = sizeof(trim_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE, &trim_size, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( trim_size == 4096, "got %u\n", trim_size );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    buffers.bufferCount = 0xdeadbeef;
    buffers.buffers = (WS_BYTES *)0xdeadbeef;
    size = sizeof(buffers);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFERS, &buffers, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !buffers.bufferCount, "got %u\n", buffers.bufferCount );
    ok( !buffers.buffers, "got %p\n", buffers.buffers );

    max_size = 0xdeadbeef;
    size = sizeof(max_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE, &max_size, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_size == 65536, "got %u\n", max_size );

    buffers.bufferCount = 0xdeadbeef;
    buffers.buffers = (WS_BYTES *)0xdeadbeef;
    size = sizeof(buffers);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &buffers, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !buffers.bufferCount, "got %u\n", buffers.bufferCount );
    todo_wine ok( buffers.buffers != NULL, "got %p\n", buffers.buffers );

    max_size = 0xdeadbeef;
    size = sizeof(max_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE, &max_size, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_size == 65536, "got %u\n", max_size );

    bytes.length = 0xdeadbeef;
    bytes.bytes = (BYTE *)0xdeadbeef;
    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_INITIAL_BUFFER, &bytes, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !bytes.length, "got %u\n", bytes.length );
    ok( !bytes.bytes, "got %p\n", bytes.bytes );

    max_ns = 0xdeadbeef;
    size = sizeof(max_ns);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_NAMESPACES, &max_ns, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_ns == 32, "got %u\n", max_ns );
    WsFreeWriter( writer );

    /* change a property */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsCreateWriter( &prop, 1, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );
    WsFreeWriter( writer );

    /* show that some properties are read-only */
    in_attr = TRUE;
    prop.id = WS_XML_WRITER_PROPERTY_IN_ATTRIBUTE;
    prop.value = &in_attr;
    prop.valueSize = sizeof(in_attr);
    hr = WsCreateWriter( &prop, 1, &writer, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    size = 1;
    prop.id = WS_XML_WRITER_PROPERTY_BYTES_WRITTEN;
    prop.value = &size;
    prop.valueSize = sizeof(size);
    hr = WsCreateWriter( &prop, 1, &writer, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    size = 1;
    prop.id = WS_XML_WRITER_PROPERTY_BYTES_TO_CLOSE;
    prop.value = &size;
    prop.valueSize = sizeof(size);
    hr = WsCreateWriter( &prop, 1, &writer, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
}

static void test_WsCreateXmlBuffer(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    WS_BYTES bytes;
    ULONG size;

    hr = WsCreateXmlBuffer( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( NULL, NULL, 0, &buffer, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    buffer = NULL;
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( buffer != NULL, "buffer not set\n" );

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_WsSetOutput(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_WRITER_PROPERTY prop;
    WS_XML_WRITER_TEXT_ENCODING encoding;
    WS_XML_WRITER_BUFFER_OUTPUT output;
    ULONG size, max_depth;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutput( NULL, NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    encoding.encoding.encodingType = WS_XML_WRITER_ENCODING_TYPE_TEXT;
    encoding.charSet               = WS_CHARSET_UTF8;

    output.output.outputType = WS_XML_WRITER_OUTPUT_TYPE_BUFFER;

    hr = WsSetOutput( writer, &encoding.encoding, &output.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* multiple calls are allowed */
    hr = WsSetOutput( writer, &encoding.encoding, &output.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* writer properties can be set with WsSetOutput */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetOutput( writer, &encoding.encoding, &output.output, &prop, 1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );
    WsFreeWriter( writer );
}

static void test_WsSetOutputToBuffer(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_BUFFER *buffer;
    WS_XML_WRITER *writer;
    WS_XML_WRITER_PROPERTY prop;
    ULONG size, max_depth;

    hr = WsSetOutputToBuffer( NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* multiple calls are allowed */
    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* writer properties can be set with WsSetOutputToBuffer */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetOutputToBuffer( writer, buffer, &prop, 1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

START_TEST(writer)
{
    test_WsCreateWriter();
    test_WsCreateXmlBuffer();
    test_WsSetOutput();
    test_WsSetOutputToBuffer();
}
