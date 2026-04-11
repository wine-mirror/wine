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

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/HTMLtree.h>
#include <libxml/tree.h>
#include <libxslt/pattern.h>
#include <libxslt/transform.h>
#include <libxslt/imports.h>
#include <libxslt/variables.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/documents.h>
#include <libxslt/extensions.h>
#include <libxslt/xsltInternals.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"
#include <activscp.h>
#include "objsafe.h"

#include "msxml_private.h"
#include "saxreader_extensions.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static const IID IID_domnode = {0x4f2f4ba2,0xb822,0x11df,{0x8b,0x8a,0x68,0x50,0xdf,0xd7,0x20,0x85}};

struct string_buffer
{
    WCHAR *data;
    size_t count;
    size_t capacity;

    HRESULT _status;
    HRESULT *status;
};

static void string_buffer_init(struct string_buffer *buffer)
{
    memset(buffer, 0, sizeof(*buffer));
    buffer->status = &buffer->_status;
}

static void string_append(struct string_buffer *buffer, const WCHAR *str, UINT len)
{
    if (*buffer->status != S_OK)
        return;

    if (!array_reserve((void **)&buffer->data, &buffer->capacity, buffer->count + len, sizeof(*str)))
    {
        *buffer->status = E_OUTOFMEMORY;
        free(buffer->data);
        return;
    }

    memcpy(buffer->data + buffer->count, str, len * sizeof(WCHAR));
    buffer->count += len;
}

static void string_buffer_cleanup(struct string_buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->count = buffer->capacity = 0;
}

static HRESULT string_to_bstr_slice(struct string_buffer *buffer, size_t offset, size_t length, BSTR *str)
{
    if (!str)
    {
        string_buffer_cleanup(buffer);
        return E_INVALIDARG;
    }

    *str = NULL;

    if (*buffer->status != S_OK)
    {
        string_buffer_cleanup(buffer);
        return *buffer->status;
    }

    if ((length && offset >= buffer->count) || length + offset > buffer->count)
    {
        *buffer->status = E_INVALIDARG;
        string_buffer_cleanup(buffer);
        return *buffer->status;
    }

    *str = SysAllocStringLen(buffer->data + offset, length);
    string_buffer_cleanup(buffer);

    return *str ? S_OK : E_OUTOFMEMORY;
}

static HRESULT string_to_bstr(struct string_buffer *buffer, BSTR *str)
{
    return string_to_bstr_slice(buffer, 0, buffer->count, str);
}

bool node_query_interface(struct domnode *node, REFIID riid, void **obj)
{
    if (IsEqualGUID(&IID_domnode, riid))
    {
        *obj = node;
        return true;
    }

    return false;
}

/* common ISupportErrorInfo implementation */
typedef struct {
   ISupportErrorInfo ISupportErrorInfo_iface;
   LONG ref;

   const tid_t* iids;
} SupportErrorInfo;

static inline SupportErrorInfo *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return CONTAINING_RECORD(iface, SupportErrorInfo, ISupportErrorInfo_iface);
}

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISupportErrorInfo)) {
        *obj = iface;
        ISupportErrorInfo_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, refcount %ld.\n", iface, ref );
    return ref;
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    enum tid_t const *tid;

    TRACE("%p, %s.\n", iface, debugstr_guid(riid));

    tid = This->iids;
    while (*tid != NULL_tid)
    {
        if (IsEqualGUID(riid, get_riid_from_tid(*tid)))
            return S_OK;
        tid++;
    }

    return S_FALSE;
}

static const struct ISupportErrorInfoVtbl SupportErrorInfoVtbl = {
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo
};

HRESULT node_create_supporterrorinfo(enum tid_t const *iids, void **obj)
{
    SupportErrorInfo *This;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    This->ref = 1;
    This->iids = iids;

    *obj = &This->ISupportErrorInfo_iface;

    return S_OK;
}

struct domnode *get_node_obj(void *node)
{
    struct domnode *obj = NULL;
    HRESULT hr;

    hr = IUnknown_QueryInterface((IUnknown *)node, &IID_domnode, (void **)&obj);
    if (!obj) WARN("Node is not our IXMLDOMNode implementation.\n");
    return SUCCEEDED(hr) ? obj : NULL;
}

static struct domnode *node_get_doc(struct domnode *node)
{
    return node->owner ? node->owner : node;
}

HRESULT node_get_name(struct domnode *node, BSTR *name)
{
    return return_bstr(node->qname, name);
}

HRESULT node_append_data(struct domnode *node, BSTR data)
{
    UINT len, current;
    BSTR str;

    if (!(len = SysStringLen(data)))
        return S_OK;

    current = SysStringLen(node->data);
    if (!(str = SysAllocStringLen(NULL, current + len)))
        return E_OUTOFMEMORY;

    memcpy(str, node->data, current * sizeof(WCHAR));
    memcpy(&str[current], data, len * sizeof(WCHAR));
    str[current + len] = 0;

    SysFreeString(node->data);
    node->data = str;

    return S_OK;
}

static HRESULT variant_get_str_value(const VARIANT *src, VARIANT *tmp, BSTR *value)
{
    HRESULT hr;

    VariantInit(tmp);
    *value = NULL;

    if (V_VT(src) == VT_BSTR)
    {
        *value = V_BSTR(src);
        return S_OK;
    }

    hr = VariantChangeType(tmp, src, 0, VT_BSTR);
    if (hr != S_OK)
    {
        WARN("Failed to change to BSTR\n");
        return hr;
    }

    *value = V_BSTR(tmp);
    return S_OK;
}

static HRESULT node_put_text_value(struct domnode *node, const VARIANT *value)
{
    HRESULT hr;
    VARIANT v;
    BSTR str;

    if (FAILED(hr = variant_get_str_value(value, &v, &str)))
        return hr;

    hr = node_put_data(node, str);
    VariantClear(&v);

    return hr;
}

HRESULT node_put_value(struct domnode *node, VARIANT *value)
{
    HRESULT hr;

    switch (node->type)
    {
        case NODE_TEXT:
        case NODE_COMMENT:
        case NODE_PROCESSING_INSTRUCTION:
        case NODE_CDATA_SECTION:
        case NODE_ATTRIBUTE:
            hr = node_put_text_value(node, value);
            break;
        default:
            FIXME("Unimplemented type %d.\n", node->type);
            hr = E_NOTIMPL;
    }

    return hr;
}

static HRESULT get_node(struct domnode *node, IXMLDOMNode **out)
{
    if (!out)
        return E_INVALIDARG;

    return create_node(node, out);
}

HRESULT node_get_parent(struct domnode *node, IXMLDOMNode **ret)
{
    return get_node(node->parent, ret);
}

HRESULT node_get_child_nodes(struct domnode *node, IXMLDOMNodeList **ret)
{
    if(!ret)
        return E_INVALIDARG;

    return create_children_nodelist(node, ret);
}

static struct domnode *node_from_entry(struct list *entry)
{
    return entry ? LIST_ENTRY(entry, struct domnode, entry) : NULL;
}

struct domnode *domnode_get_first_child(struct domnode *node)
{
    return node_from_entry(list_head(&node->children));
}

static struct domnode *domnode_get_previous_sibling(struct domnode *node)
{
    if (node->parent)
        return node_from_entry(list_prev(&node->parent->children, &node->entry));

    return NULL;
}

HRESULT node_get_first_child(struct domnode *node, IXMLDOMNode **ret)
{
    return get_node(domnode_get_first_child(node), ret);
}

HRESULT node_get_last_child(struct domnode *node, IXMLDOMNode **ret)
{
    return get_node(node_from_entry(list_tail(&node->children)), ret);
}

HRESULT node_get_previous_sibling(struct domnode *node, IXMLDOMNode **ret)
{
    struct domnode *sibling = NULL;

    if (node->parent)
        sibling = node_from_entry(list_prev(&node->parent->children, &node->entry));
    return get_node(sibling, ret);
}

struct domnode *domnode_get_next_sibling(struct domnode *node)
{
    if (node->parent)
        return node_from_entry(list_next(&node->parent->children, &node->entry));

    return NULL;
}

HRESULT node_get_next_sibling(struct domnode *node, IXMLDOMNode **ret)
{
    return get_node(domnode_get_next_sibling(node), ret);
}

static bool domnode_is_valid_child_type(const struct domnode *node, struct domnode *child)
{
    struct domnode *n;

    if (!(node->type == NODE_DOCUMENT
            || node->type == NODE_DOCUMENT_FRAGMENT
            || node->type == NODE_ELEMENT
            || node->type == NODE_ATTRIBUTE))
    {
        return false;
    }

    if (child->type == NODE_DOCUMENT_FRAGMENT)
    {
        LIST_FOR_EACH_ENTRY(n, &child->children, struct domnode, entry)
        {
            if (!domnode_is_valid_child_type(node, n)) return false;
        }

        return true;
    }

    switch (node->type)
    {
        case NODE_DOCUMENT:
            if (child->type == NODE_ELEMENT)
            {
                /* Document is allowed a single element child. */

                LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
                {
                    if (n->type == NODE_ELEMENT)
                        return false;
                }

                return true;
            }

            return (child->type == NODE_COMMENT
                    || child->type == NODE_PROCESSING_INSTRUCTION);

        case NODE_DOCUMENT_FRAGMENT:
        case NODE_ELEMENT:
            return (child->type == NODE_TEXT
                    || child->type == NODE_COMMENT
                    || child->type == NODE_ELEMENT
                    || child->type == NODE_CDATA_SECTION
                    || child->type == NODE_ENTITY_REFERENCE
                    || child->type == NODE_PROCESSING_INSTRUCTION);

        case NODE_ATTRIBUTE:
            return (child->type ==  NODE_TEXT
                    || child->type == NODE_ENTITY_REFERENCE);

        default:
            return false;
    }
}

/* Link to a new owner, transferring outstanding references. */
static void domnode_set_owner(struct domnode *node, struct domnode *owner)
{
    struct domnode *n, *old_owner;

    /* Document node does not have the owner set. */
    owner = owner->owner ? owner->owner : owner;

    assert(!!owner);

    old_owner = node->owner;
    if (old_owner == owner)
        return;

    list_remove(&node->owner_entry);
    node->owner = owner;
    owner->refcount += node->refcount;
    list_add_tail(&owner->owned, &node->owner_entry);

    LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
    {
        domnode_set_owner(n, owner);
    }

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        domnode_set_owner(n, owner);
    }

    old_owner->refcount -= node->refcount;
    if (old_owner->refcount == 0)
        domnode_destroy_tree(old_owner);
}

static void domnode_unlink_child(struct domnode *child)
{
    struct domnode *node, *top;

    if (!child->parent)
         return;

    /* TODO: should fail to remove doctype child */

    /* Drop references for the parent and up */
    node = child->parent;
    while (node)
    {
        top = node;
        node->refcount -= child->refcount;
        node = node->parent;
    }

    list_remove(&child->entry);
    child->parent = NULL;

    if (top->refcount == 0)
        domnode_destroy_tree(top);
}

static void domnode_add_refs(struct domnode *node, unsigned int count)
{
    struct domnode *p = node;

    if (node->owner)
        node->owner->refcount += count;
    while (p)
    {
        p->refcount += count;
        p = p->parent;
    }
}

static void domnode_insert_domnode(struct domnode *node, struct domnode *child, struct domnode *ref_child)
{
    struct domnode *n, *next;

    if (child->type == NODE_DOCUMENT_FRAGMENT)
    {
        LIST_FOR_EACH_ENTRY_SAFE(n, next, &child->children, struct domnode, entry)
        {
            domnode_insert_domnode(node, n, ref_child);
        }

        return;
    }

    domnode_unlink_child(child);

    if (ref_child)
        list_add_before(&ref_child->entry, &child->entry);
    else
        list_add_tail(&node->children, &child->entry);

    child->parent = node;
    domnode_add_refs(child->parent, child->refcount);
    domnode_set_owner(child, node);
}

HRESULT node_insert_before(struct domnode *node, IXMLDOMNode *new_child, const VARIANT *ref_child,
        IXMLDOMNode **ret)
{
    struct domnode *child_node, *ref_node = NULL;

    if (!new_child)
        return E_INVALIDARG;

    if (ret)
        *ret = NULL;

    if (!(child_node = get_node_obj(new_child)))
        return E_FAIL;

    switch (V_VT(ref_child))
    {
        case VT_EMPTY:
        case VT_NULL:
            break;

        case VT_UNKNOWN:
        case VT_DISPATCH:
            if (!V_UNKNOWN(ref_child))
                break;

            if (!(ref_node = get_node_obj(V_UNKNOWN(ref_child))))
                return E_FAIL;
            if (ref_node->parent != node)
            {
                WARN("Reference node is not a child of this node.\n");
                return E_FAIL;
            }
            break;

        default:
            FIXME("Unexpected reference node type %s.\n", debugstr_variant(ref_child));
            return E_FAIL;
    }

    if (!domnode_is_valid_child_type(node, child_node))
    {
        WARN("Can't add node %p as a child for %p.\n", child_node, node);
        return E_FAIL;
    }

    domnode_insert_domnode(node, child_node, ref_node);

    if (ret)
    {
        *ret = new_child;
        IXMLDOMNode_AddRef(*ret);
    }

    return S_OK;
}

HRESULT node_replace_child(struct domnode *node, IXMLDOMNode *newChild, IXMLDOMNode *oldChild,
        IXMLDOMNode **ret)
{
    struct domnode *old_child, *new_child, *curr;
    VARIANT ref;
    HRESULT hr;

    if (!newChild || !oldChild)
        return E_INVALIDARG;

    if (ret)
        *ret = NULL;

    if (!(new_child = get_node_obj(newChild)))
        return E_FAIL;

    if (!(old_child = get_node_obj(oldChild)))
        return E_FAIL;

    if (old_child->parent != node)
    {
        WARN("Node %p is not a child of %p.\n", old_child, node);
        return E_INVALIDARG;
    }

    curr = node;
    while (curr)
    {
        if (curr == new_child)
        {
            WARN("Attempt to create a cycle.\n");
            return E_FAIL;
        }
        curr = curr->parent;
    }

    V_VT(&ref) = VT_DISPATCH;
    V_DISPATCH(&ref) = (IDispatch *)oldChild;
    if (FAILED(hr = node_insert_before(node, newChild, &ref, NULL)))
        return hr;

    return node_remove_child(node, oldChild, ret);
}

static void domnode_unlink_children(struct domnode *node)
{
    struct domnode *child, *next;

    LIST_FOR_EACH_ENTRY_SAFE(child, next, &node->children, struct domnode, entry)
    {
        domnode_unlink_child(child);
    }
}

static void domnode_append_child(struct domnode *parent, struct domnode *node)
{
    domnode_unlink_child(node);

    list_add_tail(&parent->children, &node->entry);
    node->parent = parent;
    domnode_add_refs(node->parent, node->refcount);
    domnode_set_owner(node, parent);
}

