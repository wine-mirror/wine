/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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


#define COBJMACROS

#include "windows.h"
#include "ole2.h"
#include "xmldom.h"
#include "msxml2.h"
#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

const CLSID CLSID_DOMDocument2 = {0xf6d90f11, 0x9c73, 0x11d3, {0xb3, 0x2e, 0x00, 0xc0, 0x4f, 0x99, 0x0b, 0xb4}};
const IID IID_IXMLDOMDocument2 = {0x2933bf95, 0x7b36, 0x11d2, {0xb2, 0x0e, 0x00, 0xc0, 0x4f, 0x98, 0x3e, 0x60}};

static const WCHAR szEmpty[] = { 0 };
static const WCHAR szIncomplete[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',0
};
static const WCHAR szComplete1[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','p','e','n','>','<','/','o','p','e','n','>','\n',0
};
static const WCHAR szComplete2[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','o','>','<','/','o','>','\n',0
};
static const WCHAR szComplete3[] = {
    '<','?','x','m','l',' ',
    'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','a','>','<','/','a','>','\n',0
};
static const WCHAR szComplete4[] = {
    '<','?','x','m','l',' ','v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>','\n',
    '<','l','c',' ','d','l','=','\'','s','t','r','1','\'','>','\n',
        '<','b','s',' ','v','r','=','\'','s','t','r','2','\'',' ','s','z','=','\'','1','2','3','4','\'','>',
            'f','n','1','.','t','x','t','\n',
        '<','/','b','s','>','\n',
        '<','p','r',' ','i','d','=','\'','s','t','r','3','\'',' ','v','r','=','\'','1','.','2','.','3','\'',' ',
                    'p','n','=','\'','w','i','n','e',' ','2','0','0','5','0','8','0','4','\'','>','\n',
            'f','n','2','.','t','x','t','\n',
        '<','/','p','r','>','\n',
        '<','e','m','p','t','y','>','<','/','e','m','p','t','y','>','\n',
        '<','f','o','>','\n',
            '<','b','a','>','\n',
                'f','1','\n',
            '<','/','b','a','>','\n',
        '<','/','f','o','>','\n',
    '<','/','l','c','>','\n',0
};
static const WCHAR szComplete5[] = {
    '<','S',':','s','e','a','r','c','h',' ','x','m','l','n','s',':','D','=','"','D','A','V',':','"',' ',
    'x','m','l','n','s',':','C','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y','"',
    ' ','x','m','l','n','s',':','S','=','"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','o','f','f','i','c','e',':','c','l','i','p','g','a','l','l','e','r','y',':','s','e','a','r','c','h','"','>',
        '<','S',':','s','c','o','p','e','>',
            '<','S',':','d','e','e','p','>','/','<','/','S',':','d','e','e','p','>',
        '<','/','S',':','s','c','o','p','e','>',
        '<','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
            '<','C',':','t','e','x','t','o','r','p','r','o','p','e','r','t','y','/','>',
            'c','o','m','p','u','t','e','r',
        '<','/','S',':','c','o','n','t','e','n','t','f','r','e','e','t','e','x','t','>',
    '<','/','S',':','s','e','a','r','c','h','>',0
};

static const CHAR szExampleXML[] =
"<?xml version='1.0' encoding='utf-8'?>\n"
"<root xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"    <elem>\n"
"        <a>A1 field</a>\n"
"        <b>B1 field</b>\n"
"        <c>C1 field</c>\n"
"        <description xmlns:foo='http://www.winehq.org' xmlns:bar='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"            <html xmlns='http://www.w3.org/1999/xhtml'>\n"
"                This is <strong>a</strong> <i>description</i>. <bar:x/>\n"
"            </html>\n"
"        </description>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A2 field</a>\n"
"        <b>B2 field</b>\n"
"        <c type=\"old\">C2 field</c>\n"
"    </elem>\n"
"\n"
"    <elem xmlns='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"        <a>A3 field</a>\n"
"        <b>B3 field</b>\n"
"        <c>C3 field</c>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A4 field</a>\n"
"        <b>B4 field</b>\n"
"        <foo:c>C4 field</foo:c>\n"
"    </elem>\n"
"</root>\n";

static const WCHAR szNonExistentFile[] = {
    'c', ':', '\\', 'N', 'o', 'n', 'e', 'x', 'i', 's', 't', 'e', 'n', 't', '.', 'x', 'm', 'l', 0
};
static const WCHAR szDocument[] = {
    '#', 'd', 'o', 'c', 'u', 'm', 'e', 'n', 't', 0
};

static const WCHAR szOpen[] = { 'o','p','e','n',0 };
static WCHAR szdl[] = { 'd','l',0 };
static const WCHAR szvr[] = { 'v','r',0 };
static const WCHAR szlc[] = { 'l','c',0 };
static WCHAR szbs[] = { 'b','s',0 };
static const WCHAR szstr1[] = { 's','t','r','1',0 };
static const WCHAR szstr2[] = { 's','t','r','2',0 };
static const WCHAR szstar[] = { '*',0 };
static const WCHAR szfn1_txt[] = {'f','n','1','.','t','x','t',0};

