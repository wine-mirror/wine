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

static const char data2[] =
    {0xef,0xbb,0xbf,'<','t','e','x','t','>','t','e','s','t','<','/','t','e','x','t','>',0};

static const char data3[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
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

static const char data11[] =
    "<o:OfficeConfig xmlns:o=\"urn:schemas-microsoft-com:office:office\">"
    "<o:services o:GenerationTime=\"2015-09-03T18:47:54\">"
    "<!--Build: 16.0.6202.6852-->"
    "</o:services>"
    "</o:OfficeConfig>";

static const char data12[] =
    "<services>"
    "<service><id>1</id></service>"
    "<service><id>2</id></service>"
    "</services>";

static const char data13[] =
    "<services></services>";

static const char data14[] =
    "<services>"
    "<wrapper>"
    "<service><id>1</id></service>"
    "<service><id>2</id></service>"
    "</wrapper>"
    "</services>";

static const char data15[] =
    "<services>"
    "<wrapper>"
    "<service>1</service>"
    "<service>2</service>"
    "</wrapper>"
    "</services>";

static const char data16[] =
    "<services>"
    "<wrapper>"
    "<service name='1'>1</service>"
    "<service name='2'>2</service>"
    "</wrapper>"
    "</services>";

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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    reader = NULL;
    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    size = sizeof(buffer_size);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_STREAM_BUFFER_SIZE, &buffer_size, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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
    hr = WsCreateReader( &prop, 1, &reader, NULL );
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
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    column = 1;
    prop.id = WS_XML_READER_PROPERTY_COLUMN;
    prop.value = &column;
    prop.valueSize = sizeof(column);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    in_attr = TRUE;
    prop.id = WS_XML_READER_PROPERTY_IN_ATTRIBUTE;
    prop.value = &in_attr;
    prop.valueSize = sizeof(in_attr);
    hr = WsCreateReader( &prop, 1, &reader, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
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
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_READER_PROPERTY prop;
    WS_XML_READER_TEXT_ENCODING enc;
    WS_XML_READER_BUFFER_INPUT input;
    WS_CHARSET charset;
    const WS_XML_NODE *node;
    ULONG i, size, max_depth;
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

    hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* multiple calls are allowed */
    hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* charset is detected by WsSetInput */
    enc.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
    enc.charSet               = WS_CHARSET_AUTO;

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        input.encodedData     = tests[i].data;
        input.encodedDataSize = tests[i].size;
        hr = WsSetInput( reader, &enc.encoding, &input.input, NULL, 0, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        charset = 0xdeadbeef;
        size = sizeof(charset);
        hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, size, NULL );
        todo_wine_if (tests[i].todo)
        {
            ok( hr == tests[i].hr, "%u: got %08x expected %08x\n", i, hr, tests[i].hr );
            if (hr == S_OK)
                ok( charset == tests[i].charset, "%u: got %u expected %u\n", i, charset, tests[i].charset );
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
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetInputToBuffer( NULL, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsSetInputToBuffer( reader, NULL, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node != NULL, "node not set\n" );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    /* multiple calls are allowed */
    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* reader properties can be set with WsSetInputToBuffer */
    max_depth = 16;
    prop.id = WS_XML_READER_PROPERTY_MAX_DEPTH;
    prop.value = &max_depth;
    prop.valueSize = sizeof(max_depth);
    hr = WsSetInputToBuffer( reader, buffer, &prop, 1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    max_depth = 0xdeadbeef;
    size = sizeof(max_depth);
    hr = WsGetReaderProperty( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( max_depth == 16, "got %u\n", max_depth );

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
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    node = NULL;
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );
    WsFreeReader( reader );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
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

static void test_WsReadToStartElement(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node, *node2;
    int found;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data1, sizeof(data1) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsFillReader( reader, sizeof(data1) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadToStartElement( NULL, NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == FALSE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        if (elem->prefix)
        {
            ok( !elem->prefix->length, "got %u\n", elem->prefix->length );
        }
        ok( elem->localName != NULL, "localName not set\n" );
        if (elem->localName)
        {
            ok( elem->localName->length == 4, "got %u\n", elem->localName->length );
            ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        }
        ok( elem->ns != NULL, "ns not set\n" );
        if (elem->ns)
        {
            ok( !elem->ns->length, "got %u\n", elem->ns->length );
        }
        ok( !elem->attributeCount, "got %u\n", elem->attributeCount );
        ok( elem->attributes == NULL, "attributes set\n" );
        ok( !elem->isEmpty, "isEmpty not zero\n" );
    }

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    node2 = NULL;
    hr = WsGetReaderNode( reader, &node2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node2 == node, "different node\n" );

    hr = set_input( reader, data3, sizeof(data3) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data3) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->localName != NULL, "localName not set\n" );
        if (elem->localName)
        {
            ok( elem->localName->length == 4, "got %u\n", elem->localName->length );
            ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        }
    }

    hr = set_input( reader, data4, sizeof(data4) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data4) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsReadStartElement( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( text->node.nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", text->node.nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( text->text->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", text->text->textType );
            ok( utf8->value.length == 4, "got %u\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "test", 4 ), "wrong data\n" );
        }
    }

    hr = WsReadStartElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    node2 = NULL;
    hr = WsGetReaderNode( reader, &node2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node2 == node, "different node\n" );

    hr = set_input( reader, data8, sizeof(data8) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data8) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
        ok( !memcmp( elem->localName->bytes, "node1", 5), "wrong name\n" );
    }

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );
        ok( !memcmp( elem->localName->bytes, "node2", 5), "wrong name\n" );
    }

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* WsReadEndElement advances reader to EOF */
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    WsFreeReader( reader );
}

static void test_WsReadEndElement(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    const WS_XML_NODE *node;
    int found;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, data2, sizeof(data2) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data2) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* WsReadEndElement advances reader to EOF */
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, data5, sizeof(data5) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data5) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, data10, sizeof(data10) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data10) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, "<a></A>", sizeof("<a></A>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof("<a></a>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, "<a></a>", sizeof("<a></a>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof("<a></a>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = set_input( reader, "<a/>", sizeof("<a/>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof("<a/>") - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

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
    ok( hr == S_OK, "got %08x\n", hr );

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_input( reader, tests[i].text, strlen(tests[i].text) );
        ok( hr == S_OK, "got %08x\n", hr );

        hr = WsFillReader( reader, strlen(tests[i].text), NULL, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsReadNode( reader, NULL );
        todo_wine_if (tests[i].todo)
            ok( hr == tests[i].hr, "%u: got %08x\n", i, hr );
        if (hr == S_OK)
        {
            node = NULL;
            hr = WsGetReaderNode( reader, &node, NULL );
            ok( hr == S_OK, "%u: got %08x\n", i, hr );
            ok( node != NULL, "%u: node not set\n", i );
            if (node)
            {
                todo_wine_if (tests[i].todo)
                    ok( node->nodeType == tests[i].type, "%u: got %u\n", i, node->nodeType );
            }
        }
    }

    hr = set_input( reader, data6, sizeof(data6) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data6) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;
        WS_XML_UTF8_TEXT *text;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %u\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes set\n" );
        ok( elem->localName != NULL, "localName not set\n" );
        ok( elem->localName->length == 4, "got %u\n", elem->localName->length );
        ok( !memcmp( elem->localName->bytes, "text", 4 ), "wrong data\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( !elem->ns->length, "got %u\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( elem->attributeCount == 2, "got %u\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );
        ok( !elem->isEmpty, "isEmpty not zero\n" );

        attr = elem->attributes[0];
        ok( !attr->singleQuote, "got %u\n", attr->singleQuote );
        ok( !attr->isXmlNs, "got %u\n", attr->isXmlNs );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 4, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr", 4 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( !attr->ns->length, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes == NULL, "bytes set\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( text->value.length == 5, "got %u\n", text->value.length );
        ok( !memcmp( text->value.bytes, "value", 5 ), "wrong data\n" );

        attr = elem->attributes[1];
        ok( attr->singleQuote == 1, "got %u\n", attr->singleQuote );
        ok( !attr->isXmlNs, "got %u\n", attr->isXmlNs );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 5, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr2", 5 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( !attr->ns->length, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes == NULL, "bytes set\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( text->value.length == 6, "got %u\n", text->value.length );
        ok( !memcmp( text->value.bytes, "value2", 6 ), "wrong data\n" );
    }

    hr = set_input( reader, data7, sizeof(data7) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data7) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_COMMENT_NODE *comment = (WS_XML_COMMENT_NODE *)node;

        ok( comment->node.nodeType == WS_XML_NODE_TYPE_COMMENT, "got %u\n", comment->node.nodeType );
        ok( comment->value.length == 9, "got %u\n", comment->value.length );
        ok( !memcmp( comment->value.bytes, " comment ", 9 ), "wrong data\n" );
    }

    WsFreeReader( reader );
}

static void prepare_type_test( WS_XML_READER *reader, const char *data, ULONG size )
{
    HRESULT hr;

    hr = set_input( reader, data, size );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, size, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
}

static void test_WsReadType(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const GUID guid1 = {0,0,0,{0,0,0,0,0,0,0,0}};
    static const GUID guid2 = {0,0,0,{0,0,0,0,0,0,0,0xa1}};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_HEAP *heap;
    enum { ONE = 1, TWO = 2 };
    WS_XML_STRING one = { 3, (BYTE *)"ONE" };
    WS_XML_STRING two = { 3, (BYTE *)"TWO" };
    WS_ENUM_VALUE enum_values[] = { { ONE, &one }, { TWO, &two } };
    WS_ENUM_DESCRIPTION enum_desc;
    int val_enum;
    WCHAR *val_str;
    BOOL val_bool;
    INT8 val_int8;
    INT16 val_int16;
    INT32 val_int32;
    INT64 val_int64;
    UINT8 val_uint8;
    UINT16 val_uint16;
    UINT32 val_uint32;
    UINT64 val_uint64;
    GUID val_guid;
    WS_BYTES val_bytes;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    prepare_type_test( reader, data2, sizeof(data2) - 1 );
    hr = WsReadType( NULL, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, NULL, sizeof(val_str), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str) + 1, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    val_str = NULL;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_WSZ_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &val_str, sizeof(val_str), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_str != NULL, "pointer not set\n" );
    if (val_str) ok( !lstrcmpW( val_str, testW ), "wrong data\n" );

    val_bool = -1;
    prepare_type_test( reader, "<t>true</t>", sizeof("<t>true</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(BOOL), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bool == TRUE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>false</t>", sizeof("<t>false</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(BOOL), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bool == FALSE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>FALSE</t>", sizeof("<t>FALSE</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( val_bool == -1, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>1</t>", sizeof("<t>1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bool == TRUE, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>2</t>", sizeof("<t>2</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( val_bool == -1, "got %d\n", val_bool );

    val_bool = -1;
    prepare_type_test( reader, "<t>0</t>", sizeof("<t>0</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BOOL_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bool, sizeof(val_bool), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bool == FALSE, "got %d\n", val_bool );

    val_int8 = 0;
    prepare_type_test( reader, "<t>-128</t>", sizeof("<t>-128</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_int8 == -128, "got %d\n", val_int8 );

    val_int8 = 0;
    prepare_type_test( reader, "<t> </t>", sizeof("<t> </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_int8, "got %d\n", val_int8 );

    val_int8 = 0;
    prepare_type_test( reader, "<t>-</t>", sizeof("<t>-</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_int8, "got %d\n", val_int8 );

    val_int8 = -1;
    prepare_type_test( reader, "<t>-0</t>", sizeof("<t>-0</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !val_int8, "got %d\n", val_int8 );

    val_int8 = 0;
    prepare_type_test( reader, "<t>-129</t>", sizeof("<t>-129</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int8, sizeof(val_int8), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_int8, "got %d\n", val_int8 );

    val_int16 = 0;
    prepare_type_test( reader, "<t>-32768</t>", sizeof("<t>-32768</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int16, sizeof(val_int16), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_int16 == -32768, "got %d\n", val_int16 );

    val_int16 = 0;
    prepare_type_test( reader, "<t>-32769</t>", sizeof("<t>-32769</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int16, sizeof(val_int16), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_int16, "got %d\n", val_int16 );

    val_int32 = 0;
    prepare_type_test( reader, "<t>-2147483648</t>", sizeof("<t>-2147483648</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int32, sizeof(val_int32), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_int32 == -2147483647 - 1, "got %d\n", val_int32 );

    val_int32 = 0;
    prepare_type_test( reader, "<t>-2147483649</t>", sizeof("<t>-2147483649</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int32, sizeof(val_int32), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_int32, "got %d\n", val_int32 );

    val_int64 = 0;
    prepare_type_test( reader, "<t>-9223372036854775808</t>", sizeof("<t>-9223372036854775808</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int64, sizeof(val_int64), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_int64 == -9223372036854775807 - 1, "wrong value\n" );

    val_int64 = 0;
    prepare_type_test( reader, "<t>-9223372036854775809</t>", sizeof("<t>-9223372036854775809</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_INT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_int64, sizeof(val_int64), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_int64, "wrong value\n" );

    val_uint8 = 0;
    prepare_type_test( reader, "<t> 255 </t>", sizeof("<t> 255 </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_uint8 == 255, "got %u\n", val_uint8 );

    val_uint8 = 0;
    prepare_type_test( reader, "<t>+255</t>", sizeof("<t>+255</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_uint8, "got %u\n", val_uint8 );

    val_uint8 = 0;
    prepare_type_test( reader, "<t>-255</t>", sizeof("<t>-255</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    todo_wine ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_uint8, "got %u\n", val_uint8 );

    val_uint8 = 0;
    prepare_type_test( reader, "<t>0xff</t>", sizeof("<t>0xff</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_uint8, "got %u\n", val_uint8 );

    val_uint8 = 0;
    prepare_type_test( reader, "<t>256</t>", sizeof("<t>256</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT8_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint8, sizeof(val_uint8), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_uint8, "got %u\n", val_uint8 );

    val_uint16 = 0;
    prepare_type_test( reader, "<t>65535</t>", sizeof("<t>65535</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint16, sizeof(val_uint16), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_uint16 == 65535, "got %u\n", val_uint16 );

    val_uint16 = 0;
    prepare_type_test( reader, "<t>65536</t>", sizeof("<t>65536</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT16_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint16, sizeof(val_uint16), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_uint16, "got %u\n", val_uint16 );

    val_uint32 = 0;
    prepare_type_test( reader, "<t>4294967295</t>", sizeof("<t>4294967295</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint32, sizeof(val_uint32), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_uint32 == ~0, "got %u\n", val_uint32 );

    val_uint32 = 0;
    prepare_type_test( reader, "<t>4294967296</t>", sizeof("<t>4294967296</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT32_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint32, sizeof(val_uint32), NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
    ok( !val_uint32, "got %u\n", val_uint32 );

    val_uint64 = 0;
    prepare_type_test( reader, "<t>18446744073709551615</t>", sizeof("<t>18446744073709551615</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint64, sizeof(val_uint64), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_uint64 == ~0, "wrong value\n" );

    val_uint64 = 0;
    prepare_type_test( reader, "<t>18446744073709551616</t>", sizeof("<t>18446744073709551616</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_UINT64_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_uint64, sizeof(val_uint64), NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( !val_uint64, "wrong value\n" );

    enum_desc.values       = enum_values;
    enum_desc.valueCount   = sizeof(enum_values)/sizeof(enum_values[0]);
    enum_desc.maxByteCount = 3;
    enum_desc.nameIndices  = NULL;

    val_enum = 0;
    prepare_type_test( reader, "<t>ONE</t>", sizeof("<t>ONE</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_ENUM_TYPE, &enum_desc,
                     WS_READ_REQUIRED_VALUE, heap, &val_enum, sizeof(val_enum), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_enum == 1, "got %d\n", val_enum );

    prepare_type_test( reader, "<t>{00000000-0000-0000-0000-000000000000}</t>",
                       sizeof("<t>{00000000-0000-0000-0000-000000000000}</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    memset( &val_guid, 0xff, sizeof(val_guid) );
    prepare_type_test( reader, "<t> 00000000-0000-0000-0000-000000000000 </t>",
                       sizeof("<t> 00000000-0000-0000-0000-000000000000 </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( IsEqualGUID( &val_guid, &guid1 ), "wrong guid\n" );

    memset( &val_guid, 0, sizeof(val_guid) );
    prepare_type_test( reader, "<t>00000000-0000-0000-0000-0000000000a1</t>",
                       sizeof("<t>00000000-0000-0000-0000-0000000000a1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( IsEqualGUID( &val_guid, &guid2 ), "wrong guid\n" );

    memset( &val_guid, 0, sizeof(val_guid) );
    prepare_type_test( reader, "<t>00000000-0000-0000-0000-0000000000A1</t>",
                       sizeof("<t>00000000-0000-0000-0000-0000000000A1</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_GUID_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_guid, sizeof(val_guid), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( IsEqualGUID( &val_guid, &guid2 ), "wrong guid\n" );

    memset( &val_bytes, 0, sizeof(val_bytes) );
    prepare_type_test( reader, "<t>dGVzdA==</t>", sizeof("<t>dGVzdA==</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bytes.length == 4, "got %u\n", val_bytes.length );
    ok( !memcmp( val_bytes.bytes, "test", 4 ), "wrong data\n" );

    memset( &val_bytes, 0, sizeof(val_bytes) );
    prepare_type_test( reader, "<t> dGVzdA== </t>", sizeof("<t> dGVzdA== </t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val_bytes.length == 4, "got %u\n", val_bytes.length );
    ok( !memcmp( val_bytes.bytes, "test", 4 ), "wrong data\n" );

    prepare_type_test( reader, "<t>dGVzdA===</t>", sizeof("<t>dGVzdA===</t>") - 1 );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_BYTES_TYPE, NULL,
                     WS_READ_REQUIRED_VALUE, heap, &val_bytes, sizeof(val_bytes), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsGetXmlAttribute(void)
{
    static const WCHAR valueW[] = {'v','a','l','u','e',0};
    HRESULT hr;
    WS_XML_READER *reader;
    WS_XML_STRING xmlstr;
    WS_HEAP *heap;
    WCHAR *str;
    ULONG count;
    int found;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, data9, sizeof(data9) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(data9) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    found = -1;
    hr = WsReadToStartElement( reader, NULL, NULL, &found, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( found == TRUE, "got %d\n", found );

    xmlstr.bytes      = (BYTE *)"attr";
    xmlstr.length     = sizeof("attr") - 1;
    xmlstr.dictionary = NULL;
    xmlstr.id         = 0;
    str = NULL;
    count = 0;
    hr = WsGetXmlAttribute( reader, &xmlstr, heap, &str, &count, NULL );
    todo_wine ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( str != NULL, "str not set\n" );
    todo_wine ok( count == 5, "got %u\n", count );
    /* string is not null-terminated */
    if (str) ok( !memcmp( str, valueW, count * sizeof(WCHAR) ), "wrong data\n" );

    xmlstr.bytes      = (BYTE *)"none";
    xmlstr.length     = sizeof("none") - 1;
    xmlstr.dictionary = NULL;
    xmlstr.id         = 0;
    str = (WCHAR *)0xdeadbeef;
    count = 0xdeadbeef;
    hr = WsGetXmlAttribute( reader, &xmlstr, heap, &str, &count, NULL );
    todo_wine ok( hr == S_FALSE, "got %08x\n", hr );
    todo_wine ok( str == NULL, "str not set\n" );
    todo_wine ok( !count, "got %u\n", count );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_WsXmlStringEquals(void)
{
    BYTE bom[] = {0xef,0xbb,0xbf};
    WS_XML_STRING str1 = {0, NULL}, str2 = {0, NULL};
    HRESULT hr;

    hr = WsXmlStringEquals( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsXmlStringEquals( &str1, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsXmlStringEquals( NULL, &str2, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    str1.length = 1;
    str1.bytes  = (BYTE *)"a";
    hr = WsXmlStringEquals( &str1, &str1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    str2.length = 1;
    str2.bytes  = (BYTE *)"b";
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );

    str2.length = 1;
    str2.bytes  = bom;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );

    str1.length = 3;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );

    str2.length = 3;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );

    str1.length = 3;
    str1.bytes  = bom;
    hr = WsXmlStringEquals( &str1, &str2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
}

static void test_WsAlloc(void)
{
    HRESULT hr;
    WS_HEAP *heap;
    void *ptr;

    hr = WsCreateHeap( 256, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    ptr = NULL;
    hr = WsAlloc( NULL, 16, &ptr, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( ptr == NULL, "ptr set\n" );

    ptr = NULL;
    hr = WsAlloc( heap, 512, &ptr, NULL );
    todo_wine ok( hr == WS_E_QUOTA_EXCEEDED, "got %08x\n", hr );
    todo_wine ok( ptr == NULL, "ptr not set\n" );

    ptr = NULL;
    hr = WsAlloc( heap, 16, &ptr, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ptr != NULL, "ptr not set\n" );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( NULL, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* reader must be set to an XML buffer */
    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = set_input( reader, data8, sizeof(data8) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    WsFreeReader( reader );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* <a><b/></a> */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_EOF, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* first element is child node of BOF node */
    hr = WsMoveReader( reader, WS_MOVE_TO_BOF, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    /* EOF node is last child of BOF node */
    hr = WsMoveReader( reader, WS_MOVE_TO_BOF, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_END_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "a", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_BOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_PARENT_ELEMENT, NULL, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    WsFreeWriter( writer );
    WsFreeHeap( heap );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateWriter( NULL, 0, &writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* <a><b>test</b></a> */
    hr = WsWriteStartElement( writer, NULL, &localname, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteStartElement( writer, NULL, &localname2, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes  = (BYTE *)"test";
    utf8.value.length = sizeof("test") - 1;
    hr = WsWriteText( writer, &utf8.text, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteEndElement( writer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsMoveReader( reader, WS_MOVE_TO_ROOT_ELEMENT, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsMoveReader( reader, WS_MOVE_TO_CHILD_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 1, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "b", 1 ), "wrong data\n" );

    hr = WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, size, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
}

static void test_simple_struct_type(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, NULL,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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

    test = NULL;
    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str><str>test2</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    test = NULL;
    prepare_struct_type_test( reader, "<?xml version=\"1.0\" encoding=\"utf-8\"?><str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !lstrcmpW( test->str, testW ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    test = NULL;
    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !lstrcmpW( test->str, testW ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    test = NULL;
    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !lstrcmpW( test->str, testW ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    prepare_struct_type_test( reader, "<str>test</str>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 3, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "str", 3 ), "wrong data\n" );

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !lstrcmpW( test->str, testW ), "wrong data\n" );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    /* attribute field mapping */
    f.mapping = WS_ATTRIBUTE_FIELD_MAPPING;

    test = NULL;
    prepare_struct_type_test( reader, "<test str=\"test\"/>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test)
    {
        ok( test->str != NULL, "str not set\n" );
        if (test->str) ok( !lstrcmpW( test->str, testW ), "wrong data test %p test->str %p\n", test, test->str );
    }

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFillReader( reader, sizeof(test) - 1, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_CDATA, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        ok( node->nodeType == WS_XML_NODE_TYPE_TEXT, "got %u\n", node->nodeType );
        ok( text->text != NULL, "text not set\n" );
        if (text->text)
        {
            WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
            ok( utf8->text.textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", utf8->text.textType );
            ok( utf8->value.length == 6, "got %u\n", utf8->value.length );
            ok( !memcmp( utf8->value.bytes, "<data>", 6 ), "wrong data\n" );
        }
    }

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_CDATA, "got %u\n", node->nodeType );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node) ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    WsFreeReader( reader );
}

static void test_WsFindAttribute(void)
{
    static const char test[] = "<t attr='value' attr2='value2'></t>";
    WS_XML_STRING ns = {0, NULL}, localname = {4, (BYTE *)"attr"};
    WS_XML_STRING localname2 = {5, (BYTE *)"attr2"}, localname3 = {5, (BYTE *)"attr3"};
    WS_XML_READER *reader;
    ULONG index;
    HRESULT hr;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFindAttribute( reader, &localname, &ns, TRUE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFindAttribute( reader, &localname, NULL, TRUE, &index, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsFindAttribute( reader, NULL, &ns, TRUE, &index, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname, &ns, TRUE, &index, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !index, "got %u\n", index );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname2, &ns, TRUE, &index, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( index == 1, "got %u\n", index );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname, &ns, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    ok( index == 0xdeadbeef, "got %u\n", index );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname3, &ns, TRUE, &index, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( index == 0xdeadbeef, "got %u\n", index );

    hr = set_input( reader, test, sizeof(test) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadNode( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    index = 0xdeadbeef;
    hr = WsFindAttribute( reader, &localname3, &ns, FALSE, &index, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );
    ok( index == ~0u, "got %u\n", index );

    WsFreeReader( reader );
}

static void prepare_namespace_test( WS_XML_READER *reader, const char *data )
{
    HRESULT hr;
    ULONG size = strlen( data );

    hr = set_input( reader, data, size );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
}

static void test_WsGetNamespaceFromPrefix(void)
{
    WS_XML_STRING prefix = {0, NULL};
    const WS_XML_STRING *ns;
    const WS_XML_NODE *node;
    WS_XML_READER *reader;
    HRESULT hr;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, NULL, FALSE, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, NULL, FALSE, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsGetNamespaceFromPrefix( NULL, &prefix, FALSE, &ns, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    ns = (const WS_XML_STRING *)0xdeadbeef;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    ok( ns == (const WS_XML_STRING *)0xdeadbeef, "ns set\n" );

    hr = set_input( reader, "<prefix:t xmlns:prefix2='ns'/>", sizeof("<prefix:t xmlns:prefix2='ns'/>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %u\n", ns->length );

    prepare_namespace_test( reader, "<t xmls='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %u\n", ns->length );

    prepare_namespace_test( reader, "<prefix:t xmlns:prefix='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns) ok( !ns->length, "got %u\n", ns->length );

    prepare_namespace_test( reader, "<prefix:t xmlns:prefix='ns'></t>" );
    prefix.bytes = (BYTE *)"prefix";
    prefix.length = 6;
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 2, "got %u\n", ns->length );
        ok( !memcmp( ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t xmlns:prefix='ns'></t>" );
    ns = NULL;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 2, "got %u\n", ns->length );
        ok( !memcmp( ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:prefix='ns'></t>", sizeof("<t xmlns:prefix='ns'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;
        WS_XML_UTF8_TEXT *text;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %u\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes not set\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( !elem->ns->length, "got %u\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( elem->attributeCount == 1, "got %u\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->singleQuote, "singleQuote not set\n" );
        ok( attr->isXmlNs, "isXmlNs not set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 6, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->prefix->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 6, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
        ok( attr->value != NULL, "value not set\n" );

        text = (WS_XML_UTF8_TEXT *)attr->value;
        ok( attr->value->textType == WS_XML_TEXT_TYPE_UTF8, "got %u\n", attr->value->textType );
        ok( !text->value.length, "got %u\n", text->value.length );
        ok( text->value.bytes == NULL, "bytes set\n" );
    }

    prepare_namespace_test( reader, "<t xmlns:prefix='ns'></t>" );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    prefix.bytes = (BYTE *)"xml";
    prefix.length = 3;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 36, "got %u\n", ns->length );
        ok( !memcmp( ns->bytes, "http://www.w3.org/XML/1998/namespace", 36 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t></t>" );
    ns = NULL;
    prefix.bytes = (BYTE *)"xmlns";
    prefix.length = 5;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( ns != NULL, "ns not set\n" );
    if (ns)
    {
        ok( ns->length == 29, "got %u\n", ns->length );
        ok( !memcmp( ns->bytes, "http://www.w3.org/2000/xmlns/", 29 ), "wrong data\n" );
    }

    prepare_namespace_test( reader, "<t></t>" );
    ns = (WS_XML_STRING *)0xdeadbeef;
    prefix.bytes = (BYTE *)"prefix2";
    prefix.length = 7;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, TRUE, &ns, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );
    ok( ns == (WS_XML_STRING *)0xdeadbeef, "ns set\n" );

    prepare_namespace_test( reader, "<t></t>" );
    ns = (WS_XML_STRING *)0xdeadbeef;
    prefix.bytes = (BYTE *)"prefix2";
    prefix.length = 7;
    hr = WsGetNamespaceFromPrefix( reader, &prefix, FALSE, &ns, NULL );
    ok( hr == S_FALSE, "got %08x\n", hr );
    ok( ns == NULL, "ns not set\n" );

    hr = set_input( reader, "<t prefix:attr='' xmlns:prefix='ns'></t>", sizeof("<t prefix:attr='' xmlns:prefix='ns'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->attributeCount == 2, "got %u\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->singleQuote, "singleQuote not set\n" );
        ok( !attr->isXmlNs, "isXmlNs is set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 6, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->prefix->bytes, "prefix", 6 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 4, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "attr", 4 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:p='ns'><u xmlns:p='ns2'/></t>", sizeof("<t xmlns:p='ns'><u xmlns:p='ns2'/></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns'><p:u p:a=''/></t>", sizeof("<t xmlns:p='ns'><p:u p:a=''/></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->attributeCount == 1, "got %u\n", elem->attributeCount );
        ok( elem->attributes != NULL, "attributes not set\n" );

        attr = elem->attributes[0];
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( attr->prefix->length == 1, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes != NULL, "bytes set\n" );
        ok( !memcmp( attr->prefix->bytes, "p", 1 ), "wrong data\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 1, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "a", 1 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns='ns'></t>", sizeof("<t xmlns='ns'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (node)
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
        WS_XML_ATTRIBUTE *attr;

        ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
        ok( elem->prefix != NULL, "prefix not set\n" );
        ok( !elem->prefix->length, "got %u\n", elem->prefix->length );
        ok( elem->prefix->bytes == NULL, "bytes not set\n" );
        ok( elem->ns != NULL, "ns not set\n" );
        ok( elem->ns->length == 2, "got %u\n", elem->ns->length );
        ok( elem->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( elem->ns->bytes, "ns", 2 ), "wrong data\n" );

        attr = elem->attributes[0];
        ok( attr->isXmlNs, "isXmlNs is not set\n" );
        ok( attr->prefix != NULL, "prefix not set\n" );
        ok( !attr->prefix->length, "got %u\n", attr->prefix->length );
        ok( attr->prefix->bytes == NULL, "bytes set\n" );
        ok( attr->localName != NULL, "localName not set\n" );
        ok( attr->localName->length == 5, "got %u\n", attr->localName->length );
        ok( !memcmp( attr->localName->bytes, "xmlns", 5 ), "wrong data\n" );
        ok( attr->ns != NULL, "ns not set\n" );
        ok( attr->ns->length == 2, "got %u\n", attr->ns->length );
        ok( attr->ns->bytes != NULL, "bytes not set\n" );
        ok( !memcmp( attr->ns->bytes, "ns", 2 ), "wrong data\n" );
    }

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:p='ns2'></t>", sizeof("<t xmlns:p='ns' xmlns:p='ns2'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:p='ns'></t>", sizeof("<t xmlns:p='ns' xmlns:p='ns'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    todo_wine ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    hr = set_input( reader, "<t xmlns:p='ns' xmlns:P='ns2'></t>", sizeof("<t xmlns:p='ns' xmlns:P='ns2'></t>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeReader( reader );
}

static void test_text_field_mapping(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

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
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->str != NULL, "str not set\n" );
    ok( !lstrcmpW( test->str, testW ), "got %s\n", wine_dbgstr_w(test->str) );

    WsFreeReader( reader );
    WsFreeHeap( heap );
}

static void test_complex_struct_type(void)
{
    static const WCHAR timestampW[] =
        {'2','0','1','5','-','0','9','-','0','3','T','1','8',':','4','7',':','5','4',0};
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* element content type mapping */
    prepare_struct_type_test( reader, data11 );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 12, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "OfficeConfig", 12 ), "wrong data\n" );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 8, "got %u\n", elem->localName->length );
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
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( !lstrcmpW( test->services->generationtime, timestampW ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_END_ELEMENT, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    hr = WsReadEndElement( reader, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    /* element type mapping */
    prepare_struct_type_test( reader, data11 );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    elem = (const WS_XML_ELEMENT_NODE *)node;
    ok( elem->node.nodeType == WS_XML_NODE_TYPE_ELEMENT, "got %u\n", elem->node.nodeType );
    ok( elem->localName->length == 12, "got %u\n", elem->localName->length );
    ok( !memcmp( elem->localName->bytes, "OfficeConfig", 12 ), "wrong data\n" );

    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), error );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    if (test) ok( !lstrcmpW( test->services->generationtime, timestampW ), "wrong data\n" );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( node->nodeType == WS_XML_NODE_TYPE_EOF, "got %u\n", node->nodeType );

    WsFreeReader( reader );
    WsFreeHeap( heap );
    WsFreeError( error );
}

static void test_repeating_element(void)
{
    static const WCHAR oneW[] = {'1',0}, twoW[] = {'2',0};
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

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    prepare_struct_type_test( reader, data12 );

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
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( test->service_count == 2, "got %u\n", test->service_count );
    ok( test->service[0].id == 1, "got %u\n", test->service[0].id );
    ok( test->service[1].id == 2, "got %u\n", test->service[1].id );

    /* array of pointers */
    prepare_struct_type_test( reader, data12 );
    f.options = WS_FIELD_POINTER;
    test4 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test4, sizeof(test4), NULL );
    ok( hr == S_OK || broken(hr == E_INVALIDARG) /* win7 */, "got %08x\n", hr );
    if (test4)
    {
        ok( test4->service != NULL, "service not set\n" );
        ok( test4->service_count == 2, "got %u\n", test4->service_count );
        ok( test4->service[0]->id == 1, "got %u\n", test4->service[0]->id );
        ok( test4->service[1]->id == 2, "got %u\n", test4->service[1]->id );
    }

    /* item range */
    prepare_struct_type_test( reader, data13 );
    f.options = 0;
    range.minItemCount = 0;
    range.maxItemCount = 1;
    f.itemRange = &range;
    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( !test->service_count, "got %u\n", test->service_count );

    /* wrapper element */
    prepare_struct_type_test( reader, data14 );
    f.itemRange = NULL;
    f.localName = &str_wrapper;
    f.ns        = &str_ns;
    test = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test, sizeof(test), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test != NULL, "test not set\n" );
    ok( test->service != NULL, "service not set\n" );
    ok( test->service_count == 2, "got %u\n", test->service_count );
    ok( test->service[0].id == 1, "got %u\n", test->service[0].id );
    ok( test->service[1].id == 2, "got %u\n", test->service[1].id );

    /* repeating text field mapping */
    prepare_struct_type_test( reader, data15 );
    f2.mapping   = WS_TEXT_FIELD_MAPPING;
    f2.localName = NULL;
    f2.ns        = NULL;
    f2.type      = WS_WSZ_TYPE;
    s2.size      = sizeof(struct service2);
    s2.alignment = TYPE_ALIGNMENT(struct service2);
    test2 = NULL;
    hr = WsReadType( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRUCT_TYPE, &s,
                     WS_READ_REQUIRED_POINTER, heap, &test2, sizeof(test2), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test2 != NULL, "test2 not set\n" );
    ok( test2->service != NULL, "service not set\n" );
    ok( test2->service_count == 2, "got %u\n", test2->service_count );
    ok( !lstrcmpW( test2->service[0].id, oneW ), "wrong data\n" );
    ok( !lstrcmpW( test2->service[1].id, twoW ), "wrong data\n" );

    /* repeating attribute field + text field mapping */
    prepare_struct_type_test( reader, data16 );
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
    ok( hr == S_OK, "got %08x\n", hr );
    ok( test3 != NULL, "test3 not set\n" );
    ok( test3->service != NULL, "service not set\n" );
    ok( test3->service_count == 2, "got %u\n", test3->service_count );
    ok( !lstrcmpW( test3->service[0].name, oneW ), "wrong data\n" );
    ok( !lstrcmpW( test3->service[0].id, oneW ), "wrong data\n" );
    ok( !lstrcmpW( test3->service[1].name, twoW ), "wrong data\n" );
    ok( !lstrcmpW( test3->service[1].id, twoW ), "wrong data\n" );

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
    ok( hr == S_OK, "got %08x\n", hr );

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

    hr = WsAlloc( heap, 128, &ptr, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( requested == 128, "got %u\n", (ULONG)requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( actual == 128, "got %u\n", (ULONG)actual );

    hr = WsAlloc( heap, 1, &ptr, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( requested == 129, "got %u\n", (ULONG)requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( actual == 384, "got %u\n", (ULONG)actual );

    hr = WsResetHeap( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsResetHeap( heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    requested = 0xdeadbeef;
    size = sizeof(requested);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_REQUESTED_SIZE, &requested, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !requested, "got %u\n", (ULONG)requested );

    actual = 0xdeadbeef;
    size = sizeof(actual);
    hr = WsGetHeapProperty( heap, WS_HEAP_PROPERTY_ACTUAL_SIZE, &actual, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( actual == 128, "got %u\n", (ULONG)actual );

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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        memset( &date, 0, sizeof(date) );
        prepare_type_test( reader, tests[i].str, strlen(tests[i].str) );
        hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_DATETIME_TYPE, NULL,
                         WS_READ_REQUIRED_VALUE, heap, &date, sizeof(date), NULL );
        ok( hr == tests[i].hr, "%u: got %08x\n", i, hr );
        if (hr == S_OK)
        {
            ok( date.ticks == tests[i].ticks, "%u: got %x%08x\n", i, (ULONG)(date.ticks >> 32), (ULONG)date.ticks );
            ok( date.format == tests[i].format, "%u: got %u\n", i, date.format );
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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    dt.ticks  = 0x701ce172277000;
    dt.format = WS_DATETIME_FORMAT_UTC;
    hr = WsDateTimeToFileTime( &dt, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsDateTimeToFileTime( NULL, &ft, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        memset( &ft, 0, sizeof(ft) );
        hr = WsDateTimeToFileTime( &tests[i].dt, &ft, NULL );
        ok( hr == tests[i].hr, "%u: got %08x\n", i, hr );
        if (hr == S_OK)
        {
            ok( ft.dwLowDateTime == tests[i].ft.dwLowDateTime, "%u: got %08x\n", i, ft.dwLowDateTime );
            ok( ft.dwHighDateTime == tests[i].ft.dwHighDateTime, "%u: got %08x\n", i, ft.dwHighDateTime );
        }
    }
}

static void test_WsFileTimeToDateTime(void)
{
    WS_DATETIME dt;
    FILETIME ft;
    HRESULT hr;

    hr = WsFileTimeToDateTime( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    ft.dwLowDateTime = ft.dwHighDateTime = 0;
    hr = WsFileTimeToDateTime( &ft, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsFileTimeToDateTime( NULL, &dt, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    dt.ticks = 0xdeadbeef;
    dt.format = 0xdeadbeef;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( dt.ticks == 0x701ce1722770000, "got %x%08x\n", (ULONG)(dt.ticks >> 32), (ULONG)dt.ticks );
    ok( dt.format == WS_DATETIME_FORMAT_UTC, "got %u\n", dt.format );

    ft.dwLowDateTime  = 0xd1c03fff;
    ft.dwHighDateTime = 0x24c85a5e;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( dt.ticks == 0x2bca2875f4373fff, "got %x%08x\n", (ULONG)(dt.ticks >> 32), (ULONG)dt.ticks );
    ok( dt.format == WS_DATETIME_FORMAT_UTC, "got %u\n", dt.format );

    ft.dwLowDateTime++;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    ft.dwLowDateTime  = 0xdd88ffff;
    ft.dwHighDateTime = 0xf8fe31e8;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    ft.dwLowDateTime++;
    hr = WsFileTimeToDateTime( &ft, &dt, NULL );
    ok( hr == WS_E_NUMERIC_OVERFLOW, "got %08x\n", hr );
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
        {"<t>+</t>", S_OK, 0},
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        val = 0;
        prepare_type_test( reader, tests[i].str, strlen(tests[i].str) );
        hr = WsReadType( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, WS_DOUBLE_TYPE, NULL,
                         WS_READ_REQUIRED_VALUE, heap, &val, sizeof(val), NULL );
        ok( hr == tests[i].hr, "%u: got %08x\n", i, hr );
        if (hr == tests[i].hr) ok( val == tests[i].val, "%u: got %x%08x\n", i, (ULONG)(val >> 32), (ULONG)val );
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
    ok( hr == S_OK, "got %08x\n", hr );

    desc.elementLocalName = &localname;
    desc.elementNs        = &ns;
    desc.type             = WS_UINT32_TYPE;
    desc.typeDescription  = NULL;

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( NULL, &desc, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( reader, NULL, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadElement( reader, &desc, WS_READ_REQUIRED_VALUE, NULL, NULL, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    val = 0xdeadbeef;
    hr = WsReadElement( reader, &desc, WS_READ_REQUIRED_VALUE, NULL, &val, sizeof(val), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val == 1, "got %u\n", val );

    WsFreeReader( reader );
}

static void test_WsReadValue(void)
{
    HRESULT hr;
    WS_XML_READER *reader;
    UINT32 val;

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( NULL, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, NULL, sizeof(val), NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    /* reader must be positioned correctly */
    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    prepare_struct_type_test( reader, "<t>1</t>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadStartElement( reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    val = 0xdeadbeef;
    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val == 1, "got %u\n", val );

    prepare_struct_type_test( reader, "<u t='1'></u>" );
    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadValue( reader, WS_UINT32_VALUE_TYPE, &val, sizeof(val), NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %08x\n", hr );

    WsFreeReader( reader );
}

static void test_WsResetError(void)
{
    WS_ERROR_PROPERTY prop;
    ULONG size, code;
    WS_ERROR *error;
    LANGID langid;
    HRESULT hr;

    hr = WsResetError( NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    error = NULL;
    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( error != NULL, "error not set\n" );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetError( error );
    ok( hr == S_OK, "got %08x\n", hr );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !code, "got %u\n", code );

    WsFreeError( error );

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

    hr = WsResetError( error );
    ok( hr == S_OK, "got %08x\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );

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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* reader must be set to an XML buffer */
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = set_input( reader, "<t/>", sizeof("<t/>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetInputToBuffer( reader, buffer, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderPosition( reader, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetReaderPosition( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf1, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetInputToBuffer( reader, buf1, NULL, 0, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsSetReaderPosition( reader, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    pos.buffer = pos.node = NULL;
    hr = WsGetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( pos.buffer == buf1, "wrong buffer\n" );
    ok( pos.node != NULL, "node not set\n" );

    hr = WsSetReaderPosition( reader, &pos, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    /* different buffer */
    hr = WsCreateXmlBuffer( heap, NULL, 0, &buf2, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    pos.buffer = buf2;
    hr = WsSetReaderPosition( reader, &pos, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

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
    ok( hr == S_OK, "got %08x\n", hr );

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        hr = set_input( reader, tests[i].str, strlen(tests[i].str) );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        hr = WsReadNode( reader, NULL );
        ok( hr == tests[i].hr, "%u: got %08x\n", i, hr );
        if (hr != S_OK) continue;

        hr = WsGetReaderNode( reader, &node, NULL );
        ok( hr == S_OK, "%u: got %08x\n", i, hr );

        utf8 = (const WS_XML_UTF8_TEXT *)((const WS_XML_TEXT_NODE *)node)->text;
        ok( utf8->value.length == strlen(tests[i].res), "%u: got %u\n", i, utf8->value.length );
        ok( !memcmp( utf8->value.bytes, tests[i].res, strlen(tests[i].res) ), "%u: wrong data\n", i );
    }

    hr = set_input( reader, "<t a='&#xA;&#xA;'/>", sizeof("<t a='&#xA;&#xA;'/>") - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadToStartElement( reader, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsGetReaderNode( reader, &node, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    utf8 = (const WS_XML_UTF8_TEXT *)((const WS_XML_ELEMENT_NODE *)node)->attributes[0]->value;
    ok( utf8->value.length == 2, "got %u\n", utf8->value.length );
    ok( !memcmp( utf8->value.bytes, "\n\n", 2 ), "wrong data\n" );

    WsFreeReader( reader );
}

static void test_field_options(void)
{
    static const char xml[] =
        "<t xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\"><wsz i:nil=\"true\"/>"
        "<s i:nil=\"true\"/></t>";
    static const GUID guid_null = {0};
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
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateReader( NULL, 0, &reader, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = set_input( reader, xml, sizeof(xml) - 1 );
    ok( hr == S_OK, "got %08x\n", hr );

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
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !test->wsz, "wsz is set\n" );
    ok( !test->s, "s is set\n" );
    ok( test->int32 == -1, "got %d\n", test->int32 );
    ok( IsEqualGUID( &test->guid, &guid_null ), "wrong guid\n" );

    WsFreeReader( reader );
    WsFreeHeap( heap );
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
}