HRESULT node_put_data(struct domnode *node, const WCHAR *data)
{
    struct string_buffer buffer;
    const WCHAR *p = data;
    struct domnode *child;

    if (node->flags & DOMNODE_READONLY_VALUE)
        return E_FAIL;

    /* Markup is checked and rejected. */

    if (node->type == NODE_COMMENT && data && wcsstr(data, L"-->"))
        return E_INVALIDARG;

    if (node->type == NODE_PROCESSING_INSTRUCTION && data && wcsstr(data, L"?>"))
        return E_INVALIDARG;

    string_buffer_init(&buffer);

    while (p && *p)
    {
        if (*p == '\r')
        {
            if (p[1] == '\n') ++p;
            string_append(&buffer, L"\n", 1);
        }
        else
        {
            string_append(&buffer, p, 1);
        }
        ++p;
    }

    switch (node->type)
    {
        case NODE_ATTRIBUTE:
        case NODE_ELEMENT:
            domnode_unlink_children(node);

            /* TODO: error handling */
            domnode_create(NODE_TEXT, NULL, 0, NULL, 0, node->owner, &child);
            domnode_append_child(node, child);

            return string_to_bstr(&buffer, &child->data);

        default:
            SysFreeString(node->data);
            return string_to_bstr(&buffer, &node->data);
    }
}

struct node_dump_ns
{
    struct list entry;
    BSTR prefix;
    BSTR uri;
    bool own_uri;
    int depth;
};

struct node_dump_context
{
    struct string_buffer buffer;
    struct
    {
        char *data;
        size_t capacity;
    } scratch;

    struct list namespaces;
    int depth;

    unsigned int indent;
    bool needs_formatting;

    bool only_utf16_encoding_decl;

    IStream *stream;
    UINT codepage;
    HRESULT status;
};

static void node_dump_push_namespace(struct node_dump_context *context, BSTR prefix, BSTR uri, bool own_uri)
{
    struct node_dump_ns *ns;

    if (context->status != S_OK)
        return;

    if (!(ns = calloc(1, sizeof(*ns))))
    {
        context->status = E_OUTOFMEMORY;
        return;
    }

    ns->depth = context->depth;
    ns->prefix = prefix;
    ns->uri = uri;
    ns->own_uri = own_uri;

    list_add_tail(&context->namespaces, &ns->entry);
}

static void node_dump_pop_namespaces(struct node_dump_context *context)
{
    struct node_dump_ns *ns, *next;

    LIST_FOR_EACH_ENTRY_SAFE_REV(ns, next, &context->namespaces, struct node_dump_ns, entry)
    {
        if (ns->depth != context->depth)
            break;

        list_remove(&ns->entry);
        if (ns->own_uri)
            SysFreeString(ns->uri);
        free(ns);
    }
}

static void node_dump(struct domnode *node, struct node_dump_context *context);

static void node_dump_append(struct node_dump_context *context, const WCHAR *text, unsigned int length)
{
    ULONG written, required;

    if (context->status != S_OK)
        return;

    if (context->stream)
    {
        if (context->codepage == ~0u)
        {
            context->status = IStream_Write(context->stream, text, length * sizeof(WCHAR), &written);
        }
        else
        {
            /* NOTE: might be useful to buffer */

            required = WideCharToMultiByte(context->codepage, 0, text, length, NULL, 0, NULL, NULL);

            if (!array_reserve((void **)&context->scratch.data, &context->scratch.capacity,
                    required, sizeof(*context->scratch.data)))
            {
                context->status = E_OUTOFMEMORY;
                return;
            }

            WideCharToMultiByte(context->codepage, 0, text, length, context->scratch.data, required, NULL, NULL);
            context->status = IStream_Write(context->stream, context->scratch.data, required, &written);
        }
    }
    else
    {
        string_append(&context->buffer, text, length);
    }
}

static HRESULT node_get_pi_data(const struct domnode *node, BSTR *p)
{
    struct node_dump_context context = { 0 };
    bool separate = false;
    struct domnode *attr;

    if (!wcscmp(node->name, L"xml"))
    {
        string_buffer_init(&context.buffer);

        LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
        {
            if (separate)
                node_dump_append(&context, L" ", 1);
            node_dump(attr, &context);
            separate = true;
        }
        return string_to_bstr(&context.buffer, p);
    }
    else
    {
        return return_bstr(node->data ? node->data : L"", p);
    }
}

HRESULT node_get_data(struct domnode *node, BSTR *p)
{
    switch (node->type)
    {
        case NODE_TEXT:
        case NODE_COMMENT:
        case NODE_CDATA_SECTION:
            return return_bstr(node->data ? node->data : L"", p);
        case NODE_PROCESSING_INSTRUCTION:
            return node_get_pi_data(node, p);
        default:
            FIXME("Unhandled type %d.\n", node->type);
            return E_NOTIMPL;
    }
}

HRESULT node_remove_child(struct domnode *node, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    struct domnode *child_node;

    if (!child)
        return E_INVALIDARG;

    if (oldChild)
        *oldChild = NULL;

    child_node = get_node_obj(child);
    if (!child_node)
        return E_FAIL;

    if (child_node->parent != node)
    {
        WARN("Node %p is not a child of %p.\n", child, node);
        return E_INVALIDARG;
    }

    if (child_node->type == NODE_DOCUMENT_TYPE)
    {
        WARN("Can't remove doctype child.\n");
        return E_FAIL;
    }

    domnode_unlink_child(child_node);

    if (oldChild)
    {
        IXMLDOMNode_AddRef(child);
        *oldChild = child;
    }

    /* Child node is never freed here, we'll always have outstanding references at this point. */

    return S_OK;
}

static void domnode_unlink_attribute(struct domnode *attr)
{
    struct domnode *node, *top;

    if (!attr->parent)
        return;

    /* Drop references for the parent and up */
    node = attr->parent;
    while (node)
    {
        top = node;
        node->refcount -= attr->refcount;
        node = node->parent;
    }

    list_remove(&attr->entry);
    attr->parent = NULL;

    if (top->refcount == 0)
        domnode_destroy_tree(top);
}

HRESULT node_remove_attribute_node(struct domnode *node, IXMLDOMAttribute *attr, IXMLDOMAttribute **old_attr)
{
    struct domnode *attr_node;

    if (!attr)
        return E_INVALIDARG;

    if (old_attr)
        *old_attr = NULL;

    attr_node = get_node_obj(attr);
    if (!attr_node)
        return E_FAIL;

    if (attr_node->parent != node)
    {
        WARN("Node %p is not an attribute of %p.\n", attr, node);
        return E_INVALIDARG;
    }

    domnode_unlink_attribute(attr_node);

    if (old_attr)
    {
        IXMLDOMAttribute_AddRef(attr);
        *old_attr = attr;
    }

    /* Attribute node is never freed here, we'll always have outstanding references at this point. */

    return S_OK;
}

HRESULT domnode_get_attribute(const struct domnode *node, const WCHAR *name, struct domnode **attr)
{
    struct domnode *n;

    /* TODO: return E_FAIL for invalid XML names */

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        if (!wcscmp(n->qname, name))
        {
            *attr = n;
            return S_OK;
        }
    }

    *attr = NULL;
    return S_FALSE;
}

static bool is_same_uri(const struct domnode *node, const WCHAR *uri)
{
    if (node->uri)
        return uri && !wcscmp(node->uri, uri);

    return !uri;
}

static HRESULT domnode_get_qualified_attribute(const struct domnode *node, const WCHAR *name,
        const WCHAR *uri, struct domnode **attr)
{
    struct domnode *n;

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        if (!wcscmp(n->name, name) && is_same_uri(n, uri))
        {
            *attr = n;
            return S_OK;
        }
    }

    *attr = NULL;
    return S_FALSE;
}

HRESULT node_remove_attribute(struct domnode *node, const WCHAR *name, IXMLDOMNode **ret)
{
    struct domnode *attr;
    HRESULT hr;

    if (!name)
        return E_INVALIDARG;

    if (ret)
        *ret = NULL;

    if ((hr = domnode_get_attribute(node, name, &attr)) != S_OK)
        return hr;

    if (ret)
        hr = create_node(attr, ret);

    if (SUCCEEDED(hr))
        domnode_unlink_attribute(attr);

    return hr;
}

HRESULT domnode_create(DOMNodeType type, const WCHAR *name, int name_len, const WCHAR *uri, int uri_len,
        struct domnode *owner, struct domnode **node)
{
    struct domnode *object;
    WCHAR *p;
    BSTR str;

    *node = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    str = SysAllocStringLen(name, name_len);
    if (type == NODE_ELEMENT || type == NODE_ATTRIBUTE)
    {
        if (!parser_is_valid_qualified_name(str))
        {
            SysFreeString(str);
            return E_FAIL;
        }

        if ((p = wcschr(str, ':')))
        {
            object->prefix = SysAllocStringLen(str, p - str);
            object->name = SysAllocString(p + 1);
        }
        else
        {
            object->name = str;
        }
    }
    else
    {
        object->name = str;
    }
    object->qname = str;

    if (uri_len)
    {
        if (!(object->uri = SysAllocStringLen(uri, uri_len)))
        {
            free(object->name);
            free(object);
            return E_OUTOFMEMORY;
        }
    }

    object->type = type;
    switch (object->type)
    {
        case NODE_PROCESSING_INSTRUCTION:
            if (!wcscmp(object->name, L"xml"))
                object->flags |= DOMNODE_READONLY_VALUE;
            break;
        default:
            ;
    }

    list_init(&object->entry);
    list_init(&object->owner_entry);
    list_init(&object->children);
    list_init(&object->attributes);
    list_init(&object->owned);
    object->owner = owner;
    /* Document node does not have an owner */
    if (owner)
        list_add_tail(&owner->owned, &object->owner_entry);

    *node = object;

    return S_OK;
}

static void domnode_append_attribute(struct domnode *parent, struct domnode *node)
{
    domnode_unlink_attribute(node);

    list_add_tail(&parent->attributes, &node->entry);
    node->parent = parent;
    domnode_add_refs(node->parent, node->refcount);
    domnode_set_owner(node, parent);
}

static HRESULT parse_xml_decl_append_attribute(struct domnode *pi, const WCHAR *name, BSTR value)
{
    struct domnode *attribute;
    HRESULT hr;

    if (FAILED(hr = domnode_create(NODE_ATTRIBUTE, name, wcslen(name), NULL, 0, pi->owner, &attribute)))
        return hr;

    if (SUCCEEDED(hr = node_put_data(attribute, value)))
        domnode_append_attribute(pi, attribute);
    else
        domnode_destroy_tree(attribute);

    return hr;
}

static HRESULT parse_xml_decl(struct domnode *pi, const WCHAR *data)
{
    BSTR version, encoding, standalone;
    HRESULT hr;

    if (FAILED(hr = parse_xml_decl_body(data, &version, &encoding, &standalone)))
        return hr;

    hr = parse_xml_decl_append_attribute(pi, L"version", version);

    if (SUCCEEDED(hr) && encoding)
        hr = parse_xml_decl_append_attribute(pi, L"encoding", encoding);

    if (SUCCEEDED(hr) && standalone)
        hr = parse_xml_decl_append_attribute(pi, L"standalone", standalone);

    SysFreeString(version);
    SysFreeString(encoding);
    SysFreeString(standalone);

    return hr;
}

/* Parses declaration into attributes, public API does not allow setting data for such nodes. */
HRESULT create_pi_node(struct domnode *doc, const WCHAR *target, const WCHAR *data, IXMLDOMProcessingInstruction **ret)
{
    struct domnode *pi;
    HRESULT hr;

    if (!ret)
        return E_INVALIDARG;

    if (!target || !*target)
        return E_FAIL;

    *ret = NULL;

    if (FAILED(hr = domnode_create(NODE_PROCESSING_INSTRUCTION, target, wcslen(target), NULL, 0, doc, &pi)))
        return hr;

    if (!wcscmp(target, L"xml"))
        hr = parse_xml_decl(pi, data);
    else
        hr = node_put_data(pi, data);

    if (SUCCEEDED(hr))
        return create_node(pi, (IXMLDOMNode **)ret);

    domnode_destroy_tree(pi);

    return hr;
}

HRESULT node_append_child(struct domnode *node, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    VARIANT var;

    VariantInit(&var);
    return node_insert_before(node, child, &var, outChild);
}

HRESULT node_has_childnodes(const struct domnode *node, VARIANT_BOOL *ret)
{
    if (!ret)
        return E_INVALIDARG;

    if (list_empty(&node->children))
    {
        *ret = VARIANT_FALSE;
        return S_FALSE;
    }

    *ret = VARIANT_TRUE;
    return S_OK;
}

HRESULT node_get_owner_document(const struct domnode *node, IXMLDOMDocument **doc)
{
    IXMLDOMNode *node_obj;
    HRESULT hr;

    if (!doc)
        return E_INVALIDARG;

    if (FAILED(hr = get_node(node->owner, &node_obj)))
        return hr;

    hr = IXMLDOMNode_QueryInterface(node_obj, &IID_IXMLDOMDocument, (void **)doc);
    IXMLDOMNode_Release(node_obj);

    return hr;
}

struct domnode *domnode_addref(struct domnode *node)
{
    domnode_add_refs(node, 1);
    return node;
}

static struct domnode *domnode_drop_refs(struct domnode *node, unsigned int count)
{
    struct domnode *top = NULL;

    if (node->owner)
        node->owner->refcount -= count;
    while (node)
    {
        top = node;
        node->refcount -= count;
        node = node->parent;
    }

    return top;
}

struct domdoc_properties *domdoc_create_properties(MSXML_VERSION version)
{
    struct domdoc_properties *properties = malloc(sizeof(*properties));

    list_init(&properties->selectNsList);
    properties->preserving = VARIANT_FALSE;
    properties->validating = VARIANT_TRUE;
    properties->schemaCache = NULL;
    properties->selectNsStr = calloc(1, sizeof(xmlChar));
    properties->selectNsStr_len = 0;

    /* properties that are dependent on object versions */
    properties->version = version;
    properties->XPath = (version == MSXML4 || version == MSXML6);
    properties->prohibit_dtd = version == MSXML6;

    /* document uri */
    properties->uri = NULL;

    return properties;
}

static void domdoc_properties_destroy(struct domdoc_properties *properties)
{
    if (properties->schemaCache)
        IXMLDOMSchemaCollection2_Release(properties->schemaCache);
    domdoc_properties_clear_selection_namespaces(&properties->selectNsList);
    free((xmlChar*)properties->selectNsStr);
    if (properties->uri)
        IUri_Release(properties->uri);
    free(properties);
}

static struct domdoc_properties* domdoc_properties_clone(struct domdoc_properties const* properties)
{
    struct domdoc_properties* pcopy = malloc(sizeof(*pcopy));
    select_ns_entry const* ns = NULL;
    select_ns_entry* new_ns = NULL;
    int len = (properties->selectNsStr_len+1)*sizeof(xmlChar);
    ptrdiff_t offset;