#define expect_bstr_eq_and_free(bstr, expect) { \
    BSTR bstrExp = alloc_str_from_narrow(expect); \
    ok(lstrcmpW(bstr, bstrExp) == 0, "String differs\n"); \
    SysFreeString(bstr); \
    SysFreeString(bstrExp); \
}

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_check(expr) { \
    HRESULT r = expr; \
    ok(r == S_OK, #expr " returned %x\n", r); \
}

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %x\n", r, expect); \
}

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

BSTR alloced_bstrs[256];
int alloced_bstrs_count = 0;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < sizeof(alloced_bstrs)/sizeof(alloced_bstrs[0]));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs()
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static VARIANT _variantbstr_(const char *str)
{
    VARIANT v;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_(str);
    return v;
}

static void get_str_for_type(DOMNodeType type, char *buf)
{
    switch (type)
    {
        case NODE_ATTRIBUTE:
            strcpy(buf, "A");
            break;
        case NODE_ELEMENT:
            strcpy(buf, "E");
            break;
        case NODE_DOCUMENT:
            strcpy(buf, "D");
            break;
        default:
            wsprintfA(buf, "[%d]", type);
    }
}

static int get_node_position(IXMLDOMNode *node)
{
    HRESULT r;
    int pos = 0;

    IXMLDOMNode_AddRef(node);
    do
    {
        IXMLDOMNode *new_node;

        pos++;
        r = IXMLDOMNode_get_previousSibling(node, &new_node);
        ok(!FAILED(r), "get_previousSibling failed\n");
        IXMLDOMNode_Release(node);
        node = new_node;
    } while (r == S_OK);
    return pos;
}

static void node_to_string(IXMLDOMNode *node, char *buf)
{
    HRESULT r = S_OK;
    DOMNodeType type;

    if (node == NULL)
    {
        lstrcpyA(buf, "(null)");
        return;
    }

    IXMLDOMNode_AddRef(node);
    while (r == S_OK)
    {
        IXMLDOMNode *new_node;

        ole_check(IXMLDOMNode_get_nodeType(node, &type));
        get_str_for_type(type, buf);
        buf+=strlen(buf);

        if (type == NODE_ATTRIBUTE)
        {
            BSTR bstr;
            ole_check(IXMLDOMNode_get_nodeName(node, &bstr));
            *(buf++) = '\'';
            wsprintfA(buf, "%ws", bstr);
            buf += strlen(buf);
            *(buf++) = '\'';
            SysFreeString(bstr);

            r = IXMLDOMNode_selectSingleNode(node, _bstr_(".."), &new_node);
        }
        else
        {
            int pos = get_node_position(node);
            DOMNodeType parent_type = NODE_INVALID;
            r = IXMLDOMNode_get_parentNode(node, &new_node);

            /* currently wine doesn't create a node for the <?xml ... ?>. To be able to test query
             * results we "fix" it */
            if (r == S_OK)
                ole_check(IXMLDOMNode_get_nodeType(node, &parent_type));
            /* we need also to workaround the no document node problem - see below */
            if (((r == S_FALSE && type != NODE_DOCUMENT) || parent_type == NODE_DOCUMENT) && type != NODE_PROCESSING_INSTRUCTION && pos==1)
            {
                todo_wine ok(FALSE, "The first child of the document node in MSXML is the <?xml ... ?> processing instruction\n");
                pos++;
            }
            wsprintf(buf, "%d", pos);
            buf += strlen(buf);
        }

        ok(!FAILED(r), "get_parentNode failed (%08x)\n", r);
        IXMLDOMNode_Release(node);
        node = new_node;
        if (r == S_OK)
            *(buf++) = '.';
    }

    /* currently we can't access document node in wine. All our examples this is the
     * root node so to be able to test query results we add it */
    if (type != NODE_DOCUMENT)
    {
        todo_wine ok(FALSE, "Document node is not the last returned node!\n");
        *(buf++) = '.';
        *(buf++) = 'D';
        *(buf++) = '1';
    }
    *buf = 0;
}

static char *list_to_string(IXMLDOMNodeList *list)
{
    static char buf[4096];
    char *pos = buf;
    long len = 0;
    int i;

    if (list == NULL)
    {
        lstrcpyA(buf, "(null)");
        return buf;
    }
    ole_check(IXMLDOMNodeList_get_length(list, &len));
    for (i = 0; i < len; i++)
    {
        IXMLDOMNode *node;
        if (i > 0)
            *(pos++) = ' ';
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        node_to_string(node, pos);
        pos += strlen(pos);
        IXMLDOMNode_Release(node);
    }
    *pos = 0;
    return buf;
}

#define expect_node(node, expstr) { char str[4096]; node_to_string(node, str); ok(strcmp(str, expstr)==0, "Invalid node: %s, exptected %s\n", str, expstr); }
#define expect_list_and_release(list, expstr) { char *str = list_to_string(list); ok(strcmp(str, expstr)==0, "Invalid node list: %s, exptected %s\n", str, expstr); if (list) IXMLDOMNodeList_Release(list); }

