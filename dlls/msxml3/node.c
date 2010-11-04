/*
 *    Node implementation
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

#include "config.h"

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#ifdef HAVE_LIBXML2
# include <libxml/HTMLtree.h>
#endif

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

static const WCHAR szBinBase64[]  = {'b','i','n','.','b','a','s','e','6','4',0};
static const WCHAR szString[]     = {'s','t','r','i','n','g',0};
static const WCHAR szNumber[]     = {'n','u','m','b','e','r',0};
static const WCHAR szInt[]        = {'I','n','t',0};
static const WCHAR szFixed[]      = {'F','i','x','e','d','.','1','4','.','4',0};
static const WCHAR szBoolean[]    = {'B','o','o','l','e','a','n',0};
static const WCHAR szDateTime[]   = {'d','a','t','e','T','i','m','e',0};
static const WCHAR szDateTimeTZ[] = {'d','a','t','e','T','i','m','e','.','t','z',0};
static const WCHAR szDate[]       = {'D','a','t','e',0};
static const WCHAR szTime[]       = {'T','i','m','e',0};
static const WCHAR szTimeTZ[]     = {'T','i','m','e','.','t','z',0};
static const WCHAR szI1[]         = {'i','1',0};
static const WCHAR szI2[]         = {'i','2',0};
static const WCHAR szI4[]         = {'i','4',0};
static const WCHAR szIU1[]        = {'u','i','1',0};
static const WCHAR szIU2[]        = {'u','i','2',0};
static const WCHAR szIU4[]        = {'u','i','4',0};
static const WCHAR szR4[]         = {'r','4',0};
static const WCHAR szR8[]         = {'r','8',0};
static const WCHAR szFloat[]      = {'f','l','o','a','t',0};
static const WCHAR szUUID[]       = {'u','u','i','d',0};
static const WCHAR szBinHex[]     = {'b','i','n','.','h','e','x',0};

static const IID IID_xmlnode = {0x4f2f4ba2,0xb822,0x11df,{0x8b,0x8a,0x68,0x50,0xdf,0xd7,0x20,0x85}};

xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type )
{
    xmlnode *This;

    if ( !iface )
        return NULL;
    This = get_node_obj( iface );
    if ( !This || !This->node )
        return NULL;
    if ( type && This->node->type != type )
        return NULL;
    return This->node;
}

BOOL node_query_interface(xmlnode *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_xmlnode, riid)) {
        TRACE("(%p)->(IID_xmlnode %p)\n", This, ppv);
        *ppv = This;
        return TRUE;
    }

    if(This->dispex.outer)
        return dispex_query_interface(&This->dispex, riid, ppv);

    return FALSE;
}

xmlnode *get_node_obj(IXMLDOMNode *node)
{
    xmlnode *obj;
    HRESULT hres;

    hres = IXMLDOMNode_QueryInterface(node, &IID_xmlnode, (void**)&obj);
    return SUCCEEDED(hres) ? obj : NULL;
}

static inline xmlnode *impl_from_IXMLDOMNode( IXMLDOMNode *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpVtbl));
}

HRESULT node_get_nodeName(xmlnode *This, BSTR *name)
{
    if (!name)
        return E_INVALIDARG;

    *name = bstr_from_xmlChar(This->node->name);
    if (!*name)
        return S_FALSE;

    return S_OK;
}

HRESULT node_get_content(xmlnode *This, VARIANT *value)
{
    xmlChar *content;

    if(!value)
        return E_INVALIDARG;

    content = xmlNodeGetContent(This->node);
    V_VT(value) = VT_BSTR;
    V_BSTR(value) = bstr_from_xmlChar( content );
    xmlFree(content);

    TRACE("%p returned %s\n", This, debugstr_w(V_BSTR(value)));
    return S_OK;
}

HRESULT node_set_content(xmlnode *This, LPCWSTR value)
{
    xmlChar *str;

    str = xmlChar_from_wchar(value);
    if(!str)
        return E_OUTOFMEMORY;

    xmlNodeSetContent(This->node, str);
    heap_free(str);
    return S_OK;
}

static HRESULT node_set_content_escaped(xmlnode *This, LPCWSTR value)
{
    xmlChar *str, *escaped;

    str = xmlChar_from_wchar(value);
    if(!str)
        return E_OUTOFMEMORY;

    escaped = xmlEncodeSpecialChars(NULL, str);
    if(!escaped)
    {
        heap_free(str);
        return E_OUTOFMEMORY;
    }

    xmlNodeSetContent(This->node, escaped);

    heap_free(str);
    xmlFree(escaped);

    return S_OK;
}

HRESULT node_put_value(xmlnode *This, VARIANT *value)
{
    VARIANT string_value;
    HRESULT hr;

    VariantInit(&string_value);
    hr = VariantChangeType(&string_value, value, 0, VT_BSTR);
    if(FAILED(hr)) {
        WARN("Couldn't convert to VT_BSTR\n");
        return hr;
    }

    hr = node_set_content(This, V_BSTR(&string_value));
    VariantClear(&string_value);

    return hr;
}

HRESULT node_put_value_escaped(xmlnode *This, VARIANT *value)
{
    VARIANT string_value;
    HRESULT hr;

    VariantInit(&string_value);
    hr = VariantChangeType(&string_value, value, 0, VT_BSTR);
    if(FAILED(hr)) {
        WARN("Couldn't convert to VT_BSTR\n");
        return hr;
    }

    hr = node_set_content_escaped(This, V_BSTR(&string_value));
    VariantClear(&string_value);

    return hr;
}

static HRESULT get_node(
    xmlnode *This,
    const char *name,
    xmlNodePtr node,
    IXMLDOMNode **out )
{
    TRACE("(%p)->(%s %p %p)\n", This, name, node, out );

    if ( !out )
        return E_INVALIDARG;

    /* if we don't have a doc, use our parent. */
    if(node && !node->doc && node->parent)
        node->doc = node->parent->doc;

    *out = create_node( node );
    if (!*out)
        return S_FALSE;
    return S_OK;
}