    if (pcopy)
    {
        pcopy->version = properties->version;
        pcopy->preserving = properties->preserving;
        pcopy->validating = properties->validating;
        pcopy->prohibit_dtd = properties->prohibit_dtd;
        pcopy->schemaCache = properties->schemaCache;
        if (pcopy->schemaCache)
            IXMLDOMSchemaCollection2_AddRef(pcopy->schemaCache);
        pcopy->XPath = properties->XPath;
        pcopy->selectNsStr_len = properties->selectNsStr_len;
        list_init( &pcopy->selectNsList );
        pcopy->selectNsStr = malloc(len);
        memcpy((xmlChar*)pcopy->selectNsStr, properties->selectNsStr, len);
        offset = pcopy->selectNsStr - properties->selectNsStr;

        LIST_FOR_EACH_ENTRY( ns, (&properties->selectNsList), select_ns_entry, entry )
        {
            new_ns = malloc(sizeof(select_ns_entry));
            memcpy(new_ns, ns, sizeof(select_ns_entry));
            new_ns->href += offset;
            new_ns->prefix += offset;
            list_add_tail(&pcopy->selectNsList, &new_ns->entry);
        }

        pcopy->uri = properties->uri;
        if (pcopy->uri)
            IUri_AddRef(pcopy->uri);
    }

    return pcopy;
}

void domnode_destroy_tree(struct domnode *tree)
{
    struct domnode *node, *next;

    if (tree->type == NODE_DOCUMENT)
    {
        /* For the document nodes we can skip hierarchical calls,
           since all the nodes are in the owner list. */

        LIST_FOR_EACH_ENTRY_SAFE(node, next, &tree->owned, struct domnode, owner_entry)
        {
            list_remove(&node->owner_entry);
            node->owner = NULL;
            list_init(&node->children);
            list_init(&node->attributes);
            domnode_destroy_tree(node);
        }
        domdoc_properties_destroy(tree->properties);
        tree->properties = NULL;
    }
    else
    {
        list_remove(&tree->owner_entry);

        LIST_FOR_EACH_ENTRY_SAFE(node, next, &tree->children, struct domnode, entry)
        {
            list_remove(&node->entry);
            node->parent = NULL;
            domnode_destroy_tree(node);
        }

        LIST_FOR_EACH_ENTRY_SAFE(node, next, &tree->attributes, struct domnode, entry)
        {
            list_remove(&node->entry);
            domnode_destroy_tree(node);
        }
    }

    if (tree->prefix)
    {
        SysFreeString(tree->prefix);
        SysFreeString(tree->qname);
    }
    SysFreeString(tree->name);
    SysFreeString(tree->data);
    SysFreeString(tree->uri);

    free(tree);
}

void domnode_release(struct domnode *node)
{
    struct domnode *top;

    top = domnode_drop_refs(node, 1);
    if (top->refcount == 0)
        domnode_destroy_tree(top);
}

HRESULT node_clone_domnode(struct domnode *node, bool deep, struct domnode **cloned)
{
    struct domnode *object, *n, *child;
    HRESULT hr;

    *cloned = NULL;

    if (FAILED(hr = domnode_create(node->type, node->name, SysStringLen(node->name), node->uri, SysStringLen(node->uri),
            node->owner, &object)))
    {
        return hr;
    }

    if (node->type == NODE_DOCUMENT)
        object->properties = domdoc_properties_clone(node->properties);

    if (deep)
    {
        LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
        {
            node_clone_domnode(n, true, &child);
            domnode_append_child(object, child);
        }
    }

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        node_clone_domnode(n, true, &child);
        domnode_append_attribute(object, child);
    }

    *cloned = object;

    return hr;
}

HRESULT node_clone(struct domnode *node, VARIANT_BOOL deep, IXMLDOMNode **out)
{
    struct domnode *cloned_node;
    HRESULT hr;

    if (!out)
        return E_INVALIDARG;

    *out = NULL;

    if (FAILED(hr = node_clone_domnode(node, !!deep, &cloned_node)))
        return hr;

    if (FAILED(hr = create_node(cloned_node, out)))
        domnode_destroy_tree(cloned_node);

    return hr;
}

static bool is_preserving_whitespace(const struct domnode *node)
{
    const struct domnode *doc = node->owner ? node->owner : node;
    struct domnode *attr;
    BSTR value;
    bool ret;

    /* Document-wide property */
    if (doc->properties->preserving == VARIANT_TRUE)
        return true;

    if (node->type != NODE_ELEMENT)
        return false;

    while (node)
    {
        if (domnode_get_qualified_attribute(node, L"space",
                L"http://www.w3.org/XML/1998/namespace", &attr) == S_OK)
        {
            if (node_get_text(attr, &value) == S_OK)
            {
                ret = !wcscmp(value, L"preserve");
                SysFreeString(value);
                return ret;
            }
        }

        node = node->parent;
    }

    return false;
}

struct node_get_text_context
{
    struct string_buffer buffer;
    bool first_textual_child;
    bool preserve;
    bool ignored_ws;
    int depth;
};

/* Trim leading and trailing white spaces */
static WCHAR *domnode_get_text_trim(WCHAR *text, UINT *length)
{
    WCHAR *start = text;
    UINT len = *length;

    while (start && xml_is_space(*start))
    {
        ++start;
        --len;
    }

    while (len && xml_is_space(start[len - 1]))
        --len;

    *length = len;
    return start;
}

static const WCHAR *domnode_get_text_trim_leading(const WCHAR *text, UINT *length)
{
    const WCHAR *start = text;
    UINT len = *length;

    while (start && xml_is_space(*start))
    {
        ++start;
        --len;
    }

    *length = len;
    return start;
}

static void domnode_get_text_trim_trailing(struct string_buffer *buffer)
{
    while (buffer->count && xml_is_space(buffer->data[buffer->count - 1]))
        --buffer->count;
}

static void domnode_append_ignored(const struct domnode *node, unsigned int flags, struct node_get_text_context *context)
{
    /* Skip all leading ignored space */
    if (!context->buffer.count)
        return;

    if (!context->ignored_ws && (node->flags & flags))
    {
        context->ignored_ws = true;
        if (!context->first_textual_child || context->preserve)
        {
            string_append(&context->buffer, L" ", 1);
            context->first_textual_child = false;
        }
    }
}

static void domnode_append_ignored_pre(struct domnode *node, struct node_get_text_context *context)
{
    struct domnode *sibling = domnode_get_previous_sibling(node);

    if (sibling)
    {
        if (sibling->type == NODE_ELEMENT)
            return domnode_append_ignored(sibling, DOMNODE_IGNORED_WS, context);
    }
    else if (node->parent && node->parent->type == NODE_ELEMENT)
    {
        domnode_append_ignored(node, DOMNODE_IGNORED_WS_AFTER_STARTTAG, context);
    }
}

static bool is_space_data(const WCHAR *data)
{
    while (*data)
    {
        if (!xml_is_space(*data)) return false;
        ++data;
    }

    return true;
}

static void domnode_get_text(const struct domnode *node, struct node_get_text_context *context)
{
    struct string_buffer *buffer = &context->buffer;
    struct domnode *child;
    const WCHAR *start;
    UINT length;

    switch (node->type)
    {
        case NODE_TEXT:
            if (context->first_textual_child && !context->preserve)
            {
                length = SysStringLen(node->data);
                start = domnode_get_text_trim_leading(node->data, &length);
                string_append(&context->buffer, start, length);
            }
            else if (!context->preserve && is_space_data(node->data))
            {
                string_append(&context->buffer, L" ", 1);
            }
            else
            {
                string_append(&context->buffer, node->data, SysStringLen(node->data));
            }
            context->first_textual_child = false;
            context->ignored_ws = false;
            break;

        case NODE_CDATA_SECTION:
            string_append(buffer, node->data, SysStringLen(node->data));
            context->first_textual_child = false;
            context->ignored_ws = false;
            break;

        case NODE_ELEMENT:
            LIST_FOR_EACH_ENTRY(child, &node->children, struct domnode, entry)
            {
                domnode_append_ignored_pre(child, context);
                if (child->type == NODE_ELEMENT) ++context->depth;
                domnode_get_text(child, context);
                if (child->type == NODE_ELEMENT) --context->depth;
            }
            break;

        default:
            ;
    }
}

HRESULT node_get_text(const struct domnode *node, BSTR *text)
{
    struct node_get_text_context context = { 0 };
    struct domnode *child;
    const WCHAR *start;
    UINT length;

    string_buffer_init(&context.buffer);
    context.preserve = is_preserving_whitespace(node);
    switch (node->type)
    {
        case NODE_COMMENT:
        case NODE_ENTITY_REFERENCE:
        case NODE_CDATA_SECTION:
            return return_bstr(node->data, text);

        case NODE_PROCESSING_INSTRUCTION:
            return node_get_pi_data(node, text);

        case NODE_TEXT:
            if (context.preserve)
            {
                string_append(&context.buffer, node->data, SysStringLen(node->data));
            }
            else
            {
                length = SysStringLen(node->data);
                start = domnode_get_text_trim(node->data, &length);
                string_append(&context.buffer, start, length);
            }
            return string_to_bstr(&context.buffer, text);

        case NODE_ATTRIBUTE:
            LIST_FOR_EACH_ENTRY(child, &node->children, struct domnode, entry)
            {
                string_append(&context.buffer, child->data, SysStringLen(child->data));
            }
            return string_to_bstr(&context.buffer, text);

        case NODE_DOCUMENT:
        case NODE_DOCUMENT_FRAGMENT:

            context.first_textual_child = true;
            LIST_FOR_EACH_ENTRY(child, &node->children, struct domnode, entry)
            {
                domnode_get_text(child, &context);
            }
            if (!context.preserve)
                domnode_get_text_trim_trailing(&context.buffer);
            return string_to_bstr(&context.buffer, text);

        case NODE_ELEMENT:

            /* Trim leading spaces of the first textual node. Append everything else as is,
               then trim trailing spaces. */

            context.first_textual_child = true;
            domnode_get_text(node, &context);
            if (!context.preserve)
                domnode_get_text_trim_trailing(&context.buffer);
            return string_to_bstr(&context.buffer, text);

        default:
            FIXME("not implemented for node type %d\n", node->type);
            return E_NOTIMPL;
    }
}

HRESULT node_get_preserved_text(const struct domnode *node, BSTR *text)
{
    struct node_get_text_context context = { 0 };

    string_buffer_init(&context.buffer);
    context.preserve = true;

    switch (node->type)
    {
        case NODE_ELEMENT:
            domnode_get_text(node, &context);
            return string_to_bstr(&context.buffer, text);

        default:
            FIXME("not implemented for node type %d\n", node->type);
            return E_NOTIMPL;
    }
}

HRESULT node_get_value(struct domnode *node, VARIANT *value)
{
    if (!value)
        return E_INVALIDARG;

    switch (node->type)
    {
        case NODE_TEXT:
        case NODE_CDATA_SECTION:
        case NODE_ATTRIBUTE:
        case NODE_PROCESSING_INSTRUCTION:
        case NODE_COMMENT:
            V_VT(value) = VT_BSTR;
            return node_get_text(node, &V_BSTR(value));
        default:
            FIXME("Unsupported type %d.\n", node->type);
            return E_NOTIMPL;
    }
}

static void node_dump_xml_text(struct domnode *node, struct node_dump_context *context)
{
    BSTR p = node->data;

    while (p && *p)
    {
        if (*p == '<')
            node_dump_append(context, L"&lt;", 4);
        else if (*p == '>')
            node_dump_append(context, L"&gt;", 4);
        else if (*p == '&')
            node_dump_append(context, L"&amp;", 5);
        else if (*p == '\n')
            node_dump_append(context, L"\r\n", 2);
        else
            node_dump_append(context, p, 1);
        ++p;
    }
}

static void node_dump_xml_attr(struct domnode *node, struct node_dump_context *context)
{
    BSTR p = node->data;

    while (p && *p)
    {
        if (*p == '<')
            node_dump_append(context, L"&lt;", 4);
        else if (*p == '>')
            node_dump_append(context, L"&gt;", 4);
        else if (*p == '&')
            node_dump_append(context, L"&amp;", 5);
        else if (*p == '"')
            node_dump_append(context, L"&quot;", 6);
        else
            node_dump_append(context, p, 1);
        ++p;
    }
}

static void node_dump_xml(struct domnode *node, struct node_dump_context *context)
{
    struct string_buffer *buffer = &context->buffer;
    BSTR p = node->data;

    while (p && *p)
    {
        if (*p == '\n')
            string_append(buffer, L"\r\n", 2);
        else
            string_append(buffer, p, 1);
        ++p;
    }
}

static void node_dump_pi(struct domnode *node, struct node_dump_context *context)
{
    VARIANT value;

    node_dump_append(context, L"<?", 2);
    node_dump_append(context, node->name, SysStringLen(node->name));
    if (!wcscmp(node->name, L"xml"))
    {
        /* Make sure attributes are in correct order. */

        node_dump_append(context, L" version=\"", 10);
        if (node_get_attribute_value(node, L"version", &value) == S_OK)
        {
            node_dump_append(context, V_BSTR(&value), SysStringLen(V_BSTR(&value)));
            VariantClear(&value);
        }
        else
        {
            node_dump_append(context, L"1.0", 3);
        }
        node_dump_append(context, L"\"", 1);

        if (node_get_attribute_value(node, L"encoding", &value) == S_OK)
        {
            if ((context->only_utf16_encoding_decl && !wcsicmp(V_BSTR(&value), L"UTF-16"))
                    || !context->only_utf16_encoding_decl)
            {
                node_dump_append(context, L" encoding=\"", 11);
                node_dump_append(context, V_BSTR(&value), SysStringLen(V_BSTR(&value)));
                node_dump_append(context, L"\"", 1);
            }
            VariantClear(&value);
        }

        if (node_get_attribute_value(node, L"standalone", &value) == S_OK)
        {
            node_dump_append(context, L" standalone=\"", 13);
            node_dump_append(context, V_BSTR(&value), SysStringLen(V_BSTR(&value)));
            node_dump_append(context, L"\"", 1);
            VariantClear(&value);
        }
    }
    else
    {
        node_dump_append(context, L" ", 1);
        node_dump_xml(node, context);
    }
    node_dump_append(context, L"?>", 2);
}

static void node_dump_format(struct node_dump_context *context)
{
    struct string_buffer *buffer = &context->buffer;
    unsigned int indent = context->indent;

    if (!context->needs_formatting)
        return;

    string_append(buffer, L"\r\n", 2);

    while (indent--)
    {
        string_append(buffer, L"\t", 1);
    }
}

static void node_dump_qualified_name(struct node_dump_context *context, struct domnode *node)
{
    node_dump_append(context, node->qname, SysStringLen(node->qname));
}

static bool is_same_namespace_prefix(const struct domnode *node, const WCHAR *prefix)
{
    if (node->prefix)
        return prefix && !wcscmp(node->prefix, prefix);

    return !prefix;
}

static bool is_namespace_definition(struct domnode *node)
{
    if (!wcscmp(node->qname, L"xmlns"))
        return true;
    if (is_same_namespace_prefix(node, L"xmlns"))
        return true;
    return false;
}

static bool is_namespace_defined(struct node_dump_context *context, struct domnode *node)
{
    struct node_dump_ns *ns;

    if (!node->uri)
        return true;

    /* The xmlns:xml namespace is predefined. */
    if (is_same_namespace_prefix(node, L"xml"))
        return true;

    LIST_FOR_EACH_ENTRY_REV(ns, &context->namespaces, struct node_dump_ns, entry)
    {
        if (is_same_namespace_prefix(node, ns->prefix))
            return !wcscmp(node->uri, ns->uri);
    }

    return false;
}

