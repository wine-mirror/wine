/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2007-2008 Alistair Leslie-Hughes
 * Copyright 2010-2011 Adam Martinson for CodeWeavers
 * Copyright 2010-2013 Nikolay Sivov for CodeWeavers
 * Copyright 2023 Daniel Lehman
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
#define CONST_VTABLE

#include <stdio.h>
#include <assert.h>

#include "windows.h"

#include "initguid.h"
#include "objsafe.h"
#include "docobj.h"
#include "dispex.h"

#include "msxml6.h"

#include "wine/test.h"

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static const WCHAR email_xml[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 87)</subject>"
"   <body>"
"       It no longer causes spontaneous combustion..."
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_0D[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 88)</subject>"
"   <body>"
"       <undecl />"
"       XML_ELEMENT_UNDECLARED 0xC00CE00D"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_0E[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 89)</subject>"
"   <body>"
"       XML_ELEMENT_ID_NOT_FOUND 0xC00CE00E"
"   </body>"
"   <attachment id=\"patch\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_11[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <recipients>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 90)</subject>"
"   <body>"
"       XML_EMPTY_NOT_ALLOWED 0xC00CE011"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_13[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<msg attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 91)</subject>"
"   <body>"
"       XML_ROOT_NAME_MISMATCH 0xC00CE013"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</msg>";

