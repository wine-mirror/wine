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

#include <stdbool.h>

#include "dispex.h"

#include "wine/list.h"

#include <libxml/xpath.h>

#include "msxml_dispex.h"

extern const CLSID * DOMDocument_version(MSXML_VERSION v);

enum error_codes
{
    E_SAX_UNDEFINEDREF = 0xc00ce002,
    E_SAX_INFINITEREFLOOP = 0xc00ce003,
    E_SAX_UNPARSEDENTITYREF = 0xc00ce006,
    E_SAX_CONTAINSCOLON = 0xc00ce00c,
    E_SAX_UNDECLAREDPREFIX = 0xc00ce01d,

    E_SAX_MISSINGEQUALS = 0xc00ce501,
    E_SAX_MISSINGQUOTE = 0xc00ce502,
    E_SAX_COMMENTSYNTAX = 0xc00ce503,
    E_SAX_BADSTARTNAMECHAR = 0xc00ce504,
    E_SAX_BADNAMECHAR = 0xc00ce505,
    E_SAX_BADCHARINSTRING = 0xc00ce506,
    E_SAX_XMLDECLSYNTAX = 0xc00ce507,
    E_SAX_MISSINGWHITESPACE = 0xc00ce509,
    E_SAX_EXPECTINGTAGEND = 0xc00ce50a,
    E_SAX_BADCHARINDTD = 0xc00ce50b,
    E_SAX_MISSINGSEMICOLON = 0xc00ce50d,
    E_SAX_BADCHARINENTREF = 0xc00ce50e,
    E_SAX_BADCHARINMIXEDMODEL = 0xc00ce515,
    E_SAX_MISSING_STAR = 0xc00ce516,
    E_SAX_MISSING_PAREN = 0xc00ce518,
    E_SAX_BADCHARINENUMERATION = 0xc00ce519,
    E_SAX_PIDECLSYNTAX = 0xc00ce51a,
    E_SAX_EXPECTINGCLOSEQUOTE = 0xc00ce51b,
    E_SAX_MULTIPLE_COLONS = 0xc00ce51c,
    E_SAX_INVALID_UNICODE = 0xc00ce51f,
    E_SAX_WHITESPACEORQUESTIONMARK = 0xc00ce520,
    E_SAX_UNEXPECTEDENDTAG = 0xc00ce552,
    E_SAX_DUPLICATEATTRIBUTE = 0xc00ce554,
    E_SAX_INVALIDATROOTLEVEL = 0xc00ce556,
    E_SAX_BAD_XMLDECL = 0xc00ce557,
    E_SAX_MISSINGROOT = 0xc00ce558,
    E_SAX_UNEXPECTED_EOF = 0xc00ce559,
    E_SAX_INVALID_CDATACLOSINGTAG = 0xc00ce55c,
    E_SAX_UNCLOSEDPI = 0xc00ce55d,
    E_SAX_UNCLOSEDENDTAG = 0xc00ce55f,
    E_SAX_UNCLOSEDCDATA = 0xc00ce564,
    E_SAX_BADDECLNAME = 0xc00ce565,
    E_SAX_RESERVEDNAMESPACE = 0xc00ce568,
    E_SAX_UNEXPECTED_ATTRIBUTE = 0xc00ce56c,
    E_SAX_ENDTAGMISMATCH = 0xc00ce56d,
    E_SAX_INVALIDENCODING = 0xc00ce56e,
    E_SAX_INVALIDSWITCH = 0xc00ce56f,
    E_SAX_INVALID_MODEL = 0xc00ce571,
    E_SAX_INVALID_TYPE = 0xc00ce572,
    E_SAX_BADXMLCASE = 0xc00ce576,
    E_SAX_INVALID_STANDALONE = 0xc00ce579,
    E_SAX_INVALID_VERSION = 0xc00ce57f,
    E_DOM_MAX_ELEMENT_DEPTH = 0xc00ce586,

    E_SAX_MAX_ELEMENT_DEPTH = 0xc00cee92,
};


static inline bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;
    return true;
}

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

extern HINSTANCE MSXML_hInstance;

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

extern void schemasInit(void);
extern void schemasCleanup(void);

struct dom_namespace
{
    struct list entry;
    BSTR qname;
    BSTR name;
    BSTR uri;
};

enum domnode_flags
{
    DOMNODE_READONLY_VALUE = 0x1,
    DOMNODE_IGNORED_WS_AFTER_STARTTAG = 0x2,
    DOMNODE_IGNORED_WS = 0x4,
    DOMNODE_PARSED_VALUE = 0x8,
};