HRESULT node_get_parent(xmlnode *This, IXMLDOMNode **parent)
{
    return get_node( This, "parent", This->node->parent, parent );
}

HRESULT node_get_child_nodes(xmlnode *This, IXMLDOMNodeList **ret)
{
    if(!ret)
        return E_INVALIDARG;

    *ret = create_children_nodelist(This->node);
    if(!*ret)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT node_get_first_child(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "firstChild", This->node->children, ret);
}

HRESULT node_get_last_child(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "lastChild", This->node->last, ret);
}

HRESULT node_get_previous_sibling(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "previous", This->node->prev, ret);
}

HRESULT node_get_next_sibling(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "next", This->node->next, ret);
}

HRESULT node_insert_before(xmlnode *This, IXMLDOMNode *new_child, const VARIANT *ref_child,
        IXMLDOMNode **ret)
{
    xmlNodePtr before_node, new_child_node;
    IXMLDOMNode *before = NULL;
    xmlnode *node_obj;
    HRESULT hr;

    if(!new_child)
        return E_INVALIDARG;

    node_obj = get_node_obj(new_child);
    if(!node_obj) {
        FIXME("newChild is not our node implementation\n");
        return E_FAIL;
    }

    switch(V_VT(ref_child))
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        hr = IUnknown_QueryInterface(V_UNKNOWN(ref_child), &IID_IXMLDOMNode, (LPVOID)&before);
        if(FAILED(hr)) return hr;
        break;

    default:
        FIXME("refChild var type %x\n", V_VT(ref_child));
        return E_FAIL;
    }

    new_child_node = node_obj->node;
    TRACE("new_child_node %p This->node %p\n", new_child_node, This->node);

    if(!new_child_node->parent)
        if(xmldoc_remove_orphan(new_child_node->doc, new_child_node) != S_OK)
            WARN("%p is not an orphan of %p\n", new_child_node, new_child_node->doc);

    if(before)
    {
        node_obj = get_node_obj(before);
        IXMLDOMNode_Release(before);
        if(!node_obj) {
            FIXME("before node is not our node implementation\n");
            return E_FAIL;
        }

        before_node = node_obj->node;
        xmlAddPrevSibling(before_node, new_child_node);
    }
    else
    {
        xmlAddChild(This->node, new_child_node);
    }

    if(ret) {
        IXMLDOMNode_AddRef(new_child);
        *ret = new_child;
    }

    TRACE("ret S_OK\n");
    return S_OK;
}

HRESULT node_replace_child(xmlnode *This, IXMLDOMNode *newChild, IXMLDOMNode *oldChild,
        IXMLDOMNode **ret)
{
    xmlnode *old_child, *new_child;
    xmlDocPtr leaving_doc;
    xmlNode *my_ancestor;

    /* Do not believe any documentation telling that newChild == NULL
       means removal. It does certainly *not* apply to msxml3! */
    if(!newChild || !oldChild)
        return E_INVALIDARG;

    if(ret)
        *ret = NULL;

    old_child = get_node_obj(oldChild);
    if(!old_child) {
        FIXME("oldChild is not our node implementation\n");
        return E_FAIL;
    }

    if(old_child->node->parent != This->node)
    {
        WARN("childNode %p is not a child of %p\n", oldChild, This);
        return E_INVALIDARG;
    }

    new_child = get_node_obj(newChild);
    if(!new_child) {
        FIXME("newChild is not our node implementation\n");
        return E_FAIL;
    }

    my_ancestor = This->node;
    while(my_ancestor)
    {
        if(my_ancestor == new_child->node)
        {
            WARN("tried to create loop\n");
            return E_FAIL;
        }
        my_ancestor = my_ancestor->parent;
    }

    if(!new_child->node->parent)
        if(xmldoc_remove_orphan(new_child->node->doc, new_child->node) != S_OK)
            WARN("%p is not an orphan of %p\n", new_child->node, new_child->node->doc);

    leaving_doc = new_child->node->doc;
    xmldoc_add_ref(old_child->node->doc);
    xmlReplaceNode(old_child->node, new_child->node);
    xmldoc_release(leaving_doc);

    xmldoc_add_orphan(old_child->node->doc, old_child->node);

    if(ret)
    {
        IXMLDOMNode_AddRef(oldChild);
        *ret = oldChild;
    }

    return S_OK;
}

static HRESULT WINAPI xmlnode_removeChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlnode *child_node;

    TRACE("(%p)->(%p %p)\n", This, childNode, oldChild);

    if(!childNode) return E_INVALIDARG;

    if(oldChild)
        *oldChild = NULL;

    child_node = get_node_obj(childNode);
    if(!child_node) {
        FIXME("childNode is not our node implementation\n");
        return E_FAIL;
    }

    if(child_node->node->parent != This->node)
    {
        WARN("childNode %p is not a child of %p\n", childNode, iface);
        return E_INVALIDARG;
    }

    xmlUnlinkNode(child_node->node);

    if(oldChild)
    {
        IXMLDOMNode_AddRef(childNode);
        *oldChild = childNode;
    }

    return S_OK;
}

static HRESULT WINAPI xmlnode_appendChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    DOMNodeType type;
    VARIANT var;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, newChild, outNewChild);

    hr = IXMLDOMNode_get_nodeType(newChild, &type);
    if(FAILED(hr) || type == NODE_ATTRIBUTE) {
        if(outNewChild) *outNewChild = NULL;
        return E_FAIL;
    }

    VariantInit(&var);
    return IXMLDOMNode_insertBefore(This->iface, newChild, var, outNewChild);
}

