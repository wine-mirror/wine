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
#include "msxml2.h"
#include "dispex.h"

#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

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

static const CHAR szDatatypeXDR[] =
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'\n"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'>\n"
"   <ElementType name='base64Data' content='textOnly' dt:type='bin.base64'/>\n"
"   <ElementType name='hexData' content='textOnly' dt:type='bin.hex'/>\n"
"   <ElementType name='boolData' content='textOnly' dt:type='boolean'/>\n"
"   <ElementType name='charData' content='textOnly' dt:type='char'/>\n"
"   <ElementType name='dateData' content='textOnly' dt:type='date'/>\n"
"   <ElementType name='dateTimeData' content='textOnly' dt:type='dateTime'/>\n"
"   <ElementType name='dateTimeTzData' content='textOnly' dt:type='dateTime.tz'/>\n"
"   <ElementType name='entityData' content='textOnly' dt:type='entity'/>\n"
"   <ElementType name='entitiesData' content='textOnly' dt:type='entities'/>\n"
"   <ElementType name='fixedData' content='textOnly' dt:type='fixed.14.4'/>\n"
"   <ElementType name='floatData' content='textOnly' dt:type='float'/>\n"
"   <ElementType name='i1Data' content='textOnly' dt:type='i1'/>\n"
"   <ElementType name='i2Data' content='textOnly' dt:type='i2'/>\n"
"   <ElementType name='i4Data' content='textOnly' dt:type='i4'/>\n"
"   <ElementType name='i8Data' content='textOnly' dt:type='i8'/>\n"
"   <ElementType name='intData' content='textOnly' dt:type='int'/>\n"
"   <ElementType name='nmtokData' content='textOnly' dt:type='nmtoken'/>\n"
"   <ElementType name='nmtoksData' content='textOnly' dt:type='nmtokens'/>\n"
"   <ElementType name='numData' content='textOnly' dt:type='number'/>\n"
"   <ElementType name='r4Data' content='textOnly' dt:type='r4'/>\n"
"   <ElementType name='r8Data' content='textOnly' dt:type='r8'/>\n"
"   <ElementType name='stringData' content='textOnly' dt:type='string'/>\n"
"   <ElementType name='timeData' content='textOnly' dt:type='time'/>\n"
"   <ElementType name='timeTzData' content='textOnly' dt:type='time.tz'/>\n"
"   <ElementType name='u1Data' content='textOnly' dt:type='ui1'/>\n"
"   <ElementType name='u2Data' content='textOnly' dt:type='ui2'/>\n"
"   <ElementType name='u4Data' content='textOnly' dt:type='ui4'/>\n"
"   <ElementType name='u8Data' content='textOnly' dt:type='ui8'/>\n"
"   <ElementType name='uriData' content='textOnly' dt:type='uri'/>\n"
"   <ElementType name='uuidData' content='textOnly' dt:type='uuid'/>\n"
"\n"
"   <ElementType name='Name' content='textOnly' dt:type='nmtoken'/>\n"
"   <ElementType name='Value' content='eltOnly' order='many'>\n"
"       <element type='base64Data'/>\n"
"       <element type='hexData'/>\n"
"       <element type='boolData'/>\n"
"       <element type='charData'/>\n"
"       <element type='dateData'/>\n"
"       <element type='dateTimeData'/>\n"
"       <element type='dateTimeTzData'/>\n"
"       <element type='entityData'/>\n"
"       <element type='entitiesData'/>\n"
"       <element type='fixedData'/>\n"
"       <element type='floatData'/>\n"
"       <element type='i1Data'/>\n"
"       <element type='i2Data'/>\n"
"       <element type='i4Data'/>\n"
"       <element type='i8Data'/>\n"
"       <element type='intData'/>\n"
"       <element type='nmtokData'/>\n"
"       <element type='nmtoksData'/>\n"
"       <element type='numData'/>\n"
"       <element type='r4Data'/>\n"
"       <element type='r8Data'/>\n"
"       <element type='stringData'/>\n"
"       <element type='timeData'/>\n"
"       <element type='timeTzData'/>\n"
"       <element type='u1Data'/>\n"
"       <element type='u2Data'/>\n"
"       <element type='u4Data'/>\n"
"       <element type='u8Data'/>\n"
"       <element type='uriData'/>\n"
"       <element type='uuidData'/>\n"
"   </ElementType>\n"
"   <ElementType name='Property' content='eltOnly' order='seq'>\n"
"       <element type='Name'/>\n"
"       <element type='Value'/>\n"
"   </ElementType>\n"
"   <ElementType name='Properties' content='eltOnly'>\n"
"       <element type='Property' minOccurs='0' maxOccurs='*'/>\n"
"   </ElementType>\n"
"</Schema>";