static void node_dump_namespace_for_node(struct node_dump_context *context, struct domnode *node)
{
    node_dump_push_namespace(context, node->prefix, node->uri, false);
    node_dump_append(context, L" xmlns", 6);
    if (node->prefix)
    {
        node_dump_append(context, L":", 1);
        node_dump_append(context, node->prefix, SysStringLen(node->prefix));
    }
    node_dump_append(context, L"=\"", 2);
    node_dump_append(context, node->uri, SysStringLen(node->uri));
    node_dump_append(context, L"\"", 1);
}

static WCHAR *node_dump_get_namespace_prefix(struct domnode *attr)
{
    return attr->prefix ? attr->name : NULL;
}

static void node_dump_element_attributes(struct node_dump_context *context, struct domnode *node)
{
    struct domnode *attr;
    BSTR text;

    /* Collect explicitly defined namespaces */
    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        if (is_namespace_definition(attr))
        {
            node_get_text(attr, &text);
            node_dump_push_namespace(context, node_dump_get_namespace_prefix(attr), text, true);
        }
    }

    /* If namespace is implied, check if it's been defined already. This needs to be done for
       the element too. */

    if (!is_namespace_defined(context, node))
        node_dump_namespace_for_node(context, node);

    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        if (is_namespace_definition(attr))
            continue;

        if (!is_namespace_defined(context, attr))
            node_dump_namespace_for_node(context, attr);
    }

    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        node_dump_append(context, L" ", 1);
        node_dump(attr, context);
    }
}

static void node_dump(struct domnode *node, struct node_dump_context *context)
{
    struct domnode *n;

    switch (node->type)
    {
        case NODE_COMMENT:
            node_dump_format(context);
            node_dump_append(context, L"<!--", 4);
            node_dump_xml(node, context);
            node_dump_append(context, L"-->", 3);
            context->needs_formatting = node->flags & DOMNODE_IGNORED_WS;
            break;

        case NODE_PROCESSING_INSTRUCTION:
            node_dump_format(context);
            node_dump_pi(node, context);
            context->needs_formatting = node->flags & DOMNODE_IGNORED_WS;
            break;

        case NODE_TEXT:
            node_dump_xml_text(node, context);
            context->needs_formatting = false;
            break;

        case NODE_CDATA_SECTION:
            node_dump_format(context);
            node_dump_append(context, L"<![CDATA[", 9);
            node_dump_xml(node, context);
            node_dump_append(context, L"]]>", 3);
            context->needs_formatting = node->flags & DOMNODE_IGNORED_WS;
            break;

        case NODE_ELEMENT:
            node_dump_format(context);
            node_dump_append(context, L"<", 1);
            node_dump_qualified_name(context, node);
            node_dump_element_attributes(context, node);
            if (list_empty(&node->children))
            {
                node_dump_append(context, L"/>", 2);
            }
            else
            {
                node_dump_append(context, L">", 1);

                ++context->depth;
                ++context->indent;
                context->needs_formatting = node->flags & DOMNODE_IGNORED_WS_AFTER_STARTTAG;
                LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
                {
                    node_dump(n, context);
                }
                --context->indent;
                --context->depth;

                node_dump_format(context);
                node_dump_append(context, L"</", 2);
                node_dump_qualified_name(context, node);
                node_dump_append(context, L">", 1);
            }
            node_dump_pop_namespaces(context);
            context->needs_formatting = node->flags & DOMNODE_IGNORED_WS;
            break;

        case NODE_DOCUMENT_FRAGMENT:
            LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
            {
                node_dump(n, context);
            }
            break;

        case NODE_ATTRIBUTE:
            node_dump_qualified_name(context, node);
            node_dump_append(context, L"=\"", 2);
            LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
            {
                node_dump_xml_attr(n, context);
            }
            node_dump_append(context, L"\"", 1);
            break;

        case NODE_ENTITY_REFERENCE:
            node_dump_append(context, L"&", 1);
            node_dump_append(context, node->name, SysStringLen(node->name));
            node_dump_append(context, L";", 1);
            break;

        case NODE_DOCUMENT_TYPE:
            node_dump_append(context, node->data, SysStringLen(node->data));
            break;

        default:
            FIXME("Unimplemented for type %d.\n", node->type);
            context->status = E_NOTIMPL;
            break;
    }
}

static void node_dump_context_init(struct node_dump_context *context, UINT codepage, IStream *stream)
{
    memset(context, 0, sizeof(*context));
    string_buffer_init(&context->buffer);
    list_init(&context->namespaces);
    context->buffer.status = &context->status;
    context->codepage = codepage;
    context->stream = stream;
}

static HRESULT node_dump_context_cleanup(struct node_dump_context *context)
{
    HRESULT status = context->status;

    free(context->scratch.data);
    return status;
}

HRESULT node_get_xml(struct domnode *node, BSTR *ret)
{
    struct node_dump_context context;
    struct domnode *n;

    if (!ret)
        return E_INVALIDARG;

    node_dump_context_init(&context, ~0u, NULL);
    context.only_utf16_encoding_decl = true;

    if (node->type == NODE_DOCUMENT)
    {
        LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
        {
            node_dump(n, &context);
            node_dump_append(&context, L"\r\n", 2);
        }
    }
    else
    {
        node_dump(node, &context);
    }
    return string_to_bstr(&context.buffer, ret);
}

/* duplicates xmlBufferWriteQuotedString() logic */
static void xml_write_quotedstring(xmlOutputBufferPtr buf, const xmlChar *string)
{
    const xmlChar *cur, *base;

    if (xmlStrchr(string, '\"'))
    {
        if (xmlStrchr(string, '\''))
        {
            xmlOutputBufferWrite(buf, 1, "\"");
            base = cur = string;

            while (*cur)
            {
                if (*cur == '"')
                {
                    if (base != cur)
                        xmlOutputBufferWrite(buf, cur-base, (const char*)base);
                    xmlOutputBufferWrite(buf, 6, "&quot;");
                    cur++;
                    base = cur;
                }
                else
                    cur++;
            }
            if (base != cur)
                xmlOutputBufferWrite(buf, cur-base, (const char*)base);
            xmlOutputBufferWrite(buf, 1, "\"");
        }
        else
        {
            xmlOutputBufferWrite(buf, 1, "\'");
            xmlOutputBufferWriteString(buf, (const char*)string);
            xmlOutputBufferWrite(buf, 1, "\'");
        }
    }
    else
    {
        xmlOutputBufferWrite(buf, 1, "\"");
        xmlOutputBufferWriteString(buf, (const char*)string);
        xmlOutputBufferWrite(buf, 1, "\"");
    }
}

static int XMLCALL transform_to_stream_write(void *context, const char *buffer, int len)
{
    DWORD written;
    HRESULT hr = ISequentialStream_Write((ISequentialStream *)context, buffer, len, &written);
    return hr == S_OK ? written : -1;
}

/* Output for method "text" */
static void transform_write_text(xmlDocPtr result, xsltStylesheetPtr style, xmlOutputBufferPtr output)
{
    xmlNodePtr cur = result->children;
    while (cur)
    {
        if (cur->type == XML_TEXT_NODE)
            xmlOutputBufferWriteString(output, (const char*)cur->content);

        /* skip to next node */
        if (cur->children)
        {
            if ((cur->children->type != XML_ENTITY_DECL) &&
                (cur->children->type != XML_ENTITY_REF_NODE) &&
                (cur->children->type != XML_ENTITY_NODE))
            {
                cur = cur->children;
                continue;
            }
        }

        if (cur->next) {
            cur = cur->next;
            continue;
        }

        do
        {
            cur = cur->parent;
            if (cur == NULL)
                break;
            if (cur == (xmlNodePtr) style->doc) {
                cur = NULL;
                break;
            }
            if (cur->next) {
                cur = cur->next;
                break;
            }
        } while (cur);
    }
}

#undef XSLT_GET_IMPORT_PTR
#define XSLT_GET_IMPORT_PTR(res, style, name) {          \
    xsltStylesheetPtr st = style;                        \
    res = NULL;                                          \
    while (st != NULL) {                                 \
        if (st->name != NULL) { res = st->name; break; } \
        st = xsltNextImport(st);                         \
    }}

#undef XSLT_GET_IMPORT_INT
#define XSLT_GET_IMPORT_INT(res, style, name) {         \
    xsltStylesheetPtr st = style;                       \
    res = -1;                                           \
    while (st != NULL) {                                \
        if (st->name != -1) { res = st->name; break; }  \
        st = xsltNextImport(st);                        \
    }}

static void transform_write_xmldecl(xmlDocPtr result, xsltStylesheetPtr style, BOOL omit_encoding, xmlOutputBufferPtr output)
{
    int omit_xmldecl, standalone;

    XSLT_GET_IMPORT_INT(omit_xmldecl, style, omitXmlDeclaration);
    if (omit_xmldecl == 1) return;

    XSLT_GET_IMPORT_INT(standalone, style, standalone);

    xmlOutputBufferWriteString(output, "<?xml version=");
    if (result->version)
    {
        xmlOutputBufferWriteString(output, "\"");
        xmlOutputBufferWriteString(output, (const char *)result->version);
        xmlOutputBufferWriteString(output, "\"");
    }
    else
        xmlOutputBufferWriteString(output, "\"1.0\"");

    if (!omit_encoding)
    {
        const xmlChar *encoding;

        /* default encoding is UTF-16 */
        XSLT_GET_IMPORT_PTR(encoding, style, encoding);
        xmlOutputBufferWriteString(output, " encoding=");
        xmlOutputBufferWriteString(output, "\"");
        xmlOutputBufferWriteString(output, encoding ? (const char *)encoding : "UTF-16");
        xmlOutputBufferWriteString(output, "\"");
    }

    /* standalone attribute */
    if (standalone != -1)
        xmlOutputBufferWriteString(output, standalone == 0 ? " standalone=\"no\"" : " standalone=\"yes\"");

    xmlOutputBufferWriteString(output, "?>");
}

static void htmldtd_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc)
{
    xmlDtdPtr cur = doc->intSubset;

    xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if (cur->ExternalID)
    {
        xmlOutputBufferWriteString(buf, " PUBLIC ");
        xml_write_quotedstring(buf, cur->ExternalID);
        if (cur->SystemID)
        {
            xmlOutputBufferWriteString(buf, " ");
            xml_write_quotedstring(buf, cur->SystemID);
        }
    }
    else if (cur->SystemID)
    {
        xmlOutputBufferWriteString(buf, " SYSTEM ");
        xml_write_quotedstring(buf, cur->SystemID);
    }
    xmlOutputBufferWriteString(buf, ">\n");
}

/* Duplicates htmlDocContentDumpFormatOutput() the way we need it - doesn't add trailing newline. */
static void htmldoc_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc, const char *encoding, int format)
{
    xmlElementType type;

    /* force HTML output */
    type = doc->type;
    doc->type = XML_HTML_DOCUMENT_NODE;
    if (doc->intSubset)
        htmldtd_dumpcontent(buf, doc);
    if (doc->children) {
        xmlNodePtr cur = doc->children;
        while (cur) {
            htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
            cur = cur->next;
        }
    }
    doc->type = type;
}

static inline BOOL transform_is_empty_resultdoc(xmlDocPtr result)
{
    return !result->children || ((result->children->type == XML_DTD_NODE) && !result->children->next);
}

static inline BOOL transform_is_valid_method(xsltStylesheetPtr style)
{
    return !style->methodURI || !(style->method && xmlStrEqual(style->method, (const xmlChar *)"xhtml"));
}

/* Helper to write transformation result to specified output buffer. */
static HRESULT node_transform_write(xsltStylesheetPtr style, xmlDocPtr result, BOOL omit_encoding, const char *encoding, xmlOutputBufferPtr output)
{
    const xmlChar *method;
    int indent;

    if (!transform_is_valid_method(style))
    {
        ERR("unknown output method\n");
        return E_FAIL;
    }

    XSLT_GET_IMPORT_PTR(method, style, method)
    XSLT_GET_IMPORT_INT(indent, style, indent);

    if (!method && (result->type == XML_HTML_DOCUMENT_NODE))
        method = (const xmlChar *) "html";

    if (method && xmlStrEqual(method, (const xmlChar *)"html"))
    {
        htmlSetMetaEncoding(result, (const xmlChar *)encoding);
        if (indent == -1)
            indent = 1;
        htmldoc_dumpcontent(output, result, encoding, indent);
    }
    else if (method && xmlStrEqual(method, (const xmlChar *)"xhtml"))
    {
        htmlSetMetaEncoding(result, (const xmlChar *) encoding);
        htmlDocContentDumpOutput(output, result, encoding);
    }
    else if (method && xmlStrEqual(method, (const xmlChar *)"text"))
        transform_write_text(result, style, output);
    else
    {
        transform_write_xmldecl(result, style, omit_encoding, output);

        if (result->children)
        {
            xmlNodePtr child = result->children;

            while (child)
            {
                xmlNodeDumpOutput(output, result, child, 0, indent == 1, encoding);
                if (indent && ((child->type == XML_DTD_NODE) || ((child->type == XML_COMMENT_NODE) && child->next)))
                    xmlOutputBufferWriteString(output, "\r\n");
                child = child->next;
            }
        }
    }

    xmlOutputBufferFlush(output);
    return S_OK;
}

/* For BSTR output is always UTF-16, without 'encoding' attribute */
static HRESULT node_transform_write_to_bstr(xsltStylesheetPtr style, xmlDocPtr result, BSTR *str)
{
    HRESULT hr = S_OK;

    if (transform_is_empty_resultdoc(result))
        *str = SysAllocStringLen(NULL, 0);
    else
    {
        xmlOutputBufferPtr output = xmlAllocOutputBuffer(xmlFindCharEncodingHandler("UTF-16"));
        const xmlChar *content;
        size_t len;

        *str = NULL;
        if (!output)
            return E_OUTOFMEMORY;

        hr = node_transform_write(style, result, TRUE, "UTF-16", output);
        content = xmlBufContent(output->conv);
        len = xmlBufUse(output->conv);
        /* UTF-16 encoder places UTF-16 bom, we don't need it for BSTR */
        content += sizeof(WCHAR);
        *str = SysAllocStringLen((WCHAR*)content, len/sizeof(WCHAR) - 1);
        xmlOutputBufferClose(output);
    }

    return *str ? hr : E_OUTOFMEMORY;
}

static HRESULT node_transform_write_to_stream(xsltStylesheetPtr style, xmlDocPtr result, ISequentialStream *stream)
{
    static const xmlChar *utf16 = (const xmlChar*)"UTF-16";
    xmlOutputBufferPtr output;
    const xmlChar *encoding;
    HRESULT hr;

    if (transform_is_empty_resultdoc(result))
    {
        WARN("empty result document\n");
        return S_OK;
    }

    if (style->methodURI && (!style->method || !xmlStrEqual(style->method, (const xmlChar *) "xhtml")))
    {
        ERR("unknown output method\n");
        return E_FAIL;
    }

    /* default encoding is UTF-16 */
    XSLT_GET_IMPORT_PTR(encoding, style, encoding);
    if (!encoding)
        encoding = utf16;

    output = xmlOutputBufferCreateIO(transform_to_stream_write, NULL, stream, xmlFindCharEncodingHandler((const char*)encoding));
    if (!output)
        return E_OUTOFMEMORY;

    hr = node_transform_write(style, result, FALSE, (const char*)encoding, output);
    xmlOutputBufferClose(output);
    return hr;
}

