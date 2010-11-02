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

#include "initguid.h"
#include "windows.h"
#include "ole2.h"
#include "xmldom.h"
#include "msxml2.h"
#include "dispex.h"

#include "wine/test.h"

static const WCHAR xdr_schema_uri[] = {'x','-','s','c','h','e','m','a',':','t','e','s','t','.','x','m','l',0};

static const WCHAR xdr_schema_xml[] = {
    '<','S','c','h','e','m','a',' ','x','m','l','n','s','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','x','m','l','-','d','a','t','a','\"','\n',
    'x','m','l','n','s',':','d','t','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','d','a','t','a','t','y','p','e','s','\"','>','\n',
    '<','/','S','c','h','e','m','a','>','\n',0
};

static const CHAR xdr_schema1_uri[] = "x-schema:test1.xdr";
static const CHAR xdr_schema1_xml[] =
"<?xml version='1.0'?>"
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'"
"        name='test1.xdr'>"
"   <ElementType name='x' dt:type='boolean'/>"
"   <ElementType name='y'>"
"       <datatype dt:type='int'/>"
"   </ElementType>"
"   <ElementType name='z'/>"
"   <ElementType name='root' content='eltOnly' model='open' order='seq'>"
"       <element type='x'/>"
"       <element type='y'/>"
"       <element type='z'/>"
"   </ElementType>"
"</Schema>";

static const CHAR xdr_schema2_uri[] = "x-schema:test2.xdr";
static const CHAR xdr_schema2_xml[] =
"<?xml version='1.0'?>"
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'"
"        name='test2.xdr'>"
"   <ElementType name='x' dt:type='bin.base64'/>"
"   <ElementType name='y' dt:type='uuid'/>"
"   <ElementType name='z'/>"
"   <ElementType name='root' content='eltOnly' model='closed' order='one'>"
"       <element type='x'/>"
"       <element type='y'/>"
"       <element type='z'/>"
"   </ElementType>"
"</Schema>";

static const CHAR xdr_schema3_uri[] = "x-schema:test3.xdr";
static const CHAR xdr_schema3_xml[] =
"<?xml version='1.0'?>"
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'"
"        name='test3.xdr'>"
"   <ElementType name='root' content='textOnly' model='open'>"
"       <AttributeType name='x' dt:type='int'/>"
"       <AttributeType name='y' dt:type='enumeration' dt:values='a b c'/>"
"       <AttributeType name='z' dt:type='uuid'/>"
"       <attribute type='x'/>"
"       <attribute type='y'/>"
"       <attribute type='z'/>"
"   </ElementType>"
"</Schema>";

static const CHAR xsd_schema1_uri[] = "x-schema:test1.xsd";
static const CHAR xsd_schema1_xml[] =
"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test1.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

static const CHAR xsd_schema2_uri[] = "x-schema:test2.xsd";
static const CHAR xsd_schema2_xml[] =
"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test2.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

static const CHAR xsd_schema3_uri[] = "x-schema:test3.xsd";
static const CHAR xsd_schema3_xml[] =
"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test3.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";


#define check_ref_expr(expr, n) { \
    LONG ref = expr; \
    ok(ref == n, "expected %i refs, got %i\n", n, ref); \
}

#define check_refs(iface, obj, n) { \
    LONG ref = iface ## _AddRef(obj); \
    ok(ref == n+1, "expected %i refs, got %i\n", n+1, ref); \
    ref = iface ## _Release(obj); \
    ok(ref == n, "expected %i refs, got %i\n", n, ref); \
}

#define ole_check(expr) { \
    HRESULT r = expr; \
    ok(r == S_OK, #expr " returned %x\n", r); \
}

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %x\n", r, expect); \
}

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < sizeof(alloced_bstrs)/sizeof(alloced_bstrs[0]));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static VARIANT _variantdoc_(void* doc)
{
    VARIANT v;
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)doc;
    return v;
}

