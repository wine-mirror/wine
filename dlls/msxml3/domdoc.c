/*
 *    DOM Document implementation
 *
 * Copyright 2005 Mike McCormack
 * Copyright 2010-2011 Adam Martinson for CodeWeavers
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
#include <libxml/xpathInternals.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "olectl.h"
#include "msxml6.h"
#include "wininet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "objsafe.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct ConnectionPoint ConnectionPoint;
typedef struct domdoc domdoc;

struct ConnectionPoint
{
    IConnectionPoint IConnectionPoint_iface;
    const IID *iid;

    ConnectionPoint *next;
    IConnectionPointContainer *container;
    domdoc *doc;

    union
    {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;
};

typedef enum {
    EVENTID_READYSTATECHANGE = 0,
    EVENTID_DATAAVAILABLE,
    EVENTID_TRANSFORMNODE,
    EVENTID_LAST
} eventid_t;

struct domdoc
{
    DispatchEx dispex;
    IXMLDOMDocument3          IXMLDOMDocument3_iface;
    IPersistStreamInit        IPersistStreamInit_iface;
    IObjectWithSite           IObjectWithSite_iface;
    IObjectSafety             IObjectSafety_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG refcount;
    VARIANT_BOOL async;
    VARIANT_BOOL resolving;
    HRESULT error;

    struct domnode *node;

    /* IObjectWithSite */
    IUnknown *site;
    IUri *base_uri;

    /* IObjectSafety */
    DWORD safeopt;

    /* connection list */
    ConnectionPoint *cp_list;
    ConnectionPoint cp_domdocevents;
    ConnectionPoint cp_propnotif;
    ConnectionPoint cp_dispatch;

    /* events */
    IDispatch *events[EVENTID_LAST];

    IXMLDOMSchemaCollection2 *namespaces;
};

static HRESULT set_doc_event(domdoc *doc, eventid_t eid, const VARIANT *v)
{
    IDispatch *disp;

    switch (V_VT(v))
    {
    case VT_UNKNOWN:
        if (V_UNKNOWN(v))
            IUnknown_QueryInterface(V_UNKNOWN(v), &IID_IDispatch, (void**)&disp);
        else
            disp = NULL;
        break;
    case VT_DISPATCH:
        disp = V_DISPATCH(v);
        if (disp) IDispatch_AddRef(disp);
        break;
    default:
        return DISP_E_TYPEMISMATCH;
    }

    if (doc->events[eid]) IDispatch_Release(doc->events[eid]);
    doc->events[eid] = disp;

    return S_OK;
}

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPoint, IConnectionPoint_iface);
}

int registerNamespaces(struct domnode *node, xmlXPathContextPtr ctxt)
{
    struct domnode *doc = node->owner ? node->owner : node;
    const select_ns_entry* ns = NULL;
    int n = 0;

    LIST_FOR_EACH_ENTRY( ns, &doc->properties->selectNsList, select_ns_entry, entry )
    {
        xmlXPathRegisterNs(ctxt, ns->prefix, ns->href);
        ++n;
    }

    return n;
}

void domdoc_properties_clear_selection_namespaces(struct list *pNsList)
{
    select_ns_entry *ns, *ns2;
    LIST_FOR_EACH_ENTRY_SAFE( ns, ns2, pNsList, select_ns_entry, entry )
    {
        free(ns);
    }
    list_init(pNsList);
}

static void release_namespaces(domdoc *doc)
{
    if (doc->namespaces)
    {
        IXMLDOMSchemaCollection2_Release(doc->namespaces);
        doc->namespaces = NULL;
    }
}

MSXML_VERSION domdoc_version(const struct domnode *doc)
{
    assert(doc->type == NODE_DOCUMENT);
    return doc->properties->version;
}

static void attach_doc_node(domdoc *doc, struct domnode *node)
{
    release_namespaces(doc);

    domnode_addref(node);
    node_move_children(doc->node, node);
    domnode_release(node);
}

static inline domdoc *impl_from_IXMLDOMDocument3( IXMLDOMDocument3 *iface )
{
    return CONTAINING_RECORD(iface, domdoc, IXMLDOMDocument3_iface);
}

static inline domdoc *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, domdoc, IPersistStreamInit_iface);
}

static inline domdoc *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, domdoc, IObjectWithSite_iface);
}

static inline domdoc *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, domdoc, IObjectSafety_iface);
}

static inline domdoc *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, domdoc, IConnectionPointContainer_iface);
}

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface, REFIID riid, void **obj)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument3_QueryInterface(&doc->IXMLDOMDocument3_iface, riid, obj);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument3_AddRef(&doc->IXMLDOMDocument3_iface);
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);
    return IXMLDOMDocument3_Release(&doc->IXMLDOMDocument3_iface);
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *clsid)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);

    TRACE("%p, %p.\n", iface, clsid);

    if (!clsid)
        return E_POINTER;

    *clsid = *DOMDocument_version(doc->node->properties->version);

    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    FIXME("%p: stub!\n", iface);

    return S_FALSE;
}