struct import_buffer
{
    char *data;
    int cur;
    int len;
};

static int XMLCALL import_loader_io_read(void *context, char *out, int len)
{
    struct import_buffer *buffer = (struct import_buffer *)context;

    TRACE("%p, %p, %d\n", context, out, len);

    if (buffer->cur == buffer->len)
        return 0;

    len = min(len, buffer->len - buffer->cur);
    memcpy(out, &buffer->data[buffer->cur], len);
    buffer->cur += len;

    TRACE("read %d\n", len);

    return len;
}

static int XMLCALL import_loader_io_close(void * context)
{
    struct import_buffer *buffer = (struct import_buffer *)context;

    TRACE("%p\n", context);

    free(buffer->data);
    free(buffer);
    return 0;
}

static HRESULT import_loader_onDataAvailable(void *ctxt, char *ptr, DWORD len)
{
    xmlParserInputPtr *input = (xmlParserInputPtr *)ctxt;
    xmlParserInputBufferPtr inputbuffer;
    struct import_buffer *buffer;

    buffer = malloc(sizeof(*buffer));

    buffer->data = malloc(len);
    memcpy(buffer->data, ptr, len);
    buffer->cur = 0;
    buffer->len = len;

    inputbuffer = xmlParserInputBufferCreateIO(import_loader_io_read, import_loader_io_close, buffer,
            XML_CHAR_ENCODING_NONE);
    *input = xmlNewIOInputStream(NULL, inputbuffer, XML_CHAR_ENCODING_NONE);
    if (!*input)
        xmlFreeParserInputBuffer(inputbuffer);

    return *input ? S_OK : E_FAIL;
}

static HRESULT xslt_doc_get_uri(const xmlChar *uri, void *_ctxt, xsltLoadType type, IUri **doc_uri)
{
    xsltStylesheetPtr style = (xsltStylesheetPtr)_ctxt;
    IUri *href_uri;
    HRESULT hr;
    BSTR uriW;

    *doc_uri = NULL;

    uriW = bstr_from_xmlChar(uri);
    hr = CreateUri(uriW, Uri_CREATE_ALLOW_RELATIVE | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &href_uri);
    SysFreeString(uriW);
    if (FAILED(hr))
    {
        WARN("Failed to create href uri, %#lx.\n", hr);
        return hr;
    }

    if (type == XSLT_LOAD_STYLESHEET && style->doc && style->doc->name)
    {
        IUri *base_uri;
        BSTR baseuriW;

        baseuriW = bstr_from_xmlChar((xmlChar *)style->doc->name);
        hr = CreateUri(baseuriW, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &base_uri);
        SysFreeString(baseuriW);
        if (FAILED(hr))
        {
            WARN("Failed to create base uri, %#lx.\n", hr);
            return hr;
        }

        hr = CoInternetCombineIUri(base_uri, href_uri, 0, doc_uri, 0);
        IUri_Release(base_uri);
        if (FAILED(hr))
            WARN("Failed to combine uris, hr %#lx.\n", hr);
    }
    else
    {
        *doc_uri = href_uri;
        IUri_AddRef(*doc_uri);
    }

    IUri_Release(href_uri);

    return hr;
}

xmlDocPtr xslt_doc_default_loader(const xmlChar *uri, xmlDictPtr dict, int options,
    void *_ctxt, xsltLoadType type)
{
    IUri *import_uri = NULL;
    xmlParserInputPtr input;
    xmlParserCtxtPtr pctxt;
    xmlDocPtr doc = NULL;
    IMoniker *moniker;
    HRESULT hr;
    bsc_t *bsc;
    BSTR uriW;

    TRACE("%s, %p, %#x, %p, %d\n", debugstr_a((const char *)uri), dict, options, _ctxt, type);

    pctxt = xmlNewParserCtxt();
    if (!pctxt)
        return NULL;

    if (dict && pctxt->dict)
    {
        xmlDictFree(pctxt->dict);
        pctxt->dict = NULL;
    }

    if (dict)
    {
        pctxt->dict = dict;
        xmlDictReference(pctxt->dict);
    }

    xmlCtxtUseOptions(pctxt, options);

    hr = xslt_doc_get_uri(uri, _ctxt, type, &import_uri);
    if (FAILED(hr))
        goto failed;

    hr = CreateURLMonikerEx2(NULL, import_uri, &moniker, 0);
    if (FAILED(hr))
        goto failed;

    hr = bind_url(moniker, import_loader_onDataAvailable, &input, &bsc);
    IMoniker_Release(moniker);
    if (FAILED(hr))
        goto failed;

    if (FAILED(detach_bsc(bsc)))
        goto failed;

    if (!input)
        goto failed;

    inputPush(pctxt, input);
    xmlParseDocument(pctxt);

    if (pctxt->wellFormed)
    {
        doc = pctxt->myDoc;
        /* Set imported uri, to give nested imports a chance. */
        if (IUri_GetPropertyBSTR(import_uri, Uri_PROPERTY_ABSOLUTE_URI, &uriW, 0) == S_OK)
        {
            doc->name = (char *)xmlchar_from_wcharn(uriW, SysStringLen(uriW), TRUE);
            SysFreeString(uriW);
        }
    }
    else
    {
        doc = NULL;
        xmlFreeDoc(pctxt->myDoc);
        pctxt->myDoc = NULL;
    }

failed:
    xmlFreeParserCtxt(pctxt);
    if (import_uri)
        IUri_Release(import_uri);

    return doc;
}

#ifdef _WIN64

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

struct xsl_scripts
{
    struct
    {
        IActiveScript *script;
        const xmlChar *uri;
    } *entries;
    size_t count;
    size_t capacity;
};

static IActiveScript *xpath_msxsl_script_get_script(xmlXPathParserContextPtr ctxt, const xmlChar *uri)
{
    xsltTransformContextPtr xslt_ctxt = xsltXPathGetTransformContext(ctxt);
    struct xsl_scripts *scripts = xslt_ctxt->userData;

    for (size_t i = 0; i < scripts->count; ++i)
    {
        if (xmlStrEqual(scripts->entries[i].uri, uri))
            return scripts->entries[i].script;
    }

    return NULL;
}

static HRESULT xpath_create_nodelist_from_set(xmlNodeSetPtr set, bool needs_copy, IXMLDOMNodeList **ret)
{
    xmlNodeSet _copy = { 0 };
    xmlXPathObjectPtr obj;
    HRESULT hr;

    if (needs_copy)
    {
        _copy.nodeTab = xmlMalloc(set->nodeNr * sizeof(*set->nodeTab));
        _copy.nodeNr = set->nodeNr;
        _copy.nodeMax = set->nodeNr;
        for (int i = 0; i < set->nodeNr; ++i)
            _copy.nodeTab[i] = xmlCopyNode(set->nodeTab[i], 1);
        set = &_copy;
    }

    obj = xmlXPathNewNodeSetList(set);
    hr = create_selection_from_nodeset(obj, ret);
    xmlFree(_copy.nodeTab);

    return hr;
}

static void xpath_msxsl_script_function(xmlXPathParserContextPtr ctxt, int nargs)
{
    xsltTransformContextPtr xslt_ctxt = xsltXPathGetTransformContext(ctxt);
    xmlXPathContextPtr xpath = ((struct _xsltTransformContext *)xslt_ctxt)->xpathCtxt;
    DISPPARAMS params = { 0 };
    IActiveScript *script;
    VARIANT *args = NULL;
    IDispatch *disp;
    VARIANT result;
    DISPID dispid;
    HRESULT hr;
    BSTR name;

    TRACE("%s:%s\n", xpath->functionURI, xpath->function);

    script = xpath_msxsl_script_get_script(ctxt, xpath->functionURI);
    if (!script)
    {
        WARN("Couldn't find a script for %s.\n", xpath->functionURI);
        return;
    }

    if (FAILED(IActiveScript_GetScriptDispatch(script, NULL, &disp)))
        return;

    name = bstr_from_xmlChar(xpath->function);
    hr = IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, &dispid);
    SysFreeString(name);
    if (FAILED(hr))
    {
        WARN("Couldn't find %s function.\n", xpath->function);
        IDispatch_Release(disp);
        return;
    }

    if (nargs)
    {
        args = calloc(nargs, sizeof(*args));

        for (int i = 0; i < nargs; ++i)
        {
            xmlXPathObjectType obj_type = XPATH_UNDEFINED;
            xmlXPathObjectPtr obj = NULL;
            xmlChar *s;

            /* Since we don't want to impose expectations on argument types,
               first inspect actual type on stack, and then pop with corresponding function. */
            if (ctxt->valueNr > 0)
            {
                obj = ctxt->valueTab[ctxt->valueNr - 1];
                obj_type = obj->type;
            }

            switch (obj_type)
            {
                case XPATH_XSLT_TREE:
                case XPATH_NODESET:
                {
                    xmlNodeSetPtr nodeset = xmlXPathPopNodeSet(ctxt);
                    IXMLDOMNodeList *nodelist;

                    hr = xpath_create_nodelist_from_set(nodeset, obj_type == XPATH_XSLT_TREE, &nodelist);

                    V_VT(&args[i]) = VT_DISPATCH;
                    V_DISPATCH(&args[i]) = (IDispatch *)nodelist;
                    break;
                }
                case XPATH_STRING:
                    s = xmlXPathPopString(ctxt);
                    V_VT(&args[i]) = VT_BSTR;
                    V_BSTR(&args[i]) = bstr_from_xmlChar(s);
                    xmlFree(s);
                    break;
                case XPATH_NUMBER:
                    V_VT(&args[i]) = VT_R8;
                    V_R8(&args[i]) = xmlXPathPopNumber(ctxt);
                    break;
                case XPATH_BOOLEAN:
                    V_VT(&args[i]) = VT_BOOL;
                    V_BOOL(&args[i]) = xmlXPathPopBoolean(ctxt) ? VARIANT_TRUE : VARIANT_FALSE;
                    break;
                default:
                    FIXME("Unexpected XPath (%s) value type %d.\n", xpath->function, obj->type);
                    return;
            }
        }

        params.rgvarg = args;
        params.cArgs = nargs;
    }

    VariantInit(&result);
    hr = IDispatch_Invoke(disp, dispid, &IID_NULL, 0, DISPATCH_METHOD, &params, &result, NULL, NULL);
    if (FAILED(hr))
        WARN("User script Invoke() failed %#lx, function %s.\n", hr, xpath->function);

    if (args)
    {
        for (int i = 0; i < nargs; ++i)
            VariantClear(&args[i]);
        free(args);
    }

    switch (V_VT(&result))
    {
        case VT_BSTR:
        {
            xmlChar *s = xmlchar_from_wcharn(V_BSTR(&result), -1, TRUE);
            xmlXPathReturnString(ctxt, s);
            break;
        }
        case VT_BOOL:
            xmlXPathReturnBoolean(ctxt, !!V_BOOL(&result));
            break;
        case VT_INT:
        case VT_I1:
        case VT_UI1:
        case VT_I2:
        case VT_UI2:
        case VT_I4:
        case VT_UI4:
        case VT_R4:
        case VT_R8:
            VariantChangeType(&result, &result, 0, VT_R8);
            xmlXPathReturnNumber(ctxt, V_R8(&result));
            break;
        case VT_EMPTY:
            xmlXPathReturnEmptyString(ctxt);
            break;
        default:
            FIXME("Unexpected return value %s.\n", debugstr_variant(&result));
            xmlXPathReturnEmptyString(ctxt);
    }

    IDispatch_Release(disp);
    VariantClear(&result);
}

struct msxsl_script_site
{
    IActiveScriptSite IActiveScriptSite_iface;
    IServiceProvider IServiceProvider_iface;
    LONG refcount;

    IServiceProvider *provider;
};

static struct msxsl_script_site *impl_from_IActiveScriptSite(IActiveScriptSite *iface)
{
    return CONTAINING_RECORD(iface, struct msxsl_script_site, IActiveScriptSite_iface);
}

static struct msxsl_script_site *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct msxsl_script_site, IServiceProvider_iface);
}

static HRESULT WINAPI msxsl_script_site_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **obj)
{
    struct msxsl_script_site *site = impl_from_IActiveScriptSite(iface);

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IActiveScriptSite, riid))
    {
        *obj = iface;
        IActiveScriptSite_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid))
    {
        *obj = &site->IServiceProvider_iface;
        IServiceProvider_AddRef(&site->IServiceProvider_iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI msxsl_script_site_AddRef(IActiveScriptSite *iface)
{
    struct msxsl_script_site *site = impl_from_IActiveScriptSite(iface);
    return InterlockedIncrement(&site->refcount);
}

static ULONG WINAPI msxsl_script_site_Release(IActiveScriptSite *iface)
{
    struct msxsl_script_site *site = impl_from_IActiveScriptSite(iface);
    LONG refcount = InterlockedDecrement(&site->refcount);

    if (!refcount)
    {
        if (site->provider)
            IServiceProvider_Release(site->provider);
        free(site);
    }

    return refcount;
}

static HRESULT WINAPI msxsl_script_site_GetLCID(IActiveScriptSite *iface, LCID *lcid)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI msxsl_script_site_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR name,
        DWORD mask, IUnknown **item, ITypeInfo **typeinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI msxsl_script_site_GetDocVersionString(IActiveScriptSite *iface, BSTR *version)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI msxsl_script_site_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *result, const EXCEPINFO *ei)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI msxsl_script_site_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE state)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI msxsl_script_site_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *script_error)
{
    return S_OK;
}

static HRESULT WINAPI msxsl_script_site_OnEnterScript(IActiveScriptSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI msxsl_script_site_OnLeaveScript(IActiveScriptSite *iface)
{
    return S_OK;
}

static const IActiveScriptSiteVtbl msxsl_script_site_vtbl =
{
    msxsl_script_site_QueryInterface,
    msxsl_script_site_AddRef,
    msxsl_script_site_Release,
    msxsl_script_site_GetLCID,
    msxsl_script_site_GetItemInfo,
    msxsl_script_site_GetDocVersionString,
    msxsl_script_site_OnScriptTerminate,
    msxsl_script_site_OnStateChange,
    msxsl_script_site_OnScriptError,
    msxsl_script_site_OnEnterScript,
    msxsl_script_site_OnLeaveScript
};

static HRESULT WINAPI msxsl_script_site_servprov_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    struct msxsl_script_site *site = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_QueryInterface(&site->IActiveScriptSite_iface, riid, obj);
}

static ULONG WINAPI msxsl_script_site_servprov_AddRef(IServiceProvider *iface)
{
    struct msxsl_script_site *site = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_AddRef(&site->IActiveScriptSite_iface);
}

static ULONG WINAPI msxsl_script_site_servprov_Release(IServiceProvider *iface)
{
    struct msxsl_script_site *site = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_Release(&site->IActiveScriptSite_iface);
}

