/*
 * XML Document implementation
 *
 * Copyright 2007 James Hawkins
 * Copyright 2025 Nikolay Sivov
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

#include <stdarg.h>
#include <stdbool.h>
#include <libxml/parser.h>

#include "windef.h"
#include "winbase.h"
#include "msxml2.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct attribute
{
    struct list entry;
    BSTR name;
    BSTR value;
};

struct node
{
    struct list entry;
    LONG refcount;
    BSTR name;
    BSTR data;
    LONG type;

    struct node *parent;
    struct list children;
    struct list attributes;
};

static struct node *_node_grab(struct node *node)
{
    ++node->refcount;
    return node;
}

static void _node_release(struct node *node);
static void _node_release_attribute(struct attribute *attr);

static HRESULT _node_remove_child(struct node *parent, struct node *child)
{
    if (parent != child->parent)
        return E_INVALIDARG;

    list_remove(&child->entry);
    child->parent = NULL;
    _node_release(child);

    return S_OK;
}

static void _node_release(struct node *node)
{
    struct attribute *attr, *next_attr;
    struct node *child, *next_child;

    if (!node || --node->refcount) return;

    /* At this point node should be detached. */
    LIST_FOR_EACH_ENTRY_SAFE(child, next_child, &node->children, struct node, entry)
    {
        _node_remove_child(node, child);
    }

    LIST_FOR_EACH_ENTRY_SAFE(attr, next_attr, &node->attributes, struct attribute, entry)
    {
        _node_release_attribute(attr);
    }

    SysFreeString(node->name);
    free(node);
}

static struct node *_node_get_child(struct node *node, LONG index)
{
    struct node *child = NULL;

    if (list_empty(&node->children)) return NULL;

    LIST_FOR_EACH_ENTRY(child, &node->children, struct node, entry)
    {
        if (--index == -1) break;
    }

    return index > -1 ? NULL : _node_grab(child);
}

static void _node_append_child(struct node *parent, struct node *child)
{
    /* TODO: assert, at this point node should be detached. */

    child->parent = parent;
    list_add_tail(&parent->children, &child->entry);
    _node_grab(child);
}

static void _node_prepend_child(struct node *parent, struct node *child)
{
    /* TODO: assert, at this point node should be detached. */

    child->parent = parent;
    list_add_head(&parent->children, &child->entry);
    _node_grab(child);
}

static void _node_add_child(struct node *parent, struct node *child)
{
    _node_grab(child);
    if (child->parent)
        _node_remove_child(child->parent, child);
    _node_prepend_child(parent, child);
    _node_release(child);
}

static HRESULT _node_get_tagname(struct node *node, BSTR *p)
{
    HRESULT hr = E_NOTIMPL;

    if (!p)
        return E_INVALIDARG;

    if (node->type == XMLELEMTYPE_ELEMENT || node->type == XMLELEMTYPE_PI)
    {
        hr = return_bstr(node->name, p);
        /* TODO: misplaced */
        if (SUCCEEDED(hr))
            CharUpperBuffW(*p, SysStringLen(*p));
    }
    else
    {
        *p = NULL;
    }

    return hr;
}

static HRESULT _node_put_tagname(struct node *node, BSTR p)
{
    SysFreeString(node->name);
    return return_bstr(p, &node->name);
}

static void _node_release_attribute(struct attribute *attr)
{
    list_remove(&attr->entry);
    SysFreeString(attr->name);
    SysFreeString(attr->value);
    free(attr);
}

static struct attribute *_node_get_attribute(struct node *node, const WCHAR *name)
{
    struct attribute *attr;

    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct attribute, entry)
    {
        if (!wcsicmp(attr->name, name))
            return attr;
    }

    return NULL;
}

static HRESULT _node_remove_attribute(struct node *node, const WCHAR *name)
{
    struct attribute *attr;

    if (!name)
        return E_INVALIDARG;

    if (!(attr = _node_get_attribute(node, name)))
        return S_FALSE;

    _node_release_attribute(attr);
    return S_OK;
}

static HRESULT _node_append_attribute(struct node *node, const WCHAR *name, int n_len,
        const WCHAR *value, int v_len)
{
    struct attribute *attr;

    if (!(attr = calloc(1, sizeof(*attr))))
        return E_OUTOFMEMORY;
    attr->name = n_len == -1 ? SysAllocString(name) : SysAllocStringLen(name, n_len);
    attr->value = v_len == -1 ? SysAllocString(value) : SysAllocStringLen(value, v_len);
    list_add_tail(&node->attributes, &attr->entry);

    return S_OK;
}

static HRESULT node_set_attribute_value(struct node *node, const WCHAR *name, const VARIANT *value)
{
    struct attribute *attr;

    if (!name || V_VT(value) != VT_BSTR)
        return E_INVALIDARG;

    if (!(attr = _node_get_attribute(node, name)))
        return _node_append_attribute(node, name, -1, V_BSTR(value), SysStringLen(V_BSTR(value)));

    SysFreeString(attr->value);
    return return_bstr(V_BSTR(value), &attr->value);
}