static HRESULT WINAPI xmlnode_hasChildNodes(
    IXMLDOMNode *iface,
    VARIANT_BOOL* hasChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, hasChild);

    if (!hasChild)
        return E_INVALIDARG;
    if (!This->node->children)
    {
        *hasChild = VARIANT_FALSE;
        return S_FALSE;
    }

    *hasChild = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI xmlnode_get_ownerDocument(
    IXMLDOMNode *iface,
    IXMLDOMDocument** DOMDocument)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, DOMDocument);

    return DOMDocument_create_from_xmldoc(This->node->doc, (IXMLDOMDocument3**)DOMDocument);
}

HRESULT node_clone(xmlnode *This, VARIANT_BOOL deep, IXMLDOMNode **cloneNode)
{
    IXMLDOMNode *node;
    xmlNodePtr clone;

    if(!cloneNode) return E_INVALIDARG;

    clone = xmlCopyNode(This->node, deep ? 1 : 2);
    if (clone)
    {
        clone->doc = This->node->doc;
        xmldoc_add_orphan(clone->doc, clone);

        node = create_node(clone);
        if (!node)
        {
            ERR("Copy failed\n");
            return E_FAIL;
        }

        *cloneNode = node;
    }
    else
    {
        ERR("Copy failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlnode_get_nodeTypeString(
    IXMLDOMNode *iface,
    BSTR* xmlnodeType)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    const xmlChar *str;

    TRACE("(%p)->(%p)\n", This, xmlnodeType );

    if (!xmlnodeType)
        return E_INVALIDARG;

    if ( !This->node )
        return E_FAIL;

    switch( This->node->type )
    {
    case XML_ATTRIBUTE_NODE:
        str = (const xmlChar*) "attribute";
        break;
    case XML_CDATA_SECTION_NODE:
        str = (const xmlChar*) "cdatasection";
        break;
    case XML_COMMENT_NODE:
        str = (const xmlChar*) "comment";
        break;
    case XML_DOCUMENT_NODE:
        str = (const xmlChar*) "document";
        break;
    case XML_DOCUMENT_FRAG_NODE:
        str = (const xmlChar*) "documentfragment";
        break;
    case XML_ELEMENT_NODE:
        str = (const xmlChar*) "element";
        break;
    case XML_ENTITY_NODE:
        str = (const xmlChar*) "entity";
        break;
    case XML_ENTITY_REF_NODE:
        str = (const xmlChar*) "entityreference";
        break;
    case XML_NOTATION_NODE:
        str = (const xmlChar*) "notation";
        break;
    case XML_PI_NODE:
        str = (const xmlChar*) "processinginstruction";
        break;
    case XML_TEXT_NODE:
        str = (const xmlChar*) "text";
        break;
    default:
        FIXME("Unknown node type (%d)\n", This->node->type);
        str = This->node->name;
        break;
    }

    *xmlnodeType = bstr_from_xmlChar( str );
    if (!*xmlnodeType)
        return S_FALSE;

    return S_OK;
}

static inline xmlChar* trim_whitespace(xmlChar* str)
{
    xmlChar* ret = str;
    int len;

    if (!str)
        return NULL;

    while (*ret && isspace(*ret))
        ++ret;
    len = xmlStrlen(ret);
    while (isspace(ret[len-1]))
        --len;

    ret = xmlStrndup(ret, len);
    xmlFree(str);
    return ret;
}

static xmlChar* do_get_text(xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar* str;
    BOOL preserving = is_preserving_whitespace(node);

    if (!node->children)
    {
        str = xmlNodeGetContent(node);
    }
    else
    {
        xmlElementType prev_type = XML_TEXT_NODE;
        xmlChar* tmp;
        str = xmlStrdup(BAD_CAST "");
        for (child = node->children; child != NULL; child = child->next)
        {
            switch (child->type)
            {
            case XML_ELEMENT_NODE:
                tmp = do_get_text(child);
                break;
            case XML_TEXT_NODE:
            case XML_CDATA_SECTION_NODE:
            case XML_ENTITY_REF_NODE:
            case XML_ENTITY_NODE:
                tmp = xmlNodeGetContent(child);
                break;
            default:
                tmp = NULL;
                break;
            }

            if (tmp)
            {
                if (*tmp)
                {
                    if (prev_type == XML_ELEMENT_NODE && child->type == XML_ELEMENT_NODE)
                        str = xmlStrcat(str, BAD_CAST " ");
                    str = xmlStrcat(str, tmp);
                    prev_type = child->type;
                }
                xmlFree(tmp);
            }
        }
    }

    switch (node->type)
    {
    case XML_ELEMENT_NODE:
    case XML_TEXT_NODE:
    case XML_ENTITY_REF_NODE:
    case XML_ENTITY_NODE:
    case XML_DOCUMENT_NODE:
    case XML_DOCUMENT_FRAG_NODE:
        if (!preserving)
            str = trim_whitespace(str);
        break;
    default:
        break;
    }

    return str;
}

static HRESULT WINAPI xmlnode_get_text(
    IXMLDOMNode *iface,
    BSTR* text)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    BSTR str = NULL;
    xmlChar *pContent;

    TRACE("(%p, type %d)->(%p)\n", This, This->node->type, text);

    if ( !text )
        return E_INVALIDARG;

    pContent = do_get_text((xmlNodePtr)This->node);
    if(pContent)
    {
        str = bstr_from_xmlChar(pContent);
        xmlFree(pContent);
    }

    /* Always return a string. */
    if (!str) str = SysAllocStringLen( NULL, 0 );

    TRACE("%p %s\n", This, debugstr_w(str) );
    *text = str;
 
    return S_OK;
}