static HRESULT domdoc_load_from_stream(domdoc *doc, ISequentialStream *stream)
{
    LARGE_INTEGER zero = {{0}};
    struct domnode *node;
    DWORD read, written;
    IStream *hstream;
    BYTE buf[4096];
    HRESULT hr;

    hstream = NULL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &hstream);
    if (FAILED(hr))
        return hr;

    do
    {
        ISequentialStream_Read(stream, buf, sizeof(buf), &read);
        hr = IStream_Write(hstream, buf, read, &written);
    } while (SUCCEEDED(hr) && written != 0 && read != 0);

    if (FAILED(hr))
    {
        ERR("failed to copy stream, hr %#lx.\n", hr);
        IStream_Release(hstream);
        return hr;
    }

    hr = IStream_Seek(hstream, zero, STREAM_SEEK_SET, NULL);
    hr = parse_stream((ISequentialStream *)hstream, false, doc->node->properties, &node);
    IStream_Release(hstream);

    if (!node)
    {
        ERR("Failed to parse xml\n");
        return E_FAIL;
    }

    attach_doc_node(doc, node);
    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, IStream *stream)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);

    TRACE("%p, %p.\n", iface, stream);

    if (!stream)
        return E_INVALIDARG;

    return doc->error = domdoc_load_from_stream(doc, (ISequentialStream *)stream);
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, IStream *stream, BOOL clr_dirty)
{
    domdoc *doc = impl_from_IPersistStreamInit(iface);

    TRACE("%p, %p, %d.\n", iface, stream, clr_dirty);

    return node_save(doc->node, stream);
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface, ULARGE_INTEGER *size)
{
    TRACE("%p, %p.\n", iface, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    TRACE("%p.\n", iface);

    return S_OK;
}

static const IPersistStreamInitVtbl xmldoc_IPersistStreamInit_VTable =
{
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

static const tid_t domdoc_se_tids[] =
{
    IXMLDOMNode_tid,
    IXMLDOMDocument_tid,
    IXMLDOMDocument2_tid,
    IXMLDOMDocument3_tid,
    NULL_tid
};

static HRESULT WINAPI domdoc_QueryInterface(IXMLDOMDocument3 *iface, REFIID riid, void **obj)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IXMLDOMNode) ||
        IsEqualGUID(riid, &IID_IXMLDOMDocument) ||
        IsEqualGUID(riid, &IID_IXMLDOMDocument2)||
        IsEqualGUID(riid, &IID_IXMLDOMDocument3))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(&IID_IPersistStream, riid) ||
             IsEqualGUID(&IID_IPersistStreamInit, riid))
    {
        *obj = &doc->IPersistStreamInit_iface;
    }
    else if (IsEqualGUID(&IID_IObjectWithSite, riid))
    {
        *obj = &doc->IObjectWithSite_iface;
    }
    else if (IsEqualGUID(&IID_IObjectSafety, riid))
    {
        *obj = &doc->IObjectSafety_iface;
    }
    else if( IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(domdoc_se_tids, obj);
    }
    else if (dispex_query_interface(&doc->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (node_query_interface(doc->node, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID(riid, &IID_IConnectionPointContainer))
    {
        *obj = &doc->IConnectionPointContainer_iface;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI domdoc_AddRef( IXMLDOMDocument3 *iface )
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    ULONG refcount = InterlockedIncrement(&doc->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domdoc_Release( IXMLDOMDocument3 *iface )
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    LONG refcount = InterlockedDecrement(&doc->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        int eid;

        if (doc->site)
            IUnknown_Release(doc->site);
        if (doc->base_uri)
            IUri_Release(doc->base_uri);

        domnode_release(doc->node);

        for (eid = 0; eid < EVENTID_LAST; eid++)
            if (doc->events[eid]) IDispatch_Release(doc->events[eid]);

        release_namespaces(doc);
        free(doc);
    }

    return refcount;
}

static HRESULT WINAPI domdoc_GetTypeInfoCount(IXMLDOMDocument3 *iface, UINT *count)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    return IDispatchEx_GetTypeInfoCount(&doc->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domdoc_GetTypeInfo(IXMLDOMDocument3 *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    return IDispatchEx_GetTypeInfo(&doc->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domdoc_GetIDsOfNames(IXMLDOMDocument3 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    return IDispatchEx_GetIDsOfNames(&doc->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domdoc_Invoke(IXMLDOMDocument3 *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    return IDispatchEx_Invoke(&doc->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domdoc_get_nodeName(IXMLDOMDocument3 *iface, BSTR *name)
{
    TRACE("%p, %p.\n", iface, name);

    return return_bstr(L"#document", name);
}

static HRESULT WINAPI domdoc_get_nodeValue(IXMLDOMDocument3 *iface, VARIANT *value)
{
    TRACE("%p, %p.\n", iface, value);

    return return_null_var(value);
}

static HRESULT WINAPI domdoc_put_nodeValue(IXMLDOMDocument3 *iface, VARIANT value)
{
    TRACE("%p, %s.\n", iface, debugstr_variant(&value));

    return E_FAIL;
}

static HRESULT WINAPI domdoc_get_nodeType(IXMLDOMDocument3 *iface, DOMNodeType *type)
{
    TRACE("%p, %p.\n", iface, type);

    *type = NODE_DOCUMENT;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_parentNode(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domdoc_get_childNodes(IXMLDOMDocument3 *iface, IXMLDOMNodeList **list)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, list);

    return node_get_child_nodes(doc->node, list);
}

static HRESULT WINAPI domdoc_get_firstChild(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_first_child(doc->node, node);
}

static HRESULT WINAPI domdoc_get_lastChild(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, node);

    return node_get_last_child(doc->node, node);
}

static HRESULT WINAPI domdoc_get_previousSibling(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domdoc_get_nextSibling(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    TRACE("%p, %p.\n", iface, node);

    return return_null_node(node);
}

static HRESULT WINAPI domdoc_get_attributes(IXMLDOMDocument3 *iface,  IXMLDOMNamedNodeMap **map)
{
    TRACE("%p, %p.\n", iface, map);

    return return_null_ptr((void **)map);
}

static HRESULT WINAPI domdoc_insertBefore(IXMLDOMDocument3 *iface, IXMLDOMNode* newChild,
        VARIANT refChild, IXMLDOMNode** outNewChild)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %s, %p.\n", iface, newChild, debugstr_variant(&refChild), outNewChild);

    return node_insert_before(doc->node, newChild, &refChild, outNewChild);
}

static HRESULT WINAPI domdoc_replaceChild(IXMLDOMDocument3 *iface, IXMLDOMNode *newChild,
        IXMLDOMNode* oldChild, IXMLDOMNode **outOldChild)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %p, %p.\n", iface, newChild, oldChild, outOldChild);

    return node_replace_child(doc->node, newChild, oldChild, outOldChild);
}

static HRESULT WINAPI domdoc_removeChild(IXMLDOMDocument3 *iface, IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %p.\n", iface, child, oldChild);

    return node_remove_child(doc->node, child, oldChild);
}

static HRESULT WINAPI domdoc_appendChild(IXMLDOMDocument3 *iface, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %p.\n", iface, child, outChild);

    return node_append_child(doc->node, child, outChild);
}

static HRESULT WINAPI domdoc_hasChildNodes(IXMLDOMDocument3 *iface, VARIANT_BOOL *ret)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, ret);

    return node_has_childnodes(doc->node, ret);
}

static HRESULT WINAPI domdoc_get_ownerDocument(IXMLDOMDocument3 *iface, IXMLDOMDocument **owner)
{
    TRACE("%p, %p.\n", iface, owner);

    return return_null_node((IXMLDOMNode **)owner);
}

static HRESULT WINAPI domdoc_cloneNode(IXMLDOMDocument3 *iface, VARIANT_BOOL deep, IXMLDOMNode **node)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %d, %p.\n", iface, deep, node);

    return node_clone(doc->node, deep, node);
}

static HRESULT WINAPI domdoc_get_nodeTypeString(IXMLDOMDocument3 *iface, BSTR *p)
{
    TRACE("%p, %p.\n", iface, p);

    return return_bstr(L"document", p);
}

static HRESULT WINAPI domdoc_get_text(IXMLDOMDocument3 *iface, BSTR *p)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, p);

    return node_get_text(doc->node, p);
}

static HRESULT WINAPI domdoc_put_text(IXMLDOMDocument3 *iface, BSTR text)
{
    TRACE("%p, %s.\n", iface, debugstr_w(text));

    return E_FAIL;
}

static HRESULT WINAPI domdoc_get_specified(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_definition(IXMLDOMDocument3 *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_nodeTypedValue(IXMLDOMDocument3 *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domdoc_put_nodeTypedValue(IXMLDOMDocument3 *iface, VARIANT v)
{
    FIXME("%p, %s.\n", iface, debugstr_variant(&v));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_dataType(IXMLDOMDocument3 *iface, VARIANT *v)
{
    TRACE("%p, %p.\n", iface, v);

    return return_null_var(v);
}

static HRESULT WINAPI domdoc_put_dataType(IXMLDOMDocument3 *iface, BSTR p)
{
    FIXME("%p, %s.\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domdoc_get_xml(IXMLDOMDocument3 *iface, BSTR *p)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    return node_get_xml(doc->node, p);
}

static HRESULT WINAPI domdoc_transformNode(IXMLDOMDocument3 *iface, IXMLDOMNode *node, BSTR *p)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %p.\n", iface, node, p);

    return node_transform_node(doc->node, node, p);
}

static HRESULT WINAPI domdoc_selectNodes(IXMLDOMDocument3 *iface, BSTR p, IXMLDOMNodeList **list)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), list);

    return node_select_nodes(doc->node, p, list);
}

static HRESULT WINAPI domdoc_selectSingleNode(IXMLDOMDocument3 *iface, BSTR p, IXMLDOMNode **node)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), node);

    return node_select_singlenode(doc->node, p, node);
}

static HRESULT WINAPI domdoc_get_parsed(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    FIXME("%p, %p stub!\n", iface, v);

    *v = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_namespaceURI(IXMLDOMDocument3 *iface, BSTR *uri)
{
    TRACE("%p, %p.\n", iface, uri);

    return return_null_bstr(uri);
}

static HRESULT WINAPI domdoc_get_prefix(IXMLDOMDocument3 *iface, BSTR *prefix)
{
    TRACE("%p, %p.\n", iface, prefix);

    return return_null_bstr(prefix);
}

static HRESULT WINAPI domdoc_get_baseName(IXMLDOMDocument3 *iface, BSTR *name)
{
    TRACE("%p, %p.\n", iface, name);

    return return_null_bstr(name);
}

static HRESULT WINAPI domdoc_transformNodeToObject(IXMLDOMDocument3 *iface,
        IXMLDOMNode *stylesheet, VARIANT output)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %s.\n", iface, stylesheet, debugstr_variant(&output));

    switch (V_VT(&output))
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
    {
        IXMLDOMDocument *output_doc;
        ISequentialStream *stream;
        HRESULT hr;
        BSTR str;

        if (!V_UNKNOWN(&output))
            return E_INVALIDARG;

        /* FIXME: we're not supposed to query for document interface, should use IStream
           which we don't support currently. */
        if (IUnknown_QueryInterface(V_UNKNOWN(&output), &IID_IXMLDOMDocument, (void **)&output_doc) == S_OK)
        {
            VARIANT_BOOL b;

            if (FAILED(hr = node_transform_node(doc->node, stylesheet, &str)))
                return hr;

            hr = IXMLDOMDocument_loadXML(output_doc, str, &b);
            SysFreeString(str);
            return hr;
        }
        else if (IUnknown_QueryInterface(V_UNKNOWN(&output), &IID_ISequentialStream, (void**)&stream) == S_OK)
        {
            hr = node_transform_node_params(doc->node, stylesheet, NULL, stream, NULL);
            ISequentialStream_Release(stream);
            return hr;
        }
        else
        {
            FIXME("Unsupported destination type.\n");
            return E_INVALIDARG;
        }
    }
    default:
        FIXME("Output type %d not handled.\n", V_VT(&output));
        return E_NOTIMPL;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_doctype(IXMLDOMDocument3 *iface, IXMLDOMDocumentType **doctype)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = NULL;

    TRACE("%p, %p.\n", iface, doctype);

    if (!doctype)
        return E_INVALIDARG;

    LIST_FOR_EACH_ENTRY(node, &doc->node->children, struct domnode, entry)
    {
        if (node->type == NODE_DOCUMENT_TYPE)
        {
            return create_node(node, (IXMLDOMNode **)doctype);
        }
    }

    return return_null_node((IXMLDOMNode **)doctype);
}

static HRESULT WINAPI domdoc_get_implementation(IXMLDOMDocument3 *iface, IXMLDOMImplementation **impl)
{
    TRACE("%p, %p.\n", iface, impl);

    if (!impl)
        return E_INVALIDARG;

    return create_dom_implementation(impl);
}

static struct domnode * domdoc_get_document_element(struct domdoc *doc)
{
    struct domnode *node;

    LIST_FOR_EACH_ENTRY(node, &doc->node->children, struct domnode, entry)
    {
        if (node->type == NODE_ELEMENT)
            return node;
    }

    return NULL;
}

static HRESULT WINAPI domdoc_get_documentElement(IXMLDOMDocument3 *iface, IXMLDOMElement **ret)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *element;
    IXMLDOMNode *node;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    if (!ret)
        return E_INVALIDARG;

    *ret = NULL;

    if ((element = domdoc_get_document_element(doc)))
    {
        if (SUCCEEDED(hr = create_node(element, &node)))
        {
            hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void **)ret);
            IXMLDOMNode_Release(node);
            return hr;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI domdoc_putref_documentElement(IXMLDOMDocument3 *iface, IXMLDOMElement *element)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *current;
    IXMLDOMNode *node;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, element);

    if ((current = domdoc_get_document_element(doc)))
    {
        if (SUCCEEDED(hr = create_node(current, &node)))
        {
            hr = node_replace_child(doc->node, (IXMLDOMNode *)element, node, NULL);
            IXMLDOMNode_Release(node);
        }
        return hr;
    }
    else
    {
        hr = node_append_child(doc->node, (IXMLDOMNode *)element, NULL);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createElement(IXMLDOMDocument3 *iface, BSTR tagname, IXMLDOMElement **element)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(tagname), element);

    if (!element || !tagname) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, tagname, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)element);
        IXMLDOMNode_Release(node);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createDocumentFragment(IXMLDOMDocument3 *iface, IXMLDOMDocumentFragment **frag)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, frag);

    if (!frag) return E_INVALIDARG;

    *frag = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMDocumentFragment, (void**)frag);
        IXMLDOMNode_Release(node);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createTextNode(IXMLDOMDocument3 *iface, BSTR data, IXMLDOMText **text)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(data), text);

    if (!text) return E_INVALIDARG;

    *text = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_TEXT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)text);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMText_put_data(*text, data);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createComment(IXMLDOMDocument3 *iface, BSTR data, IXMLDOMComment **comment)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(data), comment);

    if (!comment) return E_INVALIDARG;

    *comment = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_COMMENT;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)comment);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMComment_put_data(*comment, data);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createCDATASection(IXMLDOMDocument3 *iface, BSTR data, IXMLDOMCDATASection **cdata)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(data), cdata);

    if (!cdata) return E_INVALIDARG;

    *cdata = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_CDATA_SECTION;

    hr = IXMLDOMDocument3_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)cdata);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMCDATASection_put_data(*cdata, data);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createProcessingInstruction(IXMLDOMDocument3 *iface,
        BSTR target, BSTR data, IXMLDOMProcessingInstruction **pi)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_w(target), debugstr_w(data), pi);

    return create_pi_node(doc->node, target, data, pi);
}