/* TODO: add a test for xml:lang */
static HRESULT node_get_attribute_value(struct node *node, const WCHAR *name, VARIANT *value)
{
    struct attribute *attr;

    if (!value)
        return E_INVALIDARG;

    VariantInit(value);

    if (!(attr = _node_get_attribute(node, name)))
        return S_FALSE;

    V_VT(value) = VT_BSTR;
    return return_bstr(attr->value, &V_BSTR(value));
}

static HRESULT node_create(const WCHAR *name, int name_len, LONG type, struct node **ret)
{
    struct node *node;

    *ret = NULL;

    if (!(node = calloc(1, sizeof(*node))))
        return E_OUTOFMEMORY;

    node->refcount = 1;
    node->name = SysAllocStringLen(name, name_len);
    node->type = type;
    list_init(&node->children);
    list_init(&node->attributes);

    *ret = node;

    return S_OK;
}

enum parse_text_type
{
    PARSE_TEXT_TYPE_NONE,
    PARSE_TEXT_TYPE_CDATA,
    PARSE_TEXT_TYPE_TEXT,
};

struct buffer
{
    WCHAR *text;
    size_t count;
    size_t size;
};

struct parse_context
{
    ISAXContentHandler content_handler;
    ISAXLexicalHandler lexical_handler;
    ISAXXMLReader *reader;

    struct node *node;

    struct buffer buffer;
    enum parse_text_type text_type;

    /* Parsed output */
    struct node *root;
    struct
    {
        BSTR name;
    } dtd;
};

static bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
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

static HRESULT text_buffer_append(struct buffer *buffer, const WCHAR *chars, int count)
{
    if (!count)
        return S_OK;

    if (!array_reserve((void **)&buffer->text, &buffer->size, buffer->count + count, sizeof(*buffer->text)))
        return E_OUTOFMEMORY;

    memcpy(buffer->text + buffer->count, chars, count * sizeof(*chars));
    buffer->count += count;

    return S_OK;
}

static void node_dump_text(const struct node *node, struct buffer *buffer)
{
    struct node *child;

    LIST_FOR_EACH_ENTRY(child, &node->children, struct node, entry)
    {
        if (child->type == XMLELEMTYPE_TEXT)
        {
            text_buffer_append(buffer, child->data, SysStringLen(child->data));
            continue;
        }
        node_dump_text(child, buffer);
    }
}

static HRESULT _node_get_text(struct node *node, BSTR *p)
{
    struct buffer buffer = { 0 };

    if (!p)
        return E_INVALIDARG;

    if (node->type == XMLELEMTYPE_ELEMENT)
    {
        node_dump_text(node, &buffer);
        *p = SysAllocStringLen(buffer.text, buffer.count);
        free(buffer.text);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    return return_bstr(node->data ? node->data : L"", p);
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static bool is_whitespace(const WCHAR *chars, size_t count)
{
    while (count--)
    {
        if (!(*chars == 0x20 || *chars == 0x9 || *chars == 0xd || *chars == 0xa))
            return false;
        chars++;
    }

    return true;
}

static HRESULT parse_create_text_node(struct parse_context *c)
{
    struct node *node;
    HRESULT hr;

    if (!c->node)
        return E_UNEXPECTED;

    if (c->text_type == PARSE_TEXT_TYPE_NONE)
        return S_OK;

    if (c->text_type == PARSE_TEXT_TYPE_TEXT)
    {
        if (is_whitespace(c->buffer.text, c->buffer.count))
        {
            c->buffer.count = 0;
            c->text_type = PARSE_TEXT_TYPE_NONE;
            return S_OK;
        }
    }

    if (FAILED(hr = node_create(L"", 0, XMLELEMTYPE_TEXT, &node)))
        return hr;
    node->data = SysAllocStringLen(c->buffer.text, c->buffer.count);

    _node_append_child(c->node, node);
    _node_release(node);

    c->buffer.count = 0;
    c->text_type = PARSE_TEXT_TYPE_NONE;

    return hr;
}

static struct parse_context *impl_from_ISAXContentHandler(ISAXContentHandler *iface)
{
    return CONTAINING_RECORD(iface, struct parse_context, content_handler);
}

static HRESULT WINAPI parse_content_handler_QueryInterface(ISAXContentHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_ISAXContentHandler)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ISAXContentHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI parse_content_handler_AddRef(ISAXContentHandler *iface)
{
    return 2;
}

static ULONG WINAPI parse_content_handler_Release(ISAXContentHandler *iface)
{
    return 1;
}

static HRESULT WINAPI parse_content_handler_putDocumentLocator(ISAXContentHandler *iface,
ISAXLocator *locator)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_startDocument(ISAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_endDocument(ISAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_startPrefixMapping(ISAXContentHandler *iface,
        const WCHAR *prefix, int prefix_len, const WCHAR *uri, int uri_len)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_endPrefixMapping(ISAXContentHandler *iface,
        const WCHAR *prefix, int prefix_len)
{
    return S_OK;
}

static void node_set_attributes(struct node *node, ISAXAttributes *attrs)
{
    int count, n_len, v_len;
    const WCHAR *n, *v;

    ISAXAttributes_getLength(attrs, &count);

    for (int i = 0; i < count; ++i)
    {
        ISAXAttributes_getQName(attrs, i, &n, &n_len);
        ISAXAttributes_getValue(attrs, i, &v, &v_len);

        _node_append_attribute(node, n, n_len, v, v_len);
    }
}

static HRESULT WINAPI parse_content_handler_startElement(ISAXContentHandler *iface, const WCHAR *uri, int uri_len,
        const WCHAR *name, int name_len, const WCHAR *qname, int qname_len, ISAXAttributes *attrs)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);
    struct node *node;
    HRESULT hr;

    hr = parse_create_text_node(c);

    if (FAILED(hr = node_create(qname, qname_len, XMLELEMTYPE_ELEMENT, &node)))
        return hr;

    if (c->node)
    {
        _node_append_child(c->node, node);
        _node_release(node);
    }

    if (!c->root)
        c->root = node;

    c->node = node;
    if (attrs)
        node_set_attributes(node, attrs);

    return S_OK;
}

static HRESULT WINAPI parse_content_handler_endElement(ISAXContentHandler *iface, const WCHAR *uri, int uri_len,
        const WCHAR *name, int name_len, const WCHAR *qname, int qname_len)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);
    HRESULT hr;

    if (FAILED(hr = parse_create_text_node(c))) return hr;
    c->node = c->node->parent;
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_characters(ISAXContentHandler *iface, const WCHAR *chars, int count)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);

    if (!c->node)
        return E_UNEXPECTED;

    if (c->text_type == PARSE_TEXT_TYPE_NONE)
        c->text_type = PARSE_TEXT_TYPE_TEXT;

    return text_buffer_append(&c->buffer, chars, count);
}