static void test_domdoc( void )
{
    HRESULT r;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMParseError *error;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    VARIANT var;
    BSTR str;
    long code;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    /* try some stupid things */
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML failed\n");

    b = VARIANT_TRUE;
    r = IXMLDOMDocument_loadXML( doc, NULL, &b );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == VARIANT_FALSE, "failed to load XML string\n");

    /* try to load a document from a nonexistent file */
    b = VARIANT_TRUE;
    str = SysAllocString( szNonExistentFile );
    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = str;

    r = IXMLDOMDocument_load( doc, var, &b);
    ok( r == S_FALSE, "load (from file) failed\n");
    ok( b == VARIANT_FALSE, "failed to load XML file\n");
    SysFreeString( str );

    /* try load an empty document */
    b = VARIANT_TRUE;
    str = SysAllocString( szEmpty );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == VARIANT_FALSE, "failed to load XML string\n");
    SysFreeString( str );

    /* check that there's no document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try finding a node */
    node = NULL;
    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_selectSingleNode( doc, str, &node );
    ok( r == S_FALSE, "ret %08x\n", r );
    SysFreeString( str );

    b = VARIANT_TRUE;
    str = SysAllocString( szIncomplete );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == VARIANT_FALSE, "failed to load XML string\n");
    SysFreeString( str );

    /* check that there's no document element */
    element = (IXMLDOMElement*)1;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");
    ok( element == NULL, "Element should be NULL\n");

    /* try to load something valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete1 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try with a null out pointer - crashes */
    if (0)
    {
        r = IXMLDOMDocument_get_documentElement( doc, NULL );
        ok( r == S_OK, "should be no document element\n");
    }

    /* check if nodename is correct */
    r = IXMLDOMDocument_get_nodeName( doc, NULL );
    ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

    /* content doesn't matter here */
    str = SysAllocString( szNonExistentFile );
    r = IXMLDOMDocument_get_nodeName( doc, &str );
    ok ( r == S_OK, "get_nodeName wrong code\n");
    ok ( str != NULL, "str is null\n");
    ok( !lstrcmpW( str, szDocument ), "incorrect nodeName\n");
    SysFreeString( str );


    /* check that there's a document element */
    element = NULL;
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    if( element )
    {
        BSTR tag = NULL;

        /* check if the tag is correct */
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szOpen ), "incorrect tag name\n");
        SysFreeString( tag );

        /* figure out what happens if we try to reload the document */
        str = SysAllocString( szComplete2 );
        r = IXMLDOMDocument_loadXML( doc, str, &b );
        ok( r == S_OK, "loadXML failed\n");
        ok( b == VARIANT_TRUE, "failed to load XML string\n");
        SysFreeString( str );

        /* check if the tag is still correct */
        tag = NULL;
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szOpen ), "incorrect tag name\n");
        SysFreeString( tag );

        IXMLDOMElement_Release( element );
        element = NULL;
    }

    /* as soon as we call loadXML again, the document element will disappear */
    b = 2;
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == 2, "variant modified\n");
    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_FALSE, "should be no document element\n");

    /* try to load something else simple and valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete3 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try something a little more complicated */
    b = FALSE;
    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_parseError( doc, &error );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMParseError_get_errorCode( error, &code );
    ok( r == S_FALSE, "returns %08x\n", r );
    ok( code == 0, "code %ld\n", code );
    IXMLDOMParseError_Release( error );

    r = IXMLDOMDocument_Release( doc );
    ok( r == 0, "document ref count incorrect\n");

}