HRESULT node_put_text(xmlnode *This, BSTR text)
{
    xmlChar *str, *str2;

    TRACE("(%p)->(%s)\n", This, debugstr_w(text));

    str = xmlChar_from_wchar(text);

    /* Escape the string. */
    str2 = xmlEncodeEntitiesReentrant(This->node->doc, str);
    heap_free(str);

    xmlNodeSetContent(This->node, str2);
    xmlFree(str2);

    return S_OK;
}

static inline BYTE hex_to_byte(xmlChar c)
{
    if(c <= '9') return c-'0';
    if(c <= 'F') return c-'A'+10;
    return c-'a'+10;
}

static inline BYTE base64_to_byte(xmlChar c)
{
    if(c == '+') return 62;
    if(c == '/') return 63;
    if(c <= '9') return c-'0'+52;
    if(c <= 'Z') return c-'A';
    return c-'a'+26;
}

static inline HRESULT VARIANT_from_xmlChar(xmlChar *str, VARIANT *v, BSTR type)
{
    if(!type || !lstrcmpiW(type, szString) ||
            !lstrcmpiW(type, szNumber) || !lstrcmpiW(type, szUUID))
    {
        V_VT(v) = VT_BSTR;
        V_BSTR(v) = bstr_from_xmlChar(str);

        if(!V_BSTR(v))
            return E_OUTOFMEMORY;
    }
    else if(!lstrcmpiW(type, szDateTime) || !lstrcmpiW(type, szDateTimeTZ) ||
            !lstrcmpiW(type, szDate) || !lstrcmpiW(type, szTime) ||
            !lstrcmpiW(type, szTimeTZ))
    {
        VARIANT src;
        WCHAR *p, *e;
        SYSTEMTIME st;
        DOUBLE date = 0.0;

        st.wYear = 1899;
        st.wMonth = 12;
        st.wDay = 30;
        st.wDayOfWeek = st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;

        V_VT(&src) = VT_BSTR;
        V_BSTR(&src) = bstr_from_xmlChar(str);

        if(!V_BSTR(&src))
            return E_OUTOFMEMORY;

        p = V_BSTR(&src);
        e = p + SysStringLen(V_BSTR(&src));

        if(p+4<e && *(p+4)=='-') /* parse date (yyyy-mm-dd) */
        {
            st.wYear = atoiW(p);
            st.wMonth = atoiW(p+5);
            st.wDay = atoiW(p+8);
            p += 10;

            if(*p == 'T') p++;
        }

        if(p+2<e && *(p+2)==':') /* parse time (hh:mm:ss.?) */
        {
            st.wHour = atoiW(p);
            st.wMinute = atoiW(p+3);
            st.wSecond = atoiW(p+6);
            p += 8;

            if(*p == '.')
            {
                p++;
                while(isdigitW(*p)) p++;
            }
        }

        SystemTimeToVariantTime(&st, &date);
        V_VT(v) = VT_DATE;
        V_DATE(v) = date;

        if(*p == '+') /* parse timezone offset (+hh:mm) */
            V_DATE(v) += (DOUBLE)atoiW(p+1)/24 + (DOUBLE)atoiW(p+4)/1440;
        else if(*p == '-') /* parse timezone offset (-hh:mm) */
            V_DATE(v) -= (DOUBLE)atoiW(p+1)/24 + (DOUBLE)atoiW(p+4)/1440;

        VariantClear(&src);
    }
    else if(!lstrcmpiW(type, szBinHex))
    {
        SAFEARRAYBOUND sab;
        int i, len;

        len = xmlStrlen(str)/2;
        sab.lLbound = 0;
        sab.cElements = len;

        V_VT(v) = (VT_ARRAY|VT_UI1);
        V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

        if(!V_ARRAY(v))
            return E_OUTOFMEMORY;

        for(i=0; i<len; i++)
            ((BYTE*)V_ARRAY(v)->pvData)[i] = (hex_to_byte(str[2*i])<<4)
                + hex_to_byte(str[2*i+1]);
    }
    else if(!lstrcmpiW(type, szBinBase64))
    {
        SAFEARRAYBOUND sab;
        int i, len;

        len  = xmlStrlen(str);
        if(str[len-2] == '=') i = 2;
        else if(str[len-1] == '=') i = 1;
        else i = 0;

        sab.lLbound = 0;
        sab.cElements = len/4*3-i;

        V_VT(v) = (VT_ARRAY|VT_UI1);
        V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

        if(!V_ARRAY(v))
            return E_OUTOFMEMORY;

        for(i=0; i<len/4; i++)
        {
            ((BYTE*)V_ARRAY(v)->pvData)[3*i] = (base64_to_byte(str[4*i])<<2)
                + (base64_to_byte(str[4*i+1])>>4);
            if(3*i+1 < sab.cElements)
                ((BYTE*)V_ARRAY(v)->pvData)[3*i+1] = (base64_to_byte(str[4*i+1])<<4)
                    + (base64_to_byte(str[4*i+2])>>2);
            if(3*i+2 < sab.cElements)
                ((BYTE*)V_ARRAY(v)->pvData)[3*i+2] = (base64_to_byte(str[4*i+2])<<6)
                    + base64_to_byte(str[4*i+3]);
        }
    }
    else
    {
        VARIANT src;
        HRESULT hres;

        if(!lstrcmpiW(type, szInt) || !lstrcmpiW(type, szI4))
            V_VT(v) = VT_I4;
        else if(!lstrcmpiW(type, szFixed))
            V_VT(v) = VT_CY;
        else if(!lstrcmpiW(type, szBoolean))
            V_VT(v) = VT_BOOL;
        else if(!lstrcmpiW(type, szI1))
            V_VT(v) = VT_I1;
        else if(!lstrcmpiW(type, szI2))
            V_VT(v) = VT_I2;
        else if(!lstrcmpiW(type, szIU1))
            V_VT(v) = VT_UI1;
        else if(!lstrcmpiW(type, szIU2))
            V_VT(v) = VT_UI2;
        else if(!lstrcmpiW(type, szIU4))
            V_VT(v) = VT_UI4;
        else if(!lstrcmpiW(type, szR4))
            V_VT(v) = VT_R4;
        else if(!lstrcmpiW(type, szR8) || !lstrcmpiW(type, szFloat))
            V_VT(v) = VT_R8;
        else
        {
            FIXME("Type handling not yet implemented\n");
            V_VT(v) = VT_BSTR;
        }

        V_VT(&src) = VT_BSTR;
        V_BSTR(&src) = bstr_from_xmlChar(str);

        if(!V_BSTR(&src))
            return E_OUTOFMEMORY;

        hres = VariantChangeTypeEx(v, &src, MAKELCID(MAKELANGID(
                        LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT),0, V_VT(v));
        VariantClear(&src);
        return hres;
    }

    return S_OK;
}

static HRESULT WINAPI xmlnode_get_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* typedValue)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    VARIANT type;
    xmlChar *content;
    HRESULT hres = S_FALSE;

    TRACE("(%p)->(%p)\n", This, typedValue);

    if(!typedValue)
        return E_INVALIDARG;

    V_VT(typedValue) = VT_NULL;

    if(This->node->type == XML_ELEMENT_NODE ||
            This->node->type == XML_TEXT_NODE ||
            This->node->type == XML_ENTITY_REF_NODE)
        hres = IXMLDOMNode_get_dataType(This->iface, &type);

    if(hres != S_OK && This->node->type != XML_ELEMENT_NODE)
        return IXMLDOMNode_get_nodeValue(This->iface, typedValue);

    content = xmlNodeGetContent(This->node);
    hres = VARIANT_from_xmlChar(content, typedValue,
            hres==S_OK ? V_BSTR(&type) : NULL);
    xmlFree(content);
    VariantClear(&type);

    return hres;
}

