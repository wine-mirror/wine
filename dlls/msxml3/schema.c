/*
 * Schema cache implementation
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

#define COBJMACROS

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

/* We use a chained hashtable, which can hold any number of schemas
 * TODO: versioned constructor
 * TODO: grow/shrink hashtable depending on load factor
 * TODO: implement read-only where appropriate
 */

/* This is just the number of buckets, should be prime */
#define DEFAULT_HASHTABLE_SIZE 17

#ifdef HAVE_LIBXML2

#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/schemasInternals.h>
#include <libxml/hash.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlIO.h>

xmlDocPtr XDR_to_XSD_doc(xmlDocPtr xdr_doc, xmlChar const* nsURI);

static const xmlChar XSD_schema[] = "schema";
static const xmlChar XSD_nsURI[] = "http://www.w3.org/2001/XMLSchema";
static const xmlChar XDR_schema[] = "Schema";
static const xmlChar XDR_nsURI[] = "urn:schemas-microsoft-com:xml-data";
static const xmlChar DT_nsURI[] = "urn:schemas-microsoft-com:datatypes";

static xmlChar const* datatypes_schema = NULL;
static HGLOBAL datatypes_handle = NULL;
static HRSRC datatypes_rsrc = NULL;

/* Supported Types:
 * msxml3 - XDR only
 * msxml4 - XDR & XSD
 * msxml5 - XDR & XSD
 * mxsml6 - XSD only
 */
typedef enum _SCHEMA_TYPE {
    SCHEMA_TYPE_INVALID,
    SCHEMA_TYPE_XDR,
    SCHEMA_TYPE_XSD
} SCHEMA_TYPE;

typedef struct _schema_cache
{
    const struct IXMLDOMSchemaCollection2Vtbl* lpVtbl;
    xmlHashTablePtr cache;
    LONG ref;
} schema_cache;

typedef struct _cache_entry
{
    SCHEMA_TYPE type;
    xmlSchemaPtr schema;
    xmlDocPtr doc;
    LONG ref;
} cache_entry;

typedef struct _cache_index_data
{
    LONG index;
    BSTR* out;
} cache_index_data;

/* datatypes lookup stuff
 * generated with help from gperf */
#define DT_MIN_STR_LEN 2
#define DT_MAX_STR_LEN 11
#define DT_MIN_HASH_VALUE 2
#define DT_MAX_HASH_VALUE 115

static const xmlChar DT_bin_base64[] = "bin.base64";
static const xmlChar DT_bin_hex[] = "bin.hex";
static const xmlChar DT_boolean[] = "boolean";
static const xmlChar DT_char[] = "char";
static const xmlChar DT_date[] = "date";
static const xmlChar DT_date_tz[] = "date.tz";
static const xmlChar DT_dateTime[] = "dateTime";
static const xmlChar DT_dateTime_tz[] = "dateTime.tz";
static const xmlChar DT_entity[] = "entity";
static const xmlChar DT_entities[] = "entities";
static const xmlChar DT_enumeration[] = "enumeration";
static const xmlChar DT_fixed_14_4[] = "fixed.14.4";
static const xmlChar DT_float[] = "float";
static const xmlChar DT_i1[] = "i1";
static const xmlChar DT_i2[] = "i2";
static const xmlChar DT_i4[] = "i4";
static const xmlChar DT_i8[] = "i8";
static const xmlChar DT_id[] = "id";
static const xmlChar DT_idref[] = "idref";
static const xmlChar DT_idrefs[] = "idrefs";
static const xmlChar DT_int[] = "int";
static const xmlChar DT_nmtoken[] = "nmtoken";
static const xmlChar DT_nmtokens[] = "nmtokens";
static const xmlChar DT_notation[] = "notation";
static const xmlChar DT_number[] = "number";
static const xmlChar DT_r4[] = "r4";
static const xmlChar DT_r8[] = "r8";
static const xmlChar DT_string[] = "string";
static const xmlChar DT_time[] = "time";
static const xmlChar DT_time_tz[] = "time.tz";
static const xmlChar DT_ui1[] = "ui1";
static const xmlChar DT_ui2[] = "ui2";
static const xmlChar DT_ui4[] = "ui4";
static const xmlChar DT_ui8[] = "ui8";
static const xmlChar DT_uri[] = "uri";
static const xmlChar DT_uuid[] = "uuid";