static void test_domnode( void )
{
    HRESULT r;
    IXMLDOMDocument *doc = NULL, *owner = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNamedNodeMap *map = NULL;
    IXMLDOMNode *node = NULL, *next = NULL;
    IXMLDOMNodeList *list = NULL;
    DOMNodeType type = NODE_INVALID;
    VARIANT_BOOL b;
    BSTR str;
    VARIANT var;
    long count;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    b = FALSE;
    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    if (doc)
    {
        b = 1;
        r = IXMLDOMNode_hasChildNodes( doc, &b );
        ok( r == S_OK, "hasChildNoes bad return\n");
        ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

        r = IXMLDOMDocument_get_documentElement( doc, &element );
        ok( r == S_OK, "should be a document element\n");
        ok( element != NULL, "should be an element\n");
    }
    else
        ok( FALSE, "no document\n");

    VariantInit(&var);
    ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");
    r = IXMLDOMNode_get_nodeValue( doc, &var );
    ok( r == S_FALSE, "nextNode returned wrong code\n");
    ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
    ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

    if (element)
    {
        owner = NULL;
        r = IXMLDOMNode_get_ownerDocument( element, &owner );
        todo_wine {
        ok( r == S_OK, "get_ownerDocument return code\n");
        }
        ok( owner != doc, "get_ownerDocument return\n");

        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( element, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not an element\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( element, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szlc) == 0, "basename was wrong\n");

        /* check if nodename is correct */
        r = IXMLDOMElement_get_nodeName( element, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");
    
        /* content doesn't matter here */
        str = SysAllocString( szNonExistentFile );
        r = IXMLDOMElement_get_nodeName( element, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szlc ), "incorrect nodeName\n");
        SysFreeString( str );

        str = SysAllocString( szNonExistentFile );	
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == E_FAIL, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_EMPTY, "vt = %x\n", V_VT(&var));
        VariantClear(&var);
        SysFreeString( str );

        str = SysAllocString( szdl );	
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        r = IXMLDOMElement_getAttribute( element, str, &var );
        ok( r == S_OK, "getAttribute ret %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "wrong attr value\n");
        VariantClear( &var );
        SysFreeString( str );

        r = IXMLDOMElement_get_attributes( element, &map );
        ok( r == S_OK, "get_attributes returned wrong code\n");
        ok( map != NULL, "should be attributes\n");

        b = 1;
        r = IXMLDOMNode_hasChildNodes( element, &b );
        ok( r == S_OK, "hasChildNoes bad return\n");
        ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");
    }
    else
        ok( FALSE, "no element\n");

    if (map)
    {
        ISupportErrorInfo *support_error;
        r = IXMLDOMNamedNodeMap_QueryInterface( map, &IID_ISupportErrorInfo, (LPVOID*)&support_error );
        ok( r == S_OK, "ret %08x\n", r );

        r = ISupportErrorInfo_InterfaceSupportsErrorInfo( support_error, &IID_IXMLDOMNamedNodeMap );
todo_wine
{
        ok( r == S_OK, "ret %08x\n", r );
}
        ISupportErrorInfo_Release( support_error );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( node != NULL, "should be attributes\n");
        IXMLDOMNode_Release(node);
        SysFreeString( str );

        str = SysAllocString( szdl );
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, NULL );
        ok( r == E_INVALIDARG, "getNamedItem should return E_INVALIDARG\n");
        SysFreeString( str );

        /* something that isn't in szComplete4 */
        str = SysAllocString( szOpen );
        node = (IXMLDOMNode *) 1;
        r = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok( r = S_FALSE, "getNamedItem found a node that wasn't there\n");
        ok( node == NULL, "getNamedItem should have returned NULL\n");
        SysFreeString( str );

	/* test indexed access of attributes */
        r = IXMLDOMNamedNodeMap_get_length( map, &count );
        ok ( r == S_OK, "get_length wrong code\n");
        ok ( count == 1, "get_length != 1\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, -1, &node);
        ok ( r == S_FALSE, "get_item (-1) wrong code\n");
        ok ( node == NULL, "there is no node\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 1, &node);
        ok ( r == S_FALSE, "get_item (1) wrong code\n");
        ok ( node == NULL, "there is no attribute\n");

        node = NULL;
        r = IXMLDOMNamedNodeMap_get_item( map, 0, &node);
        ok ( r == S_OK, "get_item (0) wrong code\n");
        ok ( node != NULL, "should be attribute\n");

        r = IXMLDOMNode_get_nodeName( node, NULL );
        ok ( r == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

        /* content doesn't matter here */
        str = SysAllocString( szNonExistentFile );
        r = IXMLDOMNode_get_nodeName( node, &str );
        ok ( r == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, szdl ), "incorrect node name\n");
        SysFreeString( str );

        /* test sequential access of attributes */
        node = NULL;
        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (first time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r != S_OK, "nextNode (second time) wrong code\n");
        ok ( node == NULL, "nextNode, there is no attribute\n");

        r = IXMLDOMNamedNodeMap_reset( map );
        ok ( r == S_OK, "reset should return S_OK\n");

        r = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( r == S_OK, "nextNode (third time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
    }
    else
        ok( FALSE, "no map\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ATTRIBUTE, "node not an attribute\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, NULL );
        ok( r == E_INVALIDARG, "get_baseName returned wrong code\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szdl) == 0, "basename was wrong\n");

        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_OK, "returns %08x\n", r );
        ok( V_VT(&var) == VT_BSTR, "vt %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), szstr1), "nodeValue incorrect\n");
        VariantClear(&var);

        r = IXMLDOMNode_get_childNodes( node, NULL );
        ok( r == E_INVALIDARG, "get_childNodes returned wrong code\n");

        r = IXMLDOMNode_get_childNodes( node, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        if (list)
        {
            r = IXMLDOMNodeList_nextNode( list, &next );
            ok( r == S_OK, "nextNode returned wrong code\n");
        }
        else
            ok( FALSE, "no childlist\n");

        if (next)
        {
            b = 1;
            r = IXMLDOMNode_hasChildNodes( next, &b );
            ok( r == S_FALSE, "hasChildNoes bad return\n");
            ok( b == VARIANT_FALSE, "hasChildNoes wrong result\n");

            type = NODE_INVALID;
            r = IXMLDOMNode_get_nodeType( next, &type);
            ok( r == S_OK, "getNamedItem returned wrong code\n");
            ok( type == NODE_TEXT, "node not text\n");

            str = (BSTR) 1;
            r = IXMLDOMNode_get_baseName( next, &str );
            ok( r == S_FALSE, "get_baseName returned wrong code\n");
            ok( str == NULL, "basename was wrong\n");
        }
        else
            ok( FALSE, "no next\n");

        if (next)
            IXMLDOMNode_Release( next );
        next = NULL;
        if (list)
            IXMLDOMNodeList_Release( list );
        list = NULL;
        if (node)
            IXMLDOMNode_Release( node );
    }
    else
        ok( FALSE, "no node\n");
    node = NULL;

    if (map)
        IXMLDOMNamedNodeMap_Release( map );

    /* now traverse the tree from the root element */
    if (element)
    {
        IXMLDOMNode *node;
        r = IXMLDOMNode_get_childNodes( element, &list );
        ok( r == S_OK, "get_childNodes returned wrong code\n");

        /* using get_item for child list doesn't advance the position */
        ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
        expect_node(node, "E2.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_nextNode(list, &node));
        expect_node(node, "E1.E2.D1");
        IXMLDOMNode_Release(node);
        ole_check(IXMLDOMNodeList_reset(list));

        IXMLDOMNodeList_AddRef(list);
        expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1");
        ole_check(IXMLDOMNodeList_reset(list));
    }
    else
        ok( FALSE, "no element\n");

    node = (void*)0xdeadbeef;
    r = IXMLDOMNode_selectSingleNode( element, szdl, &node );
    ok( r == S_FALSE, "ret %08x\n", r );
    ok( node == NULL, "node %p\n", node );
    r = IXMLDOMNode_selectSingleNode( element, szbs, &node );
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNode_Release( node );
    ok( r == 0, "ret %08x\n", r );

    if (list)
    {
        r = IXMLDOMNodeList_get_length( list, &count );
        ok( r == S_OK, "get_length returns %08x\n", r );
        ok( count == 4, "get_length got %ld\n", count );

        r = IXMLDOMNodeList_nextNode( list, &node );
        ok( r == S_OK, "nextNode returned wrong code\n");
    }
    else
        ok( FALSE, "no list\n");

    if (node)
    {
        type = NODE_INVALID;
        r = IXMLDOMNode_get_nodeType( node, &type);
        ok( r == S_OK, "getNamedItem returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not text\n");

        VariantInit(&var);
        ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");
        r = IXMLDOMNode_get_nodeValue( node, &var );
        ok( r == S_FALSE, "nextNode returned wrong code\n");
        ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
        ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

        r = IXMLDOMNode_hasChildNodes( node, NULL );
        ok( r == E_INVALIDARG, "hasChildNoes bad return\n");

        b = 1;
        r = IXMLDOMNode_hasChildNodes( node, &b );
        ok( r == S_OK, "hasChildNoes bad return\n");
        ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

        str = NULL;
        r = IXMLDOMNode_get_baseName( node, &str );
        ok( r == S_OK, "get_baseName returned wrong code\n");
        ok( lstrcmpW(str,szbs) == 0, "basename was wrong\n");
    }
    else
        ok( FALSE, "no node\n");

    if (node)
        IXMLDOMNode_Release( node );
    if (list)
        IXMLDOMNodeList_Release( list );
    if (element)
        IXMLDOMElement_Release( element );

    b = FALSE;
    str = SysAllocString( szComplete5 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    b = 1;
    r = IXMLDOMNode_hasChildNodes( doc, &b );
    ok( r == S_OK, "hasChildNoes bad return\n");
    ok( b == VARIANT_TRUE, "hasChildNoes wrong result\n");

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    if (element)
    {
        static const WCHAR szSSearch[] = {'S',':','s','e','a','r','c','h',0};
        BSTR tag = NULL;

        /* check if the tag is correct */
        r = IXMLDOMElement_get_tagName( element, &tag );
        ok( r == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, szSSearch ), "incorrect tag name\n");
        SysFreeString( tag );
    }

    if (element)
        IXMLDOMElement_Release( element );
    if (doc)
        IXMLDOMDocument_Release( doc );
}

static void test_refs(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node = NULL, *node2;
    IXMLDOMNodeList *node_list = NULL;
    LONG ref;
    IUnknown *unk, *unk2;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;
    ref = IXMLDOMDocument_Release(doc);
    ok( ref == 0, "ref %d\n", ref);

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 2, "ref %d\n", ref );
    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 3, "ref %d\n", ref );
    IXMLDOMDocument_Release( doc );
    IXMLDOMDocument_Release( doc );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    ref = IXMLDOMDocument_AddRef( doc );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMDocument_Release( doc );

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "rets %08x\n", r);
    ref = IXMLDOMNodeList_AddRef( node_list );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMNodeList_Release( node_list );

    IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "rets %08x\n", r);

    IXMLDOMNodeList_get_item( node_list, 0, &node2 );
    ok( r == S_OK, "rets %08x\n", r);

    ref = IXMLDOMNode_AddRef( node );
    ok( ref == 2, "ref %d\n", ref );
    IXMLDOMNode_Release( node );

    ref = IXMLDOMNode_Release( node );
    ok( ref == 0, "ref %d\n", ref );
    ref = IXMLDOMNode_Release( node2 );
    ok( ref == 0, "ref %d\n", ref );

    ref = IXMLDOMNodeList_Release( node_list );
    ok( ref == 0, "ref %d\n", ref );

    ok( node != node2, "node %p node2 %p\n", node, node2 );

    ref = IXMLDOMDocument_Release( doc );
    ok( ref == 0, "ref %d\n", ref );

    ref = IXMLDOMElement_AddRef( element );
    todo_wine {
    ok( ref == 3, "ref %d\n", ref );
    }
    IXMLDOMElement_Release( element );

    /* IUnknown must be unique however we obtain it */
    r = IXMLDOMElement_QueryInterface( element, &IID_IUnknown, (LPVOID*)&unk );
    ok( r == S_OK, "rets %08x\n", r );
    r = IXMLDOMElement_QueryInterface( element, &IID_IXMLDOMNode, (LPVOID*)&node );
    ok( r == S_OK, "rets %08x\n", r );
    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (LPVOID*)&unk2 );
    ok( r == S_OK, "rets %08x\n", r );
    ok( unk == unk2, "unk %p unk2 %p\n", unk, unk2 );

    IUnknown_Release( unk2 );
    IUnknown_Release( unk );
    IXMLDOMNode_Release( node );

    IXMLDOMElement_Release( element );

}