static void* _create_object(const GUID *clsid, const char *name, const IID *iid, int line)
{
    void *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, &obj);
    if (hr != S_OK)
        win_skip_(__FILE__,line)("failed to create %s instance: 0x%08x\n", name, hr);

    return obj;
}

#define _create(cls) cls, #cls

#define create_document(iid) _create_object(&_create(CLSID_DOMDocument), iid, __LINE__)

#define create_document_version(v, iid) _create_object(&_create(CLSID_DOMDocument ## v), iid, __LINE__)

#define create_cache(iid) _create_object(&_create(CLSID_XMLSchemaCache), iid, __LINE__)

#define create_cache_version(v, iid) _create_object(&_create(CLSID_XMLSchemaCache ## v), iid, __LINE__)

static void test_schema_refs(void)
{
    IXMLDOMDocument2 *doc;
    IXMLDOMSchemaCollection *cache;
    VARIANT v;
    VARIANT_BOOL b;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument2);
    if (!doc)
        return;

    cache = create_cache(&IID_IXMLDOMSchemaCollection);
    if(!cache)
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }

    VariantInit(&v);
    str = SysAllocString(xdr_schema_xml);
    ole_check(IXMLDOMDocument2_loadXML(doc, str, &b));
    ok(b == VARIANT_TRUE, "b %04x\n", b);
    SysFreeString(str);

    str = SysAllocString(xdr_schema_uri);
    ole_check(IXMLDOMSchemaCollection_add(cache, str, _variantdoc_(doc)));

    /* IXMLDOMSchemaCollection_add doesn't add a ref on doc */
    check_refs(IXMLDOMDocument2, doc, 1);

    SysFreeString(str);

    V_VT(&v) = VT_INT;
    ole_expect(IXMLDOMDocument2_get_schemas(doc, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "vt %x\n", V_VT(&v));

    check_ref_expr(IXMLDOMSchemaCollection_AddRef(cache), 2);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)cache;

    /* check that putref_schemas takes a ref */
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    check_refs(IXMLDOMSchemaCollection, cache, 3);

    VariantClear(&v); /* refs now 2 */

    V_VT(&v) = VT_INT;
    /* check that get_schemas adds a ref */
    ole_check(IXMLDOMDocument2_get_schemas(doc, &v));
    ok(V_VT(&v) == VT_DISPATCH, "vt %x\n", V_VT(&v));
    check_refs(IXMLDOMSchemaCollection, cache, 3);

    /* get_schemas doesn't release a ref if passed VT_DISPATCH - ie it doesn't call VariantClear() */
    ole_check(IXMLDOMDocument2_get_schemas(doc, &v));
    ok(V_VT(&v) == VT_DISPATCH, "vt %x\n", V_VT(&v));
    check_refs(IXMLDOMSchemaCollection, cache, 4);

    /* release the two refs returned by get_schemas */
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache), 3);
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache), 2);

    /* check that taking another ref on the document doesn't change the schema's ref count */
    check_ref_expr(IXMLDOMDocument2_AddRef(doc), 2);
    check_refs(IXMLDOMSchemaCollection, cache, 2);
    check_ref_expr(IXMLDOMDocument2_Release(doc), 1);

    /* call putref_schema with some odd variants */
    V_VT(&v) = VT_INT;
    ole_expect(IXMLDOMDocument2_putref_schemas(doc, v), E_FAIL);
    check_refs(IXMLDOMSchemaCollection, cache, 2);

    /* calling with VT_EMPTY releases the schema */
    V_VT(&v) = VT_EMPTY;
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    check_refs(IXMLDOMSchemaCollection, cache, 1);

    /* try setting with VT_UNKNOWN */
    check_ref_expr(IXMLDOMSchemaCollection_AddRef(cache), 2);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)cache;
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    check_refs(IXMLDOMSchemaCollection, cache, 3);

    VariantClear(&v); /* refs now 2 */

    /* calling with VT_NULL releases the schema */
    V_VT(&v) = VT_NULL;
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    check_refs(IXMLDOMSchemaCollection, cache, 1);

    /* refs now 1 */
    /* set again */
    check_ref_expr(IXMLDOMSchemaCollection_AddRef(cache), 2);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)cache;
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    check_refs(IXMLDOMSchemaCollection, cache, 3);

    VariantClear(&v); /* refs now 2 */

    /* release the final ref on the doc which should release its ref on the schema */
    check_ref_expr(IXMLDOMDocument2_Release(doc), 0);

    check_refs(IXMLDOMSchemaCollection, cache, 1);
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache), 0);
}