static DWORD dt_hash( xmlChar const* str, int len /* calculated if -1 */)
{
    static const BYTE assoc_values[] =
    {
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116,  10, 116, 116,  55,
         45, 116,   5, 116,   0, 116,   0, 116, 116, 116,
        116, 116, 116, 116, 116,   5,   0,   0,  20,   0,
          0,  10,   0,   0, 116,   0,   0,   0,  15,   5,
        116, 116,  10,   0,   0,   0, 116, 116,   0,   0,
         10, 116, 116, 116, 116, 116, 116,   5,   0,   0,
         20,   0,   0,  10,   0,   0, 116,   0,   0,   0,
         15,   5, 116, 116,  10,   0,   0,   0, 116, 116,
          0,   0,  10, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 116, 116, 116
    };
    DWORD hval = (len == -1)? xmlStrlen(str) : len;

    switch (hval)
    {
        default:
            hval += assoc_values[str[10]];
            /*FALLTHROUGH*/
        case 10:
            hval += assoc_values[str[9]];
            /*FALLTHROUGH*/
        case 9:
            hval += assoc_values[str[8]];
            /*FALLTHROUGH*/
        case 8:
            hval += assoc_values[str[7]];
            /*FALLTHROUGH*/
        case 7:
            hval += assoc_values[str[6]];
            /*FALLTHROUGH*/
        case 6:
            hval += assoc_values[str[5]];
            /*FALLTHROUGH*/
        case 5:
            hval += assoc_values[str[4]];
            /*FALLTHROUGH*/
        case 4:
            hval += assoc_values[str[3]];
            /*FALLTHROUGH*/
        case 3:
            hval += assoc_values[str[2]];
            /*FALLTHROUGH*/
        case 2:
            hval += assoc_values[str[1]];
            /*FALLTHROUGH*/
        case 1:
            hval += assoc_values[str[0]];
            break;
    }
    return hval;
}

static const xmlChar const* DT_string_table[DT__N_TYPES] =
{
    DT_bin_base64,
    DT_bin_hex,
    DT_boolean,
    DT_char,
    DT_date,
    DT_date_tz,
    DT_dateTime,
    DT_dateTime_tz,
    DT_entity,
    DT_entities,
    DT_enumeration,
    DT_fixed_14_4,
    DT_float,
    DT_i1,
    DT_i2,
    DT_i4,
    DT_i8,
    DT_id,
    DT_idref,
    DT_idrefs,
    DT_int,
    DT_nmtoken,
    DT_nmtokens,
    DT_notation,
    DT_number,
    DT_r4,
    DT_r8,
    DT_string,
    DT_time,
    DT_time_tz,
    DT_ui1,
    DT_ui2,
    DT_ui4,
    DT_ui8,
    DT_uri,
    DT_uuid
};

static const XDR_DT DT_lookup_table[] =
{
    -1, -1,
    DT_I8,
    DT_UI8,
    DT_TIME,
    -1, -1,
    DT_I4,
    DT_UI4,
    -1, -1, -1,
    DT_R8,
    DT_URI,
    -1,
    DT_FLOAT,
    -1,
    DT_R4,
    DT_INT,
    DT_CHAR,
    -1,
    DT_ENTITY,
    DT_ID,
    DT_ENTITIES,
    DT_UUID,
    -1, -1,
    DT_TIME_TZ,
    -1,
    DT_DATE,
    -1,
    DT_NUMBER,
    DT_BIN_HEX,
    DT_DATETIME,
    -1,
    DT_IDREF,
    DT_IDREFS,
    DT_BOOLEAN,
    -1, -1, -1,
    DT_STRING,
    DT_NMTOKEN,
    DT_NMTOKENS,
    -1,
    DT_BIN_BASE64,
    -1,
    DT_I2,
    DT_UI2,
    -1, -1, -1,
    DT_DATE_TZ,
    DT_NOTATION,
    -1, -1,
    DT_DATETIME_TZ,
    DT_I1,
    DT_UI1,
    -1, -1,
    DT_ENUMERATION,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    DT_FIXED_14_4
};