static void test_create(void)
{
    HRESULT r;
    VARIANT var;
    BSTR str, name;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *root, *node, *child;
    IXMLDOMNamedNodeMap *attr_map;
    IUnknown *unk;
    LONG ref;
    long num;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    str = SysAllocString( szlc );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMDocument_appendChild( doc, node, &root );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == root, "%p %p\n", node, root );

    ref = IXMLDOMNode_AddRef( node );
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMNode_Release( node );

    ref = IXMLDOMNode_Release( node );
    ok(ref == 1, "ref %d\n", ref);
    SysFreeString( str );

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    str = SysAllocString( szbs );
    r = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok( r == S_OK, "returns %08x\n", r );

    ref = IXMLDOMNode_AddRef( node );
    ok(ref == 2, "ref = %d\n", ref);
    IXMLDOMNode_Release( node );

    r = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (LPVOID*)&unk );
    ok( r == S_OK, "returns %08x\n", r );

    ref = IXMLDOMNode_AddRef( unk );
    ok(ref == 3, "ref = %d\n", ref);
    IXMLDOMNode_Release( unk );

    V_VT(&var) = VT_EMPTY;
    r = IXMLDOMNode_insertBefore( root, (IXMLDOMNode*)unk, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( unk == (IUnknown*)child, "%p %p\n", unk, child );
    IXMLDOMNode_Release( child );
    IUnknown_Release( unk );


    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    r = IXMLDOMNode_insertBefore( root, node, var, &child );
    ok( r == S_OK, "returns %08x\n", r );
    ok( node == child, "%p %p\n", node, child );
    IXMLDOMNode_Release( child );
    IXMLDOMNode_Release( node );


    r = IXMLDOMNode_QueryInterface( root, &IID_IXMLDOMElement, (LPVOID*)&element );
    ok( r == S_OK, "returns %08x\n", r );

    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 0, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szdl );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr2 );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 1, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( !lstrcmpW(V_BSTR(&var), szstr2), "wrong attr value\n");
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( szstr1 );
    name = SysAllocString( szlc );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMElement_get_attributes( element, &attr_map );
    ok( r == S_OK, "returns %08x\n", r );
    r = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok( r == S_OK, "returns %08x\n", r );
    ok( num == 2, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    name = SysAllocString( szbs );
    r = IXMLDOMElement_setAttribute( element, name, var );
    ok( r == S_OK, "returns %08x\n", r );
    VariantClear(&var);
    r = IXMLDOMElement_getAttribute( element, name, &var );
    ok( r == S_OK, "returns %08x\n", r );
    ok( V_VT(&var) == VT_BSTR, "variant type %x\n", V_VT(&var));
    VariantClear(&var);
    SysFreeString(name);

    IXMLDOMElement_Release( element );
    IXMLDOMNode_Release( root );
    IXMLDOMDocument_Release( doc );
}