static void test_collection_refs(void)
{
    IXMLDOMDocument2 *schema1, *schema2, *schema3;
    IXMLDOMSchemaCollection *cache1, *cache2, *cache3;
    VARIANT_BOOL b;
    LONG length;

    schema1 = create_document(&IID_IXMLDOMDocument2);
    schema2 = create_document(&IID_IXMLDOMDocument2);
    schema3 = create_document(&IID_IXMLDOMDocument2);

    cache1 = create_cache(&IID_IXMLDOMSchemaCollection);
    cache2 = create_cache(&IID_IXMLDOMSchemaCollection);
    cache3 = create_cache(&IID_IXMLDOMSchemaCollection);

    if (!schema1 || !schema2 || !schema3 || !cache1 || !cache2 || !cache3)
    {
        if (schema1) IXMLDOMDocument2_Release(schema1);
        if (schema2) IXMLDOMDocument2_Release(schema2);
        if (schema3) IXMLDOMDocument2_Release(schema3);

        if (cache1) IXMLDOMSchemaCollection_Release(cache1);
        if (cache2) IXMLDOMSchemaCollection_Release(cache2);
        if (cache3) IXMLDOMSchemaCollection_Release(cache2);

        return;
    }

    ole_check(IXMLDOMDocument2_loadXML(schema1, _bstr_(xdr_schema1_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema2, _bstr_(xdr_schema2_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema3, _bstr_(xdr_schema3_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMSchemaCollection_add(cache1, _bstr_(xdr_schema1_uri), _variantdoc_(schema1)));
    ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xdr_schema2_uri), _variantdoc_(schema2)));
    ole_check(IXMLDOMSchemaCollection_add(cache3, _bstr_(xdr_schema3_uri), _variantdoc_(schema3)));

    check_ref_expr(IXMLDOMDocument2_Release(schema1), 0);
    check_ref_expr(IXMLDOMDocument2_Release(schema2), 0);
    check_ref_expr(IXMLDOMDocument2_Release(schema3), 0);
    schema1 = NULL;
    schema2 = NULL;
    schema3 = NULL;

    /* releasing the original doc does not affect the schema cache */
    ole_check(IXMLDOMSchemaCollection_get(cache1, _bstr_(xdr_schema1_uri), (IXMLDOMNode**)&schema1));
    ole_check(IXMLDOMSchemaCollection_get(cache2, _bstr_(xdr_schema2_uri), (IXMLDOMNode**)&schema2));
    ole_check(IXMLDOMSchemaCollection_get(cache3, _bstr_(xdr_schema3_uri), (IXMLDOMNode**)&schema3));

    /* we get a read-only domdoc interface, created just for us */
    if (schema1) check_refs(IXMLDOMDocument2, schema1, 1);
    if (schema2) check_refs(IXMLDOMDocument2, schema2, 1);
    if (schema3) check_refs(IXMLDOMDocument2, schema3, 1);

    ole_expect(IXMLDOMSchemaCollection_addCollection(cache1, NULL), E_POINTER);
    ole_check(IXMLDOMSchemaCollection_addCollection(cache2, cache1));
    ole_check(IXMLDOMSchemaCollection_addCollection(cache3, cache2));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache1, &length));
    ok(length == 1, "expected length 1, got %i\n", length);

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache2, &length));
    ok(length == 2, "expected length 2, got %i\n", length);

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache3, &length));
    ok(length == 3, "expected length 3, got %i\n", length);


    /* merging collections does not affect the ref count */
    check_refs(IXMLDOMSchemaCollection, cache1, 1);
    check_refs(IXMLDOMSchemaCollection, cache2, 1);
    check_refs(IXMLDOMSchemaCollection, cache3, 1);

    /* nor does it affect the domdoc instances */
    if (schema1) check_refs(IXMLDOMDocument2, schema1, 1);
    if (schema2) check_refs(IXMLDOMDocument2, schema2, 1);
    if (schema3) check_refs(IXMLDOMDocument2, schema3, 1);

    if (schema1) check_ref_expr(IXMLDOMDocument2_Release(schema1), 0);
    if (schema2) check_ref_expr(IXMLDOMDocument2_Release(schema2), 0);
    if (schema3) check_ref_expr(IXMLDOMDocument2_Release(schema3), 0);
    schema1 = NULL;
    schema2 = NULL;
    schema3 = NULL;

    /* releasing the domdoc instances doesn't change the cache */
    ole_check(IXMLDOMSchemaCollection_get(cache1, _bstr_(xdr_schema1_uri), (IXMLDOMNode**)&schema1));
    ole_check(IXMLDOMSchemaCollection_get(cache2, _bstr_(xdr_schema2_uri), (IXMLDOMNode**)&schema2));
    ole_check(IXMLDOMSchemaCollection_get(cache3, _bstr_(xdr_schema3_uri), (IXMLDOMNode**)&schema3));

    /* we can just get them again */
    if (schema1) check_refs(IXMLDOMDocument2, schema1, 1);
    if (schema2) check_refs(IXMLDOMDocument2, schema2, 1);
    if (schema3) check_refs(IXMLDOMDocument2, schema3, 1);

    /* releasing the caches does not affect the domdoc instances */
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache1), 0);
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache2), 0);
    check_ref_expr(IXMLDOMSchemaCollection_Release(cache3), 0);

    /* they're just for us */
    if (schema1) check_refs(IXMLDOMDocument2, schema1, 1);
    if (schema2) check_refs(IXMLDOMDocument2, schema2, 1);
    if (schema3) check_refs(IXMLDOMDocument2, schema3, 1);

    if (schema1) check_ref_expr(IXMLDOMDocument2_Release(schema1), 0);
    if (schema2) check_ref_expr(IXMLDOMDocument2_Release(schema2), 0);
    if (schema3) check_ref_expr(IXMLDOMDocument2_Release(schema3), 0);

    free_bstrs();
}