static HRESULT WINAPI domdoc_createAttribute(IXMLDOMDocument3 *iface, BSTR name, IXMLDOMAttribute **attribute)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), attribute);

    if (!attribute || !name) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ATTRIBUTE;

    hr = IXMLDOMDocument3_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMAttribute, (void**)attribute);
        IXMLDOMNode_Release(node);
    }

    return hr;
}

static HRESULT WINAPI domdoc_createEntityReference(IXMLDOMDocument3 *iface, BSTR name, IXMLDOMEntityReference **entityref)
{
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), entityref);

    if (!entityref) return E_INVALIDARG;

    *entityref = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ENTITY_REFERENCE;

    hr = IXMLDOMDocument3_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMEntityReference, (void**)entityref);
        IXMLDOMNode_Release(node);
    }

    return hr;
}

static HRESULT WINAPI domdoc_getElementsByTagName(IXMLDOMDocument3 *iface, BSTR name, IXMLDOMNodeList **list)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), list);

    return node_get_elements_by_tagname(doc->node, name, list);
}

static bool get_node_type_from_typestring(const WCHAR *name, DOMNodeType *type)
{
    if (!wcsicmp(name, L"attribute")) *type = NODE_ATTRIBUTE;
    else if (!wcsicmp(name, L"cdatasection")) *type = NODE_CDATA_SECTION;
    else if (!wcsicmp(name, L"comment")) *type = NODE_COMMENT;
    else if (!wcsicmp(name, L"document")) *type = NODE_DOCUMENT;
    else if (!wcsicmp(name, L"documentfragment")) *type = NODE_DOCUMENT_FRAGMENT;
    else if (!wcsicmp(name, L"documentype")) *type = NODE_DOCUMENT_TYPE;
    else if (!wcsicmp(name, L"element")) *type = NODE_ELEMENT;
    else if (!wcsicmp(name, L"entity")) *type = NODE_ENTITY;
    else if (!wcsicmp(name, L"entityreference")) *type = NODE_ENTITY_REFERENCE;
    else if (!wcsicmp(name, L"notation")) *type = NODE_NOTATION;
    else if (!wcsicmp(name, L"processinginstruction")) *type = NODE_PROCESSING_INSTRUCTION;
    else if (!wcsicmp(name, L"text")) *type = NODE_TEXT;
    else return false;

    return true;
}