static void test_getElementsByTagName(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNodeList *node_list;
    long len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    str = SysAllocString( szstar );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 6, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 1, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szdl );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    str = SysAllocString( szstr1 );
    r = IXMLDOMDocument_getElementsByTagName(doc, str, &node_list);
    ok( r == S_OK, "ret %08x\n", r );
    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r );
    ok( len == 0, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );
    SysFreeString( str );

    IXMLDOMDocument_Release( doc );
}

static void test_get_text(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2, *node3;
    IXMLDOMNodeList *node_list;
    IXMLDOMNamedNodeMap *node_map;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    str = SysAllocString( szbs );
    r = IXMLDOMDocument_getElementsByTagName( doc, str, &node_list );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);
    
    r = IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok( r == S_OK, "ret %08x\n", r ); 
    IXMLDOMNodeList_Release( node_list );

    r = IXMLDOMNode_get_text( node, &str );
    ok( r == S_OK, "ret %08x\n", r );
todo_wine {
    ok( !memcmp(str, szfn1_txt, sizeof(szfn1_txt)), "wrong string\n" );
 }
    ok( !memcmp(str, szfn1_txt, sizeof(szfn1_txt)-4), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_attributes( node, &node_map );
    ok( r == S_OK, "ret %08x\n", r );
    
    str = SysAllocString( szvr );
    r = IXMLDOMNamedNodeMap_getNamedItem( node_map, str, &node2 );
    ok( r == S_OK, "ret %08x\n", r );
    SysFreeString(str);

    r = IXMLDOMNode_get_text( node2, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);

    r = IXMLDOMNode_get_firstChild( node2, &node3 );
    ok( r == S_OK, "ret %08x\n", r );

    r = IXMLDOMNode_get_text( node3, &str );
    ok( r == S_OK, "ret %08x\n", r );
    ok( !memcmp(str, szstr2, sizeof(szstr2)), "wrong string\n" );
    SysFreeString(str);


    IXMLDOMNode_Release( node3 );
    IXMLDOMNode_Release( node2 );
    IXMLDOMNamedNodeMap_Release( node_map );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );
}