XDR_DT dt_get_type(xmlChar const* str, int len /* calculated if -1 */)
{
    DWORD hash = dt_hash(str, len);
    XDR_DT dt = DT_INVALID;

    if (hash <= DT_MAX_HASH_VALUE)
        dt = DT_lookup_table[hash];

    if (dt != DT_INVALID && xmlStrcasecmp(str, DT_string_table[dt]) == 0)
        return dt;

    return DT_INVALID;
}

xmlChar const* dt_get_str(XDR_DT dt)
{
    if (dt == DT_INVALID)
        return NULL;

    return DT_string_table[dt];
}

static inline xmlChar const* get_node_nsURI(xmlNodePtr node)
{
    return (node->ns != NULL)? node->ns->href : NULL;
}

static inline cache_entry* get_entry(schema_cache* This, xmlChar const* nsURI)
{
    return (!nsURI)? xmlHashLookup(This->cache, BAD_CAST "") :
                     xmlHashLookup(This->cache, nsURI);
}

static inline xmlSchemaPtr get_node_schema(schema_cache* This, xmlNodePtr node)
{
    cache_entry* entry = get_entry(This, get_node_nsURI(node));
    return (!entry)? NULL : entry->schema;
}

xmlExternalEntityLoader _external_entity_loader = NULL;

static xmlParserInputPtr external_entity_loader(const char *URL, const char *ID,
                                                xmlParserCtxtPtr ctxt)
{
    xmlParserInputPtr input;

    TRACE("(%s, %s, %p)\n", wine_dbgstr_a(URL), wine_dbgstr_a(ID), ctxt);

    assert(MSXML_hInstance != NULL);
    assert(datatypes_rsrc != NULL);
    assert(datatypes_handle != NULL);
    assert(datatypes_schema != NULL);

    /* TODO: if the desired schema is in the cache, load it from there */
    if (lstrcmpA(URL, "urn:schemas-microsoft-com:datatypes") == 0)
    {
        TRACE("loading built-in schema for %s\n", URL);
        input = xmlNewStringInputStream(ctxt, datatypes_schema);
    }
    else
    {
        input = _external_entity_loader(URL, ID, ctxt);
    }

    return input;
}

void schemasInit(void)
{
    int len;
    char* buf;
    if (!(datatypes_rsrc = FindResourceA(MSXML_hInstance, "DATATYPES", "XML")))
    {
        FIXME("failed to find resource for %s\n", DT_nsURI);
        return;
    }

    if (!(datatypes_handle = LoadResource(MSXML_hInstance, datatypes_rsrc)))
    {
        FIXME("failed to load resource for %s\n", DT_nsURI);
        return;
    }
    buf = LockResource(datatypes_handle);
    len = SizeofResource(MSXML_hInstance, datatypes_rsrc) - 1;

    /* Resource is loaded as raw data,
     * need a null-terminated string */
    while (buf[len] != '>')
        buf[len--] = 0;
    datatypes_schema = BAD_CAST buf;

    if ((void*)xmlGetExternalEntityLoader() != (void*)external_entity_loader)
    {
        _external_entity_loader = xmlGetExternalEntityLoader();
        xmlSetExternalEntityLoader(external_entity_loader);
    }
}

void schemasCleanup(void)
{
    if (datatypes_handle)
        FreeResource(datatypes_handle);
    xmlSetExternalEntityLoader(_external_entity_loader);
}

