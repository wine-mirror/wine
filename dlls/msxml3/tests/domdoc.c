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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#define COBJMACROS

#include "windows.h"
#include "ole2.h"
#include "msxml.h"
#include "xmldom.h"
#include <stdio.h>

#include "wine/test.h"

void test_domdoc( void )
{
    HRESULT r;
    IXMLDOMDocument *doc = NULL;
    IXMLDOMElement *element = NULL;
    static const GUID CLSID_DOMDocument = 
        { 0x2933BF90, 0x7B36, 0x11d2, {0xB2,0x0E,0x00,0xC0,0x4F,0x98,0x3E,0x60}};
    static const GUID IID_IXMLDOMDocument = 
        { 0x2933BF81, 0x7B36, 0x11d2, {0xB2,0x0E,0x00,0xC0,0x4F,0x98,0x3E,0x60}};
    VARIANT_BOOL b;
    BSTR str;
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
        '<','?','x','m','l',' ',
        'v','e','r','s','i','o','n','=','\'','1','.','0','\'','?','>',
        '<','l','c',' ','d','l','=','\'','s','t','r','1','\'','>',
        '<','b','s',' ','v','r','=','\'','s','t','r','2','\'',' ',
        's','z','=','\'','1','2','3','4','\'','>','f','n','1','.','t','x','t','<','/','b','s','>',
        '<','p','r',' ','i','d','=','\'','s','t','r','3','\'',' ',
        'v','r','=','\'','1','.','2','.','3','\'',' ',
        'p','n','=','\'','w','i','n','e',' ','2','0','0','5','0','8','0','4','\'','>',
        'f','n','2','.','t','x','t','<','/','p','r','>',
        '<','/','l','c','>','\n',0
    };
    static const WCHAR szOpen[] = { 'o','p','e','n',0 };

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    r = CoCreateInstance( &CLSID_DOMDocument, NULL, 
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (LPVOID*)&doc );
    /* ok( r == S_OK, "failed to create an xml dom doc\n" ); */
    if( r != S_OK )
        return;

    /* try some stupid things */
    r = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok( r == S_FALSE, "loadXML failed\n");

    b = VARIANT_TRUE;
    r = IXMLDOMDocument_loadXML( doc, NULL, &b );
    ok( r == S_FALSE, "loadXML failed\n");
    ok( b == VARIANT_FALSE, "failed to load XML string\n");

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

    /* check that there's no document element */
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

    r = IXMLDocument_Release( doc );
    ok( r == 0, "document ref count incorrect\n");

    CoUninitialize();
    ok( r == S_OK, "failed to uninit com\n");
}

START_TEST(domdoc)
{
    test_domdoc();
}