static HRESULT WINAPI parse_content_handler_ignorableWhitespace(ISAXContentHandler *iface, const WCHAR *chars, int count)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_processingInstruction(ISAXContentHandler *iface, const WCHAR *target,
        int target_len, const WCHAR *data, int data_len)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);
    struct node *pi, *text;
    HRESULT hr;

    if (!c->node)
        return E_UNEXPECTED;

    if (FAILED(hr = parse_create_text_node(c)))
        return hr;

    /* TODO: error path */
    if (FAILED(hr = node_create(target, target_len, XMLELEMTYPE_PI, &pi)))
        return hr;

    hr = node_create(L"", 0, XMLELEMTYPE_TEXT, &text);
    text->data = SysAllocStringLen(data, data_len);

    _node_append_child(c->node, pi);
    _node_append_child(c->node, text);
    _node_release(pi);
    _node_release(text);

    return S_OK;
}

static HRESULT WINAPI parse_content_handler_skippedEntity(ISAXContentHandler *iface, const WCHAR *name, int name_len)
{
    return S_OK;
}

static const ISAXContentHandlerVtbl parse_content_handler_vtbl =
{
    parse_content_handler_QueryInterface,
    parse_content_handler_AddRef,
    parse_content_handler_Release,
    parse_content_handler_putDocumentLocator,
    parse_content_handler_startDocument,
    parse_content_handler_endDocument,
    parse_content_handler_startPrefixMapping,
    parse_content_handler_endPrefixMapping,
    parse_content_handler_startElement,
    parse_content_handler_endElement,
    parse_content_handler_characters,
    parse_content_handler_ignorableWhitespace,
    parse_content_handler_processingInstruction,
    parse_content_handler_skippedEntity,
};

static struct parse_context *impl_from_ISAXLexicalHandler(ISAXLexicalHandler *iface)
{
    return CONTAINING_RECORD(iface, struct parse_context, lexical_handler);
}

static HRESULT WINAPI parse_lexical_handler_QueryInterface(ISAXLexicalHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_ISAXLexicalHandler)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ISAXLexicalHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI parse_lexical_handler_AddRef(ISAXLexicalHandler *iface)
{
    return 2;
}

static ULONG WINAPI parse_lexical_handler_Release(ISAXLexicalHandler *iface)
{
    return 1;
}

static HRESULT WINAPI parse_lexical_handler_startDTD(ISAXLexicalHandler *iface,
        const WCHAR *name, int name_len, const WCHAR *pubid, int pubid_len,
        const WCHAR *sysid, int sysid_len)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);

    SysFreeString(c->dtd.name);
    c->dtd.name = SysAllocStringLen(name, name_len);
    CharUpperBuffW(c->dtd.name, SysStringLen(c->dtd.name));

    return S_OK;
}

static HRESULT WINAPI parse_lexical_handler_endDTD(ISAXLexicalHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI parse_lexical_handler_startEntity(ISAXLexicalHandler *iface,
        const WCHAR *name, int name_len)
{
    return S_OK;
}

static HRESULT WINAPI parse_lexical_handler_endEntity(ISAXLexicalHandler *iface,
        const WCHAR *name, int name_len)
{
    return S_OK;
}

static HRESULT WINAPI parse_lexical_handler_startCDATA(ISAXLexicalHandler *iface)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);
    HRESULT hr;

    if (FAILED(hr = parse_create_text_node(c))) return hr;
    c->text_type = PARSE_TEXT_TYPE_CDATA;
    return S_OK;
}