static HRESULT WINAPI msxsl_script_site_servprov_QueryService(IServiceProvider *iface,
        REFGUID service, REFIID riid, void **obj)
{
    struct msxsl_script_site *site = impl_from_IServiceProvider(iface);

    if (site->provider)
        return IServiceProvider_QueryService(site->provider, service, riid, obj);

    *obj = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl msxsl_script_site_servprov_vtbl =
{
    msxsl_script_site_servprov_QueryInterface,
    msxsl_script_site_servprov_AddRef,
    msxsl_script_site_servprov_Release,
    msxsl_script_site_servprov_QueryService,
};

static HRESULT msxsl_create_script_site(IXMLDOMDocument *doc, IActiveScriptSite **obj)
{
    struct msxsl_script_site *object;
    IServiceProvider *provider = NULL;
    IObjectWithSite *ows;
    HRESULT hr;

    if (FAILED(hr = IXMLDOMDocument_QueryInterface(doc, &IID_IObjectWithSite, (void **)&ows)))
        return hr;

    IObjectWithSite_GetSite(ows, &IID_IServiceProvider, (void **)&provider);
    IObjectWithSite_Release(ows);

    object = calloc(1, sizeof(*object));
    object->IActiveScriptSite_iface.lpVtbl = &msxsl_script_site_vtbl;
    object->IServiceProvider_iface.lpVtbl = &msxsl_script_site_servprov_vtbl;
    object->refcount = 1;
    object->provider = provider;

    *obj = &object->IActiveScriptSite_iface;

    return S_OK;
}

static void node_transform_bind_scripts(xsltTransformContextPtr ctxt, IXMLDOMDocument *owner_doc,
        xmlDocPtr sheet, struct xsl_scripts *scripts)
{
    IActiveScriptSite *script_site;
    xmlNodePtr root, child, node;
    DWORD supported, enabled = 0;
    IObjectSafety *object_safety;
    xmlNsPtr ns;

    IXMLDOMDocument_QueryInterface(owner_doc, &IID_IObjectSafety, (void **)&object_safety);
    IObjectSafety_GetInterfaceSafetyOptions(object_safety, NULL, &supported, &enabled);
    IObjectSafety_Release(object_safety);

    if (FAILED(msxsl_create_script_site(owner_doc, &script_site)))
        return;

    root = xmlDocGetRootElement(sheet);
    for (node = root->children; node; node = node->next)
    {
        if (xmlStrEqual(node->name, BAD_CAST "script")
                && node->ns
                && xmlStrEqual(node->ns->prefix, BAD_CAST "msxsl")
                && xmlStrEqual(node->ns->href, BAD_CAST "urn:schemas-microsoft-com:xslt"))
        {
             child = node->children;

             if (child && child->type == XML_CDATA_SECTION_NODE)
             {
                 IActiveScript *active_script;
                 xmlChar *language, *prefix;
                 IActiveScriptParse *parser;
                 TYPEATTR *typeattr;
                 BSTR text, progid;
                 IDispatch *disp;
                 ITypeInfo *ti;
                 CLSID clsid;
                 HRESULT hr;

                 if (!(prefix = xmlGetProp(node, BAD_CAST "implements-prefix")))
                 {
                     WARN("<msxsl:script> without 'implements-prefix'.\n");
                     continue;
                 }

                 if (!(ns = xmlSearchNs(sheet, node, prefix)))
                 {
                     WARN("Couldn't locate script element namespace for \"%s\".\n", debugstr_a((char *)prefix));
                     continue;
                 }

                 if (!(language = xmlGetProp(node, BAD_CAST "language")))
                     language = BAD_CAST "javascript";

                 progid = bstr_from_xmlChar(language);
                 if (FAILED(hr = CLSIDFromProgID(progid, &clsid)))
                     WARN("Unknown engine progid %s.\n", debugstr_w(progid));
                 SysFreeString(progid);
                 if (FAILED(hr))
                     return;

                 if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                         &IID_IActiveScript, (void **)&active_script)))
                 {
                     WARN("Failed to create a script engine instance, hr %#lx.\n", hr);
                     continue;
                 }

                 IActiveScript_QueryInterface(active_script, &IID_IObjectSafety, (void **)&object_safety);
                 IObjectSafety_SetInterfaceSafetyOptions(object_safety, NULL, enabled, enabled);
                 IObjectSafety_Release(object_safety);

                 IActiveScript_QueryInterface(active_script, &IID_IActiveScriptParse, (void **)&parser);
                 IActiveScript_SetScriptSite(active_script, script_site);
                 IActiveScriptParse_InitNew(parser);
                 IActiveScript_SetScriptState(active_script, SCRIPTSTATE_STARTED);

                 text = bstr_from_xmlChar(child->content);
                 hr = IActiveScriptParse_ParseScriptText(parser, text, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
                 IActiveScriptParse_Release(parser);
                 SysFreeString(text);
                 if (FAILED(hr))
                 {
                     WARN("Failed to parse script for the namespace %s, hr %#lx.\n", prefix, hr);
                     IActiveScript_Release(active_script);
                     continue;
                 }

                 IActiveScript_GetScriptDispatch(active_script, NULL, &disp);
                 IDispatch_GetTypeInfo(disp, 0, LOCALE_USER_DEFAULT, &ti);

                 ITypeInfo_GetTypeAttr(ti, &typeattr);

                 for (int i = 0; i < typeattr->cFuncs; ++i)
                 {
                     FUNCDESC *funcdesc;
                     xmlChar *func_name;
                     BSTR name;

                     ITypeInfo_GetFuncDesc(ti, i, &funcdesc);
                     ITypeInfo_GetDocumentation(ti, funcdesc->memid, &name, NULL, NULL, NULL);
                     func_name = xmlchar_from_wchar(name);

                     xsltRegisterExtFunction(ctxt, func_name, ns->href, xpath_msxsl_script_function);

                     SysFreeString(name);
                     free(func_name);
                 }

                 IDispatch_Release(disp);
                 ITypeInfo_Release(ti);

                 array_reserve((void **)&scripts->entries, &scripts->capacity, scripts->count + 1, sizeof(*scripts->entries));

                 scripts->entries[scripts->count].script = active_script;
                 scripts->entries[scripts->count].uri = ns->href;
                 scripts->count++;
             }
        }
    }

    IActiveScriptSite_Release(script_site);
}

HRESULT node_transform_node_params(struct domnode *node, IXMLDOMNode *stylesheet, BSTR *p,
    ISequentialStream *stream, const struct xslprocessor_params *params)
{
    xmlDocPtr sheet_doc, node_doc;
    struct domnode *sheet_domdoc;
    IXMLDOMDocument *owner_doc;
    xsltStylesheetPtr xsltSS;
    xmlNodePtr xmlnode;
    HRESULT hr = S_OK;

    if (!stylesheet || (!p && !stream))
        return E_INVALIDARG;

    if (p) *p = NULL;

    if (FAILED(IXMLDOMNode_QueryInterface(stylesheet, &IID_IXMLDOMDocument, (void **)&owner_doc)))
    {
        if (FAILED(hr = IXMLDOMNode_get_ownerDocument(stylesheet, &owner_doc)))
            return hr;
    }

    sheet_domdoc = get_node_obj(owner_doc);
    if (!sheet_domdoc)
    {
        IXMLDOMDocument_Release(owner_doc);
        return E_FAIL;
    }

    sheet_doc = create_xmldoc_from_domdoc(sheet_domdoc, &xmlnode);
    node_doc = create_xmldoc_from_domdoc(node, &xmlnode);

    xsltSS = xsltParseStylesheetDoc(sheet_doc);
    if (xsltSS)
    {
        struct xsl_scripts scripts = { 0 };
        const char **xslparams = NULL;
        xsltTransformContextPtr ctxt;
        xmlDocPtr result;
        unsigned int i;

        /* convert our parameter list to libxml2 format */
        if (params && params->count)
        {
            struct xslprocessor_par *par;

            i = 0;
            xslparams = malloc((params->count * 2 + 1) * sizeof(char*));
            LIST_FOR_EACH_ENTRY(par, &params->list, struct xslprocessor_par, entry)
            {
                xslparams[i++] = (char*)xmlchar_from_wchar(par->name);
                xslparams[i++] = (char*)xmlchar_from_wchar(par->value);
            }
            xslparams[i] = NULL;
        }

        ctxt = xsltNewTransformContext(xsltSS, node_doc);

        node_transform_bind_scripts(ctxt, owner_doc, sheet_doc, &scripts);
        ctxt->userData = &scripts;

        if (xslparams)
        {
            /* push parameters to user context */
            xsltQuoteUserParams(ctxt, xslparams);
            result = xsltApplyStylesheetUser(xsltSS, node_doc, NULL, NULL, NULL, ctxt);

            for (i = 0; i < params->count*2; i++)
                free((char*)xslparams[i]);
            free(xslparams);
        }
        else
        {
            result = xsltApplyStylesheetUser(xsltSS, node_doc, NULL, NULL, NULL, ctxt);
        }

        for (size_t i = 0; i < scripts.count; ++i)
        {
            if (scripts.entries[i].script)
                IActiveScript_Release(scripts.entries[i].script);
        }
        free(scripts.entries);

        ctxt->userData = NULL;
        xsltFreeTransformContext(ctxt);

        if (result)
        {
            if (stream)
                hr = node_transform_write_to_stream(xsltSS, result, stream);
            else
                hr = node_transform_write_to_bstr(xsltSS, result, p);
            xmlFreeDoc(result);
        }

        xsltFreeStylesheet(xsltSS);
    }
    else
        xmlFreeDoc(sheet_doc);

    xmlFreeDoc(node_doc);

    if (p && !*p) *p = SysAllocStringLen(NULL, 0);

    IXMLDOMDocument_Release(owner_doc);

    return hr;
}

HRESULT node_transform_node(struct domnode *node, IXMLDOMNode *stylesheet, BSTR *p)
{
    return node_transform_node_params(node, stylesheet, p, NULL, NULL);
}

HRESULT node_select_nodes(struct domnode *node, BSTR query, IXMLDOMNodeList **list)
{
    struct domnode *doc = node_get_doc(node);

    if (!query || !list)
        return E_INVALIDARG;

    return create_selection(node, query, doc->properties->XPath, list);
}

HRESULT node_select_singlenode(struct domnode *node, BSTR query, IXMLDOMNode **ret)
{
    IXMLDOMNodeList *list;
    HRESULT hr;

    if (ret)
        *ret = NULL;

    hr = node_select_nodes(node, query, &list);
    if (hr == S_OK)
    {
        hr = IXMLDOMNodeList_nextNode(list, ret);
        IXMLDOMNodeList_Release(list);
    }
    return hr;
}

HRESULT node_get_namespaceURI(struct domnode *node, BSTR *uri)
{
    if (!uri)
        return E_INVALIDARG;

    *uri = NULL;

    return node->uri ? return_bstr(node->uri, uri) : S_FALSE;
}

HRESULT node_attribute_get_namespace_uri(struct domnode *node, BSTR *uri)
{
    struct domnode *doc = node->owner;
    bool version6, defaultns;

    if (!uri)
        return E_INVALIDARG;

    *uri = NULL;

    version6 = doc->properties->version == MSXML6;
    defaultns = !wcscmp(node->qname, L"xmlns");

    if (defaultns || is_same_namespace_prefix(node, L"xmlns"))
    {
        if (version6)
            *uri = SysAllocString(L"http://www.w3.org/2000/xmlns/");
        else if (!node->uri || !defaultns)
            *uri = SysAllocStringLen(NULL, 0);
        else
            *uri = SysAllocString(L"xmlns");

        return *uri ? S_OK : E_OUTOFMEMORY;
    }

    return node_get_namespaceURI(node, uri);
}

HRESULT node_get_prefix(struct domnode *node, BSTR *prefix)
{
    if (!prefix)
        return E_INVALIDARG;

    *prefix = NULL;

    return node->prefix ? return_bstr(node->prefix, prefix) : S_FALSE;
}

HRESULT node_get_attribute(const struct domnode *node, const WCHAR *name, IXMLDOMAttribute **attribute)
{
    struct domnode *attr;
    IUnknown *obj;
    HRESULT hr;

    if (attribute)
        *attribute = NULL;

    if (!parser_is_valid_qualified_name(name))
        return E_FAIL;

    if (!attribute)
        return S_FALSE;

    if (domnode_get_attribute(node, name, &attr) != S_OK)
        return S_FALSE;

    hr = create_attribute(attr, &obj);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(obj, &IID_IXMLDOMAttribute, (void **)attribute);
        IUnknown_Release(obj);
    }

    return hr;
}

HRESULT node_get_qualified_attribute(const struct domnode *node, const WCHAR *name, const WCHAR *uri, IXMLDOMNode **ret)
{
    struct domnode *attr;

    if (!ret)
        return S_FALSE;

    if (ret)
        *ret = NULL;

    if (domnode_get_qualified_attribute(node, name, uri, &attr) != S_OK)
        return S_FALSE;

    return create_node(attr, ret);
}

HRESULT node_remove_qualified_attribute(struct domnode *node, const WCHAR *name, const WCHAR *uri, IXMLDOMNode **ret)
{
    struct domnode *attr;
    HRESULT hr;

    if (!name)
        return E_INVALIDARG;

    if (ret)
        *ret = NULL;

    if ((hr = domnode_get_qualified_attribute(node, name, uri, &attr)) != S_OK)
        return hr;

    if (ret)
        hr = create_node(attr, ret);

    if (SUCCEEDED(hr))
        domnode_unlink_attribute(attr);

    return hr;
}

HRESULT node_get_attribute_by_index(const struct domnode *node, LONG index, IXMLDOMNode **attr)
{
    struct domnode *n, *curr = NULL;

    *attr = NULL;

    if (index < 0)
        return S_FALSE;

    LIST_FOR_EACH_ENTRY(n, &node->attributes, struct domnode, entry)
    {
        curr = n;
        if (!index--) break;
        curr = NULL;
    }

    if (!curr)
        return S_FALSE;

    return create_node(curr, attr);
}

HRESULT node_get_attribute_value(struct domnode *node, const WCHAR *name, VARIANT *value)
{
    struct domnode *attr, *child;
    struct string_buffer buffer;

    if (!name || !value)
        return E_INVALIDARG;

    VariantInit(value);

    if (!parser_is_valid_qualified_name(name))
        return E_FAIL;

    if (domnode_get_attribute(node, name, &attr) != S_OK)
    {
        V_VT(value) = VT_NULL;
        return S_FALSE;
    }

    string_buffer_init(&buffer);
    LIST_FOR_EACH_ENTRY(child, &attr->children, struct domnode, entry)
    {
        string_append(&buffer, child->data, SysStringLen(child->data));
    }

    V_VT(value) = VT_BSTR;
    return string_to_bstr(&buffer, &V_BSTR(value));
}

HRESULT node_set_attribute(struct domnode *node, IXMLDOMNode *attribute, IXMLDOMNode **ret)
{
    struct domnode *attr, *old_attr;
    HRESULT hr;

    if (!attribute)
        return E_INVALIDARG;

    if (!(attr = get_node_obj(attribute)))
        return E_FAIL;

    if (attr->type != NODE_ATTRIBUTE)
        return E_FAIL;

    if (attr->parent)
        return E_FAIL;

    if (ret)
        *ret = NULL;

    if (domnode_get_attribute(node, attr->qname, &old_attr) == S_OK)
    {
        if (ret)
        {
            if (FAILED(hr = create_node(old_attr, ret)))
                return hr;
        }

        list_add_before(&old_attr->entry, &attr->entry);
        attr->parent = node;

        domnode_unlink_attribute(old_attr);
    }
    else
    {
        domnode_append_attribute(node, attr);
    }

    return S_OK;
}

