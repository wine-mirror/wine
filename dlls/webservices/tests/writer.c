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
#include <math.h>
#include <float.h>
#include "windows.h"
#include "rpc.h"
#include "webservices.h"
#include "wine/test.h"

static HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING text = { {WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8 };
    WS_XML_WRITER_BUFFER_OUTPUT buf = { {WS_XML_WRITER_OUTPUT_TYPE_BUFFER} };
    return WsSetOutput( writer, &text.encoding, &buf.output, NULL, 0, NULL );
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

    hr = WsCreateWriter( NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    writer = NULL;
    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( writer != NULL, "writer not set\n" );

    /* can't retrieve properties before output is set */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    ok( max_depth == 0xdeadbeef, "max_depth set\n" );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* check some defaults */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 32, "got %lu\n", max_depth );

    allow_fragment = TRUE;
    size = sizeof(allow_fragment);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_ALLOW_FRAGMENT, &allow_fragment, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !allow_fragment, "got %d\n", allow_fragment );

    max_attrs = 0xdeadbeef;
    size = sizeof(max_attrs);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_attrs == 128, "got %lu\n", max_attrs );

    write_decl = TRUE;
    size = sizeof(write_decl);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_WRITE_DECLARATION, &write_decl, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !write_decl, "got %d\n", write_decl );

    indent = 0xdeadbeef;
    size = sizeof(indent);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_INDENT, &indent, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !indent, "got %lu\n", indent );

    trim_size = 0xdeadbeef;
    size = sizeof(trim_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE, &trim_size, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( trim_size == 4096, "got %lu\n", trim_size );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    buffers.bufferCount = 0xdeadbeef;
    buffers.buffers = (WS_BYTES *)0xdeadbeef;
    size = sizeof(buffers);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFERS, &buffers, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !buffers.bufferCount, "got %lu\n", buffers.bufferCount );
    ok( !buffers.buffers, "got %p\n", buffers.buffers );

    max_size = 0xdeadbeef;
    size = sizeof(max_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE, &max_size, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_size == 65536, "got %lu\n", max_size );

    bytes.length = 0xdeadbeef;
    bytes.bytes = (BYTE *)0xdeadbeef;
    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !bytes.length, "got %lu\n", bytes.length );
    ok( bytes.bytes != NULL, "got %p\n", bytes.bytes );

    max_size = 0xdeadbeef;
    size = sizeof(max_size);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE, &max_size, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_size == 65536, "got %lu\n", max_size );

    bytes.length = 0xdeadbeef;
    bytes.bytes = (BYTE *)0xdeadbeef;
    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_INITIAL_BUFFER, &bytes, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !bytes.length, "got %lu\n", bytes.length );
    ok( !bytes.bytes, "got %p\n", bytes.bytes );

    max_ns = 0xdeadbeef;
    size = sizeof(max_ns);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_NAMESPACES, &max_ns, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_ns == 32, "got %lu\n", max_ns );
    WsFreeWriter( writer );

    /* change a property */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsCreateWriter( &prop, 1, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );
    WsFreeWriter( writer );

    /* show that some properties are read-only */
    in_attr = TRUE;
    prop.id = WS_XML_WRITER_PROPERTY_IN_ATTRIBUTE;
    prop.value = &in_attr;
    prop.valueSize = sizeof(in_attr);
    hr = WsCreateWriter( &prop, 1, &writer, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    size = 1;
    prop.id = WS_XML_WRITER_PROPERTY_BYTES_WRITTEN;
    prop.value = &size;
    prop.valueSize = sizeof(size);
    hr = WsCreateWriter( &prop, 1, &writer, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    size = 1;
    prop.id = WS_XML_WRITER_PROPERTY_BYTES_TO_CLOSE;
    prop.value = &size;
    prop.valueSize = sizeof(size);
    hr = WsCreateWriter( &prop, 1, &writer, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
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
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( NULL, NULL, 0, &buffer, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    buffer = NULL;
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( buffer != NULL, "buffer not set\n" );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    size = sizeof(bytes);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %#lx\n", hr );

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

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutput( NULL, NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    encoding.encoding.encodingType = WS_XML_WRITER_ENCODING_TYPE_TEXT;
    encoding.charSet               = WS_CHARSET_UTF8;

    output.output.outputType = WS_XML_WRITER_OUTPUT_TYPE_BUFFER;

    hr = WsSetOutput( writer, &encoding.encoding, &output.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* multiple calls are allowed */
    hr = WsSetOutput( writer, &encoding.encoding, &output.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* writer properties can be set with WsSetOutput */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetOutput( writer, &encoding.encoding, &output.output, &prop, 1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );
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
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* multiple calls are allowed */
    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* writer properties can be set with WsSetOutputToBuffer */
    max_depth = 16;
    prop.id = WS_XML_WRITER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetOutputToBuffer( writer, buffer, &prop, 1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static char strbuf[512];
static const char *debugstr_bytes( const BYTE *bytes, ULONG len )
{
    const BYTE *src = bytes;
    char *dst = strbuf;

    while (len)
    {
        BYTE c = *src++;
        if (dst - strbuf > sizeof(strbuf) - 7) break;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        default:
            if (c >= ' ' && c < 127) *dst++ = c;
            else
            {
                sprintf( dst, "\\%02x", c );
                dst += 3;
            }
        }
        len--;
    }
    if (len)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst = 0;
    return strbuf;
}

static void check_output( WS_XML_WRITER *writer, const char *expected, unsigned int line )
{
    WS_BYTES bytes;
    ULONG size = sizeof(bytes);
    int len = strlen( expected );
    HRESULT hr;

    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "%u: got %#lx\n", line, hr );
    ok( bytes.length == len, "%u: got %lu expected %d\n", line, bytes.length, len );
    if (bytes.length != len) return;
    ok( !memcmp( bytes.bytes, expected, len ),
        "%u: got %s expected %s\n", line, debugstr_bytes(bytes.bytes, bytes.length), expected );
}

static void test_WsWriteStartElement(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, ns = {2, (BYTE *)"ns"}, ns2 = {3, (BYTE *)"ns2"};
    WS_XML_STRING localname = {1, (BYTE *)"a"}, localname2 =  {1, (BYTE *)"b"}, empty = {0, NULL};

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( NULL, &prefix, &localname, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* first call to WsWriteStartElement doesn't output anything */
    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    /* two ways to close an element */
    hr = WsWriteEndStartElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\">", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"></p:a>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"/>", __LINE__ );

    /* nested elements */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\">", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"><p:b/>", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"><p:b/></p:a>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\">", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"><b xmlns=\"ns2\"/>", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"><b xmlns=\"ns2\"/></p:a>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &empty, &localname, &empty, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndStartElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<a></a>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteStartAttribute(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, localname = {3, (BYTE *)"str"};
    WS_XML_STRING localname2 = {3, (BYTE *)"len"}, ns = {2, (BYTE *)"ns"}, empty = {0, NULL};
    WS_XML_UTF8_TEXT text = {{WS_XML_TEXT_TYPE_UTF8}};

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( NULL, &prefix, &localname, &ns, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* WsWriteStartAttribute doesn't output anything */
    hr = WsWriteStartAttribute( writer, &prefix, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    text.value.length  = 1;
    text.value.bytes   = (BYTE *)"0";
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    /* WsWriteEndAttribute doesn't output anything */
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str p:len=\"0\" xmlns:p=\"ns\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, &empty, &localname2, &empty, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str len=\"\" xmlns:p=\"ns\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &empty, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str len=\"\" p:str=\"\" xmlns:p=\"ns\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteType(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE*)"p"}, localname = {3, (BYTE *)"str"}, ns = {2, (BYTE *)"ns"};
    const WCHAR *val_str;
    enum {ONE = 1, TWO = 2};
    WS_XML_STRING one = {3, (BYTE *)"ONE" }, two = {3, (BYTE *)"TWO"};
    WS_ENUM_VALUE enum_values[] = {{ONE, &one}, {TWO, &two}};
    WS_ENUM_DESCRIPTION enum_desc;
    int val_enum;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    val_str = L"test";
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* required value */
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, NULL, sizeof(L"test"), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, L"test", sizeof(L"test"), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* required pointer */
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, NULL, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, L"test", sizeof(L"test"), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(WCHAR **), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test</p:str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    val_str = L"test";
    hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str p:str=\"test\" xmlns:p=\"ns\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    val_str = L"test";
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test</p:str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    enum_desc.values       = enum_values;
    enum_desc.valueCount   = ARRAY_SIZE(enum_values);
    enum_desc.maxByteCount = 3;
    enum_desc.nameIndices  = NULL;

    val_enum = 0;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                      WS_WRITE_REQUIRED_VALUE, &val_enum, sizeof(val_enum), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    val_enum = 3;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                      WS_WRITE_REQUIRED_VALUE, &val_enum, sizeof(val_enum), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    val_enum = ONE;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                      WS_WRITE_REQUIRED_VALUE, &val_enum, sizeof(val_enum), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">ONE</p:str>", __LINE__ );

    WsFreeWriter( writer );
}

static void prepare_basic_type_test( WS_XML_WRITER *writer )
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    HRESULT hr;

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_basic_type(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL}, xmlstr;
    GUID guid;
    const WCHAR *str;
    WS_STRING string;
    WS_BYTES bytes;
    WS_UNIQUE_ID id;
    ULONG i;
    static const struct
    {
        WS_TYPE     type;
        INT64       val;
        ULONG       size;
        const char *result;
        const char *result2;
    }
    tests[] =
    {
        { WS_BOOL_TYPE, TRUE, sizeof(BOOL), "<t>true</t>", "<t t=\"true\"/>" },
        { WS_BOOL_TYPE, FALSE, sizeof(BOOL), "<t>false</t>", "<t t=\"false\"/>" },
        { WS_INT8_TYPE, -128, sizeof(INT8), "<t>-128</t>", "<t t=\"-128\"/>" },
        { WS_INT16_TYPE, -32768, sizeof(INT16), "<t>-32768</t>", "<t t=\"-32768\"/>" },
        { WS_INT32_TYPE, -2147483647 - 1, sizeof(INT32), "<t>-2147483648</t>",
          "<t t=\"-2147483648\"/>" },
        { WS_INT64_TYPE, -9223372036854775807 - 1, sizeof(INT64), "<t>-9223372036854775808</t>",
          "<t t=\"-9223372036854775808\"/>" },
        { WS_UINT8_TYPE, 255, sizeof(UINT8), "<t>255</t>", "<t t=\"255\"/>" },
        { WS_UINT16_TYPE, 65535, sizeof(UINT16), "<t>65535</t>", "<t t=\"65535\"/>" },
        { WS_UINT32_TYPE, ~0u, sizeof(UINT32), "<t>4294967295</t>", "<t t=\"4294967295\"/>" },
        { WS_UINT64_TYPE, ~0, sizeof(UINT64), "<t>18446744073709551615</t>",
          "<t t=\"18446744073709551615\"/>" },
    };

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element content type mapping */
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        prepare_basic_type_test( writer );
        hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_VALUE, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* element type mapping is the same as element content type mapping for basic types */
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        const INT64 *ptr = &tests[i].val;

        prepare_basic_type_test( writer );
        hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_POINTER, &ptr, sizeof(ptr), NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* attribute type mapping */
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        prepare_basic_type_test( writer );
        hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_VALUE, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        check_output( writer, tests[i].result2, __LINE__ );
    }

    prepare_basic_type_test( writer );
    memset( &guid, 0, sizeof(guid) );
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_GUID_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &guid, sizeof(guid), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>00000000-0000-0000-0000-000000000000</t>", __LINE__ );

    prepare_basic_type_test( writer );
    string.chars  = (WCHAR *)L"test";
    string.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRING_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &string, sizeof(string), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test</t>", __LINE__ );

    prepare_basic_type_test( writer );
    str = L"test";
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL, WS_WRITE_REQUIRED_POINTER,
                      &str, sizeof(str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test</t>", __LINE__ );

    prepare_basic_type_test( writer );
    xmlstr.bytes  = (BYTE *)"test";
    xmlstr.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_XML_STRING_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &xmlstr, sizeof(xmlstr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test</t>", __LINE__ );

    prepare_basic_type_test( writer );
    bytes.bytes  = (BYTE *)"test";
    bytes.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &bytes, sizeof(bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>dGVzdA==</t>", __LINE__ );

    prepare_basic_type_test( writer );
    bytes.length = 0;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &bytes, sizeof(bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t/>", __LINE__ );

    prepare_basic_type_test( writer );
    bytes.bytes  = NULL;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &bytes, sizeof(bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t/>", __LINE__ );

    prepare_basic_type_test( writer );
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL, WS_WRITE_NILLABLE_VALUE,
                      &bytes, sizeof(bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/>",
                  __LINE__ );

    prepare_basic_type_test( writer );
    memset( &id, 0, sizeof(id) );
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &id, sizeof(id), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>urn:uuid:00000000-0000-0000-0000-000000000000</t>", __LINE__ );

    prepare_basic_type_test( writer );
    id.uri.length = 4;
    id.uri.chars  = (WCHAR *)L"test";
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                      &id, sizeof(id), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test</t>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_simple_struct_type(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_XML_STRING localname = {6, (BYTE *)"struct"}, ns = {0, NULL};
    struct test
    {
        const WCHAR *field;
    } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;

    test = malloc( sizeof(*test) );
    test->field  = L"value";
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, test, sizeof(*test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<struct>value</struct>", __LINE__ );

    /* required value */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, test, sizeof(*test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<struct>value</struct>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<struct>value</struct>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<struct struct=\"value\"/>", __LINE__ );

    free( test );
    WsFreeWriter( writer );
}

static void test_WsWriteElement(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_ELEMENT_DESCRIPTION desc;
    WS_XML_STRING localname = {3, (BYTE *)"str"}, ns = {0, NULL};
    struct test { const WCHAR *str; } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* text field mapping */
    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TEXT_FIELD_MAPPING;
    f.type    = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 1;

    desc.elementLocalName = &localname;
    desc.elementNs        = &ns;
    desc.type             = WS_STRUCT_TYPE;
    desc.typeDescription  = &s;

    test = malloc( sizeof(*test) );
    test->str = L"test";
    hr = WsWriteElement( NULL, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteElement( writer, NULL, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<str>test</str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<str><str>test</str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute field mapping */
    f.mapping = WS_ATTRIBUTE_FIELD_MAPPING;

    /* requires localName and ns to be set */
    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    f.localName = &localname;
    f.ns        = &ns;
    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<str str=\"test\"/>", __LINE__ );

    free( test );
    WsFreeWriter( writer );
}

static void test_WsWriteValue(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    ULONG i;
    static const struct
    {
        WS_VALUE_TYPE type;
        INT64         val;
        ULONG         size;
        const char   *result;
        const char   *result2;
    }
    tests[] =
    {
        { WS_BOOL_VALUE_TYPE, ~0, sizeof(BOOL), "<t>true</t>", "<t t=\"true\"/>" },
        { WS_BOOL_VALUE_TYPE, FALSE, sizeof(BOOL), "<t>false</t>", "<t t=\"false\"/>" },
        { WS_INT8_VALUE_TYPE, -128, sizeof(INT8), "<t>-128</t>", "<t t=\"-128\"/>" },
        { WS_INT16_VALUE_TYPE, -32768, sizeof(INT16), "<t>-32768</t>", "<t t=\"-32768\"/>" },
        { WS_INT32_VALUE_TYPE, -2147483647 - 1, sizeof(INT32), "<t>-2147483648</t>",
          "<t t=\"-2147483648\"/>" },
        { WS_INT64_VALUE_TYPE, -9223372036854775807 - 1, sizeof(INT64), "<t>-9223372036854775808</t>",
          "<t t=\"-9223372036854775808\"/>" },
        { WS_UINT8_VALUE_TYPE, 255, sizeof(UINT8), "<t>255</t>", "<t t=\"255\"/>" },
        { WS_UINT16_VALUE_TYPE, 65535, sizeof(UINT16), "<t>65535</t>", "<t t=\"65535\"/>" },
        { WS_UINT32_VALUE_TYPE, ~0u, sizeof(UINT32), "<t>4294967295</t>", "<t t=\"4294967295\"/>" },
        { WS_UINT64_VALUE_TYPE, ~0, sizeof(UINT64), "<t>18446744073709551615</t>",
          "<t t=\"18446744073709551615\"/>" },
    };

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteValue( NULL, tests[0].type, &tests[0].val, tests[0].size, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteValue( writer, tests[0].type, &tests[0].val, tests[0].size, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* zero size */
    hr = WsWriteValue( writer, tests[0].type, &tests[0].val, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* NULL value */
    hr = WsWriteValue( writer, tests[0].type, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* element type mapping */
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteValue( writer, tests[i].type, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* attribute type mapping */
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteValue( writer, tests[i].type, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        check_output( writer, tests[i].result2, __LINE__ );
    }

    WsFreeWriter( writer );
}

static void test_WsWriteAttribute(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_ATTRIBUTE_DESCRIPTION desc;
    WS_XML_STRING localname = {3, (BYTE *)"str"}, ns = {0, NULL};
    struct test { const WCHAR *str; } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* text field mapping */
    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TEXT_FIELD_MAPPING;
    f.type    = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 1;

    desc.attributeLocalName = &localname;
    desc.attributeNs        = &ns;
    desc.type               = WS_STRUCT_TYPE;
    desc.typeDescription    = &s;

    test = malloc( sizeof(*test) );
    test->str = L"test";
    hr = WsWriteAttribute( NULL, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteAttribute( writer, NULL, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<str str=\"test\"/>", __LINE__ );

    free( test );
    WsFreeWriter( writer );
}

static void test_WsWriteStartCData(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_XML_UTF8_TEXT text = {{WS_XML_TEXT_TYPE_UTF8}};

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndCData( writer, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteStartCData( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><![CDATA[", __LINE__ );

    text.value.bytes = (BYTE *)"<data>";
    text.value.length = 6;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><![CDATA[<data>", __LINE__ );

    hr = WsWriteEndCData( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><![CDATA[<data>]]>", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><![CDATA[<data>]]></t>", __LINE__ );

    WsFreeWriter( writer );
}

static void check_output_buffer( WS_XML_BUFFER *buffer, const char *expected, unsigned int line )
{
    WS_XML_WRITER *writer;
    WS_BYTES bytes;
    ULONG size = sizeof(bytes);
    int len = strlen(expected);
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteXmlBuffer( writer, buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "%u: got %#lx\n", line, hr );
    ok( bytes.length == len, "%u: got %lu expected %d\n", line, bytes.length, len );
    if (bytes.length != len) return;
    ok( !memcmp( bytes.bytes, expected, len ), "%u: got %s expected %s\n", line, bytes.bytes, expected );

    WsFreeWriter( writer );
}

static void prepare_xmlns_test( WS_XML_WRITER *writer, WS_HEAP **heap, WS_XML_BUFFER **buffer )
{
    WS_XML_STRING prefix = {6, (BYTE *)"prefix"}, localname = {1, (BYTE *)"t"}, ns = {2, (BYTE *)"ns"};
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( *heap, NULL, 0, buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, *buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_WsWriteXmlnsAttribute(void)
{
    WS_XML_STRING ns = {2, (BYTE *)"ns"}, ns2 = {3, (BYTE *)"ns2"};
    WS_XML_STRING prefix = {6, (BYTE *)"prefix"}, prefix2 = {7, (BYTE *)"prefix2"};
    WS_XML_STRING xmlns = {6, (BYTE *)"xmlns"}, attr = {4, (BYTE *)"attr"};
    WS_XML_STRING localname = {1, (BYTE *)"u"};
    WS_HEAP *heap;
    WS_XML_BUFFER *buffer;
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteXmlnsAttribute( NULL, NULL, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeHeap( heap );

    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, NULL, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeHeap( heap );

    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteXmlnsAttribute( writer, NULL, &ns, FALSE, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    WsFreeHeap( heap );

    /* no prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, NULL, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix=\"ns\" xmlns=\"ns2\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2=\"ns2\" xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* implicitly set element prefix namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* explicitly set element prefix namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix='ns'/>", __LINE__ );
    WsFreeHeap( heap );

    /* repeated calls, same namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2=\"ns\" xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* repeated calls, different namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    WsFreeHeap( heap );

    /* single quotes */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2='ns' xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* different namespace, different prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2='ns2' xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* different namespace, same prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    WsFreeHeap( heap );

    /* regular attribute */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteStartAttribute( writer, &xmlns, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    WsFreeHeap( heap );

    /* attribute order */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartAttribute( writer, &prefix, &attr, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t prefix:attr='' xmlns:prefix='ns' xmlns:prefix2='ns2'/>", __LINE__ );
    WsFreeHeap( heap );

    /* scope */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &prefix2, &localname, &ns2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2='ns2' xmlns:prefix=\"ns\"><prefix2:u/></prefix:t>",
                        __LINE__ );
    WsFreeHeap( heap );

    WsFreeWriter( writer );
}

static void prepare_prefix_test( WS_XML_WRITER *writer )
{
    const WS_XML_STRING p = {1, (BYTE *)"p"}, localname = {1, (BYTE *)"t"}, ns = {2, (BYTE *)"ns"};
    HRESULT hr;

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndStartElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_WsGetPrefixFromNamespace(void)
{
    const WS_XML_STRING p = {1, (BYTE *)"p"}, localname = {1, (BYTE *)"t"}, *prefix;
    const WS_XML_STRING ns = {2, (BYTE *)"ns"}, ns2 = {3, (BYTE *)"ns2"};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetPrefixFromNamespace( NULL, NULL, FALSE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetPrefixFromNamespace( NULL, NULL, FALSE, &prefix, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetPrefixFromNamespace( writer, NULL, FALSE, &prefix, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* element must be committed */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    /* but writer can't be positioned on end element node */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    /* required = TRUE */
    prefix = NULL;
    prepare_prefix_test( writer );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( prefix != NULL, "prefix not set\n" );
    if (prefix)
    {
        ok( prefix->length == 1, "got %lu\n", prefix->length );
        ok( !memcmp( prefix->bytes, "p", 1 ), "wrong prefix\n" );
    }

    prefix = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetPrefixFromNamespace( writer, &ns2, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( prefix == (const WS_XML_STRING *)0xdeadbeef, "prefix set\n" );

    /* required = FALSE */
    prefix = NULL;
    prepare_prefix_test( writer );
    hr = WsGetPrefixFromNamespace( writer, &ns, FALSE, &prefix, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( prefix != NULL, "prefix not set\n" );
    if (prefix)
    {
        ok( prefix->length == 1, "got %lu\n", prefix->length );
        ok( !memcmp( prefix->bytes, "p", 1 ), "wrong prefix\n" );
    }

    prefix = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetPrefixFromNamespace( writer, &ns2, FALSE, &prefix, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );
    ok( prefix == NULL, "prefix not set\n" );

    WsFreeWriter( writer );
}

static void test_complex_struct_type(void)
{
    static const char expected[] =
        "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">"
        "<o:services o:GenerationTime=\"2015-09-03T18:47:54\"/>"
        "</o:OfficeConfig>";
    WS_XML_STRING str_officeconfig = {12, (BYTE *)"OfficeConfig"};
    WS_XML_STRING str_services = {8, (BYTE *)"services"};
    WS_XML_STRING str_generationtime = {14, (BYTE *)"GenerationTime"};
    WS_XML_STRING ns = {39, (BYTE *)"urn:schemas-microsoft-com:office:office"};
    WS_XML_STRING prefix = {1, (BYTE *)"o"};
    DWORD size;
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_BUFFER *buffer;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f2, f3, *fields[1], *fields2[2];
    struct services
    {
        const WCHAR *generationtime;
        BYTE         dummy[12];
    };
    struct officeconfig
    {
        struct services *services;
    } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &str_officeconfig, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping         = WS_ATTRIBUTE_FIELD_MAPPING;
    f2.localName       = &str_generationtime;
    f2.ns              = &ns;
    f2.type            = WS_WSZ_TYPE;
    f2.options         = WS_FIELD_OPTIONAL;
    fields2[0] = &f2;

    memset( &f3, 0, sizeof(f3) );
    f3.mapping  = WS_ANY_ATTRIBUTES_FIELD_MAPPING;
    f3.type     = WS_ANY_ATTRIBUTES_TYPE;
    f3.offset   = FIELD_OFFSET(struct services, dummy);
    fields2[1] = &f3;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(*test->services);
    s2.alignment     = 4;
    s2.fields        = fields2;
    s2.fieldCount    = 2;
    s2.typeLocalName = &str_services;
    s2.typeNs        = &ns;

    memset( &f, 0, sizeof(f) );
    f.mapping         = WS_ELEMENT_FIELD_MAPPING;
    f.localName       = &str_services;
    f.ns              = &ns;
    f.type            = WS_STRUCT_TYPE;
    f.typeDescription = &s2;
    f.options         = WS_FIELD_POINTER;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(*test);
    s.alignment     = 4;
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_officeconfig;
    s.typeNs        = &ns;

    size = sizeof(struct officeconfig) + sizeof(struct services);
    test = calloc( 1, size );
    test->services = (struct services *)(test + 1);
    test->services->generationtime = L"2015-09-03T18:47:54";
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, expected, __LINE__ );

    free( test );
    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_WsMoveWriter(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"a"}, localname2 = {1, (BYTE *)"b"}, ns = {0, NULL};
    WS_HEAP *heap;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    HRESULT hr;

    hr = WsMoveWriter( NULL, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* writer must be set to an XML buffer */
    hr = WsMoveWriter( writer, WS_MOVE_TO_EOF, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* <a><b/></a> */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_CHILD_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_END_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_END_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveWriter( writer, WS_MOVE_TO_BOF, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_WsGetWriterPosition(void)
{
    WS_HEAP *heap;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    WS_XML_NODE_POSITION pos;
    HRESULT hr;

    hr = WsGetWriterPosition( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetWriterPosition( writer, &pos, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* writer must be set to an XML buffer */
    hr = WsGetWriterPosition( writer, &pos, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetWriterPosition( writer, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos.buffer != NULL, "buffer not set\n" );
    ok( pos.node != NULL, "node not set\n" );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_WsSetWriterPosition(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_HEAP *heap;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buf1, *buf2;
    WS_XML_NODE_POSITION pos;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetWriterPosition( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buf1, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetWriterPosition( writer, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos.buffer == buf1, "wrong buffer\n" );
    ok( pos.node != NULL, "node not set\n" );

    hr = WsSetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* different buffer */
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    pos.buffer = buf2;
    hr = WsSetWriterPosition( writer, &pos, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buf1, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* try to write at non-final position */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos.buffer == buf1, "wrong buffer\n" );
    ok( pos.node != NULL, "node not set\n" );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buf1, "<t/>", __LINE__ );

    hr = WsSetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_WsWriteXmlBuffer(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_XML_WRITER *writer1, *writer2;
    WS_XML_BUFFER *buffer1, *buffer2;
    WS_HEAP *heap;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer1, buffer1, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer1, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer1, "<t/>", __LINE__ );

    hr = WsCreateWriter( NULL, 0, &writer2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer2, buffer2, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteXmlBuffer( writer2, buffer1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer2, "<t/>", __LINE__ );

    hr = WsMoveWriter( writer2, WS_MOVE_TO_PREVIOUS_ELEMENT, NULL, NULL );
    todo_wine ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteXmlBuffer( writer2, buffer1, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeWriter( writer1 );
    WsFreeWriter( writer2 );
    WsFreeHeap( heap );
}

static void test_WsWriteNode(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {4, (BYTE *)"attr"}, ns = {0, NULL};
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}};
    WS_XML_ATTRIBUTE attr, *attrs[1];
    WS_XML_ELEMENT_NODE elem;
    WS_XML_COMMENT_NODE comment = {{WS_XML_NODE_TYPE_COMMENT}};
    WS_XML_NODE node;
    WS_XML_TEXT_NODE text = {{WS_XML_NODE_TYPE_TEXT}};
    WS_HEAP *heap;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteNode( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteNode( writer, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf8.value.bytes   = (BYTE *)"value";
    utf8.value.length  = sizeof("value") - 1;

    attr.singleQuote = TRUE;
    attr.isXmlNs     = FALSE;
    attr.prefix      = NULL;
    attr.localName   = &localname2;
    attr.ns          = &ns;
    attr.value       = &utf8.text;
    attrs[0] = &attr;

    elem.node.nodeType  = WS_XML_NODE_TYPE_ELEMENT;
    elem.prefix         = NULL;
    elem.localName      = &localname;
    elem.ns             = &ns;
    elem.attributeCount = 1;
    elem.attributes     = attrs;
    elem.isEmpty        = FALSE;
    hr = WsWriteNode( writer, &elem.node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    comment.value.bytes   = (BYTE *)"comment";
    comment.value.length  = sizeof("comment") - 1;
    hr = WsWriteNode( writer, &comment.node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node.nodeType = WS_XML_NODE_TYPE_EOF;
    hr = WsWriteNode( writer, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node.nodeType = WS_XML_NODE_TYPE_BOF;
    hr = WsWriteNode( writer, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node.nodeType = WS_XML_NODE_TYPE_CDATA;
    hr = WsWriteNode( writer, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf8.value.bytes   = (BYTE *)"cdata";
    utf8.value.length  = sizeof("cdata") - 1;
    text.text          = &utf8.text;
    hr = WsWriteNode( writer, &text.node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node.nodeType = WS_XML_NODE_TYPE_END_CDATA;
    hr = WsWriteNode( writer, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf8.value.bytes   = (BYTE *)"text";
    utf8.value.length  = sizeof("text") - 1;
    hr = WsWriteNode( writer, &text.node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node.nodeType = WS_XML_NODE_TYPE_END_ELEMENT;
    hr = WsWriteNode( writer, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<t attr='value'><!--comment--><![CDATA[cdata]]>text</t>", __LINE__ );

    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static HRESULT set_input( WS_XML_READER *reader, const char *data, ULONG size )
{
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}, WS_CHARSET_AUTO};
    WS_XML_READER_BUFFER_INPUT buf = {{WS_XML_READER_INPUT_TYPE_BUFFER}, (void *)data, size};
    return WsSetInput( reader, &text.encoding, &buf.input, NULL, 0, NULL );
}

static void test_WsCopyNode(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"u"}, ns = {0, NULL};
    WS_XML_NODE_POSITION pos, pos2;
    const WS_XML_NODE *node;
    WS_XML_WRITER *writer;
    WS_XML_READER *reader;
    WS_XML_BUFFER *buffer;
    WS_BUFFERS bufs;
    WS_HEAP *heap;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<t><u/></t>", __LINE__ );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, "<v/>", sizeof("<v/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof("<v/>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetWriterPosition( writer, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCopyNode( writer, reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<t><u/><v/></t>", __LINE__ );

    hr = WsGetWriterPosition( writer, &pos2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos2.buffer == pos.buffer, "wrong buffer\n" );
    ok( pos2.node == pos.node, "wrong node\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* reader positioned at EOF */
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCopyNode( writer, reader, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    /* reader positioned at BOF */
    hr = set_input( reader, "<v/>", sizeof("<v/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof("<v/>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCopyNode( writer, reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<v/>", __LINE__ );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    memset( &bufs, 0, sizeof(bufs) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BUFFERS, &bufs, sizeof(bufs), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( bufs.bufferCount == 1, "got %lu\n", bufs.bufferCount );
    ok( bufs.buffers != NULL, "buffers not set\n" );

    /* reader positioned at BOF, single text node */
    hr = set_input( reader, "text", sizeof("text") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCopyNode( writer, reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeReader( reader );
    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void test_text_types(void)
{
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"u"};
    WS_XML_STRING ns = {0, NULL}, ns2 = {2, (BYTE *)"ns"};
    WS_XML_WRITER *writer;
    static const WS_XML_UTF8_TEXT val_utf8 = { {WS_XML_TEXT_TYPE_UTF8}, {4, (BYTE *)"utf8"} };
    static WS_XML_UTF16_TEXT val_utf16 = { {WS_XML_TEXT_TYPE_UTF16} };
    static WS_XML_QNAME_TEXT val_qname = { {WS_XML_TEXT_TYPE_QNAME} };
    static const WS_XML_GUID_TEXT val_guid = { {WS_XML_TEXT_TYPE_GUID} };
    static const WS_XML_UNIQUE_ID_TEXT val_urn = { {WS_XML_TEXT_TYPE_UNIQUE_ID} };
    static const WS_XML_BOOL_TEXT val_bool = { {WS_XML_TEXT_TYPE_BOOL}, TRUE };
    static const WS_XML_INT32_TEXT val_int32 = { {WS_XML_TEXT_TYPE_INT32}, -2147483647 - 1 };
    static const WS_XML_INT64_TEXT val_int64 = { {WS_XML_TEXT_TYPE_INT64}, -9223372036854775807 - 1 };
    static const WS_XML_UINT64_TEXT val_uint64 = { {WS_XML_TEXT_TYPE_UINT64}, ~0 };
    static const WS_XML_DATETIME_TEXT val_datetime = { {WS_XML_TEXT_TYPE_DATETIME}, {0, WS_DATETIME_FORMAT_UTC} };
    static const WS_XML_DOUBLE_TEXT val_double = { {WS_XML_TEXT_TYPE_DOUBLE}, 1.1 };
    static const WS_XML_BASE64_TEXT val_base64 = { {WS_XML_TEXT_TYPE_BASE64}, (BYTE *)"test", 4 };
    static const struct
    {
        const WS_XML_TEXT *text;
        const char        *result;
    }
    tests[] =
    {
        { &val_utf8.text,   "<t>utf8</t>" },
        { &val_utf16.text,  "<t>utf16</t>" },
        { &val_guid.text,   "<t>00000000-0000-0000-0000-000000000000</t>" },
        { &val_urn.text,    "<t>urn:uuid:00000000-0000-0000-0000-000000000000</t>" },
        { &val_bool.text,   "<t>true</t>" },
        { &val_int32.text,  "<t>-2147483648</t>" },
        { &val_int64.text,  "<t>-9223372036854775808</t>" },
        { &val_uint64.text, "<t>18446744073709551615</t>" },
        { &val_datetime.text, "<t>0001-01-01T00:00:00Z</t>" },
        { &val_double.text, "<t>1.1</t>" },
        { &val_base64.text, "<t>dGVzdA==</t>" },
        { &val_qname.text,  "<t>u</t>" },
    };
    HRESULT hr;
    ULONG i;

    val_utf16.bytes     = (BYTE *)L"utf16";
    val_utf16.byteCount = 10;
    val_qname.localName = &localname2;
    val_qname.ns        = &ns;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %#lx\n", hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteText( writer, tests[i].text, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &prefix, &localname, &ns2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    val_qname.prefix    = &prefix;
    val_qname.localName = &localname2;
    val_qname.ns        = &ns2;
    hr = WsWriteText( writer, &val_qname.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:t xmlns:p=\"ns\">p:u</p:t>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_double(void)
{
    static const BOOL is_win64 = sizeof(void*) > sizeof(int);
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    unsigned int fpword, fpword_orig;
    static const struct
    {
        double      val;
        const char *result;
        BOOL        todo64;
    }
    tests[] =
    {
        {0.0, "<t>0</t>"},
        {1.0, "<t>1</t>"},
        {-1.0, "<t>-1</t>"},
        {1.0000000000000001, "<t>1</t>"},
        {1.0000000000000002, "<t>1.0000000000000002</t>"},
        {1.0000000000000003, "<t>1.0000000000000002</t>"},
        {1.0000000000000004, "<t>1.0000000000000004</t>"},
        {100000000000000, "<t>100000000000000</t>"},
        {1000000000000000, "<t>1E+15</t>"},
        {0.1, "<t>0.1</t>", TRUE},
        {0.01, "<t>1E-2</t>", TRUE},
        {-0.1, "<t>-0.1</t>", TRUE},
        {-0.01, "<t>-1E-2</t>", TRUE},
        {1.7976931348623158e308, "<t>1.7976931348623157E+308</t>", TRUE},
        {-1.7976931348623158e308, "<t>-1.7976931348623157E+308</t>", TRUE},
    };
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_DOUBLE_TEXT text;
    ULONG i;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %#lx\n", hr );

    text.text.textType = WS_XML_TEXT_TYPE_DOUBLE;
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %#lx\n", hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        text.value = tests[i].val;
        hr = WsWriteText( writer, &text.text, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (tests[i].todo64 && is_win64)
        {
            WS_BYTES bytes;
            ULONG size = sizeof(bytes);
            int len = strlen( tests[i].result );
            HRESULT hr;

            memset( &bytes, 0, sizeof(bytes) );
            hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
            ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
            todo_wine
            ok( bytes.length == len && !memcmp( bytes.bytes, tests[i].result, len ),
                "%lu: got %lu %s expected %d %s\n", i, bytes.length,
                debugstr_bytes(bytes.bytes, bytes.length), len, tests[i].result );
        }
        else check_output( writer, tests[i].result, __LINE__ );
    }

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    text.value = NAN;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>NaN</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    text.value = INFINITY;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>INF</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    text.value = -INFINITY;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>-INF</t>", __LINE__ );

    fpword_orig = _control87( 0, 0 );
    fpword = _control87( _MCW_EM | _RC_CHOP | _PC_64, _MCW_EM | _MCW_RC | _MCW_PC );
    ok( fpword == (_MCW_EM | _RC_CHOP | _PC_64), "got %#x\n", fpword );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    text.value = 100000000000000;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>100000000000000</t>", __LINE__ );

    fpword = _control87( 0, 0 );
    ok( fpword == (_MCW_EM | _RC_CHOP | _PC_64), "got %#x\n", fpword );
    _control87( fpword_orig, _MCW_EM | _MCW_RC | _MCW_PC );

    WsFreeWriter( writer );
}

static void test_field_options(void)
{
    static const char expected[] =
        "<t><bool a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/><int32>-1</int32>"
        "<xmlstr a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/></t>";
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, f2, f3, f4, f5, *fields[5];
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL}, str_guid = {4, (BYTE *)"guid"};
    WS_XML_STRING str_int32 = {5, (BYTE *)"int32"}, str_bool = {4, (BYTE *)"bool"};
    WS_XML_STRING str_xmlstr = {6, (BYTE *)"xmlstr"}, str_str = {3, (BYTE *)"str"};
    INT32 val = -1;
    struct test
    {
        GUID           guid;
        BOOL          *bool_ptr;
        INT32         *int32_ptr;
        WS_XML_STRING  xmlstr;
        WCHAR         *str;
    } test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.localName = &str_guid;
    f.ns        = &ns;
    f.type      = WS_GUID_TYPE;
    f.options   = WS_FIELD_OPTIONAL;
    fields[0] = &f;

    memset( &f2, 0, sizeof(f2) );
    f2.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.localName = &str_bool;
    f2.offset    = FIELD_OFFSET(struct test, bool_ptr);
    f2.ns        = &ns;
    f2.type      = WS_BOOL_TYPE;
    f2.options   = WS_FIELD_POINTER|WS_FIELD_NILLABLE;
    fields[1] = &f2;

    memset( &f3, 0, sizeof(f3) );
    f3.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f3.localName = &str_int32;
    f3.offset    = FIELD_OFFSET(struct test, int32_ptr);
    f3.ns        = &ns;
    f3.type      = WS_INT32_TYPE;
    f3.options   = WS_FIELD_POINTER|WS_FIELD_NILLABLE;
    fields[2] = &f3;

    memset( &f4, 0, sizeof(f4) );
    f4.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f4.localName = &str_xmlstr;
    f4.offset    = FIELD_OFFSET(struct test, xmlstr);
    f4.ns        = &ns;
    f4.type      = WS_XML_STRING_TYPE;
    f4.options   = WS_FIELD_NILLABLE;
    fields[3] = &f4;

    memset( &f5, 0, sizeof(f5) );
    f5.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f5.localName = &str_str;
    f5.offset    = FIELD_OFFSET(struct test, str);
    f5.ns        = &ns;
    f5.type      = WS_WSZ_TYPE;
    f5.options   = WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE;
    fields[4] = &f5;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 5;

    memset( &test, 0, sizeof(test) );
    test.int32_ptr = &val;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, expected, __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteText(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"a"}, ns = {0, NULL};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}};
    WS_XML_UTF16_TEXT utf16 = {{WS_XML_TEXT_TYPE_UTF16}};
    WS_XML_GUID_TEXT guid = {{WS_XML_TEXT_TYPE_GUID}};

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf8.value.bytes  = (BYTE *)"test";
    utf8.value.length = 4;
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element, utf8 */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test", __LINE__ );

    utf8.value.bytes  = (BYTE *)"tset";
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtset", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtset</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute, utf8 */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    utf8.value.bytes  = (BYTE *)"test";
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a=\"tsettest\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element, utf16 */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf16.bytes     = (BYTE *)L"test";
    utf16.byteCount = 8;
    hr = WsWriteText( writer, &utf16.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>test", __LINE__ );

    hr = WsWriteText( writer, &utf16.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtest", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtest</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute, utf16 */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf16.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteText( writer, &utf16.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a=\"testtest\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element, guid */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &guid.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>00000000-0000-0000-0000-000000000000", __LINE__ );

    hr = WsWriteText( writer, &guid.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>00000000-0000-0000-0000-00000000000000000000-0000-0000-0000-000000000000",
                  __LINE__ );

    /* continue with different text type */
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>00000000-0000-0000-0000-00000000000000000000-0000-0000-0000-000000000000test",
                  __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute, guid */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &guid.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteText( writer, &guid.text, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute, mix allowed text types */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf16.text, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* cdata */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartCData( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteText( writer, &guid.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndCData( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><![CDATA[test00000000-0000-0000-0000-000000000000]]></t>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteArray(void)
{
    static const WS_XML_STRING localname = {4, (BYTE *)"item"}, localname2 = {5, (BYTE *)"array"};
    static const WS_XML_STRING ns = {0, NULL};
    WS_XML_WRITER *writer;
    BOOL array_bool[2];
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteArray( writer, NULL, NULL, 0, NULL, 0, 0, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, NULL, NULL, 0, NULL, 0, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, &localname, NULL, 0, NULL, 0, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, &localname, &ns, 0, NULL, 0, 0, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteArray( writer, &localname, &ns, ~0u, NULL, 0, 0, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, NULL, 0, 0, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    array_bool[0] = FALSE;
    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, 0, 0, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, sizeof(array_bool), 0, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, NULL, sizeof(array_bool), 0, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, NULL, sizeof(array_bool), 0, 1, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, sizeof(array_bool), 0, 1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<item>false</item>", __LINE__ );

    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, sizeof(array_bool) - 1, 0, 2, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, sizeof(array_bool), 0, 3, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    array_bool[1] = TRUE;
    hr = WsWriteArray( writer, &localname, &ns, WS_BOOL_VALUE_TYPE, array_bool, sizeof(array_bool), 0, 2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<array><item>false</item><item>true</item></array>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_escapes(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}};
    WS_XML_WRITER *writer;
    struct test
    {
        const char *text;
        const char *result;
        BOOL        single;
    };
    static const struct test tests_elem[] =
    {
        { "<",  "<t>&lt;</t>" },
        { ">",  "<t>&gt;</t>" },
        { "\"", "<t>\"</t>" },
        { "&",  "<t>&amp;</t>" },
        { "&&",  "<t>&amp;&amp;</t>" },
        { "'",  "<t>'</t>" },
    };
    static const struct test tests_attr[] =
    {
        { "<",  "<t t=\"&lt;\"/>" },
        { ">",  "<t t=\">\"/>" },
        { "\"", "<t t=\"&quot;\"/>" },
        { "&", "<t t=\"&amp;\"/>" },
        { "'", "<t t=\"'\"/>" },
        { "\"", "<t t='\"'/>", TRUE },
        { "'", "<t t='&apos;'/>", TRUE },
    };
    static const struct test tests_cdata[] =
    {
        { "<",  "<t><![CDATA[<]]></t>" },
        { ">",  "<t><![CDATA[>]]></t>" },
        { "\"", "<t><![CDATA[\"]]></t>" },
        { "&",  "<t><![CDATA[&]]></t>" },
        { "[",  "<t><![CDATA[[]]></t>" },
        { "]",  "<t><![CDATA[]]]></t>" },
        { "'",  "<t><![CDATA[']]></t>" },
    };
    static const struct test tests_comment[] =
    {
        { "<",  "<t><!--<--></t>" },
        { ">",  "<t><!-->--></t>" },
        { "\"", "<t><!--\"--></t>" },
        { "&",  "<t><!--&--></t>" },
        { "'",  "<t><!--'--></t>" },
    };
    HRESULT hr;
    ULONG i;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests_elem ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        utf8.value.bytes  = (BYTE *)tests_elem[i].text;
        utf8.value.length = strlen( tests_elem[i].text );
        hr = WsWriteText( writer, &utf8.text, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        check_output( writer, tests_elem[i].result, __LINE__ );
    }

    for (i = 0; i < ARRAY_SIZE( tests_attr ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, tests_attr[i].single, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        utf8.value.bytes  = (BYTE *)tests_attr[i].text;
        utf8.value.length = strlen( tests_attr[i].text );
        hr = WsWriteText( writer, &utf8.text, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        check_output( writer, tests_attr[i].result, __LINE__ );
    }

    for (i = 0; i < ARRAY_SIZE( tests_cdata ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteStartCData( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        utf8.value.bytes  = (BYTE *)tests_cdata[i].text;
        utf8.value.length = strlen( tests_cdata[i].text );
        hr = WsWriteText( writer, &utf8.text, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndCData( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        check_output( writer, tests_cdata[i].result, __LINE__ );
    }

    for (i = 0; i < ARRAY_SIZE( tests_comment ); i++)
    {
        WS_XML_COMMENT_NODE comment = {{WS_XML_NODE_TYPE_COMMENT}};

        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        comment.value.bytes  = (BYTE *)tests_comment[i].text;
        comment.value.length = strlen( tests_comment[i].text );
        hr = WsWriteNode( writer, &comment.node, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        check_output( writer, tests_comment[i].result, __LINE__ );
    }

    WsFreeWriter( writer );
}

static void test_write_option(void)
{
    static const WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_XML_WRITER *writer;
    int val_int = -1, val_int_zero = 0, *ptr_int = &val_int, *ptr_int_null = NULL;
    const WCHAR *ptr_wsz = L"s", *ptr_wsz_null = NULL;
    static const WS_XML_STRING val_xmlstr = {1, (BYTE *)"x"}, val_xmlstr_zero = {0, NULL};
    const WS_XML_STRING *ptr_xmlstr = &val_xmlstr, *ptr_xmlstr_null = NULL;
    struct
    {
        WS_TYPE          type;
        WS_WRITE_OPTION  option;
        const void      *value;
        ULONG            size;
        HRESULT          hr;
        const char      *result;
    }
    tests[] =
    {
        { WS_INT32_TYPE, 0, NULL, 0, E_INVALIDARG },
        { WS_INT32_TYPE, 0, "str", 0, E_INVALIDARG },
        { WS_INT32_TYPE, 0, NULL, sizeof(val_int), E_INVALIDARG },
        { WS_INT32_TYPE, 0, &val_int, sizeof(val_int), E_INVALIDARG },
        { WS_INT32_TYPE, 0, &val_int_zero, sizeof(val_int_zero), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_VALUE, NULL, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_VALUE, &val_int, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_VALUE, NULL, sizeof(val_int), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_VALUE, &val_int, sizeof(val_int), S_OK, "<t>-1</t>" },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_VALUE, &val_int_zero, sizeof(val_int_zero), S_OK, "<t>0</t>" },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_VALUE, NULL, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_VALUE, &val_int, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_VALUE, NULL, sizeof(val_int), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_VALUE, &val_int, sizeof(val_int), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_VALUE, &val_int_zero, sizeof(val_int_zero), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_int, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, sizeof(ptr_int), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_int, sizeof(ptr_int), S_OK, "<t>-1</t>" },
        { WS_INT32_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_int_null, sizeof(ptr_int_null), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_int, 0, E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, sizeof(ptr_int), E_INVALIDARG },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_int, sizeof(ptr_int), S_OK, "<t>-1</t>" },
        { WS_INT32_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_int_null, sizeof(ptr_int_null), S_OK,
          "<t a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/>" },
        { WS_XML_STRING_TYPE, 0, NULL, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, 0, &val_xmlstr, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, 0, NULL, sizeof(val_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, 0, &val_xmlstr, sizeof(val_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, 0, &val_xmlstr_zero, sizeof(val_xmlstr_zero), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, NULL, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &val_xmlstr, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, NULL, sizeof(&val_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &val_xmlstr, sizeof(val_xmlstr), S_OK, "<t>x</t>" },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_VALUE, &val_xmlstr_zero, sizeof(val_xmlstr_zero), S_OK, "<t/>" },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_VALUE, &val_xmlstr, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_VALUE, NULL, sizeof(&val_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_VALUE, &val_xmlstr, sizeof(&val_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_VALUE, &val_xmlstr_zero, sizeof(val_xmlstr_zero), S_OK,
          "<t a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/>" },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_xmlstr, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, sizeof(ptr_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_xmlstr, sizeof(ptr_xmlstr), S_OK, "<t>x</t>" },
        { WS_XML_STRING_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_xmlstr_null, sizeof(ptr_xmlstr_null), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_xmlstr, 0, E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, sizeof(ptr_xmlstr), E_INVALIDARG },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_xmlstr, sizeof(ptr_xmlstr), S_OK, "<t>x</t>" },
        { WS_XML_STRING_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_xmlstr_null, sizeof(ptr_xmlstr_null), S_OK,
          "<t a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/>" },
        { WS_WSZ_TYPE, 0, NULL, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, 0, &ptr_wsz, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, 0, NULL, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, 0, &ptr_wsz, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, 0, &ptr_wsz_null, sizeof(ptr_wsz_null), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_VALUE, &ptr_wsz, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_VALUE, &ptr_wsz, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_wsz, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, NULL, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_wsz, sizeof(ptr_wsz), S_OK, "<t>s</t>" },
        { WS_WSZ_TYPE, WS_WRITE_REQUIRED_POINTER, &ptr_wsz_null, sizeof(ptr_wsz_null), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_wsz, 0, E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_POINTER, NULL, sizeof(ptr_wsz), E_INVALIDARG },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_wsz, sizeof(ptr_wsz), S_OK, "<t>s</t>" },
        { WS_WSZ_TYPE, WS_WRITE_NILLABLE_POINTER, &ptr_wsz_null, sizeof(ptr_wsz_null), S_OK,
          "<t a:nil=\"true\" xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/>" },
    };
    HRESULT hr;
    ULONG i;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, tests[i].type, NULL, tests[i].option, tests[i].value,
                          tests[i].size, NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        WsWriteEndElement( writer, NULL );
        if (hr == S_OK) check_output( writer, tests[i].result, __LINE__ );
    }

    WsFreeWriter( writer );
}

static BOOL check_result( WS_XML_WRITER *writer, const char *expected )
{
    WS_BYTES bytes;
    ULONG size = sizeof(bytes);
    int len = strlen( expected );

    memset( &bytes, 0, sizeof(bytes) );
    WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    if (bytes.length != len) return FALSE;
    return !memcmp( bytes.bytes, expected, len );
}

static void test_datetime(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    static const struct
    {
        unsigned __int64   ticks;
        WS_DATETIME_FORMAT format;
        HRESULT            hr;
        const char        *result;
        const char        *result2;
        HRESULT            hr_broken;
    }
    tests[] =
    {
        { 0, WS_DATETIME_FORMAT_UTC, S_OK, "<t>0001-01-01T00:00:00Z</t>" },
        { 0, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T00:00:00+00:00</t>" },
        { 0, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:00:00</t>" },
        { 1, WS_DATETIME_FORMAT_UTC, S_OK, "<t>0001-01-01T00:00:00.0000001Z</t>" },
        { 1, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T00:00:00.0000001+00:00</t>" },
        { 1, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:00:00.0000001</t>" },
        { 10, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:00:00.000001</t>" },
        { 1000000, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:00:00.1</t>" },
        { 0x23c34600, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T00:01:00+00:00</t>" },
        { 0x861c46800, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T01:00:00+00:00</t>" },
        { 0x430e234000, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T08:00:00+00:00</t>" },
        { 0x4b6fe7a800, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>0001-01-01T09:00:00+00:00</t>" },
        { 0x989680, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:00:01</t>" },
        { 0x23c34600, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T00:01:00</t>" },
        { 0x861c46800, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-01T01:00:00</t>" },
        { 0xc92a69c000, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0001-01-02T00:00:00</t>" },
        { 0x11ed178c6c000, WS_DATETIME_FORMAT_NONE, S_OK, "<t>0002-01-01T00:00:00</t>" },
        { 0x2bca2875f4373fff, WS_DATETIME_FORMAT_UTC, S_OK, "<t>9999-12-31T23:59:59.9999999Z</t>" },
        { 0x2bca2875f4373fff, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>9999-12-31T15:59:59.9999999-08:00</t>",
          "<t>9999-12-31T17:59:59.9999999-06:00</t>" /* win7 */, WS_E_INVALID_FORMAT },
        { 0x2bca2875f4373fff, WS_DATETIME_FORMAT_NONE, S_OK, "<t>9999-12-31T23:59:59.9999999</t>" },
        { 0x2bca2875f4374000, WS_DATETIME_FORMAT_UTC, WS_E_INVALID_FORMAT },
        { 0x2bca2875f4374000, WS_DATETIME_FORMAT_LOCAL, WS_E_INVALID_FORMAT },
        { 0x2bca2875f4374000, WS_DATETIME_FORMAT_NONE, WS_E_INVALID_FORMAT },
        { 0x8d3123e7df74000, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>2015-12-31T16:00:00-08:00</t>",
          "<t>2015-12-31T18:00:00-06:00</t>" /* win7 */ },
        { 0x701ce1722770000, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>1601-01-01T00:00:00+00:00</t>" },
        { 0x701ce5a309a4000, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>1601-01-01T00:00:00-08:00</t>",
          "<t>1601-01-01T02:00:00-06:00</t>" /* win7 */ },
        { 0x701ce5e617c7400, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>1601-01-01T00:30:00-08:00</t>",
          "<t>1601-01-01T02:30:00-06:00</t>" /* win7 */ },
        { 0x701ce51ced5d800, WS_DATETIME_FORMAT_LOCAL, S_OK, "<t>1601-01-01T07:00:00+00:00</t>",
          "<t>1601-01-01T01:00:00-06:00</t>" /* win7 */ },
        { 0, WS_DATETIME_FORMAT_NONE + 1, WS_E_INVALID_FORMAT },
        { 0x38a080649c000, WS_DATETIME_FORMAT_UTC, S_OK, "<t>0004-02-28T00:00:00Z</t>" },
        { 0x38ad130b38000, WS_DATETIME_FORMAT_UTC, S_OK, "<t>0004-02-29T00:00:00Z</t>" },
        { 0x8c1505f0e438000, WS_DATETIME_FORMAT_UTC, S_OK, "<t>2000-02-29T00:00:00Z</t>" },
        { 0x8d46035e7870000, WS_DATETIME_FORMAT_UTC, S_OK, "<t>2017-03-01T00:00:00Z</t>" },
    };
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_DATETIME date;
    ULONG i;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %#lx\n", hr );

        date.ticks  = tests[i].ticks;
        date.format = tests[i].format;
        WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_DATETIME_TYPE, NULL, WS_WRITE_REQUIRED_VALUE,
                          &date, sizeof(date), NULL );
        WsWriteEndElement( writer, NULL );
        ok( hr == tests[i].hr || broken(hr == tests[i].hr_broken), "%lu: got %#lx\n", i, hr );
        if (hr != tests[i].hr && hr == tests[i].hr_broken) break;
        if (hr == S_OK)
        {
            ok( check_result( writer, tests[i].result ) ||
                (tests[i].result2 && broken(check_result( writer, tests[i].result2 ))),
                "%lu: wrong result\n", i );
        }
    }

    WsFreeWriter( writer );
}

static void test_repeating_element(void)
{
    WS_XML_STRING localname = {4, (BYTE *)"test"}, ns = {0, NULL}, data = {4, (BYTE *)"data"};
    WS_XML_STRING val = {3, (BYTE *)"val"}, wrapper = {7, (BYTE *)"wrapper"}, structval = {9, (BYTE *)"structval"};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f2, *fields[1], *fields2[1];
    WS_ITEM_RANGE range;
    struct test
    {
        const WCHAR **val;
        ULONG         count;
    } *test;
    struct test2
    {
        INT32 *val;
        ULONG  count;
    } *test2;
    struct value
    {
        INT32 data;
    } value;
    struct test3
    {
        const struct value **val;
        ULONG                count;
    } *test3;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* array of strings, wrapper */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping       = WS_REPEATING_ELEMENT_FIELD_MAPPING;
    f.localName     = &wrapper;
    f.ns            = &ns;
    f.type          = WS_WSZ_TYPE;
    f.countOffset   = FIELD_OFFSET(struct test, count);
    f.itemLocalName = &val;
    f.itemNs        = &ns;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.typeLocalName = &localname;
    s.typeNs        = &ns;
    s.fields        = fields;
    s.fieldCount    = 1;

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    test = malloc( sizeof(*test) + 2 * sizeof(const WCHAR *) );
    test->val = (const WCHAR **)(test + 1);
    test->val[0] = L"1";
    test->val[1] = L"2";
    test->count  = 2;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<test><wrapper><val>1</val><val>2</val></wrapper></test>", __LINE__ );
    free( test );

    /* array of integers, no wrapper */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    f.type      = WS_INT32_TYPE;
    f.localName = NULL;
    f.ns        = NULL;

    test2 = malloc( sizeof(*test2) + 2 * sizeof(INT32) );
    test2->val = (INT32 *)(test2 + 1);
    test2->val[0] = 1;
    test2->val[1] = 2;
    test2->count  = 2;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<test><val>1</val><val>2</val></test>", __LINE__ );

    /* item range has no effect */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    range.minItemCount = 0;
    range.maxItemCount = 0;
    f.itemRange = &range;

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<test><val>1</val><val>2</val></test>", __LINE__ );
    free( test2 );

    /* nillable item */
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping       = WS_ELEMENT_FIELD_MAPPING;
    f2.localName     = &data;
    f2.ns            = &ns;
    f2.type          = WS_INT32_TYPE;
    fields2[0] = &f2;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(struct test3);
    s2.alignment     = TYPE_ALIGNMENT(struct test3);
    s2.typeLocalName = &structval;
    s2.typeNs        = &ns;
    s2.fields        = fields2;
    s2.fieldCount    = 1;

    f.type            = WS_STRUCT_TYPE;
    f.typeDescription = &s2;
    f.localName       = &wrapper;
    f.ns              = &ns;
    f.itemRange       = NULL;
    f.options         = WS_FIELD_POINTER|WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM;

    value.data = -1;
    test3 = malloc( sizeof(*test3) + 2 * sizeof(const struct value *) );
    test3->val = (const struct value **)(test3 + 1);
    test3->val[0] = &value;
    test3->val[1] = NULL;
    test3->count  = 2;

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test3, sizeof(test3), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<test><wrapper><val><data>-1</data></val><val a:nil=\"true\" "
                          "xmlns:a=\"http://www.w3.org/2001/XMLSchema-instance\"/></wrapper></test>", __LINE__ );
    free( test3 );

    WsFreeWriter( writer );
}

static const WS_XML_STRING *init_xmlstring( const char *src, WS_XML_STRING *str )
{
    if (!src) return NULL;
    str->length     = strlen( src );
    str->bytes      = (BYTE *)src;
    str->dictionary = NULL;
    str->id         = 0;
    return str;
}

static void test_WsWriteQualifiedName(void)
{
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, localname = {1, (BYTE *)"t"}, ns = {2, (BYTE *)"ns"};
    WS_XML_WRITER *writer;
    HRESULT hr;
    ULONG i;
    static const struct
    {
        const char *prefix;
        const char *localname;
        const char *ns;
        HRESULT     hr;
        const char *result;
    } tests[] =
    {
        { NULL, NULL, NULL, E_INVALIDARG, NULL },
        { NULL, "t2", NULL, E_INVALIDARG, NULL },
        { "p2", "t2", NULL, S_OK, "<p:t xmlns:p=\"ns\">p2:t2" },
        { NULL, "t2", "ns2", WS_E_INVALID_FORMAT, NULL },
        { NULL, "t2", "ns", S_OK, "<p:t xmlns:p=\"ns\">p:t2" },
        { "p2", "t2", "ns2", S_OK, "<p:t xmlns:p=\"ns\">p2:t2" },
        { "p2", "t2", "ns", S_OK, "<p:t xmlns:p=\"ns\">p2:t2" },
        { "p", "t", NULL, S_OK, "<p:t xmlns:p=\"ns\">p:t" },
        { NULL, "t", "ns", S_OK, "<p:t xmlns:p=\"ns\">p:t" },
        { "p2", "", "", S_OK, "<p:t xmlns:p=\"ns\">p2:" },
        { "p2", "", "ns2", S_OK, "<p:t xmlns:p=\"ns\">p2:" },
        { "p2", "t2", "", S_OK, "<p:t xmlns:p=\"ns\">p2:t2" },
        { "", "t2", "", S_OK, "<p:t xmlns:p=\"ns\">t2" },
        { "", "", "ns2", S_OK, "<p:t xmlns:p=\"ns\">" },
        { "", "", "", S_OK, "<p:t xmlns:p=\"ns\">" },
    };

    hr = WsWriteQualifiedName( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteQualifiedName( writer, NULL, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteQualifiedName( writer, NULL, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        WS_XML_STRING prefix2, localname2, ns2;
        const WS_XML_STRING *prefix_ptr, *localname_ptr, *ns_ptr;

        hr = set_output( writer );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix_ptr = init_xmlstring( tests[i].prefix, &prefix2 );
        localname_ptr = init_xmlstring( tests[i].localname, &localname2 );
        ns_ptr = init_xmlstring( tests[i].ns, &ns2 );

        hr = WsWriteQualifiedName( writer, prefix_ptr, localname_ptr, ns_ptr, NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (tests[i].hr == S_OK && hr == S_OK) check_output( writer, tests[i].result, __LINE__ );
    }

    WsFreeWriter( writer );
}

static void test_WsWriteBytes(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"a"}, ns = {0, NULL};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsWriteBytes( NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, NULL, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, "test", 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, NULL, 1, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, "test", sizeof("test"), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, "test", sizeof("test"), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, "test", sizeof("test"), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>dGVzdAA=</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteBytes( writer, "test", sizeof("test"), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a=\"dGVzdAA=\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteChars(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"a"}, ns = {0, NULL};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsWriteChars( NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, NULL, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteChars( writer, NULL, 1, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtest</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteChars( writer, L"test", 4, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a=\"testtest\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteCharsUtf8(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"a"}, ns = {0, NULL};
    static const BYTE test[] = {'t','e','s','t'};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsWriteCharsUtf8( NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, NULL, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, 0, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, NULL, 1, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t>testtest</t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* attribute */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteCharsUtf8( writer, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t a=\"testtest\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static void check_output_bin( WS_XML_WRITER *writer, const char *expected, int len, unsigned int line )
{
    WS_BYTES bytes;
    ULONG size = sizeof(bytes);
    HRESULT hr;

    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "%u: got %#lx\n", line, hr );
    ok( bytes.length == len, "%u: got %lu expected %d\n", line, bytes.length, len );
    if (bytes.length != len) return;
    ok( !memcmp( bytes.bytes, expected, bytes.length ), "%u: got %s expected %s\n", line,
        debugstr_bytes(bytes.bytes, bytes.length), debugstr_bytes((const BYTE *)expected, len) );
}

static void test_binary_encoding(void)
{
    static const char res[] =
        {0x40,0x01,'t',0x01};
    static const char res2[] =
        {0x6d,0x01,'t',0x09,0x01,'p',0x02,'n','s',0x01};
    static const char res3[] =
        {0x41,0x02,'p','2',0x01,'t',0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char res4[] =
        {0x41,0x02,'p','2',0x01,'t',0x09,0x02,'p','2',0x02,'n','s',0x99,0x04,'t','e','s','t'};
    static const char res100[] =
        {0x40,0x01,'t',0x04,0x01,'t',0x98,0x00,0x01};
    static const char res101[] =
        {0x40,0x01,'t',0x35,0x01,'t',0x98,0x00,0x09,0x01,'p',0x02,'n','s',0x01};
    static const char res102[] =
        {0x40,0x01,'t',0x05,0x02,'p','2',0x01,'t',0x98,0x00,0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char res103[] =
        {0x40,0x01,'t',0x05,0x02,'p','2',0x01,'t',0x98,0x04,'t','e','s','t',0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char res200[] =
        {0x02,0x07,'c','o','m','m','e','n','t'};
    WS_XML_WRITER_BINARY_ENCODING bin = {{WS_XML_WRITER_ENCODING_TYPE_BINARY}};
    WS_XML_WRITER_BUFFER_OUTPUT buf = {{WS_XML_WRITER_OUTPUT_TYPE_BUFFER}};
    static const char prefix[] = "p", prefix2[] = "p2";
    static const char localname[] = "t", ns[] = "ns";
    const WS_XML_STRING *prefix_ptr, *localname_ptr, *ns_ptr;
    WS_XML_STRING str, str2, str3, localname2 = {1, (BYTE *)"t"}, empty = {0, NULL};
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}};
    WS_XML_COMMENT_NODE comment = {{WS_XML_NODE_TYPE_COMMENT}};
    WS_XML_WRITER *writer;
    HRESULT hr;
    ULONG i;
    static const struct
    {
        const char *prefix;
        const char *localname;
        const char *ns;
        const char *text;
        const char *result;
        int         len_result;
    }
    elem_tests[] =
    {
        { NULL, localname, "", NULL, res, sizeof(res) },            /* short element */
        { prefix, localname, ns, NULL, res2, sizeof(res2) },        /* one character prefix element */
        { prefix2, localname, ns, NULL, res3, sizeof(res3) },       /* element */
        { prefix2, localname, ns, "test", res4, sizeof(res4) },     /* element with text */
    };
    static const struct
    {
        const char *prefix;
        const char *localname;
        const char *ns;
        const char *value;
        const char *result;
        int         len_result;
    }
    attr_tests[] =
    {
        { NULL, localname, "", NULL, res100, sizeof(res100) },          /* short attribute */
        { prefix, localname, ns, NULL, res101, sizeof(res101) },        /* one character prefix attribute */
        { prefix2, localname, ns, NULL, res102, sizeof(res102) },       /* attribute */
        { prefix2, localname, ns, "test", res103, sizeof(res103) },     /* attribute with value */
    };

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( elem_tests ); i++)
    {
        hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix_ptr = init_xmlstring( elem_tests[i].prefix, &str );
        localname_ptr = init_xmlstring( elem_tests[i].localname, &str2 );
        ns_ptr = init_xmlstring( elem_tests[i].ns, &str3 );

        hr = WsWriteStartElement( writer, prefix_ptr, localname_ptr, ns_ptr, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (elem_tests[i].text)
        {
            utf8.value.length = strlen( elem_tests[i].text );
            utf8.value.bytes  = (BYTE *)elem_tests[i].text;
            hr = WsWriteText( writer, &utf8.text, NULL );
            ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        }
        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK) check_output_bin( writer, elem_tests[i].result, elem_tests[i].len_result, __LINE__ );
    }

    for (i = 0; i < ARRAY_SIZE( attr_tests ); i++)
    {
        hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix_ptr = init_xmlstring( attr_tests[i].prefix, &str );
        localname_ptr = init_xmlstring( attr_tests[i].localname, &str2 );
        ns_ptr = init_xmlstring( elem_tests[i].ns, &str3 );

        hr = WsWriteStartElement( writer, NULL, &localname2, &empty, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartAttribute( writer, prefix_ptr, localname_ptr, ns_ptr, FALSE, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (attr_tests[i].value)
        {
            utf8.value.length = strlen( attr_tests[i].value );
            utf8.value.bytes  = (BYTE *)attr_tests[i].value;
            hr = WsWriteText( writer, &utf8.text, NULL );
            ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        }
        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK) check_output_bin( writer, attr_tests[i].result, attr_tests[i].len_result, __LINE__ );
    }

    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    comment.value.bytes   = (BYTE *)"comment";
    comment.value.length  = sizeof("comment") - 1;
    hr = WsWriteNode( writer, &comment.node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr == S_OK) check_output_bin( writer, res200, sizeof(res200), __LINE__ );

    WsFreeWriter( writer );
}

static void test_namespaces(void)
{
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, prefix2 = {1, (BYTE *)"q"};
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"a"};
    WS_XML_STRING ns = {1, (BYTE *)"n"}, ns2 = {1, (BYTE *)"o"};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartAttribute( writer, &prefix2, &localname2, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<p:t q:a=\"\" xmlns:p=\"n\" xmlns:q=\"o\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static const WS_XML_STRING *init_xmlstring_dict( WS_XML_DICTIONARY *dict, ULONG id, WS_XML_STRING *str )
{
    if (id >= dict->stringCount) return NULL;
    str->length     = dict->strings[id].length;
    str->bytes      = dict->strings[id].bytes;
    str->dictionary = dict;
    str->id         = id;
    return str;
}

static HRESULT CALLBACK dict_cb( void *state, const WS_XML_STRING *str, BOOL *found, ULONG *id, WS_ERROR *error )
{
    ULONG *call_count = state;

    (*call_count)++;
    switch (str->bytes[0])
    {
    case 't':
        *id = 1;
        *found = TRUE;
        break;

    case 'n':
        *id = 2;
        *found = TRUE;
        break;

    case 'z':
        *id = 3;
        *found = TRUE;
        break;

    case 'v':
        *found = FALSE;
        return WS_E_OTHER;

    default:
        *found = FALSE;
        break;
    }
    return S_OK;
}

static void test_dictionary(void)
{
    static const char res[] =
        {0x42,0x04,0x01};
    static const char res2[] =
        {0x42,0x06,0x01};
    static const char res3[] =
        {0x53,0x06,0x0b,0x01,'p',0x0a,0x01};
    static const char res4[] =
        {0x43,0x02,'p','2',0x06,0x0b,0x02,'p','2',0x0a,0x01};
    static const char res5[] =
        {0x42,0x03,0x0a,0x05,0x01};
    static const char res6[] =
        {0x40,0x01,0x75,0x0a,0x05,0x01};
    static const char res7[] =
        {0x40,0x01,0x76,0x0a,0x05,0x01};
    static const char res8[] =
        {0x42,0x03,0x0a,0x05,0x01};
    static const char res9[] =
        {0x42,0x07,0x0a,0x05,0x01};
    static const char res10[] =
        {0x42,0xd6,0x03,0x0a,0x05,0x01};
    static const char res100[] =
        {0x42,0x06,0x06,0x06,0x98,0x00,0x01};
    static const char res101[] =
        {0x42,0x06,0x1b,0x06,0x98,0x00,0x0b,0x01,'p',0x0a,0x01};
    static const char res102[] =
        {0x42,0x06,0x07,0x02,'p','2',0x06,0x98,0x00,0x0b,0x02,'p','2',0x0a,0x01};
    WS_XML_WRITER_BINARY_ENCODING bin = {{WS_XML_WRITER_ENCODING_TYPE_BINARY}};
    WS_XML_WRITER_BUFFER_OUTPUT buf = {{WS_XML_WRITER_OUTPUT_TYPE_BUFFER}};
    WS_XML_STRING prefix, localname, ns, strings[6];
    const WS_XML_STRING *prefix_ptr, *localname_ptr, *ns_ptr;
    WS_XML_DICTIONARY dict, *dict_builtin;
    WS_XML_WRITER *writer;
    HRESULT hr;
    ULONG i, call_count;
    static const struct
    {
        ULONG       prefix;
        ULONG       localname;
        ULONG       ns;
        const char *result;
        int         len_result;
    }
    elem_tests[] =
    {
        { ~0u, 2, 0, res, sizeof(res) },    /* short dictionary element, invalid dict id */
        { ~0u, 3, 0, res2, sizeof(res2) },  /* short dictionary element */
        { 1, 3, 5, res3, sizeof(res3) },    /* single character prefix dictionary element */
        { 4, 3, 5, res4, sizeof(res4) },    /* dictionary element */
    };
    static const struct
    {
        ULONG       prefix;
        ULONG       localname;
        ULONG       ns;
        const char *result;
        int         len_result;
    }
    attr_tests[] =
    {
        { ~0u, 3, 0, res100, sizeof(res100) },  /* short dictionary attribute */
        { 1, 3, 5, res101, sizeof(res101) },    /* single character prefix dictionary attribute */
        { 4, 3, 5, res102, sizeof(res102) },    /* dictionary attribute */
    };

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    strings[0].length     = 0;
    strings[0].bytes      = NULL;
    strings[0].dictionary = &dict;
    strings[0].id         = 0;
    strings[1].length     = 1;
    strings[1].bytes      = (BYTE *)"p";
    strings[1].dictionary = &dict;
    strings[1].id         = 1;
    strings[2].length     = 1;
    strings[2].bytes      = (BYTE *)"t";
    strings[2].dictionary = &dict;
    strings[2].id         = ~0u;
    strings[3].length     = 1;
    strings[3].bytes      = (BYTE *)"u";
    strings[3].dictionary = &dict;
    strings[3].id         = 3;
    strings[4].length     = 2;
    strings[4].bytes      = (BYTE *)"p2";
    strings[4].dictionary = &dict;
    strings[4].id         = 4;
    strings[5].length     = 2;
    strings[5].bytes      = (BYTE *)"ns";
    strings[5].dictionary = &dict;
    strings[5].id         = 5;

    UuidCreate( &dict.guid );
    dict.strings     = strings;
    dict.stringCount = ARRAY_SIZE( strings );
    dict.isConst     = TRUE;

    bin.staticDictionary = &dict;

    for (i = 0; i < ARRAY_SIZE( elem_tests ); i++)
    {
        hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix_ptr = init_xmlstring_dict( &dict, elem_tests[i].prefix, &prefix );
        localname_ptr = init_xmlstring_dict( &dict, elem_tests[i].localname, &localname );
        ns_ptr = init_xmlstring_dict( &dict, elem_tests[i].ns, &ns );

        hr = WsWriteStartElement( writer, prefix_ptr, localname_ptr, ns_ptr, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK) check_output_bin( writer, elem_tests[i].result, elem_tests[i].len_result, __LINE__ );
    }

    for (i = 0; i < ARRAY_SIZE( attr_tests ); i++)
    {
        hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix_ptr = init_xmlstring_dict( &dict, attr_tests[i].prefix, &prefix );
        localname_ptr = init_xmlstring_dict( &dict, attr_tests[i].localname, &localname );
        ns_ptr = init_xmlstring_dict( &dict, attr_tests[i].ns, &ns );

        hr = WsWriteStartElement( writer, NULL, &strings[3], &strings[0], NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartAttribute( writer, prefix_ptr, localname_ptr, ns_ptr, FALSE, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK) check_output_bin( writer, attr_tests[i].result, attr_tests[i].len_result, __LINE__ );
    }

    /* callback */
    bin.staticDictionary = NULL;
    bin.dynamicStringCallback = dict_cb;
    bin.dynamicStringCallbackState = &call_count;

    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring( "t", &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 2, "got %lu\n", call_count );
    check_output_bin( writer, res5, sizeof(res5), __LINE__ );

    /* unknown string */
    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring( "u", &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 2, "got %lu\n", call_count );
    check_output_bin( writer, res6, sizeof(res6), __LINE__ );

    /* unknown string, error return from callback */
    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring( "v", &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 2, "got %lu\n", call_count );
    check_output_bin( writer, res7, sizeof(res7), __LINE__ );

    /* dictionary and callback */
    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_1, &dict_builtin, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    bin.staticDictionary = dict_builtin;

    /* string in dictionary, no string dictionary set */
    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring( "t", &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 2, "got %lu\n", call_count );
    check_output_bin( writer, res8, sizeof(res8), __LINE__ );

    /* string not in dictionary, no string dictionary set */
    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring( "z", &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 2, "got %lu\n", call_count );
    check_output_bin( writer, res9, sizeof(res9), __LINE__ );

    /* string in dictionary, string dictionary set */
    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    init_xmlstring_dict( dict_builtin, 235, &localname );
    init_xmlstring( "ns", &ns );
    call_count = 0;
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( call_count == 1, "got %lu\n", call_count );
    check_output_bin( writer, res10, sizeof(res10), __LINE__ );

    WsFreeWriter( writer );
}

static void test_union_type(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_a = {1, (BYTE *)"a"}, str_b = {1, (BYTE *)"b"};
    static WS_XML_STRING str_none = {4, (BYTE *)"none"}, str_s = {1, (BYTE *)"s"}, str_t = {1, (BYTE *)"t"};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_UNION_DESCRIPTION u;
    WS_UNION_FIELD_DESCRIPTION f, f2, f3, *fields[3];
    WS_FIELD_DESCRIPTION f_struct, *fields_struct[1];
    WS_STRUCT_DESCRIPTION s;
    enum choice {CHOICE_A = 30, CHOICE_B = 20, CHOICE_C = 10, CHOICE_NONE = 0};
    ULONG index[2] = {1, 0};
    struct test
    {
        enum choice choice;
        union
        {
            const WCHAR *a;
            UINT32       b;
            BOOL         none;
        } value;
    } test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.value           = CHOICE_A;
    f.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.field.localName = &str_a;
    f.field.ns        = &str_ns;
    f.field.type      = WS_WSZ_TYPE;
    f.field.offset    = FIELD_OFFSET(struct test, value.a);
    fields[0] = &f;

    memset( &f2, 0, sizeof(f2) );
    f2.value           = CHOICE_B;
    f2.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.field.localName = &str_b;
    f2.field.ns        = &str_ns;
    f2.field.type      = WS_UINT32_TYPE;
    f2.field.offset    = FIELD_OFFSET(struct test, value.b);
    fields[1] = &f2;

    memset( &u, 0, sizeof(u) );
    u.size          = sizeof(struct test);
    u.alignment     = TYPE_ALIGNMENT(struct test);
    u.fields        = fields;
    u.fieldCount    = 2;
    u.enumOffset    = FIELD_OFFSET(struct test, choice);
    u.noneEnumValue = CHOICE_NONE;

    memset( &f_struct, 0, sizeof(f_struct) );
    f_struct.mapping         = WS_ELEMENT_CHOICE_FIELD_MAPPING;
    f_struct.type            = WS_UNION_TYPE;
    f_struct.typeDescription = &u;
    fields_struct[0] = &f_struct;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields_struct;
    s.fieldCount    = 1;
    s.typeLocalName = &str_s;
    s.typeNs        = &str_ns;

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    test.choice  = CHOICE_A;
    test.value.a = L"test";
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><a>test</a></t>", __LINE__ );

    u.valueIndices = index;
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    test.choice  = CHOICE_B;
    test.value.b = 123;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><b>123</b></t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    test.choice = CHOICE_C;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    test.choice  = CHOICE_NONE;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    /* field value equals noneEnumValue */
    memset( &f3, 0, sizeof(f3) );
    f3.value           = CHOICE_NONE;
    f3.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f3.field.localName = &str_none;
    f3.field.ns        = &str_ns;
    f3.field.type      = WS_BOOL_TYPE;
    f3.field.offset    = FIELD_OFFSET(struct test, value.none);
    fields[2] = &f3;

    u.fieldCount = 3;
    u.valueIndices = NULL;
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    test.choice     = CHOICE_NONE;
    test.value.none = TRUE;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><none>true</none></t>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    f_struct.options = WS_FIELD_OPTIONAL;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t/>", __LINE__ );

    WsFreeWriter( writer );
}

static void prepare_binary_type_test( WS_XML_WRITER *writer, const WS_XML_STRING *prefix,
                                      const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    WS_XML_WRITER_BINARY_ENCODING bin = {{WS_XML_WRITER_ENCODING_TYPE_BINARY}};
    WS_XML_WRITER_BUFFER_OUTPUT buf = {{WS_XML_WRITER_OUTPUT_TYPE_BUFFER}};
    HRESULT hr;

    hr = WsSetOutput( writer, &bin.encoding, &buf.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteStartElement( writer, prefix, localname, ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_text_types_binary(void)
{
    static WS_XML_STRING str_s = {1, (BYTE *)"s"}, str_t = {1, (BYTE *)"t"}, str_u = {1, (BYTE *)"u"};
    static WS_XML_STRING str_ns = {0, NULL};
    static const char res[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x99,0x04,'t','e','s','t',0x01};
    static const char res2[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9e,0x03,'t','e','s',0x9f,0x01,'t',0x01};
    static const char res2a[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9f,0x03,'t','e','s',0x01};
    static const char res2b[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9f,0x01,'t',0x01};
    static const char res2c[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x01,0x01};
    static const char res2d[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9f,0xff,'a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a',0x01};
    static const char res2e[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9e,0xff,'a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
         'a','a','a','a','a',0x9f,0x01,'a',0x01};
    static const char res2f[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x9f,0x06,'t','e','s','t','t','e',0x01};
    static const char res3[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x87,0x01};
    static const char res4[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x89,0xff,0x01};
    static const char res5[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x83,0x01};
    static const char res5b[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x89,0x02,0x01};
    static const char res6[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x81,0x01};
    static const char res7[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x89,0x02,0x01};
    static const char res8[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x93,0xcd,0xcc,0xcc,0xcc,0xcc,0xcc,0x00,0x40,0x01};
    static const char res8a[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x93,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x7f,0x01};
    static const char res8b[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x93,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0xff,0x01};
    static const char res8c[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x93,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x7f,0x01};
    static const char res9[] =
        {0x40,0x01,'t',0x40,0x01,'u',0xb1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    static const char res10[] =
        {0x40,0x01,'t',0x40,0x01,'u',0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    static const char res11[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x01};
    static const char res11b[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01};
    static const char res11c[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    static const char res11d[] =
        {0x40,0x01,'t',0x40,0x01,'u',0x97,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x01};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_UNION_DESCRIPTION u;
    WS_UNION_FIELD_DESCRIPTION f, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, *fields[11];
    WS_FIELD_DESCRIPTION f_struct, *fields_struct[1];
    WS_STRUCT_DESCRIPTION s;
    struct test
    {
        WS_XML_TEXT_TYPE type;
        union
        {
            WS_XML_STRING val_utf8;
            WS_STRING     val_utf16;
            WS_BYTES      val_bytes;
            BOOL          val_bool;
            INT32         val_int32;
            INT64         val_int64;
            UINT64        val_uint64;
            double        val_double;
            GUID          val_guid;
            WS_UNIQUE_ID  val_unique_id;
            WS_DATETIME   val_datetime;
        } u;
    } test;
    BYTE buf[256];

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.value           = WS_XML_TEXT_TYPE_UTF8;
    f.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.field.localName = &str_u;
    f.field.ns        = &str_ns;
    f.field.type      = WS_XML_STRING_TYPE;
    f.field.offset    = FIELD_OFFSET(struct test, u.val_utf8);
    fields[0] = &f;

    memset( &f2, 0, sizeof(f2) );
    f2.value           = WS_XML_TEXT_TYPE_UTF16;
    f2.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.field.localName = &str_u;
    f2.field.ns        = &str_ns;
    f2.field.type      = WS_STRING_TYPE;
    f2.field.offset    = FIELD_OFFSET(struct test, u.val_utf16);
    fields[1] = &f2;

    memset( &f3, 0, sizeof(f3) );
    f3.value           = WS_XML_TEXT_TYPE_BASE64;
    f3.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f3.field.localName = &str_u;
    f3.field.ns        = &str_ns;
    f3.field.type      = WS_BYTES_TYPE;
    f3.field.offset    = FIELD_OFFSET(struct test, u.val_bytes);
    fields[2] = &f3;

    memset( &f4, 0, sizeof(f4) );
    f4.value           = WS_XML_TEXT_TYPE_BOOL;
    f4.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f4.field.localName = &str_u;
    f4.field.ns        = &str_ns;
    f4.field.type      = WS_BOOL_TYPE;
    f4.field.offset    = FIELD_OFFSET(struct test, u.val_bool);
    fields[3] = &f4;

    memset( &f5, 0, sizeof(f5) );
    f5.value           = WS_XML_TEXT_TYPE_INT32;
    f5.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f5.field.localName = &str_u;
    f5.field.ns        = &str_ns;
    f5.field.type      = WS_INT32_TYPE;
    f5.field.offset    = FIELD_OFFSET(struct test, u.val_int32);
    fields[4] = &f5;

    memset( &f6, 0, sizeof(f6) );
    f6.value           = WS_XML_TEXT_TYPE_INT64;
    f6.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f6.field.localName = &str_u;
    f6.field.ns        = &str_ns;
    f6.field.type      = WS_INT64_TYPE;
    f6.field.offset    = FIELD_OFFSET(struct test, u.val_int64);
    fields[5] = &f6;

    memset( &f7, 0, sizeof(f7) );
    f7.value           = WS_XML_TEXT_TYPE_UINT64;
    f7.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f7.field.localName = &str_u;
    f7.field.ns        = &str_ns;
    f7.field.type      = WS_UINT64_TYPE;
    f7.field.offset    = FIELD_OFFSET(struct test, u.val_uint64);
    fields[6] = &f7;

    memset( &f8, 0, sizeof(f8) );
    f8.value           = WS_XML_TEXT_TYPE_DOUBLE;
    f8.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f8.field.localName = &str_u;
    f8.field.ns        = &str_ns;
    f8.field.type      = WS_DOUBLE_TYPE;
    f8.field.offset    = FIELD_OFFSET(struct test, u.val_double);
    fields[7] = &f8;

    memset( &f9, 0, sizeof(f9) );
    f9.value           = WS_XML_TEXT_TYPE_GUID;
    f9.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f9.field.localName = &str_u;
    f9.field.ns        = &str_ns;
    f9.field.type      = WS_GUID_TYPE;
    f9.field.offset    = FIELD_OFFSET(struct test, u.val_guid);
    fields[8] = &f9;

    memset( &f10, 0, sizeof(f10) );
    f10.value           = WS_XML_TEXT_TYPE_UNIQUE_ID;
    f10.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f10.field.localName = &str_u;
    f10.field.ns        = &str_ns;
    f10.field.type      = WS_UNIQUE_ID_TYPE;
    f10.field.offset    = FIELD_OFFSET(struct test, u.val_unique_id);
    fields[9] = &f10;

    memset( &f11, 0, sizeof(f11) );
    f11.value           = WS_XML_TEXT_TYPE_DATETIME;
    f11.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f11.field.localName = &str_u;
    f11.field.ns        = &str_ns;
    f11.field.type      = WS_DATETIME_TYPE;
    f11.field.offset    = FIELD_OFFSET(struct test, u.val_datetime);
    fields[10] = &f11;

    memset( &u, 0, sizeof(u) );
    u.size          = sizeof(struct test);
    u.alignment     = TYPE_ALIGNMENT(struct test);
    u.fields        = fields;
    u.fieldCount    = 11;
    u.enumOffset    = FIELD_OFFSET(struct test, type);

    memset( &f_struct, 0, sizeof(f_struct) );
    f_struct.mapping         = WS_ELEMENT_CHOICE_FIELD_MAPPING;
    f_struct.type            = WS_UNION_TYPE;
    f_struct.typeDescription = &u;
    fields_struct[0] = &f_struct;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields_struct;
    s.fieldCount    = 1;
    s.typeLocalName = &str_s;
    s.typeNs        = &str_ns;

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_UTF8;
    test.u.val_utf8.bytes  = (BYTE *)"test";
    test.u.val_utf8.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res, sizeof(res), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_UTF16;
    test.u.val_utf16.chars  = (WCHAR *)L"test";
    test.u.val_utf16.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res, sizeof(res), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = (BYTE *)"test";
    test.u.val_bytes.length = 4;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2, sizeof(res2), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = (BYTE *)"tes";
    test.u.val_bytes.length = 3;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2a, sizeof(res2a), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = (BYTE *)"t";
    test.u.val_bytes.length = 1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2b, sizeof(res2b), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = (BYTE *)"";
    test.u.val_bytes.length = 0;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2c, sizeof(res2c), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    memset( buf, 'a', sizeof(buf) );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = buf;
    test.u.val_bytes.length = 255;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2d, sizeof(res2d), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = buf;
    test.u.val_bytes.length = 256;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2e, sizeof(res2e), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BASE64;
    test.u.val_bytes.bytes  = (BYTE *)"testte";
    test.u.val_bytes.length = 6;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res2f, sizeof(res2f), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_BOOL;
    test.u.val_bool = TRUE;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res3, sizeof(res3), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_INT32;
    test.u.val_int32 = -1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res4, sizeof(res4), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_INT64;
    test.u.val_int64 = -1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res4, sizeof(res4), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_UINT64;
    test.u.val_uint64 = 1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res5, sizeof(res5), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_UINT64;
    test.u.val_uint64 = 2;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res5b, sizeof(res5b), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = 0.0;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res6, sizeof(res6), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = 2.0;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res7, sizeof(res7), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = 2.1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res8, sizeof(res8), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = INFINITY;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res8a, sizeof(res8a), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = -INFINITY;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res8b, sizeof(res8b), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DOUBLE;
    test.u.val_double = NAN;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res8c, sizeof(res8c), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_GUID;
    memset( &test.u.val_guid, 0, sizeof(test.u.val_guid) );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res9, sizeof(res9), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_UNIQUE_ID;
    memset( &test.u.val_unique_id, 0, sizeof(test.u.val_unique_id) );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res10, sizeof(res10), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DATETIME;
    memset( &test.u.val_datetime, 0, sizeof(test.u.val_datetime) );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res11, sizeof(res11), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DATETIME;
    memset( &test.u.val_datetime, 0, sizeof(test.u.val_datetime) );
    test.u.val_datetime.format = WS_DATETIME_FORMAT_LOCAL;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res11b, sizeof(res11b), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DATETIME;
    memset( &test.u.val_datetime, 0, sizeof(test.u.val_datetime) );
    test.u.val_datetime.format = WS_DATETIME_FORMAT_NONE;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res11c, sizeof(res11c), __LINE__ );

    prepare_binary_type_test( writer, NULL, &str_t, &str_ns );
    test.type = WS_XML_TEXT_TYPE_DATETIME;
    memset( &test.u.val_datetime, 0, sizeof(test.u.val_datetime) );
    test.u.val_datetime.ticks = 1;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s, WS_WRITE_REQUIRED_VALUE,
                      &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_bin( writer, res11d, sizeof(res11d), __LINE__ );

    WsFreeWriter( writer );
}

static void test_repeating_element_choice(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_a = {1, (BYTE *)"a"}, str_b = {1, (BYTE *)"b"};
    static WS_XML_STRING str_s = {1, (BYTE *)"s"}, str_t = {1, (BYTE *)"t"};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_UNION_DESCRIPTION u;
    WS_UNION_FIELD_DESCRIPTION f, f2, *fields[2];
    WS_FIELD_DESCRIPTION f_items, *fields_items[1];
    WS_STRUCT_DESCRIPTION s;
    enum choice {CHOICE_A, CHOICE_B, CHOICE_NONE};
    struct item
    {
        enum choice choice;
        union
        {
            const WCHAR *a;
            UINT32       b;
        } value;
    } items[2];
    struct test
    {
        struct item *items;
        ULONG        count;
    } test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.value           = CHOICE_A;
    f.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.field.localName = &str_a;
    f.field.ns        = &str_ns;
    f.field.type      = WS_WSZ_TYPE;
    f.field.offset    = FIELD_OFFSET(struct item, value.a);
    fields[0] = &f;

    memset( &f2, 0, sizeof(f2) );
    f2.value           = CHOICE_B;
    f2.field.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.field.localName = &str_b;
    f2.field.ns        = &str_ns;
    f2.field.type      = WS_UINT32_TYPE;
    f2.field.offset    = FIELD_OFFSET(struct item, value.b);
    fields[1] = &f2;

    memset( &u, 0, sizeof(u) );
    u.size          = sizeof(struct item);
    u.alignment     = TYPE_ALIGNMENT(struct item);
    u.fields        = fields;
    u.fieldCount    = 2;
    u.enumOffset    = FIELD_OFFSET(struct item, choice);
    u.noneEnumValue = CHOICE_NONE;

    memset( &f_items, 0, sizeof(f_items) );
    f_items.mapping         = WS_REPEATING_ELEMENT_CHOICE_FIELD_MAPPING;
    f_items.localName       = &str_t;
    f_items.ns              = &str_ns;
    f_items.type            = WS_UNION_TYPE;
    f_items.typeDescription = &u;
    f_items.countOffset     = FIELD_OFFSET(struct test, count);
    fields_items[0] = &f_items;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields_items;
    s.fieldCount    = 1;
    s.typeLocalName = &str_s;
    s.typeNs        = &str_ns;

    items[0].choice  = CHOICE_A;
    items[0].value.a = L"test";
    items[1].choice  = CHOICE_B;
    items[1].value.b = 1;
    test.items = items;
    test.count = 2;

    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t><a>test</a><b>1</b></t>", __LINE__ );

    items[0].choice = CHOICE_NONE;
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    test.count = 0;
    hr = set_output( writer );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t/>", __LINE__ );

    WsFreeWriter( writer );
}

static const struct stream_test
{
    ULONG min_size;
    ULONG ret_size;
}
stream_tests[] =
{
    { 0, 4 },
    { 1, 4 },
    { 4, 4 },
    { 5, 4 },
};

static CALLBACK HRESULT write_callback( void *state, const WS_BYTES *buf, ULONG count,
                                        const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    ULONG i = *(ULONG *)state;
    ok( buf->length == stream_tests[i].ret_size, "%lu: got %lu\n", i, buf->length );
    ok( !memcmp( buf->bytes, "<t/>", stream_tests[i].ret_size ), "%lu: wrong data\n", i );
    ok( count == 1, "%lu: got %lu\n", i, count );
    return S_OK;
}

static void test_stream_output(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_t = {1, (BYTE *)"t"};
    WS_XML_WRITER_TEXT_ENCODING text = {{WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8};
    WS_XML_WRITER_STREAM_OUTPUT stream;
    WS_XML_WRITER *writer;
    HRESULT hr;
    ULONG i = 0;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFlushWriter( writer, 0, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    stream.output.outputType = WS_XML_WRITER_OUTPUT_TYPE_STREAM;
    stream.writeCallback      = write_callback;
    stream.writeCallbackState = &i;
    hr = WsSetOutput( writer, &text.encoding, &stream.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutput( writer, &text.encoding, &stream.output, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsFlushWriter( writer, 0, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE(stream_tests); i++)
    {
        stream.writeCallbackState = &i;
        hr = WsSetOutput( writer, &text.encoding, &stream.output, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteStartElement( writer, NULL, &str_t, &str_ns, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
        hr = WsFlushWriter( writer, stream_tests[i].min_size, NULL, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
    }

    WsFreeWriter( writer );
}

static void test_description_type(void)
{
    static WS_XML_STRING ns = {0, NULL}, localname = {1, (BYTE *)"t"}, val = {3, (BYTE *)"val"};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_FIELD_DESCRIPTION f, f2, *fields[2];
    WS_STRUCT_DESCRIPTION s;
    struct test
    {
        const WS_STRUCT_DESCRIPTION *desc;
        INT32                        val;
    } test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TYPE_ATTRIBUTE_FIELD_MAPPING;
    f.type    = WS_DESCRIPTION_TYPE;
    fields[0] = &f;

    memset( &f2, 0, sizeof(f2) );
    f2.mapping   = WS_ATTRIBUTE_FIELD_MAPPING;
    f2.localName = &val;
    f2.ns        = &ns;
    f2.offset    = FIELD_OFFSET(struct test, val);
    f2.type      = WS_INT32_TYPE;
    fields[1] = &f2;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 2;
    s.typeLocalName = &localname;
    s.typeNs        = &ns;

    test.desc = &s;
    test.val  = -1;

    hr = set_output( writer );
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_VALUE, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output( writer, "<t val=\"-1\"/>", __LINE__ );

    WsFreeWriter( writer );
}

START_TEST(writer)
{
    test_WsCreateWriter();
    test_WsCreateXmlBuffer();
    test_WsSetOutput();
    test_WsSetOutputToBuffer();
    test_WsWriteStartElement();
    test_WsWriteStartAttribute();
    test_WsWriteType();
    test_basic_type();
    test_simple_struct_type();
    test_WsWriteElement();
    test_WsWriteValue();
    test_WsWriteAttribute();
    test_WsWriteStartCData();
    test_WsWriteXmlnsAttribute();
    test_WsGetPrefixFromNamespace();
    test_complex_struct_type();
    test_WsMoveWriter();
    test_WsGetWriterPosition();
    test_WsSetWriterPosition();
    test_WsWriteXmlBuffer();
    test_WsWriteNode();
    test_WsCopyNode();
    test_text_types();
    test_double();
    test_field_options();
    test_WsWriteText();
    test_WsWriteArray();
    test_escapes();
    test_write_option();
    test_datetime();
    test_repeating_element();
    test_WsWriteQualifiedName();
    test_WsWriteBytes();
    test_WsWriteChars();
    test_WsWriteCharsUtf8();
    test_binary_encoding();
    test_namespaces();
    test_dictionary();
    test_union_type();
    test_text_types_binary();
    test_repeating_element_choice();
    test_stream_output();
    test_description_type();
}