static HRESULT get_node_type(const VARIANT *v, DOMNodeType * type)
{
    VARIANT tmp;
    HRESULT hr;

    /* Check for type strings first, still allowing BSTR -> I4 conversion. */

    if (V_VT(v) == VT_BSTR)
    {
        if (get_node_type_from_typestring(V_BSTR(v), type))
            return S_OK;
    }

    VariantInit(&tmp);
    hr = VariantChangeType(&tmp, v, 0, VT_I4);
    if(FAILED(hr))
        return E_INVALIDARG;

    *type = V_I4(&tmp);

    return S_OK;
}

static HRESULT WINAPI domdoc_createNode(IXMLDOMDocument3 *iface, VARIANT type, BSTR name,
        BSTR uri, IXMLDOMNode **node)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *domnode;
    DOMNodeType node_type;
    HRESULT hr;

    TRACE("%p, %s, %s, %s, %p.\n", iface, debugstr_variant(&type), debugstr_w(name), debugstr_w(uri), node);

    if (!node)
        return E_INVALIDARG;

    if (FAILED(hr = get_node_type(&type, &node_type)))
        return hr;

    switch (node_type)
    {
        /* Check if we need a name */
        case NODE_ELEMENT:
        case NODE_ATTRIBUTE:
        case NODE_ENTITY_REFERENCE:
        case NODE_PROCESSING_INSTRUCTION:

            if (!name || *name == 0)
                return E_FAIL;
            break;

        /* Unsupported types */
        case NODE_DOCUMENT:
        case NODE_DOCUMENT_TYPE:
        case NODE_ENTITY:
        case NODE_NOTATION:
            return E_INVALIDARG;

        default:
            break;
    }

    *node = NULL;

    if (FAILED(hr = domnode_create(node_type, name, SysStringLen(name), uri, SysStringLen(uri),
            doc->node, &domnode)))
    {
        return hr;
    }

    return create_node(domnode, node);
}

static HRESULT WINAPI domdoc_nodeFromID(IXMLDOMDocument3 *iface, BSTR id, IXMLDOMNode **node)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_w(id), node);

    return E_NOTIMPL;
}

static HRESULT domdoc_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    struct domnode *node = NULL;
    ISequentialStream *stream;
    domdoc *doc = obj;
    HRESULT hr;

    stream_wrapper_create(ptr, len, &stream);
    hr = parse_stream(stream, false, doc->node->properties, &node);
    ISequentialStream_Release(stream);

    if (hr == S_OK)
        attach_doc_node(doc, node);

    return hr;
}

static HRESULT domdoc_load_moniker(domdoc *This, IMoniker *mon)
{
    bsc_t *bsc;
    HRESULT hr;

    hr = bind_url(mon, domdoc_onDataAvailable, This, &bsc);
    if(FAILED(hr))
        return hr;

    return detach_bsc(bsc);
}

static HRESULT WINAPI domdoc_load(IXMLDOMDocument3 *iface, VARIANT source, VARIANT_BOOL *result)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    ISequentialStream *stream = NULL;
    LPWSTR filename = NULL;
    HRESULT hr = S_FALSE;
    struct domnode *node;

    TRACE("%p, %s, %p.\n", iface, debugstr_variant(&source), result);

    if (!result)
        return E_POINTER;

    *result = VARIANT_FALSE;

    switch (V_VT(&source))
    {
    case VT_BSTR:
        filename = V_BSTR(&source);
        break;
    case VT_BSTR|VT_BYREF:
        if (!V_BSTRREF(&source)) return E_INVALIDARG;
        filename = *V_BSTRREF(&source);
        break;
    case VT_ARRAY|VT_UI1:
        {
            SAFEARRAY *psa = V_ARRAY(&source);
            char *str;
            LONG len;
            UINT dim;

            dim = SafeArrayGetDim(psa);
            switch (dim)
            {
            case 0:
                ERR("SAFEARRAY == NULL\n");
                hr = doc->error = E_INVALIDARG;
                break;
            case 1:
                /* Only takes UTF-8 strings.
                 * NOT NULL-terminated. */
                hr = SafeArrayAccessData(psa, (void**)&str);
                if (FAILED(hr))
                {
                    doc->error = hr;
                    WARN("failed to access array data, hr %#lx.\n", hr);
                    break;
                }
                SafeArrayGetUBound(psa, 1, &len);

                if (SUCCEEDED(hr = stream_wrapper_create(str, len + 1, &stream)))
                {
                    hr = doc->error = domdoc_load_from_stream(doc, stream);
                    ISequentialStream_Release(stream);
                }
                SafeArrayUnaccessData(psa);

                if (hr == S_OK)
                {
                    hr = doc->error = S_OK;
                    *result = VARIANT_TRUE;
                }
                else
                {
                    doc->error = E_FAIL;
                    TRACE("failed to parse document\n");
                }

                return doc->error == S_OK ? S_OK : S_FALSE;
            default:
                FIXME("unhandled SAFEARRAY dim: %d\n", dim);
                hr = doc->error = E_NOTIMPL;
            }
        }
        break;
    case VT_UNKNOWN:
    {
        IXMLDOMDocument3 *newdoc = NULL;

        if (!V_UNKNOWN(&source)) return E_INVALIDARG;

        hr = IUnknown_QueryInterface(V_UNKNOWN(&source), &IID_IXMLDOMDocument3, (void **)&newdoc);
        if (hr == S_OK)
        {
            if (newdoc)
            {
                struct domnode *node = get_node_obj((IXMLDOMNode *)newdoc);
                struct domnode *cloned;

                hr = node_clone_domnode(node, true, &cloned);
                IXMLDOMDocument3_Release(newdoc);

                if (FAILED(hr))
                {
                    WARN("Failed to clone a document, hr %#lx.\n", hr);
                    return hr;
                }

                attach_doc_node(doc, cloned);
                if (SUCCEEDED(hr))
                    *result = VARIANT_TRUE;

                return hr;
            }
        }

        hr = IUnknown_QueryInterface(V_UNKNOWN(&source), &IID_IStream, (void**)&stream);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(V_UNKNOWN(&source), &IID_ISequentialStream, (void**)&stream);

        if (hr == S_OK)
        {
            hr = doc->error = domdoc_load_from_stream(doc, stream);
            if (hr == S_OK)
                *result = VARIANT_TRUE;
            ISequentialStream_Release(stream);
            return hr;
        }

        FIXME("unsupported IUnknown type (%#lx) (%p)\n", hr, V_UNKNOWN(&source)->lpVtbl);
        break;
    }
    default:
        FIXME("VT type not supported (%d)\n", V_VT(&source));
    }

    if (filename)
    {
        IUri *uri = NULL;
        IMoniker *mon;

        if (doc->node->properties->uri)
        {
            IUri_Release(doc->node->properties->uri);
            doc->node->properties->uri = NULL;
        }

        hr = create_uri(doc->base_uri, filename, &uri);
        if (SUCCEEDED(hr))
            hr = CreateURLMonikerEx2(NULL, uri, &mon, 0);
        if (SUCCEEDED(hr))
        {
            hr = domdoc_load_moniker(doc, mon);
            IMoniker_Release(mon);
        }

        if (SUCCEEDED(hr))
        {
            //get_doc(doc)->name = (char *)xmlchar_from_wcharn(filename, -1, TRUE);
            doc->node->properties->uri = uri;
            hr = doc->error = S_OK;
            *result = VARIANT_TRUE;
        }
        else
        {
            if (uri)
                IUri_Release(uri);
            doc->error = E_FAIL;
        }
    }

    if (!filename || FAILED(hr))
    {
        hr = domnode_create(NODE_DOCUMENT, NULL, 0, NULL, 0, NULL, &node);
        if (node)
        {
            node->properties = domdoc_create_properties(doc->node->properties->version);
            attach_doc_node(doc, node);
        }
        if (SUCCEEDED(hr))
            hr = S_FALSE;
    }

    TRACE("hr %#lx.\n", hr);

    return hr;
}