static HRESULT WINAPI xmlnode_put_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT typedValue)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_put_dataType(
    IXMLDOMNode *iface,
    BSTR dataTypeName)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    HRESULT hr = E_FAIL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(dataTypeName));

    if(dataTypeName == NULL)
        return E_INVALIDARG;

    /* An example of this is. The Text in the node needs to be a 0 or 1 for a boolean type.
       This applies to changing types (string->bool) or setting a new one
     */
    FIXME("Need to Validate the data before allowing a type to be set.\n");

    /* Check all supported types. */
    if(lstrcmpiW(dataTypeName,szString) == 0  ||
       lstrcmpiW(dataTypeName,szNumber) == 0  ||
       lstrcmpiW(dataTypeName,szUUID) == 0    ||
       lstrcmpiW(dataTypeName,szInt) == 0     ||
       lstrcmpiW(dataTypeName,szI4) == 0      ||
       lstrcmpiW(dataTypeName,szFixed) == 0   ||
       lstrcmpiW(dataTypeName,szBoolean) == 0 ||
       lstrcmpiW(dataTypeName,szDateTime) == 0 ||
       lstrcmpiW(dataTypeName,szDateTimeTZ) == 0 ||
       lstrcmpiW(dataTypeName,szDate) == 0    ||
       lstrcmpiW(dataTypeName,szTime) == 0    ||
       lstrcmpiW(dataTypeName,szTimeTZ) == 0  ||
       lstrcmpiW(dataTypeName,szI1) == 0      ||
       lstrcmpiW(dataTypeName,szI2) == 0      ||
       lstrcmpiW(dataTypeName,szIU1) == 0     ||
       lstrcmpiW(dataTypeName,szIU2) == 0     ||
       lstrcmpiW(dataTypeName,szIU4) == 0     ||
       lstrcmpiW(dataTypeName,szR4) == 0      ||
       lstrcmpiW(dataTypeName,szR8) == 0      ||
       lstrcmpiW(dataTypeName,szFloat) == 0   ||
       lstrcmpiW(dataTypeName,szBinHex) == 0  ||
       lstrcmpiW(dataTypeName,szBinBase64) == 0)
    {
        xmlChar* str = xmlChar_from_wchar(dataTypeName);
        xmlAttrPtr attr;

        if (!str) return E_OUTOFMEMORY;

        attr = xmlHasNsProp(This->node, (const xmlChar*)"dt",
                            (const xmlChar*)"urn:schemas-microsoft-com:datatypes");
        if (attr)
        {
            attr = xmlSetNsProp(This->node, attr->ns, (const xmlChar*)"dt", str);
            hr = S_OK;
        }
        else
        {
            xmlNsPtr ns = xmlNewNs(This->node, (const xmlChar*)"urn:schemas-microsoft-com:datatypes", (const xmlChar*)"dt");
            if (ns)
            {
                attr = xmlNewNsProp(This->node, ns, (const xmlChar*)"dt", str);
                if (attr)
                {
                    xmlAddChild(This->node, (xmlNodePtr)attr);
                    hr = S_OK;
                }
                else
                    ERR("Failed to create Attribute\n");
            }
            else
                ERR("Failed to create Namespace\n");
        }
        heap_free( str );
    }

    return hr;
}