static LONG cache_entry_add_ref(cache_entry* entry)
{
    LONG ref = InterlockedIncrement(&entry->ref);
    TRACE("%p new ref %d\n", entry, ref);
    return ref;
}

static LONG cache_entry_release(cache_entry* entry)
{
    LONG ref = InterlockedDecrement(&entry->ref);
    TRACE("%p new ref %d\n", entry, ref);

    if (ref == 0)
    {
        if (entry->type == SCHEMA_TYPE_XSD)
        {
            xmldoc_release(entry->doc);
            entry->schema->doc = NULL;
            xmlSchemaFree(entry->schema);
            heap_free(entry);
        }
        else /* SCHEMA_TYPE_XDR */
        {
            xmldoc_release(entry->doc);
            xmldoc_release(entry->schema->doc);
            entry->schema->doc = NULL;
            xmlSchemaFree(entry->schema);
            heap_free(entry);
        }
    }
    return ref;
}

static inline schema_cache* impl_from_IXMLDOMSchemaCollection2(IXMLDOMSchemaCollection2* iface)
{
    return (schema_cache*)((char*)iface - FIELD_OFFSET(schema_cache, lpVtbl));
}

static inline SCHEMA_TYPE schema_type_from_xmlDocPtr(xmlDocPtr schema)
{
    xmlNodePtr root;
    if (schema)
        root = xmlDocGetRootElement(schema);
    if (root && root->ns)
    {

        if (xmlStrEqual(root->name, XDR_schema) &&
            xmlStrEqual(root->ns->href, XDR_nsURI))
        {
            return SCHEMA_TYPE_XDR;
        }
        else if (xmlStrEqual(root->name, XSD_schema) &&
                 xmlStrEqual(root->ns->href, XSD_nsURI))
        {
            return SCHEMA_TYPE_XSD;
        }
    }
    return SCHEMA_TYPE_INVALID;
}

static BOOL link_datatypes(xmlDocPtr schema)
{
    xmlNodePtr root, next, child;
    xmlNsPtr ns;

    assert((void*)xmlGetExternalEntityLoader() == (void*)external_entity_loader);
    root = xmlDocGetRootElement(schema);
    if (!root)
        return FALSE;

    for (ns = root->nsDef; ns != NULL; ns = ns->next)
    {
        if (xmlStrEqual(ns->href, DT_nsURI))
            break;
    }

    if (!ns)
        return FALSE;

    next = xmlFirstElementChild(root);
    child = xmlNewChild(root, NULL, BAD_CAST "import", NULL);
    if (next) child = xmlAddPrevSibling(next, child);
    xmlSetProp(child, BAD_CAST "namespace", DT_nsURI);
    xmlSetProp(child, BAD_CAST "schemaLocation", DT_nsURI);

    return TRUE;
}

static cache_entry* cache_entry_from_url(char const* url, xmlChar const* nsURI)
{
    cache_entry* entry = heap_alloc(sizeof(cache_entry));
    xmlSchemaParserCtxtPtr spctx = xmlSchemaNewParserCtxt(url);
    entry->type = SCHEMA_TYPE_XSD;
    entry->ref = 0;
    if (spctx)
    {
        if((entry->schema = xmlSchemaParse(spctx)))
        {
            /* TODO: if the nsURI is different from the default xmlns or targetNamespace,
             *       do we need to do something special here? */
            xmldoc_init(entry->schema->doc, &CLSID_DOMDocument40);
            entry->doc = entry->schema->doc;
            xmldoc_add_ref(entry->doc);
        }
        else
        {
            heap_free(entry);
            entry = NULL;
        }
        xmlSchemaFreeParserCtxt(spctx);
    }
    else
    {
        FIXME("schema for nsURI %s not found\n", wine_dbgstr_a(url));
        heap_free(entry);
        entry = NULL;
    }
    return entry;
}