static HRESULT WINAPI domdoc_get_readyState(IXMLDOMDocument3 *iface, LONG *value)
{
    FIXME("%p, %p.\n", iface, value);

    if (!value)
        return E_INVALIDARG;

    *value = READYSTATE_COMPLETE;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_parseError(IXMLDOMDocument3 *iface, IXMLDOMParseError **obj)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    BSTR error_string = NULL;

    FIXME("%p, %p semi-stub\n", iface, obj);

    if (doc->error)
        error_string = SysAllocString(L"error");

    *obj = create_parseError(doc->error, NULL, error_string, NULL, 0, 0, 0);
    if (!*obj) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_url(IXMLDOMDocument3 *iface, BSTR *url)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, url);

    if (!url)
        return E_INVALIDARG;

    if (!doc->node->properties->uri)
        return return_null_bstr(url);

    return IUri_GetPropertyBSTR(doc->node->properties->uri, Uri_PROPERTY_DISPLAY_URI, url, 0);
}

static HRESULT WINAPI domdoc_get_async(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, v);

    *v = doc->async;
    return S_OK;
}

static HRESULT WINAPI domdoc_put_async(IXMLDOMDocument3 *iface, VARIANT_BOOL v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %d.\n", iface, v);

    doc->async = v;
    return S_OK;
}

static HRESULT WINAPI domdoc_abort(IXMLDOMDocument3 *iface)
{
    FIXME("%p\n", iface);

    return E_NOTIMPL;
}

/* Don't rely on data to be in BSTR format, treat it as WCHAR string. */
static HRESULT WINAPI domdoc_loadXML(IXMLDOMDocument3 *iface, BSTR data, VARIANT_BOOL *result)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node;
    HRESULT hr = E_FAIL;
    VARIANT_BOOL b;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(data), result);

    if (!result) result = &b;

    *result = VARIANT_FALSE;

    if (data && *data)
    {
        ISequentialStream *stream;
        WCHAR *ptr = data;

        /* Skip leading spaces. */
        if (doc->node->properties->version == MSXML_DEFAULT || doc->node->properties->version == MSXML26)
        {
            while (*ptr && iswspace(*ptr)) ptr++;
        }

        if (SUCCEEDED(hr = stream_wrapper_create(ptr, lstrlenW(ptr) * sizeof(WCHAR), &stream)))
        {
            hr = parse_stream(stream, true, doc->node->properties, &node);
            ISequentialStream_Release(stream);
        }

        if (hr == S_OK)
        {
            attach_doc_node(doc, node);
            *result = VARIANT_TRUE;
        }

        doc->error = hr;

        if (FAILED(hr)) hr = S_FALSE;
    }
    else
    {
        domnode_create(NODE_DOCUMENT, NULL, 0, NULL, 0, NULL, &node);
        node->properties = domdoc_create_properties(doc->node->properties->version);
        attach_doc_node(doc, node);

        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domdoc_save(IXMLDOMDocument3 *iface, VARIANT dest)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    IStream *stream;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_variant(&dest));

    switch (V_VT(&dest))
    {
        case VT_UNKNOWN:
            {
                IUnknown *unk = V_UNKNOWN(&dest);
                IXMLDOMDocument3 *document;

                hr = IUnknown_QueryInterface(unk, &IID_IXMLDOMDocument3, (void **)&document);
                if (hr == S_OK)
                {
                    VARIANT_BOOL success;
                    BSTR xml;

                    hr = IXMLDOMDocument3_get_xml(iface, &xml);
                    if (hr == S_OK)
                    {
                        hr = IXMLDOMDocument3_loadXML(document, xml, &success);
                        SysFreeString(xml);
                    }

                    IXMLDOMDocument3_Release(document);
                    return hr;
                }

                hr = IUnknown_QueryInterface(unk, &IID_IStream, (void **)&stream);
                if (hr == S_OK)
                {
                    hr = node_save(doc->node, stream);
                    IStream_Release(stream);
                }
            }
            break;

    case VT_BSTR:
    case VT_BSTR | VT_BYREF:
        {
            const WCHAR *path;

            path = V_VT(&dest) & VT_BYREF ? *V_BSTRREF(&dest) : V_BSTR(&dest);

            hr = SHCreateStreamOnFileEx(path, STGM_CREATE | STGM_WRITE, FILE_ATTRIBUTE_NORMAL, TRUE, NULL, &stream);
            if (FAILED(hr))
            {
                WARN("Failed to create a file stream, hr %#lx.\n", hr);
                return hr;
            }

            hr = node_save(doc->node, stream);
            IStream_Release(stream);
        }
        break;

    default:
        FIXME("Unhandled destination type %s.\n", debugstr_variant(&dest));
        return S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domdoc_get_validateOnParse(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = doc->node;

    TRACE("%p, %p.\n", iface, v);

    *v = node->properties->validating;
    return S_OK;
}

static HRESULT WINAPI domdoc_put_validateOnParse(IXMLDOMDocument3 *iface, VARIANT_BOOL v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = doc->node;

    TRACE("%p, %d.\n", iface, v);

    node->properties->validating = v;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_resolveExternals(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p.\n", iface, v);

    *v = doc->resolving;
    return S_OK;
}

static HRESULT WINAPI domdoc_put_resolveExternals(IXMLDOMDocument3 *iface, VARIANT_BOOL v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %d.\n", iface, v);

    doc->resolving = v;
    return S_OK;
}

static HRESULT WINAPI domdoc_get_preserveWhiteSpace(IXMLDOMDocument3 *iface, VARIANT_BOOL *v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = doc->node;

    TRACE("%p, %p.\n", iface, v);

    *v = node->properties->preserving;
    return S_OK;
}

static HRESULT WINAPI domdoc_put_preserveWhiteSpace(IXMLDOMDocument3 *iface, VARIANT_BOOL v)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = doc->node;

    TRACE("%p, %d.\n", iface, v);

    node->properties->preserving = v;
    return S_OK;
}

static HRESULT WINAPI domdoc_put_onreadystatechange(IXMLDOMDocument3 *iface, VARIANT event)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %s.\n", iface, debugstr_variant(&event));

    return set_doc_event(doc, EVENTID_READYSTATECHANGE, &event);
}

static HRESULT WINAPI domdoc_put_onDataAvailable(IXMLDOMDocument3 *iface, VARIANT sink)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&sink));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_put_onTransformNode(IXMLDOMDocument3 *iface, VARIANT sink)
{
    FIXME("%p, %s: stub\n", iface, debugstr_variant(&sink));

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_namespaces(IXMLDOMDocument3 *iface, IXMLDOMSchemaCollection **collection)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domnode *node = doc->node;
    HRESULT hr;

    FIXME("%p, %p: semi-stub\n", iface, collection);

    if (!collection)
        return E_POINTER;

    if (!doc->namespaces)
    {
        hr = SchemaCache_create(node->properties->version, (void **)&doc->namespaces);
        if (hr != S_OK) return hr;

        hr = cache_from_doc_ns(doc->namespaces, doc->node);
        if (hr != S_OK)
            release_namespaces(doc);
    }

    if (doc->namespaces)
        return IXMLDOMSchemaCollection2_QueryInterface(doc->namespaces, &IID_IXMLDOMSchemaCollection, (void **)collection);

    return hr;
}

static HRESULT WINAPI domdoc_get_schemas(IXMLDOMDocument3 *iface, VARIANT *schema)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    IXMLDOMSchemaCollection2 *cur_schema;
    struct domnode *node = doc->node;
    HRESULT hr = S_FALSE;

    TRACE("%p, %p.\n", iface, schema);

    V_VT(schema) = VT_NULL;
    /* just to reset pointer part, cause that's what application is expected to use */
    V_DISPATCH(schema) = NULL;

    if ((cur_schema = node->properties->schemaCache))
    {
        hr = IXMLDOMSchemaCollection2_QueryInterface(cur_schema, &IID_IDispatch, (void**)&V_DISPATCH(schema));
        if (SUCCEEDED(hr))
            V_VT(schema) = VT_DISPATCH;
    }
    return hr;
}