static HRESULT WINAPI parse_lexical_handler_endCDATA(ISAXLexicalHandler *iface)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);
    return parse_create_text_node(c);
}

static HRESULT WINAPI parse_lexical_handler_comment(ISAXLexicalHandler *iface, const WCHAR *chars, int count)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);
    struct node *node;
    HRESULT hr;

    if (!c->node)
        return E_UNEXPECTED;

    hr = parse_create_text_node(c);

    if (FAILED(hr = node_create(L"", 0, XMLELEMTYPE_COMMENT, &node)))
        return hr;
    node->data = SysAllocStringLen(chars, count);

    _node_append_child(c->node, node);
    _node_release(node);

    return S_OK;
}

static const ISAXLexicalHandlerVtbl parse_lexical_handler_vtbl =
{
    parse_lexical_handler_QueryInterface,
    parse_lexical_handler_AddRef,
    parse_lexical_handler_Release,
    parse_lexical_handler_startDTD,
    parse_lexical_handler_endDTD,
    parse_lexical_handler_startEntity,
    parse_lexical_handler_endEntity,
    parse_lexical_handler_startCDATA,
    parse_lexical_handler_endCDATA,
    parse_lexical_handler_comment,
};

static HRESULT parse_context_init(struct parse_context *c)
{
    IUnknown *unk;
    HRESULT hr;
    VARIANT v;

    memset(c, 0, sizeof(*c));
    c->content_handler.lpVtbl = &parse_content_handler_vtbl;
    c->lexical_handler.lpVtbl = &parse_lexical_handler_vtbl;

    if (FAILED(hr = SAXXMLReader_create(MSXML3, (void **)&unk)))
        return hr;

    IUnknown_QueryInterface(unk, &IID_ISAXXMLReader, (void **)&c->reader);
    IUnknown_Release(unk);

    ISAXXMLReader_putContentHandler(c->reader, &c->content_handler);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)&c->lexical_handler;
    ISAXXMLReader_putProperty(c->reader, L"http://xml.org/sax/properties/lexical-handler", v);

    return hr;
}

static void parse_context_cleanup(struct parse_context *c)
{
    _node_release(c->root);
    SysFreeString(c->dtd.name);
    free(c->buffer.text);
    if (c->reader)
        ISAXXMLReader_Release(c->reader);
}

static HRESULT xmlelementcollection_create(struct node *node, IXMLElementCollection **ret);

/**********************************************************************
 * IXMLElement2
 */
typedef struct _xmlelem
{
    IXMLElement2 IXMLElement2_iface;
    LONG ref;
    struct node *node;
} xmlelem;

static inline xmlelem *impl_from_IXMLElement2(IXMLElement2 *iface)
{
    return CONTAINING_RECORD(iface, xmlelem, IXMLElement2_iface);
}

static HRESULT WINAPI xmlelem_QueryInterface(IXMLElement2 *iface, REFIID riid, void** ppvObject)
{
    xmlelem *This = impl_from_IXMLElement2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IXMLElement) ||
        IsEqualGUID(riid, &IID_IXMLElement2))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLElement2_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlelem_AddRef(IXMLElement2 *iface)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlelem_Release(IXMLElement2 *iface)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        _node_release(This->node);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI xmlelem_GetTypeInfoCount(IXMLElement2 *iface, UINT* pctinfo)
{
    TRACE("%p, %p.\n", iface, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI xmlelem_GetTypeInfo(IXMLElement2 *iface, UINT iTInfo,
                                          LCID lcid, ITypeInfo** ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IXMLElement2_tid, ppTInfo);
}

static HRESULT WINAPI xmlelem_GetIDsOfNames(IXMLElement2 *iface, REFIID riid,
                                            LPOLESTR* rgszNames, UINT cNames,
                                            LCID lcid, DISPID* rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLElement2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmlelem_Invoke(IXMLElement2 *iface, DISPID dispIdMember,
                                     REFIID riid, LCID lcid, WORD wFlags,
                                     DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                     EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLElement2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmlelem_get_tagName(IXMLElement2 *iface, BSTR *p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %p.\n", iface, p);

    return _node_get_tagname(element->node, p);
}

static HRESULT WINAPI xmlelem_put_tagName(IXMLElement2 *iface, BSTR p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    return _node_put_tagname(element->node, p);
}

static HRESULT xmlelement_create(struct node *node, IXMLElement2 **ret);

static HRESULT WINAPI xmlelem_get_parent(IXMLElement2 *iface, IXMLElement2 **parent)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %p.\n", iface, parent);

    if (!parent)
        return E_INVALIDARG;

    *parent = NULL;

    if (!element->node->parent)
        return S_FALSE;

    return xmlelement_create(element->node->parent, parent);
}

static HRESULT WINAPI xmlelem_setAttribute(IXMLElement2 *iface, BSTR name, VARIANT value)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_w(name), debugstr_variant(&value));

    return node_set_attribute_value(element->node, name, &value);
}