static const WCHAR email_xml_14[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <to>wine-patches@winehq.org</to>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 92)</subject>"
"   <body>"
"       XML_INVALID_CONTENT 0xC00CE014"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_15[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\" ip=\"127.0.0.1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 93)</subject>"
"   <body>"
"       XML_ATTRIBUTE_NOT_DEFINED 0xC00CE015"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_16[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 94)</subject>"
"   <body enc=\"ASCII\">"
"       XML_ATTRIBUTE_FIXED 0xC00CE016"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_17[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\" sent=\"true\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 95)</subject>"
"   <body>"
"       XML_ATTRIBUTE_VALUE 0xC00CE017"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_18[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email attachments=\"patch1\">"
"   oops"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 96)</subject>"
"   <body>"
"       XML_ILLEGAL_TEXT 0xC00CE018"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const WCHAR email_xml_20[] =
L"<?xml version=\"1.0\"?>"
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"
"<email>"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 97)</subject>"
"   <body>"
"       XML_REQUIRED_ATTRIBUTE_MISSING 0xC00CE020"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const WCHAR *str)
{
    assert(alloced_bstrs_count < ARRAY_SIZE(alloced_bstrs));
    alloced_bstrs[alloced_bstrs_count] = SysAllocString(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

struct attrtest_t {
    const WCHAR *name;
    const WCHAR *uri;
    const WCHAR *prefix;
    const WCHAR *href;
};

static struct attrtest_t attrtests[] = {
    { L"xmlns", L"http://www.w3.org/2000/xmlns/", NULL, L"http://www.w3.org/2000/xmlns/" },
    { L"xmlns", L"nondefaulturi", NULL, L"http://www.w3.org/2000/xmlns/" },
    { L"c", L"http://www.w3.org/2000/xmlns/", NULL, L"http://www.w3.org/2000/xmlns/" },
    { L"c", L"nsref1", NULL, L"nsref1" },
    { L"ns:c", L"nsref1", L"ns", L"nsref1" },
    { L"xmlns:c", L"http://www.w3.org/2000/xmlns/", L"xmlns", L"http://www.w3.org/2000/xmlns/" },
    { L"xmlns:c", L"nondefaulturi", L"xmlns", L"http://www.w3.org/2000/xmlns/" },
    { 0 }
};

/* see dlls/msxml[34]/tests/domdoc.c */
static void test_create_attribute(void)
{
    struct attrtest_t *ptr = attrtests;
    IXMLDOMElement *el;
    IXMLDOMDocument2 *doc;
    IXMLDOMNode *node;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    BSTR str;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Failed to create DOMDocument60, hr %#lx.\n", hr);

    while (ptr->name)
    {
        V_VT(&var) = VT_I1;
        V_I1(&var) = NODE_ATTRIBUTE;
        hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(ptr->name), _bstr_(ptr->uri), &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        str = NULL;
        hr = IXMLDOMNode_get_prefix(node, &str);
        if (ptr->prefix)
        {
            ok(hr == S_OK, "%d: unexpected hr %#lx\n", i, hr);
            ok(!lstrcmpW(str, _bstr_(ptr->prefix)), "%d: got prefix %s, expected %s\n",
                i, wine_dbgstr_w(str), wine_dbgstr_w(ptr->prefix));
        }
        else
        {
            ok(hr == S_FALSE, "%d: unexpected hr %#lx\n", i, hr);
            ok(str == NULL, "%d: got prefix %s\n", i, wine_dbgstr_w(str));
        }
        SysFreeString(str);

        str = NULL;
        hr = IXMLDOMNode_get_namespaceURI(node, &str);
        ok(hr == S_OK, "%d: unexpected hr %#lx\n", i, hr);
        ok(!lstrcmpW(str, _bstr_(ptr->href)) ||
            broken(!ptr->prefix && !lstrcmpW(str, L"xmlns")), /* win7 msxml6 */
            "%d: got uri %s, expected %s\n", i, wine_dbgstr_w(str), wine_dbgstr_w(ptr->href));
        SysFreeString(str);

        IXMLDOMNode_Release(node);
        free_bstrs();

        i++;
        ptr++;
    }

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(L"e"), NULL, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&el);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(L"xmlns:a"),
        _bstr_(L"http://www.w3.org/2000/xmlns/"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_setAttributeNode(el, (IXMLDOMAttribute*)node, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* for some reason default namespace uri is not reported */
    hr = IXMLDOMNode_get_namespaceURI(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"http://www.w3.org/2000/xmlns/") ||
        broken(!ptr->prefix && !lstrcmpW(str, L"xmlns")), /* win7 msxml6 */
        "got uri %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMNode_Release(node);
    IXMLDOMElement_Release(el);
    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

/* see dlls/msxml[34]/tests/domdoc.c */
static void test_namespaces_as_attributes(void)
{
    struct test
    {
        const WCHAR *xml;
        int explen;
        const WCHAR *names[3];
        const WCHAR *prefixes[3];
        const WCHAR *basenames[3];
        const WCHAR *uris[3];
        const WCHAR *texts[3];
        const WCHAR *xmls[3];
    };
    static const struct test tests[] =
    {
        {
            L"<a ns:b=\"b attr\" d=\"d attr\" xmlns:ns=\"nshref\" />", 3,
            { L"ns:b",   L"d",      L"xmlns:ns" }, /* nodeName */
            { L"ns",     NULL,      L"xmlns" },    /* prefix */
            { L"b",      L"d",      L"ns" },       /* baseName */
            { L"nshref", NULL,      L"" },         /* namespaceURI */
            { L"b attr", L"d attr", L"nshref" },   /* text */
            { L"ns:b=\"b attr\"", L"d=\"d attr\"", L"xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* property only */
        {
            L"<a d=\"d attr\" />", 1,
            { L"d" },                   /* nodeName */
            { NULL },                   /* prefix */
            { L"d" },                   /* baseName */
            { NULL },                   /* namespaceURI */
            { L"d attr" },              /* text */
            { L"d=\"d attr\"" },        /* xml */
        },
        /* namespace only */
        {
            L"<a xmlns:ns=\"nshref\" />", 1,
            { L"xmlns:ns" },            /* nodeName */
            { L"xmlns" },               /* prefix */
            { L"ns" },                  /* baseName */
            { L"" },                    /* namespaceURI */
            { L"nshref" },              /* text */
            { L"xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* default namespace */
        {
            L"<a xmlns=\"nshref\" />", 1,
            { L"xmlns" },                           /* nodeName */
            { NULL },                               /* prefix */
            { L"xmlns" },                           /* baseName */
            { L"http://www.w3.org/2000/xmlns/" },   /* namespaceURI */
            { L"nshref" },                          /* text */
            { L"xmlns=\"nshref\"" },                /* xml */
        },
        /* no properties or namespaces */
        {
            L"<a />", 0,
        },

        { NULL }
    };
    const struct test *test;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMNode *node, *item;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    LONG len, i;
    HRESULT hr;
    BSTR str;

    test = tests;
    while (test->xml)
    {
        hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_loadXML(doc, _bstr_(test->xml), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        node = NULL;
        hr = IXMLDOMDocument2_get_firstChild(doc, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        len = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == test->explen, "got %ld\n", len);

        item = NULL;
        hr = IXMLDOMNamedNodeMap_get_item(map, test->explen+1, &item);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(!item, "Item should be NULL\n");

        for (i = 0; i < len; i++)
        {
            item = NULL;
            hr = IXMLDOMNamedNodeMap_get_item(map, i, &item);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            str = NULL;
            hr = IXMLDOMNode_get_nodeName(item, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->names[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_prefix(item, &str);
            if (test->prefixes[i])
            {
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, test->prefixes[i]), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);
            }
            else
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

            str = NULL;
            hr = IXMLDOMNode_get_baseName(item, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->basenames[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_namespaceURI(item, &str);
            if (test->uris[i])
            {
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                if (test->prefixes[i] && !lstrcmpW(test->prefixes[i], L"xmlns"))
                    ok(!lstrcmpW(str, L"http://www.w3.org/2000/xmlns/"),
                                 "got %s\n", wine_dbgstr_w(str));
                else
                    ok(!lstrcmpW(str, test->uris[i]), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);
            }
            else
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

            str = NULL;
            hr = IXMLDOMNode_get_text(item, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->texts[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_xml(item, &str);
            ok(SUCCEEDED(hr), "Failed to get node xml, hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->xmls[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            IXMLDOMNode_Release(item);
        }

        IXMLDOMNamedNodeMap_Release(map);
        IXMLDOMNode_Release(node);
        IXMLDOMDocument2_Release(doc);

        test++;
    }
    free_bstrs();
}

static const WCHAR *leading_spaces_xmldata[] =
{
    L"\n<?xml version=\"1.0\" encoding=\"UTF-16\" ?><root/>",
    L" <?xml version=\"1.0\"?><root/>",
    L"\n<?xml version=\"1.0\"?><root/>",
    L"\t<?xml version=\"1.0\"?><root/>",
    L"\r\n<?xml version=\"1.0\"?><root/>",
    L"\r<?xml version=\"1.0\"?><root/>",
    L"\r\r\r\r\t\t \n\n <?xml version=\"1.0\"?><root/>",
    0
};

static void test_leading_spaces(void)
{
    WCHAR path[MAX_PATH];
    IXMLDOMDocument *doc;
    const WCHAR **data;
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"leading_spaces.xml");

    data = leading_spaces_xmldata;
    while (*data)
    {
        IStream *stream;
        DWORD written;
        HANDLE file;

        /* IStream */
        hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IStream_Write(stream, *data, lstrlenW(*data) * sizeof(WCHAR), NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)stream;
        b = 0xc;
        hr = IXMLDOMDocument_load(doc, var, &b);
        todo_wine
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "Unexpected %d.\n", b);

        IStream_Release(stream);

        /* Disk file */
        file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        ok(file != INVALID_HANDLE_VALUE, "Failed to create a file %s: %lu.\n", debugstr_w(path), GetLastError());

        WriteFile(file, *data, lstrlenW(*data) * sizeof(WCHAR), &written, NULL);
        CloseHandle(file);

        b = 0xc;
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_(path);
        hr = IXMLDOMDocument_load(doc, var, &b);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "Unexpected %d.\n", b);

        DeleteFileW(path);

        /* WCHAR buffer */
        b = 0xc;
        hr = IXMLDOMDocument_loadXML(doc, _bstr_(*data), &b);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "Unexpected %d.\n", b);

        data++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_get_ownerDocument(void)
{
    IXMLDOMDocument *doc1;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);

    hr = IXMLDOMDocument2_get_ownerDocument(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    doc1 = (void *)0xdead;
    hr = IXMLDOMDocument2_get_ownerDocument(doc, &doc1);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!doc1, "Unexpected pointer.\n");

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(L"<a>text</a>"), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "Unexpected result %d.\n", b);

    hr = IXMLDOMDocument2_get_ownerDocument(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    doc1 = (void *)0xdead;
    hr = IXMLDOMDocument2_get_ownerDocument(doc, &doc1);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!doc1, "Unexpected pointer.\n");

    IXMLDOMDocument2_Release(doc);
}

static void test_get_parentNode(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *node;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_parentNode(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    node = (void *)0x1;
    hr = IXMLDOMDocument_get_parentNode(doc, &node);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!node, "Unexpected node %p.\n", node);

    IXMLDOMDocument_Release(doc);
}

static void test_normalize_attribute_values(void)
{
    IXMLDOMElement *element;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = 0x1;
    hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "Unexpected value %d.\n", b);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(L"<a attr=\"v\r\na\tl\r\" />"), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_get_documentElement(doc, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 10;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"NormalizeAttributeValues"), &var);
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BOOL, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_FALSE, "Unexpected property value.\n");

    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_TRUE;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"NormalizeAttributeValues"), var);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 10;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"NormalizeAttributeValues"), &var);
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BOOL, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_TRUE, "Unexpected property value.\n");

    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v a l "), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"v\r\na\tl\n\t");
    hr = IXMLDOMElement_setAttribute(element, _bstr_(L"attr"), var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);

    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!wcscmp(V_BSTR(&var), L"v\r\na\tl\n\t"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!wcscmp(V_BSTR(&var), L"v\r\na\tl\n\t"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_FALSE;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"NormalizeAttributeValues"), var);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n\t"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_getAttribute(element, _bstr_(L"attr"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(V_BSTR(&var), L"v\na\tl\n\t"), "Unexpected value %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMElement_Release(element);
    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_prohibitdtd(void)
{
    IXMLDOMDocument2 *doc, *doc2;
    IXMLDOMNode *node;
    HRESULT hr;
    VARIANT v;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    VariantInit(&v);
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"ProhibitDTD"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected type %d.\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "Unexpected value %d.\n", V_BOOL(&v));

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml), NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(L"<a/>"), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_I2;
    V_I2(&v) = 0;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"ProhibitDTD"), v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    VariantInit(&v);
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"ProhibitDTD"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected type %d.\n", V_VT(&v));
    ok(!V_BOOL(&v), "Unexpected value %d.\n", V_BOOL(&v));

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_cloneNode(doc, VARIANT_FALSE, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMDocument2, (void **)&doc2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    VariantInit(&v);
    hr = IXMLDOMDocument2_getProperty(doc2, _bstr_(L"ProhibitDTD"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected type %d.\n", V_VT(&v));
    ok(!V_BOOL(&v), "Unexpected value %d.\n", V_BOOL(&v));
    IXMLDOMDocument2_Release(doc2);

    IXMLDOMDocument2_Release(doc);
}

static void test_interfaces(void)
{
    IXMLDOMDocument *doc;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(doc, &IID_IXMLDOMDocument, TRUE);
    check_interface(doc, &IID_IPersistStreamInit, TRUE);
    check_interface(doc, &IID_IObjectWithSite, TRUE);
    check_interface(doc, &IID_IObjectSafety, TRUE);
    check_interface(doc, &IID_IConnectionPointContainer, TRUE);
    check_interface(doc, &IID_IDispatch, TRUE);
    check_interface(doc, &IID_IDispatchEx, TRUE);
    check_interface(doc, &IID_IUnknown, TRUE);
    check_interface(doc, &IID_IPersistStream, TRUE);
    check_interface(doc, &IID_ISequentialStream, FALSE);
    check_interface(doc, &IID_IPersist, FALSE);
todo_wine
{
    check_interface(doc, &IID_IOleCommandTarget, TRUE);
    check_interface(doc, &IID_IPersistMoniker, TRUE);
    check_interface(doc, &IID_IProvideClassInfo, TRUE);
    check_interface(doc, &IID_IStream, TRUE);
}
    IXMLDOMDocument_Release(doc);
}

static void test_dtd_validation(void)
{
    IXMLDOMParseError *err;
    IXMLDOMDocument2 *doc;
    HRESULT hr;
    VARIANT v;
    LONG res;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_I2;
    V_I2(&v) = 0;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"ProhibitDTD"), v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_put_validateOnParse(doc, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    res = 0x123;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(res == S_OK, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_0D), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ELEMENT_UNDECLARED */
    todo_wine ok(res == 0xC00CE00D, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_0E), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ELEMENT_ID_NOT_FOUND */
    todo_wine ok(res == 0xC00CE00E, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_11), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_EMPTY_NOT_ALLOWED */
    todo_wine ok(res == 0xC00CE011, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_13), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ROOT_NAME_MISMATCH */
    todo_wine ok(res == 0xC00CE013, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_14), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_INVALID_CONTENT */
    todo_wine ok(res == 0xC00CE014, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_15), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ATTRIBUTE_NOT_DEFINED */
    todo_wine ok(res == 0xC00CE015, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_16), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ATTRIBUTE_FIXED */
    todo_wine ok(res == 0xC00CE016, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_17), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ATTRIBUTE_VALUE */
    todo_wine ok(res == 0xC00CE017, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_18), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_ILLEGAL_TEXT */
    todo_wine ok(res == 0xC00CE018, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(email_xml_20), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    err = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    res = 0;
    hr = IXMLDOMParseError_get_errorCode(err, &res);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* XML_REQUIRED_ATTRIBUTE_MISSING */
    todo_wine ok(res == 0xC00CE020, "Unexpected code %#lx.\n", res);
    IXMLDOMParseError_Release(err);

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_max_element_depth_values(void)
{
    IXMLDOMParseError *parse_error;
    IXMLDOMDocument2 *doc, *doc3;
    IXMLDOMDocument *doc2;
    VARIANT var;
    HRESULT hr;
    LONG code;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"maxElementDepth"), &var);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* The default max element depth value should be 256. */
    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 0xdeadbeef;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"MaxElementDepth"), &var);
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_I4(&var) == 256, "Unexpected property value.\n");

    /* Changes to the depth value should be observable when subsequently retrieved. */
    V_VT(&var) = VT_I4;
    V_I4(&var) = 32;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"MaxElementDepth"), var);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 0xdeadbeef;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_(L"MaxElementDepth"), &var);
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_I4(&var) == 32, "Unexpected property value.\n");

    hr = IXMLDOMDocument2_cloneNode(doc, VARIANT_FALSE, (IXMLDOMNode **)&doc2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_QueryInterface(doc2, &IID_IXMLDOMDocument2, (void **)&doc3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 0xdeadbeef;
    hr = IXMLDOMDocument2_getProperty(doc3, _bstr_(L"MaxElementDepth"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_I4(&var) == 32, "Unexpected property value.\n");

    IXMLDOMDocument2_Release(doc3);
    IXMLDOMDocument_Release(doc2);

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"MaxElementDepth"), var);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 2147483648;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"MaxElementDepth"), var);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_(L"MaxElementDepth"), var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(L"<a>text<!-- comment --></a>"), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(L"<a>text<!-- comment --><b/></a>"), NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_get_parseError(doc, &parse_error);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMParseError_get_errorCode(parse_error, &code);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(code == 0xc00ce586, "Unexpected error code %#lx.\n", code);
    IXMLDOMParseError_Release(parse_error);

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

START_TEST(domdoc)
{
    HRESULT hr;
    IXMLDOMDocument2 *doc;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    if (hr != S_OK)
    {
        win_skip("class &CLSID_DOMDocument60 not supported\n");
        return;
    }
    IXMLDOMDocument2_Release(doc);

    test_namespaces_as_attributes();
    test_create_attribute();
    test_leading_spaces();
    test_get_ownerDocument();
    test_get_parentNode();
    test_normalize_attribute_values();
    test_prohibitdtd();
    test_interfaces();
    test_dtd_validation();
    test_max_element_depth_values();

    CoUninitialize();
}
