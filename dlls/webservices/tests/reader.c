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
#include "rpc.h"
#include "webservices.h"
#include "wine/test.h"

static const GUID guid_null;

static const char data1[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

static const char data2[] =
    {0xef,0xbb,0xbf,'<','t','e','x','t','>','t','e','s','t','<','/','t','e','x','t','>',0};

static const char data3[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?> "
    "<text>test</TEXT>";

static const char data4[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
    "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">\r\n"
    " <o:services o:GenerationTime=\"2015-09-03T18:47:54\">\r\n"
    "  <!--Build: 16.0.6202.6852-->\r\n"
    "  <o:default>\r\n"
    "   <o:ticket o:headerName=\"Authorization\" o:headerValue=\"{}\" />\r\n"
    "  </o:default>\r\n"
    "  <o:service o:name=\"LiveOAuthLoginStart\">\r\n"
    "   <o:url>https://login.[Live.WebHost]/oauth20_authorize.srf</o:url>\r\n"
    "  </o:service>\r\n"
    "</o:services>\r\n"
    "</o:OfficeConfig>\r\n";

static const char data5[] =
    "</text>";

static const char data6[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<text attr= \"value\" attr2='value2'>test</text>";

static const char data7[] =
    "<!-- comment -->";

static const char data8[] =
    "<node1><node2>test</node2></node1>";

static const char data9[] =
    "<text xml:attr=\"value\">test</text>";

static const char data10[] =
    "<a></b>";

static void test_WsCreateError(void)
{
    HRESULT hr;
    WS_ERROR *error;
    WS_ERROR_PROPERTY prop;
    ULONG size, code, count;
    LANGID langid;

    hr = WsCreateError( NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    error = NULL;
    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( error != NULL, "error not set\n" );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !code, "got %lu\n", code );

    code = 0xdeadbeef;
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( code == 0xdeadbeef, "got %lu\n", code );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( langid == GetUserDefaultUILanguage(), "got %u\n", langid );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID + 1, &count, size );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    ok( count == 0xdeadbeef, "got %lu\n", count );
    WsFreeError( error );

    count = 1;
    prop.id = WS_ERROR_PROPERTY_STRING_COUNT;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    code = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE;
    prop.value = &code;
    prop.valueSize = sizeof(code);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    prop.id = WS_ERROR_PROPERTY_LANGID;
    prop.value = &langid;
    prop.valueSize = sizeof(langid);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );
    WsFreeError( error );

    count = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_LANGID + 1;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
}

static void test_WsCreateHeap(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_HEAP_PROPERTY prop;
    SIZE_T max, trim, requested, actual;
    ULONG size;

    hr = WsCreateHeap( 0, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    heap = NULL;
    hr = WsCreateHeap( 0, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( heap != NULL, "heap not set\n" );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    heap = NULL;
    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( heap != NULL, "heap not set\n" );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    max = 0xdeadbeef;
    size = sizeof(max);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_MAX_SIZE, &max, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max == 1 << 16, "got %Iu\n", max );

    trim = 0xdeadbeef;
    size = sizeof(trim);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_TRIM_SIZE, &trim, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( trim == 1 << 6, "got %Iu\n", trim );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !requested, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !actual, "got %Iu\n", actual );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE + 1, &actual, size, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    ok( actual == 0xdeadbeef, "got %Iu\n", actual );
    WsFreeHeap( heap );

    max = 1 << 16;
    prop.id = WS_HEAP_PROPERTY_MAX_SIZE;
    prop.value = &max;
    prop.valueSize = sizeof(max);
    hr = WsCreateHeap( 1 << 16, 1 << 6, &prop, 1, &heap, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 1 << 6, NULL, 1, &heap, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
}

static HRESULT set_input( WS_XML_READER *reader, const char *data, ULONG size )
{
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}, WS_CHARSET_AUTO};
    WS_XML_READER_BUFFER_INPUT buf;

    buf.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    buf.encodedData     = (void *)data;
    buf.encodedDataSize = size;
    return WsSetInput( reader, &text.encoding, &buf.input, NULL, 0, NULL );
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

    hr = WsCreateReader( NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    reader = NULL;
    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( reader != NULL, "reader not set\n" );

    /* can't retrieve properties before input is set */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    ok( max_depth == 0xdeadbeef, "max_depth set\n" );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* check some defaults */
    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 32, "got %lu\n", max_depth );

    allow_fragment = TRUE;
    size = sizeof(allow_fragment);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_ALLOW_FRAGMENT, &allow_fragment, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !allow_fragment, "got %d\n", allow_fragment );

    max_attrs = 0xdeadbeef;
    size = sizeof(max_attrs);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_attrs == 128, "got %lu\n", max_attrs );

    read_decl = FALSE;
    size = sizeof(read_decl);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_READ_DECLARATION, &read_decl, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( read_decl, "got %u\n", read_decl );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    size = sizeof(trim_size);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_UTF8_TRIM_SIZE, &trim_size, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    size = sizeof(buffer_size);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_STREAM_BUFFER_SIZE, &buffer_size, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_ns = 0xdeadbeef;
    size = sizeof(max_ns);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_NAMESPACES, &max_ns, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_ns == 32, "got %lu\n", max_ns );
    WsFreeReader( reader );

    /* change a property */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );
    WsFreeReader( reader );

    /* show that some properties are read-only */
    row = 1;
    prop.id = WS_XML_READER_PROPERTY_ROW;
    prop.value = &row;
    prop.valueSize = sizeof(row);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    column = 1;
    prop.id = WS_XML_READER_PROPERTY_COLUMN;
    prop.value = &column;
    prop.valueSize = sizeof(column);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    in_attr = TRUE;
    prop.id = WS_XML_READER_PROPERTY_IN_ATTRIBUTE;
    prop.value = &in_attr;
    prop.valueSize = sizeof(in_attr);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
}

static void test_WsSetInput(void)
{
    static char test1[] = {0xef,0xbb,0xbf,'<','a','/','>'};
    static char test2[] = {'<','a','/','>'};
    static char test3[] = {'<','!','-','-'};
    static char test4[] = {'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"',
                           ' ','e','n','c','o','d','i','n','g','=','"','u','t','f','-','8','"','?','>'};
    static char test5[] = {'<','?','x','m','l',' ','e','n','c','o','d','i','n','g','=',
                           '"','u','t','f','-','8','"','?','>'};
    static char test6[] = {'<','?','x','m','l'};
    static char test7[] = {'<','?','y','m','l'};
    static char test8[] = {'<','?'};
    static char test9[] = {'<','!'};
    static char test10[] = {0xff,0xfe,'<',0,'a',0,'/',0,'>',0};
    static char test11[] = {'<',0,'a',0,'/',0,'>',0};
    static char test12[] = {'<',0,'!',0,'-',0,'-',0};
    static char test13[] = {'<',0,'?',0};
    static char test14[] = {'a','b'};
    static char test15[] = {'a','b','c'};
    static char test16[] = {'a',0};
    static char test17[] = {'a',0,'b',0};
    static char test18[] = {'<',0,'a',0,'b',0};
    static char test19[] = {'<',0,'a',0};
    static char test20[] = {0,'a','b'};
    static char test21[] = {0,0};
    static char test22[] = {0,0,0};
    static char test23[] = {'<',0,'?',0,'x',0,'m',0,'l',0};
    static char test24[] = {'<',0,'a',0,'>',0,'b',0,'<',0,'/',0,'>',0};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_READER_PROPERTY prop;
    WS_XML_READER_TEXT_ENCODING enc;
    WS_XML_READER_BUFFER_INPUT input;
    WS_XML_TEXT_NODE *text;
    WS_XML_UTF8_TEXT *utf8;
    WS_CHARSET charset;
    const WS_XML_NODE *node;
    ULONG i, size, max_depth;
    BOOL found;
    static const struct
    {
        void       *data;
        ULONG       size;
        HRESULT     hr;
        WS_CHARSET  charset;
        int         todo;
    }
    tests[] =
    {
        { test1, sizeof(test1), S_OK, WS_CHARSET_UTF8 },
        { test2, sizeof(test2), S_OK, WS_CHARSET_UTF8 },
        { test3, sizeof(test3), S_OK, WS_CHARSET_UTF8 },
        { test4, sizeof(test4), S_OK, WS_CHARSET_UTF8 },
        { test5, sizeof(test5), WS_E_INVALID_FORMAT, 0, 1 },
        { test6, sizeof(test6), WS_E_INVALID_FORMAT, 0, 1 },
        { test7, sizeof(test7), WS_E_INVALID_FORMAT, 0, 1 },
        { test8, sizeof(test8), WS_E_INVALID_FORMAT, 0 },
        { test9, sizeof(test9), WS_E_INVALID_FORMAT, 0 },
        { test10, sizeof(test10), S_OK, WS_CHARSET_UTF16LE },
        { test11, sizeof(test11), S_OK, WS_CHARSET_UTF16LE },
        { test12, sizeof(test12), S_OK, WS_CHARSET_UTF16LE },
        { test13, sizeof(test13), WS_E_INVALID_FORMAT, 0, 1 },
        { test14, sizeof(test14), WS_E_INVALID_FORMAT, 0 },
        { test15, sizeof(test15), S_OK, WS_CHARSET_UTF8 },
        { test16, sizeof(test16), WS_E_INVALID_FORMAT, 0 },
        { test17, sizeof(test17), S_OK, WS_CHARSET_UTF8 },
        { test18, sizeof(test18), S_OK, WS_CHARSET_UTF16LE },
        { test19, sizeof(test19), S_OK, WS_CHARSET_UTF16LE },
        { test20, sizeof(test20), S_OK, WS_CHARSET_UTF8 },
        { test21, sizeof(test21), WS_E_INVALID_FORMAT, 0 },
        { test22, sizeof(test22), S_OK, WS_CHARSET_UTF8 },
        { test23, sizeof(test23), WS_E_INVALID_FORMAT, 0, 1 },
    };

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInput( NULL, NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_UTF8;

    input.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    input.encodedData     = (void *)data1;
    input.encodedDataSize = sizeof(data1) - 1;

    hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* multiple calls are allowed */
    hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* charset is detected by WsSetInput */
    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_AUTO;

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        input.encodedData     = tests[i].data;
        input.encodedDataSize = tests[i].size;
        hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        charset = 0xdeadbeef;
        size = sizeof(charset);
        hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
        todo_wine_if (tests[i].todo)
        {
            ok( hr == tests[i].hr, "%lu: got %#lx expected %#lx\n", i, hr, tests[i].hr );
            if (hr == S_OK)
                ok( charset == tests[i].charset, "%lu: got %u expected %u\n", i, charset, tests[i].charset );
        }
    }

    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_UTF8;

    /* reader properties can be set with WsSetInput */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetInput( reader, &enc.encoding, &input.input, &prop, 1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );

    /* show that the reader converts text to UTF-8 */
    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_UTF16LE;
    input.encodedData     = (void *)test24;
    input.encodedDataSize = sizeof(test24);
    hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr == S_OK)
    {
        ok( found == TRUE, "got %d\n", found );

        hr = WsReadStartElement( reader, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsGetReaderNode( reader, &node, NULL );
        ok( hr == S_OK, "got %#lx\n", hr );
        text = (WS_XML_TEXT_NODE *)node;
        ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
        ok( text->text != NULL, "text not set\n" );
        utf8 = (WS_XML_UTF8_TEXT *)text->text;
        ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
        ok( utf8->value.length == 1, "got %lu\n", utf8->value.length );
        ok( utf8->value.bytes[0] == 'b', "wrong data\n" );
    }
    WsFreeReader( reader );
}

static void test_WsSetInputToBuffer(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_BUFFER *buffer;
    WS_XML_READER *reader;
    WS_XML_READER_PROPERTY prop;
    const WS_XML_NODE *node;
    ULONG size, max_depth;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* multiple calls are allowed */
    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* reader properties can be set with WsSetInputToBuffer */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetInputToBuffer( reader, buffer, &prop, 1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( max_depth == 16, "got %lu\n", max_depth );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsFillReader(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;

    /* what happens of we don't call WsFillReader? */
    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( NULL, sizeof(data1) - 1, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* min_size larger than input size */
    hr = WsFillReader( reader, sizeof(data1), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeReader( reader );
}

static void test_WsReadToStartElement(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node, *node2;
    int found;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == FALSE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        if (elem->prefix)
        {
            ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
        }
        ok( elem->localName != NULL, "localName not set\n" );
        if (elem->localName)
        {
            ok( elem->localName->length == 4, "got %lu\n", elem->localName->length );
            ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        }
        ok( elem->ns != NULL, "ns not set\n" );
        if (elem->ns)
        {
            ok( !elem->ns->length, "got %lu\n", elem->ns->length );
        }
        ok( !elem->attributeCount, "got %lu\n", elem->attributeCount );
        ok( elem->attributes == NULL, "attributes set\n" );
        ok( !elem->isEmpty, "isEmpty not zero\n" );
    }

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    node2 = NULL;
    hr = WsGetReaderNode( reader, &node2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node2 == node, "different node\n" );

    hr = set_input( reader, data3, sizeof(data3) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data3) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->localName != NULL, "localName not set\n" );
        if (elem->localName)
        {
            ok( elem->localName->length == 4, "got %lu\n", elem->localName->length );
            ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        }
    }

    hr = set_input( reader, data4, sizeof(data4) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data4) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );
    WsFreeReader( reader );
}

static void test_WsReadStartElement(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node, *node2;
    int found;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsReadStartElement( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
            ok( utf8->value.length == 4, "got %lu\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "test", 4 ), "wrong data\n" );
        }
    }

    hr = WsReadStartElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    node2 = NULL;
    hr = WsGetReaderNode( reader, &node2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node2 == node, "different node\n" );

    hr = set_input( reader, data8, sizeof(data8) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data8) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
        ok( !memcmp( elem->localName->bytes, "node1", 5), "wrong name\n" );
    }

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
        ok( !memcmp( elem->localName->bytes, "node2", 5), "wrong name\n" );
    }

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* WsReadEndElement advances reader to EOF */
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, data3, sizeof(data3) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
            ok( utf8->value.length == 4, "got %lu\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "test", 4 ), "wrong data\n" );
        }
    }

    hr = set_input( reader, " <text>test</text>", sizeof(" <text>test</text>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
            ok( utf8->value.length == 4, "got %lu\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "test", 4 ), "wrong data\n" );
        }
    }

    WsFreeReader( reader );
}

static void test_WsReadEndElement(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    int found;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* WsReadEndElement advances reader to EOF */
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, data5, sizeof(data5) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data5) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, data10, sizeof(data10) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data10) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, "<a></A>", sizeof("<a></A>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof("<a></a>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, "<a></a>", sizeof("<a></a>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof("<a></a>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, "<a/>", sizeof("<a/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof("<a/>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeReader( reader );
}

static void test_WsReadNode(void)
{
    static const char str1[] = "<a>";
    static const char str2[] = "< a>";
    static const char str3[] = "<a >";
    static const char str4[] = "<<a>>";
    static const char str5[] = "<>";
    static const char str6[] = "</a>";
    static const char str7[] = " <a>";
    static const char str8[] = "<?xml>";
    static const char str9[] = "<?xml?>";
    static const char str10[] = "<?xml ?>";
    static const char str11[] = "<?xml version=\"1.0\"?>";
    static const char str12[] = "<text>test</text>";
    static const char str13[] = "<?xml version=\"1.0\"?><text>test</text>";
    static const char str14[] = "";
    static const char str15[] = "<!--";
    static const char str16[] = "<!---->";
    static const char str17[] = "<!--comment-->";
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_DICTIONARY *dict;
    const WS_XML_NODE *node;
    unsigned int i;
    int found;
    static const struct
    {
        const char      *text;
        HRESULT          hr;
        WS_XML_NODE_TYPE type;
        int              todo;
    }
    tests[] =
    {
        { str1, S_OK, WS_XML_NODE_TYPE_ELEMENT },
        { str2, WS_E_INVALID_FORMAT, 0 },
        { str3, S_OK, WS_XML_NODE_TYPE_ELEMENT },
        { str4, WS_E_INVALID_FORMAT, 0 },
        { str5, WS_E_INVALID_FORMAT, 0 },
        { str6, WS_E_INVALID_FORMAT, 0 },
        { str7, S_OK, WS_XML_NODE_TYPE_TEXT },
        { str8, WS_E_INVALID_FORMAT, 0 },
        { str9, WS_E_INVALID_FORMAT, 0 },
        { str10, WS_E_INVALID_FORMAT, 0, 1 },
        { str11, S_OK, WS_XML_NODE_TYPE_EOF },
        { str12, S_OK, WS_XML_NODE_TYPE_ELEMENT },
        { str13, S_OK, WS_XML_NODE_TYPE_ELEMENT },
        { str14, WS_E_INVALID_FORMAT, 0, 1 },
        { str15, WS_E_INVALID_FORMAT, 0 },
        { str16, S_OK, WS_XML_NODE_TYPE_COMMENT },
        { str17, S_OK, WS_XML_NODE_TYPE_COMMENT },
    };

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_input( reader, tests[i].text, strlen(tests[i].text) );
        ok( hr == S_OK, "got %#lx\n", hr );

        hr = WsFillReader( reader, strlen(tests[i].text), NULL, NULL );
        ok( hr == S_OK, "%u: got %#lx\n", i, hr );

        hr = WsReadNode( reader, NULL );
        todo_wine_if (tests[i].todo)
            ok( hr == tests[i].hr, "%u: got %#lx\n", i, hr );
        if (hr == S_OK)
        {
            node = NULL;
            hr = WsGetReaderNode( reader, &node, NULL );
            ok( hr == S_OK, "%u: got %#lx\n", i, hr );
            ok( node != NULL, "%u: node not set\n", i );
            if (node)
            {
                todo_wine_if (tests[i].todo)
                    ok( node->nodeType == tests[i].type, "%u: got %u\n", i, node->nodeType );
            }
        }
    }

    hr = set_input( reader, data6, sizeof(data6) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data6) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;
        WS_XML_UTF8_TEXT *text;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes set\n" );
        ok( elem->localName != NULL, "localName not set\n" );
        ok( elem->localName->length == 4, "got %lu\n", elem->localName->length );
        ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( !elem->ns->length, "got %lu\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );
        ok( !elem->isEmpty, "isEmpty not zero\n" );

        attr = elem->attributes[0];
        ok( !attr->singleQuote, "got %u\n", attr->singleQuote );
        ok( !attr->isXmlNs, "got %u\n", attr->isXmlNs );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 4, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr", 4 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( !attr->ns->length, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes == NULL, "bytes set\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( text->value.length == 5, "got %lu\n", text->value.length );
        ok( !memcmp( text->value.bytes, "value", 5 ), "wrong data\n" );

        attr = elem->attributes[1];
        ok( attr->singleQuote == 1, "got %u\n", attr->singleQuote );
        ok( !attr->isXmlNs, "got %u\n", attr->isXmlNs );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 5, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr2", 5 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( !attr->ns->length, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes == NULL, "bytes set\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( text->value.length == 6, "got %lu\n", text->value.length );
        ok( !memcmp( text->value.bytes, "value2", 6 ), "wrong data\n" );
    }

    hr = set_input( reader, data7, sizeof(data7) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data7) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_COMMENT_NODE *comment = (WS_XML_COMMENT_NODE *)node;

        ok( comment->node.nodeType == WS_XML_NODE_TYPE_COMMENT, "got %u\n", comment->node.nodeType );
        ok( comment->value.length == 9, "got %lu\n", comment->value.length );
        ok( !memcmp( comment->value.bytes, " comment ", 9 ), "wrong data\n" );
    }

    dict = (WS_XML_DICTIONARY *)0xdeadbeef;
    hr = WsGetDictionary( WS_ENCODING_XML_UTF8, &dict, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict == NULL, "got %p\n", dict );

    dict = NULL;
    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_1, &dict, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict != NULL, "dict not set\n" );

    dict = NULL;
    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_SESSION_1, &dict, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict != NULL, "dict not set\n" );

    WsFreeReader( reader );
}

static HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING text = {{WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8};
    WS_XML_WRITER_BUFFER_OUTPUT buf = {{WS_XML_WRITER_OUTPUT_TYPE_BUFFER}};
    return WsSetOutput( writer, &text.encoding, &buf.output, NULL, 0, NULL );
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

static void prepare_type_test( WS_XML_READER *reader, const char *data, ULONG size )
{
    HRESULT hr;

    hr = set_input( reader, data, size );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_WsReadType(void)
{
    static const GUID guid = {0,0,0,{0,0,0,0,0,0,0,0xa1}};
    static const char utf8[] = {'<','t','>',0xe2,0x80,0x99,'<','/','t','>'};
    static const char faultxml[] =
        "<s:Body xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Fault>"
        "<faultcode>s:Client</faultcode><faultstring>Example Fault</faultstring>"
        "<faultactor>http://example.com/fault</faultactor>"
        "<detail><ErrorCode>1030</ErrorCode></detail></s:Fault></s:Body>";
    static const char faultcode[] = "Client";
    static const WCHAR faultstring[] = L"Example Fault";
    static const WCHAR faultactor[] = L"http://example.com/fault";
    static const char faultdetail[] = "<detail><ErrorCode>1030</ErrorCode></detail>";
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    enum { ONE = 1, TWO = 2 };
    WS_XML_STRING one = { 3, (BYTE *)"ONE" }, two = { 3, (BYTE *)"TWO" }, val_xmlstr, *ptr_xmlstr;
    WS_ENUM_VALUE enum_values[] = { { ONE, &one }, { TWO, &two } };
    WS_ENUM_DESCRIPTION enum_desc;
    int val_enum, *ptr_enum;
    WCHAR *val_str;
    BOOL val_bool, *ptr_bool;
    INT8 val_int8, *ptr_int8;
    INT16 val_int16, *ptr_int16;
    INT32 val_int32, *ptr_int32;
    INT64 val_int64, *ptr_int64;
    UINT8 val_uint8, *ptr_uint8;
    UINT16 val_uint16, *ptr_uint16;
    UINT32 val_uint32, *ptr_uint32;
    UINT64 val_uint64, *ptr_uint64;
    GUID val_guid, *ptr_guid;
    WS_BYTES val_bytes, *ptr_bytes;
    WS_STRING val_string, *ptr_string;
    WS_UNIQUE_ID val_id, *ptr_id;
    WS_XML_QNAME val_qname, *ptr_qname;
    WS_FAULT_DESCRIPTION fault_desc;
    WS_FAULT fault;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    prepare_type_test( reader, data2, sizeof(data2) - 1 );
    hr = WsReadType( NULL, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, NULL, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str) + 1, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    val_str = NULL;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_str != NULL, "pointer not set\n" );
    if (val_str) ok( !wcscmp( val_str, L"test" ), "wrong data\n" );

    val_bool = -1;
    prepare_type_test( reader, "<t>true</t>", sizeof("<t>true</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(BOOL), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bool == TRUE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>false</t>", sizeof("<t>false</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(BOOL), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bool == FALSE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>FALSE</t>", sizeof("<t>FALSE</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( val_bool == -1, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>1</t>", sizeof("<t>1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bool == TRUE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>2</t>", sizeof("<t>2</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( val_bool == -1, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>0</t>", sizeof("<t>0</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bool == FALSE, "got %d\n", val_bool );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_bool, sizeof(ptr_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_int8 = 0;
    prepare_type_test( reader, "<t>-128</t>", sizeof("<t>-128</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_int8 == -128, "got %d\n", val_int8 );

    prepare_type_test( reader, "<t> </t>", sizeof("<t> </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t>-</t>", sizeof("<t>-</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_int8 = -1;
    prepare_type_test( reader, "<t>-0</t>", sizeof("<t>-0</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !val_int8, "got %d\n", val_int8 );

    prepare_type_test( reader, "<t>-129</t>", sizeof("<t>-129</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_int8, sizeof(ptr_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_int16 = 0;
    prepare_type_test( reader, "<t>-32768</t>", sizeof("<t>-32768</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int16, sizeof(val_int16), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_int16 == -32768, "got %d\n", val_int16 );

    prepare_type_test( reader, "<t>-32769</t>", sizeof("<t>-32769</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int16, sizeof(val_int16), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int16, sizeof(val_int16), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_int16, sizeof(ptr_int16), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_int32 = 0;
    prepare_type_test( reader, "<t>-2147483648</t>", sizeof("<t>-2147483648</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int32, sizeof(val_int32), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_int32 == -2147483647 - 1, "got %d\n", val_int32 );

    prepare_type_test( reader, "<t>-2147483649</t>", sizeof("<t>-2147483649</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int32, sizeof(val_int32), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int32, sizeof(val_int32), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_int32, sizeof(ptr_int32), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_int64 = 0;
    prepare_type_test( reader, "<t>-9223372036854775808</t>", sizeof("<t>-9223372036854775808</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int64, sizeof(val_int64), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_int64 == -9223372036854775807 - 1, "wrong value\n" );

    prepare_type_test( reader, "<t>-9223372036854775809</t>", sizeof("<t>-9223372036854775809</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int64, sizeof(val_int64), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int64, sizeof(val_int64), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_int64, sizeof(ptr_int64), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_uint8 = 0;
    prepare_type_test( reader, "<t> 255 </t>", sizeof("<t> 255 </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_uint8 == 255, "got %u\n", val_uint8 );

    prepare_type_test( reader, "<t>+255</t>", sizeof("<t>+255</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t>-255</t>", sizeof("<t>-255</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    todo_wine ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t>0xff</t>", sizeof("<t>0xff</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t>256</t>", sizeof("<t>256</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_uint8, sizeof(ptr_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_uint16 = 0;
    prepare_type_test( reader, "<t>65535</t>", sizeof("<t>65535</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint16, sizeof(val_uint16), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_uint16 == 65535, "got %u\n", val_uint16 );

    prepare_type_test( reader, "<t>65536</t>", sizeof("<t>65536</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint16, sizeof(val_uint16), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint16, sizeof(val_uint16), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_uint16, sizeof(ptr_uint16), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_uint32 = 0;
    prepare_type_test( reader, "<t>4294967295</t>", sizeof("<t>4294967295</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint32, sizeof(val_uint32), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_uint32 == ~0, "got %u\n", val_uint32 );

    prepare_type_test( reader, "<t>4294967296</t>", sizeof("<t>4294967296</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint32, sizeof(val_uint32), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint32, sizeof(val_uint32), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_uint32, sizeof(ptr_uint32), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_uint64 = 0;
    prepare_type_test( reader, "<t>18446744073709551615</t>", sizeof("<t>18446744073709551615</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint64, sizeof(val_uint64), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_uint64 == ~0, "wrong value\n" );

    prepare_type_test( reader, "<t>18446744073709551616</t>", sizeof("<t>18446744073709551616</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint64, sizeof(val_uint64), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint64, sizeof(val_uint64), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_uint64, sizeof(ptr_uint64), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    enum_desc.values       = enum_values;
    enum_desc.valueCount   = ARRAY_SIZE( enum_values );
    enum_desc.maxByteCount = 3;
    enum_desc.nameIndices  = NULL;

    val_enum = 0;
    prepare_type_test( reader, "<t>ONE</t>", sizeof("<t>ONE</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                     WS_READ_REQUIRED_VALUE, heap, &val_enum, sizeof(val_enum), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_enum == 1, "got %d\n", val_enum );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                     WS_READ_REQUIRED_VALUE, heap, &val_enum, sizeof(val_enum), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_enum, sizeof(ptr_enum), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t>{00000000-0000-0000-0000-000000000000}</t>",
                       sizeof("<t>{00000000-0000-0000-0000-000000000000}</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    memset( &val_guid, 0xff, sizeof(val_guid) );
    prepare_type_test( reader, "<t> 00000000-0000-0000-0000-000000000000 </t>",
                       sizeof("<t> 00000000-0000-0000-0000-000000000000 </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( IsEqualGUID( &val_guid, &guid_null ), "wrong guid\n" );

    memset( &val_guid, 0, sizeof(val_guid) );
    prepare_type_test( reader, "<t>00000000-0000-0000-0000-0000000000a1</t>",
                       sizeof("<t>00000000-0000-0000-0000-0000000000a1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( IsEqualGUID( &val_guid, &guid ), "wrong guid\n" );

    memset( &val_guid, 0, sizeof(val_guid) );
    prepare_type_test( reader, "<t>00000000-0000-0000-0000-0000000000A1</t>",
                       sizeof("<t>00000000-0000-0000-0000-0000000000A1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( IsEqualGUID( &val_guid, &guid ), "wrong guid\n" );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_guid, sizeof(ptr_guid), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    memset( &val_bytes, 0, sizeof(val_bytes) );
    prepare_type_test( reader, "<t>dGVzdA==</t>", sizeof("<t>dGVzdA==</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bytes.length == 4, "got %lu\n", val_bytes.length );
    ok( !memcmp( val_bytes.bytes, "test", 4 ), "wrong data\n" );

    memset( &val_bytes, 0, sizeof(val_bytes) );
    prepare_type_test( reader, "<t> dGVzdA== </t>", sizeof("<t> dGVzdA== </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_bytes.length == 4, "got %lu\n", val_bytes.length );
    ok( !memcmp( val_bytes.bytes, "test", 4 ), "wrong data\n" );

    prepare_type_test( reader, "<t>dGVzdA===</t>", sizeof("<t>dGVzdA===</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    val_bytes.length = 0xdeadbeef;
    val_bytes.bytes  = (BYTE *)0xdeadbeef;
    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !val_bytes.length, "got %lu\n", val_bytes.length );
    todo_wine ok( val_bytes.bytes != NULL, "got %p\n", val_bytes.bytes );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_bytes, sizeof(ptr_bytes), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !ptr_bytes->length, "got %lu\n", ptr_bytes->length );
    todo_wine ok( ptr_bytes->bytes != NULL, "got %p\n", ptr_bytes->bytes );

    val_str = NULL;
    prepare_type_test( reader, utf8, sizeof(utf8) );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_str != NULL, "pointer not set\n" );
    ok( !lstrcmpW( val_str, L"\x2019" ), "got %s\n", wine_dbgstr_w(val_str) );

    val_str = NULL;
    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_str != NULL, "got %p\n", val_str );
    ok( !val_str[0], "got %s\n", wine_dbgstr_w(val_str) );

    memset( &val_xmlstr, 0, sizeof(val_xmlstr) );
    prepare_type_test( reader, "<t> test </t>", sizeof("<t> test </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_XML_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_xmlstr, sizeof(val_xmlstr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_xmlstr.length == 6, "got %lu\n", val_xmlstr.length );
    ok( !memcmp( val_xmlstr.bytes, " test ", 6 ), "wrong data\n" );

    val_xmlstr.length = 0xdeadbeef;
    val_xmlstr.bytes  = (BYTE *)0xdeadbeef;
    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_XML_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_xmlstr, sizeof(val_xmlstr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !val_xmlstr.length, "got %lu\n", val_bytes.length );
    todo_wine ok( val_xmlstr.bytes != NULL, "got %p\n", val_bytes.bytes );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_XML_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_xmlstr, sizeof(ptr_xmlstr), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !ptr_xmlstr->length, "got %lu\n", ptr_bytes->length );
    todo_wine ok( ptr_xmlstr->bytes != NULL, "got %p\n", ptr_bytes->bytes );

    memset( &val_string, 0, sizeof(val_string) );
    prepare_type_test( reader, "<t> test </t>", sizeof("<t> test </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_string, sizeof(val_string), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_string.length == 6, "got %lu\n", val_string.length );
    ok( !memcmp( val_string.chars, L" test ", 12 ), "wrong data\n" );

    val_string.length = 0xdeadbeef;
    val_string.chars  = (WCHAR *)0xdeadbeef;
    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_string, sizeof(val_string), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !val_string.length, "got %lu\n", val_string.length );
    todo_wine ok( val_string.chars != NULL, "got %p\n", val_string.chars );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRING_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_string, sizeof(ptr_string), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !ptr_string->length, "got %lu\n", ptr_string->length );
    todo_wine ok( ptr_string->chars != NULL, "got %p\n", ptr_string->chars );

    memset( &val_id, 0, sizeof(val_id) );
    val_id.guid.Data1 = 0xdeadbeef;
    prepare_type_test( reader, "<t> test </t>", sizeof("<t> test </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_id, sizeof(val_id), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_id.uri.length == 6, "got %lu\n", val_string.length );
    ok( !memcmp( val_id.uri.chars, L" test ", 12 ), "wrong data\n" );
    ok( IsEqualGUID( &val_id.guid, &guid_null ), "wrong guid\n" );

    memset( &val_id, 0, sizeof(val_id) );
    prepare_type_test( reader, "<t>urn:uuid:00000000-0000-0000-0000-0000000000a1</t>",
                       sizeof("<t>urn:uuid:00000000-0000-0000-0000-0000000000a1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_id, sizeof(val_id), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !val_id.uri.length, "got %lu\n", val_string.length );
    ok( val_id.uri.chars == NULL, "chars set %s\n", wine_dbgstr_wn(val_id.uri.chars, val_id.uri.length) );
    ok( IsEqualGUID( &val_id.guid, &guid ), "wrong guid\n" );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_id, sizeof(val_id), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, "<t></t>", sizeof("<t></t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UNIQUE_ID_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_id, sizeof(ptr_id), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    memset( &val_qname, 0, sizeof(val_qname) );
    hr = set_input( reader, "<t>u</t>", sizeof("<t>u</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_XML_QNAME_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_qname, sizeof(val_qname), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_qname.localName.length == 1, "got %lu\n", val_qname.localName.length );
    ok( val_qname.localName.bytes[0] == 'u', "wrong data\n" );
    ok( !val_qname.ns.length, "got %lu\n", val_qname.ns.length );
    ok( val_qname.ns.bytes != NULL, "bytes not set\n" );

    memset( &val_qname, 0, sizeof(val_qname) );
    hr = set_input( reader, "<p:t xmlns:p=\"ns\"> p:u </p:t>", sizeof("<p:t xmlns:p=\"ns\"> p:u </p:t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_XML_QNAME_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_qname, sizeof(val_qname), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val_qname.localName.length == 1, "got %lu\n", val_qname.localName.length );
    ok( val_qname.localName.bytes[0] == 'u', "wrong data\n" );
    ok( val_qname.ns.length == 2, "got %lu\n", val_qname.ns.length );
    ok( !memcmp( val_qname.ns.bytes, "ns", 2 ), "wrong data\n" );

    hr = set_input( reader, "<t></t>", sizeof("<t></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_XML_QNAME_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_qname, sizeof(val_qname), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, "<t></t>", sizeof("<t></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_XML_QNAME_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &ptr_qname, sizeof(ptr_qname), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_type_test( reader, faultxml, sizeof(faultxml) - 1 );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    memset( &fault, 0, sizeof(fault) );

    fault_desc.envelopeVersion = WS_ENVELOPE_VERSION_SOAP_1_1;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_FAULT_TYPE, &fault_desc,
                     WS_READ_REQUIRED_VALUE, heap, &fault, sizeof(fault), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( fault.code->value.localName.length == sizeof(faultcode) - 1, "got %lu\n", fault.code->value.localName.length );
    ok( !memcmp( fault.code->value.localName.bytes, faultcode, sizeof(faultcode) - 1 ), "wrong fault code\n" );
    ok( !fault.code->subCode, "subcode is not NULL\n" );
    ok( fault.reasonCount == 1, "got %lu\n", fault.reasonCount );
    ok( fault.reasons[0].text.length == ARRAY_SIZE(faultstring) - 1, "got %lu\n", fault.reasons[0].text.length );
    ok( !memcmp( fault.reasons[0].text.chars, faultstring, (ARRAY_SIZE(faultstring) - 1) * sizeof(WCHAR) ),
        "wrong fault string\n" );
    ok( fault.actor.length == ARRAY_SIZE(faultactor) - 1, "got %lu\n", fault.actor.length  );
    ok( !memcmp( fault.actor.chars, faultactor, ARRAY_SIZE(faultactor) - 1 ), "wrong fault actor\n" );
    ok( !fault.node.length, "fault node not empty\n" );
    ok( fault.detail != NULL, "fault detail not set\n" );
    check_output_buffer( fault.detail, faultdetail, __LINE__ );

    fault_desc.envelopeVersion = WS_ENVELOPE_VERSION_NONE;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_FAULT_TYPE, &fault_desc,
                     WS_READ_REQUIRED_VALUE, heap, &fault, sizeof(fault), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsGetXmlAttribute(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_STRING xmlstr;
    WS_HEAP *heap;
    WCHAR *str;
    ULONG count;
    int found;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, data9, sizeof(data9) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(data9) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );

    xmlstr.bytes      = (BYTE *)"attr";
    xmlstr.length     = sizeof("attr") - 1;
    xmlstr.dictionary = NULL;
    xmlstr.id         = 0;
    str = NULL;
    count = 0;
    hr = WsGetXmlAttribute( reader, &xmlstr, heap, &str, &count, NULL );
    todo_wine ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( str != NULL, "str not set\n" );
    todo_wine ok( count == 5, "got %lu\n", count );
    /* string is not null-terminated */
    if (str) ok( !memcmp( str, L"value", count * sizeof(WCHAR) ), "wrong data\n" );

    xmlstr.bytes      = (BYTE *)"none";
    xmlstr.length     = sizeof("none") - 1;
    xmlstr.dictionary = NULL;
    xmlstr.id         = 0;
    str = (WCHAR *)0xdeadbeef;
    count = 0xdeadbeef;
    hr = WsGetXmlAttribute( reader, &xmlstr, heap, &str, &count, NULL );
    todo_wine ok( hr == S_FALSE, "got %#lx\n", hr );
    todo_wine ok( str == NULL, "str not set\n" );
    todo_wine ok( !count, "got %lu\n", count );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsXmlStringEquals(void)
{
    BYTE bom[] = {0xef,0xbb,0xbf};
    WS_XML_STRING str1 = {0, NULL}, str2 = {0, NULL};
    HRESULT hr;

    hr = WsXmlStringEquals( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsXmlStringEquals( &str1, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsXmlStringEquals( NULL, &str2, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    str1.length = 1;
    str1.bytes  = (BYTE *)"a";
    hr = WsXmlStringEquals( &str1, &str1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    str2.length = 1;
    str2.bytes  = (BYTE *)"b";
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );

    str2.length = 1;
    str2.bytes  = bom;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );

    str1.length = 3;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );

    str2.length = 3;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );

    str1.length = 3;
    str1.bytes  = bom;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_WsAlloc(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    void *ptr;
    SIZE_T requested, actual;
    ULONG size;

    hr = WsCreateHeap( 256, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    ptr = (void *)0xdeadbeef;
    hr = WsAlloc( NULL, 16, &ptr, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    ok( ptr == (void *)0xdeadbeef, "ptr set\n" );

    ptr = (void *)0xdeadbeef;
    hr = WsAlloc( heap, 512, &ptr, NULL );
    ok( hr == WS_E_QUOTA_EXCEEDED, "got %#lx\n", hr );
    ok( ptr == (void *)0xdeadbeef, "ptr set\n" );

    ptr = NULL;
    hr = WsAlloc( heap, 16, &ptr, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ptr != NULL, "ptr not set\n" );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( requested == 16, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( actual == 128, "got %Iu\n", actual );

    WsFreeHeap( heap );
}

static void test_WsMoveReader(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_READER *reader;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    WS_XML_STRING localname = {1, (BYTE *)"a"}, localname2 = {1, (BYTE *)"b"}, ns = {0, NULL};
    const WS_XML_NODE *node;
    WS_XML_ELEMENT_NODE *elem;
    WS_XML_UTF8_TEXT utf8;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( NULL, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* reader must be set to an XML buffer */
    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, data8, sizeof(data8) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    WsFreeReader( reader );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
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

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* first element is child node of BOF node */
    hr = WsMoveReader( reader, WS_MOVE_TO_BOF, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* EOF node is last child of BOF node */
    hr = WsMoveReader( reader, WS_MOVE_TO_BOF, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_END_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeWriter( writer );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* <a><b>test</b></a> */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset(&utf8, 0, sizeof(utf8));
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes  = (BYTE *)"test";
    utf8.value.length = sizeof("test") - 1;
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    WsFreeReader( reader );
    WsFreeWriter( writer );
    WsFreeHeap( heap );
}

static void prepare_struct_type_test( WS_XML_READER *reader, const char *data )
{
    HRESULT hr;
    ULONG size = strlen( data );

    hr = set_input( reader, data, size );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_simple_struct_type(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_XML_STRING ns = {0, NULL}, localname = {3, (BYTE *)"str"};
    WS_XML_STRING localname2 = {4, (BYTE *)"test"};
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;
    struct test { WCHAR *str; } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* element field mapping */
    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.localName = &localname;
    f.ns        = &ns;
    f.type      = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &localname2;
    s.typeNs        = &ns;

    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str><str>test2</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str><str>test2</str>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    s.structOptions = WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT;
    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str><str>test2</str>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    s.structOptions = 0;

    test = NULL;
    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !wcscmp( test->str, L"test" ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    test = NULL;
    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !wcscmp( test->str, L"test" ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    test = NULL;
    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !wcscmp( test->str, L"test" ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 3, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "str", 3 ), "wrong data\n" );

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !wcscmp( test->str, L"test" ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* attribute field mapping */
    f.mapping = WS_ATTRIBUTE_FIELD_MAPPING;

    test = NULL;
    prepare_struct_type_test( reader, "<test str=\"test\"/>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !wcscmp( test->str, L"test" ), "wrong data test %p test->str %p\n", test, test->str );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_cdata(void)
{
    static const char test[] = "<t><![CDATA[<data>]]></t>";
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, sizeof(test) - 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_CDATA, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
            ok( utf8->value.length == 6, "got %lu\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "<data>", 6 ), "wrong data\n" );
        }
    }

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_CDATA, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    WsFreeReader( reader );
}

static void test_WsFindAttribute(void)
{
    static const char test[] = "<t attr='value' attr2='value2'></t>";
    static const char test2[] = "<p:t attr='value' p:attr2='value2' xmlns:p=\"ns\"></t>";
    WS_XML_STRING ns = {0, NULL}, ns2 = {2, (BYTE *)"ns"}, localname = {4, (BYTE *)"attr"};
    WS_XML_STRING localname2 = {5, (BYTE *)"attr2"}, localname3 = {5, (BYTE *)"attr3"};
    WS_XML_READER *reader;
    ULONG index;
    HRESULT hr;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFindAttribute( reader, &localname, &ns, TRUE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFindAttribute( reader, &localname, NULL, TRUE, &index, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFindAttribute( reader, NULL, &ns, TRUE, &index, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname, &ns, TRUE, &index, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !index, "got %lu\n", index );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname2, &ns, TRUE, &index, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( index == 1, "got %lu\n", index );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname, &ns, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    ok( index == 0xdeadbeef, "got %lu\n", index );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname3, &ns, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( index == 0xdeadbeef, "got %lu\n", index );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname3, &ns, FALSE, &index, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );
    ok( index == ~0u, "got %lu\n", index );

    hr = set_input( reader, test2, sizeof(test2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname, &ns, TRUE, &index, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !index, "got %lu\n", index );

    hr = WsFindAttribute( reader, &localname2, &ns2, TRUE, &index, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFindAttribute( reader, &localname2, &ns, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, test2, sizeof(test2) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFindAttribute( reader, &localname, &ns2, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeReader( reader );
}

static void prepare_namespace_test( WS_XML_READER *reader, const char *data )
{
    HRESULT hr;
    ULONG size = strlen( data );

    hr = set_input( reader, data, size );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
}

static void test_WsGetNamespaceFromPrefix(void)
{
    WS_XML_STRING prefix = {0, NULL};
    const WS_XML_STRING *ns;
    const WS_XML_NODE *node;
    WS_XML_READER *reader;
    HRESULT hr;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, NULL, FALSE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, NULL, FALSE, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, &prefix, FALSE, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    ns = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    ok( ns == (const WS_XML_STRING *)0xdeadbeef, "ns set\n" );

    hr = set_input( reader, "<prefix:t xmlns:prefix2='ns'/>", sizeof("<prefix:t xmlns:prefix2='ns'/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %lu\n", ns->length );

    prepare_namespace_test( reader, "<t xmls='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %lu\n", ns->length );

    prepare_namespace_test( reader, "<prefix:t xmlns:prefix='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %lu\n", ns->length );

    prepare_namespace_test( reader, "<prefix:t xmlns:prefix='ns'></t>" );
    prefix.bytes = (BYTE *)"prefix";
    prefix.length = 6;
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 2, "got %lu\n", ns->length );
        ok( !memcmp( ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t xmlns:prefix='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 2, "got %lu\n", ns->length );
        ok( !memcmp( ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:prefix='ns'></t>", sizeof("<t xmlns:prefix='ns'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;
        WS_XML_UTF8_TEXT *text;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes not set\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( !elem->ns->length, "got %lu\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->singleQuote, "singleQuote not set\n" );
        ok( attr->isXmlNs, "isXmlNs not set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 6, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->prefix->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 6, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( !text->value.length, "got %lu\n", text->value.length );
        ok( text->value.bytes == NULL, "bytes set\n" );
    }

    prepare_namespace_test( reader, "<t xmlns:prefix='ns'></t>" );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    prefix.bytes = (BYTE *)"xml";
    prefix.length = 3;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 36, "got %lu\n", ns->length );
        ok( !memcmp( ns->bytes, "http://www.w3.org/XML/1998/namespace", 36 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    prefix.bytes = (BYTE *)"xmlns";
    prefix.length = 5;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 29, "got %lu\n", ns->length );
        ok( !memcmp( ns->bytes, "http://www.w3.org/2000/xmlns/", 29 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t></t>" );
    ns = (WS_XML_STRING *)0xdeadbeef;
    prefix.bytes = (BYTE *)"prefix2";
    prefix.length = 7;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( ns == (WS_XML_STRING *)0xdeadbeef, "ns set\n" );

    prepare_namespace_test( reader, "<t></t>" );
    ns = (WS_XML_STRING *)0xdeadbeef;
    prefix.bytes = (BYTE *)"prefix2";
    prefix.length = 7;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, FALSE, &ns, NULL );
    ok( hr == S_FALSE, "got %#lx\n", hr );
    ok( ns == NULL, "ns not set\n" );

    hr = set_input( reader, "<t prefix:attr='' xmlns:prefix='ns'></t>", sizeof("<t prefix:attr='' xmlns:prefix='ns'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->singleQuote, "singleQuote not set\n" );
        ok( !attr->isXmlNs, "isXmlNs is set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 6, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->prefix->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 4, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr", 4 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:p='ns'><u xmlns:p='ns2'/></t>", sizeof("<t xmlns:p='ns'><u xmlns:p='ns2'/></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns'><p:u p:a=''/></t>", sizeof("<t xmlns:p='ns'><p:u p:a=''/></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes set\n" );
        ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "a", 1 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns='ns'></t>", sizeof("<t xmlns='ns'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes not set\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong data\n" );

        attr = elem->attributes[0];
        ok( attr->isXmlNs, "isXmlNs is not set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 5, "got %lu\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "xmlns", 5 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:p='ns2'></t>", sizeof("<t xmlns:p='ns' xmlns:p='ns2'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:p='ns'></t>", sizeof("<t xmlns:p='ns' xmlns:p='ns'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:P='ns2'></t>", sizeof("<t xmlns:p='ns' xmlns:P='ns2'></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeReader( reader );
}

static void test_text_field_mapping(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_STRUCT_DESCRIPTION s;
    WS_FIELD_DESCRIPTION f, *fields[1];
    struct test
    {
        WCHAR *str;
    } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<a>test</a>" );

    memset( &f, 0, sizeof(f) );
    f.mapping = WS_TEXT_FIELD_MAPPING;
    f.type    = WS_WSZ_TYPE;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 1;

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->str != NULL, "str not set\n" );
    ok( !wcscmp( test->str, L"test" ), "got %s\n", wine_dbgstr_w(test->str) );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_complex_struct_type(void)
{
    static const char data[] =
        "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">"
        "<o:services o:GenerationTime=\"2015-09-03T18:47:54\">"
        "<!--Build: 16.0.6202.6852-->"
        "</o:services>"
        "</o:OfficeConfig>";
    static const char data2[] =
        "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">"
        "<o:services o:GenerationTime=\"2015-09-03T18:47:54\"></o:services>"
        "<trailing>content</trailing>"
        "</o:OfficeConfig>";
    HRESULT hr;
    WS_ERROR *error;
    WS_ERROR_PROPERTY prop;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f2, *fields[1], *fields2[1];
    WS_XML_STRING str_officeconfig = {12, (BYTE *)"OfficeConfig"};
    WS_XML_STRING str_services = {8, (BYTE *)"services"};
    WS_XML_STRING str_generationtime = {14, (BYTE *)"GenerationTime"};
    WS_XML_STRING ns = {39, (BYTE *)"urn:schemas-microsoft-com:office:office"};
    LANGID langid = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;
    struct services
    {
        WCHAR *generationtime;
    };
    struct officeconfig
    {
        struct services *services;
    } *test;

    prop.id        = WS_ERROR_PROPERTY_LANGID;
    prop.value     = &langid;
    prop.valueSize = sizeof(langid);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element content type mapping */
    prepare_struct_type_test( reader, data );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 12, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "OfficeConfig", 12 ), "wrong data\n" );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 8, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "services", 8 ), "wrong data\n" );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping         = WS_ATTRIBUTE_FIELD_MAPPING;
    f2.localName       = &str_generationtime;
    f2.ns              = &ns;
    f2.type            = WS_WSZ_TYPE;
    f2.options         = WS_FIELD_OPTIONAL;
    fields2[0] = &f2;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(*test->services);
    s2.alignment     = TYPE_ALIGNMENT(struct services);
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
    s.alignment     = TYPE_ALIGNMENT(struct officeconfig);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_officeconfig;
    s.typeNs        = &ns;

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), error );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( !wcscmp( test->services->generationtime, L"2015-09-03T18:47:54" ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    /* element type mapping */
    prepare_struct_type_test( reader, data );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 12, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "OfficeConfig", 12 ), "wrong data\n" );

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), error );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test) ok( !wcscmp( test->services->generationtime, L"2015-09-03T18:47:54" ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* trailing content */
    prepare_struct_type_test( reader, data2 );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    s.structOptions = WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    prepare_struct_type_test( reader, data2 );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    s.structOptions = 0;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), error );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeReader( reader );
    WsFreeHeap( heap );
    WsFreeError( error );
}

static void test_repeating_element(void)
{
    static const char data[] =
        "<services>"
        "<service><id>1</id></service>"
        "<service><id>2</id></service>"
        "</services>";
    static const char data2[] =
        "<services></services>";
    static const char data3[] =
        "<services>"
        "<wrapper>"
        "<service><id>1</id></service>"
        "<service><id>2</id></service>"
        "</wrapper>"
        "</services>";
    static const char data4[] =
        "<services>"
        "<wrapper>"
        "<service>1</service>"
        "<service>2</service>"
        "</wrapper>"
        "</services>";
    static const char data5[] =
        "<services>"
        "<wrapper>"
        "<service name='1'>1</service>"
        "<service name='2'>2</service>"
        "</wrapper>"
        "</services>";
    static const char data6[] =
        "<services>"
        "<service><name></name></service>"
        "</services>";
    WS_XML_STRING str_name = {4, (BYTE *)"name"};
    WS_XML_STRING str_services = {8, (BYTE *)"services"};
    WS_XML_STRING str_service = {7, (BYTE *)"service"};
    WS_XML_STRING str_wrapper = {7, (BYTE *)"wrapper"};
    WS_XML_STRING str_id = {2, (BYTE *)"id"};
    WS_XML_STRING str_ns = {0, NULL};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f2, f3, *fields[1], *fields2[2];
    WS_ITEM_RANGE range;
    struct service { UINT32 id; };
    struct service2 { WCHAR *id; };
    struct service3 { WCHAR *name; WCHAR *id; };
    struct service4 { WS_STRING name; };
    struct services
    {
        struct service *service;
        ULONG           service_count;
    } *test;
    struct services2
    {
        struct service2 *service;
        ULONG            service_count;
    } *test2;
    struct services3
    {
        struct service3 *service;
        ULONG            service_count;
    } *test3;
    struct services4
    {
        struct service **service;
        ULONG            service_count;
    } *test4;
    struct services5
    {
        struct service4 *service;
        ULONG            service_count;
    } *test5;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    prepare_struct_type_test( reader, data );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.localName = &str_id;
    f2.ns        = &str_ns;
    f2.type      = WS_UINT32_TYPE;
    fields2[0]   = &f2;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(struct service);
    s2.alignment     = TYPE_ALIGNMENT(struct service);
    s2.fields        = fields2;
    s2.fieldCount    = 1;
    s2.typeLocalName = &str_service;

    memset( &f, 0, sizeof(f) );
    f.mapping         = WS_REPEATING_ELEMENT_FIELD_MAPPING;
    f.countOffset     = FIELD_OFFSET(struct services, service_count);
    f.type            = WS_STRUCT_TYPE;
    f.typeDescription = &s2;
    f.itemLocalName   = &str_service;
    f.itemNs          = &str_ns;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct services);
    s.alignment     = TYPE_ALIGNMENT(struct services);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_services;

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( test->service_count == 2, "got %lu\n", test->service_count );
    ok( test->service[0].id == 1, "got %u\n", test->service[0].id );
    ok( test->service[1].id == 2, "got %u\n", test->service[1].id );

    /* array of pointers */
    prepare_struct_type_test( reader, data );
    f.options = WS_FIELD_POINTER;
    test4 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test4, sizeof(test4), NULL );
    ok( hr == S_OK || broken(hr == E_INVALIDARG) /* win7 */, "got %#lx\n", hr );
    if (test4)
    {
        ok( test4->service != NULL, "service not set\n" );
        ok( test4->service_count == 2, "got %lu\n", test4->service_count );
        ok( test4->service[0]->id == 1, "got %u\n", test4->service[0]->id );
        ok( test4->service[1]->id == 2, "got %u\n", test4->service[1]->id );
    }

    /* item range */
    prepare_struct_type_test( reader, data2 );
    f.options = 0;
    range.minItemCount = 0;
    range.maxItemCount = 1;
    f.itemRange = &range;
    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( !test->service_count, "got %lu\n", test->service_count );

    /* wrapper element */
    prepare_struct_type_test( reader, data3 );
    f.itemRange = NULL;
    f.localName = &str_wrapper;
    f.ns        = &str_ns;
    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( test->service_count == 2, "got %lu\n", test->service_count );
    ok( test->service[0].id == 1, "got %u\n", test->service[0].id );
    ok( test->service[1].id == 2, "got %u\n", test->service[1].id );

    /* repeating text field mapping */
    prepare_struct_type_test( reader, data4 );
    f2.mapping   = WS_TEXT_FIELD_MAPPING;
    f2.localName = NULL;
    f2.ns        = NULL;
    f2.type      = WS_WSZ_TYPE;
    s2.size      = sizeof(struct service2);
    s2.alignment = TYPE_ALIGNMENT(struct service2);
    test2 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test2 != NULL, "test2 not set\n" );
    ok( test2->service != NULL, "service not set\n" );
    ok( test2->service_count == 2, "got %lu\n", test2->service_count );
    ok( !wcscmp( test2->service[0].id, L"1" ), "wrong data\n" );
    ok( !wcscmp( test2->service[1].id, L"2" ), "wrong data\n" );

    /* repeating attribute field + text field mapping */
    prepare_struct_type_test( reader, data5 );
    f2.offset    = FIELD_OFFSET(struct service3, id);
    memset( &f3, 0, sizeof(f3) );
    f3.mapping   = WS_ATTRIBUTE_FIELD_MAPPING;
    f3.localName = &str_name;
    f3.ns        = &str_ns;
    f3.type      = WS_WSZ_TYPE;
    fields2[0]   = &f3;
    fields2[1]   = &f2;
    s2.size      = sizeof(struct service3);
    s2.alignment = TYPE_ALIGNMENT(struct service3);
    s2.fieldCount = 2;
    test3 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test3, sizeof(test3), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test3 != NULL, "test3 not set\n" );
    ok( test3->service != NULL, "service not set\n" );
    ok( test3->service_count == 2, "got %lu\n", test3->service_count );
    ok( !wcscmp( test3->service[0].name, L"1" ), "wrong data\n" );
    ok( !wcscmp( test3->service[0].id, L"1" ), "wrong data\n" );
    ok( !wcscmp( test3->service[1].name, L"2" ), "wrong data\n" );
    ok( !wcscmp( test3->service[1].id, L"2" ), "wrong data\n" );

    /* empty text, item range */
    prepare_struct_type_test( reader, data6 );

    memset( &f2, 0, sizeof(f2) );
    f2.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f2.localName = &str_name;
    f2.ns        = &str_ns;
    f2.type      = WS_STRING_TYPE;
    fields2[0]   = &f2;

    memset( &s2, 0, sizeof(s2) );
    s2.size          = sizeof(struct service4);
    s2.alignment     = TYPE_ALIGNMENT(struct service4);
    s2.fields        = fields2;
    s2.fieldCount    = 1;
    s2.typeLocalName = &str_service;

    range.minItemCount = 1;
    range.maxItemCount = 2;
    memset( &f, 0, sizeof(f) );
    f.mapping         = WS_REPEATING_ELEMENT_FIELD_MAPPING;
    f.countOffset     = FIELD_OFFSET(struct services5, service_count);
    f.type            = WS_STRUCT_TYPE;
    f.typeDescription = &s2;
    f.itemLocalName   = &str_service;
    f.itemNs          = &str_ns;
    f.itemRange       = &range;
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct services5);
    s.alignment     = TYPE_ALIGNMENT(struct services5);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_services;

    test5 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test5, sizeof(test5), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test5 != NULL, "test5 not set\n" );
    ok( test5->service != NULL, "service not set\n" );
    ok( test5->service_count == 1, "got %lu\n", test5->service_count );
    ok( !test5->service[0].name.length, "got %lu\n", test5->service[0].name.length );
    todo_wine ok( test5->service[0].name.chars != NULL, "chars set\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsResetHeap(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    SIZE_T requested, actual;
    ULONG size;
    void *ptr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !requested, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !actual, "got %Iu\n", actual );

    hr = WsAlloc( heap, 128, &ptr, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( requested == 128, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( actual == 128, "got %Iu\n", actual );

    hr = WsAlloc( heap, 1, &ptr, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( requested == 129, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( actual == 384, "got %Iu\n", actual );

    hr = WsResetHeap( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsResetHeap( heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !requested, "got %Iu\n", requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( actual == 128, "got %Iu\n", actual );

    WsFreeHeap( heap );
}

static void test_datetime(void)
{
    static const struct
    {
        const char        *str;
        HRESULT            hr;
        __int64            ticks;
        WS_DATETIME_FORMAT format;
    }
    tests[] =
    {
        {"<t>0000-01-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00Z</t>", S_OK, 0, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00.0Z</t>", S_OK, 0, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.1Z</t>", S_OK, 0x0000f4240, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.01Z</t>", S_OK, 0x0000186a0, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.0000001Z</t>", S_OK, 1, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.9999999Z</t>", S_OK, 0x00098967f, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.0000000Z</t>", S_OK, 0, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T00:00:00.00000001Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00Z-</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>-0001-01-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-00-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-13-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-12-01T00:00:00Z</t>", S_OK, 0x1067555f88000, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-00T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>2001-01-32T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>2001-01-31T00:00:00Z</t>", S_OK, 0x8c2592fe3794000, WS_DATETIME_FORMAT_UTC},
        {"<t>1900-02-29T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>2000-02-29T00:00:00Z</t>", S_OK, 0x8c1505f0e438000, 0},
        {"<t>2001-02-29T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>2001-02-28T00:00:00Z</t>", S_OK, 0x8c26f30870a4000, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-00-01U00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T24:00:00Z</t>", S_OK, 0xc92a69c000, WS_DATETIME_FORMAT_UTC},
        {"<t>0001-01-01T24:00:01Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:60:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:60Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00Y</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00+00:01</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0001-01-01T00:00:00-00:01</t>", S_OK, 0x023c34600, WS_DATETIME_FORMAT_LOCAL},
        {"<t>9999-12-31T24:00:00+00:01</t>", S_OK, 0x2bca2875d073fa00, WS_DATETIME_FORMAT_LOCAL},
        {"<t>9999-12-31T24:00:00-00:01</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0002-01-01T00:00:00+14:01</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0002-01-01T00:00:00+15:00</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0002-01-01T00:00:00+13:60</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>0002-01-01T00:00:00+13:59</t>", S_OK, 0x11e5c43cc5600, WS_DATETIME_FORMAT_LOCAL},
        {"<t>0002-01-01T00:00:00+01:00</t>", S_OK, 0x11ec917025800, WS_DATETIME_FORMAT_LOCAL},
        {"<t>2016-01-01T00:00:00-01:00</t>", S_OK, 0x8d31246dfbba800, WS_DATETIME_FORMAT_LOCAL},
        {"<t>2016-01-01T00:00:00Z</t>", S_OK, 0x8d3123e7df74000, WS_DATETIME_FORMAT_UTC},
        {"<t> 2016-01-02T03:04:05Z </t>", S_OK, 0x8d313215fb64080, WS_DATETIME_FORMAT_UTC},
        {"<t>+2016-01-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t></t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>01-01-01T00:00:00Z</t>", WS_E_INVALID_FORMAT, 0, 0},
        {"<t>1601-01-01T00:00:00Z</t>", S_OK, 0x701ce1722770000, WS_DATETIME_FORMAT_UTC},
    };
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_DATETIME date;
    ULONG i;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        memset( &date, 0, sizeof(date) );
        prepare_type_test( reader, tests[i].str, strlen(tests[i].str) );
        hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_DATETIME_TYPE, NULL,
                         WS_READ_REQUIRED_VALUE, heap, &date, sizeof(date), NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK)
        {
            ok( date.ticks == tests[i].ticks, "%lu: got %s\n", i, wine_dbgstr_longlong(date.ticks) );
            ok( date.format == tests[i].format, "%lu: got %u\n", i, date.format );
        }
    }

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsDateTimeToFileTime(void)
{
    static const struct
    {
        WS_DATETIME dt;
        HRESULT     hr;
        FILETIME    ft;
    }
    tests[] =
    {
        { {0, WS_DATETIME_FORMAT_UTC}, WS_E_INVALID_FORMAT, {0, 0} },
        { {0x701ce172276ffff, WS_DATETIME_FORMAT_UTC}, WS_E_INVALID_FORMAT, {0, 0} },
        { {0x701ce1722770000, WS_DATETIME_FORMAT_UTC}, S_OK, {0, 0} },
        { {0x2bca2875f4373fff, WS_DATETIME_FORMAT_UTC}, S_OK, {0xd1c03fff, 0x24c85a5e} },
        { {0x2bca2875f4374000, WS_DATETIME_FORMAT_UTC}, S_OK, {0xd1c04000, 0x24c85a5e} },
        { {0x2bca2875f4374000, WS_DATETIME_FORMAT_LOCAL}, S_OK, {0xd1c04000, 0x24c85a5e} },
        { {~0, WS_DATETIME_FORMAT_UTC}, S_OK, {0xdd88ffff, 0xf8fe31e8} },
    };
    WS_DATETIME dt;
    FILETIME ft;
    HRESULT hr;
    ULONG i;

    hr = WsDateTimeToFileTime( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    dt.ticks  = 0x701ce172277000;
    dt.format = WS_DATETIME_FORMAT_UTC;
    hr = WsDateTimeToFileTime( &dt, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsDateTimeToFileTime( NULL, &ft, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        memset( &ft, 0, sizeof(ft) );
        hr = WsDateTimeToFileTime( &tests[i].dt, &ft, NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK)
        {
            ok( ft.dwLowDateTime == tests[i].ft.dwLowDateTime, "%lu: got %#lx\n", i, ft.dwLowDateTime );
            ok( ft.dwHighDateTime == tests[i].ft.dwHighDateTime, "%lu: got %#lx\n", i, ft.dwHighDateTime );
        }
    }
}

static void test_WsFileTimeToDateTime(void)
{
    WS_DATETIME dt;
    FILETIME ft;
    HRESULT hr;

    hr = WsFileTimeToDateTime( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    ft.dwLowDateTime = ft.dwHighDateTime = 0;
    hr = WsFileTimeToDateTime( &ft, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsFileTimeToDateTime( NULL, &dt, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    dt.ticks = 0xdeadbeef;
    dt.format = 0xdeadbeef;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dt.ticks == 0x701ce1722770000, "got %s\n", wine_dbgstr_longlong(dt.ticks) );
    ok( dt.format == WS_DATETIME_FORMAT_UTC, "got %u\n", dt.format );

    ft.dwLowDateTime  = 0xd1c03fff;
    ft.dwHighDateTime = 0x24c85a5e;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dt.ticks == 0x2bca2875f4373fff, "got %s\n", wine_dbgstr_longlong(dt.ticks) );
    ok( dt.format == WS_DATETIME_FORMAT_UTC, "got %u\n", dt.format );

    ft.dwLowDateTime++;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    ft.dwLowDateTime  = 0xdd88ffff;
    ft.dwHighDateTime = 0xf8fe31e8;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    ft.dwLowDateTime++;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );
}

static void test_double(void)
{
    static const struct
    {
        const char *str;
        HRESULT     hr;
        ULONGLONG   val;
    }
    tests[] =
    {
        {"<t>0.0</t>", S_OK, 0},
        {"<t>-0.0</t>", S_OK, 0x8000000000000000},
        {"<t>+0.0</t>", S_OK, 0},
        {"<t>-</t>", S_OK, 0},
        {"<t>-.</t>", S_OK, 0},
        {"<t>+</t>", S_OK, 0},
        {"<t>+.</t>", S_OK, 0},
        {"<t>.</t>", S_OK, 0},
        {"<t>.0</t>", S_OK, 0},
        {"<t>0.</t>", S_OK, 0},
        {"<t>0</t>", S_OK, 0},
        {"<t> 0 </t>", S_OK, 0},
        {"<t></t>", WS_E_INVALID_FORMAT, 0},
        {"<t>0,1</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1.1.</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1</t>", S_OK, 0x3ff0000000000000},
        {"<t>1.0000000000000002</t>", S_OK, 0x3ff0000000000001},
        {"<t>1.0000000000000004</t>", S_OK, 0x3ff0000000000002},
        {"<t>10000000000000000000</t>", S_OK, 0x43e158e460913d00},
        {"<t>100000000000000000000</t>", S_OK, 0x4415af1d78b58c40},
        {"<t>2</t>", S_OK, 0x4000000000000000},
        {"<t>-2</t>", S_OK, 0xc000000000000000},
        {"<t>nodouble</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>INF</t>", S_OK, 0x7ff0000000000000},
        {"<t>-INF</t>", S_OK, 0xfff0000000000000},
        {"<t>+INF</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>Infinity</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>-Infinity</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>inf</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>NaN</t>", S_OK, 0xfff8000000000000},
        {"<t>-NaN</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>NAN</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>0.3</t>", S_OK, 0x3fd3333333333333},
        {"<t>0.33</t>", S_OK, 0x3fd51eb851eb851f},
        {"<t>0.333</t>", S_OK, 0x3fd54fdf3b645a1d},
        {"<t>0.3333</t>", S_OK, 0x3fd554c985f06f69},
        {"<t>0.33333</t>", S_OK, 0x3fd555475a31a4be},
        {"<t>0.333333</t>", S_OK, 0x3fd55553ef6b5d46},
        {"<t>0.3333333</t>", S_OK, 0x3fd55555318abc87},
        {"<t>0.33333333</t>", S_OK, 0x3fd5555551c112da},
        {"<t>0.333333333</t>", S_OK, 0x3fd5555554f9b516},
        {"<t>0.3333333333</t>", S_OK, 0x3fd55555554c2bb5},
        {"<t>0.33333333333</t>", S_OK, 0x3fd5555555546ac5},
        {"<t>0.3333333333333</t>", S_OK, 0x3fd55555555552fd},
        {"<t>0.33333333333333</t>", S_OK, 0x3fd5555555555519},
        {"<t>0.333333333333333</t>", S_OK, 0x3fd555555555554f},
        {"<t>0.3333333333333333</t>", S_OK, 0x3fd5555555555555},
        {"<t>0.33333333333333333</t>", S_OK, 0x3fd5555555555555},
        {"<t>0.1e10</t>", S_OK, 0x41cdcd6500000000},
        {"<t>1e</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1e0</t>", S_OK, 0x3ff0000000000000},
        {"<t>1e+1</t>", S_OK, 0x4024000000000000},
        {"<t>1e-1</t>", S_OK, 0x3fb999999999999a},
        {"<t>e10</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1e10.</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1E10</t>", S_OK, 0x4202a05f20000000},
        {"<t>1e10</t>", S_OK, 0x4202a05f20000000},
        {"<t>1e-10</t>", S_OK, 0x3ddb7cdfd9d7bdbb},
        {"<t>1.7976931348623158e308</t>", S_OK, 0x7fefffffffffffff},
        {"<t>1.7976931348623159e308</t>", S_OK, 0x7ff0000000000000},
        {"<t>4.94065645841247e-324</t>", S_OK, 0x1},
    };
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    ULONGLONG val;
    ULONG i;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        val = 0;
        prepare_type_test( reader, tests[i].str, strlen(tests[i].str) );
        hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_DOUBLE_TYPE, NULL,
                         WS_READ_REQUIRED_VALUE, heap, &val, sizeof(val), NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr == tests[i].hr) ok( val == tests[i].val, "%lu: got %s\n", i, wine_dbgstr_longlong(val) );
    }

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsReadElement(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_ELEMENT_DESCRIPTION desc;
    UINT32 val;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    desc.elementLocalName = &localname;
    desc.elementNs        = &ns;
    desc.type             = WS_UINT32_TYPE;
    desc.typeDescription  = NULL;

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( NULL, &desc, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( reader, NULL, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( reader, &desc, WS_READ_REQUIRED_VALUE, NULL, NULL, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    val = 0xdeadbeef;
    hr = WsReadElement( reader, &desc, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val == 1, "got %u\n", val );

    WsFreeReader( reader );
}

static void test_WsReadValue(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    UINT32 val;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( NULL, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, NULL, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    /* reader must be positioned correctly */
    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    val = 0xdeadbeef;
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val == 1, "got %u\n", val );

    prepare_struct_type_test( reader, "<u t='1'></u>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    WsFreeReader( reader );
}

static void test_WsResetError(void)
{
    WS_ERROR_PROPERTY prop;
    ULONG size, code, count;
    WS_ERROR *error;
    LANGID langid;
    WS_STRING str;
    WS_FAULT fault;
    WS_XML_STRING xmlstr;
    WS_FAULT *faultp;
    HRESULT hr;

    hr = WsResetError( NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    error = NULL;
    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( error != NULL, "error not set\n" );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetError( error );
    ok( hr == S_OK, "got %#lx\n", hr );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !code, "got %lu\n", code );

    WsFreeError( error );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    prop.id = WS_ERROR_PROPERTY_LANGID;
    prop.value = &langid;
    prop.valueSize = sizeof(langid);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );

    hr = WsResetError( error );
    ok( hr == S_OK, "got %#lx\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );

    WsFreeError( error );

    str.chars = (WCHAR *) L"str";
    str.length = 3;

    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddErrorString(error, &str );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsAddErrorString(error, &str );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );

    hr = WsResetError( error );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 0, "got %lu\n", count );

    WsFreeError( error );

    memset( &fault, 0, sizeof(fault) );
    memset( &xmlstr, 0, sizeof(xmlstr) );
    xmlstr.bytes = (BYTE *)"str";
    xmlstr.length = 3;

    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(fault) );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &xmlstr, sizeof(xmlstr) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetError( error );
    ok( hr == S_OK, "got %#lx\n", hr );

    faultp = (WS_FAULT *)0xdeadbeef;
    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &faultp, sizeof(faultp) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( faultp == NULL, "faultp != NULL\n" );

    xmlstr.length = 0xdeadbeef;
    xmlstr.bytes = (BYTE *)0xdeadbeef;
    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &xmlstr, sizeof(xmlstr) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( xmlstr.length == 0, "got %lu\n", xmlstr.length );

    WsFreeError( error );
}

static void test_WsGetReaderPosition(void)
{
    WS_HEAP *heap;
    WS_XML_READER *reader;
    WS_XML_BUFFER *buffer;
    WS_XML_NODE_POSITION pos;
    HRESULT hr;

    hr = WsGetReaderPosition( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* reader must be set to an XML buffer */
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t/>", sizeof("<t/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderPosition( reader, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos.buffer != NULL, "buffer not set\n" );
    ok( pos.node != NULL, "node not set\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsSetReaderPosition(void)
{
    WS_HEAP *heap;
    WS_XML_READER *reader;
    WS_XML_BUFFER *buf1, *buf2;
    WS_XML_NODE_POSITION pos;
    HRESULT hr;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetReaderPosition( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf1, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, buf1, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetReaderPosition( reader, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( pos.buffer == buf1, "wrong buffer\n" );
    ok( pos.node != NULL, "node not set\n" );

    hr = WsSetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* different buffer */
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    pos.buffer = buf2;
    hr = WsSetReaderPosition( reader, &pos, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_entities(void)
{
    static const char str1[] = "<t>&#xA</t>";
    static const char str2[] = "<t>&#xA;</t>";
    static const char str3[] = "<t>&#xa;</t>";
    static const char str4[] = "<t>&#xaaaa;</t>";
    static const char str5[] = "<t>&#xaaaaa;</t>";
    static const char str6[] = "<t>&1</t>";
    static const char str7[] = "<t>&1;</t>";
    static const char str8[] = "<t>&1111;</t>";
    static const char str9[] = "<t>&11111;</t>";
    static const char str10[] = "<t>&lt;</t>";
    static const char str11[] = "<t>&gt;</t>";
    static const char str12[] = "<t>&quot;</t>";
    static const char str13[] = "<t>&amp;</t>";
    static const char str14[] = "<t>&apos;</t>";
    static const char str15[] = "<t>&sopa;</t>";
    static const char str16[] = "<t>&#;</t>";
    static const char str17[] = "<t>&;</t>";
    static const char str18[] = "<t>&&</t>";
    static const char str19[] = "<t>&</t>";
    static const char str20[] = "<t>&#xaaaaaa;</t>";
    static const char str21[] = "<t>&#xd7ff;</t>";
    static const char str22[] = "<t>&#xd800;</t>";
    static const char str23[] = "<t>&#xdfff;</t>";
    static const char str24[] = "<t>&#xe000;</t>";
    static const char str25[] = "<t>&#xfffe;</t>";
    static const char str26[] = "<t>&#xffff;</t>";
    static const char str27[] = "<t>&LT;</t>";
    static const char str28[] = "<t>&#x0;</t>";
    static const char str29[] = "<t>&#0;</t>";
    static const char str30[] = "<t>&#65;</t>";
    static const char str31[] = "<t>&#65393;</t>";
    static const char str32[] = "<t>&#x10ffff;</t>";
    static const char str33[] = "<t>&#x110000;</t>";
    static const char str34[] = "<t>&#1114111;</t>";
    static const char str35[] = "<t>&#1114112;</t>";
    static const char res4[] = {0xea, 0xaa, 0xaa, 0x00};
    static const char res5[] = {0xf2, 0xaa, 0xaa, 0xaa, 0x00};
    static const char res21[] = {0xed, 0x9f, 0xbf, 0x00};
    static const char res24[] = {0xee, 0x80, 0x80, 0x00};
    static const char res31[] = {0xef, 0xbd, 0xb1, 0x00};
    static const char res32[] = {0xf4, 0x8f, 0xbf, 0xbf, 0x00};
    static const struct
    {
        const char *str;
        HRESULT     hr;
        const char *res;
    }
    tests[] =
    {
        { str1, WS_E_INVALID_FORMAT },
        { str2, S_OK, "\n" },
        { str3, S_OK, "\n" },
        { str4, S_OK, res4 },
        { str5, S_OK, res5 },
        { str6, WS_E_INVALID_FORMAT },
        { str7, WS_E_INVALID_FORMAT },
        { str8, WS_E_INVALID_FORMAT },
        { str9, WS_E_INVALID_FORMAT },
        { str10, S_OK, "<" },
        { str11, S_OK, ">" },
        { str12, S_OK, "\"" },
        { str13, S_OK, "&" },
        { str14, S_OK, "'" },
        { str15, WS_E_INVALID_FORMAT },
        { str16, WS_E_INVALID_FORMAT },
        { str17, WS_E_INVALID_FORMAT },
        { str18, WS_E_INVALID_FORMAT },
        { str19, WS_E_INVALID_FORMAT },
        { str20, WS_E_INVALID_FORMAT },
        { str21, S_OK, res21 },
        { str22, WS_E_INVALID_FORMAT },
        { str23, WS_E_INVALID_FORMAT },
        { str24, S_OK, res24 },
        { str25, WS_E_INVALID_FORMAT },
        { str26, WS_E_INVALID_FORMAT },
        { str27, WS_E_INVALID_FORMAT },
        { str28, WS_E_INVALID_FORMAT },
        { str29, WS_E_INVALID_FORMAT },
        { str30, S_OK, "A" },
        { str31, S_OK, res31 },
        { str32, S_OK, res32 },
        { str33, WS_E_INVALID_FORMAT },
        { str34, S_OK, res32 },
        { str35, WS_E_INVALID_FORMAT },
    };
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    const WS_XML_UTF8_TEXT *utf8;
    ULONG i;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_input( reader, tests[i].str, strlen(tests[i].str) );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsReadNode( reader, NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr != S_OK) continue;

        hr = WsGetReaderNode( reader, &node, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        utf8 = (const WS_XML_UTF8_TEXT *)((const WS_XML_TEXT_NODE *)node)->text;
        ok( utf8->value.length == strlen(tests[i].res), "%lu: got %lu\n", i, utf8->value.length );
        ok( !memcmp( utf8->value.bytes, tests[i].res, strlen(tests[i].res) ), "%lu: wrong data\n", i );
    }

    hr = set_input( reader, "<t a='&#xA;&#xA;'/>", sizeof("<t a='&#xA;&#xA;'/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    utf8 = (const WS_XML_UTF8_TEXT *)((const WS_XML_ELEMENT_NODE *)node)->attributes[0]->value;
    ok( utf8->value.length == 2, "got %lu\n", utf8->value.length );
    ok( !memcmp( utf8->value.bytes, "\n\n", 2 ), "wrong data\n" );

    WsFreeReader( reader );
}

static void test_field_options(void)
{
    static const char xml[] =
        "<t xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\"><wsz i:nil=\"true\"/>"
        "<s i:nil=\"true\"/></t>";
    HRESULT hr;
    WS_HEAP *heap;
    WS_XML_READER *reader;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f2, f3, f4, f5, *fields[4], *fields2[1];
    WS_XML_STRING ns = {0, NULL}, str_wsz = {3, (BYTE *)"wsz"}, str_s = {1, (BYTE *)"s"};
    WS_XML_STRING str_int32 = {5, (BYTE *)"int32"}, str_guid = {4, (BYTE *)"guid"};
    WS_DEFAULT_VALUE def_val;
    INT32 val_int32;
    struct s
    {
        INT32 int32;
    };
    struct test
    {
        WCHAR    *wsz;
        struct s *s;
        INT32     int32;
        GUID      guid;
    } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, xml, sizeof(xml) - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f.localName = &str_wsz;
    f.ns        = &ns;
    f.type      = WS_WSZ_TYPE;
    f.options   = WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE;
    fields[0] = &f;

    memset( &f3, 0, sizeof(f3) );
    f3.mapping   = WS_ELEMENT_FIELD_MAPPING;
    f3.localName = &str_int32;
    f3.ns        = &ns;
    f3.type      = WS_INT32_TYPE;
    fields2[0] = &f3;

    memset( &s2, 0, sizeof(s2) );
    s2.size       = sizeof(struct s);
    s2.alignment  = TYPE_ALIGNMENT(struct s);
    s2.fields     = fields2;
    s2.fieldCount = 1;

    memset( &f2, 0, sizeof(f2) );
    f2.mapping         = WS_ELEMENT_FIELD_MAPPING;
    f2.localName       = &str_s;
    f2.ns              = &ns;
    f2.type            = WS_STRUCT_TYPE;
    f2.typeDescription = &s2;
    f2.offset          = FIELD_OFFSET(struct test, s);
    f2.options         = WS_FIELD_POINTER|WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE;
    fields[1] = &f2;

    val_int32 = -1;
    def_val.value     = &val_int32;
    def_val.valueSize = sizeof(val_int32);

    memset( &f4, 0, sizeof(f4) );
    f4.mapping      = WS_ELEMENT_FIELD_MAPPING;
    f4.localName    = &str_int32;
    f4.ns           = &ns;
    f4.type         = WS_INT32_TYPE;
    f4.offset       = FIELD_OFFSET(struct test, int32);
    f4.options      = WS_FIELD_OPTIONAL;
    f4.defaultValue = &def_val;
    fields[2] = &f4;

    memset( &f5, 0, sizeof(f5) );
    f5.mapping      = WS_ELEMENT_FIELD_MAPPING;
    f5.localName    = &str_guid;
    f5.ns           = &ns;
    f5.type         = WS_GUID_TYPE;
    f5.offset       = FIELD_OFFSET(struct test, guid);
    f5.options      = WS_FIELD_OPTIONAL;
    fields[3] = &f5;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 4;

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !test->wsz, "wsz is set\n" );
    ok( !test->s, "s is set\n" );
    ok( test->int32 == -1, "got %d\n", test->int32 );
    ok( IsEqualGUID( &test->guid, &guid_null ), "wrong guid\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsReadBytes(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    const WS_XML_TEXT_NODE *text;
    const WS_XML_UTF8_TEXT *utf8;
    BYTE buf[4];
    ULONG count;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBytes( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadBytes( reader, NULL, 0, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t>dGV4dA==</t>", sizeof("<t>dGV4dA==</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBytes( reader, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>dGV4dA==</t>", sizeof("<t>dGV4dA==</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadBytes( reader, buf, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>dGV4dA==</t>", sizeof("<t>dGV4dA==</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadBytes( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    count = 0xdeadbeef;
    hr = WsReadBytes( reader, NULL, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadBytes( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( buf, "te", 2 ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
    utf8 = (const WS_XML_UTF8_TEXT *)text->text;
    ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
    ok( utf8->value.length == 8, "got %lu\n", utf8->value.length );
    ok( !memcmp( utf8->value.bytes, "dGV4dA==", 8 ), "wrong data\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( buf, "xt", 2 ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );

    count = 0xdeadbeef;
    hr = WsReadBytes( reader, buf, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", text->node.nodeType );

    WsFreeReader( reader );
}

static void test_WsReadChars(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    const WS_XML_TEXT_NODE *text;
    const WS_XML_UTF8_TEXT *utf8;
    unsigned char buf[4];
    WCHAR bufW[4];
    ULONG count;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadChars( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadChars( reader, NULL, 0, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadChars( reader, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadChars( reader, bufW, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadChars( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    count = 0xdeadbeef;
    hr = WsReadChars( reader, NULL, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadChars( reader, bufW, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadChars( reader, bufW, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadChars( reader, bufW, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadChars( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadChars( reader, bufW, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( bufW, L"te", 2 * sizeof(WCHAR) ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
    utf8 = (const WS_XML_UTF8_TEXT *)text->text;
    ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
    ok( utf8->value.length == 4, "got %lu\n", utf8->value.length );
    ok( !memcmp( utf8->value.bytes, "text", 4 ), "wrong data\n" );

    /* continue reading in a different encoding */
    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( buf, "xt", 2 ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );

    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", text->node.nodeType );

    WsFreeReader( reader );
}

static void test_WsReadCharsUtf8(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    const WS_XML_TEXT_NODE *text;
    const WS_XML_UTF8_TEXT *utf8;
    BYTE buf[4];
    ULONG count;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadCharsUtf8( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadCharsUtf8( reader, NULL, 0, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadCharsUtf8( reader, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadCharsUtf8( reader, buf, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t>text</t>", sizeof("<t>text</t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, NULL, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );
    ok( !buf[0], "wrong data\n" );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, NULL, 0, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( buf, "te", 2 ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
    utf8 = (const WS_XML_UTF8_TEXT *)text->text;
    ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
    ok( utf8->value.length == 4, "got %lu\n", utf8->value.length );
    ok( !memcmp( utf8->value.bytes, "text", 4 ), "wrong data\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 2, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( !memcmp( buf, "xt", 2 ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );

    count = 0xdeadbeef;
    hr = WsReadCharsUtf8( reader, buf, 1, &count, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !count, "got %lu\n", count );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text = (const WS_XML_TEXT_NODE *)node;
    ok( text->node.nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", text->node.nodeType );

    WsFreeReader( reader );
}

static void test_WsReadQualifiedName(void)
{
    static const char utf8[] = {'<','a','>',0xc3,0xab,'<','/','a','>',0};
    static const char localname_utf8[] = {0xc3,0xab,0};
    WS_XML_STRING prefix, localname, ns;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    HRESULT hr;
    BOOL found;
    ULONG i;
    static const struct
    {
        const char *str;
        HRESULT     hr;
        const char *prefix;
        const char *localname;
        const char *ns;
    } tests[] =
    {
        { "<a></a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a> </a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a>:</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a>t</a>", S_OK, "", "t", "" },
        { "<a>p:</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a>p:t</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a>:t</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">p:t</a>", S_OK, "p", "t", "ns" },
        { "<a xmlns:p=\"ns\">p:t:</a>", S_OK, "p", "t:", "ns" },
        { "<a xmlns:p=\"ns\">p:</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">:t</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">:</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">t</a>", S_OK, "", "t", "" },
        { "<a xmlns:p=\"ns\"> </a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\"></a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">p:t u</a>", S_OK, "p", "t u", "ns" },
        { utf8, S_OK, "", localname_utf8, "" },
        { "<a> t </a>", S_OK, "", "t", "" },
        { "<a xmlns:p=\"ns\"> p:t</a>", S_OK, "p", "t", "ns" },
        { "<a xmlns:p=\"ns\">p :t</a>", WS_E_INVALID_FORMAT, NULL, NULL, NULL },
        { "<a xmlns:p=\"ns\">p: t</a>", S_OK, "p", " t", "ns" },
    };

    hr = WsReadQualifiedName( NULL, NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadQualifiedName( reader, NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadQualifiedName( reader, heap, NULL, NULL, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t/>", sizeof("<t/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadQualifiedName( reader, heap, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = set_input( reader, "<t/>", sizeof("<t/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadQualifiedName( reader, heap, NULL, &localname, NULL, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        hr = set_input( reader, tests[i].str, strlen(tests[i].str) );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsReadStartElement( reader, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        prefix.length = localname.length = ns.length = 0xdeadbeef;
        prefix.bytes = localname.bytes = ns.bytes = (BYTE *)0xdeadbeef;

        hr = WsReadQualifiedName( reader, heap, &prefix, &localname, &ns, NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (tests[i].hr == S_OK && hr == S_OK)
        {
            ok( prefix.length == strlen( tests[i].prefix ), "%lu: got %lu\n", i, prefix.length );
            ok( !memcmp( prefix.bytes, tests[i].prefix, prefix.length ), "%lu: wrong data\n", i );

            ok( localname.length == strlen( tests[i].localname ), "%lu: got %lu\n", i, localname.length );
            ok( !memcmp( localname.bytes, tests[i].localname, localname.length ), "%lu: wrong data\n", i );

            ok( ns.length == strlen( tests[i].ns ), "%lu: got %lu\n", i, ns.length );
            ok( !memcmp( ns.bytes, tests[i].ns, ns.length ), "%lu: wrong data\n", i );
        }
        else if (tests[i].hr != S_OK)
        {
            ok( prefix.length == 0xdeadbeef, "got %lu\n", prefix.length );
            ok( prefix.bytes == (BYTE *)0xdeadbeef, "got %p\n", prefix.bytes );

            ok( localname.length == 0xdeadbeef, "got %lu\n", localname.length );
            ok( localname.bytes == (BYTE *)0xdeadbeef, "got %p\n", localname.bytes );

            ok( ns.length == 0xdeadbeef, "got %lu\n", ns.length );
            ok( ns.bytes == (BYTE *)0xdeadbeef, "got %p\n", ns.bytes );
        }
    }

    WsFreeHeap( heap );
    WsFreeReader( reader );
}

static void test_WsReadAttribute(void)
{
    WS_XML_STRING localname = {1, (BYTE *)"a"}, ns = {0, NULL};
    WS_XML_READER *reader;
    WS_ATTRIBUTE_DESCRIPTION desc;
    WS_HEAP *heap;
    UINT32 *val;
    BOOL found;
    HRESULT hr;

    hr = WsReadAttribute( NULL, NULL, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadAttribute( reader, NULL, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    desc.attributeLocalName = &localname;
    desc.attributeNs        = &ns;
    desc.type               = WS_UINT32_TYPE;
    desc.typeDescription    = NULL;
    hr = WsReadAttribute( reader, &desc, 0, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadAttribute( reader, &desc, WS_READ_REQUIRED_POINTER, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 8, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadAttribute( reader, &desc, WS_READ_REQUIRED_POINTER, heap, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadAttribute( reader, &desc, WS_READ_REQUIRED_POINTER, heap, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    prepare_struct_type_test( reader, "<t a='1'>" );
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    val = NULL;
    hr = WsReadAttribute( reader, &desc, WS_READ_REQUIRED_POINTER, heap, &val, sizeof(val), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val != NULL, "val not set\n" );
    ok( *val == 1, "got %u\n", *val );

    WsFreeHeap( heap );
    WsFreeReader( reader );
}

static void test_WsSkipNode(void)
{
    const WS_XML_NODE *node;
    WS_XML_READER *reader;
    HRESULT hr;

    hr = WsSkipNode( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSkipNode( reader, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<t><u></u></t>", sizeof("<t><u></u></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* BOF */
    hr = WsSkipNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    /* element */
    hr = WsSkipNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* EOF */
    hr = WsSkipNode( reader, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = set_input( reader, "<!--comment--><t></t>", sizeof("<!--comment--><t></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* non-element */
    hr = WsSkipNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_COMMENT, "got %u\n", node->nodeType );

    hr = WsSkipNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    WsFreeReader( reader );
}

static HRESULT set_input_bin( WS_XML_READER *reader, const char *data, ULONG size, WS_XML_DICTIONARY *dict )
{
    WS_XML_READER_BINARY_ENCODING bin = {{WS_XML_READER_ENCODING_TYPE_BINARY}, dict};
    WS_XML_READER_BUFFER_INPUT buf;

    buf.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    buf.encodedData     = (void *)data;
    buf.encodedDataSize = size;
    return WsSetInput( reader, &bin.encoding, &buf.input, NULL, 0, NULL );
}

static const WS_XML_TEXT_NODE *read_text_node( WS_XML_READER *reader )
{
    const WS_XML_NODE *node;
    if (WsReadNode( reader, NULL ) != S_OK) return NULL;
    if (WsReadNode( reader, NULL ) != S_OK) return NULL;
    if (WsGetReaderNode( reader, &node, NULL ) != S_OK) return NULL;
    if (node->nodeType != WS_XML_NODE_TYPE_TEXT) return NULL;
    return (const WS_XML_TEXT_NODE *)node;
}

static void test_binary_encoding(void)
{
    WS_XML_STRING str_s = {1, (BYTE *)"s"}, str_s_a = {3, (BYTE *)"s_a"}, str_s_b = {3, (BYTE *)"s_b"};
    WS_XML_STRING str_a = {1, (BYTE *)"a"}, str_b = {1, (BYTE *)"b"};
    static WS_XML_STRING localname = {1, (BYTE *)"t"}, ns = {0, NULL};
    static const char test[] =
        {0x40,0x01,'t',0x01};
    static const char test2[] =
        {0x6d,0x01,'t',0x09,0x01,'p',0x02,'n','s',0x01};
    static const char test3[] =
        {0x41,0x02,'p','2',0x01,'t',0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char test4[] =
        {0x41,0x02,'p','2',0x01,'t',0x09,0x02,'p','2',0x02,'n','s',0x99,0x04,'t','e','s','t'};
    static const char test5[] =
        {0x40,0x01,'t',0x9f,0x01,'a'};
    static const char test6[] =
        {0x40,0x01,'t',0xa0,0x01,0x00,'a',0x9f,0x01,'b'};
    static const char test7[] =
        {0x40,0x01,'t',0xb5,0xff,0xff,0xff,0xff};
    static const char test8[] =
        {0x40,0x01,'t',0xb5,0x00,0x00,0x00,0x00};
    static const char test9[] =
        {0x40,0x01,'t',0x81};
    static const char test10[] =
        {0x40,0x01,'t',0x83};
    static const char test11[] =
        {0x40,0x01,'t',0x85};
    static const char test12[] =
        {0x40,0x01,'t',0x87};
    static const char test13[] =
        {0x40,0x01,'t',0x89,0xff};
    static const char test14[] =
        {0x40,0x01,'t',0x8b,0xff,0xff};
    static const char test15[] =
        {0x40,0x01,'t',0x8d,0xff,0xff,0xff,0xff};
    static const char test16[] =
        {0x40,0x01,'t',0x8f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    static const char test17[] =
        {0x40,0x01,'t',0x93,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const char test18[] =
        {0x40,0x01,'t',0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const char test19[] =
        {0x40,0x01,'t',0x99,0x01,0x61};
    static const char test20[] =
        {0x40,0x01,'t',0x9b,0x01,0x00,0x61};
    static const char test21[] =
        {0x40,0x01,'t',0x9d,0x01,0x00,0x00,0x00,0x61};
    static const char test22[] =
        {0x40,0x01,'t',0x9f,0x01,0x61};
    static const char test23[] =
        {0x40,0x01,'t',0xa1,0x01,0x00,0x61};
    static const char test24[] =
        {0x40,0x01,'t',0xa3,0x01,0x00,0x00,0x00,0x61};
    static const char test25[] =
        {0x40,0x01,'t',0xa9};
    static const char test26[] =
        {0x40,0x01,'t',0xab,0x0c};
    static const char test27[] =
        {0x40,0x01,'t',0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const char test28[] =
        {0x40,0x01,'t',0xb1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const char test29[] =
        {0x40,0x01,'t',0xb3,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const char test30[] =
        {0x40,0x01,'t',0x08,0x02,'n','s',0x01};
    static const char test31[] =
        {0x40,0x01,'t',0x09,0x01,'p',0x02,'n','s',0x01};
    static const char test32[] =
        {0x40,0x01,'t',0xb3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    static const char test100[] =
        {0x40,0x01,'t',0x04,0x01,'t',0x98,0x00,0x01};
    static const char test101[] =
        {0x40,0x01,'t',0x35,0x01,'t',0x98,0x00,0x09,0x01,'p',0x02,'n','s',0x01};
    static const char test102[] =
        {0x40,0x01,'t',0x05,0x02,'p','2',0x01,'t',0x98,0x00,0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char test103[] =
        {0x40,0x01,'t',0x05,0x02,'p','2',0x01,'t',0x98,0x04,'t','e','s','t',0x09,0x02,'p','2',0x02,'n','s',0x01};
    static const char test200[] =
        {0x02,0x07,'c','o','m','m','e','n','t'};
    static const char test_endelem[] =
        { 0x40, 0x01, 't', 0x40, 0x01, 'a', 0x83, 0x40, 0x01, 's', 0x40, 0x03, 's', '_', 'a', 0x82, 0x01, 0x01,
          0x40, 0x01, 'b', 0x83, 0x01 };
    const WS_XML_NODE *node;
    const WS_XML_TEXT_NODE *text_node;
    const WS_XML_ELEMENT_NODE *elem;
    const WS_XML_ATTRIBUTE *attr;
    const WS_XML_UTF8_TEXT *utf8_text;
    const WS_XML_BASE64_TEXT *base64_text;
    const WS_XML_INT32_TEXT *int32_text;
    const WS_XML_INT64_TEXT *int64_text;
    const WS_XML_DOUBLE_TEXT *double_text;
    const WS_XML_DATETIME_TEXT *datetime_text;
    const WS_XML_BOOL_TEXT *bool_text;
    const WS_XML_UNIQUE_ID_TEXT *unique_id_text;
    const WS_XML_GUID_TEXT *guid_text;
    const WS_XML_UINT64_TEXT *uint64_text;
    const WS_XML_COMMENT_NODE *comment;
    WS_XML_DICTIONARY *dict;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    BOOL found;
    HRESULT hr;
    WS_STRUCT_DESCRIPTION s, s2;
    WS_FIELD_DESCRIPTION f, f1[3], f2[2], *fields[3], *fields2[2];
    struct typetest
    {
        WS_BYTES data;
    } *typetest;
    struct typetest2
    {
        UINT32 val;
    } *typetest2;
    struct typetest3
    {
        UINT64 val;
    } *typetest3;
    struct s
    {
        INT32 s_a;
        INT32 s_b;
    };
    struct test
    {
        INT32     a;
        struct s  *s;
        INT32     b;
    } *test_struct;

    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_1, &dict, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* short element */
    hr = set_input_bin( reader, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->prefix->bytes == NULL, "bytes set\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->localName->dictionary != NULL, "dictionary not set\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( !elem->attributeCount, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* single character prefix element */
    hr = set_input_bin( reader, test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->prefix->length == 1, "got %lu\n", elem->prefix->length );
    ok( !memcmp( elem->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* element */
    hr = set_input_bin( reader, test3, sizeof(test3), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->prefix->length == 2, "got %lu\n", elem->prefix->length );
    ok( !memcmp( elem->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* element with text */
    hr = set_input_bin( reader, test4, sizeof(test4), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->prefix->length == 2, "got %lu\n", elem->prefix->length );
    ok( !memcmp( elem->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );
    text_node = (const WS_XML_TEXT_NODE *)node;
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (const WS_XML_UTF8_TEXT *)text_node->text;
    ok( utf8_text->value.length == 4, "got %lu\n", utf8_text->value.length );
    ok( !memcmp( utf8_text->value.bytes, "test", 4 ), "wrong text\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* bool text, TRUE */
    hr = set_input_bin( reader, test7, sizeof(test7), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BOOL, "got %u\n", text_node->text->textType );
    bool_text = (WS_XML_BOOL_TEXT *)text_node->text;
    ok( bool_text->value == TRUE, "got %d\n", bool_text->value );

    /* bool text, FALSE */
    hr = set_input_bin( reader, test8, sizeof(test8), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BOOL, "got %u\n", text_node->text->textType );
    bool_text = (WS_XML_BOOL_TEXT *)text_node->text;
    ok( !bool_text->value, "got %d\n", bool_text->value );

    /* zero text */
    hr = set_input_bin( reader, test9, sizeof(test9), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT32, "got %u\n", text_node->text->textType );
    int32_text = (WS_XML_INT32_TEXT *)text_node->text;
    ok( !int32_text->value, "got %d\n", int32_text->value );

    /* one text */
    hr = set_input_bin( reader, test10, sizeof(test10), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT32, "got %u\n", text_node->text->textType );
    int32_text = (WS_XML_INT32_TEXT *)text_node->text;
    ok( int32_text->value == 1, "got %d\n", int32_text->value );

    /* false text */
    hr = set_input_bin( reader, test11, sizeof(test11), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BOOL, "got %u\n", text_node->text->textType );
    bool_text = (WS_XML_BOOL_TEXT *)text_node->text;
    ok( !bool_text->value, "got %d\n", bool_text->value );

    /* true text */
    hr = set_input_bin( reader, test12, sizeof(test12), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BOOL, "got %u\n", text_node->text->textType );
    bool_text = (WS_XML_BOOL_TEXT *)text_node->text;
    ok( bool_text->value == TRUE, "got %d\n", bool_text->value );

    /* int32 text, int8 record */
    hr = set_input_bin( reader, test13, sizeof(test13), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT32, "got %u\n", text_node->text->textType );
    int32_text = (WS_XML_INT32_TEXT *)text_node->text;
    ok( int32_text->value == -1, "got %d\n", int32_text->value );

    /* int32 text, int16 record */
    hr = set_input_bin( reader, test14, sizeof(test14), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT32, "got %u\n", text_node->text->textType );
    int32_text = (WS_XML_INT32_TEXT *)text_node->text;
    ok( int32_text->value == -1, "got %d\n", int32_text->value );

    /* int32 text, int32 record */
    hr = set_input_bin( reader, test15, sizeof(test15), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT32, "got %u\n", text_node->text->textType );
    int32_text = (WS_XML_INT32_TEXT *)text_node->text;
    ok( int32_text->value == -1, "got %d\n", int32_text->value );

    /* int64 text, int64 record */
    hr = set_input_bin( reader, test16, sizeof(test16), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_INT64, "got %u\n", text_node->text->textType );
    int64_text = (WS_XML_INT64_TEXT *)text_node->text;
    ok( int64_text->value == -1, "got %s\n", wine_dbgstr_longlong(int64_text->value) );

    /* double text */
    hr = set_input_bin( reader, test17, sizeof(test17), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_DOUBLE, "got %u\n", text_node->text->textType );
    double_text = (WS_XML_DOUBLE_TEXT *)text_node->text;
    ok( !double_text->value, "got %s\n", wine_dbgstr_longlong(double_text->value) );

    /* datetime text */
    hr = set_input_bin( reader, test18, sizeof(test18), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_DATETIME, "got %u\n", text_node->text->textType );
    datetime_text = (WS_XML_DATETIME_TEXT *)text_node->text;
    ok( !datetime_text->value.ticks, "got %s\n", wine_dbgstr_longlong(datetime_text->value.ticks) );
    ok( datetime_text->value.format == WS_DATETIME_FORMAT_NONE, "got %u\n", datetime_text->value.format );

    /* utf8 text, chars8 record */
    hr = set_input_bin( reader, test19, sizeof(test19), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (WS_XML_UTF8_TEXT *)text_node->text;
    ok( utf8_text->value.length == 1, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes[0] == 'a', "got %02x\n", utf8_text->value.bytes[0] );

    /* utf8 text, chars16 record */
    hr = set_input_bin( reader, test20, sizeof(test20), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (WS_XML_UTF8_TEXT *)text_node->text;
    ok( utf8_text->value.length == 1, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes[0] == 'a', "got %02x\n", utf8_text->value.bytes[0] );

    /* utf8 text, chars32 record */
    hr = set_input_bin( reader, test21, sizeof(test21), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (WS_XML_UTF8_TEXT *)text_node->text;
    ok( utf8_text->value.length == 1, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes[0] == 'a', "got %02x\n", utf8_text->value.bytes[0] );

    /* base64 text, bytes8 record */
    hr = set_input_bin( reader, test22, sizeof(test22), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BASE64, "got %u\n", text_node->text->textType );
    base64_text = (WS_XML_BASE64_TEXT *)text_node->text;
    ok( base64_text->length == 1, "got %lu\n", base64_text->length );
    ok( base64_text->bytes[0] == 'a', "got %02x\n", base64_text->bytes[0] );

    /* base64 text, bytes16 record */
    hr = set_input_bin( reader, test23, sizeof(test23), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BASE64, "got %u\n", text_node->text->textType );
    base64_text = (WS_XML_BASE64_TEXT *)text_node->text;
    ok( base64_text->length == 1, "got %lu\n", base64_text->length );
    ok( base64_text->bytes[0] == 'a', "got %02x\n", base64_text->bytes[0] );

    /* base64 text, bytes32 record */
    hr = set_input_bin( reader, test24, sizeof(test24), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BASE64, "got %u\n", text_node->text->textType );
    base64_text = (WS_XML_BASE64_TEXT *)text_node->text;
    ok( base64_text->length == 1, "got %lu\n", base64_text->length );
    ok( base64_text->bytes[0] == 'a', "got %02x\n", base64_text->bytes[0] );

    /* empty text */
    hr = set_input_bin( reader, test25, sizeof(test25), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (WS_XML_UTF8_TEXT *)text_node->text;
    ok( !utf8_text->value.length, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes != NULL, "bytes not set\n" );

    /* dictionary text */
    hr = set_input_bin( reader, test26, sizeof(test26), dict );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text_node->text->textType );
    utf8_text = (WS_XML_UTF8_TEXT *)text_node->text;
    ok( utf8_text->value.length == 2, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes[0] == 'T', "got %02x\n", utf8_text->value.bytes[0] );
    ok( utf8_text->value.bytes[1] == 'o', "got %02x\n", utf8_text->value.bytes[0] );

    /* unique id text */
    hr = set_input_bin( reader, test27, sizeof(test27), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UNIQUE_ID, "got %u\n", text_node->text->textType );
    unique_id_text = (WS_XML_UNIQUE_ID_TEXT *)text_node->text;
    ok( IsEqualGUID( &unique_id_text->value, &guid_null ), "wrong data\n" );

    /* guid text */
    hr = set_input_bin( reader, test28, sizeof(test28), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_GUID, "got %u\n", text_node->text->textType );
    guid_text = (WS_XML_GUID_TEXT *)text_node->text;
    ok( IsEqualGUID( &guid_text->value, &guid_null ), "wrong data\n" );

    /* uint64 text */
    hr = set_input_bin( reader, test29, sizeof(test29), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    text_node = read_text_node( reader );
    ok( text_node != NULL, "no text\n" );
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_UINT64, "got %u\n", text_node->text->textType );
    uint64_text = (WS_XML_UINT64_TEXT *)text_node->text;
    ok( uint64_text->value == 1, "got %s\n", wine_dbgstr_longlong(uint64_text->value) );

    /* short xmlns attribute */
    hr = set_input_bin( reader, test30, sizeof(test30), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->prefix->bytes == NULL, "bytes set\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
    ok( attr->prefix->bytes == NULL, "bytes set\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    /* xmlns attribute */
    hr = set_input_bin( reader, test31, sizeof(test31), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->prefix->bytes == NULL, "bytes set\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    /* short attribute */
    hr = set_input_bin( reader, test100, sizeof(test100), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !attr->ns->length, "got %lu\n", attr->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8_text = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8_text->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8_text->text.textType );
    ok( !utf8_text->value.length, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes != NULL, "bytes not set\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* single character prefix attribute */
    hr = set_input_bin( reader, test101, sizeof(test101), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "ns not set\n" );
    ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8_text = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8_text->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8_text->text.textType );
    ok( !utf8_text->value.length, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes != NULL, "bytes not set\n" );
    attr = elem->attributes[1];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* attribute */
    hr = set_input_bin( reader, test102, sizeof(test102), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "ns not set\n" );
    ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8_text = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8_text->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8_text->text.textType );
    ok( !utf8_text->value.length, "got %lu\n", utf8_text->value.length );
    ok( utf8_text->value.bytes != NULL, "bytes not set\n" );
    attr = elem->attributes[1];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* attribute with value */
    hr = set_input_bin( reader, test103, sizeof(test103), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "ns not set\n" );
    ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8_text = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8_text->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8_text->text.textType );
    ok( utf8_text->value.length == 4, "got %lu\n", utf8_text->value.length );
    ok( !memcmp( utf8_text->value.bytes, "test", 4 ), "wrong value\n" );
    attr = elem->attributes[1];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* comment */
    hr = set_input_bin( reader, test200, sizeof(test200), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_COMMENT, "got %u\n", node->nodeType );
    comment = (const WS_XML_COMMENT_NODE *)node;
    ok( comment->value.length == 7, "got %lu\n", comment->value.length );
    ok( !memcmp( comment->value.bytes, "comment", 7 ), "wrong data\n" );

    hr = set_input_bin( reader, test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( found == TRUE, "got %d\n", found );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* element with byte record text */
    hr = set_input_bin( reader, test5, sizeof(test5), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );
    text_node = (const WS_XML_TEXT_NODE *)node;
    ok( text_node->text->textType == WS_XML_TEXT_TYPE_BASE64, "got %u\n", text_node->text->textType );
    base64_text = (const WS_XML_BASE64_TEXT *)text_node->text;
    ok( base64_text->length == 1, "got %lu\n", base64_text->length );
    ok( base64_text->bytes[0] == 'a', "wrong data %02x\n", base64_text->bytes[0] );

    /* element with mixed byte record text */
    hr = set_input_bin( reader, test6, sizeof(test6), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 8, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping      = WS_ELEMENT_FIELD_MAPPING;
    f.localName    = &localname;
    f.ns           = &ns;
    f.type         = WS_BYTES_TYPE;
    f.offset       = FIELD_OFFSET(struct typetest, data);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct typetest);
    s.alignment  = TYPE_ALIGNMENT(struct typetest);
    s.fields     = fields;
    s.fieldCount = 1;

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &typetest, sizeof(typetest), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( typetest->data.length == 2, "got %lu\n", typetest->data.length );
    ok( !memcmp( typetest->data.bytes, "ab", 2 ), "wrong data\n" );

    /* record value too large for description type */
    hr = set_input_bin( reader, test32, sizeof(test32), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 8, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping      = WS_ELEMENT_FIELD_MAPPING;
    f.localName    = &localname;
    f.ns           = &ns;
    f.type         = WS_UINT32_TYPE;
    f.offset       = FIELD_OFFSET(struct typetest2, val);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct typetest2);
    s.alignment  = TYPE_ALIGNMENT(struct typetest2);
    s.fields     = fields;
    s.fieldCount = 1;

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &typetest2, sizeof(typetest2), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    /* record value too small for description type */
    hr = set_input_bin( reader, test16, sizeof(test16), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 8, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping      = WS_ELEMENT_FIELD_MAPPING;
    f.localName    = &localname;
    f.ns           = &ns;
    f.type         = WS_UINT64_TYPE;
    f.offset       = FIELD_OFFSET(struct typetest3, val);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct typetest3);
    s.alignment  = TYPE_ALIGNMENT(struct typetest3);
    s.fields     = fields;
    s.fieldCount = 1;

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &typetest3, sizeof(typetest3), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %#lx\n", hr );

    /* Test optional ending field on a nested struct. */
    hr = set_input_bin( reader, test_endelem, sizeof(test_endelem), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f1[0], 0, sizeof(f1[0]) );
    f1[0].mapping   = WS_ELEMENT_FIELD_MAPPING;
    f1[0].localName = &str_a;
    f1[0].ns        = &ns;
    f1[0].type      = WS_INT32_TYPE;
    fields[0] = &f1[0];

    memset( &f1[1], 0, sizeof(f1[1]) );
    f1[1].mapping         = WS_ELEMENT_FIELD_MAPPING;
    f1[1].localName       = &str_s;
    f1[1].ns              = &ns;
    f1[1].type            = WS_STRUCT_TYPE;
    f1[1].typeDescription = &s2;
    f1[1].options         = WS_FIELD_POINTER;
    f1[1].offset          = FIELD_OFFSET(struct test, s);
    fields[1] = &f1[1];

    memset( &f1[2], 0, sizeof(f1[2]) );
    f1[2].mapping      = WS_ELEMENT_FIELD_MAPPING;
    f1[2].localName    = &str_b;
    f1[2].ns           = &ns;
    f1[2].type         = WS_INT32_TYPE;
    f1[2].offset       = FIELD_OFFSET(struct test, b);
    fields[2] = &f1[2];

    memset( &f2[0], 0, sizeof(f2[0]) );
    f2[0].mapping      = WS_ELEMENT_FIELD_MAPPING;
    f2[0].localName    = &str_s_a;
    f2[0].ns           = &ns;
    f2[0].type         = WS_INT32_TYPE;
    fields2[0] = &f2[0];

    memset( &f2[1], 0, sizeof(f2[1]) );
    f2[1].mapping      = WS_ELEMENT_FIELD_MAPPING;
    f2[1].localName    = &str_s_b;
    f2[1].ns           = &ns;
    f2[1].type         = WS_INT32_TYPE;
    f2[1].offset       = FIELD_OFFSET(struct s, s_b);
    f2[1].options      = WS_FIELD_OPTIONAL;
    fields2[1] = &f2[1];

    memset( &s2, 0, sizeof(s2) );
    s2.size       = sizeof(struct s);
    s2.alignment  = TYPE_ALIGNMENT(struct s);
    s2.fields     = fields2;
    s2.fieldCount = 2;

    memset( &s, 0, sizeof(s) );
    s.size       = sizeof(struct test);
    s.alignment  = TYPE_ALIGNMENT(struct test);
    s.fields     = fields;
    s.fieldCount = 3;

    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test_struct, sizeof(test_struct), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test_struct->a == 1, "got %d\n", test_struct->a );
    ok( !!test_struct->s, "s is not set\n" );
    ok( test_struct->s->s_a == 1, "got %d\n", test_struct->s->s_a );
    ok( test_struct->s->s_b == 0, "got %d\n", test_struct->s->s_b );
    ok( test_struct->b == 1, "got %d\n", test_struct->b );

    WsFreeHeap( heap );
    WsFreeReader( reader );
}

static void test_dictionary(void)
{
    static const GUID dict_static =
        {0xf93578f8,0x5852,0x4eb7,{0xa6,0xfc,0xe7,0x2b,0xb7,0x1d,0xb6,0x22}};
    static const char test[] =
        {0x42,0x04,0x01};
    static const char test2[] =
        {0x53,0x06,0x0b,0x01,'p',0x0a,0x01};
    static const char test3[] =
        {0x43,0x02,'p','2',0x06,0x0b,0x02,'p','2',0x0a,0x01};
    static const char test4[] =
        {0x42,0x06,0x06,0x06,0x98,0x00,0x01};
    static const char test5[] =
        {0x42,0x06,0x1b,0x06,0x98,0x00,0x0b,0x01,'p',0x0a,0x01};
    static const char test6[] =
        {0x42,0x06,0x07,0x02,'p','2',0x06,0x98,0x00,0x0b,0x02,'p','2',0x0a,0x01};
    static const char test7[] =
        {0x40,0x01,'t',0x0a,0x0a,0x01};
    static const char test8[] =
        {0x40,0x01,'t',0x0b,0x01,'p',0x0a,0x01};
    static const char test9[] =
        {0x42,0x0c,0x01};
    static const char test10[] =
        {0x42,0x04,0xab,0x0c,0x01};
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;
    const WS_XML_ATTRIBUTE *attr;
    const WS_XML_UTF8_TEXT *utf8;
    WS_XML_STRING strings[6];
    WS_XML_DICTIONARY dict, *dict2;
    WS_XML_READER *reader;
    HRESULT hr;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

    /* short dictionary element */
    hr = set_input_bin( reader, test, sizeof(test), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->prefix->bytes == NULL, "bytes set\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->localName->dictionary == &dict, "unexpected dict\n" );
    ok( elem->localName->id == ~0u, "unexpected id %#lx\n", elem->localName->id );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( !elem->attributeCount, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* single character prefix dictionary element */
    hr = set_input_bin( reader, test2, sizeof(test2), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->prefix->length == 1, "got %lu\n", elem->prefix->length );
    ok( !memcmp( elem->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->ns->dictionary == &dict, "unexpected dict\n" );
    ok( attr->ns->id == 5, "unexpected id %#lx\n", attr->ns->id );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* dictionary element */
    hr = set_input_bin( reader, test3, sizeof(test3), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->prefix->length == 2, "got %lu\n", elem->prefix->length );
    ok( !memcmp( elem->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( elem->localName->dictionary == &dict, "unexpected dict\n" );
    ok( elem->localName->id == 3, "unexpected id %#lx\n", elem->localName->id );
    ok( elem->ns->length == 2, "got %lu\n", elem->ns->length );
    ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* short dictionary attribute */
    hr = set_input_bin( reader, test4, sizeof(test4), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( attr->localName->dictionary == &dict, "unexpected dict\n" );
    ok( attr->localName->id == 3, "unexpected id %#lx\n", attr->localName->id );
    ok( !memcmp( attr->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( !attr->ns->length, "got %lu\n", attr->ns->length );
    ok( elem->ns->bytes != NULL, "bytes not set\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8 = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
    ok( !utf8->value.length, "got %lu\n", utf8->value.length );
    ok( utf8->value.bytes != NULL, "bytes not set\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* single character prefix dictionary attribute */
    hr = set_input_bin( reader, test5, sizeof(test5), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "ns not set\n" );
    ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( attr->localName->dictionary == &dict, "unexpected dict\n" );
    ok( attr->localName->id == 3, "unexpected id %#lx\n", attr->localName->id );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8 = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
    ok( !utf8->value.length, "got %lu\n", utf8->value.length );
    ok( utf8->value.bytes != NULL, "bytes not set\n" );
    attr = elem->attributes[1];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* dictionary attribute */
    hr = set_input_bin( reader, test6, sizeof(test6), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( !elem->ns->length, "got %lu\n", elem->ns->length );
    ok( elem->ns->bytes != NULL, "ns not set\n" );
    ok( elem->attributeCount == 2, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( !attr->isXmlNs, "is xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->localName->length == 1, "got %lu\n", attr->localName->length );
    ok( !memcmp( attr->localName->bytes, "u", 1 ), "wrong name\n" );
    ok( attr->localName->dictionary == &dict, "unexpected dict\n" );
    ok( attr->localName->id == 3, "unexpected id %#lx\n", attr->localName->id );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->value != NULL, "value not set\n" );
    utf8 = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
    ok( !utf8->value.length, "got %lu\n", utf8->value.length );
    ok( utf8->value.bytes != NULL, "bytes not set\n" );
    attr = elem->attributes[1];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 2, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p2", 2 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );

    /* short dictionary xmlns attribute */
    hr = set_input_bin( reader, test7, sizeof(test7), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( !attr->prefix->length, "got %lu\n", attr->prefix->length );
    ok( attr->prefix->bytes == NULL, "bytes set\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->ns->dictionary == &dict, "unexpected dict\n" );
    ok( attr->ns->id == 5, "unexpected id %#lx\n", attr->ns->id );
    utf8 = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
    ok( !utf8->value.length, "got %lu\n", utf8->value.length );
    todo_wine ok( utf8->value.bytes != NULL, "bytes not set\n" );

    /* dictionary xmlns attribute */
    hr = set_input_bin( reader, test8, sizeof(test8), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( !elem->prefix->length, "got %lu\n", elem->prefix->length );
    ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "t", 1 ), "wrong name\n" );
    ok( elem->attributeCount == 1, "got %lu\n", elem->attributeCount );
    ok( !elem->isEmpty, "empty\n" );
    attr = elem->attributes[0];
    ok( !attr->singleQuote, "single quote\n" );
    ok( attr->isXmlNs, "not xmlns\n" );
    ok( attr->prefix->length == 1, "got %lu\n", attr->prefix->length );
    ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong prefix\n" );
    ok( attr->ns->length == 2, "got %lu\n", attr->ns->length );
    ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong namespace\n" );
    ok( attr->ns->dictionary == &dict, "unexpected dict\n" );
    ok( attr->ns->id == 5, "unexpected id %#lx\n", attr->ns->id );
    utf8 = (const WS_XML_UTF8_TEXT *)attr->value;
    ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
    ok( !utf8->value.length, "got %lu\n", utf8->value.length );
    todo_wine ok( utf8->value.bytes != NULL, "bytes not set\n" );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* element name string id out of range */
    hr = set_input_bin( reader, test9, sizeof(test9), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadNode( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    /* text string id out of range */
    hr = set_input_bin( reader, test10, sizeof(test10), &dict );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    hr = WsReadNode( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetDictionary( 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsGetDictionary( WS_ENCODING_XML_UTF8, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    dict2 = (WS_XML_DICTIONARY *)0xdeadbeef;
    hr = WsGetDictionary( WS_ENCODING_XML_UTF8, &dict2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict2 == NULL, "got %p\n", dict2 );

    dict2 = NULL;
    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_1, &dict2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict2 != NULL, "dict2 not set\n" );
    ok( dict2 != &dict, "got %p\n", dict2 );

    dict2 = NULL;
    hr = WsGetDictionary( WS_ENCODING_XML_BINARY_SESSION_1, &dict2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( dict2 != NULL, "dict2 not set\n" );
    ok( dict2 != &dict, "got %p\n", dict2 );
    ok( !memcmp( &dict2->guid, &dict_static, sizeof(dict_static) ),
        "got %s\n", wine_dbgstr_guid(&dict2->guid) );
    ok( dict2->stringCount == 488 || dict2->stringCount == 487 /* < win10 */, "got %lu\n", dict2->stringCount );
    ok( dict2->strings[0].length == 14, "got %lu\n", dict2->strings[0].length );
    ok( !memcmp( dict2->strings[0].bytes, "mustUnderstand", 14 ), "wrong data\n" );

    WsFreeReader( reader );
}

static HRESULT prepare_xml_buffer_test( WS_XML_READER *reader, WS_HEAP *heap )
{
    WS_XML_STRING localname = {1, (BYTE *)"t"}, localname2 = {1, (BYTE *)"u"}, ns = {0, NULL};
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buffer;
    HRESULT hr;

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
    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeWriter( writer );
    return S_OK;
}

static void test_WsReadXmlBuffer(void)
{
    const WS_XML_NODE *node;
    WS_XML_READER *reader;
    WS_XML_BUFFER *buffer;
    WS_HEAP *heap;
    HRESULT hr;

    hr = WsReadXmlBuffer( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadXmlBuffer( reader, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadXmlBuffer( reader, heap, NULL, NULL );
    ok( hr == E_FAIL, "got %#lx\n", hr );

    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    todo_wine ok( hr == E_FAIL, "got %#lx\n", hr );

    hr = set_input( reader, "<t><u><v/></u></t></w>", sizeof("<t><u><v/></u></t></w>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    /* reader positioned at element */
    buffer = NULL;
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( buffer != NULL, "buffer not set\n" );
    check_output_buffer( buffer, "<u><v/></u>", __LINE__ );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* reader positioned at end element */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == E_FAIL, "got %#lx\n", hr );

    hr = set_input( reader, "<t><u/></t><v/>", sizeof("<t><u/></t><v/>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* reader positioned at BOF */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = set_input( reader, "<!--comment--><t></t>", sizeof("<!--comment--><t></t>") - 1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_COMMENT, "got %u\n", node->nodeType );

    /* reader positioned at non-element */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == E_FAIL, "got %#lx\n", hr );

    hr = prepare_xml_buffer_test( reader, heap );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* reader positioned at BOF, input buffer */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<t><u/></t>", __LINE__ );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* reader positioned at EOF, input buffer */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == E_FAIL, "got %#lx\n", hr );

    hr = prepare_xml_buffer_test( reader, heap );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    /* reader positioned at element, input buffer */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    check_output_buffer( buffer, "<u/>", __LINE__ );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* reader positioned at end element, input buffer */
    hr = WsReadXmlBuffer( reader, heap, &buffer, NULL );
    ok( hr == E_FAIL, "got %#lx\n", hr );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_union_type(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_a = {1, (BYTE *)"a"}, str_b = {1, (BYTE *)"b"};
    static WS_XML_STRING str_s = {1, (BYTE *)"s"};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_UNION_DESCRIPTION u;
    WS_UNION_FIELD_DESCRIPTION f, f2, *fields[2];
    WS_FIELD_DESCRIPTION f_struct, *fields_struct[1];
    WS_STRUCT_DESCRIPTION s;
    const WS_XML_NODE *node;
    enum choice {CHOICE_A, CHOICE_B, CHOICE_NONE};
    struct test
    {
        enum choice choice;
        union
        {
            WCHAR  *a;
            UINT32  b;
        } value;
    } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

    test = NULL;
    prepare_struct_type_test( reader, "<a>test</a>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->choice == CHOICE_A, "got %d\n", test->choice );
    ok( !wcscmp(test->value.a, L"test"), "got %s\n", wine_dbgstr_w(test->value.a) );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    test = NULL;
    prepare_struct_type_test( reader, "<b>123</b>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->choice == CHOICE_B, "got %d\n", test->choice );
    ok( test->value.b == 123, "got %u\n", test->value.b );

    prepare_struct_type_test( reader, "<c>456</c>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    f_struct.options = WS_FIELD_NILLABLE;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    f_struct.options = WS_FIELD_POINTER|WS_FIELD_NILLABLE;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    f_struct.options = WS_FIELD_POINTER;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    test = NULL;
    f_struct.options = WS_FIELD_OPTIONAL;
    prepare_struct_type_test( reader, "<c>456</c>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    todo_wine ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->choice == CHOICE_NONE, "got %d\n", test->choice );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_float(void)
{
    static const struct
    {
        const char *str;
        HRESULT     hr;
        ULONG       val;
    }
    tests[] =
    {
        {"<t>0.0</t>", S_OK, 0},
        {"<t>-0.0</t>", S_OK, 0x80000000},
        {"<t>+0.0</t>", S_OK, 0},
        {"<t>-</t>", S_OK, 0},
        {"<t>+</t>", S_OK, 0},
        {"<t>.0</t>", S_OK, 0},
        {"<t>0.</t>", S_OK, 0},
        {"<t>0</t>", S_OK, 0},
        {"<t> 0 </t>", S_OK, 0},
        {"<t></t>", WS_E_INVALID_FORMAT, 0},
        {"<t>0,1</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1.1.</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1</t>", S_OK, 0x3f800000},
        {"<t>1.0000001</t>", S_OK, 0x3f800001},
        {"<t>1.0000002</t>", S_OK, 0x3f800002},
        {"<t>10000000000000000000</t>", S_OK, 0x5f0ac723},
        {"<t>100000000000000000000</t>", S_OK, 0x60ad78ec},
        {"<t>2</t>", S_OK, 0x40000000},
        {"<t>-2</t>", S_OK, 0xc0000000},
        {"<t>nofloat</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>INF</t>", S_OK, 0x7f800000},
        {"<t>-INF</t>", S_OK, 0xff800000},
        {"<t>+INF</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>Infinity</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>-Infinity</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>inf</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>NaN</t>", S_OK, 0xffc00000},
        {"<t>-NaN</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>NAN</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>0.3</t>", S_OK, 0x3e99999a},
        {"<t>0.33</t>", S_OK, 0x3ea8f5c3},
        {"<t>0.333</t>", S_OK, 0x3eaa7efa},
        {"<t>0.3333</t>", S_OK, 0x3eaaa64c},
        {"<t>0.33333</t>", S_OK, 0x3eaaaa3b},
        {"<t>0.333333</t>", S_OK, 0x3eaaaa9f},
        {"<t>0.3333333</t>", S_OK, 0x3eaaaaaa},
        {"<t>0.33333333</t>", S_OK, 0x3eaaaaab},
        {"<t>0.333333333</t>", S_OK, 0x3eaaaaab},
        {"<t>0.1e10</t>", S_OK, 0x4e6e6b28},
        {"<t>1e</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1e0</t>", S_OK, 0x3f800000},
        {"<t>1e+1</t>", S_OK, 0x41200000},
        {"<t>1e-1</t>", S_OK, 0x3dcccccd},
        {"<t>e10</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1e10.</t>", WS_E_INVALID_FORMAT, 0},
        {"<t>1E10</t>", S_OK, 0x501502f9},
        {"<t>1e10</t>", S_OK, 0x501502f9},
        {"<t>1e-10</t>", S_OK, 0x2edbe6ff},
        {"<t>3.4028235e38</t>", S_OK, 0x7f7fffff},
        {"<t>3.4028236e38</t>", S_OK, 0x7f800000},
        {"<t>1.1754942e-38</t>", S_OK, 0x007fffff},
        {"<t>1.1754943e-38</t>", S_OK, 0x00800000},
    };
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    ULONG val, i;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        val = 0;
        prepare_type_test( reader, tests[i].str, strlen(tests[i].str) );
        hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_FLOAT_TYPE, NULL,
                         WS_READ_REQUIRED_VALUE, heap, &val, sizeof(val), NULL );
        ok( hr == tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr == tests[i].hr) ok( val == tests[i].val, "%lu: got %#lx\n", i, val );
    }

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_repeating_element_choice(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_a = {1, (BYTE *)"a"}, str_b = {1, (BYTE *)"b"};
    static WS_XML_STRING str_s = {1, (BYTE *)"s"}, str_t = {1, (BYTE *)"t"};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_UNION_DESCRIPTION u;
    WS_UNION_FIELD_DESCRIPTION f, f2, *fields[2];
    WS_FIELD_DESCRIPTION f_items, *fields_items[1];
    WS_STRUCT_DESCRIPTION s;
    const WS_XML_NODE *node;
    enum choice {CHOICE_A, CHOICE_B, CHOICE_NONE};
    struct item
    {
        enum choice choice;
        union
        {
            WCHAR  *a;
            UINT32  b;
        } value;
    };
    struct test
    {
        struct item *items;
        ULONG        count;
    } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

    test = NULL;
    prepare_struct_type_test( reader, "<t><a>test</a></t>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->count == 1, "got %lu\n", test->count );
    ok( test->items[0].choice == CHOICE_A, "got %d\n", test->items[0].choice );
    ok( !wcscmp(test->items[0].value.a, L"test"), "got %s\n", wine_dbgstr_w(test->items[0].value.a) );

    test = NULL;
    prepare_struct_type_test( reader, "<t><b>123</b></t>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->count == 1, "got %lu\n", test->count );
    ok( test->items[0].choice == CHOICE_B, "got %d\n", test->items[0].choice );
    ok( test->items[0].value.b == 123, "got %u\n", test->items[0].value.b );

    test = NULL;
    prepare_struct_type_test( reader, "<t><a>test</a><b>123</b></t>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->count == 2, "got %lu\n", test->count );
    ok( test->items[0].choice == CHOICE_A, "got %d\n", test->items[0].choice );
    ok( !wcscmp(test->items[0].value.a, L"test"), "got %s\n", wine_dbgstr_w(test->items[0].value.a) );
    ok( test->items[1].choice == CHOICE_B, "got %d\n", test->items[1].choice );
    ok( test->items[1].value.b == 123, "got %u\n", test->items[1].value.b );

    prepare_struct_type_test( reader, "<t><c>123</c></t>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
    if (node->nodeType == WS_XML_NODE_TYPE_ELEMENT)
    {
        const WS_XML_ELEMENT_NODE *elem = (const WS_XML_ELEMENT_NODE *)node;
        ok( elem->localName->length == 1, "got %lu\n", elem->localName->length );
        ok( elem->localName->bytes[0] == 'c', "got '%c'\n", elem->localName->bytes[0] );
    }

    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( !test->count, "got %lu\n", test->count );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_empty_text_field(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_t = {1, (BYTE *)"t"};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_FIELD_DESCRIPTION f, *fields[1];
    WS_STRUCT_DESCRIPTION s;
    struct test
    {
        WS_STRING str;
    } *test;
    struct test2
    {
        WCHAR *str;
    } *test2;
    struct test3
    {
        BOOL boolean;
    } *test3;
    struct test4
    {
        WS_XML_STRING str;
    } *test4;
    struct test5
    {
        WS_BYTES bytes;
    } *test5;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_STRING_TYPE;
    f.offset    = FIELD_OFFSET(struct test, str);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test);
    s.alignment     = TYPE_ALIGNMENT(struct test);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_t;
    s.typeNs        = &str_ns;

    test = NULL;
    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( !test->str.length, "got %lu\n", test->str.length );
    todo_wine ok( test->str.chars != NULL, "chars not set\n" );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_WSZ_TYPE;
    f.offset    = FIELD_OFFSET(struct test2, str);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test2);
    s.alignment     = TYPE_ALIGNMENT(struct test2);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_t;
    s.typeNs        = &str_ns;

    test2 = NULL;
    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test2 != NULL, "test2 not set\n" );
    ok( test2->str != NULL, "str not set\n" );
    ok( !test2->str[0], "not empty\n" );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_BOOL_TYPE;
    f.offset    = FIELD_OFFSET(struct test3, boolean);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test3);
    s.alignment     = TYPE_ALIGNMENT(struct test3);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_t;
    s.typeNs        = &str_ns;

    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test3, sizeof(test3), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_XML_STRING_TYPE;
    f.offset    = FIELD_OFFSET(struct test4, str);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test4);
    s.alignment     = TYPE_ALIGNMENT(struct test4);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_t;
    s.typeNs        = &str_ns;

    test4 = NULL;
    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test4, sizeof(test4), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test4 != NULL, "test4 not set\n" );
    ok( !test4->str.length, "got %lu\n", test4->str.length );
    todo_wine ok( test4->str.bytes != NULL, "bytes not set\n" );

    memset( &f, 0, sizeof(f) );
    f.mapping   = WS_TEXT_FIELD_MAPPING;
    f.type      = WS_BYTES_TYPE;
    f.offset    = FIELD_OFFSET(struct test5, bytes);
    fields[0] = &f;

    memset( &s, 0, sizeof(s) );
    s.size          = sizeof(struct test5);
    s.alignment     = TYPE_ALIGNMENT(struct test5);
    s.fields        = fields;
    s.fieldCount    = 1;
    s.typeLocalName = &str_t;
    s.typeNs        = &str_ns;

    test5 = NULL;
    prepare_struct_type_test( reader, "<t></t>" );
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test5, sizeof(test5), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test5 != NULL, "test5 not set\n" );
    ok( !test5->bytes.length, "got %lu\n", test5->bytes.length );
    todo_wine ok( test5->bytes.bytes != NULL, "bytes not set\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static const char stream_utf8[] = {0xef,0xbb,0xbf,'<','t','/','>',0};
static const struct stream_test
{
    const char *xml;
    HRESULT     hr;
    int         todo;
}
stream_tests[] =
{
    { "", WS_E_QUOTA_EXCEEDED },
    { "<?xml version=\"1.0\" encoding=\"utf-8\"?><t/>", S_OK },
    { "<t/>", S_OK },
    { stream_utf8, S_OK, 1 },
};

static CALLBACK HRESULT read_callback( void *state, void *buf, ULONG buflen, ULONG *retlen,
                                       const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct stream_test *test = state;
    ULONG len = strlen( test->xml );

    ok( state != NULL, "NULL state\n" );
    ok( buf != NULL, "NULL buf\n" );
    ok( buflen > 0, "zero buflen\n" );
    ok( retlen != NULL, "NULL retlen\n" );
    if (buflen < len) return WS_E_QUOTA_EXCEEDED;
    memcpy( buf, test->xml, len );
    *retlen = len;
    return S_OK;
}

static void test_stream_input(void)
{
    static WS_XML_STRING str_ns = {0, NULL}, str_t = {1, (BYTE *)"t"};
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}};
    WS_XML_READER_STREAM_INPUT stream;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    WS_CHARSET charset;
    HRESULT hr;
    BOOL found;
    ULONG i, size;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    stream.input.inputType   = WS_XML_READER_INPUT_TYPE_STREAM;
    stream.readCallback      = read_callback;
    stream.readCallbackState = (void *)&stream_tests[2];
    hr = WsSetInput( reader, &text.encoding, &stream.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    size = sizeof(charset);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
    todo_wine ok( hr == WS_E_QUOTA_EXCEEDED, "got %#lx\n", hr );

    hr = WsSetInput( reader, &text.encoding, &stream.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, strlen(stream_tests[2].xml), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    charset = 0xdeadbeef;
    size = sizeof(charset);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( charset == WS_CHARSET_UTF8, "got %u\n", charset );

    found = -1;
    hr = WsReadToStartElement( reader, &str_t, &str_ns, &found, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsFillReader( reader, 1, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE(stream_tests); i++)
    {
        stream.readCallbackState = (void *)&stream_tests[i];
        hr = WsSetInput( reader, &text.encoding, &stream.input, NULL, 0, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        hr = WsFillReader( reader, strlen( stream_tests[i].xml ), NULL, NULL );
        ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

        found = -1;
        hr = WsReadToStartElement( reader, &str_t, &str_ns, &found, NULL );
        todo_wine_if(stream_tests[i].todo) ok( hr == stream_tests[i].hr, "%lu: got %#lx\n", i, hr );
        if (hr == S_OK)
        {
            ok( found == TRUE, "%lu: got %d\n", i, found );
            hr = WsReadStartElement( reader, NULL );
            ok( hr == S_OK, "%lu: got %#lx\n", i, hr );

            hr = WsGetReaderNode( reader, &node, NULL );
            ok( hr == S_OK, "%lu: got %#lx\n", i, hr );
            if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "%lu: got %u\n", i, node->nodeType );
        }
    }

    WsFreeReader( reader );
}

static void test_description_type(void)
{
    static WS_XML_STRING ns = {0, NULL}, ns2 = {2, (BYTE *)"ns"}, localname = {1, (BYTE *)"t"};
    static WS_XML_STRING val = {3, (BYTE *)"val"};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    WS_FIELD_DESCRIPTION f, f2, *fields[2];
    WS_STRUCT_DESCRIPTION s;
    struct test
    {
        const WS_STRUCT_DESCRIPTION *desc;
        INT32                        val;
    } *test;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

    prepare_struct_type_test( reader, "<t val=\"-1\" xmlns=\"ns\"/>" );
    hr = WsReadToStartElement( reader, &localname, &ns2, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->val == -1, "got %d\n", test->val );
        ok( test->desc == &s, "got %p\n", test->desc );
    }

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsAddErrorString(void)
{
    static const WS_STRING emptystr = { 0 };
    static const WS_STRING str1 = WS_STRING_VALUE( L"str1" );
    static const WS_STRING str2 = WS_STRING_VALUE( L"str2" );
    ULONG count;
    WS_ERROR *error;
    WS_STRING out;
    HRESULT hr;

    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAddErrorString( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    hr = WsAddErrorString( NULL, &str1 );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    hr = WsAddErrorString( error, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsAddErrorString( error, &emptystr );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsAddErrorString(error, &str2 );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsAddErrorString(error, &str1 );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0xdeadbeef;
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, sizeof(count) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 3, "got %lu\n", count );

    hr = WsGetErrorString( error, 0, &out );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( out.length == str1.length, "got %lu\n", out.length );
    ok( !memcmp( out.chars, str1.chars, str1.length ), "wrong error string\n" );

    hr = WsGetErrorString( error, 1, &out );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( out.length == str2.length, "got %lu\n", out.length );
    ok( !memcmp( out.chars, str2.chars, str2.length ), "wrong error string\n" );

    hr = WsGetErrorString( error, 2, &out );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( out.length == 0, "got %lu\n", out.length );
    ok( out.chars != NULL, "out.chars == NULL\n" );

    WsFreeError( error );
}

static void test_WsSetFaultErrorProperty(void)
{
    static const WCHAR expected_errorstr[] = L"The fault reason was: 'Some reason'.";
    static const char detailxml[] = "<detail><ErrorCode>1030</ErrorCode></detail>";
    static const LANGID langid = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
    static const WS_XML_STRING action = { 24, (BYTE *)"http://example.com/fault" };
    WS_ERROR_PROPERTY prop;
    WS_ERROR *error;
    WS_FAULT fault;
    WS_FAULT *faultp;
    WS_XML_STRING outxmlstr;
    WS_STRING outstr;
    ULONG count;
    WS_HEAP *heap;
    WS_XML_READER *reader;
    HRESULT hr;

    prop.id = WS_ERROR_PROPERTY_LANGID;
    prop.value = (void *)&langid;
    prop.valueSize = sizeof(langid);

    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, NULL, sizeof(WS_FAULT) );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, NULL, sizeof(WS_XML_STRING) );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &fault, 0, sizeof(fault) );

    fault.code = calloc( 1, sizeof(WS_FAULT_CODE) );
    fault.code->value.localName.bytes = (BYTE *)"Server";
    fault.code->value.localName.length = 6;
    fault.code->subCode = calloc( 1, sizeof(WS_FAULT_CODE) );
    fault.code->subCode->value.localName.bytes = (BYTE *)"SubCode";
    fault.code->subCode->value.localName.length = 7;

    fault.reasons = calloc( 1, sizeof(*fault.reasons) );
    fault.reasonCount = 1;
    fault.reasons[0].lang.chars = (WCHAR *) L"en-US";
    fault.reasons[0].lang.length = 5;
    fault.reasons[0].text.chars = (WCHAR *) L"Some reason";
    fault.reasons[0].text.length = 11;

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(WS_FAULT) );
    ok( hr == S_OK, "got %#lx\n", hr );

    faultp = NULL;
    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &faultp, sizeof(faultp) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( faultp != NULL, "faultp not set\n" );
    ok( faultp != &fault, "fault not copied\n" );

    ok( faultp->code && faultp->code != fault.code, "fault code not copied\n" );
    ok( faultp->code->value.localName.length == 6, "got %lu\n", faultp->code->value.localName.length );
    ok( !memcmp( faultp->code->value.localName.bytes, fault.code->value.localName.bytes, 6 ),
        "wrong code localName\n" );
    ok( faultp->code->value.localName.bytes != fault.code->value.localName.bytes,
        "fault code localName not copied\n" );
    ok( faultp->code->value.ns.length == 0, "got %lu\n", faultp->code->value.ns.length );
    ok( faultp->code->subCode && faultp->code->subCode != fault.code->subCode,
        "fault code subCode not copied\n" );
    ok( faultp->code->subCode->value.localName.length == 7,"got %lu\n", faultp->code->subCode->value.localName.length );
    ok( !memcmp( faultp->code->subCode->value.localName.bytes, fault.code->subCode->value.localName.bytes, 7 ),
        "wrong subCode localName\n" );
    ok( faultp->code->subCode->value.localName.bytes != fault.code->subCode->value.localName.bytes,
        "fault code subCode localName not copied\n" );
    ok( faultp->code->subCode->value.ns.length == 0, "got %lu\n", faultp->code->subCode->value.ns.length );
    ok( faultp->code->subCode->subCode == NULL, "fault->code->subCode->subCode != NULL\n");

    ok( faultp->reasons != fault.reasons, "fault reasons not copied\n" );
    ok( faultp->reasonCount == 1, "got %lu\n", faultp->reasonCount );
    ok( faultp->reasons[0].lang.length == 5, "got %lu\n", faultp->reasons[0].text.length );
    ok( !memcmp( faultp->reasons[0].lang.chars, fault.reasons[0].lang.chars, 5 * sizeof(WCHAR) ),
        "wrong fault reason lang\n" );
    ok( faultp->reasons[0].lang.chars != fault.reasons[0].lang.chars,
        "fault reason lang not copied\n" );
    ok( faultp->reasons[0].text.length == 11, "got %lu\n", faultp->reasons[0].text.length );
    ok( !memcmp( faultp->reasons[0].text.chars, fault.reasons[0].text.chars, 11 * sizeof(WCHAR) ),
        "wrong fault reason text\n" );
    ok( faultp->reasons[0].text.chars != fault.reasons[0].text.chars,
        "fault reason text not copied\n" );

    ok( faultp->actor.length == 0, "got %lu\n", faultp->actor.length );
    ok( faultp->node.length == 0, "got %lu\n", faultp->node.length );
    ok( faultp->detail == NULL, "faultp->detail != NULL\n" );

    count = 0xdeadbeef;
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, sizeof(count) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 1, "got %lu\n", count );

    hr = WsGetErrorString( error, 0, &outstr );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( outstr.length == ARRAY_SIZE(expected_errorstr) - 1, "got %lu\n", outstr.length );
    ok( !memcmp( outstr.chars, expected_errorstr, (ARRAY_SIZE(expected_errorstr) - 1) * sizeof(WCHAR) ),
        "wrong error string\n" );

    outxmlstr.bytes = (BYTE *)0xdeadbeef;
    outxmlstr.length = 0xdeadbeef;
    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &outxmlstr, sizeof(outxmlstr) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( outxmlstr.length == 0, "got %lu\n", outxmlstr.length );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &action, sizeof(action) );
    ok( hr == S_OK, "got %#lx\n", hr );

    outxmlstr.bytes = (BYTE *)0xdeadbeef;
    outxmlstr.length = 0xdeadbeef;
    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &outxmlstr, sizeof(outxmlstr) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( outxmlstr.length == 24, "got %lu\n", outxmlstr.length );
    ok( !memcmp( outxmlstr.bytes, action.bytes, 24 ), "wrong fault action\n" );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = set_input( reader, detailxml, strlen(detailxml) );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadXmlBuffer( reader, heap, &fault.detail, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(WS_FAULT) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &faultp, sizeof(faultp) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( faultp != NULL, "faultp not set\n" );
    ok( faultp->detail != NULL, "fault detail not set\n" );
    ok( faultp->detail != fault.detail, "fault detail not copied\n" );
    check_output_buffer( faultp->detail, detailxml, __LINE__ );

    free( fault.code->subCode );
    free( fault.code );
    free( fault.reasons );
    WsFreeReader( reader );
    WsFreeHeap( heap );
    WsFreeError( error );
}

static void test_WsGetFaultErrorDetail(void)
{
    static const char detailxml[] = "<detail><ErrorCode>1030</ErrorCode></detail>";
    static const char badxml[] = "<bad><ErrorCode>1030</ErrorCode></bad>";

    WS_ERROR *error;
    WS_HEAP *heap;
    WS_XML_READER *reader;
    WS_FAULT fault;
    WS_XML_STRING action = { 24, (BYTE *)"http://example.com/fault" };
    WS_XML_STRING action2 = { 25, (BYTE *)"http://example.com/fault2" };
    WS_XML_STRING localname = { 9, (BYTE *)"ErrorCode" }, localname2 = { 9, (BYTE *)"OtherCode" };
    WS_XML_STRING ns = { 0 };
    WS_ELEMENT_DESCRIPTION desc = { &localname, &ns, WS_UINT32_TYPE, NULL };
    WS_ELEMENT_DESCRIPTION desc2 = { &localname2, &ns, WS_UINT32_TYPE, NULL };
    WS_FAULT_DETAIL_DESCRIPTION fault_desc;
    UINT32 code;
    UINT32 *codep;
    HRESULT hr;

    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &fault, 0, sizeof(fault) );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = set_input( reader, detailxml, strlen(detailxml) );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadXmlBuffer( reader, heap, &fault.detail, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(fault) );
    ok( hr == S_OK, "got %#lx\n", hr );

    fault_desc.action = NULL;
    fault_desc.detailElementDescription = &desc;

    code = 0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( code == 1030, "got %u\n", code );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_OPTIONAL_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( codep != NULL, "codep == NULL\n" );
    ok( *codep == 1030, "got %u\n", *codep );

    fault_desc.action = &action;

    code = 0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( code == 1030, "got %u\n", code );

    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &action, sizeof(action) );
    ok( hr == S_OK, "got %#lx\n", hr );

    fault_desc.action = NULL;

    code = 0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( code == 1030, "got %u\n", code );

    fault_desc.action = &action2;

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_OPTIONAL_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_NILLABLE_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    fault_desc.action = NULL;
    fault_desc.detailElementDescription = &desc2;

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_OPTIONAL_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_NILLABLE_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    hr = set_input( reader, badxml, strlen(badxml) );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsReadXmlBuffer( reader, heap, &fault.detail, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(fault) );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_VALUE, heap, &code, sizeof(code) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_REQUIRED_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_OPTIONAL_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    codep = (UINT32 *)0xdeadbeef;
    hr = WsGetFaultErrorDetail( error, &fault_desc, WS_READ_NILLABLE_POINTER, heap, &codep, sizeof(codep) );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );
    ok( codep == NULL, "codep != NULL\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
    WsFreeError( error );
}

START_TEST(reader)
{
    test_WsCreateError();
    test_WsCreateHeap();
    test_WsCreateReader();
    test_WsSetInput();
    test_WsSetInputToBuffer();
    test_WsFillReader();
    test_WsReadToStartElement();
    test_WsReadStartElement();
    test_WsReadEndElement();
    test_WsReadNode();
    test_WsReadType();
    test_WsGetXmlAttribute();
    test_WsXmlStringEquals();
    test_WsAlloc();
    test_WsMoveReader();
    test_simple_struct_type();
    test_cdata();
    test_WsFindAttribute();
    test_WsGetNamespaceFromPrefix();
    test_text_field_mapping();
    test_complex_struct_type();
    test_repeating_element();
    test_WsResetHeap();
    test_datetime();
    test_WsDateTimeToFileTime();
    test_WsFileTimeToDateTime();
    test_double();
    test_WsReadElement();
    test_WsReadValue();
    test_WsResetError();
    test_WsGetReaderPosition();
    test_WsSetReaderPosition();
    test_entities();
    test_field_options();
    test_WsReadBytes();
    test_WsReadChars();
    test_WsReadCharsUtf8();
    test_WsReadQualifiedName();
    test_WsReadAttribute();
    test_WsSkipNode();
    test_binary_encoding();
    test_dictionary();
    test_WsReadXmlBuffer();
    test_union_type();
    test_float();
    test_repeating_element_choice();
    test_empty_text_field();
    test_stream_input();
    test_description_type();
    test_WsAddErrorString();
    test_WsSetFaultErrorProperty();
    test_WsGetFaultErrorDetail();
}