static const CHAR szDatatypeXML[] =
"<?xml version='1.0'?>\n"
"<Properties xmlns='urn:x-schema:datatype-test-xdr'>\n"
"   <Property>\n"
"       <Name>testBase64</Name>\n"
"       <Value>\n"
"           <base64Data>+HugeNumber+</base64Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testHex</Name>\n"
"       <Value>\n"
"           <hexData>deadbeef</hexData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testBool</Name>\n"
"       <Value>\n"
"           <boolData>1</boolData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testChar</Name>\n"
"       <Value>\n"
"           <charData>u</charData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testDate</Name>\n"
"       <Value>\n"
"           <dateData>1998-02-01</dateData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testDateTime</Name>\n"
"       <Value>\n"
"           <dateTimeData>1998-02-01T12:34:56</dateTimeData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testDateTimeTz</Name>\n"
"       <Value>\n"
"           <dateTimeTzData>1998-02-01T12:34:56-06:00</dateTimeTzData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testFixed</Name>\n"
"       <Value>\n"
"           <fixedData>3.1416</fixedData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testFloat</Name>\n"
"       <Value>\n"
"           <floatData>3.14159</floatData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testI1</Name>\n"
"       <Value>\n"
"           <i1Data>42</i1Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testI2</Name>\n"
"       <Value>\n"
"           <i2Data>420</i2Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testI4</Name>\n"
"       <Value>\n"
"           <i4Data>-420000000</i4Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testI8</Name>\n"
"       <Value>\n"
"           <i8Data>-4200000000</i8Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testInt</Name>\n"
"       <Value>\n"
"           <intData>42</intData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testNmtoken</Name>\n"
"       <Value>\n"
"           <nmtokData>tok1</nmtokData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testNmtokens</Name>\n"
"       <Value>\n"
"           <nmtoksData>tok1 tok2 tok3</nmtoksData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testNumber</Name>\n"
"       <Value>\n"
"           <numData>3.14159</numData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testR4</Name>\n"
"       <Value>\n"
"           <r4Data>3.14159265</r4Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testR8</Name>\n"
"       <Value>\n"
"           <r8Data>3.14159265358979323846</r8Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testString</Name>\n"
"       <Value>\n"
"           <stringData>foo bar</stringData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testTime</Name>\n"
"       <Value>\n"
"           <timeData>12:34:56</timeData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testTimeTz</Name>\n"
"       <Value>\n"
"           <timeTzData>12:34:56-06:00</timeTzData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testU1</Name>\n"
"       <Value>\n"
"           <u1Data>255</u1Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testU2</Name>\n"
"       <Value>\n"
"           <u2Data>65535</u2Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testU4</Name>\n"
"       <Value>\n"
"           <u4Data>4294967295</u4Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testU8</Name>\n"
"       <Value>\n"
"           <u8Data>18446744073709551615</u8Data>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testURI</Name>\n"
"       <Value>\n"
"           <uriData>urn:schemas-microsoft-com:datatypes</uriData>\n"
"       </Value>\n"
"   </Property>\n"
"   <Property>\n"
"       <Name>testUUID</Name>\n"
"       <Value>\n"
"           <uuidData>2933BF81-7B36-11D2-B20E-00C04F983E60</uuidData>\n"
"       </Value>\n"
"   </Property>\n"
"</Properties>";