static BSTR EnsureCorrectEOL(BSTR sInput)
{
    int nNum = 0;
    BSTR sNew;
    int nLen;
    int i;

    nLen = SysStringLen(sInput);
    /* Count line endings */
    for(i=0; i < nLen; i++)
    {
        if(sInput[i] == '\n')
            nNum++;
    }

    TRACE("len=%d, num=%d\n", nLen, nNum);

    /* Add linefeed as needed */
    if(nNum > 0)
    {
        int nPlace = 0;
        sNew = SysAllocStringLen(NULL, nLen + nNum+1);
        for(i=0; i < nLen; i++)
        {
            if(sInput[i] == '\n')
            {
                sNew[i+nPlace] = '\r';
                nPlace++;
            }
            sNew[i+nPlace] = sInput[i];
        }

        SysFreeString(sInput);
    }
    else
    {
        sNew = sInput;
    }

    TRACE("len %d\n", SysStringLen(sNew));

    return sNew;
}

/* Removes encoding information and last character (nullbyte) */
static BSTR EnsureNoEncoding(BSTR sInput)
{
    static const WCHAR wszEncoding[] = {'e','n','c','o','d','i','n','g','='};
    BSTR sNew;
    WCHAR *pBeg, *pEnd;

    pBeg = sInput;
    while(*pBeg != '\n' && memcmp(pBeg, wszEncoding, sizeof(wszEncoding)))
        pBeg++;

    if(*pBeg == '\n')
    {
        SysReAllocStringLen(&sInput, sInput, SysStringLen(sInput)-1);
        return sInput;
    }
    pBeg--;

    pEnd = pBeg + sizeof(wszEncoding)/sizeof(WCHAR) + 2;
    while(*pEnd != '\"') pEnd++;
    pEnd++;

    sNew = SysAllocStringLen(NULL,
            pBeg-sInput + SysStringLen(sInput)-(pEnd-sInput)-1);
    memcpy(sNew, sInput, (pBeg-sInput)*sizeof(WCHAR));
    memcpy(&sNew[pBeg-sInput], pEnd, (SysStringLen(sInput)-(pEnd-sInput)-1)*sizeof(WCHAR));

    SysFreeString(sInput);
    return sNew;
}

/*
 * We are trying to replicate the same behaviour as msxml by converting
 * line endings to \r\n and using indents as \t. The problem is that msxml
 * only formats nodes that have a line ending. Using libxml we cannot
 * reproduce behaviour exactly.
 *
 */
HRESULT node_get_xml(xmlnode *This, BOOL ensure_eol, BOOL ensure_no_encoding, BSTR *ret)
{
    xmlBufferPtr xml_buf;
    xmlNodePtr xmldecl;
    int size;

    if(!ret)
        return E_INVALIDARG;

    *ret = NULL;

    xml_buf = xmlBufferCreate();
    if(!xml_buf)
        return E_OUTOFMEMORY;

    xmldecl = xmldoc_unlink_xmldecl( This->node->doc );

    size = xmlNodeDump(xml_buf, This->node->doc, This->node, 0, 1);
    if(size > 0) {
        const xmlChar *buf_content;
        BSTR content;

        /* Attribute Nodes return a space in front of their name */
        buf_content = xmlBufferContent(xml_buf);

        content = bstr_from_xmlChar(buf_content + (buf_content[0] == ' ' ? 1 : 0));
        if(ensure_eol)
            content = EnsureCorrectEOL(content);
        if(ensure_no_encoding)
            content = EnsureNoEncoding(content);

        *ret = content;
    }else {
        *ret = SysAllocStringLen(NULL, 0);
    }

    xmlBufferFree(xml_buf);
    xmldoc_link_xmldecl( This->node->doc, xmldecl );
    return *ret ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI xmlnode_transformNode(
    IXMLDOMNode *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString)
{
#ifdef SONAME_LIBXSLT
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlnode *pStyleSheet = NULL;
    xsltStylesheetPtr xsltSS = NULL;
    xmlDocPtr result = NULL;

    TRACE("(%p)->(%p %p)\n", This, styleSheet, xmlString);

    if (!libxslt_handle)
        return E_NOTIMPL;
    if(!styleSheet || !xmlString)
        return E_INVALIDARG;

    *xmlString = NULL;

    pStyleSheet = get_node_obj(styleSheet);
    if(!pStyleSheet) {
        FIXME("styleSheet is not our xmlnode implementation\n");
        return E_FAIL;
    }

    xsltSS = pxsltParseStylesheetDoc( pStyleSheet->node->doc);
    if(xsltSS)
    {
        result = pxsltApplyStylesheet(xsltSS, This->node->doc, NULL);
        if(result)
        {
            const xmlChar *pContent;

            if(result->type == XML_HTML_DOCUMENT_NODE)
            {
                xmlOutputBufferPtr	pOutput = xmlAllocOutputBuffer(NULL);
                if(pOutput)
                {
                    htmlDocContentDumpOutput(pOutput, result->doc, NULL);
                    pContent = xmlBufferContent(pOutput->buffer);
                    *xmlString = bstr_from_xmlChar(pContent);
                    xmlOutputBufferClose(pOutput);
                }
            }
            else
            {
                xmlBufferPtr pXmlBuf;
                int nSize;

                pXmlBuf = xmlBufferCreate();
                if(pXmlBuf)
                {
                    nSize = xmlNodeDump(pXmlBuf, NULL, (xmlNodePtr)result, 0, 0);
                    if(nSize > 0)
                    {
                        pContent = xmlBufferContent(pXmlBuf);
                        *xmlString = bstr_from_xmlChar(pContent);
                    }
                    xmlBufferFree(pXmlBuf);
                }
            }
            xmlFreeDoc(result);
        }
        /* libxslt "helpfully" frees the XML document the stylesheet was
           generated from, too */
        xsltSS->doc = NULL;
        pxsltFreeStylesheet(xsltSS);
    }

    if(*xmlString == NULL)
        *xmlString = SysAllocStringLen(NULL, 0);

    return S_OK;
#else
    FIXME("libxslt headers were not found at compile time\n");
    return E_NOTIMPL;
#endif
}

static HRESULT WINAPI xmlnode_selectNodes(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(queryString), resultList );

    if (!queryString || !resultList) return E_INVALIDARG;

    return queryresult_create( This->node, queryString, resultList );
}