static HRESULT WINAPI domdoc_putref_schemas(IXMLDOMDocument3 *iface, VARIANT schema)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    IXMLDOMSchemaCollection2 *new_schema = NULL;
    struct domnode *node = doc->node;
    HRESULT hr = E_FAIL;

    FIXME("%p, %s: semi-stub\n", iface, debugstr_variant(&schema));

    switch(V_VT(&schema))
    {
    case VT_UNKNOWN:
        if (V_UNKNOWN(&schema))
        {
            hr = IUnknown_QueryInterface(V_UNKNOWN(&schema), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
            break;
        }
        /* fallthrough */
    case VT_DISPATCH:
        if (V_DISPATCH(&schema))
        {
            hr = IDispatch_QueryInterface(V_DISPATCH(&schema), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
            break;
        }
        /* fallthrough */
    case VT_NULL:
    case VT_EMPTY:
        hr = S_OK;
        break;

    default:
        WARN("Can't get schema from vt %x\n", V_VT(&schema));
    }

    if (SUCCEEDED(hr))
    {
        IXMLDOMSchemaCollection2 *old_schema = InterlockedExchangePointer((void **)&node->properties->schemaCache, new_schema);
        if (old_schema)
            IXMLDOMSchemaCollection2_Release(old_schema);
    }

    return hr;
}

static HRESULT WINAPI domdoc_validateNode(IXMLDOMDocument3 *iface, IXMLDOMNode *node, IXMLDOMParseError **err)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);

    TRACE("%p, %p, %p.\n", iface, node, err);

    /* TODO: check ready state */

    if (!node)
    {
        if (err)
            *err = create_parseError(0, NULL, NULL, NULL, 0, 0, 0);
        return E_POINTER;
    }

    return node_validate(doc->node, node, err);
}

static HRESULT WINAPI domdoc_validate(IXMLDOMDocument3 *iface, IXMLDOMParseError **err)
{
    TRACE("%p, %p.\n", iface, err);

    return IXMLDOMDocument3_validateNode(iface, (IXMLDOMNode *)iface, err);
}

static HRESULT WINAPI domdoc_setProperty(IXMLDOMDocument3 *iface, BSTR p, VARIANT value)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domdoc_properties *properties = doc->node->properties;
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %s, %s.\n", iface, debugstr_w(p), debugstr_variant(&value));

    if (wcsicmp(p, L"SelectionLanguage") == 0)
    {
        VARIANT varStr;
        BSTR bstr;

        V_VT(&varStr) = VT_EMPTY;
        if (V_VT(&value) != VT_BSTR)
        {
            if (FAILED(hr = VariantChangeType(&varStr, &value, 0, VT_BSTR)))
                return hr;
            bstr = V_BSTR(&varStr);
        }
        else
            bstr = V_BSTR(&value);

        hr = S_OK;
        if (wcsicmp(bstr, L"XPath") == 0)
            properties->XPath = TRUE;
        else if (wcsicmp(bstr, L"XSLPattern") == 0)
            properties->XPath = FALSE;
        else
            hr = E_FAIL;

        VariantClear(&varStr);
        return hr;
    }
    else if (!wcsicmp(p, L"SelectionNamespaces"))
    {
        xmlChar *nsStr = (xmlChar *)properties->selectNsStr;
        struct list *pNsList;
        VARIANT varStr;
        BSTR bstr;

        V_VT(&varStr) = VT_EMPTY;
        if (V_VT(&value) != VT_BSTR)
        {
            if (FAILED(hr = VariantChangeType(&varStr, &value, 0, VT_BSTR)))
                return hr;
            bstr = V_BSTR(&varStr);
        }
        else
            bstr = V_BSTR(&value);

        hr = S_OK;

        pNsList = &properties->selectNsList;
        domdoc_properties_clear_selection_namespaces(pNsList);
        free(nsStr);
        nsStr = xmlchar_from_wchar(bstr);

        TRACE("property value: \"%s\"\n", debugstr_w(bstr));

        properties->selectNsStr = nsStr;
        properties->selectNsStr_len = xmlStrlen(nsStr);
        if (bstr && *bstr)
        {
            xmlChar *pTokBegin, *pTokEnd, *pTokInner;
            select_ns_entry* ns_entry = NULL;

            pTokBegin = nsStr;

            /* skip leading spaces */
            while (*pTokBegin == ' '  || *pTokBegin == '\n' ||
                   *pTokBegin == '\t' || *pTokBegin == '\r')
                ++pTokBegin;

            for (; *pTokBegin; pTokBegin = pTokEnd)
            {
                if (ns_entry)
                    memset(ns_entry, 0, sizeof(select_ns_entry));
                else
                    ns_entry = calloc(1, sizeof(select_ns_entry));

                while (*pTokBegin == ' ')
                    ++pTokBegin;
                pTokEnd = pTokBegin;
                while (*pTokEnd != ' ' && *pTokEnd != 0)
                    ++pTokEnd;

                /* so it failed to advance which means we've got some trailing spaces */
                if (pTokEnd == pTokBegin) break;

                if (xmlStrncmp(pTokBegin, (xmlChar const*)"xmlns", 5) != 0)
                {
                    hr = E_FAIL;
                    WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                          debugstr_w(bstr), debugstr_an((const char*)pTokBegin, pTokEnd-pTokBegin));
                    continue;
                }

                pTokBegin += 5;
                if (*pTokBegin == '=')
                {
                    /*valid for XSLPattern?*/
                    FIXME("Setting default xmlns not supported - skipping.\n");
                    continue;
                }
                else if (*pTokBegin == ':')
                {
                    ns_entry->prefix = ++pTokBegin;
                    for (pTokInner = pTokBegin; pTokInner != pTokEnd && *pTokInner != '='; ++pTokInner)
                        ;

                    if (pTokInner == pTokEnd)
                    {
                        hr = E_FAIL;
                        WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                              debugstr_w(bstr), debugstr_an((const char*)pTokBegin, pTokEnd-pTokBegin));
                        continue;
                    }

                    ns_entry->prefix_end = *pTokInner;
                    *pTokInner = 0;
                    ++pTokInner;

                    if (pTokEnd-pTokInner > 1 &&
                        ((*pTokInner == '\'' && *(pTokEnd-1) == '\'') ||
                         (*pTokInner == '"' && *(pTokEnd-1) == '"')))
                    {
                        ns_entry->href = ++pTokInner;
                        ns_entry->href_end = *(pTokEnd-1);
                        *(pTokEnd-1) = 0;
                        list_add_tail(pNsList, &ns_entry->entry);

                        ns_entry = NULL;
                        continue;
                    }
                    else
                    {
                        WARN("Syntax error in xmlns string: %s\n\tat token: %s\n",
                              debugstr_w(bstr), debugstr_an((const char*)pTokInner, pTokEnd-pTokInner));
                        list_add_tail(pNsList, &ns_entry->entry);

                        ns_entry = NULL;
                        hr = E_FAIL;
                        continue;
                    }
                }
                else
                {
                    hr = E_FAIL;
                    continue;
                }
            }
            free(ns_entry);
        }

        VariantClear(&varStr);
        return hr;
    }
    else if (wcsicmp(p, L"ValidateOnParse") == 0)
    {
        if (properties->version < MSXML4)
            return E_FAIL;
        else
        {
            properties->validating = V_BOOL(&value);
            return S_OK;
        }
    }
    else if (wcsicmp(p, L"NewParser") == 0 ||
             wcsicmp(p, L"ResolveExternals") == 0 ||
             wcsicmp(p, L"AllowXsltScript") == 0 ||
             wcsicmp(p, L"NormalizeAttributeValues") == 0 ||
             wcsicmp(p, L"AllowDocumentFunction") == 0 ||
             wcsicmp(p, L"MaxElementDepth") == 0 ||
             wcsicmp(p, L"UseInlineSchema") == 0)
    {
        /* Ignore */
        FIXME("Ignoring property %s, value %s\n", debugstr_w(p), debugstr_variant(&value));
        return S_OK;
    }

    if (!wcscmp(p, L"ProhibitDTD"))
    {
        V_VT(&v) = VT_EMPTY;
        if (FAILED(hr = VariantChangeType(&v, &value, 0, VT_BOOL)))
        {
            WARN("Failed to convert to boolean.\n");
            return hr;
        }

        properties->prohibit_dtd = V_BOOL(&v) == VARIANT_TRUE;
        return S_OK;
    }

    FIXME("Unknown property %s\n", debugstr_w(p));
    return E_FAIL;
}