static HRESULT WINAPI xmlelem_getAttribute(IXMLElement2 *iface, BSTR name,
    VARIANT *value)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), value);

    return node_get_attribute_value(element->node, name, value);
}

static HRESULT WINAPI xmlelem_removeAttribute(IXMLElement2 *iface, BSTR name)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(name));

    return _node_remove_attribute(element->node, name);
}

static HRESULT WINAPI xmlelem_get_children(IXMLElement2 *iface, IXMLElementCollection **p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    return xmlelementcollection_create(element->node, p);
}

static HRESULT WINAPI xmlelem_get_type(IXMLElement2 *iface, LONG *p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = element->node->type;
    return S_OK;
}

static HRESULT WINAPI xmlelem_get_text(IXMLElement2 *iface, BSTR *p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %p.\n", iface, p);

    return _node_get_text(element->node, p);
}

static HRESULT WINAPI xmlelem_put_text(IXMLElement2 *iface, BSTR p)
{
    xmlelem *element = impl_from_IXMLElement2(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (element->node->type == XMLELEMTYPE_ELEMENT)
        return E_NOTIMPL;

    SysFreeString(element->node->data);
    element->node->data = NULL;
    return return_bstr(p, &element->node->data);
}

static HRESULT WINAPI xmlelem_addChild(IXMLElement2 *iface, IXMLElement2 *child_iface,
        LONG index, LONG reserved)
{
    xmlelem *element = impl_from_IXMLElement2(iface);
    xmlelem *child = impl_from_IXMLElement2(child_iface);

    TRACE("%p, %p, %ld, %ld.\n", iface, child_iface, index, reserved);

    /* TODO: test what index argument does */
    _node_add_child(element->node, child->node);
    return S_OK;
}

static HRESULT WINAPI xmlelem_removeChild(IXMLElement2 *iface, IXMLElement2 *child_iface)
{
    xmlelem *element = impl_from_IXMLElement2(iface);
    xmlelem *child = impl_from_IXMLElement2(child_iface);

    TRACE("%p, %p.\n", iface, child_iface);

    if (!child_iface)
        return E_INVALIDARG;

    return _node_remove_child(element->node, child->node);
}

static HRESULT WINAPI xmlelem_attributes(IXMLElement2 *iface, IXMLElementCollection **p)
{
    FIXME("%p, %p stub\n", iface, p);

    return E_NOTIMPL;
}

static const struct IXMLElement2Vtbl xmlelem_vtbl =
{
    xmlelem_QueryInterface,
    xmlelem_AddRef,
    xmlelem_Release,
    xmlelem_GetTypeInfoCount,
    xmlelem_GetTypeInfo,
    xmlelem_GetIDsOfNames,
    xmlelem_Invoke,
    xmlelem_get_tagName,
    xmlelem_put_tagName,
    xmlelem_get_parent,
    xmlelem_setAttribute,
    xmlelem_getAttribute,
    xmlelem_removeAttribute,
    xmlelem_get_children,
    xmlelem_get_type,
    xmlelem_get_text,
    xmlelem_put_text,
    xmlelem_addChild,
    xmlelem_removeChild,
    xmlelem_attributes,
};

static HRESULT xmlelement_create(struct node *node, IXMLElement2 **ret)
{
    xmlelem *elem;

    TRACE("%p\n", ret);

    if (!ret)
        return E_INVALIDARG;

    *ret = NULL;

    if (!(elem = calloc(1, sizeof(*elem))))
        return E_OUTOFMEMORY;

    elem->IXMLElement2_iface.lpVtbl = &xmlelem_vtbl;
    elem->ref = 1;
    elem->node = _node_grab(node);

    *ret = &elem->IXMLElement2_iface;

    TRACE("returning iface %p\n", *ret);
    return S_OK;
}

/************************************************************************
 * IXMLElementCollection
 */
typedef struct _xmlelem_collection
{
    IXMLElementCollection IXMLElementCollection_iface;
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;
    LONG length;
    struct node *node;

    LONG current;
} xmlelem_collection;

static inline xmlelem_collection *impl_from_IXMLElementCollection(IXMLElementCollection *iface)
{
    return CONTAINING_RECORD(iface, xmlelem_collection, IXMLElementCollection_iface);
}

static inline xmlelem_collection *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, xmlelem_collection, IEnumVARIANT_iface);
}

static HRESULT WINAPI xmlelem_collection_QueryInterface(IXMLElementCollection *iface, REFIID riid, void** ppvObject)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXMLElementCollection))
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        *ppvObject = &This->IEnumVARIANT_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLElementCollection_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlelem_collection_AddRef(IXMLElementCollection *iface)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlelem_collection_Release(IXMLElementCollection *iface)
{
    xmlelem_collection *collection = impl_from_IXMLElementCollection(iface);
    LONG ref = InterlockedDecrement(&collection->ref);

    TRACE("%p\n", iface);

    if (!ref)
    {
        _node_release(collection->node);
        free(collection);
    }

    return ref;
}