static const CHAR szOpenSeqXDR[] =
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'>\n"
"   <ElementType name='w' content='empty' model='closed'/>\n"
"   <ElementType name='x' content='empty' model='closed'/>\n"
"   <ElementType name='y' content='empty' model='closed'/>\n"
"   <ElementType name='z' content='empty' model='closed'/>\n"
"   <ElementType name='test' content='eltOnly' model='open' order='seq'>\n"
"       <element type='x'/>\n"
"       <group order='seq'>\n"
"           <element type='x'/>\n"
"           <element type='y'/>\n"
"           <element type='z'/>\n"
"       </group>\n"
"       <element type='z'/>\n"
"   </ElementType>\n"
"</Schema>";

static const CHAR szOpenSeqXML1[] = "<test><x/><x/><y/><z/><z/></test>";
static const CHAR szOpenSeqXML2[] = "<test><x/><x/><y/><z/><z/><w/></test>";
static const CHAR szOpenSeqXML3[] = "<test><w/><x/><x/><y/><z/><z/></test>";
static const CHAR szOpenSeqXML4[] = "<test><x/><x/><y/><z/><z/><v/></test>";

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

#define _expect64(expr, str, base, TYPE, CONV) { \
    TYPE v1 = expr; \
    TYPE v2 = CONV(str, NULL, base); \
    ok(v1 == v2, #expr "returned 0x%08x%08x, expected 0x%08x%08x\n", \
                  (ULONG)((ULONG64)v1 >> 32), (ULONG)((ULONG64)v2 & (ULONG64)0xffffffff), \
                  (ULONG)((ULONG64)v1 >> 32), (ULONG)((ULONG64)v2 & (ULONG64)0xffffffff)); \
}

#define expect_int64(expr, x, base) _expect64(expr, #x, base, LONG64, strtoll)
#define expect_uint64(expr, x, base) _expect64(expr, #x, base, ULONG64, strtoull)

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
    static const WCHAR emptyW[] = {0};
    IXMLDOMDocument2 *doc;
    IXMLDOMNode *node;
    IXMLDOMSchemaCollection *cache;
    VARIANT v;
    VARIANT_BOOL b;
    BSTR str;
    LONG len;

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

    node = (void*)0xdeadbeef;
    ole_check(IXMLDOMSchemaCollection_get(cache, NULL, &node));
    ok(node == NULL, "%p\n", node);

    /* NULL uri pointer, still adds a document */
    ole_check(IXMLDOMSchemaCollection_add(cache, NULL, _variantdoc_(doc)));
    len = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &len));
    ok(len == 1, "got %d\n", len);
    /* read back - empty valid BSTR */
    str = NULL;
    ole_check(IXMLDOMSchemaCollection_get_namespaceURI(cache, 0, &str));
    ok(str && *str == 0, "got %p\n", str);
    SysFreeString(str);

    node = NULL;
    ole_check(IXMLDOMSchemaCollection_get(cache, NULL, &node));
    ok(node != NULL, "%p\n", node);
    IXMLDOMNode_Release(node);

    node = NULL;
    str = SysAllocString(emptyW);
    ole_check(IXMLDOMSchemaCollection_get(cache, str, &node));
    ok(node != NULL, "%p\n", node);
    IXMLDOMNode_Release(node);
    SysFreeString(str);

    /* remove with NULL uri */
    ole_check(IXMLDOMSchemaCollection_remove(cache, NULL));
    len = -1;
    ole_check(IXMLDOMSchemaCollection_get_length(cache, &len));
    ok(len == 0, "got %d\n", len);

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
            ok(winetest_strcmpW(content[j], bstr), "got duplicate entry\n");
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
                ok(winetest_strcmpW(content[j], bstr), "got duplicate entry\n");
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