static HRESULT WINAPI domdoc_getProperty(IXMLDOMDocument3 *iface, BSTR p, VARIANT *var)
{
    domdoc *doc = impl_from_IXMLDOMDocument3(iface);
    struct domdoc_properties *properties = doc->node->properties;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(p), var);

    if (!var)
        return E_INVALIDARG;

    if (!wcsicmp(p, L"SelectionLanguage"))
    {
        V_VT(var) = VT_BSTR;
        V_BSTR(var) = SysAllocString(properties->XPath ? L"XPath" : L"XSLPattern");
        return V_BSTR(var) ? S_OK : E_OUTOFMEMORY;
    }
    else if (!wcsicmp(p, L"SelectionNamespaces"))
    {
        int lenA, lenW;
        BSTR rebuiltStr, cur;
        const xmlChar *nsStr;
        struct list *pNsList;
        select_ns_entry* pNsEntry;

        V_VT(var) = VT_BSTR;
        nsStr = properties->selectNsStr;
        pNsList = &properties->selectNsList;
        lenA = properties->selectNsStr_len;
        lenW = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)nsStr, lenA+1, NULL, 0);
        rebuiltStr = malloc(lenW * sizeof(WCHAR));
        MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)nsStr, lenA+1, rebuiltStr, lenW);
        cur = rebuiltStr;
        /* this is fine because all of the chars that end tokens are ASCII*/
        LIST_FOR_EACH_ENTRY(pNsEntry, pNsList, select_ns_entry, entry)
        {
            while (*cur != 0) ++cur;
            if (pNsEntry->prefix_end)
            {
                *cur = pNsEntry->prefix_end;
                while (*cur != 0) ++cur;
            }

            if (pNsEntry->href_end)
            {
                *cur = pNsEntry->href_end;
            }
        }
        V_BSTR(var) = SysAllocString(rebuiltStr);
        free(rebuiltStr);
        return S_OK;
    }
    else if (!wcsicmp(p, L"ValidateOnParse"))
    {
        if (properties->version < MSXML4)
            return E_FAIL;
        else
        {
            V_VT(var) = VT_BOOL;
            V_BOOL(var) = properties->validating;
            return S_OK;
        }
    }

    if (!wcscmp(p, L"ProhibitDTD"))
    {
        V_VT(var) = VT_BOOL;
        V_BOOL(var) = properties->prohibit_dtd ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }

    FIXME("Unknown property %s\n", debugstr_w(p));
    return E_FAIL;
}

static HRESULT WINAPI domdoc_importNode(IXMLDOMDocument3 *iface, IXMLDOMNode *node, VARIANT_BOOL deep, IXMLDOMNode **clone)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, node, deep, clone);

    return E_NOTIMPL;
}

static const struct IXMLDOMDocument3Vtbl XMLDOMDocument3Vtbl =
{
    domdoc_QueryInterface,
    domdoc_AddRef,
    domdoc_Release,
    domdoc_GetTypeInfoCount,
    domdoc_GetTypeInfo,
    domdoc_GetIDsOfNames,
    domdoc_Invoke,
    domdoc_get_nodeName,
    domdoc_get_nodeValue,
    domdoc_put_nodeValue,
    domdoc_get_nodeType,
    domdoc_get_parentNode,
    domdoc_get_childNodes,
    domdoc_get_firstChild,
    domdoc_get_lastChild,
    domdoc_get_previousSibling,
    domdoc_get_nextSibling,
    domdoc_get_attributes,
    domdoc_insertBefore,
    domdoc_replaceChild,
    domdoc_removeChild,
    domdoc_appendChild,
    domdoc_hasChildNodes,
    domdoc_get_ownerDocument,
    domdoc_cloneNode,
    domdoc_get_nodeTypeString,
    domdoc_get_text,
    domdoc_put_text,
    domdoc_get_specified,
    domdoc_get_definition,
    domdoc_get_nodeTypedValue,
    domdoc_put_nodeTypedValue,
    domdoc_get_dataType,
    domdoc_put_dataType,
    domdoc_get_xml,
    domdoc_transformNode,
    domdoc_selectNodes,
    domdoc_selectSingleNode,
    domdoc_get_parsed,
    domdoc_get_namespaceURI,
    domdoc_get_prefix,
    domdoc_get_baseName,
    domdoc_transformNodeToObject,
    domdoc_get_doctype,
    domdoc_get_implementation,
    domdoc_get_documentElement,
    domdoc_putref_documentElement,
    domdoc_createElement,
    domdoc_createDocumentFragment,
    domdoc_createTextNode,
    domdoc_createComment,
    domdoc_createCDATASection,
    domdoc_createProcessingInstruction,
    domdoc_createAttribute,
    domdoc_createEntityReference,
    domdoc_getElementsByTagName,
    domdoc_createNode,
    domdoc_nodeFromID,
    domdoc_load,
    domdoc_get_readyState,
    domdoc_get_parseError,
    domdoc_get_url,
    domdoc_get_async,
    domdoc_put_async,
    domdoc_abort,
    domdoc_loadXML,
    domdoc_save,
    domdoc_get_validateOnParse,
    domdoc_put_validateOnParse,
    domdoc_get_resolveExternals,
    domdoc_put_resolveExternals,
    domdoc_get_preserveWhiteSpace,
    domdoc_put_preserveWhiteSpace,
    domdoc_put_onreadystatechange,
    domdoc_put_onDataAvailable,
    domdoc_put_onTransformNode,
    domdoc_get_namespaces,
    domdoc_get_schemas,
    domdoc_putref_schemas,
    domdoc_validate,
    domdoc_setProperty,
    domdoc_getProperty,
    domdoc_validateNode,
    domdoc_importNode
};

/* IConnectionPointContainer */
static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    domdoc *doc = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_QueryInterface(&doc->IXMLDOMDocument3_iface, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    domdoc *doc = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_AddRef(&doc->IXMLDOMDocument3_iface);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    domdoc *doc = impl_from_IConnectionPointContainer(iface);
    return IXMLDOMDocument3_Release(&doc->IXMLDOMDocument3_iface);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    FIXME("%p, %p: stub\n", iface, ppEnum);

    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **cp)
{
    domdoc *doc = impl_from_IConnectionPointContainer(iface);
    ConnectionPoint *iter;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), cp);

    *cp = NULL;

    for (iter = doc->cp_list; iter; iter = iter->next)
    {
        if (IsEqualGUID(iter->iid, riid))
            *cp = &iter->IConnectionPoint_iface;
    }

    if (*cp)
    {
        IConnectionPoint_AddRef(*cp);
        return S_OK;
    }

    FIXME("unsupported riid %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;
}