static cache_entry* cache_entry_from_xsd_doc(xmlDocPtr doc, xmlChar const* nsURI)
{
    cache_entry* entry = heap_alloc(sizeof(cache_entry));
    xmlSchemaParserCtxtPtr spctx;
    xmlDocPtr new_doc = xmlCopyDoc(doc, 1);

    link_datatypes(new_doc);

    /* TODO: if the nsURI is different from the default xmlns or targetNamespace,
     *       do we need to do something special here? */
    entry->type = SCHEMA_TYPE_XSD;
    entry->ref = 0;
    spctx = xmlSchemaNewDocParserCtxt(new_doc);

    if ((entry->schema = xmlSchemaParse(spctx)))
    {
        xmldoc_init(entry->schema->doc, &CLSID_DOMDocument40);
        entry->doc = entry->schema->doc;
        xmldoc_add_ref(entry->doc);
    }
    else
    {
        FIXME("failed to parse doc\n");
        xmlFreeDoc(new_doc);
        heap_free(entry);
        entry = NULL;
    }
    xmlSchemaFreeParserCtxt(spctx);
    return entry;
}

static cache_entry* cache_entry_from_xdr_doc(xmlDocPtr doc, xmlChar const* nsURI)
{
    cache_entry* entry = heap_alloc(sizeof(cache_entry));
    xmlSchemaParserCtxtPtr spctx;
    xmlDocPtr new_doc = xmlCopyDoc(doc, 1), xsd_doc = XDR_to_XSD_doc(doc, nsURI);

    link_datatypes(xsd_doc);

    entry->type = SCHEMA_TYPE_XDR;
    entry->ref = 0;
    spctx = xmlSchemaNewDocParserCtxt(xsd_doc);

    if ((entry->schema = xmlSchemaParse(spctx)))
    {
        entry->doc = new_doc;
        xmldoc_init(entry->schema->doc, &CLSID_DOMDocument30);
        xmldoc_init(entry->doc, &CLSID_DOMDocument30);
        xmldoc_add_ref(entry->doc);
        xmldoc_add_ref(entry->schema->doc);
    }
    else
    {
        FIXME("failed to parse doc\n");
        xmlFreeDoc(new_doc);
        xmlFreeDoc(xsd_doc);
        heap_free(entry);
        entry = NULL;
    }
    xmlSchemaFreeParserCtxt(spctx);

    return entry;
}

static HRESULT WINAPI schema_cache_QueryInterface(IXMLDOMSchemaCollection2* iface,
                                                  REFIID riid, void** ppvObject)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualIID(riid, &IID_IUnknown) ||
         IsEqualIID(riid, &IID_IDispatch) ||
         IsEqualIID(riid, &IID_IXMLDOMSchemaCollection) ||
         IsEqualIID(riid, &IID_IXMLDOMSchemaCollection2) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMSchemaCollection2_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI schema_cache_AddRef(IXMLDOMSchemaCollection2* iface)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p new ref %d\n", This, ref);
    return ref;
}

static void cache_free(void* data, xmlChar* name /* ignored */)
{
    cache_entry_release((cache_entry*)data);
}