HRESULT node_set_attribute_value(struct domnode *node, const WCHAR *name, const VARIANT *value)
{
    struct parsed_name attr_name;
    struct domnode *attr;
    BSTR attr_value;
    bool match;
    HRESULT hr;
    VARIANT v;

    if (FAILED(parse_qualified_name(name, &attr_name)))
        return E_FAIL;

    if (FAILED(hr = variant_get_str_value(value, &v, &attr_value)))
    {
        parsed_name_cleanup(&attr_name);
        return hr;
    }

    /* Check for conflict with element namespace. It's possible to have no uri set on element,
       while still having qualified name. */
    match = node->uri && is_same_namespace_prefix(node, attr_name.local) && !is_same_uri(node, attr_value);
    parsed_name_cleanup(&attr_name);

    if (match)
    {
        VariantClear(&v);
        return E_INVALIDARG;
    }

    if (domnode_get_attribute(node, name, &attr) != S_OK)
    {
        if (SUCCEEDED(hr = domnode_create(NODE_ATTRIBUTE, name, wcslen(name), NULL, 0, node->owner, &attr)))
            domnode_append_attribute(node, attr);
    }

    if (SUCCEEDED(hr))
        hr = node_put_data(attr, attr_value);
    VariantClear(&v);

    return hr;
}

HRESULT node_get_base_name(struct domnode *node, BSTR *name)
{
    return return_bstr(node->name, name);
}

HRESULT create_node(struct domnode *node, IXMLDOMNode **ret)
{
    IUnknown *obj;
    HRESULT hr;

    *ret = NULL;

    if (!node)
        return S_FALSE;

    switch (node->type)
    {
    case NODE_ELEMENT:
        hr = create_element(node, &obj);
        break;
    case NODE_ATTRIBUTE:
        hr = create_attribute(node, &obj);
        break;
    case NODE_TEXT:
        hr = create_text(node, &obj);
        break;
    case NODE_CDATA_SECTION:
        hr = create_cdata(node, &obj);
        break;
    case NODE_ENTITY_REFERENCE:
        hr = create_entity_ref(node, &obj);
        break;
    case NODE_PROCESSING_INSTRUCTION:
        hr = create_pi(node, &obj);
        break;
    case NODE_COMMENT:
        hr = create_comment(node, &obj);
        break;
    case NODE_DOCUMENT:
        hr = create_domdoc(node, &obj);
        break;
    case NODE_DOCUMENT_FRAGMENT:
        hr = create_doc_fragment(node, &obj);
        break;
    case NODE_DOCUMENT_TYPE:
        hr = create_doc_type(node, &obj);
        break;
    case NODE_ENTITY:
    case NODE_NOTATION:
        FIXME("Unsupported node type %d.\n", node->type);
        return E_NOTIMPL;
    default:
        WARN("Invalid node type %d\n", node->type);
        return E_FAIL;
    }

    if (hr == S_OK)
    {
        hr = IUnknown_QueryInterface(obj, &IID_IXMLDOMNode, (void **)ret);
        IUnknown_Release(obj);
    }

    return hr;
}

struct parse_context
{
    ISAXExtensionHandler extension_handler;
    ISAXContentHandler content_handler;
    ISAXLexicalHandler lexical_handler;
    ISAXXMLReaderExtension *reader_extension;
    ISAXXMLReader *reader;

    struct domnode *node;

    struct string_buffer buffer;

    /* Parsed output */
    struct domnode *root;

    HRESULT status;
};

static void parse_context_node_create(struct parse_context *context, DOMNodeType type,
        const WCHAR *name, int name_len, const WCHAR *uri, int uri_len, struct domnode *owner,
        struct domnode **node)
{
    *node = NULL;

    if (context->status != S_OK)
        return;

    context->status = domnode_create(type, name, name_len, uri, uri_len, owner, node);
}

static void parse_context_append_child(struct parse_context *context, struct domnode *parent, struct domnode *child)
{
    if (context->status == S_OK)
        domnode_append_child(parent, child);
}

static void parse_context_append_attribute(struct parse_context *context, struct domnode *node, struct domnode *attribute)
{
    if (context->status == S_OK)
        domnode_append_attribute(node, attribute);
}

static void parse_context_node_put_data(struct parse_context *context, struct domnode *node, const WCHAR *data, int data_len)
{
    BSTR str;

    if (context->status != S_OK)
        return;

    if (!(str = SysAllocStringLen(data, data_len)))
    {
        context->status = E_OUTOFMEMORY;
        return;
    }

    context->status = node_put_data(node, str);
    SysFreeString(str);
}

static HRESULT parse_context_create_text_node(struct parse_context *c, DOMNodeType type)
{
    bool preserve = is_preserving_whitespace(c->node), space = true;
    bool ignored_whitespace = false;
    struct domnode *node;

    if (c->status != S_OK)
        return c->status;

    if (type == NODE_CDATA_SECTION)
    {
        parse_context_node_create(c, type, NULL, 0, NULL, 0, c->root, &node);
        parse_context_node_put_data(c, node, c->buffer.data, c->buffer.count);
        parse_context_append_child(c, c->node, node);

        c->buffer.count = 0;

        return c->status;
    }

    if (c->buffer.count == 0)
        return S_OK;

    if (!preserve)
    {
        for (size_t i = 0; i < c->buffer.count; ++i)
        {
            if (!xml_is_space(c->buffer.data[i]))
            {
                space = false;
                break;
            }
        }

        if (space)
        {
            c->buffer.count = 0;
            ignored_whitespace = true;
        }
    }

    if (c->buffer.count)
    {
        parse_context_node_create(c, type, NULL, 0, NULL, 0, c->root, &node);
        parse_context_node_put_data(c, node, c->buffer.data, c->buffer.count);
        parse_context_append_child(c, c->node, node);

        c->buffer.count = 0;
    }
    else if (ignored_whitespace)
    {
        if (list_empty(&c->node->children))
        {
            c->node->flags |= DOMNODE_IGNORED_WS_AFTER_STARTTAG;
        }
        else
        {
            struct domnode *last_child = node_from_entry(list_tail(&c->node->children));
            last_child->flags |= DOMNODE_IGNORED_WS;
        }
    }

    return c->status;
}

static struct parse_context *impl_from_ISAXContentHandler(ISAXContentHandler *iface)
{
    return CONTAINING_RECORD(iface, struct parse_context, content_handler);
}

static struct parse_context *impl_from_ISAXExtensionHandler(ISAXExtensionHandler *iface)
{
    return CONTAINING_RECORD(iface, struct parse_context, extension_handler);
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

static HRESULT WINAPI parse_content_handler_putDocumentLocator(ISAXContentHandler *iface, ISAXLocator *locator)
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

static HRESULT WINAPI parse_content_handler_startElement(ISAXContentHandler *iface, const WCHAR *uri, int uri_len,
        const WCHAR *name, int name_len, const WCHAR *qname, int qname_len, ISAXAttributes *attrs)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);
    struct domnode *element, *attr;
    int count, length;
    const WCHAR *str;

    parse_context_create_text_node(c, NODE_TEXT);
    parse_context_node_create(c, NODE_ELEMENT, qname, qname_len, uri, uri_len, c->root, &element);
    parse_context_append_child(c, c->node, element);
    c->node = element;

    /* TODO: error handling */

    if (attrs)
    {
        ISAXAttributes_getLength(attrs, &count);

        for (int i = 0; i < count; ++i)
        {
            ISAXAttributes_getQName(attrs, i, &str, &length);
            ISAXAttributes_getURI(attrs, i, &uri, &uri_len);

            parse_context_node_create(c, NODE_ATTRIBUTE, str, length, uri, uri_len, c->root, &attr);
            parse_context_append_attribute(c, element, attr);

            ISAXAttributes_getValue(attrs, i, &str, &length);
            parse_context_node_put_data(c, attr, str, length);

            if (attr && is_namespace_definition(attr))
                attr->flags |= DOMNODE_READONLY_VALUE;
        }
    }

    return c->status;
}

static HRESULT WINAPI parse_content_handler_endElement(ISAXContentHandler *iface, const WCHAR *uri, int uri_len,
        const WCHAR *name, int name_len, const WCHAR *qname, int qname_len)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);

    parse_context_create_text_node(c, NODE_TEXT);
    c->node = c->node->parent;

    return c->status;
}

static HRESULT WINAPI parse_content_handler_characters(ISAXContentHandler *iface, const WCHAR *chars, int count)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);

    string_append(&c->buffer, chars, count);

    return c->status;
}

static HRESULT WINAPI parse_content_handler_ignorableWhitespace(ISAXContentHandler *iface, const WCHAR *chars, int count)
{
    return S_OK;
}

static HRESULT WINAPI parse_content_handler_processingInstruction(ISAXContentHandler *iface, const WCHAR *target,
        int target_len, const WCHAR *data, int data_len)
{
    struct parse_context *c = impl_from_ISAXContentHandler(iface);
    struct domnode *pi;

    parse_context_create_text_node(c, NODE_TEXT);
    parse_context_node_create(c, NODE_PROCESSING_INSTRUCTION, target, target_len, NULL, 0, c->root, &pi);
    parse_context_append_child(c, c->node, pi);
    parse_context_node_put_data(c, pi, data, data_len);

    return c->status;
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

static HRESULT WINAPI parse_extension_handler_QueryInterface(ISAXExtensionHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_ISAXExtensionHandler)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ISAXExtensionHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI parse_extension_handler_AddRef(ISAXExtensionHandler *iface)
{
    return 2;
}

static ULONG WINAPI parse_extension_handler_Release(ISAXExtensionHandler *iface)
{
    return 1;
}

static HRESULT WINAPI parse_extension_handler_xmldecl(ISAXExtensionHandler *iface,
        BSTR version, BSTR encoding, BSTR standalone)
{
    struct parse_context *c = impl_from_ISAXExtensionHandler(iface);
    struct domnode *pi, *node;

    parse_context_node_create(c, NODE_PROCESSING_INSTRUCTION, L"xml", 3, NULL, 0, c->root, &pi);
    parse_context_append_child(c, c->root, pi);

    parse_context_node_create(c, NODE_ATTRIBUTE, L"version", 7, NULL, 0, c->root, &node);

    parse_context_node_put_data(c, node, version, SysStringLen(version));
    parse_context_append_attribute(c, pi, node);

    if (encoding)
    {
        parse_context_node_create(c, NODE_ATTRIBUTE, L"encoding", 8, NULL, 0, c->root, &node);
        parse_context_node_put_data(c, node, encoding, SysStringLen(encoding));
        parse_context_append_attribute(c, pi, node);
    }

    if (standalone)
    {
        parse_context_node_create(c, NODE_ATTRIBUTE, L"standalone", 10, NULL, 0, c->root, &node);
        parse_context_node_put_data(c, node, standalone, SysStringLen(standalone));
        parse_context_append_attribute(c, pi, node);
    }

    return c->status;
}

static HRESULT WINAPI parse_extension_handler_dtd(ISAXExtensionHandler *iface, BSTR data)
{
    struct parse_context *c = impl_from_ISAXExtensionHandler(iface);

    if (c->node && c->node->type == NODE_DOCUMENT_TYPE)
    {
        if (!(c->node->data = SysAllocStringLen(data, SysStringLen(data))))
            c->status = E_OUTOFMEMORY;
    }

    return c->status;
}

static const ISAXExtensionHandlerVtbl parse_extension_handler_vtbl =
{
    parse_extension_handler_QueryInterface,
    parse_extension_handler_AddRef,
    parse_extension_handler_Release,
    parse_extension_handler_xmldecl,
    parse_extension_handler_dtd,
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
    struct domnode *dtd;

    parse_context_node_create(c, NODE_DOCUMENT_TYPE, name, name_len, NULL, 0, c->root, &dtd);
    parse_context_append_child(c, c->root, dtd);
    c->node = dtd;

    return c->status;
}

static HRESULT WINAPI parse_lexical_handler_endDTD(ISAXLexicalHandler *iface)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);

    if (c->node)
        c->node = c->node->parent;

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
    return parse_context_create_text_node(c, NODE_TEXT);
}

static HRESULT WINAPI parse_lexical_handler_endCDATA(ISAXLexicalHandler *iface)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);
    return parse_context_create_text_node(c, NODE_CDATA_SECTION);
}

static HRESULT WINAPI parse_lexical_handler_comment(ISAXLexicalHandler *iface, const WCHAR *chars, int count)
{
    struct parse_context *c = impl_from_ISAXLexicalHandler(iface);
    struct domnode *comment;

    parse_context_create_text_node(c, NODE_TEXT);
    parse_context_node_create(c, NODE_COMMENT, NULL, 0, NULL, 0, c->root, &comment);
    parse_context_append_child(c, c->node, comment);
    parse_context_node_put_data(c, comment, chars, count);

    return c->status;
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

struct domdoc_properties *domdoc_create_properties(MSXML_VERSION version);

static HRESULT parse_context_init(struct parse_context *c, const struct domdoc_properties *properties)
{
    IUnknown *unk;
    HRESULT hr;
    VARIANT v;

    memset(c, 0, sizeof(*c));
    c->content_handler.lpVtbl = &parse_content_handler_vtbl;
    c->extension_handler.lpVtbl = &parse_extension_handler_vtbl;
    c->lexical_handler.lpVtbl = &parse_lexical_handler_vtbl;
    c->buffer.status = &c->status;

    if (FAILED(hr = SAXXMLReader_create(MSXML3, (void **)&unk)))
        return hr;

    IUnknown_QueryInterface(unk, &IID_ISAXXMLReader, (void **)&c->reader);
    IUnknown_QueryInterface(unk, &IID_ISAXXMLReaderExtension, (void **)&c->reader_extension);
    IUnknown_Release(unk);

    ISAXXMLReader_putContentHandler(c->reader, &c->content_handler);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)&c->lexical_handler;
    ISAXXMLReader_putProperty(c->reader, L"http://xml.org/sax/properties/lexical-handler", v);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)&c->extension_handler;
    ISAXXMLReader_putProperty(c->reader, L"http://winehq.org/sax/properties/extension-handler", v);

    ISAXXMLReader_putFeature(c->reader, L"prohibit-dtd", properties->prohibit_dtd ? VARIANT_TRUE : VARIANT_FALSE);

    domnode_create(NODE_DOCUMENT, NULL, 0, NULL, 0, NULL, &c->root);
    c->root->properties = domdoc_create_properties(MSXML_DEFAULT);
    c->root->properties->preserving = properties->preserving;
    c->node = c->root;

    return hr;
}

static void parse_context_cleanup(struct parse_context *c)
{
    free(c->buffer.data);
    if (c->reader)
        ISAXXMLReader_Release(c->reader);
    if (c->reader_extension)
        ISAXXMLReaderExtension_Release(c->reader_extension);
}

