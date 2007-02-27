/*
 * Schema test
 *
 * Copyright 2007 Huw Davies
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

#include "wine/test.h"

const CLSID CLSID_XMLSchemaCache      = {0x373984c9, 0xb845, 0x449b, {0x91, 0xe7, 0x45, 0xac, 0x83, 0x03, 0x6a, 0xde}};
const IID IID_IXMLDOMSchemaCollection = {0x373984c8, 0xb845, 0x449b, {0x91, 0xe7, 0x45, 0xac, 0x83, 0x03, 0x6a, 0xde}};

static const WCHAR schema_uri[] = {'x','-','s','c','h','e','m','a',':','t','e','s','t','.','x','m','l',0};

static const WCHAR schema_xml[] = {
    '<','S','c','h','e','m','a',' ','x','m','l','n','s','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','x','m','l','-','d','a','t','a','\"','\n',
    'x','m','l','n','s',':','d','t','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','d','a','t','a','t','y','p','e','s','\"','>','\n',
    '<','/','S','c','h','e','m','a','>','\n',0
};

void test_schema_refs(void)
{
    IXMLDOMDocument2 *doc;
    IXMLDOMSchemaCollection *schema;
    HRESULT r;
    LONG ref;
    VARIANT v;
    VARIANT_BOOL b;
    BSTR str;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    r = CoCreateInstance( &CLSID_XMLSchemaCache, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMSchemaCollection, (LPVOID*)&schema );
    if( r != S_OK )
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }

    str = SysAllocString(schema_xml);
    r = IXMLDOMDocument2_loadXML(doc, str, &b);
    ok(r == S_OK, "ret %08x\n", r);
    ok(b == VARIANT_TRUE, "b %04x\n", b);
    SysFreeString(str);

    ref = IXMLDOMDocument2_AddRef(doc);
    ok(ref == 2, "ref %d\n", ref);
    VariantInit(&v);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)doc;

    str = SysAllocString(schema_uri);
    r = IXMLDOMSchemaCollection_add(schema, str, v);
    ok(r == S_OK, "ret %08x\n", r);

    /* IXMLDOMSchemaCollection_add doesn't add a ref on doc */
    ref = IXMLDOMDocument2_AddRef(doc);
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMDocument2_Release(doc);

    SysFreeString(str);
    VariantClear(&v);

    IXMLDOMSchemaCollection_Release(schema);
    IXMLDOMDocument2_Release(doc);
}

START_TEST(schema)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    test_schema_refs();

    CoUninitialize();
}
