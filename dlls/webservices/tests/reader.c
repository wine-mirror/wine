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

static const char data1[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

static void test_WsCreateError(void)
{
    HRESULT hr;
    WS_ERROR *error;
    WS_ERROR_PROPERTY prop;
    ULONG size, code, count;
    LANGID langid;

    hr = WsCreateError( NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    error = NULL;
    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( error != NULL, "error not set\n" );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %u\n", count );

    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !code, "got %u\n", code );

    code = 0xdeadbeef;
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( code == 0xdeadbeef, "got %u\n", code );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( langid == GetUserDefaultUILanguage(), "got %u\n", langid );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID + 1, &count, size );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( count == 0xdeadbeef, "got %u\n", count );
    WsFreeError( error );

    count = 1;
    prop.id = WS_ERROR_PROPERTY_STRING_COUNT;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    code = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE;
    prop.value = &code;
    prop.valueSize = sizeof(code);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    prop.id = WS_ERROR_PROPERTY_LANGID;
    prop.value = &langid;
    prop.valueSize = sizeof(langid);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %08x\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );
    WsFreeError( error );

    count = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_LANGID + 1;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
}

static void test_WsCreateHeap(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_HEAP_PROPERTY prop;
    SIZE_T max, trim, requested, actual;
    ULONG size;

    hr = WsCreateHeap( 0, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    heap = NULL;
    hr = WsCreateHeap( 0, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( heap != NULL, "heap not set\n" );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    heap = NULL;
    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( heap != NULL, "heap not set\n" );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    max = 0xdeadbeef;
    size = sizeof(max);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_MAX_SIZE, &max, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max == 1 << 16, "got %u\n", (ULONG)max );

    trim = 0xdeadbeef;
    size = sizeof(trim);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_TRIM_SIZE, &trim, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( trim == 1 << 6, "got %u\n", (ULONG)trim );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !requested, "got %u\n", (ULONG)requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !actual, "got %u\n", (ULONG)actual );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE + 1, &actual, size, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( actual == 0xdeadbeef, "got %u\n", (ULONG)actual );
    WsFreeHeap( heap );

    max = 1 << 16;
    prop.id = WS_HEAP_PROPERTY_MAX_SIZE;
    prop.value = &max;
    prop.valueSize = sizeof(max);
    hr = WsCreateHeap( 1 << 16, 1 << 6, &prop, 1, &heap, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 1, &heap, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
}

static HRESULT set_input( WS_XML_READER *reader, const char *data, ULONG size )
{
    WS_XML_READER_TEXT_ENCODING encoding;
    WS_XML_READER_BUFFER_INPUT input;

    encoding.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    encoding.charSet               = WS_CHARSET_UTF8;

    input.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    input.encodedData     = (void *)data;
    input.encodedDataSize = size;

    return WsSetInput( reader, (WS_XML_READER_ENCODING *)&encoding,
                       (WS_XML_READER_INPUT *)&input, NULL, 0, NULL );
}

static void test_WsCreateReader(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_READER_PROPERTY prop;
    ULONG size, max_depth, max_attrs, trim_size, buffer_size, max_ns;
    BOOL allow_fragment, read_decl, in_attr;
    ULONGLONG row, column;
    WS_CHARSET charset;

    hr = WsCreateReader( NULL, 0, NULL, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    reader = NULL;
    hr = WsCreateReader( NULL, 0, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );
    ok( reader != NULL, "reader not set\n" );

    /* can't retrieve properties before input is set */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    ok( max_depth == 0xdeadbeef, "max_depth set\n" );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    /* check some defaults */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 32, "got %u\n", max_depth );

    allow_fragment = TRUE;
    size = sizeof(allow_fragment);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_ALLOW_FRAGMENT, &allow_fragment, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !allow_fragment, "got %d\n", allow_fragment );

    max_attrs = 0xdeadbeef;
    size = sizeof(max_attrs);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_attrs == 128, "got %u\n", max_attrs );

    read_decl = FALSE;
    size = sizeof(read_decl);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_READ_DECLARATION, &read_decl, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( read_decl, "got %u\n", read_decl );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    size = sizeof(trim_size);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_UTF8_TRIM_SIZE, &trim_size, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    size = sizeof(buffer_size);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_STREAM_BUFFER_SIZE, &buffer_size, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    max_ns = 0xdeadbeef;
    size = sizeof(max_ns);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_NAMESPACES, &max_ns, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_ns == 32, "got %u\n", max_ns );
    WsFreeReader( reader );

    /* change a property */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsCreateReader( &prop, 1, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );
    WsFreeReader( reader );

    /* show that some properties are read-only */
    row = 1;
    prop.id = WS_XML_READER_PROPERTY_ROW;
    prop.value = &row;
    prop.valueSize = sizeof(row);
    hr = WsCreateReader( &prop, 1, &reader, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    column = 1;
    prop.id = WS_XML_READER_PROPERTY_COLUMN;
    prop.value = &column;
    prop.valueSize = sizeof(column);
    hr = WsCreateReader( &prop, 1, &reader, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    in_attr = TRUE;
    prop.id = WS_XML_READER_PROPERTY_IN_ATTRIBUTE;
    prop.value = &in_attr;
    prop.valueSize = sizeof(in_attr);
    hr = WsCreateReader( &prop, 1, &reader, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
}

static void test_WsSetInput(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_READER_PROPERTY prop;
    WS_XML_READER_TEXT_ENCODING enc;
    WS_XML_READER_BUFFER_INPUT input;
    WS_CHARSET charset;
    const WS_XML_NODE *node;
    ULONG size, max_depth;

    hr = WsCreateReader( NULL, 0, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetInput( NULL, NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_UTF8;

    input.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    input.encodedData     = (void *)data1;
    input.encodedDataSize = sizeof(data1) - 1;

    hr = WsSetInput( reader, (WS_XML_READER_ENCODING *)&enc, (WS_XML_READER_INPUT *)&input, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* multiple calls are allowed */
    hr = WsSetInput( reader, (WS_XML_READER_ENCODING *)&enc, (WS_XML_READER_INPUT *)&input, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* charset is detected by WsSetInput */
    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_AUTO;
    hr = WsSetInput( reader, (WS_XML_READER_ENCODING *)&enc, (WS_XML_READER_INPUT *)&input, NULL, 0, NULL );
    todo_wine ok( hr == S_OK, "got %08x\n", hr );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_UTF8;

    /* reader properties can be set with WsSetInput */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetInput( reader, (WS_XML_READER_ENCODING *)&enc, (WS_XML_READER_INPUT *)&input, &prop, 1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );
    WsFreeReader( reader );
}

static void test_WsFillReader(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;

    hr = WsCreateReader( NULL, 0, &reader, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( NULL, sizeof(data1) - 1, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* min_size larger than input size */
    hr = WsFillReader( reader, sizeof(data1), NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeReader( reader );
}

START_TEST(reader)
{
    test_WsCreateError();
    test_WsCreateHeap();
    test_WsCreateReader();
    test_WsSetInput();
    test_WsFillReader();
}