static void test_get_childNodes(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *node, *node2;
    IXMLDOMNodeList *node_list, *node_list2;
    long len;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_length( node_list, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 4, "len %ld\n", len);

    r = IXMLDOMNodeList_get_item( node_list, 2, &node );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNode_get_childNodes( node, &node_list2 );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_length( node_list2, &len );
    ok( r == S_OK, "ret %08x\n", r);
    ok( len == 0, "len %ld\n", len);

    r = IXMLDOMNodeList_get_item( node_list2, 0, &node2);
    ok( r == S_FALSE, "ret %08x\n", r);

    IXMLDOMNodeList_Release( node_list2 );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( node_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_removeChild(void)
{
    HRESULT r;
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *node, *node2, *node3, *node4;
    IXMLDOMNodeList *node_list, *node_list2;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_get_documentElement( doc, &element );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_get_childNodes( element, &node_list );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMNodeList_get_item( node_list, 3, &node );
    ok( r == S_OK, "ret %08x\n", r);
 
    r = IXMLDOMNode_get_childNodes( node, &node_list2 );
    ok( r == S_OK, "ret %08x\n", r);
 
    r = IXMLDOMNodeList_get_item( node_list, 0, &node4 );
    ok( r == S_OK, "ret %08x\n", r);

    r = IXMLDOMElement_removeChild( element, NULL, &node2 );
    ok( r == E_INVALIDARG, "ret %08x\n", r );

    r = IXMLDOMElement_removeChild( element, node4, &node2 );
    ok( r == S_OK, "ret %08x\n", r);
    ok( node4 == node2, "node %p node2 %p\n", node4, node2 );

    r = IXMLDOMNode_get_parentNode( node4, &node3 );
    ok( r == S_FALSE, "ret %08x\n", r);
    ok( node3 == NULL, "%p\n", node3 );

    IXMLDOMNode_Release( node2 );
    IXMLDOMNode_Release( node4 );
    IXMLDOMNodeList_Release( node_list2 );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( node_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
}

static void test_XMLHTTP(void)
{
    static const WCHAR wszBody[] = {'m','o','d','e','=','T','e','s','t',0};
    static WCHAR wszPOST[] = {'P','O','S','T',0};
    static WCHAR wszUrl[] = {'h','t','t','p',':','/','/',
        'c','r','o','s','s','o','v','e','r','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
        'p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR wszExpectedResponse[] = {'F','A','I','L','E','D',0};
    IXMLHttpRequest *pXMLHttpRequest;
    BSTR bstrResponse;
    VARIANT dummy;
    VARIANT varfalse;
    VARIANT varbody;
    HRESULT hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL,
                                  CLSCTX_INPROC_SERVER, &IID_IXMLHttpRequest,
                                  (void **)&pXMLHttpRequest);
    todo_wine {
    ok(hr == S_OK, "CoCreateInstance(CLSID_XMLHTTPRequest) should have succeeded instead of failing with 0x%08x\n", hr);
    }
    if (hr != S_OK)
        return;

    VariantInit(&dummy);
    V_VT(&dummy) = VT_ERROR;
    V_ERROR(&dummy) = DISP_E_MEMBERNOTFOUND;
    VariantInit(&varfalse);
    V_VT(&varfalse) = VT_BOOL;
    V_BOOL(&varfalse) = VARIANT_FALSE;
    V_VT(&varbody) = VT_BSTR;
    V_BSTR(&varbody) = SysAllocString(wszBody);

    hr = IXMLHttpRequest_open(pXMLHttpRequest, wszPOST, wszUrl, varfalse, dummy, dummy);
    ok(hr == S_OK, "IXMLHttpRequest_open should have succeeded instead of failing with 0x%08x\n", hr);

    hr = IXMLHttpRequest_send(pXMLHttpRequest, varbody);
    ok(hr == S_OK, "IXMLHttpRequest_send should have succeeded instead of failing with 0x%08x\n", hr);
    VariantClear(&varbody);

    hr = IXMLHttpRequest_get_responseText(pXMLHttpRequest, &bstrResponse);
    ok(hr == S_OK, "IXMLHttpRequest_get_responseText should have succeeded instead of failing with 0x%08x\n", hr);
    /* the server currently returns "FAILED" because the Content-Type header is
     * not what the server expects */
    ok(!memcmp(bstrResponse, wszExpectedResponse, sizeof(wszExpectedResponse)), "bstrResponse differs from what was expected\n");
    SysFreeString(bstrResponse);
}

static void test_IXMLDOMDocument2(void)
{
    HRESULT r;
    VARIANT_BOOL b;
    BSTR str;
    IXMLDOMDocument *doc;
    IXMLDOMDocument2 *doc2;
    VARIANT var;
    int ref;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    str = SysAllocString( szComplete4 );
    r = IXMLDOMDocument_loadXML( doc, str, &b );
    ok( r == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    r = IXMLDOMDocument_QueryInterface( doc, &IID_IXMLDOMDocument2, (void**)&doc2 );
    ok( r == S_OK, "ret %08x\n", r );
    ok( doc == (IXMLDOMDocument*)doc2, "interfaces differ\n");

    /* we will check if the variant got cleared */
    ref = IXMLDOMDocument2_AddRef(doc2);
    expect_eq(ref, 3, int, "%d");  /* doc, doc2, AddRef*/
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)doc2;

    /* invalid calls */
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("askldhfaklsdf"), &var), E_FAIL);
    expect_eq(V_VT(&var), VT_UNKNOWN, int, "%x");
    ole_expect(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), NULL), E_INVALIDARG);

    /* valid call */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");
    V_VT(&var) = VT_R4;

    /* the variant didn't get cleared*/
    expect_eq(IXMLDOMDocument2_Release(doc2), 2, int, "%d");

    /* setProperty tests */
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("askldhfaklsdf"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), var), E_FAIL);
    ole_expect(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("alskjdh faklsjd hfk")), E_FAIL);
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));
    ole_check(IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern")));

    /* contrary to what MSDN calims you can switch back from XPath to XSLPattern */
    ole_check(IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var));
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");

    IXMLDOMDocument2_Release( doc2 );
    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