static void test_length(void)
{
    IXMLDOMDocument2 *schema1, *schema2, *schema3;
    IXMLDOMSchemaCollection *cache;
    VARIANT_BOOL b;
    VARIANT v;
    LONG length;

    schema1 = create_document(&IID_IXMLDOMDocument2);
    schema2 = create_document(&IID_IXMLDOMDocument2);
    schema3 = create_document(&IID_IXMLDOMDocument2);

    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    if (!schema1 || !schema2 || !schema3 || !cache)
    {
        if (schema1) IXMLDOMDocument2_Release(schema1);
        if (schema2) IXMLDOMDocument2_Release(schema2);
        if (schema3) IXMLDOMDocument2_Release(schema3);

        if (cache) IXMLDOMSchemaCollection_Release(cache);

        return;
    }

    VariantInit(&v);

    ole_check(IXMLDOMDocument2_loadXML(schema1, _bstr_(xdr_schema1_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema2, _bstr_(xdr_schema2_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema3, _bstr_(xdr_schema3_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_expect(IXMLDOMSchemaCollection_get_length(cache, NULL), E_POINTER);

    /* MSDN lies; removing a nonexistent entry produces no error */
    ole_check(IXMLDOMSchemaCollection_remove(cache, NULL));
    ole_check(IXMLDOMSchemaCollection_remove(cache, _bstr_(xdr_schema1_uri)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 0, "expected length 0, got %i\n", length);

    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema1_uri), _variantdoc_(schema1)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 1, "expected length 1, got %i\n", length);

    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema2_uri), _variantdoc_(schema2)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 2, "expected length 2, got %i\n", length);

    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema3_uri), _variantdoc_(schema3)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 3, "expected length 3, got %i\n", length);

    /* adding with VT_NULL is the same as removing */
    V_VT(&v) = VT_NULL;
    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema1_uri), v));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 2, "expected length 2, got %i\n", length);

    ole_check(IXMLDOMSchemaCollection_remove(cache, _bstr_(xdr_schema2_uri)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 1, "expected length 1, got %i\n", length);

    ole_check(IXMLDOMSchemaCollection_remove(cache, _bstr_(xdr_schema3_uri)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &length));
    ok(length == 0, "expected length 0, got %i\n", length);

    IXMLDOMDocument2_Release(schema1);
    IXMLDOMDocument2_Release(schema2);
    IXMLDOMDocument2_Release(schema3);
    IXMLDOMSchemaCollection_Release(cache);

    free_bstrs();
}