static const struct IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IConnectionPoint, riid))
    {
        *ppv = iface;
    }

    if (*ppv)
    {
        IConnectionPoint_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(This->container);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(This->container);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *iid)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, iid);

    if (!iid) return E_POINTER;

    *iid = *This->iid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **container)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, container);

    if (!container) return E_POINTER;

    *container = This->container;
    IConnectionPointContainer_AddRef(*container);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *unk_sink,
                                             DWORD *cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    IUnknown *sink;
    HRESULT hr;
    DWORD i;

    TRACE("(%p)->(%p %p)\n", This, unk_sink, cookie);

    hr = IUnknown_QueryInterface(unk_sink, This->iid, (void**)&sink);
    if(FAILED(hr) && !IsEqualGUID(&IID_IPropertyNotifySink, This->iid))
        hr = IUnknown_QueryInterface(unk_sink, &IID_IDispatch, (void**)&sink);
    if(FAILED(hr))
        return CONNECT_E_CANNOTCONNECT;

    if(This->sinks)
    {
        for (i = 0; i < This->sinks_size; i++)
            if (!This->sinks[i].unk)
                break;

        if (i == This->sinks_size)
            This->sinks = realloc(This->sinks, (++This->sinks_size) * sizeof(*This->sinks));
    }
    else
    {
        This->sinks = malloc(sizeof(*This->sinks));
        This->sinks_size = 1;
        i = 0;
    }

    This->sinks[i].unk = sink;
    if (cookie)
        *cookie = i+1;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("%p, %ld.\n", iface, cookie);

    if (cookie == 0 || cookie > This->sinks_size || !This->sinks[cookie-1].unk)
        return CONNECT_E_NOCONNECTION;

    IUnknown_Release(This->sinks[cookie-1].unk);
    This->sinks[cookie-1].unk = NULL;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("(%p)->(%p): stub\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static void ConnectionPoint_Init(ConnectionPoint *cp, struct domdoc *doc, REFIID riid)
{
    cp->IConnectionPoint_iface.lpVtbl = &ConnectionPointVtbl;
    cp->doc = doc;
    cp->iid = riid;
    cp->sinks = NULL;
    cp->sinks_size = 0;

    cp->next = doc->cp_list;
    doc->cp_list = cp;

    cp->container = &doc->IConnectionPointContainer_iface;
}

static HRESULT WINAPI domdoc_ObjectWithSite_QueryInterface(IObjectWithSite* iface, REFIID riid, void **obj)
{
    domdoc *doc = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_QueryInterface(&doc->IXMLDOMDocument3_iface, riid, obj);
}

static ULONG WINAPI domdoc_ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    domdoc *doc = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_AddRef(&doc->IXMLDOMDocument3_iface);
}

static ULONG WINAPI domdoc_ObjectWithSite_Release(IObjectWithSite *iface)
{
    domdoc *doc = impl_from_IObjectWithSite(iface);
    return IXMLDOMDocument3_Release(&doc->IXMLDOMDocument3_iface);
}

static HRESULT WINAPI domdoc_ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID iid, void **obj)
{
    domdoc *doc = impl_from_IObjectWithSite(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(iid), obj);

    if (!doc->site)
        return E_FAIL;

    return IUnknown_QueryInterface(doc->site, iid, obj);
}

static HRESULT WINAPI domdoc_ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *punk)
{
    domdoc *doc = impl_from_IObjectWithSite(iface);

    TRACE("%p, %p.\n", iface, punk);

    if (!punk)
    {
        if (doc->site)
        {
            IUnknown_Release(doc->site);
            doc->site = NULL;
        }

        if (doc->base_uri)
        {
            IUri_Release(doc->base_uri);
            doc->base_uri = NULL;
        }

        return S_OK;
    }

    IUnknown_AddRef(punk);

    if (doc->site)
        IUnknown_Release(doc->site);

    doc->site = punk;
    doc->base_uri = get_base_uri(doc->site);

    return S_OK;
}

static const IObjectWithSiteVtbl domdocObjectSite =
{
    domdoc_ObjectWithSite_QueryInterface,
    domdoc_ObjectWithSite_AddRef,
    domdoc_ObjectWithSite_Release,
    domdoc_ObjectWithSite_SetSite,
    domdoc_ObjectWithSite_GetSite
};

static HRESULT WINAPI domdoc_Safety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    domdoc *doc = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_QueryInterface(&doc->IXMLDOMDocument3_iface, riid, ppv);
}

static ULONG WINAPI domdoc_Safety_AddRef(IObjectSafety *iface)
{
    domdoc *doc = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_AddRef(&doc->IXMLDOMDocument3_iface);
}

static ULONG WINAPI domdoc_Safety_Release(IObjectSafety *iface)
{
    domdoc *doc = impl_from_IObjectSafety(iface);
    return IXMLDOMDocument3_Release(&doc->IXMLDOMDocument3_iface);
}

#define SAFETY_SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI domdoc_Safety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *supported, DWORD *enabled)
{
    domdoc *doc = impl_from_IObjectSafety(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(riid), supported, enabled);

    if (!supported || !enabled)
        return E_POINTER;

    *supported = SAFETY_SUPPORTED_OPTIONS;
    *enabled = doc->safeopt;

    return S_OK;
}

static HRESULT WINAPI domdoc_Safety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD mask, DWORD enabled)
{
    domdoc *doc = impl_from_IObjectSafety(iface);

    TRACE("%p, %s, %lx, %lx.\n", iface, debugstr_guid(riid), mask, enabled);

    if ((mask & ~SAFETY_SUPPORTED_OPTIONS) != 0)
        return E_FAIL;

    doc->safeopt = (doc->safeopt & ~mask) | (mask & enabled);

    return S_OK;
}

#undef SAFETY_SUPPORTED_OPTIONS

static const IObjectSafetyVtbl domdocObjectSafetyVtbl = {
    domdoc_Safety_QueryInterface,
    domdoc_Safety_AddRef,
    domdoc_Safety_Release,
    domdoc_Safety_GetInterfaceSafetyOptions,
    domdoc_Safety_SetInterfaceSafetyOptions
};

static const tid_t domdoc_iface_tids[] =
{
    IXMLDOMDocument3_tid,
    0
};

static dispex_static_data_t domdoc_dispex = {
    NULL,
    IXMLDOMDocument3_tid,
    NULL,
    domdoc_iface_tids
};

HRESULT dom_document_create(MSXML_VERSION version, void **obj)
{
    struct domnode *node;
    HRESULT hr;

    TRACE("%d, %p.\n", version, obj);

    *obj = NULL;

    if (FAILED(hr = domnode_create(NODE_DOCUMENT, NULL, 0, NULL, 0, NULL, &node)))
        return hr;
    node->properties = domdoc_create_properties(version);

    hr = create_domdoc(node, (IUnknown **)obj);
    if (FAILED(hr))
        domnode_destroy_tree(node);

    return hr;
}

HRESULT create_domdoc(struct domnode *node, IUnknown **obj)
{
    domdoc *object;

    *obj = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMDocument3_iface.lpVtbl = &XMLDOMDocument3Vtbl;
    object->IPersistStreamInit_iface.lpVtbl = &xmldoc_IPersistStreamInit_VTable;
    object->IObjectWithSite_iface.lpVtbl = &domdocObjectSite;
    object->IObjectSafety_iface.lpVtbl = &domdocObjectSafetyVtbl;
    object->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;
    object->refcount = 1;
    object->async = VARIANT_TRUE;
    object->node = domnode_addref(node);

    ConnectionPoint_Init(&object->cp_dispatch, object, &IID_IDispatch);
    ConnectionPoint_Init(&object->cp_propnotif, object, &IID_IPropertyNotifySink);
    ConnectionPoint_Init(&object->cp_domdocevents, object, &DIID_XMLDOMDocumentEvents);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMDocument3_iface, &domdoc_dispex);

    *obj = (IUnknown *)&object->IXMLDOMDocument3_iface;

    return S_OK;
}
