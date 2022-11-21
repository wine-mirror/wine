/*
 *    Common definitions
 *
 * Copyright 2005 Mike McCormack
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

#ifndef __MSXML_PRIVATE__
#define __MSXML_PRIVATE__

#include "dispex.h"

#include "wine/heap.h"
#include "wine/list.h"

#include "msxml_dispex.h"

extern const CLSID * DOMDocument_version(MSXML_VERSION v) DECLSPEC_HIDDEN;


/* The XDR datatypes (urn:schemas-microsoft-com:datatypes)
 * These are actually valid for XSD schemas as well
 * See datatypes.xsd
 */
typedef enum _XDR_DT {
    DT_INVALID = -1,
    DT_BIN_BASE64,
    DT_BIN_HEX,
    DT_BOOLEAN,
    DT_CHAR,
    DT_DATE,
    DT_DATE_TZ,
    DT_DATETIME,
    DT_DATETIME_TZ,
    DT_ENTITY,
    DT_ENTITIES,
    DT_ENUMERATION,
    DT_FIXED_14_4,
    DT_FLOAT,
    DT_I1,
    DT_I2,
    DT_I4,
    DT_I8,
    DT_ID,
    DT_IDREF,
    DT_IDREFS,
    DT_INT,
    DT_NMTOKEN,
    DT_NMTOKENS,
    DT_NOTATION,
    DT_NUMBER,
    DT_R4,
    DT_R8,
    DT_STRING,
    DT_TIME,
    DT_TIME_TZ,
    DT_UI1,
    DT_UI2,
    DT_UI4,
    DT_UI8,
    DT_URI,
    DT_UUID,
    LAST_DT
} XDR_DT;

extern HINSTANCE MSXML_hInstance DECLSPEC_HIDDEN;

/* XSLProcessor parameter list */
struct xslprocessor_par
{
    struct list entry;
    BSTR name;
    BSTR value;
};

struct xslprocessor_params
{
    struct list  list;
    unsigned int count;
};

extern void schemasInit(void) DECLSPEC_HIDDEN;
extern void schemasCleanup(void) DECLSPEC_HIDDEN;

/* IXMLDOMNode Internal Structure */
typedef struct _xmlnode
{
    DispatchEx   dispex;
    IXMLDOMNode *iface;
    IXMLDOMNode *parent;
    xmlNodePtr   node;
} xmlnode;

/* IXMLDOMNamedNodeMap custom function table */
struct nodemap_funcs
{
    HRESULT (*get_named_item)(const xmlNodePtr,BSTR,IXMLDOMNode**);
    HRESULT (*set_named_item)(xmlNodePtr,IXMLDOMNode*,IXMLDOMNode**);
    HRESULT (*remove_named_item)(xmlNodePtr,BSTR,IXMLDOMNode**);
    HRESULT (*get_item)(xmlNodePtr,LONG,IXMLDOMNode**);
    HRESULT (*get_length)(xmlNodePtr,LONG*);
    HRESULT (*get_qualified_item)(const xmlNodePtr,BSTR,BSTR,IXMLDOMNode**);
    HRESULT (*remove_qualified_item)(xmlNodePtr,BSTR,BSTR,IXMLDOMNode**);
    HRESULT (*next_node)(const xmlNodePtr,LONG*,IXMLDOMNode**);
};

/* used by IEnumVARIANT to access outer object items */
struct enumvariant_funcs
{
    HRESULT (*get_item)(IUnknown*, LONG, VARIANT*);
    HRESULT (*next)(IUnknown*);
};