static void test_XDR_schemas(void)
{
    IXMLDOMDocument2 *doc, *schema;
    IXMLDOMSchemaCollection* cache;
    IXMLDOMParseError* err;
    VARIANT_BOOL b;
    VARIANT v;
    BSTR bstr;

    doc = create_document(&IID_IXMLDOMDocument2);
    schema = create_document(&IID_IXMLDOMDocument2);
    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    if (!doc || !schema || !cache)
    {
        if (doc)    IXMLDOMDocument2_Release(doc);
        if (schema) IXMLDOMDocument2_Release(schema);
        if (cache)  IXMLDOMSchemaCollection_Release(cache);

        return;
    }

    VariantInit(&v);

    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szOpenSeqXML1), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    ole_check(IXMLDOMDocument2_loadXML(schema, _bstr_(szOpenSeqXDR), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* load the schema */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMDocument2_QueryInterface(schema, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_(""), v));
    VariantClear(&v);

    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    VariantClear(&v);

    /* validate the doc
     * only declared elements in the declared order
     * this is fine */
    err = NULL;
    bstr = NULL;
    ole_check(IXMLDOMDocument2_validate(doc, &err));
    ok(err != NULL, "domdoc_validate() should always set err\n");
    ole_expect(IXMLDOMParseError_get_reason(err, &bstr), S_FALSE);
    ok(IXMLDOMParseError_get_reason(err, &bstr) == S_FALSE, "got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    /* load the next doc */
    IXMLDOMDocument2_Release(doc);
    doc = create_document(&IID_IXMLDOMDocument2);
    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szOpenSeqXML2), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    VariantClear(&v);

    /* validate the doc
     * declared elements in the declared order, with an extra declared element at the end
     * this is fine */
    err = NULL;
    bstr = NULL;
    ole_check(IXMLDOMDocument2_validate(doc, &err));
    ok(err != NULL, "domdoc_validate() should always set err\n");
    ole_expect(IXMLDOMParseError_get_reason(err, &bstr), S_FALSE);
    ok(IXMLDOMParseError_get_reason(err, &bstr) == S_FALSE, "got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    /* load the next doc */
    IXMLDOMDocument2_Release(doc);
    doc = create_document(&IID_IXMLDOMDocument2);
    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szOpenSeqXML3), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    VariantClear(&v);

    /* validate the doc
     * fails, extra elements are only allowed at the end */
    err = NULL;
    bstr = NULL;
    ole_expect(IXMLDOMDocument2_validate(doc, &err), S_FALSE);
    ok(err != NULL, "domdoc_validate() should always set err\n");
    todo_wine ok(IXMLDOMParseError_get_reason(err, &bstr) == S_OK, "got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    /* load the next doc */
    IXMLDOMDocument2_Release(doc);
    doc = create_document(&IID_IXMLDOMDocument2);
    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szOpenSeqXML4), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    VariantClear(&v);

    /* validate the doc
     * fails, undeclared elements are not allowed */
    err = NULL;
    bstr = NULL;
    ole_expect(IXMLDOMDocument2_validate(doc, &err), S_FALSE);
    ok(err != NULL, "domdoc_validate() should always set err\n");
    todo_wine ok(IXMLDOMParseError_get_reason(err, &bstr) == S_OK, "got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    IXMLDOMDocument2_Release(doc);
    IXMLDOMDocument2_Release(schema);
    IXMLDOMSchemaCollection_Release(cache);

    free_bstrs();
}

static void test_XDR_datatypes(void)
{
    IXMLDOMDocument2 *doc, *schema;
    IXMLDOMSchemaCollection* cache;
    IXMLDOMNode* node;
    IXMLDOMParseError* err;
    VARIANT_BOOL b;
    VARIANT v, vtmp;
    LONG l;
    BSTR bstr;

    VariantInit(&v);

    doc = create_document(&IID_IXMLDOMDocument2);
    schema = create_document(&IID_IXMLDOMDocument2);
    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    if (!doc || !schema || !cache)
    {
        if (doc)    IXMLDOMDocument2_Release(doc);
        if (schema) IXMLDOMDocument2_Release(schema);
        if (cache)  IXMLDOMSchemaCollection_Release(cache);

        return;
    }

    ole_check(IXMLDOMDocument2_loadXML(doc, _bstr_(szDatatypeXML), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    ole_check(IXMLDOMDocument2_loadXML(schema, _bstr_(szDatatypeXDR), &b));
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    err = NULL;
    ole_expect(IXMLDOMDocument2_validate(doc, &err), S_FALSE);
    ok(err != NULL, "domdoc_validate() should always set err\n");
    ole_check(IXMLDOMParseError_get_errorCode(err, &l));
    ok(l == E_XML_NODTD, "got %08x\n", l);
    IXMLDOMParseError_Release(err);

    /* check data types without the schema */
    node = NULL;
    memset(&vtmp, -1, sizeof(VARIANT));
    V_VT(&vtmp) = VT_NULL;
    V_BSTR(&vtmp) = NULL;
    memset(&v, -1, sizeof(VARIANT));
    V_VT(&v) = VT_EMPTY;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testBase64']/Value/base64Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    /* when returning VT_NULL, the pointer is set to NULL */
    ok(memcmp(&v, &vtmp, sizeof(VARIANT)) == 0, "got %p\n", V_BSTR(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testHex']/Value/hexData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testBool']/Value/boolData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testChar']/Value/charData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDate']/Value/dateData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDateTime']/Value/dateTimeData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDateTimeTz']/Value/dateTimeTzData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testFixed']/Value/fixedData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testFloat']/Value/floatData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI1']/Value/i1Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI2']/Value/i2Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI4']/Value/i4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI8']/Value/i8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testInt']/Value/intData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNmtoken']/Value/nmtokData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNmtokens']/Value/nmtoksData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNumber']/Value/numData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testR4']/Value/r4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testR8']/Value/r8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testString']/Value/stringData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testTime']/Value/timeData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testTimeTz']/Value/timeTzData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU1']/Value/u1Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU2']/Value/u2Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU4']/Value/u4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU8']/Value/u8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testURI']/Value/uriData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testUUID']/Value/uuidData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    /* now load the schema */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMDocument2_QueryInterface(schema, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMSchemaCollection_add(cache, _bstr_("urn:x-schema:datatype-test-xdr"), v));
    VariantClear(&v);

    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    ole_check(IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v)));
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    ole_check(IXMLDOMDocument2_putref_schemas(doc, v));
    VariantClear(&v);

    /* validate the doc */
    err = NULL;
    l = 0;
    bstr = NULL;
    ole_check(IXMLDOMDocument2_validate(doc, &err));
    ok(err != NULL, "domdoc_validate() should always set err\n");
    ole_expect(IXMLDOMParseError_get_errorCode(err, &l), S_FALSE);
    ole_expect(IXMLDOMParseError_get_reason(err, &bstr), S_FALSE);
    ok(l == 0, "got %08x : %s\n", l, wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    /* check the data types again */
    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testBase64']/Value/base64Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("bin.base64")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == (VT_ARRAY|VT_UI1), "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testHex']/Value/hexData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("bin.hex")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == (VT_ARRAY|VT_UI1), "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testBool']/Value/boolData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("boolean")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BOOL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BOOL)
        ok(V_BOOL(&v) == VARIANT_TRUE, "got %04x\n", V_BOOL(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testChar']/Value/charData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("char")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    todo_wine ok(V_VT(&v) == VT_I4, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I4)
        ok(V_I4(&v) == 'u', "got %08x\n", V_I4(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDate']/Value/dateData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("date")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_DATE, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDateTime']/Value/dateTimeData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("dateTime")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_DATE, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testDateTimeTz']/Value/dateTimeTzData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("dateTime.tz")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_DATE, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testFixed']/Value/fixedData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("fixed.14.4")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_CY, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testFloat']/Value/floatData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("float")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_R8, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_R8)
        ok(V_R8(&v) == (double)3.14159, "got %f\n", V_R8(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI1']/Value/i1Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("i1")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_I1, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I1)
        ok(V_I1(&v) == 42, "got %i\n", V_I1(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI2']/Value/i2Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("i2")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_I2, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I2)
        ok(V_I2(&v) == 420, "got %i\n", V_I2(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI4']/Value/i4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("i4")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_I4, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I4)
        ok(V_I4(&v) == -420000000, "got %i\n", V_I4(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testI8']/Value/i8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("i8")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    /* TODO: which platforms return VT_I8? */
    todo_wine ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I8)
        expect_int64(V_I8(&v), -4200000000, 10);
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testInt']/Value/intData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("int")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_I4, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_I4)
        ok(V_I4(&v) == 42, "got %i\n", V_I4(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    /* nmtoken does not return a bstr for get_dataType() */
    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNmtoken']/Value/nmtokData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    /* nmtokens does not return a bstr for get_dataType() */
    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNmtokens']/Value/nmtoksData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testNumber']/Value/numData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("number")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testR4']/Value/r4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("r4")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_R4, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_R4)
        ok(V_R4(&v) == (float)3.14159265, "got %f\n", V_R4(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testR8']/Value/r8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("r8")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_R8, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_R8)
        todo_wine ok(V_R8(&v) == (double)3.14159265358979323846, "got %.20f\n", V_R8(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    /* dt:string is the default, does not return a bstr for get_dataType() */
    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testString']/Value/stringData"), &node));
    ok(node != NULL, "expected node\n");
    ole_expect(IXMLDOMNode_get_dataType(node, &v), S_FALSE);
    ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testTime']/Value/timeData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("time")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_DATE, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testTimeTz']/Value/timeTzData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("time.tz")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_DATE, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU1']/Value/u1Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("ui1")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_UI1, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_UI1)
        ok(V_UI1(&v) == 0xFF, "got %02x\n", V_UI1(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU2']/Value/u2Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("ui2")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_UI2, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_UI2)
        ok(V_UI2(&v) == 0xFFFF, "got %04x\n", V_UI2(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU4']/Value/u4Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("ui4")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_UI4, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_UI4)
        ok(V_UI4(&v) == 0xFFFFFFFF, "got %08x\n", V_UI4(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testU8']/Value/u8Data"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("ui8")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    /* TODO: which platforms return VT_UI8? */
    todo_wine ok(V_VT(&v) == VT_NULL, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_UI8)
        expect_uint64(V_UI8(&v), 0xFFFFFFFFFFFFFFFF, 16);
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testURI']/Value/uriData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("uri")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    node = NULL;
    ole_check(IXMLDOMDocument2_selectSingleNode(doc, _bstr_("//Property[Name!text()='testUUID']/Value/uuidData"), &node));
    ok(node != NULL, "expected node\n");
    ole_check(IXMLDOMNode_get_dataType(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    if (V_VT(&v) == VT_BSTR)
        ok(lstrcmpW(V_BSTR(&v), _bstr_("uuid")) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    ole_check(IXMLDOMNode_get_nodeTypedValue(node, &v));
    ok(V_VT(&v) == VT_BSTR, "got variant type %i\n", V_VT(&v));
    VariantClear(&v);
    IXMLDOMNode_Release(node);

    ok(IXMLDOMDocument2_Release(schema) == 0, "schema not released\n");
    ok(IXMLDOMDocument2_Release(doc) == 0, "doc not released\n");
    ok(IXMLDOMSchemaCollection_Release(cache) == 0, "cache not released\n");

    free_bstrs();
}

static void test_validate_on_load(void)
{
    IXMLDOMSchemaCollection2 *cache;
    VARIANT_BOOL b;
    HRESULT hr;

    cache = create_cache_version(40, &IID_IXMLDOMSchemaCollection2);
    if (!cache) return;

    hr = IXMLDOMSchemaCollection2_get_validateOnLoad(cache, NULL);
    EXPECT_HR(hr, E_POINTER);

    b = VARIANT_FALSE;
    hr = IXMLDOMSchemaCollection2_get_validateOnLoad(cache, &b);
    EXPECT_HR(hr, S_OK);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    IXMLDOMSchemaCollection2_Release(cache);
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
    test_XDR_schemas();
    test_XDR_datatypes();
    test_validate_on_load();

    CoUninitialize();
}