static HRESULT WINAPI xmlnode_selectSingleNode(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    IXMLDOMNodeList *list;
    HRESULT r;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(queryString), resultNode );

    r = IXMLDOMNode_selectNodes(This->iface, queryString, &list);
    if(r == S_OK)
    {
        r = IXMLDOMNodeList_nextNode(list, resultNode);
        IXMLDOMNodeList_Release(list);
    }
    return r;
}

static HRESULT WINAPI xmlnode_get_namespaceURI(
    IXMLDOMNode *iface,
    BSTR* namespaceURI)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlNsPtr *ns;

    TRACE("(%p)->(%p)\n", This, namespaceURI );

    if(!namespaceURI)
        return E_INVALIDARG;

    *namespaceURI = NULL;

    if ((ns = xmlGetNsList(This->node->doc, This->node)))
    {
        if (ns[0]->href) *namespaceURI = bstr_from_xmlChar( ns[0]->href );
        xmlFree(ns);
    }

    TRACE("uri: %s\n", debugstr_w(*namespaceURI));

    return *namespaceURI ? S_OK : S_FALSE;
}

HRESULT node_get_prefix(xmlnode *This, BSTR *prefix)
{
    xmlNsPtr *ns;

    if (!prefix) return E_INVALIDARG;

    *prefix = NULL;

    if ((ns = xmlGetNsList(This->node->doc, This->node)))
    {
        if (ns[0]->prefix) *prefix = bstr_from_xmlChar( ns[0]->prefix );
        xmlFree(ns);
    }

    TRACE("prefix: %s\n", debugstr_w(*prefix));

    return *prefix ? S_OK : S_FALSE;
}

HRESULT node_get_base_name(xmlnode *This, BSTR *name)
{
    if (!name) return E_INVALIDARG;

    *name = bstr_from_xmlChar(This->node->name);
    if (!*name) return E_OUTOFMEMORY;

    TRACE("returning %s\n", debugstr_w(*name));

    return S_OK;
}

static HRESULT WINAPI xmlnode_transformNodeToObject(
    IXMLDOMNode *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, stylesheet);
    return E_NOTIMPL;
}

static const struct IXMLDOMNodeVtbl xmlnode_vtbl =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    xmlnode_removeChild,
    xmlnode_appendChild,
    xmlnode_hasChildNodes,
    xmlnode_get_ownerDocument,
    NULL,
    xmlnode_get_nodeTypeString,
    xmlnode_get_text,
    NULL,
    NULL,
    NULL,
    xmlnode_get_nodeTypedValue,
    xmlnode_put_nodeTypedValue,
    NULL,
    xmlnode_put_dataType,
    NULL,
    xmlnode_transformNode,
    xmlnode_selectNodes,
    xmlnode_selectSingleNode,
    NULL,
    xmlnode_get_namespaceURI,
    NULL,
    NULL,
    xmlnode_transformNodeToObject,
};

void destroy_xmlnode(xmlnode *This)
{
    if(This->node)
        xmldoc_release(This->node->doc);
}

void init_xmlnode(xmlnode *This, xmlNodePtr node, IXMLDOMNode *node_iface, dispex_static_data_t *dispex_data)
{
    if(node)
        xmldoc_add_ref( node->doc );

    This->lpVtbl = &xmlnode_vtbl;
    This->node = node;
    This->iface = node_iface;

    if(dispex_data)
        init_dispex(&This->dispex, (IUnknown*)This->iface, dispex_data);
    else
        This->dispex.outer = NULL;
}

typedef struct {
    xmlnode node;
    const IXMLDOMNodeVtbl *lpVtbl;
    LONG ref;
} unknode;

static inline unknode *impl_from_unkIXMLDOMNode(IXMLDOMNode *iface)
{
    return (unknode *)((char*)iface - FIELD_OFFSET(unknode, lpVtbl));
}

static HRESULT WINAPI unknode_QueryInterface(
    IXMLDOMNode *iface,
    REFIID riid,
    void** ppvObject )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown)) {
        *ppvObject = iface;
    }else if (IsEqualGUID( riid, &IID_IDispatch) ||
              IsEqualGUID( riid, &IID_IXMLDOMNode)) {
        *ppvObject = &This->lpVtbl;
    }else if(node_query_interface(&This->node, riid, ppvObject)) {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }else  {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI unknode_AddRef(
    IXMLDOMNode *iface )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI unknode_Release(
    IXMLDOMNode *iface )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    LONG ref;

    ref = InterlockedDecrement( &This->ref );
    if(!ref) {
        destroy_xmlnode(&This->node);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI unknode_GetTypeInfoCount(
    IXMLDOMNode *iface,
    UINT* pctinfo )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI unknode_GetTypeInfo(
    IXMLDOMNode *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMNode_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI unknode_GetIDsOfNames(
    IXMLDOMNode *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMNode_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI unknode_Invoke(
    IXMLDOMNode *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMNode_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI unknode_get_nodeName(
    IXMLDOMNode *iface,
    BSTR* p )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI unknode_get_nodeValue(
    IXMLDOMNode *iface,
    VARIANT* value)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, value);

    if(!value)
        return E_INVALIDARG;

    V_VT(value) = VT_NULL;
    return S_FALSE;
}

static HRESULT WINAPI unknode_put_nodeValue(
    IXMLDOMNode *iface,
    VARIANT value)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    FIXME("(%p)->(v%d)\n", This, V_VT(&value));
    return E_FAIL;
}

static HRESULT WINAPI unknode_get_nodeType(
    IXMLDOMNode *iface,
    DOMNodeType* domNodeType )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = This->node.node->type;
    return S_OK;
}

static HRESULT WINAPI unknode_get_parentNode(
    IXMLDOMNode *iface,
    IXMLDOMNode** parent )
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, parent);
    if (!parent) return E_INVALIDARG;
    *parent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI unknode_get_childNodes(
    IXMLDOMNode *iface,
    IXMLDOMNodeList** outList)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI unknode_get_firstChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_lastChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_previousSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_nextSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_attributes(
    IXMLDOMNode *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI unknode_insertBefore(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p x%d %p)\n", This, newNode, V_VT(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI unknode_replaceChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI unknode_removeChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), domNode, oldNode );
}