static void test_collection_content(void)
{
    IXMLDOMDocument2 *schema1, *schema2, *schema3, *schema4, *schema5;
    IXMLDOMSchemaCollection *cache1, *cache2;
    VARIANT_BOOL b;
    BSTR bstr;
    BSTR content[5] = {NULL, NULL, NULL, NULL, NULL};
    LONG length;
    int i, j;

    schema1 = create_document_version(30, &IID_IXMLDOMDocument2);
    schema2 = create_document_version(30, &IID_IXMLDOMDocument2);
    schema3 = create_document_version(30, &IID_IXMLDOMDocument2);

    cache1 = create_cache_version(30, &IID_IXMLDOMSchemaCollection);
    cache2 = create_cache_version(40, &IID_IXMLDOMSchemaCollection);

    if (!schema1 || !schema2 || !schema3 || !cache1)
    {
        if (schema1) IXMLDOMDocument2_Release(schema1);
        if (schema2) IXMLDOMDocument2_Release(schema2);
        if (schema3) IXMLDOMDocument2_Release(schema3);

        if (cache1) IXMLDOMSchemaCollection_Release(cache1);

        return;
    }

    ole_check(IXMLDOMDocument2_loadXML(schema1, _bstr_(xdr_schema1_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema2, _bstr_(xdr_schema2_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMDocument2_loadXML(schema3, _bstr_(xdr_schema3_xml), &b));
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    ole_check(IXMLDOMSchemaCollection_add(cache1, _bstr_(xdr_schema1_uri), _variantdoc_(schema1)));
    ole_check(IXMLDOMSchemaCollection_add(cache1, _bstr_(xdr_schema2_uri), _variantdoc_(schema2)));
    ole_check(IXMLDOMSchemaCollection_add(cache1, _bstr_(xdr_schema3_uri), _variantdoc_(schema3)));

    length = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache1, &length));
    ok(length == 3, "expected length 3, got %i\n", length);

    IXMLDOMDocument2_Release(schema1);
    IXMLDOMDocument2_Release(schema2);
    IXMLDOMDocument2_Release(schema3);

    if (cache2)
    {
        schema1 = create_document_version(40, &IID_IXMLDOMDocument2);
        schema2 = create_document_version(40, &IID_IXMLDOMDocument2);
        schema3 = create_document_version(40, &IID_IXMLDOMDocument2);
        schema4 = create_document_version(40, &IID_IXMLDOMDocument2);
        schema5 = create_document_version(40, &IID_IXMLDOMDocument2);
        ole_check(IXMLDOMDocument2_loadXML(schema1, _bstr_(xdr_schema1_xml), &b));
        ok(b == VARIANT_TRUE, "failed to load XML\n");
        ole_check(IXMLDOMDocument2_loadXML(schema2, _bstr_(xdr_schema2_xml), &b));
        ok(b == VARIANT_TRUE, "failed to load XML\n");
        ole_check(IXMLDOMDocument2_loadXML(schema3, _bstr_(xsd_schema1_xml), &b));
        ok(b == VARIANT_TRUE, "failed to load XML\n");
        ole_check(IXMLDOMDocument2_loadXML(schema4, _bstr_(xsd_schema2_xml), &b));
        ok(b == VARIANT_TRUE, "failed to load XML\n");
        ole_check(IXMLDOMDocument2_loadXML(schema5, _bstr_(xsd_schema3_xml), &b));
        ok(b == VARIANT_TRUE, "failed to load XML\n");

        /* combining XDR and XSD schemas in the same cache is fine */
        ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xdr_schema1_uri), _variantdoc_(schema1)));
        ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xdr_schema2_uri), _variantdoc_(schema2)));
        ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xsd_schema1_uri), _variantdoc_(schema3)));
        ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xsd_schema2_uri), _variantdoc_(schema4)));
        ole_check(IXMLDOMSchemaCollection_add(cache2, _bstr_(xsd_schema3_uri), _variantdoc_(schema5)));

        length = -1;
        ole_check(IXMLDOMSchemaCollection_get_length(cache2, &length));
        ok(length == 5, "expected length 5, got %i\n", length);

        IXMLDOMDocument2_Release(schema1);
        IXMLDOMDocument2_Release(schema2);
        IXMLDOMDocument2_Release(schema3);
        IXMLDOMDocument2_Release(schema4);
        IXMLDOMDocument2_Release(schema5);
    }

    bstr = NULL;
    /* error if index is out of range */
    ole_expect(IXMLDOMSchemaCollection_get_namespaceURI(cache1, 3, &bstr), E_FAIL);
    SysFreeString(bstr);
    /* error if return pointer is NULL */
    ole_expect(IXMLDOMSchemaCollection_get_namespaceURI(cache1, 0, NULL), E_POINTER);
    /* pointer is checked first */
    ole_expect(IXMLDOMSchemaCollection_get_namespaceURI(cache1, 3, NULL), E_POINTER);

    schema1 = NULL;
    /* no error if ns uri does not exist */
    ole_check(IXMLDOMSchemaCollection_get(cache1, _bstr_(xsd_schema1_uri), (IXMLDOMNode**)&schema1));
    ok(!schema1, "expected NULL\n");
    /* a NULL bstr corresponds to no-uri ns */
    ole_check(IXMLDOMSchemaCollection_get(cache1, NULL, (IXMLDOMNode**)&schema1));
    ok(!schema1, "expected NULL\n");
    /* error if return pointer is NULL */
    ole_expect(IXMLDOMSchemaCollection_get(cache1, _bstr_(xdr_schema1_uri), NULL), E_POINTER);

    for (i = 0; i < 3; ++i)
    {
        bstr = NULL;
        ole_check(IXMLDOMSchemaCollection_get_namespaceURI(cache1, i, &bstr));
        ok(bstr != NULL && *bstr, "expected non-empty string\n");
        content[i] = bstr;

        for (j = 0; j < i; ++j)
            ok(lstrcmpW(content[j], bstr), "got duplicate entry\n");
    }

    for (i = 0; i < 3; ++i)
    {
        SysFreeString(content[i]);
        content[i] = NULL;
    }

    if (cache2)
    {
        for (i = 0; i < 5; ++i)
        {
            bstr = NULL;
            ole_check(IXMLDOMSchemaCollection_get_namespaceURI(cache2, i, &bstr));
            ok(bstr != NULL && *bstr, "expected non-empty string\n");

            for (j = 0; j < i; ++j)
                ok(lstrcmpW(content[j], bstr), "got duplicate entry\n");
            content[i] = bstr;
        }

        for (i = 0; i < 5; ++i)
        {
            SysFreeString(content[i]);
            content[i] = NULL;
        }
    }

    IXMLDOMSchemaCollection_Release(cache1);
    if (cache2) IXMLDOMSchemaCollection_Release(cache2);

    free_bstrs();
}

START_TEST(schema)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    test_schema_refs();
    test_collection_refs();
    test_length();
    test_collection_content();

    CoUninitialize();
}