static ULONG WINAPI schema_cache_Release(IXMLDOMSchemaCollection2* iface)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p new ref %d\n", This, ref);

    if (ref == 0)
    {
        xmlHashFree(This->cache, cache_free);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI schema_cache_GetTypeInfoCount(IXMLDOMSchemaCollection2* iface,
                                                    UINT* pctinfo)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI schema_cache_GetTypeInfo(IXMLDOMSchemaCollection2* iface,
                                               UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI schema_cache_GetIDsOfNames(IXMLDOMSchemaCollection2* iface,
                                                 REFIID riid, LPOLESTR* rgszNames,
                                                 UINT cNames, LCID lcid, DISPID* rgDispId)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    ITypeInfo* typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI schema_cache_Invoke(IXMLDOMSchemaCollection2* iface,
                                          DISPID dispIdMember, REFIID riid, LCID lcid,
                                          WORD wFlags, DISPPARAMS* pDispParams,
                                          VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                                          UINT* puArgErr)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    ITypeInfo* typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI schema_cache_add(IXMLDOMSchemaCollection2* iface, BSTR uri, VARIANT var)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    xmlChar* name = xmlChar_from_wchar(uri);
    TRACE("(%p)->(%s, var(vt %x))\n", This, debugstr_w(uri), V_VT(&var));

    switch (V_VT(&var))
    {
        case VT_NULL:
            {
                xmlHashRemoveEntry(This->cache, name, cache_free);
            }
            break;

        case VT_BSTR:
            {
                xmlChar* url = xmlChar_from_wchar(V_BSTR(&var));
                cache_entry* entry = cache_entry_from_url((char const*)url, name);
                heap_free(url);

                if (entry)
                {
                    cache_entry_add_ref(entry);
                }
                else
                {
                    heap_free(name);
                    return E_FAIL;
                }

                xmlHashRemoveEntry(This->cache, name, cache_free);
                xmlHashAddEntry(This->cache, name, entry);
            }
            break;

        case VT_DISPATCH:
            {
                xmlDocPtr doc = NULL;
                cache_entry* entry;
                SCHEMA_TYPE type;
                IXMLDOMNode* domnode = NULL;
                IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IXMLDOMNode, (void**)&domnode);

                if (domnode)
                    doc = xmlNodePtr_from_domnode(domnode, XML_DOCUMENT_NODE)->doc;

                if (!doc)
                {
                    IXMLDOMNode_Release(domnode);
                    heap_free(name);
                    return E_INVALIDARG;
                }
                type = schema_type_from_xmlDocPtr(doc);

                if (type == SCHEMA_TYPE_XSD)
                {
                    entry = cache_entry_from_xsd_doc(doc, name);
                }
                else if (type == SCHEMA_TYPE_XDR)
                {
                    entry = cache_entry_from_xdr_doc(doc, name);
                }
                else
                {
                    WARN("invalid schema!\n");
                    entry = NULL;
                }

                IXMLDOMNode_Release(domnode);

                if (entry)
                {
                    cache_entry_add_ref(entry);
                }
                else
                {
                    heap_free(name);
                    return E_FAIL;
                }

                xmlHashRemoveEntry(This->cache, name, cache_free);
                xmlHashAddEntry(This->cache, name, entry);
            }
            break;

        default:
            {
                heap_free(name);
                return E_INVALIDARG;
            }
    }
    heap_free(name);
    return S_OK;
}

static HRESULT WINAPI schema_cache_get(IXMLDOMSchemaCollection2* iface, BSTR uri,
                                       IXMLDOMNode** node)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    xmlChar* name;
    cache_entry* entry;
    TRACE("(%p)->(%s, %p)\n", This, wine_dbgstr_w(uri), node);

    if (!node)
        return E_POINTER;

    name = xmlChar_from_wchar(uri);
    entry = (cache_entry*) xmlHashLookup(This->cache, name);
    heap_free(name);

    /* TODO: this should be read-only */
    if (entry)
        return DOMDocument_create_from_xmldoc(entry->doc, (IXMLDOMDocument3**)node);

    *node = NULL;
    return S_OK;
}

static HRESULT WINAPI schema_cache_remove(IXMLDOMSchemaCollection2* iface, BSTR uri)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    xmlChar* name = xmlChar_from_wchar(uri);
    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(uri));

    xmlHashRemoveEntry(This->cache, name, cache_free);
    heap_free(name);
    return S_OK;
}

static HRESULT WINAPI schema_cache_get_length(IXMLDOMSchemaCollection2* iface, LONG* length)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    TRACE("(%p)->(%p)\n", This, length);

    if (!length)
        return E_POINTER;
    *length = xmlHashSize(This->cache);
    return S_OK;
}

static void cache_index(void* data /* ignored */, void* index, xmlChar* name)
{
    cache_index_data* index_data = (cache_index_data*)index;

    if (index_data->index-- == 0)
        *index_data->out = bstr_from_xmlChar(name);
}