static HRESULT WINAPI unknode_appendChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newNode, outNewNode );
}

static HRESULT WINAPI unknode_hasChildNodes(
    IXMLDOMNode *iface,
    VARIANT_BOOL* pbool)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI unknode_get_ownerDocument(
    IXMLDOMNode *iface,
    IXMLDOMDocument** domDocument)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), domDocument );
}

static HRESULT WINAPI unknode_cloneNode(
    IXMLDOMNode *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_cloneNode( IXMLDOMNode_from_impl(&This->node), pbool, outNode );
}

static HRESULT WINAPI unknode_get_nodeTypeString(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_get_text(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_put_text(
    IXMLDOMNode *iface,
    BSTR p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_put_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_get_specified(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isSpecified)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI unknode_get_definition(
    IXMLDOMNode *iface,
    IXMLDOMNode** definitionNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI unknode_get_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* var1)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI unknode_put_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT var1)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI unknode_get_dataType(
    IXMLDOMNode *iface,
    VARIANT* var1)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_dataType( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI unknode_put_dataType(
    IXMLDOMNode *iface,
    BSTR p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_get_xml(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, FALSE, p);
}

static HRESULT WINAPI unknode_transformNode(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), domNode, p );
}

static HRESULT WINAPI unknode_selectNodes(
    IXMLDOMNode *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), p, outList );
}

static HRESULT WINAPI unknode_selectSingleNode(
    IXMLDOMNode *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), p, outNode );
}

static HRESULT WINAPI unknode_get_parsed(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isParsed)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI unknode_get_namespaceURI(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_get_prefix(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_prefix( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_get_baseName(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_get_baseName( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI unknode_transformNodeToObject(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    unknode *This = impl_from_unkIXMLDOMNode( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), domNode, var1 );
}

static const struct IXMLDOMNodeVtbl unknode_vtbl =
{
    unknode_QueryInterface,
    unknode_AddRef,
    unknode_Release,
    unknode_GetTypeInfoCount,
    unknode_GetTypeInfo,
    unknode_GetIDsOfNames,
    unknode_Invoke,
    unknode_get_nodeName,
    unknode_get_nodeValue,
    unknode_put_nodeValue,
    unknode_get_nodeType,
    unknode_get_parentNode,
    unknode_get_childNodes,
    unknode_get_firstChild,
    unknode_get_lastChild,
    unknode_get_previousSibling,
    unknode_get_nextSibling,
    unknode_get_attributes,
    unknode_insertBefore,
    unknode_replaceChild,
    unknode_removeChild,
    unknode_appendChild,
    unknode_hasChildNodes,
    unknode_get_ownerDocument,
    unknode_cloneNode,
    unknode_get_nodeTypeString,
    unknode_get_text,
    unknode_put_text,
    unknode_get_specified,
    unknode_get_definition,
    unknode_get_nodeTypedValue,
    unknode_put_nodeTypedValue,
    unknode_get_dataType,
    unknode_put_dataType,
    unknode_get_xml,
    unknode_transformNode,
    unknode_selectNodes,
    unknode_selectSingleNode,
    unknode_get_parsed,
    unknode_get_namespaceURI,
    unknode_get_prefix,
    unknode_get_baseName,
    unknode_transformNodeToObject
};

IXMLDOMNode *create_node( xmlNodePtr node )
{
    IUnknown *pUnk;
    IXMLDOMNode *ret;
    HRESULT hr;

    if ( !node )
        return NULL;

    TRACE("type %d\n", node->type);
    switch(node->type)
    {
    case XML_ELEMENT_NODE:
        pUnk = create_element( node );
        break;
    case XML_ATTRIBUTE_NODE:
        pUnk = create_attribute( node );
        break;
    case XML_TEXT_NODE:
        pUnk = create_text( node );
        break;
    case XML_CDATA_SECTION_NODE:
        pUnk = create_cdata( node );
        break;
    case XML_ENTITY_REF_NODE:
        pUnk = create_doc_entity_ref( node );
        break;
    case XML_PI_NODE:
        pUnk = create_pi( node );
        break;
    case XML_COMMENT_NODE:
        pUnk = create_comment( node );
        break;
    case XML_DOCUMENT_NODE:
        pUnk = create_domdoc( node );
        break;
    case XML_DOCUMENT_FRAG_NODE:
        pUnk = create_doc_fragment( node );
        break;
    case XML_DTD_NODE:
        pUnk = create_doc_type( node );
        break;
    default: {
        unknode *new_node;

        FIXME("only creating basic node for type %d\n", node->type);

        new_node = heap_alloc(sizeof(unknode));
        if(!new_node)
            return NULL;

        new_node->lpVtbl = &unknode_vtbl;
        new_node->ref = 1;
        init_xmlnode(&new_node->node, node, (IXMLDOMNode*)&new_node->lpVtbl, NULL);
        pUnk = (IUnknown*)&new_node->lpVtbl;
    }
    }

    hr = IUnknown_QueryInterface(pUnk, &IID_IXMLDOMNode, (LPVOID*)&ret);
    IUnknown_Release(pUnk);
    if(FAILED(hr)) return NULL;
    return ret;
}
#endif
