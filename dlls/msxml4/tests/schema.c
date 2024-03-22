/*
 * Schema test
 *
 * Copyright 2007 Huw Davies
 * Copyright 2010 Adam Martinson for CodeWeavers
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
#include <assert.h>
#define COBJMACROS

#include "ole2.h"
#include "msxml2.h"

#include "wine/test.h"

static IXMLDOMDocument2 *create_document(void)
{
    IXMLDOMDocument2 *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    ok(hr == S_OK, "Failed to create a document object, hr %#lx.\n", hr);

    return obj;
}

static IXMLDOMSchemaCollection *create_cache(void)
{
    IXMLDOMSchemaCollection *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLSchemaCache40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMSchemaCollection, (void **)&obj);
    ok(hr == S_OK, "Failed to create a document object, hr %#lx.\n", hr);

    return obj;
}

static HRESULT validate_regex_document(IXMLDOMDocument2 *doc, IXMLDOMDocument2 *schema, IXMLDOMSchemaCollection* cache,
    const WCHAR *regex, const WCHAR *input)
{
    static const WCHAR regex_doc[] =
L""
"<?xml version='1.0'?>"
"<root xmlns='urn:test'>%s</root>";

    static const WCHAR regex_schema[] =
L"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='urn:test'>"
"    <element name='root'>"
"        <simpleType>"
"            <restriction base='string'>"
"                <pattern value='%s'/>"
"            </restriction>"
"        </simpleType>"
"    </element>"
"</schema>";

    WCHAR buffer[1024];
    IXMLDOMParseError* err;
    BSTR namespace, bstr;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;

    VariantInit(&v);

    swprintf(buffer, ARRAY_SIZE(buffer), regex_doc, input);
    bstr = SysAllocString(buffer);
    hr = IXMLDOMDocument2_loadXML(doc, bstr, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    SysFreeString(bstr);

    swprintf(buffer, ARRAY_SIZE(buffer), regex_schema, regex);
    bstr = SysAllocString(buffer);
    hr = IXMLDOMDocument2_loadXML(schema, bstr, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    SysFreeString(bstr);

    /* add the schema to the cache */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMDocument2_QueryInterface(schema, &IID_IDispatch, (void**)&V_DISPATCH(&v));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    namespace = SysAllocString(L"urn:test");
    hr = IXMLDOMSchemaCollection_add(cache, namespace, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(namespace);
    VariantClear(&v);
/*
    if (FAILED(hr))
        return hr;
*/
    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    hr = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&v);

    /* validate the doc
     * only declared elements in the declared order
     * this is fine */
    err = NULL;
    bstr = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(err != NULL, "domdoc_validate() should always set err\n");
    if (IXMLDOMParseError_get_reason(err, &bstr) != S_FALSE)
        trace("got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    return hr;
}

static void test_regex(void)
{
    static const struct regex_test
    {
        const WCHAR *regex;
        const WCHAR *input;
    }
    tests[] =
    {
        { L"\\!", L"!" },
        { L"\\\"", L"\"" },
        { L"\\#", L"#" },
        { L"\\$", L"$" },
        { L"\\%", L"%" },
        { L"\\,", L"," },
        { L"\\/", L"/" },
        { L"\\:", L":" },
        { L"\\;", L";" },
        { L"\\=", L"=" },
        { L"\\>", L">" },
        { L"\\@", L"@" },
        { L"\\`", L"`" },
        { L"\\~", L"~" },
        { L"\\uCAFE", L"\xCAFE" },
        /* non-BMP character in surrogate pairs: */
        { L"\\uD83D\\uDE00", L"\xD83D\xDE00" },
        /* "x{,2}" is non-standard and only works on libxml2 <= v2.9.10 */
        { L"x{0,2}", L"x" }
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const struct regex_test *test = &tests[i];
        IXMLDOMDocument2 *doc, *schema;
        IXMLDOMSchemaCollection *cache;
        HRESULT hr;

        winetest_push_context("Test %s", wine_dbgstr_w(test->regex));

        doc = create_document();
        schema = create_document();
        cache = create_cache();

        hr = validate_regex_document(doc, schema, cache, tests->regex, tests->input);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (doc)
            IXMLDOMDocument2_Release(doc);
        if (schema)
            IXMLDOMDocument2_Release(schema);
        if (cache)
            IXMLDOMSchemaCollection_Release(cache);

        winetest_pop_context();
    }
}

START_TEST(schema)
{
    IUnknown *obj;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    if (FAILED(hr))
    {
        win_skip("DOMDocument40 is not supported.\n");
        CoUninitialize();
        return;
    }
    IUnknown_Release(obj);

    test_regex();

    CoUninitialize();
}