static HRESULT WINAPI xmlelem_collection_GetTypeInfoCount(IXMLElementCollection *iface, UINT* pctinfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_GetTypeInfo(IXMLElementCollection *iface, UINT iTInfo,
                                                     LCID lcid, ITypeInfo** ppTInfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_GetIDsOfNames(IXMLElementCollection *iface, REFIID riid,
                                                       LPOLESTR* rgszNames, UINT cNames,
                                                       LCID lcid, DISPID* rgDispId)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_Invoke(IXMLElementCollection *iface, DISPID dispIdMember,
                                                REFIID riid, LCID lcid, WORD wFlags,
                                                DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                                EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_put_length(IXMLElementCollection *iface, LONG v)
{
    TRACE("%p, %ld.\n", iface, v);

    return E_FAIL;
}

static HRESULT WINAPI xmlelem_collection_get_length(IXMLElementCollection *iface, LONG *p)
{
    xmlelem_collection *collection = impl_from_IXMLElementCollection(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = list_count(&collection->node->children);
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_get__newEnum(IXMLElementCollection *iface, IUnknown **ppUnk)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("(%p)->(%p)\n", This, ppUnk);

    if (!ppUnk)
        return E_INVALIDARG;

    IXMLElementCollection_AddRef(iface);
    *ppUnk = (IUnknown *)&This->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_item(IXMLElementCollection *iface, VARIANT var1,
        VARIANT var2, IDispatch **disp)
{
    xmlelem_collection *collection = impl_from_IXMLElementCollection(iface);
    struct node *node;
    HRESULT hr;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_variant(&var1), debugstr_variant(&var2), disp);

    if (!disp)
        return E_INVALIDARG;

    *disp = NULL;

    if (V_VT(&var1) != VT_I4 || V_I4(&var1) < 0)
        return E_INVALIDARG;

    if (!(node = _node_get_child(collection->node, V_I4(&var1))))
        return E_FAIL;

    hr = xmlelement_create(node, (IXMLElement2 **)disp);
    _node_release(node);

    return hr;
}

static const struct IXMLElementCollectionVtbl xmlelem_collection_vtbl =
{
    xmlelem_collection_QueryInterface,
    xmlelem_collection_AddRef,
    xmlelem_collection_Release,
    xmlelem_collection_GetTypeInfoCount,
    xmlelem_collection_GetTypeInfo,
    xmlelem_collection_GetIDsOfNames,
    xmlelem_collection_Invoke,
    xmlelem_collection_put_length,
    xmlelem_collection_get_length,
    xmlelem_collection_get__newEnum,
    xmlelem_collection_item
};

/************************************************************************
 * xmlelem_collection implementation of IEnumVARIANT.
 */
static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_QueryInterface(
    IEnumVARIANT *iface, REFIID riid, LPVOID *ppvObj)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s %p)\n", this, debugstr_guid(riid), ppvObj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        *ppvObj = iface;
        IEnumVARIANT_AddRef(iface);
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI xmlelem_collection_IEnumVARIANT_AddRef(
    IEnumVARIANT *iface)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);
    return IXMLElementCollection_AddRef(&this->IXMLElementCollection_iface);
}

static ULONG WINAPI xmlelem_collection_IEnumVARIANT_Release(
    IEnumVARIANT *iface)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);
    return IXMLElementCollection_Release(&this->IXMLElementCollection_iface);
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Next(
    IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    xmlelem_collection *collection = impl_from_IEnumVARIANT(iface);
    struct node *node;
    HRESULT hr;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, var, fetched);

    if (!var)
        return E_INVALIDARG;

    if (fetched) *fetched = 0;

    while (celt)
    {
        if (!(node = _node_get_child(collection->node, collection->current + 1)))
            break;

        V_VT(var) = VT_DISPATCH;
        hr = xmlelement_create(node, (IXMLElement2 **)&V_DISPATCH(var));
        _node_release(node);
        if (FAILED(hr))
            return hr;

        ++collection->current;
        if (fetched) ++*fetched;
        var++;
        celt--;
    }
    if (!celt) return S_OK;
    V_VT(var) = VT_EMPTY;
    return S_FALSE;
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Skip(
    IEnumVARIANT *iface, ULONG celt)
{
    FIXME("%p, %lu: stub\n", iface, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Reset(
    IEnumVARIANT *iface)
{
    xmlelem_collection *collection = impl_from_IEnumVARIANT(iface);

    TRACE("%p.\n", iface);

    collection->current = -1;
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Clone(
    IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    xmlelem_collection *This = impl_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static const struct IEnumVARIANTVtbl xmlelem_collection_IEnumVARIANTvtbl =
{
    xmlelem_collection_IEnumVARIANT_QueryInterface,
    xmlelem_collection_IEnumVARIANT_AddRef,
    xmlelem_collection_IEnumVARIANT_Release,
    xmlelem_collection_IEnumVARIANT_Next,
    xmlelem_collection_IEnumVARIANT_Skip,
    xmlelem_collection_IEnumVARIANT_Reset,
    xmlelem_collection_IEnumVARIANT_Clone
};

static HRESULT xmlelementcollection_create(struct node *node, IXMLElementCollection **ret)
{
    xmlelem_collection *collection;

    TRACE("%p\n", ret);

    *ret = NULL;

    if (list_empty(&node->children))
        return S_FALSE;

    collection = malloc(sizeof(*collection));
    if(!collection)
        return E_OUTOFMEMORY;

    collection->IXMLElementCollection_iface.lpVtbl = &xmlelem_collection_vtbl;
    collection->IEnumVARIANT_iface.lpVtbl = &xmlelem_collection_IEnumVARIANTvtbl;
    collection->ref = 1;
    collection->node = _node_grab(node);
    collection->current = -1;

    *ret = &collection->IXMLElementCollection_iface;

    TRACE("returning iface %p\n", *ret);
    return S_OK;
}

/* FIXME: IXMLDocument needs to implement
 *   - IXMLError
 *   - IPersistMoniker
 */

typedef struct _xmldoc
{
    IXMLDocument2 IXMLDocument2_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    LONG ref;

    struct node *root;
    struct
    {
        BSTR name;
    } dtd;
} xmldoc;

static inline xmldoc *impl_from_IXMLDocument2(IXMLDocument2 *iface)
{
    return CONTAINING_RECORD(iface, xmldoc, IXMLDocument2_iface);
}

static inline xmldoc *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, xmldoc, IPersistStreamInit_iface);
}

static HRESULT WINAPI xmldoc_QueryInterface(IXMLDocument2 *iface, REFIID riid, void** ppvObject)
{
    xmldoc *This = impl_from_IXMLDocument2(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IXMLDocument) ||
        IsEqualGUID(riid, &IID_IXMLDocument2))
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(&IID_IPersistStreamInit, riid) ||
             IsEqualGUID(&IID_IPersistStream, riid))
    {
        *ppvObject = &This->IPersistStreamInit_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDocument2_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmldoc_AddRef(IXMLDocument2 *iface)
{
    xmldoc *This = impl_from_IXMLDocument2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, refcount %ld.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xmldoc_Release(IXMLDocument2 *iface)
{
    xmldoc *doc = impl_from_IXMLDocument2(iface);
    LONG ref = InterlockedDecrement(&doc->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (ref == 0)
    {
        _node_release(doc->root);
        SysFreeString(doc->dtd.name);
        free(doc);
    }

    return ref;
}

static HRESULT WINAPI xmldoc_GetTypeInfoCount(IXMLDocument2 *iface, UINT* pctinfo)
{
    TRACE("%p, %p.\n", iface, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI xmldoc_GetTypeInfo(IXMLDocument2 *iface, UINT iTInfo,
                                         LCID lcid, ITypeInfo** ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IXMLDocument2_tid, ppTInfo);
}

static HRESULT WINAPI xmldoc_GetIDsOfNames(IXMLDocument2 *iface, REFIID riid,
                                           LPOLESTR* rgszNames, UINT cNames,
                                           LCID lcid, DISPID* rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmldoc_Invoke(IXMLDocument2 *iface, DISPID dispIdMember,
                                    REFIID riid, LCID lcid, WORD wFlags,
                                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                    EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmldoc_get_root(IXMLDocument2 *iface, IXMLElement2 **p)
{
    xmldoc *doc = impl_from_IXMLDocument2(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = NULL;

    if (!doc->root)
        return E_FAIL;

    return xmlelement_create(doc->root, p);
}

static HRESULT WINAPI xmldoc_get_fileSize(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_put_fileModifiedDate(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_fileUpdatedDate(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_URL(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT xmldoc_parse(xmldoc *doc, IStream *stream, const WCHAR *url)
{
    struct parse_context c;
    HRESULT hr;
    VARIANT v;

    if (FAILED(hr = parse_context_init(&c))) return hr;

    if (url)
    {
        hr = ISAXXMLReader_parseURL(c.reader, url);
    }
    else
    {
        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = (IUnknown *)stream;
        hr = ISAXXMLReader_parse(c.reader, v);
    }
    if (FAILED(hr))
    {
        WARN("Failed to parse %#lx.\n", hr);
        parse_context_cleanup(&c);
        return hr;
    }

    _node_release(doc->root);
    SysFreeString(doc->dtd.name);
    memset(&doc->dtd, 0, sizeof(doc->dtd));
    doc->root = _node_grab(c.root);
    doc->dtd.name = c.dtd.name;
    c.dtd.name = NULL;
    parse_context_cleanup(&c);

    return S_OK;
}

static HRESULT WINAPI xmldoc_put_URL(IXMLDocument2 *iface, BSTR p)
{
    xmldoc *doc = impl_from_IXMLDocument2(iface);

    TRACE("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return xmldoc_parse(doc, NULL, p);
}

static HRESULT WINAPI xmldoc_get_mimeType(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_readyState(IXMLDocument2 *iface, LONG *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_charset(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_put_charset(IXMLDocument2 *iface, BSTR p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_version(IXMLDocument2 *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"1.0", p);
}

static HRESULT WINAPI xmldoc_get_doctype(IXMLDocument2 *iface, BSTR *p)
{
    xmldoc *doc = impl_from_IXMLDocument2(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p) return E_INVALIDARG;

    if (!doc->dtd.name)
        return S_FALSE;

    return return_bstr(doc->dtd.name, p);
}

static HRESULT WINAPI xmldoc_get_dtdURL(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_createElement(IXMLDocument2 *iface, VARIANT type, VARIANT var1, IXMLElement2 **ret)
{
    struct node *node;
    HRESULT hr;

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_variant(&type), debugstr_variant(&var1), ret);

    if (!ret)
        return E_INVALIDARG;

    *ret = NULL;

    if (V_VT(&type) != VT_I4)
        return E_INVALIDARG;

    if (V_I4(&type) < XMLELEMTYPE_ELEMENT || V_I4(&type) > XMLELEMTYPE_OTHER)
        return E_NOTIMPL;

    if (FAILED(hr = node_create(L"", 0, V_I4(&type), &node)))
        return hr;

    hr = xmlelement_create(node, ret);
    _node_release(node);

    return hr;
}

static HRESULT WINAPI xmldoc_get_async(IXMLDocument2 *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub\n", iface, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_put_async(IXMLDocument2 *iface, VARIANT_BOOL v)
{
    FIXME("%p, %#x stub\n", iface, v);

    return E_NOTIMPL;
}

static const struct IXMLDocument2Vtbl xmldoc_vtbl =
{
    xmldoc_QueryInterface,
    xmldoc_AddRef,
    xmldoc_Release,
    xmldoc_GetTypeInfoCount,
    xmldoc_GetTypeInfo,
    xmldoc_GetIDsOfNames,
    xmldoc_Invoke,
    xmldoc_get_root,
    xmldoc_get_fileSize,
    xmldoc_put_fileModifiedDate,
    xmldoc_get_fileUpdatedDate,
    xmldoc_get_URL,
    xmldoc_put_URL,
    xmldoc_get_mimeType,
    xmldoc_get_readyState,
    xmldoc_get_charset,
    xmldoc_put_charset,
    xmldoc_get_version,
    xmldoc_get_doctype,
    xmldoc_get_dtdURL,
    xmldoc_createElement,
    xmldoc_get_async,
    xmldoc_put_async,
};

/************************************************************************
 * xmldoc implementation of IPersistStreamInit.
 */
static HRESULT WINAPI xmldoc_IPersistStreamInit_QueryInterface(
    IPersistStreamInit *iface, REFIID riid, LPVOID *ppvObj)
{
    xmldoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDocument2_QueryInterface(&doc->IXMLDocument2_iface, riid, ppvObj);
}

static ULONG WINAPI xmldoc_IPersistStreamInit_AddRef(
    IPersistStreamInit *iface)
{
    xmldoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDocument2_AddRef(&doc->IXMLDocument2_iface);
}

static ULONG WINAPI xmldoc_IPersistStreamInit_Release(
    IPersistStreamInit *iface)
{
    xmldoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDocument2_Release(&doc->IXMLDocument2_iface);
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_GetClassID(
    IPersistStreamInit *iface, CLSID *classid)
{
    TRACE("%p, %p.\n", iface, classid);

    if (!classid) return E_POINTER;

    *classid = CLSID_XMLDocument;
    return S_OK;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_IsDirty(
    IPersistStreamInit *iface)
{
    FIXME("(%p): stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_Load(IPersistStreamInit *iface,
        IStream *stream)
{
    xmldoc *doc = impl_from_IPersistStreamInit(iface);

    TRACE("%p, %p.\n", iface, stream);

    if (!stream)
        return E_INVALIDARG;

    return xmldoc_parse(doc, stream, NULL);
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_Save(
    IPersistStreamInit *iface, LPSTREAM pStm, BOOL fClearDirty)
{
    FIXME("(%p, %p, %d): stub!\n", iface, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_GetSizeMax(
    IPersistStreamInit *iface, ULARGE_INTEGER *pcbSize)
{
    xmldoc *This = impl_from_IPersistStreamInit(iface);
    TRACE("(%p, %p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_InitNew(
    IPersistStreamInit *iface)
{
    xmldoc *This = impl_from_IPersistStreamInit(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static const IPersistStreamInitVtbl xmldoc_IPersistStreamInit_VTable =
{
  xmldoc_IPersistStreamInit_QueryInterface,
  xmldoc_IPersistStreamInit_AddRef,
  xmldoc_IPersistStreamInit_Release,
  xmldoc_IPersistStreamInit_GetClassID,
  xmldoc_IPersistStreamInit_IsDirty,
  xmldoc_IPersistStreamInit_Load,
  xmldoc_IPersistStreamInit_Save,
  xmldoc_IPersistStreamInit_GetSizeMax,
  xmldoc_IPersistStreamInit_InitNew
};

HRESULT XMLDocument_create(LPVOID *ppObj)
{
    xmldoc *doc;

    TRACE("(%p)\n", ppObj);

    if (!(doc = calloc(1, sizeof(*doc))))
        return E_OUTOFMEMORY;

    doc->IXMLDocument2_iface.lpVtbl = &xmldoc_vtbl;
    doc->IPersistStreamInit_iface.lpVtbl = &xmldoc_IPersistStreamInit_VTable;
    doc->ref = 1;

    *ppObj = &doc->IXMLDocument2_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