static void test_XPath()
{
    HRESULT r;
    VARIANT var;
    VARIANT_BOOL b;
    IXMLDOMDocument2 *doc;
    IXMLDOMNode *rootNode;
    IXMLDOMNode *elem1Node;
    IXMLDOMNode *node;
    IXMLDOMNodeList *list;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    ole_check(IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath")));

    /* some simple queries*/
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root"), &list));
    ole_check(IXMLDOMNodeList_get_item(list, 0, &rootNode));
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E2.D1");
    if (rootNode == NULL)
        return;

    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//c"), &list));
    expect_list_and_release(list, "E3.E1.E2.D1 E3.E2.E2.D1");

    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("//c[@type]"), &list));
    expect_list_and_release(list, "E3.E2.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem"), &list));
    /* using get_item for query results advances the position */
    ole_check(IXMLDOMNodeList_get_item(list, 1, &node));
    expect_node(node, "E2.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_nextNode(list, &node));
    expect_node(node, "E4.E2.D1");
    IXMLDOMNode_Release(node);
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E4.E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("."), &list));
    expect_list_and_release(list, "E2.D1");

    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("elem[3]/preceding-sibling::*"), &list));
    ole_check(IXMLDOMNodeList_get_item(list, 0, &elem1Node));
    ole_check(IXMLDOMNodeList_reset(list));
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1");

    /* select an attribute */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//@type"), &list));
    expect_list_and_release(list, "A'type'.E3.E2.E2.D1");

    /* would evaluate to a number */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("count(*)"), &list), E_FAIL);
    /* would evaluate to a boolean */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("position()>0"), &list), E_FAIL);
    /* would evaluate to a string */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_("name()"), &list), E_FAIL);

    /* no results */
    ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_("c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("elem//c"), &list));
    expect_list_and_release(list, "");
    ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("//elem[4]"), &list));
    expect_list_and_release(list, "");

    /* foo undeclared in document node */
    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);
    /* undeclared in <root> node */
    ole_expect(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//foo:c"), &list), E_FAIL);
    /* undeclared in <elem> node */
    ole_expect(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//foo:c"), &list), E_FAIL);
    /* but this trick can be used */
    ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//*[name()='foo:c']"), &list));
    expect_list_and_release(list, "E3.E4.E2.D1");

    /* it has to be declared in SelectionNamespaces */
    todo_wine ole_check(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'")));

    /* now the namespace can be used */
    todo_wine ole_check(IXMLDOMDocument_selectNodes(doc, _bstr_("root//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(rootNode, _bstr_(".//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_("//test:c"), &list));
    todo_wine expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    todo_wine ole_check(IXMLDOMNode_selectNodes(elem1Node, _bstr_(".//test:x"), &list));
    todo_wine expect_list_and_release(list, "E5.E1.E4.E1.E2.D1");

    /* SelectionNamespaces syntax error - the namespaces doesn't work anymore but the value is stored */
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###")), E_FAIL);

    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    todo_wine ole_check(IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var));
    todo_wine expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    if (V_VT(&var) == VT_BSTR)
        expect_bstr_eq_and_free(V_BSTR(&var), "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###");

    /* extra attributes - same thing*/
    ole_expect(IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' param='test'")), E_FAIL);
    ole_expect(IXMLDOMDocument_selectNodes(doc, _bstr_("root//foo:c"), &list), E_FAIL);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNode_Release(elem1Node);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

START_TEST(domdoc)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    test_domdoc();
    test_domnode();
    test_refs();
    test_create();
    test_getElementsByTagName();
    test_get_text();
    test_get_childNodes();
    test_removeChild();
    test_XMLHTTP();
    test_IXMLDOMDocument2();
    test_XPath();

    CoUninitialize();
}