typedef struct _select_ns_entry
{
    struct list entry;
    xmlChar const* prefix;
    xmlChar prefix_end;
    xmlChar const* href;
    xmlChar href_end;
} select_ns_entry;

struct domdoc_properties
{
    MSXML_VERSION version;
    VARIANT_BOOL preserving;
    VARIANT_BOOL validating;
    IXMLDOMSchemaCollection2* schemaCache;
    struct list selectNsList;
    xmlChar const* selectNsStr;
    LONG selectNsStr_len;
    bool XPath;
    bool prohibit_dtd;
    bool normalize_attribute_values;
    int max_element_depth;
    IUri *uri;
};

struct domnode
{
    struct list entry;
    struct list owner_entry;
    unsigned int refcount;

    DOMNodeType type;
    unsigned int flags;

    BSTR name;
    BSTR prefix;
    BSTR qname;
    BSTR data;
    BSTR uri;

    struct domdoc_properties *properties;

    struct domnode *parent;
    struct domnode *owner;
    struct list children;
    struct list attributes;
    struct list owned;
};

extern IXMLDOMSchemaCollection2 *doc_properties_get_schema(struct domdoc_properties *properties);
extern MSXML_VERSION doc_properties_get_version(struct domdoc_properties *properties);

extern void domdoc_properties_clear_selection_namespaces(struct list *);
extern struct domdoc_properties *domdoc_create_properties(MSXML_VERSION version);
extern MSXML_VERSION domdoc_version(const struct domnode *doc);
extern HRESULT domnode_create(DOMNodeType type, const WCHAR *name, int name_len,
        const WCHAR *uri, int uri_len, struct domnode *owner, struct domnode **node);
extern void domnode_destroy_tree(struct domnode *tree);
extern struct domnode *domnode_addref(struct domnode *node);
extern void domnode_release(struct domnode *node);
extern struct domnode *domnode_get_first_child(struct domnode *node);
extern struct domnode *domnode_get_next_sibling(struct domnode *node);
extern HRESULT domnode_get_attribute(const struct domnode *node, const WCHAR *name, struct domnode **attr);
extern HRESULT node_clone_domnode(struct domnode *, bool, struct domnode **);
extern HRESULT parse_stream(ISequentialStream *stream, bool utf16, const struct domdoc_properties *properties,
        struct domnode **tree);
extern void dump_doc_refcounts(struct domnode *node);

struct parsed_name
{
    BSTR prefix;
    BSTR local;
    BSTR qname;
};

extern HRESULT parse_qualified_name(const WCHAR *src, struct parsed_name *name);
void parsed_name_cleanup(struct parsed_name *name);