HRESULT parse_stream(ISequentialStream *stream, bool utf16, const struct domdoc_properties *properties, struct domnode **tree)
{
    struct parse_context context;
    HRESULT hr;
    VARIANT v;

    *tree = NULL;

    if (FAILED(hr = parse_context_init(&context, properties)))
        return hr;

    if (utf16)
    {
        hr = ISAXXMLReaderExtension_parseUTF16(context.reader_extension, stream);
    }
    else
    {
        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = (IUnknown *)stream;
        hr = ISAXXMLReader_parse(context.reader, v);
    }

    if (hr == S_OK)
    {
        *tree = context.root;
        context.root = NULL;
    }

    parse_context_cleanup(&context);

    return hr;
}

struct xmldoc_context
{
    xmlDocPtr xmldoc;
    struct domnode *node;
    xmlNodePtr *xmlnode;
    xmlNodePtr tree;
};

static xmlNsPtr xmlnode_get_ns(xmlNodePtr tree, struct domnode *node, xmlNodePtr element)
{
    xmlChar *uri, *prefix;
    xmlNsPtr ns;

    if (!node->uri)
        return NULL;

    uri = xmlchar_from_wchar(node->uri);
    prefix = node->prefix ? xmlchar_from_wchar(node->prefix) : NULL;

    if ((ns = xmlSearchNs(tree->doc, element, prefix)))
    {
        if (!xmlStrEqual(ns->href, uri))
            ns = xmlNewNs(element, uri, prefix);
    }
    else
    {
        ns = xmlNewNs(element, uri, prefix);
    }

    free(uri);
    free(prefix);

    return ns;
}

static xmlNodePtr create_xmlnode_element(struct xmldoc_context *context, struct domnode *node)
{
    xmlChar *name, *value, *uri, *prefix;
    struct domnode *attr;
    xmlNodePtr xmlnode;
    xmlAttrPtr xmlattr;
    xmlNsPtr ns;
    BSTR text;

    name = xmlchar_from_wchar(node->name);
    xmlnode = xmlNewDocNode(context->xmldoc, NULL, name, NULL);
    xmlAddChild(context->tree, xmlnode);
    free(name);

    ns = xmlnode_get_ns(context->tree, node, xmlnode);
    xmlSetNs(xmlnode, ns);

    /* Add explicit definitions first. */
    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        if ((attr->prefix && !wcscmp(attr->prefix, L"xmlns"))
                || !wcscmp(attr->qname, L"xmlns"))
        {
            node_get_text(attr, &text);
            uri = xmlchar_from_wchar(text);
            prefix = wcscmp(attr->qname, L"xmlns") ? xmlchar_from_wchar(attr->name) : NULL;

            xmlNewNs(xmlnode, uri, prefix);

            SysFreeString(text);
            free(prefix);
            free(uri);
        }
    }

    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        if ((attr->prefix && !wcscmp(attr->prefix, L"xmlns"))
                || !wcscmp(attr->qname, L"xmlns"))
        {
            continue;
        }

        node_get_text(attr, &text);

        name = xmlchar_from_wchar(attr->name);
        value = xmlchar_from_wchar(text);

        ns = xmlnode_get_ns(xmlnode, attr, xmlnode);
        xmlattr = xmlNewProp(xmlnode, name, value);
        xmlattr->_private2 = attr;
        xmlSetNs((xmlNodePtr)xmlattr, ns);

        if (context->node == attr)
            *context->xmlnode = (xmlNodePtr)xmlattr;

        SysFreeString(text);
        free(value);
        free(name);
    }

    return xmlnode;
}

static xmlDtdPtr create_xmldtd_from_domnode(struct domnode *node)
{
    struct string_buffer buffer;
    xmlDtdPtr dtd = NULL;
    xmlDocPtr doc;
    BSTR str;

    string_buffer_init(&buffer);

    string_append(&buffer, L"<?xml version=\"1.0\"?>", 21);
    string_append(&buffer, node->data, SysStringLen(node->data));
    string_append(&buffer, L"<a/>", 4);
    string_to_bstr(&buffer, &str);

    doc = xmlParseMemory((const char *)str, SysStringByteLen(str));
    SysFreeString(str);

    if ((dtd = xmlGetIntSubset(doc)))
        dtd = xmlCopyDtd(dtd);
    xmlFreeDoc(doc);

    return dtd;
}

static xmlNodePtr create_xmlnode_from_domnode(struct xmldoc_context *context, struct domnode *node)
{
    xmlNodePtr xmlnode = NULL, old_tree;
    xmlChar *name, *data;
    struct domnode *n;
    xmlDtdPtr dtd;

    name = node->name ? xmlchar_from_wchar(node->name) : NULL;
    data = node->data ? xmlchar_from_wchar(node->data) : NULL;

    switch (node->type)
    {
        case NODE_ELEMENT:
            xmlnode = create_xmlnode_element(context, node);
            break;
        case NODE_COMMENT:
            xmlnode = xmlNewDocComment(context->xmldoc, data);
            xmlAddChild(context->tree, xmlnode);
            break;
        case NODE_TEXT:
            xmlnode = xmlNewDocText(context->xmldoc, data);
            xmlAddChild(context->tree, xmlnode);
            break;
        case NODE_PROCESSING_INSTRUCTION:
            xmlnode = xmlNewDocPI(context->xmldoc, name, data);
            xmlAddChild(context->tree, xmlnode);
            break;
        case NODE_CDATA_SECTION:
            xmlnode = xmlNewCDataBlock(context->xmldoc, data, xmlStrlen(data));
            xmlAddChild(context->tree, xmlnode);
            break;
        case NODE_DOCUMENT_TYPE:
            dtd = create_xmldtd_from_domnode(node);
            context->xmldoc->intSubset = dtd;
            break;
        default:
            FIXME("Not supported for node type %d.\n", node->type);
            break;
    }

    free(name);
    free(data);

    if (!xmlnode)
        return NULL;

    xmlnode->_private2 = node;

    if (context->node == node)
        *context->xmlnode = xmlnode;

    old_tree = context->tree;
    if (node->type == NODE_ELEMENT) context->tree = xmlnode;
    LIST_FOR_EACH_ENTRY(n, &node->children, struct domnode, entry)
    {
        create_xmlnode_from_domnode(context, n);
    }
    context->tree = old_tree;

    return xmlnode;
}

/* Create a whole libxml document tree, return pointer to corresponding node as well */
xmlDocPtr create_xmldoc_from_domdoc(struct domnode *node, xmlNodePtr *xmlnode)
{
    struct xmldoc_context context = { 0 };
    struct domnode *n, *doc;
    xmlDocPtr xmldoc;

    *xmlnode = NULL;

    doc = node->owner ? node->owner : node;
    xmldoc = xmlNewDoc(NULL);
    xmldoc->_private2 = doc;

    context.xmldoc = xmldoc;
    context.node = node;
    context.xmlnode = xmlnode;

    if (node == doc)
        *context.xmlnode = (xmlNodePtr)xmldoc;

    context.tree = (xmlNodePtr)xmldoc;
    LIST_FOR_EACH_ENTRY(n, &doc->children, struct domnode, entry)
    {
        create_xmlnode_from_domnode(&context, n);
    }

    return xmldoc;
}

HRESULT node_save(struct domnode *doc, IStream *stream)
{
    struct node_dump_context context = { 0 };
    struct domnode *child, *attr, *node;
    const WCHAR *encoding = NULL;
    UINT codepage;

    assert(doc->type == NODE_DOCUMENT);

    /* Get desired encoding from declaration. */
    if ((child = domnode_get_first_child(doc)))
    {
        if (child->type == NODE_PROCESSING_INSTRUCTION && !wcscmp(child->name, L"xml"))
        {
            if (domnode_get_attribute(child, L"encoding", &attr) == S_OK)
            {
                if ((node = domnode_get_first_child(attr)))
                    encoding = node->data;
            }
        }
    }

    if (!encoding)
        encoding = L"UTF-8";

    TRACE("Using %s encoding.\n", debugstr_w(encoding));

    if (!(codepage = get_codepage_for_encoding(encoding)))
    {
        FIXME("Unrecognized output encoding %s.\n", debugstr_w(encoding));
        return E_FAIL;
    }

    node_dump_context_init(&context, codepage, stream);

    LIST_FOR_EACH_ENTRY(node, &doc->children, struct domnode, entry)
    {
        node_dump(node, &context);
        node_dump_append(&context, L"\r\n", 2);
    }

    return node_dump_context_cleanup(&context);
}

static HRESULT create_xpath_query_for_tagname(const BSTR name, BSTR *query)
{
    struct string_buffer buffer;
    const WCHAR *p, *end;

    *query = NULL;

    string_buffer_init(&buffer);

    /* Special case - empty tagname - means select all nodes,
       except document itself. */
    if (!*name)
    {
        string_append(&buffer, L"/descendant::node()", 19);
        return string_to_bstr(&buffer, query);
    }

    string_append(&buffer, L"descendant::", 12);

    p = name;
    while (p && *p)
    {
        switch (*p)
        {
            case '/':
            case '*':
                string_append(&buffer, p, 1);
                ++p;
                break;
            default:
                string_append(&buffer, L"*[local-name()='", 16);
                end = p;
                while (*end && *end != '/')
                    ++end;
                string_append(&buffer, p, end - p);
                p = end;
                string_append(&buffer, L"']", 2);
        }
    }

    return string_to_bstr(&buffer, query);
}

HRESULT node_get_elements_by_tagname(struct domnode *node, BSTR name, IXMLDOMNodeList **list)
{
    BSTR query;
    HRESULT hr;

    if (!name || !list)
        return E_INVALIDARG;

    hr = create_xpath_query_for_tagname(name, &query);
    if (SUCCEEDED(hr))
        hr = create_selection(node, query, true, list);
    SysFreeString(query);

    return hr;
}

XDR_DT node_get_data_type(const struct domnode *node)
{
    struct domnode *doc = node->owner, *attr;
    IXMLDOMSchemaCollection2 *schema;
    XDR_DT dt = DT_INVALID;
    HRESULT hr;
    BSTR value;

    if (node->type != NODE_ELEMENT)
        return dt;

    if (is_same_uri(node, L"urn:schemas-microsoft-com:datatypes"))
    {
        dt = bstr_to_dt(node->name, -1);
    }
    else
    {
        domnode_get_qualified_attribute(node, L"dt", L"urn:schemas-microsoft-com:datatypes", &attr);
        if (attr)
        {
            if (FAILED(hr = node_get_text(attr, &value)))
                return dt;

            dt = bstr_to_dt(value, -1);
            SysFreeString(value);
        }
        else if ((schema = doc->properties->schemaCache))
        {
            dt = SchemaCache_get_node_dt(schema, node->name, node->uri);
        }
    }

    TRACE("=> dt:%s\n", debugstr_dt(dt));
    return dt;
}

HRESULT create_attribute_node(const WCHAR *name, const WCHAR *uri, struct domnode *owner, IXMLDOMNode **ret)
{
    struct domnode *attr;
    HRESULT hr;

    *ret = NULL;

    if (FAILED(hr = domnode_create(NODE_ATTRIBUTE, name, wcslen(name), uri, wcslen(uri), owner, &attr)))
        return hr;

    return create_node(attr, ret);
}

static void LIBXML2_LOG_CALLBACK validate_error(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_ERR(domdoc_validateNode, msg, ap);
    va_end(ap);
}

static void LIBXML2_LOG_CALLBACK validate_warning(void* ctx, char const* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    LIBXML2_CALLBACK_WARN(domdoc_validateNode, msg, ap);
    va_end(ap);
}

HRESULT node_validate(struct domnode *doc, IXMLDOMNode *node_obj, IXMLDOMParseError **err)
{
    IXMLDOMSchemaCollection2 *schema;
    struct domnode *node;
    LONG err_code = 0;
    HRESULT hr = S_OK;
    int validated = 0;
    xmlDocPtr xmldoc;
    xmlNodePtr xmlnode;

    if (!(node = get_node_obj(node_obj)))
        return E_FAIL;

    if (node->owner != doc && node != doc)
    {
        if (err)
            *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);
        return E_FAIL;
    }

    /* TODO: for now simply treat empty document as not well-formed */
    if (list_empty(&node->children))
    {
        if (err)
            *err = create_parseError(E_XML_NOTWF, NULL, NULL, NULL, 0, 0, 0);
        return S_FALSE;
    }

    xmldoc = create_xmldoc_from_domdoc(node, &xmlnode);

    /* DTD validation */
    if (xmldoc->intSubset || xmldoc->extSubset)
    {
        xmlValidCtxtPtr vctx = xmlNewValidCtxt();
        int ret;

        vctx->error = validate_error;
        vctx->warning = validate_warning;
        ++validated;

        ret = node == doc ? xmlValidateDocument(vctx, xmldoc) : xmlValidateElement(vctx, xmldoc, xmlnode);
        if (!ret)
        {
            /* TODO: get a real error code here */
            TRACE("DTD validation failed\n");
            err_code = E_XML_INVALID;
            hr = S_FALSE;
        }
        xmlFreeValidCtxt(vctx);
    }

    /* Schema validation */
    schema = doc->properties->schemaCache;
    if (hr == S_OK && schema)
    {

        hr = SchemaCache_validate_tree(schema, xmlnode);
        if (SUCCEEDED(hr))
        {
            ++validated;
            /* TODO: get a real error code here */
            if (hr == S_OK)
            {
                TRACE("schema validation succeeded\n");
            }
            else
            {
                WARN("schema validation failed\n");
                err_code = E_XML_INVALID;
            }
        }
        else
        {
            /* not really OK, just didn't find a schema for the ns */
            hr = S_OK;
        }
    }
    xmlFreeDoc(xmldoc);

    if (!validated)
    {
        WARN("no DTD or schema found\n");
        err_code = E_XML_NODTD;
        hr = S_FALSE;
    }

    if (err)
        *err = create_parseError(err_code, NULL, NULL, NULL, 0, 0, 0);

    return hr;
}

/* The only use case for this for loading documents. Original document node is preserved,
   together with its properties, and unlinked owned nodes. Current children nodes are
   unlinked, and replaced with children from the source. Such pattern makes sense only
   for document nodes. The key feature is to preserved owned unlinked nodes. */
void node_move_children(struct domnode *dest, struct domnode *src)
{
    struct domnode *child, *next;

    domnode_unlink_children(dest);

    LIST_FOR_EACH_ENTRY_SAFE(child, next, &src->children, struct domnode, entry)
    {
        domnode_append_child(dest, child);
    }
}

HRESULT node_split_text(struct domnode *node, LONG offset, IXMLDOMText **out)
{
    struct domnode *child;
    ULONG length;
    HRESULT hr;
    BSTR head;

    if (!out || offset < 0)
        return E_INVALIDARG;

    *out = NULL;

    length = SysStringLen(node->data);
    if (offset > length)
        return E_INVALIDARG;
    if (offset == length)
        return S_FALSE;

    if (FAILED(hr = domnode_create(node->type, NULL, 0, NULL, 0, node->owner, &child)))
        return hr;

    if (offset)
    {
        child->data = SysAllocStringLen(node->data + offset, length - offset);
        head = SysAllocStringLen(node->data, offset);
        SysFreeString(node->data);
        node->data = head;
    }
    else
    {
        child->data = node->data;
        node->data = NULL;
    }

    if (node->parent)
        domnode_insert_domnode(node->parent, child, domnode_get_next_sibling(node));

    if (FAILED(hr = create_node(child, (IXMLDOMNode **)out)))
        domnode_unlink_child(child);

    return hr;
}