static HRESULT WINAPI schema_cache_get_namespaceURI(IXMLDOMSchemaCollection2* iface,
                                                    LONG index, BSTR* len)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    cache_index_data data = {index,len};
    TRACE("(%p)->(%i, %p)\n", This, index, len);

    if (!len)
        return E_POINTER;
    *len = NULL;

    if (index >= xmlHashSize(This->cache))
        return E_FAIL;

    xmlHashScan(This->cache, cache_index, &data);
    return S_OK;
}

static void cache_copy(void* data, void* dest, xmlChar* name)
{
    schema_cache* This = (schema_cache*) dest;
    cache_entry* entry = (cache_entry*) data;

    if (xmlHashLookup(This->cache, name) == NULL)
    {
        cache_entry_add_ref(entry);
        xmlHashAddEntry(This->cache, name, entry);
    }
}

static HRESULT WINAPI schema_cache_addCollection(IXMLDOMSchemaCollection2* iface,
                                                 IXMLDOMSchemaCollection* otherCollection)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    schema_cache* That = impl_from_IXMLDOMSchemaCollection2((IXMLDOMSchemaCollection2*)otherCollection);
    TRACE("(%p)->(%p)\n", This, That);

    if (!otherCollection)
        return E_POINTER;

    /* TODO: detect errors while copying & return E_FAIL */
    xmlHashScan(That->cache, cache_copy, This);

    return S_OK;
}

