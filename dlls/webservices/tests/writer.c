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

    bytes.length = 0xdeadbeef;
    bytes.bytes = (BYTE *)0xdeadbeef;
    size = sizeof(buffers);
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !bytes.length, "got %u\n", bytes.length );
    ok( bytes.bytes != NULL, "got %p\n", bytes.bytes );

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

static void check_output( WS_XML_WRITER *writer, const char *expected, unsigned int line )
{
    WS_BYTES bytes;
    ULONG size = sizeof(bytes);
    int len = strlen( expected );
    HRESULT hr;

    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "%u: got %08x\n", line, hr );
    ok( bytes.length == len, "%u: got %u expected %u\n", line, bytes.length, len );
    if (bytes.length != len) return;
    ok( !memcmp( bytes.bytes, expected, len ), "%u: got %s expected %s\n", line, bytes.bytes, expected );
}

static void test_WsWriteStartElement(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, ns = {2, (BYTE *)"ns"};
    WS_XML_STRING localname = {1, (BYTE *)"a"}, localname2 =  {1, (BYTE *)"b"};

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( NULL, &prefix, &localname, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* first call to WsWriteStartElement doesn't output anything */
    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    /* two ways to close an element */
    hr = WsWriteEndStartElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\">", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"></p:a>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:a xmlns:p=\"ns\"/>", __LINE__ );

    /* nested elements */
    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<a xmlns=\"ns\">", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<a xmlns=\"ns\"><b/>", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<a xmlns=\"ns\"><b/></a>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteStartAttribute(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE *)"p"}, localname = {3, (BYTE *)"str"}, ns = {2, (BYTE *)"ns"};
    WS_XML_UTF8_TEXT text;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartAttribute( NULL, &prefix, &localname, &ns, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* WsWriteStartAttribute doesn't output anything */
    localname.length = 3;
    localname.bytes  = (BYTE *)"len";
    hr = WsWriteStartAttribute( writer, &prefix, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    text.text.textType = WS_XML_TEXT_TYPE_UTF8;
    text.value.length  = 1;
    text.value.bytes   = (BYTE *)"0";
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    /* WsWriteEndAttribute doesn't output anything */
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str p:len=\"0\" xmlns:p=\"ns\"/>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_WsWriteType(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING prefix = {1, (BYTE*)"p"}, localname = {3, (BYTE *)"str"}, ns = {2, (BYTE *)"ns"};
    const WCHAR *val_str;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    val_str = testW;
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* required value */
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, NULL, sizeof(testW), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_VALUE, testW, sizeof(testW), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* required pointer */
    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, NULL, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(WCHAR **), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test</p:str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    val_str = testW;
    hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str p:str=\"test\" xmlns:p=\"ns\"/>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    val_str = testW;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<p:str xmlns:p=\"ns\">test</p:str>", __LINE__ );

    WsFreeWriter( writer );
}

static void test_basic_type(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
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

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    /* element content type mapping */
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_VALUE, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* element type mapping is the same as element content type mapping for basic types */
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_VALUE, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* attribute type mapping */
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, tests[i].type, NULL,
                          WS_WRITE_REQUIRED_VALUE, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        check_output( writer, tests[i].result2, __LINE__ );
    }

    WsFreeWriter( writer );
}