/* constructors */
extern IUnknown         *create_domdoc( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_xmldoc( void ) DECLSPEC_HIDDEN;
extern IXMLDOMNode      *create_node( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_element( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_attribute( xmlNodePtr, BOOL ) DECLSPEC_HIDDEN;
extern IUnknown         *create_text( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_pi( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_comment( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_cdata( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IXMLDOMNodeList  *create_children_nodelist( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IXMLDOMNamedNodeMap *create_nodemap( xmlNodePtr, const struct nodemap_funcs* ) DECLSPEC_HIDDEN;
extern IUnknown         *create_doc_fragment( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_doc_entity_ref( xmlNodePtr ) DECLSPEC_HIDDEN;
extern IUnknown         *create_doc_type( xmlNodePtr ) DECLSPEC_HIDDEN;
extern HRESULT           create_selection( xmlNodePtr, xmlChar*, IXMLDOMNodeList** ) DECLSPEC_HIDDEN;
extern HRESULT           create_enumvariant( IUnknown*, BOOL, const struct enumvariant_funcs*, IEnumVARIANT**) DECLSPEC_HIDDEN;
extern HRESULT           create_dom_implementation(IXMLDOMImplementation **obj) DECLSPEC_HIDDEN;

/* data accessors */
xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type ) DECLSPEC_HIDDEN;

/* helpers */
extern xmlChar *xmlChar_from_wchar( LPCWSTR str ) DECLSPEC_HIDDEN;

extern void xmldoc_init( xmlDocPtr doc, MSXML_VERSION version ) DECLSPEC_HIDDEN;
extern LONG xmldoc_add_ref( xmlDocPtr doc ) DECLSPEC_HIDDEN;
extern LONG xmldoc_release( xmlDocPtr doc ) DECLSPEC_HIDDEN;
extern LONG xmldoc_add_refs( xmlDocPtr doc, LONG refs ) DECLSPEC_HIDDEN;
extern LONG xmldoc_release_refs ( xmlDocPtr doc, LONG refs ) DECLSPEC_HIDDEN;
extern void xmlnode_add_ref(xmlNodePtr node) DECLSPEC_HIDDEN;
extern void xmlnode_release(xmlNodePtr node) DECLSPEC_HIDDEN;
extern int xmlnode_get_inst_cnt( xmlnode *node ) DECLSPEC_HIDDEN;
extern HRESULT xmldoc_add_orphan( xmlDocPtr doc, xmlNodePtr node ) DECLSPEC_HIDDEN;
extern HRESULT xmldoc_remove_orphan( xmlDocPtr doc, xmlNodePtr node ) DECLSPEC_HIDDEN;
extern void xmldoc_link_xmldecl(xmlDocPtr doc, xmlNodePtr node) DECLSPEC_HIDDEN;
extern xmlNodePtr xmldoc_unlink_xmldecl(xmlDocPtr doc) DECLSPEC_HIDDEN;
extern MSXML_VERSION xmldoc_version( xmlDocPtr doc ) DECLSPEC_HIDDEN;

extern HRESULT XMLElement_create( xmlNodePtr node, LPVOID *ppObj, BOOL own ) DECLSPEC_HIDDEN;

extern void wineXmlCallbackLog(char const* caller, xmlErrorLevel lvl, char const* msg, va_list ap) DECLSPEC_HIDDEN;
extern void wineXmlCallbackError(char const* caller, xmlErrorPtr err) DECLSPEC_HIDDEN;

#define LIBXML2_LOG_CALLBACK WINAPIV __WINE_PRINTF_ATTR(2,3)

#define LIBXML2_CALLBACK_TRACE(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_NONE, msg, ap)

#define LIBXML2_CALLBACK_WARN(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_WARNING, msg, ap)

#define LIBXML2_CALLBACK_ERR(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_ERROR, msg, ap)

#define LIBXML2_CALLBACK_SERROR(caller, err) wineXmlCallbackError(#caller, err)

extern BOOL is_preserving_whitespace(xmlNodePtr node) DECLSPEC_HIDDEN;
extern BOOL is_xpathmode(const xmlDocPtr doc) DECLSPEC_HIDDEN;
extern void set_xpathmode(xmlDocPtr doc, BOOL xpath) DECLSPEC_HIDDEN;

extern void init_xmlnode(xmlnode*,xmlNodePtr,IXMLDOMNode*,dispex_static_data_t*) DECLSPEC_HIDDEN;
extern void destroy_xmlnode(xmlnode*) DECLSPEC_HIDDEN;
extern BOOL node_query_interface(xmlnode*,REFIID,void**) DECLSPEC_HIDDEN;
extern xmlnode *get_node_obj(IXMLDOMNode*) DECLSPEC_HIDDEN;

extern HRESULT node_append_child(xmlnode*,IXMLDOMNode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_nodeName(xmlnode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_get_content(xmlnode*,VARIANT*) DECLSPEC_HIDDEN;
extern HRESULT node_set_content(xmlnode*,LPCWSTR) DECLSPEC_HIDDEN;
extern HRESULT node_put_value(xmlnode*,VARIANT*) DECLSPEC_HIDDEN;
extern HRESULT node_put_value_escaped(xmlnode*,VARIANT*) DECLSPEC_HIDDEN;
extern HRESULT node_get_parent(xmlnode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_child_nodes(xmlnode*,IXMLDOMNodeList**) DECLSPEC_HIDDEN;
extern HRESULT node_get_first_child(xmlnode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_last_child(xmlnode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_previous_sibling(xmlnode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_next_sibling(xmlnode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_insert_before(xmlnode*,IXMLDOMNode*,const VARIANT*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_replace_child(xmlnode*,IXMLDOMNode*,IXMLDOMNode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_put_text(xmlnode*,BSTR) DECLSPEC_HIDDEN;
extern HRESULT node_get_xml(xmlnode*,BOOL,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_clone(xmlnode*,VARIANT_BOOL,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_get_prefix(xmlnode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_get_base_name(xmlnode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_get_namespaceURI(xmlnode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_remove_child(xmlnode*,IXMLDOMNode*,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_has_childnodes(const xmlnode*,VARIANT_BOOL*) DECLSPEC_HIDDEN;
extern HRESULT node_get_owner_doc(const xmlnode*,IXMLDOMDocument**) DECLSPEC_HIDDEN;
extern HRESULT node_get_text(const xmlnode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_select_nodes(const xmlnode*,BSTR,IXMLDOMNodeList**) DECLSPEC_HIDDEN;
extern HRESULT node_select_singlenode(const xmlnode*,BSTR,IXMLDOMNode**) DECLSPEC_HIDDEN;
extern HRESULT node_transform_node(const xmlnode*,IXMLDOMNode*,BSTR*) DECLSPEC_HIDDEN;
extern HRESULT node_transform_node_params(const xmlnode*,IXMLDOMNode*,BSTR*,ISequentialStream*,
    const struct xslprocessor_params*) DECLSPEC_HIDDEN;
extern HRESULT node_create_supporterrorinfo(const tid_t*,void**) DECLSPEC_HIDDEN;

extern HRESULT get_domdoc_from_xmldoc(xmlDocPtr xmldoc, IXMLDOMDocument3 **document) DECLSPEC_HIDDEN;

extern HRESULT SchemaCache_validate_tree(IXMLDOMSchemaCollection2*, xmlNodePtr) DECLSPEC_HIDDEN;
extern XDR_DT  SchemaCache_get_node_dt(IXMLDOMSchemaCollection2*, xmlNodePtr) DECLSPEC_HIDDEN;
extern HRESULT cache_from_doc_ns(IXMLDOMSchemaCollection2*, xmlnode*) DECLSPEC_HIDDEN;

extern XDR_DT str_to_dt(xmlChar const* str, int len /* calculated if -1 */) DECLSPEC_HIDDEN;
extern XDR_DT bstr_to_dt(OLECHAR const* bstr, int len /* calculated if -1 */) DECLSPEC_HIDDEN;
extern xmlChar const* dt_to_str(XDR_DT dt) DECLSPEC_HIDDEN;
extern const char* debugstr_dt(XDR_DT dt) DECLSPEC_HIDDEN;
extern OLECHAR const* dt_to_bstr(XDR_DT dt) DECLSPEC_HIDDEN;
extern HRESULT dt_validate(XDR_DT dt, xmlChar const* content) DECLSPEC_HIDDEN;

extern BSTR EnsureCorrectEOL(BSTR) DECLSPEC_HIDDEN;

extern xmlChar* tagName_to_XPath(const BSTR tagName) DECLSPEC_HIDDEN;

extern HRESULT dom_pi_put_xml_decl(IXMLDOMNode *node, BSTR data) DECLSPEC_HIDDEN;

#include <libxslt/documents.h>
extern xmlDocPtr xslt_doc_default_loader(const xmlChar *uri, xmlDictPtr dict, int options,
    void *_ctxt, xsltLoadType type) DECLSPEC_HIDDEN;

static inline BSTR bstr_from_xmlChar(const xmlChar *str)
{
    BSTR ret = NULL;

    if(str) {
        DWORD len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str, -1, NULL, 0);
        ret = SysAllocStringLen(NULL, len-1);
        if(ret)
            MultiByteToWideChar( CP_UTF8, 0, (LPCSTR)str, -1, ret, len);
    }
    else
        ret = SysAllocStringLen(NULL, 0);

    return ret;
}

static inline xmlChar *xmlchar_from_wcharn(const WCHAR *str, int nchars, BOOL use_xml_alloc)
{
    xmlChar *xmlstr;
    DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, nchars, NULL, 0, NULL, NULL );

    xmlstr = use_xml_alloc ? xmlMalloc( len + 1 ) : heap_alloc( len + 1 );
    if ( xmlstr )
    {
        WideCharToMultiByte( CP_UTF8, 0, str, nchars, (LPSTR) xmlstr, len+1, NULL, NULL );
        xmlstr[len] = 0;
    }
    return xmlstr;
}

static inline xmlChar *xmlchar_from_wchar( const WCHAR *str )
{
    return xmlchar_from_wcharn(str, -1, FALSE);
}

static inline xmlChar *heap_strdupxmlChar(const xmlChar *str)
{
    xmlChar *ret = NULL;

    if(str) {
        DWORD size;

        size = (xmlStrlen(str)+1)*sizeof(xmlChar);
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline HRESULT return_null_node(IXMLDOMNode **p)
{
    if(!p)
        return E_INVALIDARG;
    *p = NULL;
    return S_FALSE;
}

static inline HRESULT return_null_ptr(void **p)
{
    if(!p)
        return E_INVALIDARG;
    *p = NULL;
    return S_FALSE;
}

static inline HRESULT return_null_var(VARIANT *p)
{
    if(!p)
        return E_INVALIDARG;

    V_VT(p) = VT_NULL;
    return S_FALSE;
}

static inline HRESULT return_null_bstr(BSTR *p)
{
    if(!p)
        return E_INVALIDARG;

    *p = NULL;
    return S_FALSE;
}

static inline HRESULT return_var_false(VARIANT_BOOL *p)
{
    if(!p)
        return E_INVALIDARG;

    *p = VARIANT_FALSE;
    return S_FALSE;
}

extern IXMLDOMParseError *create_parseError( LONG code, BSTR url, BSTR reason, BSTR srcText,
                                             LONG line, LONG linepos, LONG filepos ) DECLSPEC_HIDDEN;
extern HRESULT SchemaCache_create(MSXML_VERSION, void**) DECLSPEC_HIDDEN;
extern HRESULT XMLDocument_create(void**) DECLSPEC_HIDDEN;
extern HRESULT SAXXMLReader_create(MSXML_VERSION, void**) DECLSPEC_HIDDEN;
extern HRESULT SAXAttributes_create(MSXML_VERSION, void**) DECLSPEC_HIDDEN;
extern HRESULT XMLHTTPRequest_create(void **) DECLSPEC_HIDDEN;
extern HRESULT ServerXMLHTTP_create(void **) DECLSPEC_HIDDEN;
extern HRESULT XSLTemplate_create(void**) DECLSPEC_HIDDEN;
extern HRESULT MXWriter_create(MSXML_VERSION, void**) DECLSPEC_HIDDEN;
extern HRESULT MXNamespaceManager_create(void**) DECLSPEC_HIDDEN;
extern HRESULT XMLParser_create(void**) DECLSPEC_HIDDEN;
extern HRESULT XMLView_create(void**) DECLSPEC_HIDDEN;

/* Error Codes - not defined anywhere in the public headers */
#define E_XML_ELEMENT_UNDECLARED            0xC00CE00D
#define E_XML_ELEMENT_ID_NOT_FOUND          0xC00CE00E
/* ... */
#define E_XML_EMPTY_NOT_ALLOWED             0xC00CE011
#define E_XML_ELEMENT_NOT_COMPLETE          0xC00CE012
#define E_XML_ROOT_NAME_MISMATCH            0xC00CE013
#define E_XML_INVALID_CONTENT               0xC00CE014
#define E_XML_ATTRIBUTE_NOT_DEFINED         0xC00CE015
#define E_XML_ATTRIBUTE_FIXED               0xC00CE016
#define E_XML_ATTRIBUTE_VALUE               0xC00CE017
#define E_XML_ILLEGAL_TEXT                  0xC00CE018
/* ... */
#define E_XML_REQUIRED_ATTRIBUTE_MISSING    0xC00CE020

#define NODE_PRIV_TRAILING_IGNORABLE_WS     0x40000000
#define NODE_PRIV_CHILD_IGNORABLE_WS        0x80000000
#define NODE_PRIV_REFCOUNT_MASK             ~(NODE_PRIV_TRAILING_IGNORABLE_WS|NODE_PRIV_CHILD_IGNORABLE_WS)
#endif /* __MSXML_PRIVATE__ */