/* Compat glue */
extern xmlDocPtr create_xmldoc_from_domdoc(struct domnode *node, xmlNodePtr *xmlnode);
static inline bool xml_is_space(WCHAR ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

/* IXMLDOMNamedNodeMap custom function table */
struct nodemap_funcs
{
    HRESULT (*get_named_item)(const struct domnode *,BSTR,IXMLDOMNode**);
    HRESULT (*set_named_item)(struct domnode *,IXMLDOMNode*,IXMLDOMNode**);
    HRESULT (*remove_named_item)(struct domnode *,BSTR,IXMLDOMNode**);
    HRESULT (*get_item)(struct domnode *,LONG,IXMLDOMNode**);
    HRESULT (*get_length)(struct domnode *,LONG*);
    HRESULT (*get_qualified_item)(const struct domnode *,BSTR,BSTR,IXMLDOMNode**);
    HRESULT (*remove_qualified_item)(struct domnode *,BSTR,BSTR,IXMLDOMNode**);
    HRESULT (*next_node)(const struct domnode *,LONG *,IXMLDOMNode **);
};

/* used by IEnumVARIANT to access outer object items */
struct enumvariant_funcs
{
    HRESULT (*get_item)(IUnknown*, LONG, VARIANT*);
    HRESULT (*next)(IUnknown*);
};

/* constructors */
extern HRESULT create_domdoc(struct domnode *node, IUnknown **);
extern HRESULT create_node(struct domnode *node, IXMLDOMNode **);
extern HRESULT create_element(struct domnode *, IUnknown **);
extern HRESULT create_attribute(struct domnode *, IUnknown **);
extern HRESULT create_attribute_node(const WCHAR *name, const WCHAR *uri, struct domnode *, IXMLDOMNode **);
extern HRESULT create_text(struct domnode *, IUnknown **);
extern HRESULT create_pi(struct domnode *, IUnknown **);
extern HRESULT create_pi_node(struct domnode *, const WCHAR *, const WCHAR *, IXMLDOMProcessingInstruction **);
extern HRESULT create_comment(struct domnode *, IUnknown **);
extern HRESULT create_cdata(struct domnode *, IUnknown **);
extern HRESULT create_children_nodelist(struct domnode *, IXMLDOMNodeList **);
extern HRESULT create_nodemap(struct domnode *, const struct nodemap_funcs *, IXMLDOMNamedNodeMap **);
extern HRESULT create_doc_fragment(struct domnode *, IUnknown **);
extern HRESULT create_entity_ref(struct domnode *, IUnknown **);
extern HRESULT create_doc_type(struct domnode *, IUnknown **);
extern HRESULT create_selection(struct domnode *, BSTR, bool xpath, IXMLDOMNodeList** );
extern HRESULT create_selection_from_nodeset( xmlXPathObjectPtr, IXMLDOMNodeList ** );
extern HRESULT create_enumvariant( IUnknown*, BOOL, const struct enumvariant_funcs*, IEnumVARIANT**);
extern HRESULT create_dom_implementation(IXMLDOMImplementation **obj);

extern void wineXmlCallbackLog(char const* caller, xmlErrorLevel lvl, char const* msg, va_list ap);
extern void wineXmlCallbackError(char const* caller, const xmlError* err);

#define LIBXML2_LOG_CALLBACK WINAPIV __WINE_PRINTF_ATTR(2,3)

#define LIBXML2_CALLBACK_TRACE(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_NONE, msg, ap)

#define LIBXML2_CALLBACK_WARN(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_WARNING, msg, ap)

#define LIBXML2_CALLBACK_ERR(caller, msg, ap) \
        wineXmlCallbackLog(#caller, XML_ERR_ERROR, msg, ap)

#define LIBXML2_CALLBACK_SERROR(caller, err) wineXmlCallbackError(#caller, err)

extern bool node_query_interface(struct domnode*,REFIID,void**);
extern struct domnode *get_node_obj(void *iface);
extern bool parser_is_valid_qualified_name(const WCHAR *name);
extern HRESULT parse_xml_decl_body(const WCHAR *text, BSTR *version, BSTR *encoding, BSTR *standalone);

extern HRESULT node_append_child(struct domnode*,IXMLDOMNode*,IXMLDOMNode**);
extern HRESULT node_attribute_get_namespace_uri(struct domnode *, BSTR *);
extern HRESULT node_get_name(struct domnode*,BSTR*);
extern HRESULT node_put_data(struct domnode*, const WCHAR *);
extern HRESULT node_get_data(struct domnode*, BSTR *);
extern XDR_DT node_get_data_type(const struct domnode*);
extern HRESULT node_append_data(struct domnode*, BSTR);
extern HRESULT node_put_value(struct domnode*,VARIANT*);
extern HRESULT node_get_attribute(const struct domnode *,const WCHAR *,IXMLDOMAttribute **);
extern HRESULT node_get_qualified_attribute(const struct domnode *, const WCHAR *name, const WCHAR *uri, IXMLDOMNode **);
extern HRESULT node_get_attribute_by_index(const struct domnode *, LONG, IXMLDOMNode **);
extern HRESULT node_get_attribute_value(struct domnode *,const WCHAR *,VARIANT *);
extern HRESULT node_set_attribute(struct domnode *, IXMLDOMNode *, IXMLDOMNode **);
extern HRESULT node_set_attribute_value(struct domnode *,const WCHAR *,const VARIANT *);
extern HRESULT node_remove_attribute(struct domnode *, const WCHAR *, IXMLDOMNode **);
extern HRESULT node_remove_attribute_node(struct domnode *,IXMLDOMAttribute *,IXMLDOMAttribute **);
extern HRESULT node_remove_qualified_attribute(struct domnode *, const WCHAR *, const WCHAR *, IXMLDOMNode **);
extern HRESULT node_get_parent(struct domnode*,IXMLDOMNode**);
extern HRESULT node_get_child_nodes(struct domnode *,IXMLDOMNodeList**);
extern HRESULT node_get_first_child(struct domnode *,IXMLDOMNode**);
extern HRESULT node_get_last_child(struct domnode *,IXMLDOMNode**);
extern HRESULT node_get_previous_sibling(struct domnode*,IXMLDOMNode**);
extern HRESULT node_get_next_sibling(struct domnode*,IXMLDOMNode**);
extern HRESULT node_insert_before(struct domnode*,IXMLDOMNode*,const VARIANT*,IXMLDOMNode**);
extern HRESULT node_replace_child(struct domnode*,IXMLDOMNode*,IXMLDOMNode*,IXMLDOMNode**);
extern HRESULT node_get_xml(struct domnode *, BSTR*);
extern HRESULT node_clone(struct domnode*,VARIANT_BOOL,IXMLDOMNode**);
extern HRESULT node_get_prefix(struct domnode*,BSTR*);
extern HRESULT node_get_base_name(struct domnode*,BSTR*);
extern HRESULT node_get_namespaceURI(struct domnode*, BSTR *);
extern HRESULT node_remove_child(struct domnode*,IXMLDOMNode*,IXMLDOMNode**);
extern HRESULT node_has_childnodes(const struct domnode*,VARIANT_BOOL*);
extern HRESULT node_get_owner_document(const struct domnode*,IXMLDOMDocument**);
extern HRESULT node_get_text(struct domnode*,BSTR*);
extern HRESULT node_get_preserved_text(const struct domnode*,BSTR*);
extern HRESULT node_get_value(struct domnode *node, VARIANT *value);
extern HRESULT node_select_nodes(struct domnode*,BSTR,IXMLDOMNodeList**);
extern HRESULT node_select_singlenode(struct domnode *,BSTR,IXMLDOMNode**);
extern HRESULT node_transform_node(struct domnode*,IXMLDOMNode*,BSTR*);
extern HRESULT node_transform_node_params(struct domnode*,IXMLDOMNode*,BSTR*,ISequentialStream*,
    const struct xslprocessor_params*);
extern HRESULT node_create_supporterrorinfo(const tid_t*,void**);
extern HRESULT node_save(struct domnode *, IStream *);
extern HRESULT node_get_elements_by_tagname(struct domnode *,BSTR,IXMLDOMNodeList**);
extern HRESULT node_validate(struct domnode *, IXMLDOMNode *, IXMLDOMParseError **);
extern void node_move_children(struct domnode *dst, struct domnode *src);
extern HRESULT node_split_text(struct domnode *, LONG, IXMLDOMText **);
extern HRESULT node_delete_data(struct domnode *, LONG, LONG);
extern HRESULT node_substring_data(struct domnode *, LONG, LONG, BSTR *);
extern HRESULT node_get_data_length(struct domnode *, LONG *);
extern HRESULT node_insert_data(struct domnode *, LONG, BSTR);
extern HRESULT node_replace_data(struct domnode *, LONG, LONG, BSTR);

extern UINT get_codepage_for_encoding(const WCHAR *encoding);

extern HRESULT SchemaCache_validate_tree(IXMLDOMSchemaCollection2*, xmlNodePtr);
extern XDR_DT SchemaCache_get_node_dt(IXMLDOMSchemaCollection2 *, const WCHAR *name, const WCHAR *uri);
extern HRESULT cache_from_doc_ns(IXMLDOMSchemaCollection2*, struct domnode *);

extern XDR_DT str_to_dt(xmlChar const* str, int len /* calculated if -1 */);
extern XDR_DT bstr_to_dt(OLECHAR const* bstr, int len /* calculated if -1 */);
extern xmlChar const* dt_to_str(XDR_DT dt);
extern const char* debugstr_dt(XDR_DT dt);
extern OLECHAR const* dt_to_bstr(XDR_DT dt);
extern HRESULT dt_validate(XDR_DT dt, const WCHAR *content);

extern HRESULT variant_get_int_property(const VARIANT *v, int *ret);

#include <libxslt/documents.h>
extern xmlDocPtr xslt_doc_default_loader(const xmlChar *uri, xmlDictPtr dict, int options,
    void *_ctxt, xsltLoadType type);

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

    xmlstr = use_xml_alloc ? xmlMalloc( len + 1 ) : malloc( len + 1 );
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

static inline xmlChar *strdupxmlChar(const xmlChar *str)
{
    xmlChar *ret = NULL;

    if(str) {
        DWORD size;

        size = (xmlStrlen(str)+1)*sizeof(xmlChar);
        ret = malloc(size);
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
    V_UINT_PTR(p) = 0;
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
                                             LONG line, LONG linepos, LONG filepos );
extern HRESULT SchemaCache_create(MSXML_VERSION, void**);
extern HRESULT XMLDocument_create(void**);
extern HRESULT SAXXMLReader_create(MSXML_VERSION, void**);
extern HRESULT SAXAttributes_create(MSXML_VERSION, void**);
extern HRESULT XMLHTTPRequest_create(void **);
extern HRESULT ServerXMLHTTP_create(void **);
extern HRESULT XSLTemplate_create(void**);
extern HRESULT MXWriter_create(MSXML_VERSION, void**);
extern HRESULT MXNamespaceManager_create(void**);
extern HRESULT XMLParser_create(void**);
extern HRESULT XMLView_create(void**);

extern HRESULT stream_wrapper_create(const void *, DWORD, ISequentialStream **);

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

#endif /* __MSXML_PRIVATE__ */