static void test_simple_struct_type(void)
{
    static const WCHAR valueW[] = {'v','a','l','u','e',0};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_XML_STRING localname = {6, (BYTE *)"struct"}, ns = {0, NULL};
    struct test
    {
        const WCHAR *field;
    } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;

    test = HeapAlloc( GetProcessHeap(), 0, sizeof(*test) );
    test->field  = valueW;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, NULL,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<struct>value</struct>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteType( writer, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<struct>value</struct>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteType( writer, WS_ATTRIBUTE_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<struct struct=\"value\"/>", __LINE__ );

    HeapFree( GetProcessHeap(), 0, test );
    WsFreeWriter( writer );
}

static void test_WsWriteElement(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_ELEMENT_DESCRIPTION desc;
    WS_XML_STRING localname = {3, (BYTE *)"str"}, ns = {0, NULL};
    struct test { const WCHAR *str; } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

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

    test = HeapAlloc( GetProcessHeap(), 0, sizeof(*test) );
    test->str = testW;
    hr = WsWriteElement( NULL, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteElement( writer, NULL, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<str>test</str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<str><str>test</str>", __LINE__ );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    /* attribute field mapping */
    f.mapping = WS_ATTRIBUTE_FIELD_MAPPING;

    /* requires localName and ns to be set */
    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    f.localName = &localname;
    f.ns        = &ns;
    hr = WsWriteElement( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<str str=\"test\"/>", __LINE__ );

    HeapFree( GetProcessHeap(), 0, test );
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

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteValue( NULL, tests[0].type, &tests[0].val, tests[0].size, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteValue( writer, tests[0].type, &tests[0].val, tests[0].size, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* zero size */
    hr = WsWriteValue( writer, tests[0].type, &tests[0].val, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* NULL value */
    hr = WsWriteValue( writer, tests[0].type, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* element type mapping */
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteValue( writer, tests[i].type, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        check_output( writer, tests[i].result, __LINE__ );
    }

    /* attribute type mapping */
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_output( writer );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteStartAttribute( writer, NULL, &localname, &ns, FALSE, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteValue( writer, tests[i].type, &tests[i].val, tests[i].size, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsWriteEndAttribute( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsWriteEndElement( writer, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        check_output( writer, tests[i].result2, __LINE__ );
    }

    WsFreeWriter( writer );
}

static void test_WsWriteAttribute(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_ATTRIBUTE_DESCRIPTION desc;
    WS_XML_STRING localname = {3, (BYTE *)"str"}, ns = {0, NULL};
    struct test { const WCHAR *str; } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

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

    test = HeapAlloc( GetProcessHeap(), 0, sizeof(*test) );
    test->str = testW;
    hr = WsWriteAttribute( NULL, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteAttribute( writer, NULL, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteAttribute( writer, &desc, WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<str str=\"test\"/>", __LINE__ );

    HeapFree( GetProcessHeap(), 0, test );
    WsFreeWriter( writer );
}

static void test_WsWriteStartCData(void)
{
    HRESULT hr;
    WS_XML_WRITER *writer;
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    WS_XML_UTF8_TEXT text;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndCData( writer, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "", __LINE__ );

    hr = WsWriteStartCData( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<t><![CDATA[", __LINE__ );

    text.text.textType = WS_XML_TEXT_TYPE_UTF8;
    text.value.bytes = (BYTE *)"<data>";
    text.value.length = 6;
    hr = WsWriteText( writer, &text.text, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<t><![CDATA[<data>", __LINE__ );

    hr = WsWriteEndCData( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output( writer, "<t><![CDATA[<data>]]>", __LINE__ );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteXmlBuffer( writer, buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &bytes, 0, sizeof(bytes) );
    hr = WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &bytes, size, NULL );
    ok( hr == S_OK, "%u: got %08x\n", line, hr );
    ok( bytes.length == len, "%u: got %u expected %u\n", line, bytes.length, len );
    if (bytes.length != len) return;
    ok( !memcmp( bytes.bytes, expected, len ), "%u: got %s expected %s\n", line, bytes.bytes, expected );

    WsFreeWriter( writer );
}

static void prepare_xmlns_test( WS_XML_WRITER *writer, WS_HEAP **heap, WS_XML_BUFFER **buffer )
{
    WS_XML_STRING prefix = {6, (BYTE *)"prefix"}, localname = {1, (BYTE *)"t"}, ns = {2, (BYTE *)"ns"};
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( *heap, NULL, 0, buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, *buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteXmlnsAttribute( NULL, NULL, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeHeap( heap );

    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, NULL, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeHeap( heap );

    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, NULL, FALSE, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteXmlnsAttribute( writer, NULL, &ns, FALSE, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    WsFreeHeap( heap );

    /* no prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, NULL, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix=\"ns\" xmlns=\"ns2\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2=\"ns2\" xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* implicitly set element prefix namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* explicitly set element prefix namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix='ns'/>", __LINE__ );
    WsFreeHeap( heap );

    /* repeated calls, same namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2=\"ns\" xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* repeated calls, different namespace */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, FALSE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    WsFreeHeap( heap );

    /* single quotes */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2='ns' xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* different namespace, different prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t xmlns:prefix2='ns2' xmlns:prefix=\"ns\"/>", __LINE__ );
    WsFreeHeap( heap );

    /* different namespace, same prefix */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    WsFreeHeap( heap );

    /* regular attribute */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteStartAttribute( writer, &xmlns, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    WsFreeHeap( heap );

    /* attribute order */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartAttribute( writer, &prefix, &attr, &ns, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndAttribute( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, "<prefix:t prefix:attr='' xmlns:prefix='ns' xmlns:prefix2='ns2'/>", __LINE__ );
    WsFreeHeap( heap );

    /* scope */
    prepare_xmlns_test( writer, &heap, &buffer );
    hr = WsWriteXmlnsAttribute( writer, &prefix2, &ns2, TRUE, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartElement( writer, &prefix2, &localname, &ns2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndStartElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
}

static void test_WsGetPrefixFromNamespace(void)
{
    const WS_XML_STRING p = {1, (BYTE *)"p"}, localname = {1, (BYTE *)"t"}, *prefix;
    const WS_XML_STRING ns = {2, (BYTE *)"ns"}, ns2 = {3, (BYTE *)"ns2"};
    WS_XML_WRITER *writer;
    HRESULT hr;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetPrefixFromNamespace( NULL, NULL, FALSE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetPrefixFromNamespace( NULL, NULL, FALSE, &prefix, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetPrefixFromNamespace( writer, NULL, FALSE, &prefix, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* element must be committed */
    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    /* but writer can't be positioned on end element node */
    hr = set_output( writer );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteStartElement( writer, &p, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    /* required = TRUE */
    prefix = NULL;
    prepare_prefix_test( writer );
    hr = WsGetPrefixFromNamespace( writer, &ns, TRUE, &prefix, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( prefix != NULL, "prefix not set\n" );
    if (prefix)
    {
        ok( prefix->length == 1, "got %u\n", prefix->length );
        ok( !memcmp( prefix->bytes, "p", 1 ), "wrong prefix\n" );
    }

    prefix = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetPrefixFromNamespace( writer, &ns2, TRUE, &prefix, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( prefix == (const WS_XML_STRING *)0xdeadbeef, "prefix set\n" );

    /* required = FALSE */
    prefix = NULL;
    prepare_prefix_test( writer );
    hr = WsGetPrefixFromNamespace( writer, &ns, FALSE, &prefix, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( prefix != NULL, "prefix not set\n" );
    if (prefix)
    {
        ok( prefix->length == 1, "got %u\n", prefix->length );
        ok( !memcmp( prefix->bytes, "p", 1 ), "wrong prefix\n" );
    }

    prefix = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetPrefixFromNamespace( writer, &ns2, FALSE, &prefix, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );
    ok( prefix == NULL, "prefix not set\n" );

    WsFreeWriter( writer );
}

static void test_complex_struct_type(void)
{
    static const char expected[] =
        "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">"
        "<o:services o:GenerationTime=\"2015-09-03T18:47:54\"/>"
        "</o:OfficeConfig>";
    static const WCHAR timestampW[] =
        {'2','0','1','5','-','0','9','-','0','3','T','1','8',':','4','7',':','5','4',0};
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
    WS_FIELD_DESCRIPTION f, f2, *fields[1], *fields2[1];
    struct services
    {
        const WCHAR *generationtime;
    };
    struct officeconfig
    {
        struct services *services;
    } *test;

    hr = WsCreateWriter( NULL, 0, &writer, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, &prefix, &str_officeconfig, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping         = WS_ATTRIBUTE_FIELD_MAPPING;
    f2.localName       = &str_generationtime;
    f2.ns              = &ns;
    f2.type            = WS_WSZ_TYPE;
    f2.options         = WS_FIELD_OPTIONAL;
    fields2[0] = &f2;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(*test->services);
    s2.alignment     = 4;
    s2.fields        = fields2;
    s2.fieldCount    = 1;
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
    test = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
    test->services = (struct services *)(test + 1);
    test->services->generationtime = timestampW;
    hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                      WS_WRITE_REQUIRED_POINTER, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    check_output_buffer( buffer, expected, __LINE__ );

    HeapFree( GetProcessHeap(), 0, test );
    WsFreeWriter( writer );
    WsFreeHeap( heap );
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
}