static HRESULT WINAPI schema_cache_get__newEnum(IXMLDOMSchemaCollection2* iface,
                                                IUnknown** ppUnk)
{
    FIXME("stub\n");
    if (ppUnk)
        *ppUnk = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_validate(IXMLDOMSchemaCollection2* iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_put_validateOnLoad(IXMLDOMSchemaCollection2* iface,
                                                      VARIANT_BOOL validateOnLoad)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_get_validateOnLoad(IXMLDOMSchemaCollection2* iface,
                                                      VARIANT_BOOL* validateOnLoad)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_getSchema(IXMLDOMSchemaCollection2* iface,
                                             BSTR namespaceURI, ISchema** schema)
{
    FIXME("stub\n");
    if (schema)
        *schema = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_getDeclaration(IXMLDOMSchemaCollection2* iface,
                                                  IXMLDOMNode* node, ISchemaItem** item)
{
    FIXME("stub\n");
    if (item)
        *item = NULL;
    return E_NOTIMPL;
}

static const struct IXMLDOMSchemaCollection2Vtbl schema_cache_vtbl =
{
    schema_cache_QueryInterface,
    schema_cache_AddRef,
    schema_cache_Release,
    schema_cache_GetTypeInfoCount,
    schema_cache_GetTypeInfo,
    schema_cache_GetIDsOfNames,
    schema_cache_Invoke,
    schema_cache_add,
    schema_cache_get,
    schema_cache_remove,
    schema_cache_get_length,
    schema_cache_get_namespaceURI,
    schema_cache_addCollection,
    schema_cache_get__newEnum,
    schema_cache_validate,
    schema_cache_put_validateOnLoad,
    schema_cache_get_validateOnLoad,
    schema_cache_getSchema,
    schema_cache_getDeclaration
};

static xmlSchemaElementPtr lookup_schema_elemDecl(xmlSchemaPtr schema, xmlNodePtr node)
{
    xmlSchemaElementPtr decl = NULL;
    xmlChar const* nsURI = get_node_nsURI(node);

    TRACE("(%p, %p)\n", schema, node);

    if (xmlStrEqual(nsURI, schema->targetNamespace))
        decl = xmlHashLookup(schema->elemDecl, node->name);

    if (!decl && xmlHashSize(schema->schemasImports) > 1)
    {
        FIXME("declaration not found in main schema - need to check schema imports!\n");
        /*xmlSchemaImportPtr import;
        if (nsURI == NULL)
            import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
        else
            import = xmlHashLookup(schema->schemasImports, node->ns->href);

        if (import != NULL)
            decl = xmlHashLookup(import->schema->elemDecl, node->name);*/
    }

    return decl;
}

static inline xmlNodePtr lookup_schema_element(xmlSchemaPtr schema, xmlNodePtr node)
{
    xmlSchemaElementPtr decl = lookup_schema_elemDecl(schema, node);
    while (decl != NULL && decl->refDecl != NULL)
        decl = decl->refDecl;
    return (decl != NULL)? decl->node : NULL;
}

static void LIBXML2_LOG_CALLBACK validate_error(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_ERR(SchemaCache_validate_tree, msg, ap);
    va_end(ap);
}

static void LIBXML2_LOG_CALLBACK validate_warning(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_WARN(SchemaCache_validate_tree, msg, ap);
    va_end(ap);
}

#ifdef HAVE_XMLSCHEMASSETVALIDSTRUCTUREDERRORS
static void validate_serror(void* ctx, xmlErrorPtr err)
{
    LIBXML2_CALLBACK_SERROR(SchemaCache_validate_tree, err);
}
#endif

HRESULT SchemaCache_validate_tree(IXMLDOMSchemaCollection2* iface, xmlNodePtr tree)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    xmlSchemaPtr schema;

    TRACE("(%p, %p)\n", This, tree);

    if (!tree)
        return E_POINTER;

    if (tree->type == XML_DOCUMENT_NODE)
        tree = xmlDocGetRootElement(tree->doc);

    schema = get_node_schema(This, tree);
    /* TODO: if the ns is not in the cache, and it's a URL,
     *       do we try to load from that? */
    if (schema)
    {
        xmlSchemaValidCtxtPtr svctx;
        int err;
        /* TODO: if validateOnLoad property is false,
         *       we probably need to validate the schema here. */
        svctx = xmlSchemaNewValidCtxt(schema);
        xmlSchemaSetValidErrors(svctx, validate_error, validate_warning, NULL);
#ifdef HAVE_XMLSCHEMASSETVALIDSTRUCTUREDERRORS
            xmlSchemaSetValidStructuredErrors(svctx, validate_serror, NULL);
#endif

        if ((xmlNodePtr)tree->doc == tree)
            err = xmlSchemaValidateDoc(svctx, (xmlDocPtr)tree);
        else
            err = xmlSchemaValidateOneElement(svctx, tree);

        xmlSchemaFreeValidCtxt(svctx);
        return err? S_FALSE : S_OK;
    }

    return E_FAIL;
}

XDR_DT SchemaCache_get_node_dt(IXMLDOMSchemaCollection2* iface, xmlNodePtr node)
{
    schema_cache* This = impl_from_IXMLDOMSchemaCollection2(iface);
    xmlSchemaPtr schema = get_node_schema(This, node);
    XDR_DT dt = DT_INVALID;

    TRACE("(%p, %p)\n", This, node);

    if (node->ns && xmlStrEqual(node->ns->href, DT_nsURI))
    {
        dt = dt_get_type(node->name, -1);
    }
    else if (schema)
    {
        xmlChar* str;
        xmlNodePtr schema_node = lookup_schema_element(schema, node);

        str = xmlGetNsProp(schema_node, BAD_CAST "dt", DT_nsURI);
        if (str)
        {
            dt = dt_get_type(str, -1);
            xmlFree(str);
        }
    }

    return dt;
}

HRESULT SchemaCache_create(IUnknown* pUnkOuter, void** ppObj)
{
    schema_cache* This = heap_alloc(sizeof(schema_cache));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &schema_cache_vtbl;
    This->cache = xmlHashCreate(DEFAULT_HASHTABLE_SIZE);
    This->ref = 1;

    *ppObj = &This->lpVtbl;
    return S_OK;
}

#else

HRESULT SchemaCache_create(IUnknown* pUnkOuter, void** ppObj)
{
    MESSAGE("This program tried to use a SchemaCache object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
