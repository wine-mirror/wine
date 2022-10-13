/*
 * Copyright 2005-2009 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wininet.h"
#include "ole2.h"
#include "perhist.h"
#include "mshtmdid.h"
#include "mshtmcid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "pluginhost.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT create_document_fragment(nsIDOMNode *nsnode, HTMLDocumentNode *doc_node, HTMLDocumentNode **ret);

HRESULT get_doc_elem_by_id(HTMLDocumentNode *doc, const WCHAR *id, HTMLElement **ret)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMElement *nselem;
    nsIDOMNode *nsnode;
    nsAString id_str;
    nsresult nsres;
    HRESULT hres;

    if(!doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&id_str, id);
    /* get element by id attribute */
    nsres = nsIDOMHTMLDocument_GetElementById(doc->nsdoc, &id_str, &nselem);
    if(FAILED(nsres)) {
        ERR("GetElementById failed: %08lx\n", nsres);
        nsAString_Finish(&id_str);
        return E_FAIL;
    }

    /* get first element by name attribute */
    nsres = nsIDOMHTMLDocument_GetElementsByName(doc->nsdoc, &id_str, &nsnode_list);
    nsAString_Finish(&id_str);
    if(FAILED(nsres)) {
        ERR("getElementsByName failed: %08lx\n", nsres);
        if(nselem)
            nsIDOMElement_Release(nselem);
        return E_FAIL;
    }

    nsres = nsIDOMNodeList_Item(nsnode_list, 0, &nsnode);
    nsIDOMNodeList_Release(nsnode_list);
    assert(nsres == NS_OK);

    if(nsnode && nselem) {
        UINT16 pos;

        nsres = nsIDOMNode_CompareDocumentPosition(nsnode, (nsIDOMNode*)nselem, &pos);
        if(NS_FAILED(nsres)) {
            FIXME("CompareDocumentPosition failed: 0x%08lx\n", nsres);
            nsIDOMNode_Release(nsnode);
            nsIDOMElement_Release(nselem);
            return E_FAIL;
        }

        TRACE("CompareDocumentPosition gave: 0x%x\n", pos);
        if(!(pos & (DOCUMENT_POSITION_PRECEDING | DOCUMENT_POSITION_CONTAINS))) {
            nsIDOMElement_Release(nselem);
            nselem = NULL;
        }
    }

    if(nsnode) {
        if(!nselem) {
            nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
            assert(nsres == NS_OK);
        }
        nsIDOMNode_Release(nsnode);
    }

    if(!nselem) {
        *ret = NULL;
        return S_OK;
    }

    hres = get_element(nselem, ret);
    nsIDOMElement_Release(nselem);
    return hres;
}

UINT get_document_charset(HTMLDocumentNode *doc)
{
    nsAString charset_str;
    UINT ret = 0;
    nsresult nsres;

    if(doc->charset)
        return doc->charset;

    nsAString_Init(&charset_str, NULL);
    nsres = nsIDOMHTMLDocument_GetCharacterSet(doc->nsdoc, &charset_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *charset;

        nsAString_GetData(&charset_str, &charset);

        if(*charset) {
            BSTR str = SysAllocString(charset);
            ret = cp_from_charset_string(str);
            SysFreeString(str);
        }
    }else {
        ERR("GetCharset failed: %08lx\n", nsres);
    }
    nsAString_Finish(&charset_str);

    if(!ret)
        return CP_UTF8;

    return doc->charset = ret;
}

typedef struct {
    HTMLDOMNode node;
    IDOMDocumentType IDOMDocumentType_iface;
} DocumentType;

static inline DocumentType *impl_from_IDOMDocumentType(IDOMDocumentType *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, IDOMDocumentType_iface);
}

static HRESULT WINAPI DocumentType_QueryInterface(IDOMDocumentType *iface, REFIID riid, void **ppv)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocumentType_AddRef(IDOMDocumentType *iface)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocumentType_Release(IDOMDocumentType *iface)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocumentType_GetTypeInfoCount(IDOMDocumentType *iface, UINT *pctinfo)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IDispatchEx_GetTypeInfoCount(&This->node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DocumentType_GetTypeInfo(IDOMDocumentType *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IDispatchEx_GetTypeInfo(&This->node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocumentType_GetIDsOfNames(IDOMDocumentType *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IDispatchEx_GetIDsOfNames(&This->node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
                                     cNames, lcid, rgDispId);
}

static HRESULT WINAPI DocumentType_Invoke(IDOMDocumentType *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    return IDispatchEx_Invoke(&This->node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
                              wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DocumentType_get_name(IDOMDocumentType *iface, BSTR *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMDocumentType_GetName((nsIDOMDocumentType*)This->node.nsnode, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI DocumentType_get_entities(IDOMDocumentType *iface, IDispatch **p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_notations(IDOMDocumentType *iface, IDispatch **p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_publicId(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_systemId(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_internalSubset(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IDOMDocumentTypeVtbl DocumentTypeVtbl = {
    DocumentType_QueryInterface,
    DocumentType_AddRef,
    DocumentType_Release,
    DocumentType_GetTypeInfoCount,
    DocumentType_GetTypeInfo,
    DocumentType_GetIDsOfNames,
    DocumentType_Invoke,
    DocumentType_get_name,
    DocumentType_get_entities,
    DocumentType_get_notations,
    DocumentType_get_publicId,
    DocumentType_get_systemId,
    DocumentType_get_internalSubset
};

static inline DocumentType *DocumentType_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, node);
}

static inline DocumentType *DocumentType_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, node.event_target.dispex);
}

static HRESULT DocumentType_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    DocumentType *This = DocumentType_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IDOMDocumentType, riid))
        *ppv = &This->IDOMDocumentType_iface;
    else
        return HTMLDOMNode_QI(&This->node, riid, ppv);

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void DocumentType_destructor(HTMLDOMNode *iface)
{
    DocumentType *This = DocumentType_from_HTMLDOMNode(iface);

    HTMLDOMNode_destructor(&This->node);
}

static HRESULT DocumentType_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    DocumentType *This = DocumentType_from_HTMLDOMNode(iface);

    return create_doctype_node(This->node.doc, nsnode, ret);
}

static const cpc_entry_t DocumentType_cpc[] = {{NULL}};

static const NodeImplVtbl DocumentTypeImplVtbl = {
    NULL,
    DocumentType_QI,
    DocumentType_destructor,
    DocumentType_cpc,
    DocumentType_clone
};

static nsISupports *DocumentType_get_gecko_target(DispatchEx *dispex)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    return (nsISupports*)This->node.nsnode;
}

static EventTarget *DocumentType_get_parent_event_target(DispatchEx *dispex)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &nsnode);
    assert(nsres == NS_OK);
    if(!nsnode)
        return NULL;

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return NULL;

    return &node->event_target;
}

static IHTMLEventObj *DocumentType_set_current_event(DispatchEx *dispex, IHTMLEventObj *event)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    return default_set_current_event(This->node.doc->window, event);
}

static event_target_vtbl_t DocumentType_event_target_vtbl = {
    {
        NULL,
    },
    DocumentType_get_gecko_target,
    NULL,
    DocumentType_get_parent_event_target,
    NULL,
    NULL,
    DocumentType_set_current_event
};

static const tid_t DocumentType_iface_tids[] = {
    IDOMDocumentType_tid,
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLDOMNode3_tid,
    0
};

static dispex_static_data_t DocumentType_dispex = {
    L"DocumentType",
    &DocumentType_event_target_vtbl.dispex_vtbl,
    DispDOMDocumentType_tid,
    DocumentType_iface_tids
};

HRESULT create_doctype_node(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    nsIDOMDocumentType *nsdoctype;
    DocumentType *doctype;
    nsresult nsres;

    if(!(doctype = heap_alloc_zero(sizeof(*doctype))))
        return E_OUTOFMEMORY;

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMDocumentType, (void**)&nsdoctype);
    assert(nsres == NS_OK);

    doctype->node.vtbl = &DocumentTypeImplVtbl;
    doctype->IDOMDocumentType_iface.lpVtbl = &DocumentTypeVtbl;
    HTMLDOMNode_Init(doc, &doctype->node, (nsIDOMNode*)nsdoctype, &DocumentType_dispex);
    nsIDOMDocumentType_Release(nsdoctype);

    *ret = &doctype->node;
    return S_OK;
}

static inline HTMLDocument *impl_from_IHTMLDocument2(IHTMLDocument2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument2_iface);
}

static HRESULT WINAPI HTMLDocument_QueryInterface(IHTMLDocument2 *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument_AddRef(IHTMLDocument2 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument_Release(IHTMLDocument2 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument_GetTypeInfoCount(IHTMLDocument2 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument_GetTypeInfo(IHTMLDocument2 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument_GetIDsOfNames(IHTMLDocument2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument_Invoke(IHTMLDocument2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_parentWindow(&This->IHTMLDocument7_iface, (IHTMLWindow2**)p);
    return hres == S_OK && !*p ? E_PENDING : hres;
}

static HRESULT WINAPI HTMLDocument_get_all(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nselem = NULL;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetDocumentElement(This->doc_node->nsdoc, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetDocumentElement failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = create_all_collection(node, TRUE);
    node_release(node);
    return hres;
}

static HRESULT WINAPI HTMLDocument_get_body(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLElement *nsbody = NULL;
    HTMLElement *element;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->doc_node->nsdoc) {
        nsresult nsres;

        nsres = nsIDOMHTMLDocument_GetBody(This->doc_node->nsdoc, &nsbody);
        if(NS_FAILED(nsres)) {
            TRACE("Could not get body: %08lx\n", nsres);
            return E_UNEXPECTED;
        }
    }

    if(!nsbody) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element((nsIDOMElement*)nsbody, &element);
    nsIDOMHTMLElement_Release(nsbody);
    if(FAILED(hres))
        return hres;

    *p = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_activeElement(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        *p = NULL;
        return S_OK;
    }

    /*
     * NOTE: Gecko may return an active element even if the document is not visible.
     * IE returns NULL in this case.
     */
    nsres = nsIDOMHTMLDocument_GetActiveElement(This->doc_node->nsdoc, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetActiveElement failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_images(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetImages(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetImages failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_applets(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetApplets(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetApplets failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_links(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetLinks(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetLinks failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_forms(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetForms(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetForms failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_anchors(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetAnchors(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetAnchors failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_title(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLDocument_SetTitle(This->doc_node->nsdoc, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        ERR("SetTitle failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_title(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    const PRUnichar *ret;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }


    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetTitle(This->doc_node->nsdoc, &nsstr);
    if (NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&nsstr, &ret);
        *p = SysAllocString(ret);
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("GetTitle failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_scripts(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetScripts(This->doc_node->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetImages failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, This->doc_node->document_mode);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(wcsicmp(v, L"on")) {
        FIXME("Unsupported arg %s\n", debugstr_w(v));
        return E_NOTIMPL;
    }

    hres = setup_edit_mode(This->doc_obj);
    if(FAILED(hres))
        return hres;

    call_property_onchanged(&This->cp_container, DISPID_IHTMLDOCUMENT2_DESIGNMODE);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p) always returning Off\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = SysAllocString(L"Off");

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_selection(IHTMLDocument2 *iface, IHTMLSelectionObject **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsISelection *nsselection;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLDocument_GetSelection(This->doc_node->nsdoc, &nsselection);
    if(NS_FAILED(nsres)) {
        ERR("GetSelection failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return HTMLSelectionObject_Create(This->doc_node, nsselection, p);
}

static HRESULT WINAPI HTMLDocument_get_readyState(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);


    TRACE("(%p)->(%p)\n", iface, p);

    if(!p)
        return E_POINTER;

    return get_readystate_string(This->window ? This->window->readystate : 0, p);
}

static HRESULT WINAPI HTMLDocument_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }
    return IHTMLWindow2_get_frames(&This->window->base.IHTMLWindow2_iface, p);
}

static HRESULT WINAPI HTMLDocument_get_embeds(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_plugins(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_alinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_alinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_bgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    IHTMLElement *element = NULL;
    IHTMLBodyElement *body;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hr = IHTMLDocument2_get_body(iface, &element);
    if (FAILED(hr))
    {
        ERR("Failed to get body (0x%08lx)\n", hr);
        return hr;
    }

    if(!element)
    {
        FIXME("Empty body element.\n");
        return hr;
    }

    hr = IHTMLElement_QueryInterface(element, &IID_IHTMLBodyElement, (void**)&body);
    if (SUCCEEDED(hr))
    {
        hr = IHTMLBodyElement_put_bgColor(body, v);
        IHTMLBodyElement_Release(body);
    }
    IHTMLElement_Release(element);

    return hr;
}

static HRESULT WINAPI HTMLDocument_get_bgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetBgColor(This->doc_node->nsdoc, &nsstr);
    hres = return_nsstr_variant(nsres, &nsstr, NSSTR_COLOR, p);
    if(hres == S_OK && V_VT(p) == VT_BSTR && !V_BSTR(p)) {
        TRACE("default #ffffff\n");
        if(!(V_BSTR(p) = SysAllocString(L"#ffffff")))
            hres = E_OUTOFMEMORY;
    }
    return hres;
}

static HRESULT WINAPI HTMLDocument_put_fgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_linkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_linkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_vlinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_vlinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_referrer(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
 }

static HRESULT WINAPI HTMLDocument_get_location(IHTMLDocument2 *iface, IHTMLLocation **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface)->doc_node;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsdoc || !This->window) {
        WARN("NULL window\n");
        return E_UNEXPECTED;
    }

    return IHTMLWindow2_get_location(&This->window->base.IHTMLWindow2_iface, p);
}

static HRESULT IHTMLDocument2_location_hook(HTMLDocument *doc, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    if(!(flags & DISPATCH_PROPERTYPUT) || !doc->window)
        return S_FALSE;

    return IDispatchEx_InvokeEx(&doc->window->base.IDispatchEx_iface, DISPID_IHTMLWINDOW2_LOCATION,
                                0, flags, dp, res, ei, caller);
}

static HRESULT WINAPI HTMLDocument_get_lastModified(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_URL(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->window) {
        FIXME("No window available\n");
        return E_FAIL;
    }

    return navigate_url(This->window, v, This->window->uri, BINDING_NAVIGATED);
}

static HRESULT WINAPI HTMLDocument_get_URL(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", iface, p);

    *p = SysAllocString(This->window && This->window->url ? This->window->url : L"about:blank");
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLDocument_put_domain(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLDocument_SetDomain(This->doc_node->nsdoc, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetDomain failed: %08lx\n", nsres);
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_domain(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetDomain(This->doc_node->nsdoc, &nsstr);
    if(NS_SUCCEEDED(nsres) && This->window && This->window->uri) {
        const PRUnichar *str;
        HRESULT hres;

        nsAString_GetData(&nsstr, &str);
        if(!*str) {
            TRACE("Gecko returned empty string, fallback to loaded URL.\n");
            nsAString_Finish(&nsstr);
            hres = IUri_GetHost(This->window->uri, p);
            return FAILED(hres) ? hres : S_OK;
        }
    }

    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLDocument_put_cookie(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    BOOL bret;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->window)
        return S_OK;

    bret = InternetSetCookieExW(This->window->url, NULL, v, 0, 0);
    if(!bret) {
        FIXME("InternetSetCookieExW failed: %lu\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_cookie(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    DWORD size;
    BOOL bret;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window) {
        *p = NULL;
        return S_OK;
    }

    size = 0;
    bret = InternetGetCookieExW(This->window->url, NULL, NULL, &size, 0, NULL);
    if(!bret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        WARN("InternetGetCookieExW failed: %lu\n", GetLastError());
        *p = NULL;
        return S_OK;
    }

    if(!size) {
        *p = NULL;
        return S_OK;
    }

    *p = SysAllocStringLen(NULL, size/sizeof(WCHAR)-1);
    if(!*p)
        return E_OUTOFMEMORY;

    bret = InternetGetCookieExW(This->window->url, NULL, *p, &size, 0, NULL);
    if(!bret) {
        ERR("InternetGetCookieExW failed: %lu\n", GetLastError());
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_expando(IHTMLDocument2 *iface, VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_expando(IHTMLDocument2 *iface, VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_charset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s) returning S_OK\n", This, debugstr_w(v));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_charset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument7_get_characterSet(&This->IHTMLDocument7_iface, p);
}

static HRESULT WINAPI HTMLDocument_put_defaultCharset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_defaultCharset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = charset_string_from_cp(GetACP());
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLDocument_get_mimeType(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileSize(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileCreatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileModifiedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileUpdatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_security(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_protocol(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_nameProp(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT document_write(HTMLDocument *This, SAFEARRAY *psarray, BOOL ln)
{
    VARIANT *var, tmp;
    JSContext *jsctx;
    nsAString nsstr;
    ULONG i, argc;
    nsresult nsres;
    HRESULT hres;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    if (!psarray)
        return S_OK;

    if(psarray->cDims != 1) {
        FIXME("cDims=%d\n", psarray->cDims);
        return E_INVALIDARG;
    }

    hres = SafeArrayAccessData(psarray, (void**)&var);
    if(FAILED(hres)) {
        WARN("SafeArrayAccessData failed: %08lx\n", hres);
        return hres;
    }

    V_VT(&tmp) = VT_EMPTY;

    jsctx = get_context_from_document(This->doc_node->nsdoc);
    argc = psarray->rgsabound[0].cElements;
    for(i=0; i < argc; i++) {
        if(V_VT(var+i) == VT_BSTR) {
            nsAString_InitDepend(&nsstr, V_BSTR(var+i));
        }else {
            hres = VariantChangeTypeEx(&tmp, var+i, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, VT_BSTR);
            if(FAILED(hres)) {
                WARN("Could not convert %s to string\n", debugstr_variant(var+i));
                break;
            }
            nsAString_InitDepend(&nsstr, V_BSTR(&tmp));
        }

        if(!ln || i != argc-1)
            nsres = nsIDOMHTMLDocument_Write(This->doc_node->nsdoc, &nsstr, jsctx);
        else
            nsres = nsIDOMHTMLDocument_Writeln(This->doc_node->nsdoc, &nsstr, jsctx);
        nsAString_Finish(&nsstr);
        if(V_VT(var+i) != VT_BSTR)
            VariantClear(&tmp);
        if(NS_FAILED(nsres)) {
            ERR("Write failed: %08lx\n", nsres);
            hres = E_FAIL;
            break;
        }
    }

    SafeArrayUnaccessData(psarray);

    return hres;
}

static HRESULT WINAPI HTMLDocument_write(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", iface, psarray);

    return document_write(This, psarray, FALSE);
}

static HRESULT WINAPI HTMLDocument_writeln(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, psarray);

    return document_write(This, psarray, TRUE);
}

static HRESULT WINAPI HTMLDocument_open(IHTMLDocument2 *iface, BSTR url, VARIANT name,
                        VARIANT features, VARIANT replace, IDispatch **pomWindowResult)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsISupports *tmp;
    nsresult nsres;

    TRACE("(%p)->(%s %s %s %s %p)\n", This, debugstr_w(url), debugstr_variant(&name),
          debugstr_variant(&features), debugstr_variant(&replace), pomWindowResult);

    *pomWindowResult = NULL;

    if(!This->window)
        return E_FAIL;

    if(!This->doc_node->nsdoc) {
        ERR("!nsdoc\n");
        return E_NOTIMPL;
    }

    if(!url || wcscmp(url, L"text/html") || V_VT(&name) != VT_ERROR
       || V_VT(&features) != VT_ERROR || V_VT(&replace) != VT_ERROR)
        FIXME("unsupported args\n");

    nsres = nsIDOMHTMLDocument_Open(This->doc_node->nsdoc, NULL, NULL, NULL,
            get_context_from_document(This->doc_node->nsdoc), 0, &tmp);
    if(NS_FAILED(nsres)) {
        ERR("Open failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(tmp)
        nsISupports_Release(tmp);

    *pomWindowResult = (IDispatch*)&This->window->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(&This->window->base.IHTMLWindow2_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_close(IHTMLDocument2 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    if(!This->doc_node->nsdoc) {
        ERR("!nsdoc\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_Close(This->doc_node->nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Close failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_clear(IHTMLDocument2 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsIDOMHTMLDocument_Clear(This->doc_node->nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Clear failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const struct {
    const WCHAR *name;
    OLECMDID id;
}command_names[] = {
    {L"copy", IDM_COPY},
    {L"cut", IDM_CUT},
    {L"fontname", IDM_FONTNAME},
    {L"fontsize", IDM_FONTSIZE},
    {L"indent", IDM_INDENT},
    {L"insertorderedlist", IDM_ORDERLIST},
    {L"insertunorderedlist", IDM_UNORDERLIST},
    {L"outdent", IDM_OUTDENT},
    {L"paste", IDM_PASTE},
    {L"respectvisibilityindesign", IDM_RESPECTVISIBILITY_INDESIGN}
};

static BOOL cmdid_from_string(const WCHAR *str, OLECMDID *cmdid)
{
    int i;

    for(i = 0; i < ARRAY_SIZE(command_names); i++) {
        if(!wcsicmp(command_names[i].name, str)) {
            *cmdid = command_names[i].id;
            return TRUE;
        }
    }

    FIXME("Unknown command %s\n", debugstr_w(str));
    return FALSE;
}

static HRESULT WINAPI HTMLDocument_queryCommandSupported(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandEnabled(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandState(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandIndeterm(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandText(IHTMLDocument2 *iface, BSTR cmdID,
                                                        BSTR *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandValue(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_execCommand(IHTMLDocument2 *iface, BSTR cmdID,
                                VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    OLECMDID cmdid;
    VARIANT ret;
    HRESULT hres;

    TRACE("(%p)->(%s %x %s %p)\n", This, debugstr_w(cmdID), showUI, debugstr_variant(&value), pfRet);

    if(!cmdid_from_string(cmdID, &cmdid))
        return OLECMDERR_E_NOTSUPPORTED;

    V_VT(&ret) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(&This->IOleCommandTarget_iface, &CGID_MSHTML, cmdid,
            showUI ? 0 : OLECMDEXECOPT_DONTPROMPTUSER, &value, &ret);
    if(FAILED(hres))
        return hres;

    if(V_VT(&ret) != VT_EMPTY) {
        FIXME("Handle ret %s\n", debugstr_variant(&ret));
        VariantClear(&ret);
    }

    *pfRet = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_execCommandShowHelp(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_createElement(IHTMLDocument2 *iface, BSTR eTag,
                                                 IHTMLElement **newElem)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(eTag), newElem);

    hres = create_element(This->doc_node, eTag, &elem);
    if(FAILED(hres))
        return hres;

    *newElem = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_onhelp(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onhelp(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_CLICK, &v);
}

static HRESULT WINAPI HTMLDocument_get_onclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CLICK, p);
}

static HRESULT WINAPI HTMLDocument_put_ondblclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DBLCLICK, &v);
}

static HRESULT WINAPI HTMLDocument_get_ondblclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DBLCLICK, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeyup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYUP, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeyup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYUP, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeydown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYDOWN, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeydown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYDOWN, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeypress(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYPRESS, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeypress(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYPRESS, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEUP, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEUP, p);
}

static HRESULT WINAPI HTMLDocument_put_onmousedown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEDOWN, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmousedown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEDOWN, p);
}

static HRESULT WINAPI HTMLDocument_put_onmousemove(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEMOVE, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmousemove(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEMOVE, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseout(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEOUT, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseout(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEOUT, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseover(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEOVER, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseover(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEOVER, p);
}

static HRESULT WINAPI HTMLDocument_put_onreadystatechange(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_READYSTATECHANGE, &v);
}

static HRESULT WINAPI HTMLDocument_get_onreadystatechange(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_READYSTATECHANGE, p);
}

static HRESULT WINAPI HTMLDocument_put_onafterupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onafterupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowexit(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowexit(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowenter(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowenter(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_ondragstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DRAGSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_ondragstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DRAGSTART, p);
}

static HRESULT WINAPI HTMLDocument_put_onselectstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SELECTSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_onselectstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SELECTSTART, p);
}

static HRESULT WINAPI HTMLDocument_elementFromPoint(IHTMLDocument2 *iface, LONG x, LONG y,
                                                    IHTMLElement **elementHit)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nselem;
    HTMLElement *element;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%ld %ld %p)\n", This, x, y, elementHit);

    nsres = nsIDOMHTMLDocument_ElementFromPoint(This->doc_node->nsdoc, x, y, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("ElementFromPoint failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *elementHit = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &element);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *elementHit = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
    return hres == S_OK && !*p ? E_FAIL : hres;
}

static HRESULT WINAPI HTMLDocument_get_styleSheets(IHTMLDocument2 *iface,
                                                   IHTMLStyleSheetsCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMStyleSheetList *nsstylelist;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetStyleSheets(This->doc_node->nsdoc, &nsstylelist);
    if(NS_FAILED(nsres)) {
        ERR("GetStyleSheets failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    hres = create_style_sheet_collection(nsstylelist,
                                         dispex_compat_mode(&This->doc_node->node.event_target.dispex), p);
    nsIDOMStyleSheetList_Release(nsstylelist);
    return hres;
}

static HRESULT WINAPI HTMLDocument_put_onbeforeupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onbeforeupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onerrorupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onerrorupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_toString(IHTMLDocument2 *iface, BSTR *String)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, String);

    return dispex_to_string(&This->doc_node->node.event_target.dispex, String);
}

static HRESULT WINAPI HTMLDocument_createStyleSheet(IHTMLDocument2 *iface, BSTR bstrHref,
                                            LONG lIndex, IHTMLStyleSheet **ppnewStyleSheet)
{
    HTMLDocument *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLHeadElement *head_elem;
    IHTMLStyleElement *style_elem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(bstrHref), lIndex, ppnewStyleSheet);

    if(!This->doc_node->nsdoc) {
        FIXME("not a real doc object\n");
        return E_NOTIMPL;
    }

    if(lIndex != -1)
        FIXME("Unsupported lIndex %ld\n", lIndex);

    if(bstrHref && *bstrHref) {
        FIXME("semi-stub for href %s\n", debugstr_w(bstrHref));
        return create_style_sheet(NULL, dispex_compat_mode(&This->doc_node->node.event_target.dispex),
                                  ppnewStyleSheet);
    }

    hres = create_element(This->doc_node, L"style", &elem);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLDocument_GetHead(This->doc_node->nsdoc, &head_elem);
    if(NS_SUCCEEDED(nsres)) {
        nsIDOMNode *head_node, *tmp_node;

        nsres = nsIDOMHTMLHeadElement_QueryInterface(head_elem, &IID_nsIDOMNode, (void**)&head_node);
        nsIDOMHTMLHeadElement_Release(head_elem);
        assert(nsres == NS_OK);

        nsres = nsIDOMNode_AppendChild(head_node, elem->node.nsnode, &tmp_node);
        nsIDOMNode_Release(head_node);
        if(NS_SUCCEEDED(nsres) && tmp_node)
            nsIDOMNode_Release(tmp_node);
    }
    if(NS_FAILED(nsres)) {
        IHTMLElement_Release(&elem->IHTMLElement_iface);
        return E_FAIL;
    }

    hres = IHTMLElement_QueryInterface(&elem->IHTMLElement_iface, &IID_IHTMLStyleElement, (void**)&style_elem);
    assert(hres == S_OK);
    IHTMLElement_Release(&elem->IHTMLElement_iface);

    hres = IHTMLStyleElement_get_styleSheet(style_elem, ppnewStyleSheet);
    IHTMLStyleElement_Release(style_elem);
    return hres;
}

static const IHTMLDocument2Vtbl HTMLDocumentVtbl = {
    HTMLDocument_QueryInterface,
    HTMLDocument_AddRef,
    HTMLDocument_Release,
    HTMLDocument_GetTypeInfoCount,
    HTMLDocument_GetTypeInfo,
    HTMLDocument_GetIDsOfNames,
    HTMLDocument_Invoke,
    HTMLDocument_get_Script,
    HTMLDocument_get_all,
    HTMLDocument_get_body,
    HTMLDocument_get_activeElement,
    HTMLDocument_get_images,
    HTMLDocument_get_applets,
    HTMLDocument_get_links,
    HTMLDocument_get_forms,
    HTMLDocument_get_anchors,
    HTMLDocument_put_title,
    HTMLDocument_get_title,
    HTMLDocument_get_scripts,
    HTMLDocument_put_designMode,
    HTMLDocument_get_designMode,
    HTMLDocument_get_selection,
    HTMLDocument_get_readyState,
    HTMLDocument_get_frames,
    HTMLDocument_get_embeds,
    HTMLDocument_get_plugins,
    HTMLDocument_put_alinkColor,
    HTMLDocument_get_alinkColor,
    HTMLDocument_put_bgColor,
    HTMLDocument_get_bgColor,
    HTMLDocument_put_fgColor,
    HTMLDocument_get_fgColor,
    HTMLDocument_put_linkColor,
    HTMLDocument_get_linkColor,
    HTMLDocument_put_vlinkColor,
    HTMLDocument_get_vlinkColor,
    HTMLDocument_get_referrer,
    HTMLDocument_get_location,
    HTMLDocument_get_lastModified,
    HTMLDocument_put_URL,
    HTMLDocument_get_URL,
    HTMLDocument_put_domain,
    HTMLDocument_get_domain,
    HTMLDocument_put_cookie,
    HTMLDocument_get_cookie,
    HTMLDocument_put_expando,
    HTMLDocument_get_expando,
    HTMLDocument_put_charset,
    HTMLDocument_get_charset,
    HTMLDocument_put_defaultCharset,
    HTMLDocument_get_defaultCharset,
    HTMLDocument_get_mimeType,
    HTMLDocument_get_fileSize,
    HTMLDocument_get_fileCreatedDate,
    HTMLDocument_get_fileModifiedDate,
    HTMLDocument_get_fileUpdatedDate,
    HTMLDocument_get_security,
    HTMLDocument_get_protocol,
    HTMLDocument_get_nameProp,
    HTMLDocument_write,
    HTMLDocument_writeln,
    HTMLDocument_open,
    HTMLDocument_close,
    HTMLDocument_clear,
    HTMLDocument_queryCommandSupported,
    HTMLDocument_queryCommandEnabled,
    HTMLDocument_queryCommandState,
    HTMLDocument_queryCommandIndeterm,
    HTMLDocument_queryCommandText,
    HTMLDocument_queryCommandValue,
    HTMLDocument_execCommand,
    HTMLDocument_execCommandShowHelp,
    HTMLDocument_createElement,
    HTMLDocument_put_onhelp,
    HTMLDocument_get_onhelp,
    HTMLDocument_put_onclick,
    HTMLDocument_get_onclick,
    HTMLDocument_put_ondblclick,
    HTMLDocument_get_ondblclick,
    HTMLDocument_put_onkeyup,
    HTMLDocument_get_onkeyup,
    HTMLDocument_put_onkeydown,
    HTMLDocument_get_onkeydown,
    HTMLDocument_put_onkeypress,
    HTMLDocument_get_onkeypress,
    HTMLDocument_put_onmouseup,
    HTMLDocument_get_onmouseup,
    HTMLDocument_put_onmousedown,
    HTMLDocument_get_onmousedown,
    HTMLDocument_put_onmousemove,
    HTMLDocument_get_onmousemove,
    HTMLDocument_put_onmouseout,
    HTMLDocument_get_onmouseout,
    HTMLDocument_put_onmouseover,
    HTMLDocument_get_onmouseover,
    HTMLDocument_put_onreadystatechange,
    HTMLDocument_get_onreadystatechange,
    HTMLDocument_put_onafterupdate,
    HTMLDocument_get_onafterupdate,
    HTMLDocument_put_onrowexit,
    HTMLDocument_get_onrowexit,
    HTMLDocument_put_onrowenter,
    HTMLDocument_get_onrowenter,
    HTMLDocument_put_ondragstart,
    HTMLDocument_get_ondragstart,
    HTMLDocument_put_onselectstart,
    HTMLDocument_get_onselectstart,
    HTMLDocument_elementFromPoint,
    HTMLDocument_get_parentWindow,
    HTMLDocument_get_styleSheets,
    HTMLDocument_put_onbeforeupdate,
    HTMLDocument_get_onbeforeupdate,
    HTMLDocument_put_onerrorupdate,
    HTMLDocument_get_onerrorupdate,
    HTMLDocument_toString,
    HTMLDocument_createStyleSheet
};

static inline HTMLDocument *impl_from_IHTMLDocument3(IHTMLDocument3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument3_iface);
}

static HRESULT WINAPI HTMLDocument3_QueryInterface(IHTMLDocument3 *iface,
                                                  REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument3_AddRef(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument3_Release(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfoCount(IHTMLDocument3 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfo(IHTMLDocument3 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument3_GetIDsOfNames(IHTMLDocument3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument3_Invoke(IHTMLDocument3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument3_releaseCapture(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_recalc(IHTMLDocument3 *iface, VARIANT_BOOL fForce)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    WARN("(%p)->(%x)\n", This, fForce);

    /* Doing nothing here should be fine for us. */
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_createTextNode(IHTMLDocument3 *iface, BSTR text,
                                                   IHTMLDOMNode **newTextNode)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMText *nstext;
    HTMLDOMNode *node;
    nsAString text_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(text), newTextNode);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&text_str, text);
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->doc_node->nsdoc, &text_str, &nstext);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = HTMLDOMTextNode_Create(This->doc_node, (nsIDOMNode*)nstext, &node);
    nsIDOMText_Release(nstext);
    if(FAILED(hres))
        return hres;

    *newTextNode = &node->IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_documentElement(IHTMLDocument3 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMElement *nselem = NULL;
    HTMLElement *element;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window && This->window->readystate == READYSTATE_UNINITIALIZED) {
        *p = NULL;
        return S_OK;
    }

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetDocumentElement(This->doc_node->nsdoc, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetDocumentElement failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &element);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &element->IHTMLElement_iface;
    return hres;
}

static HRESULT WINAPI HTMLDocument3_get_uniqueID(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_unique_id(++This->doc_node->unique_id, p);
}

static HRESULT WINAPI HTMLDocument3_attachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch* pDisp, VARIANT_BOOL *pfResult)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    return attach_event(&This->doc_node->node.event_target, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLDocument3_detachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch *pDisp)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(event), pDisp);

    return detach_event(&This->doc_node->node.event_target, event, pDisp);
}

static HRESULT WINAPI HTMLDocument3_put_onrowsdelete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsdelete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onrowsinserted(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsinserted(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncellchange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_oncellchange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetchanged(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetchanged(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondataavailable(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondataavailable(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onpropertychange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onpropertychange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_dir(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsAString dir_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&dir_str, v);
    nsres = nsIDOMHTMLDocument_SetDir(This->doc_node->nsdoc, &dir_str);
    nsAString_Finish(&dir_str);
    if(NS_FAILED(nsres)) {
        ERR("SetDir failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_dir(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsAString dir_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&dir_str, NULL);
    nsres = nsIDOMHTMLDocument_GetDir(This->doc_node->nsdoc, &dir_str);
    return return_nsstr(nsres, &dir_str, p);
}

static HRESULT WINAPI HTMLDocument3_put_oncontextmenu(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->()\n", This);

    return set_doc_event(This, EVENTID_CONTEXTMENU, &v);
}

static HRESULT WINAPI HTMLDocument3_get_oncontextmenu(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CONTEXTMENU, p);
}

static HRESULT WINAPI HTMLDocument3_put_onstop(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onstop(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createDocumentFragment(IHTMLDocument3 *iface,
                                                           IHTMLDocument2 **ppNewDoc)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMDocumentFragment *doc_frag;
    HTMLDocumentNode *docnode;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, ppNewDoc);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_CreateDocumentFragment(This->doc_node->nsdoc, &doc_frag);
    if(NS_FAILED(nsres)) {
        ERR("CreateDocumentFragment failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = create_document_fragment((nsIDOMNode*)doc_frag, This->doc_node, &docnode);
    nsIDOMDocumentFragment_Release(doc_frag);
    if(FAILED(hres))
        return hres;

    *ppNewDoc = &docnode->basedoc.IHTMLDocument2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_parentDocument(IHTMLDocument3 *iface,
                                                       IHTMLDocument2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_baseUrl(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_baseUrl(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_childNodes(IHTMLDocument3 *iface, IDispatch **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMNode_get_childNodes(&This->doc_node->node.IHTMLDOMNode_iface, p);
}

static HRESULT WINAPI HTMLDocument3_put_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_getElementsByName(IHTMLDocument3 *iface, BSTR v,
                                                      IHTMLElementCollection **ppelColl)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMNodeList *node_list;
    nsAString selector_str;
    WCHAR *selector;
    nsresult nsres;
    static const WCHAR formatW[] = L"*[id=%s],*[name=%s]";

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), ppelColl);

    if(!This->doc_node || !This->doc_node->nsdoc) {
        /* We should probably return an empty collection. */
        FIXME("No nsdoc\n");
        return E_NOTIMPL;
    }

    selector = heap_alloc(2*SysStringLen(v)*sizeof(WCHAR) + sizeof(formatW));
    if(!selector)
        return E_OUTOFMEMORY;
    swprintf(selector, 2*SysStringLen(v) + ARRAY_SIZE(formatW), formatW, v, v);

    /*
     * NOTE: IE getElementsByName implementation differs from Gecko. It searches both name and id attributes.
     * That's why we use CSS selector instead. We should also use name only when it applies to given element
     * types and search should be case insensitive. Those are currently not supported properly.
     */
    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMHTMLDocument_QuerySelectorAll(This->doc_node->nsdoc, &selector_str, &node_list);
    nsAString_Finish(&selector_str);
    heap_free(selector);
    if(NS_FAILED(nsres)) {
        ERR("QuerySelectorAll failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *ppelColl = create_collection_from_nodelist(node_list, This->doc_node->document_mode);
    nsIDOMNodeList_Release(node_list);
    return S_OK;
}


static HRESULT WINAPI HTMLDocument3_getElementById(IHTMLDocument3 *iface, BSTR v,
                                                   IHTMLElement **pel)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    hres = get_doc_elem_by_id(This->doc_node, v, &elem);
    if(FAILED(hres) || !elem) {
        *pel = NULL;
        return hres;
    }

    *pel = &elem->IHTMLElement_iface;
    return S_OK;
}


static HRESULT WINAPI HTMLDocument3_getElementsByTagName(IHTMLDocument3 *iface, BSTR v,
                                                         IHTMLElementCollection **pelColl)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMNodeList *nslist;
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pelColl);

    if(This->doc_node->nsdoc) {
        nsAString_InitDepend(&id_str, v);
        nsres = nsIDOMHTMLDocument_GetElementsByTagName(This->doc_node->nsdoc, &id_str, &nslist);
        nsAString_Finish(&id_str);
        if(FAILED(nsres)) {
            ERR("GetElementByName failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }else {
        nsIDOMDocumentFragment *docfrag;
        nsAString nsstr;

        if(v) {
            const WCHAR *ptr;
            for(ptr=v; *ptr; ptr++) {
                if(!iswalnum(*ptr)) {
                    FIXME("Unsupported invalid tag %s\n", debugstr_w(v));
                    return E_NOTIMPL;
                }
            }
        }

        nsres = nsIDOMNode_QueryInterface(This->doc_node->node.nsnode, &IID_nsIDOMDocumentFragment, (void**)&docfrag);
        if(NS_FAILED(nsres)) {
            ERR("Could not get nsIDOMDocumentFragment iface: %08lx\n", nsres);
            return E_UNEXPECTED;
        }

        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMDocumentFragment_QuerySelectorAll(docfrag, &nsstr, &nslist);
        nsAString_Finish(&nsstr);
        nsIDOMDocumentFragment_Release(docfrag);
        if(NS_FAILED(nsres)) {
            ERR("QuerySelectorAll failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }


    *pelColl = create_collection_from_nodelist(nslist, This->doc_node->document_mode);
    nsIDOMNodeList_Release(nslist);

    return S_OK;
}

static const IHTMLDocument3Vtbl HTMLDocument3Vtbl = {
    HTMLDocument3_QueryInterface,
    HTMLDocument3_AddRef,
    HTMLDocument3_Release,
    HTMLDocument3_GetTypeInfoCount,
    HTMLDocument3_GetTypeInfo,
    HTMLDocument3_GetIDsOfNames,
    HTMLDocument3_Invoke,
    HTMLDocument3_releaseCapture,
    HTMLDocument3_recalc,
    HTMLDocument3_createTextNode,
    HTMLDocument3_get_documentElement,
    HTMLDocument3_get_uniqueID,
    HTMLDocument3_attachEvent,
    HTMLDocument3_detachEvent,
    HTMLDocument3_put_onrowsdelete,
    HTMLDocument3_get_onrowsdelete,
    HTMLDocument3_put_onrowsinserted,
    HTMLDocument3_get_onrowsinserted,
    HTMLDocument3_put_oncellchange,
    HTMLDocument3_get_oncellchange,
    HTMLDocument3_put_ondatasetchanged,
    HTMLDocument3_get_ondatasetchanged,
    HTMLDocument3_put_ondataavailable,
    HTMLDocument3_get_ondataavailable,
    HTMLDocument3_put_ondatasetcomplete,
    HTMLDocument3_get_ondatasetcomplete,
    HTMLDocument3_put_onpropertychange,
    HTMLDocument3_get_onpropertychange,
    HTMLDocument3_put_dir,
    HTMLDocument3_get_dir,
    HTMLDocument3_put_oncontextmenu,
    HTMLDocument3_get_oncontextmenu,
    HTMLDocument3_put_onstop,
    HTMLDocument3_get_onstop,
    HTMLDocument3_createDocumentFragment,
    HTMLDocument3_get_parentDocument,
    HTMLDocument3_put_enableDownload,
    HTMLDocument3_get_enableDownload,
    HTMLDocument3_put_baseUrl,
    HTMLDocument3_get_baseUrl,
    HTMLDocument3_get_childNodes,
    HTMLDocument3_put_inheritStyleSheets,
    HTMLDocument3_get_inheritStyleSheets,
    HTMLDocument3_put_onbeforeeditfocus,
    HTMLDocument3_get_onbeforeeditfocus,
    HTMLDocument3_getElementsByName,
    HTMLDocument3_getElementById,
    HTMLDocument3_getElementsByTagName
};

static inline HTMLDocument *impl_from_IHTMLDocument4(IHTMLDocument4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument4_iface);
}

static HRESULT WINAPI HTMLDocument4_QueryInterface(IHTMLDocument4 *iface,
                                                   REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument4_AddRef(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument4_Release(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfoCount(IHTMLDocument4 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfo(IHTMLDocument4 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument4_GetIDsOfNames(IHTMLDocument4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument4_Invoke(IHTMLDocument4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument4_focus(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    nsIDOMHTMLElement *nsbody;
    nsresult nsres;

    TRACE("(%p)->()\n", This);

    nsres = nsIDOMHTMLDocument_GetBody(This->doc_node->nsdoc, &nsbody);
    if(NS_FAILED(nsres) || !nsbody) {
        ERR("GetBody failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLElement_Focus(nsbody);
    nsIDOMHTMLElement_Release(nsbody);
    if(NS_FAILED(nsres)) {
        ERR("Focus failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_hasFocus(IHTMLDocument4 *iface, VARIANT_BOOL *pfFocus)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    cpp_bool has_focus;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, pfFocus);

    if(!This->doc_node->nsdoc) {
        FIXME("Unimplemented for fragments.\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_HasFocus(This->doc_node->nsdoc, &has_focus);
    assert(nsres == NS_OK);

    *pfFocus = variant_bool(has_focus);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_put_onselectionchange(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SELECTIONCHANGE, &v);
}

static HRESULT WINAPI HTMLDocument4_get_onselectionchange(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SELECTIONCHANGE, p);
}

static HRESULT WINAPI HTMLDocument4_get_namespaces(IHTMLDocument4 *iface, IDispatch **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->namespaces) {
        HRESULT hres;

        hres = create_namespace_collection(dispex_compat_mode(&This->doc_node->node.event_target.dispex),
                                           &This->doc_node->namespaces);
        if(FAILED(hres))
            return hres;
    }

    IHTMLNamespaceCollection_AddRef(This->doc_node->namespaces);
    *p = (IDispatch*)This->doc_node->namespaces;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_createDocumentFromUrl(IHTMLDocument4 *iface, BSTR bstrUrl,
        BSTR bstrOptions, IHTMLDocument2 **newDoc)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(bstrUrl), debugstr_w(bstrOptions), newDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_media(IHTMLDocument4 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_media(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createEventObject(IHTMLDocument4 *iface,
        VARIANT *pvarEventObject, IHTMLEventObj **ppEventObj)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(pvarEventObject), ppEventObj);

    if(pvarEventObject && V_VT(pvarEventObject) != VT_ERROR && V_VT(pvarEventObject) != VT_EMPTY) {
        FIXME("unsupported pvarEventObject %s\n", debugstr_variant(pvarEventObject));
        return E_NOTIMPL;
    }

    return create_event_obj(dispex_compat_mode(&This->doc_node->node.event_target.dispex), ppEventObj);
}

static HRESULT WINAPI HTMLDocument4_fireEvent(IHTMLDocument4 *iface, BSTR bstrEventName,
        VARIANT *pvarEventObject, VARIANT_BOOL *pfCanceled)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(bstrEventName), pvarEventObject, pfCanceled);

    return fire_event(&This->doc_node->node, bstrEventName, pvarEventObject, pfCanceled);
}

static HRESULT WINAPI HTMLDocument4_createRenderStyle(IHTMLDocument4 *iface, BSTR v,
        IHTMLRenderStyle **ppIHTMLRenderStyle)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppIHTMLRenderStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_oncontrolselect(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_oncontrolselect(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_URLEncoded(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLDocument4Vtbl HTMLDocument4Vtbl = {
    HTMLDocument4_QueryInterface,
    HTMLDocument4_AddRef,
    HTMLDocument4_Release,
    HTMLDocument4_GetTypeInfoCount,
    HTMLDocument4_GetTypeInfo,
    HTMLDocument4_GetIDsOfNames,
    HTMLDocument4_Invoke,
    HTMLDocument4_focus,
    HTMLDocument4_hasFocus,
    HTMLDocument4_put_onselectionchange,
    HTMLDocument4_get_onselectionchange,
    HTMLDocument4_get_namespaces,
    HTMLDocument4_createDocumentFromUrl,
    HTMLDocument4_put_media,
    HTMLDocument4_get_media,
    HTMLDocument4_createEventObject,
    HTMLDocument4_fireEvent,
    HTMLDocument4_createRenderStyle,
    HTMLDocument4_put_oncontrolselect,
    HTMLDocument4_get_oncontrolselect,
    HTMLDocument4_get_URLEncoded
};

static inline HTMLDocument *impl_from_IHTMLDocument5(IHTMLDocument5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument5_iface);
}

static HRESULT WINAPI HTMLDocument5_QueryInterface(IHTMLDocument5 *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument5_AddRef(IHTMLDocument5 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument5_Release(IHTMLDocument5 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfoCount(IHTMLDocument5 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfo(IHTMLDocument5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument5_GetIDsOfNames(IHTMLDocument5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument5_Invoke(IHTMLDocument5 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument5_put_onmousewheel(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEWHEEL, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onmousewheel(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEWHEEL, p);
}

static HRESULT WINAPI HTMLDocument5_get_doctype(IHTMLDocument5 *iface, IHTMLDOMNode **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    HTMLDocumentNode *doc_node = This->doc_node;
    nsIDOMDocumentType *nsdoctype;
    HTMLDOMNode *doctype_node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(dispex_compat_mode(&doc_node->node.event_target.dispex) < COMPAT_MODE_IE9) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMHTMLDocument_GetDoctype(doc_node->nsdoc, &nsdoctype);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);
    if(!nsdoctype) {
        *p = NULL;
        return S_OK;
    }

    hres = get_node((nsIDOMNode*)nsdoctype, TRUE, &doctype_node);
    nsIDOMDocumentType_Release(nsdoctype);

    if(SUCCEEDED(hres))
        *p = &doctype_node->IHTMLDOMNode_iface;
    return hres;
}

static HRESULT WINAPI HTMLDocument5_get_implementation(IHTMLDocument5 *iface, IHTMLDOMImplementation **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    HTMLDocumentNode *doc_node = This->doc_node;

    TRACE("(%p)->(%p)\n", This, p);

    if(!doc_node->dom_implementation) {
        HRESULT hres;

        hres = create_dom_implementation(doc_node, &doc_node->dom_implementation);
        if(FAILED(hres))
            return hres;
    }

    IHTMLDOMImplementation_AddRef(doc_node->dom_implementation);
    *p = doc_node->dom_implementation;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_createAttribute(IHTMLDocument5 *iface, BSTR bstrattrName,
        IHTMLDOMAttribute **ppattribute)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    HTMLDOMAttribute *attr;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrattrName), ppattribute);

    hres = HTMLDOMAttribute_Create(bstrattrName, NULL, 0, dispex_compat_mode(&This->doc_node->node.event_target.dispex), &attr);
    if(FAILED(hres))
        return hres;

    *ppattribute = &attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_createComment(IHTMLDocument5 *iface, BSTR bstrdata,
        IHTMLDOMNode **ppRetNode)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    nsIDOMComment *nscomment;
    HTMLElement *elem;
    nsAString str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrdata), ppRetNode);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&str, bstrdata);
    nsres = nsIDOMHTMLDocument_CreateComment(This->doc_node->nsdoc, &str, &nscomment);
    nsAString_Finish(&str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = HTMLCommentElement_Create(This->doc_node, (nsIDOMNode*)nscomment, &elem);
    nsIDOMComment_Release(nscomment);
    if(FAILED(hres))
        return hres;

    *ppRetNode = &elem->node.IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusin(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUSIN, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onfocusin(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUSIN, p);
}

static HRESULT WINAPI HTMLDocument5_put_onfocusout(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUSOUT, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onfocusout(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUSOUT, p);
}

static HRESULT WINAPI HTMLDocument5_put_onactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_ondeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_ondeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_compatMode(IHTMLDocument5 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(This->doc_node->document_mode <= COMPAT_MODE_IE5 ? L"BackCompat" : L"CSS1Compat");
    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IHTMLDocument5Vtbl HTMLDocument5Vtbl = {
    HTMLDocument5_QueryInterface,
    HTMLDocument5_AddRef,
    HTMLDocument5_Release,
    HTMLDocument5_GetTypeInfoCount,
    HTMLDocument5_GetTypeInfo,
    HTMLDocument5_GetIDsOfNames,
    HTMLDocument5_Invoke,
    HTMLDocument5_put_onmousewheel,
    HTMLDocument5_get_onmousewheel,
    HTMLDocument5_get_doctype,
    HTMLDocument5_get_implementation,
    HTMLDocument5_createAttribute,
    HTMLDocument5_createComment,
    HTMLDocument5_put_onfocusin,
    HTMLDocument5_get_onfocusin,
    HTMLDocument5_put_onfocusout,
    HTMLDocument5_get_onfocusout,
    HTMLDocument5_put_onactivate,
    HTMLDocument5_get_onactivate,
    HTMLDocument5_put_ondeactivate,
    HTMLDocument5_get_ondeactivate,
    HTMLDocument5_put_onbeforeactivate,
    HTMLDocument5_get_onbeforeactivate,
    HTMLDocument5_put_onbeforedeactivate,
    HTMLDocument5_get_onbeforedeactivate,
    HTMLDocument5_get_compatMode
};

static inline HTMLDocument *impl_from_IHTMLDocument6(IHTMLDocument6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument6_iface);
}

static HRESULT WINAPI HTMLDocument6_QueryInterface(IHTMLDocument6 *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument6_AddRef(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument6_Release(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument6_GetTypeInfoCount(IHTMLDocument6 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument6_GetTypeInfo(IHTMLDocument6 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument6_GetIDsOfNames(IHTMLDocument6 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument6_Invoke(IHTMLDocument6 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument6_get_compatible(IHTMLDocument6 *iface,
        IHTMLDocumentCompatibleInfoCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_get_documentMode(IHTMLDocument6 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node) {
        FIXME("NULL doc_node\n");
        return E_UNEXPECTED;
    }

    V_VT(p) = VT_R4;
    V_R4(p) = compat_mode_info[This->doc_node->document_mode].document_mode;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument6_get_onstorage(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_STORAGE, p);
}

static HRESULT WINAPI HTMLDocument6_put_onstorage(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_STORAGE, &v);
}

static HRESULT WINAPI HTMLDocument6_get_onstoragecommit(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_STORAGECOMMIT, p);
}

static HRESULT WINAPI HTMLDocument6_put_onstoragecommit(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_STORAGECOMMIT, &v);
}

static HRESULT WINAPI HTMLDocument6_getElementById(IHTMLDocument6 *iface,
        BSTR bstrId, IHTMLElement2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrId), p);

    /*
     * Unlike IHTMLDocument3 implementation, this is standard compliant and does
     * not search for name attributes, so we may simply let Gecko do the right thing.
     */

    if(!This->doc_node->nsdoc) {
        FIXME("Not a document\n");
        return E_FAIL;
    }

    nsAString_InitDepend(&nsstr, bstrId);
    nsres = nsIDOMHTMLDocument_GetElementById(This->doc_node->nsdoc, &nsstr, &nselem);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("GetElementById failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument6_updateSettings(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IHTMLDocument6Vtbl HTMLDocument6Vtbl = {
    HTMLDocument6_QueryInterface,
    HTMLDocument6_AddRef,
    HTMLDocument6_Release,
    HTMLDocument6_GetTypeInfoCount,
    HTMLDocument6_GetTypeInfo,
    HTMLDocument6_GetIDsOfNames,
    HTMLDocument6_Invoke,
    HTMLDocument6_get_compatible,
    HTMLDocument6_get_documentMode,
    HTMLDocument6_put_onstorage,
    HTMLDocument6_get_onstorage,
    HTMLDocument6_put_onstoragecommit,
    HTMLDocument6_get_onstoragecommit,
    HTMLDocument6_getElementById,
    HTMLDocument6_updateSettings
};

static inline HTMLDocument *impl_from_IHTMLDocument7(IHTMLDocument7 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument7_iface);
}

static HRESULT WINAPI HTMLDocument7_QueryInterface(IHTMLDocument7 *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument7_AddRef(IHTMLDocument7 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument7_Release(IHTMLDocument7 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument7_GetTypeInfoCount(IHTMLDocument7 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument7_GetTypeInfo(IHTMLDocument7 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument7_GetIDsOfNames(IHTMLDocument7 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument7_Invoke(IHTMLDocument7 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument7_get_defaultView(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface)->doc_node;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window && This->window->base.outer_window) {
        *p = &This->window->base.outer_window->base.IHTMLWindow2_iface;
        IHTMLWindow2_AddRef(*p);
    }else {
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createCDATASection(IHTMLDocument7 *iface, BSTR text, IHTMLDOMNode **newCDATASectionNode)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, newCDATASectionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_getSelection(IHTMLDocument7 *iface, IHTMLSelection **ppIHTMLSelection)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, ppIHTMLSelection);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_getElementsByTagNameNS(IHTMLDocument7 *iface, VARIANT *pvarNS,
        BSTR bstrLocalName, IHTMLElementCollection **pelColl)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrLocalName), pelColl);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_createElementNS(IHTMLDocument7 *iface, VARIANT *pvarNS, BSTR bstrTag, IHTMLElement **newElem)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    nsIDOMElement *dom_element;
    HTMLElement *element;
    nsAString ns, tag;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrTag), newElem);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_FAIL;
    }

    if(pvarNS && V_VT(pvarNS) != VT_NULL && V_VT(pvarNS) != VT_BSTR)
        FIXME("Unsupported namespace %s\n", debugstr_variant(pvarNS));

    nsAString_InitDepend(&ns, pvarNS && V_VT(pvarNS) == VT_BSTR ? V_BSTR(pvarNS) : NULL);
    nsAString_InitDepend(&tag, bstrTag);
    nsres = nsIDOMHTMLDocument_CreateElementNS(This->doc_node->nsdoc, &ns, &tag, &dom_element);
    nsAString_Finish(&ns);
    nsAString_Finish(&tag);
    if(NS_FAILED(nsres)) {
        WARN("CreateElementNS failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    hres = HTMLElement_Create(This->doc_node, (nsIDOMNode*)dom_element, FALSE, &element);
    nsIDOMElement_Release(dom_element);
    if(FAILED(hres))
        return hres;

    *newElem = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createAttributeNS(IHTMLDocument7 *iface, VARIANT *pvarNS,
        BSTR bstrAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrAttrName), ppAttribute);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onmsthumbnailclick(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onmsthumbnailclick(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MSTHUMBNAILCLICK, p);
}

static HRESULT WINAPI HTMLDocument7_get_characterSet(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    nsAString charset_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_FAIL;
    }

    nsAString_Init(&charset_str, NULL);
    nsres = nsIDOMHTMLDocument_GetCharacterSet(This->doc_node->nsdoc, &charset_str);
    return return_nsstr(nsres, &charset_str, p);
}

static HRESULT WINAPI HTMLDocument7_createElement(IHTMLDocument7 *iface, BSTR bstrTag, IHTMLElement **newElem)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrTag), newElem);

    return IHTMLDocument2_createElement(&This->IHTMLDocument2_iface, bstrTag, newElem);
}

static HRESULT WINAPI HTMLDocument7_createAttribute(IHTMLDocument7 *iface, BSTR bstrAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrAttrName), ppAttribute);

    return IHTMLDocument5_createAttribute(&This->IHTMLDocument5_iface, bstrAttrName, ppAttribute);
}

static HRESULT WINAPI HTMLDocument7_getElementsByClassName(IHTMLDocument7 *iface, BSTR v, IHTMLElementCollection **pel)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    nsIDOMNodeList *nslist;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc not supported\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLDocument_GetElementsByClassName(This->doc_node->nsdoc, &nsstr, &nslist);
    nsAString_Finish(&nsstr);
    if(FAILED(nsres)) {
        ERR("GetElementByClassName failed: %08lx\n", nsres);
        return E_FAIL;
    }


    *pel = create_collection_from_nodelist(nslist, This->doc_node->document_mode);
    nsIDOMNodeList_Release(nslist);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createProcessingInstruction(IHTMLDocument7 *iface, BSTR target,
        BSTR data, IDOMProcessingInstruction **newProcessingInstruction)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(target), debugstr_w(data), newProcessingInstruction);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_adoptNode(IHTMLDocument7 *iface, IHTMLDOMNode *pNodeSource, IHTMLDOMNode3 **ppNodeDest)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p %p)\n", This, pNodeSource, ppNodeDest);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onmssitemodejumplistitemremoved(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onmssitemodejumplistitemremoved(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_all(IHTMLDocument7 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument2_get_all(&This->IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLDocument7_get_inputEncoding(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlEncoding(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_xmlStandalone(IHTMLDocument7 *iface, VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlStandalone(IHTMLDocument7 *iface, VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_xmlVersion(IHTMLDocument7 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlVersion(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_hasAttributes(IHTMLDocument7 *iface, VARIANT_BOOL *pfHasAttributes)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, pfHasAttributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onabort(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_ABORT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onabort(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_ABORT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onblur(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_BLUR, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onblur(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_BLUR, p);
}

static HRESULT WINAPI HTMLDocument7_put_oncanplay(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_oncanplay(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_oncanplaythrough(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_oncanplaythrough(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onchange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onchange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CHANGE, p);
}

static HRESULT WINAPI HTMLDocument7_put_ondrag(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DRAG, &v);
}

static HRESULT WINAPI HTMLDocument7_get_ondrag(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DRAG, p);
}

static HRESULT WINAPI HTMLDocument7_put_ondragend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragenter(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragenter(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragleave(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragleave(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragover(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragover(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondrop(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondrop(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondurationchange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondurationchange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onemptied(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onemptied(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onended(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onended(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onerror(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onerror(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLDocument7_put_onfocus(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUS, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onfocus(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUS, p);
}

static HRESULT WINAPI HTMLDocument7_put_oninput(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_INPUT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_oninput(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_INPUT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onload(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onload(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLDocument7_put_onloadeddata(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadeddata(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onloadedmetadata(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadedmetadata(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onloadstart(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadstart(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onpause(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onpause(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onplay(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onplay(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onplaying(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onplaying(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onprogress(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onprogress(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onratechange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onratechange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onreset(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onreset(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onscroll(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SCROLL, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onscroll(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SCROLL, p);
}

static HRESULT WINAPI HTMLDocument7_put_onseekend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onseekend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onseeking(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onseeking(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onselect(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onselect(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onstalled(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onstalled(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onsubmit(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SUBMIT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onsubmit(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SUBMIT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onsuspend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onsuspend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ontimeupdate(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ontimeupdate(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onvolumechange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onvolumechange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onwaiting(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onwaiting(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_normalize(IHTMLDocument7 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_importNode(IHTMLDocument7 *iface, IHTMLDOMNode *pNodeSource,
        VARIANT_BOOL fDeep, IHTMLDOMNode3 **ppNodeDest)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p %x %p)\n", This, pNodeSource, fDeep, ppNodeDest);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_parentWindow(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
}

static HRESULT WINAPI HTMLDocument7_put_body(IHTMLDocument7 *iface, IHTMLElement *v)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_body(IHTMLDocument7 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument2_get_body(&This->IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLDocument7_get_head(IHTMLDocument7 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument7(iface);
    nsIDOMHTMLHeadElement *nshead;
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    nsres = nsIDOMHTMLDocument_GetHead(This->doc_node->nsdoc, &nshead);
    assert(nsres == NS_OK);

    if(!nshead) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMHTMLHeadElement_QueryInterface(nshead, &IID_nsIDOMElement, (void**)&nselem);
    nsIDOMHTMLHeadElement_Release(nshead);
    assert(nsres == NS_OK);

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement_iface;
    return S_OK;
}

static const IHTMLDocument7Vtbl HTMLDocument7Vtbl = {
    HTMLDocument7_QueryInterface,
    HTMLDocument7_AddRef,
    HTMLDocument7_Release,
    HTMLDocument7_GetTypeInfoCount,
    HTMLDocument7_GetTypeInfo,
    HTMLDocument7_GetIDsOfNames,
    HTMLDocument7_Invoke,
    HTMLDocument7_get_defaultView,
    HTMLDocument7_createCDATASection,
    HTMLDocument7_getSelection,
    HTMLDocument7_getElementsByTagNameNS,
    HTMLDocument7_createElementNS,
    HTMLDocument7_createAttributeNS,
    HTMLDocument7_put_onmsthumbnailclick,
    HTMLDocument7_get_onmsthumbnailclick,
    HTMLDocument7_get_characterSet,
    HTMLDocument7_createElement,
    HTMLDocument7_createAttribute,
    HTMLDocument7_getElementsByClassName,
    HTMLDocument7_createProcessingInstruction,
    HTMLDocument7_adoptNode,
    HTMLDocument7_put_onmssitemodejumplistitemremoved,
    HTMLDocument7_get_onmssitemodejumplistitemremoved,
    HTMLDocument7_get_all,
    HTMLDocument7_get_inputEncoding,
    HTMLDocument7_get_xmlEncoding,
    HTMLDocument7_put_xmlStandalone,
    HTMLDocument7_get_xmlStandalone,
    HTMLDocument7_put_xmlVersion,
    HTMLDocument7_get_xmlVersion,
    HTMLDocument7_hasAttributes,
    HTMLDocument7_put_onabort,
    HTMLDocument7_get_onabort,
    HTMLDocument7_put_onblur,
    HTMLDocument7_get_onblur,
    HTMLDocument7_put_oncanplay,
    HTMLDocument7_get_oncanplay,
    HTMLDocument7_put_oncanplaythrough,
    HTMLDocument7_get_oncanplaythrough,
    HTMLDocument7_put_onchange,
    HTMLDocument7_get_onchange,
    HTMLDocument7_put_ondrag,
    HTMLDocument7_get_ondrag,
    HTMLDocument7_put_ondragend,
    HTMLDocument7_get_ondragend,
    HTMLDocument7_put_ondragenter,
    HTMLDocument7_get_ondragenter,
    HTMLDocument7_put_ondragleave,
    HTMLDocument7_get_ondragleave,
    HTMLDocument7_put_ondragover,
    HTMLDocument7_get_ondragover,
    HTMLDocument7_put_ondrop,
    HTMLDocument7_get_ondrop,
    HTMLDocument7_put_ondurationchange,
    HTMLDocument7_get_ondurationchange,
    HTMLDocument7_put_onemptied,
    HTMLDocument7_get_onemptied,
    HTMLDocument7_put_onended,
    HTMLDocument7_get_onended,
    HTMLDocument7_put_onerror,
    HTMLDocument7_get_onerror,
    HTMLDocument7_put_onfocus,
    HTMLDocument7_get_onfocus,
    HTMLDocument7_put_oninput,
    HTMLDocument7_get_oninput,
    HTMLDocument7_put_onload,
    HTMLDocument7_get_onload,
    HTMLDocument7_put_onloadeddata,
    HTMLDocument7_get_onloadeddata,
    HTMLDocument7_put_onloadedmetadata,
    HTMLDocument7_get_onloadedmetadata,
    HTMLDocument7_put_onloadstart,
    HTMLDocument7_get_onloadstart,
    HTMLDocument7_put_onpause,
    HTMLDocument7_get_onpause,
    HTMLDocument7_put_onplay,
    HTMLDocument7_get_onplay,
    HTMLDocument7_put_onplaying,
    HTMLDocument7_get_onplaying,
    HTMLDocument7_put_onprogress,
    HTMLDocument7_get_onprogress,
    HTMLDocument7_put_onratechange,
    HTMLDocument7_get_onratechange,
    HTMLDocument7_put_onreset,
    HTMLDocument7_get_onreset,
    HTMLDocument7_put_onscroll,
    HTMLDocument7_get_onscroll,
    HTMLDocument7_put_onseekend,
    HTMLDocument7_get_onseekend,
    HTMLDocument7_put_onseeking,
    HTMLDocument7_get_onseeking,
    HTMLDocument7_put_onselect,
    HTMLDocument7_get_onselect,
    HTMLDocument7_put_onstalled,
    HTMLDocument7_get_onstalled,
    HTMLDocument7_put_onsubmit,
    HTMLDocument7_get_onsubmit,
    HTMLDocument7_put_onsuspend,
    HTMLDocument7_get_onsuspend,
    HTMLDocument7_put_ontimeupdate,
    HTMLDocument7_get_ontimeupdate,
    HTMLDocument7_put_onvolumechange,
    HTMLDocument7_get_onvolumechange,
    HTMLDocument7_put_onwaiting,
    HTMLDocument7_get_onwaiting,
    HTMLDocument7_normalize,
    HTMLDocument7_importNode,
    HTMLDocument7_get_parentWindow,
    HTMLDocument7_put_body,
    HTMLDocument7_get_body,
    HTMLDocument7_get_head
};

static inline HTMLDocument *impl_from_IDocumentSelector(IDocumentSelector *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IDocumentSelector_iface);
}

static HRESULT WINAPI DocumentSelector_QueryInterface(IDocumentSelector *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI DocumentSelector_AddRef(IDocumentSelector *iface)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI DocumentSelector_Release(IDocumentSelector *iface)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI DocumentSelector_GetTypeInfoCount(IDocumentSelector *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DocumentSelector_GetTypeInfo(IDocumentSelector *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocumentSelector_GetIDsOfNames(IDocumentSelector *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI DocumentSelector_Invoke(IDocumentSelector *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DocumentSelector_querySelector(IDocumentSelector *iface, BSTR v, IHTMLElement **pel)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLDocument_QuerySelector(This->doc_node->nsdoc, &nsstr, &nselem);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("QuerySelector failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *pel = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *pel = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI DocumentSelector_querySelectorAll(IDocumentSelector *iface, BSTR v, IHTMLDOMChildrenCollection **pel)
{
    HTMLDocument *This = impl_from_IDocumentSelector(iface);
    nsIDOMNodeList *node_list;
    nsAString nsstr;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    nsAString_InitDepend(&nsstr, v);
    hres = map_nsresult(nsIDOMHTMLDocument_QuerySelectorAll(This->doc_node->nsdoc, &nsstr, &node_list));
    nsAString_Finish(&nsstr);
    if(FAILED(hres)) {
        ERR("QuerySelectorAll failed: %08lx\n", hres);
        return hres;
    }

    hres = create_child_collection(node_list, dispex_compat_mode(&This->doc_node->node.event_target.dispex), pel);
    nsIDOMNodeList_Release(node_list);
    return hres;
}

static const IDocumentSelectorVtbl DocumentSelectorVtbl = {
    DocumentSelector_QueryInterface,
    DocumentSelector_AddRef,
    DocumentSelector_Release,
    DocumentSelector_GetTypeInfoCount,
    DocumentSelector_GetTypeInfo,
    DocumentSelector_GetIDsOfNames,
    DocumentSelector_Invoke,
    DocumentSelector_querySelector,
    DocumentSelector_querySelectorAll
};

static inline HTMLDocument *impl_from_IDocumentEvent(IDocumentEvent *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IDocumentEvent_iface);
}

static HRESULT WINAPI DocumentEvent_QueryInterface(IDocumentEvent *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI DocumentEvent_AddRef(IDocumentEvent *iface)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI DocumentEvent_Release(IDocumentEvent *iface)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI DocumentEvent_GetTypeInfoCount(IDocumentEvent *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DocumentEvent_GetTypeInfo(IDocumentEvent *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocumentEvent_GetIDsOfNames(IDocumentEvent *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI DocumentEvent_Invoke(IDocumentEvent *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DocumentEvent_createEvent(IDocumentEvent *iface, BSTR eventType, IDOMEvent **p)
{
    HTMLDocument *This = impl_from_IDocumentEvent(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(eventType), p);

    return create_document_event_str(This->doc_node, eventType, p);
}

static const IDocumentEventVtbl DocumentEventVtbl = {
    DocumentEvent_QueryInterface,
    DocumentEvent_AddRef,
    DocumentEvent_Release,
    DocumentEvent_GetTypeInfoCount,
    DocumentEvent_GetTypeInfo,
    DocumentEvent_GetIDsOfNames,
    DocumentEvent_Invoke,
    DocumentEvent_createEvent
};

static void HTMLDocument_on_advise(IUnknown *iface, cp_static_data_t *cp)
{
    HTMLDocument *This = impl_from_IHTMLDocument2((IHTMLDocument2*)iface);

    if(This->window)
        update_doc_cp_events(This->doc_node, cp);
}

static inline HTMLDocument *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, ISupportErrorInfo_iface);
}

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_ISupportErrorInfo(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    HTMLDocument *This = impl_from_ISupportErrorInfo(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    HTMLDocument *This = impl_from_ISupportErrorInfo(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    FIXME("(%p)->(%s)\n", iface, debugstr_mshtml_guid(riid));
    return S_FALSE;
}

static const ISupportErrorInfoVtbl SupportErrorInfoVtbl = {
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo
};

static inline HTMLDocument *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IDispatchEx_iface);
}

static HRESULT has_elem_name(nsIDOMHTMLDocument *nsdoc, const WCHAR *name)
{
    static const WCHAR fmt[] = L":-moz-any(applet,embed,form,iframe,img,object)[name=\"%s\"]";
    WCHAR buf[128], *selector = buf;
    nsAString selector_str;
    nsIDOMElement *nselem;
    nsresult nsres;
    size_t len;

    len = wcslen(name) + ARRAY_SIZE(fmt) - 2 /* %s */;
    if(len > ARRAY_SIZE(buf) && !(selector = heap_alloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;
    swprintf(selector, len, fmt, name);

    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMHTMLDocument_QuerySelector(nsdoc, &selector_str, &nselem);
    nsAString_Finish(&selector_str);
    if(selector != buf)
        heap_free(selector);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    if(!nselem)
        return DISP_E_UNKNOWNNAME;
    nsIDOMElement_Release(nselem);
    return S_OK;
}

static HRESULT get_elem_by_name_or_id(nsIDOMHTMLDocument *nsdoc, const WCHAR *name, nsIDOMElement **ret)
{
    static const WCHAR fmt[] = L":-moz-any(embed,form,iframe,img):-moz-any([name=\"%s\"],[id=\"%s\"][name]),"
                               L":-moz-any(applet,object):-moz-any([name=\"%s\"],[id=\"%s\"])";
    WCHAR buf[384], *selector = buf;
    nsAString selector_str;
    nsIDOMElement *nselem;
    nsresult nsres;
    size_t len;

    len = wcslen(name) * 4 + ARRAY_SIZE(fmt) - 8 /* %s */;
    if(len > ARRAY_SIZE(buf) && !(selector = heap_alloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;
    swprintf(selector, len, fmt, name, name, name, name);

    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMHTMLDocument_QuerySelector(nsdoc, &selector_str, &nselem);
    nsAString_Finish(&selector_str);
    if(selector != buf)
        heap_free(selector);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    if(ret) {
        *ret = nselem;
        return S_OK;
    }

    if(nselem) {
        nsIDOMElement_Release(nselem);
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

static HRESULT dispid_from_elem_name(HTMLDocumentNode *This, const WCHAR *name, DISPID *dispid)
{
    unsigned i;

    for(i=0; i < This->elem_vars_cnt; i++) {
        if(!wcscmp(name, This->elem_vars[i])) {
            *dispid = MSHTML_DISPID_CUSTOM_MIN+i;
            return S_OK;
        }
    }

    if(This->elem_vars_cnt == This->elem_vars_size) {
        WCHAR **new_vars;

        if(This->elem_vars_size) {
            new_vars = heap_realloc(This->elem_vars, This->elem_vars_size*2*sizeof(WCHAR*));
            if(!new_vars)
                return E_OUTOFMEMORY;
            This->elem_vars_size *= 2;
        }else {
            new_vars = heap_alloc(16*sizeof(WCHAR*));
            if(!new_vars)
                return E_OUTOFMEMORY;
            This->elem_vars_size = 16;
        }

        This->elem_vars = new_vars;
    }

    This->elem_vars[This->elem_vars_cnt] = heap_strdupW(name);
    if(!This->elem_vars[This->elem_vars_cnt])
        return E_OUTOFMEMORY;

    *dispid = MSHTML_DISPID_CUSTOM_MIN+This->elem_vars_cnt++;
    return S_OK;
}

static HRESULT WINAPI DocDispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI DocDispatchEx_AddRef(IDispatchEx *iface)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return htmldoc_addref(This);
}

static ULONG WINAPI DocDispatchEx_Release(IDispatchEx *iface)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return htmldoc_release(This);
}

static HRESULT WINAPI DocDispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetTypeInfoCount(This->dispex, pctinfo);
}

static HRESULT WINAPI DocDispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetTypeInfo(This->dispex, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocDispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetIDsOfNames(This->dispex, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI DocDispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DocDispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);
    HRESULT hres;

    hres = IDispatchEx_GetDispID(This->dispex, bstrName, grfdex & ~fdexNameEnsure, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    if(This->doc_node->nsdoc) {
        hres = get_elem_by_name_or_id(This->doc_node->nsdoc, bstrName, NULL);
        if(SUCCEEDED(hres))
            hres = dispid_from_elem_name(This->doc_node, bstrName, pid);
    }

    if(hres == DISP_E_UNKNOWNNAME && (grfdex & fdexNameEnsure))
        hres = IDispatchEx_GetDispID(This->dispex, bstrName, grfdex, pid);
    return hres;
}

static HRESULT WINAPI DocDispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    if(This->window) {
        switch(id) {
        case DISPID_READYSTATE:
            TRACE("DISPID_READYSTATE\n");

            if(!(wFlags & DISPATCH_PROPERTYGET))
                return E_INVALIDARG;

            V_VT(pvarRes) = VT_I4;
            V_I4(pvarRes) = This->window->readystate;
            return S_OK;
        default:
            break;
        }
    }

    return IDispatchEx_InvokeEx(This->dispex, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI DocDispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_DeleteMemberByName(This->dispex, bstrName, grfdex);
}

static HRESULT WINAPI DocDispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_DeleteMemberByDispID(This->dispex, id);
}

static HRESULT WINAPI DocDispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetMemberProperties(This->dispex, id, grfdexFetch, pgrfdex);
}

static HRESULT WINAPI DocDispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetMemberName(This->dispex, id, pbstrName);
}

static HRESULT WINAPI DocDispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetNextDispID(This->dispex, grfdex, id, pid);
}

static HRESULT WINAPI DocDispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLDocument *This = impl_from_IDispatchEx(iface);

    return IDispatchEx_GetNameSpaceParent(This->dispex, ppunk);
}

static const IDispatchExVtbl DocDispatchExVtbl = {
    DocDispatchEx_QueryInterface,
    DocDispatchEx_AddRef,
    DocDispatchEx_Release,
    DocDispatchEx_GetTypeInfoCount,
    DocDispatchEx_GetTypeInfo,
    DocDispatchEx_GetIDsOfNames,
    DocDispatchEx_Invoke,
    DocDispatchEx_GetDispID,
    DocDispatchEx_InvokeEx,
    DocDispatchEx_DeleteMemberByName,
    DocDispatchEx_DeleteMemberByDispID,
    DocDispatchEx_GetMemberProperties,
    DocDispatchEx_GetMemberName,
    DocDispatchEx_GetNextDispID,
    DocDispatchEx_GetNameSpaceParent
};

static inline HTMLDocument *impl_from_IProvideMultipleClassInfo(IProvideMultipleClassInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IProvideMultipleClassInfo_iface);
}

static HRESULT WINAPI ProvideClassInfo_QueryInterface(IProvideMultipleClassInfo *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI ProvideClassInfo_AddRef(IProvideMultipleClassInfo *iface)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI ProvideClassInfo_Release(IProvideMultipleClassInfo *iface)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI ProvideClassInfo_GetClassInfo(IProvideMultipleClassInfo *iface, ITypeInfo **ppTI)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p)->(%p)\n", This, ppTI);
    return get_class_typeinfo(&CLSID_HTMLDocument, ppTI);
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideMultipleClassInfo *iface, DWORD dwGuidKind, GUID *pGUID)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %p)\n", This, dwGuidKind, pGUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetMultiTypeInfoCount(IProvideMultipleClassInfo *iface, ULONG *pcti)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%p)\n", This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetInfoOfIndex(IProvideMultipleClassInfo *iface, ULONG iti,
        DWORD dwFlags, ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved, IID *piidPrimary, IID *piidSource)
{
    HTMLDocument *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %lx %p %p %p %p %p)\n", This, iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource);
    return E_NOTIMPL;
}

static const IProvideMultipleClassInfoVtbl ProvideMultipleClassInfoVtbl = {
    ProvideClassInfo_QueryInterface,
    ProvideClassInfo_AddRef,
    ProvideClassInfo_Release,
    ProvideClassInfo_GetClassInfo,
    ProvideClassInfo2_GetGUID,
    ProvideMultipleClassInfo_GetMultiTypeInfoCount,
    ProvideMultipleClassInfo_GetInfoOfIndex
};

/**********************************************************
 * IMarkupServices implementation
 */
static inline HTMLDocument *impl_from_IMarkupServices(IMarkupServices *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IMarkupServices_iface);
}

static HRESULT WINAPI MarkupServices_QueryInterface(IMarkupServices *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    return htmldoc_query_interface(This, riid, ppvObject);
}

static ULONG WINAPI MarkupServices_AddRef(IMarkupServices *iface)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI MarkupServices_Release(IMarkupServices *iface)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI MarkupServices_CreateMarkupPointer(IMarkupServices *iface, IMarkupPointer **ppPointer)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);

    TRACE("(%p)->(%p)\n", This, ppPointer);

    return create_markup_pointer(ppPointer);
}

static HRESULT WINAPI MarkupServices_CreateMarkupContainer(IMarkupServices *iface, IMarkupContainer **ppMarkupContainer)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p)\n", This, ppMarkupContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_CreateElement(IMarkupServices *iface,
    ELEMENT_TAG_ID tagID, OLECHAR *pchAttributes, IHTMLElement **ppElement)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%d,%s,%p)\n", This, tagID, debugstr_w(pchAttributes), ppElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_CloneElement(IMarkupServices *iface,
    IHTMLElement *pElemCloneThis, IHTMLElement **ppElementTheClone)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElemCloneThis, ppElementTheClone);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_InsertElement(IMarkupServices *iface,
    IHTMLElement *pElementInsert, IMarkupPointer *pPointerStart,
    IMarkupPointer *pPointerFinish)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pElementInsert, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_RemoveElement(IMarkupServices *iface, IHTMLElement *pElementRemove)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p)\n", This, pElementRemove);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Remove(IMarkupServices *iface,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Copy(IMarkupServices *iface,
    IMarkupPointer *pPointerSourceStart, IMarkupPointer *pPointerSourceFinish,
    IMarkupPointer *pPointerTarget)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerSourceStart, pPointerSourceFinish, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Move(IMarkupServices *iface,
    IMarkupPointer *pPointerSourceStart, IMarkupPointer *pPointerSourceFinish,
    IMarkupPointer *pPointerTarget)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerSourceStart, pPointerSourceFinish, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_InsertText(IMarkupServices *iface,
    OLECHAR *pchText, LONG cch, IMarkupPointer *pPointerTarget)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%lx,%p)\n", This, debugstr_w(pchText), cch, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_ParseString(IMarkupServices *iface,
    OLECHAR *pchHTML, DWORD dwFlags, IMarkupContainer **ppContainerResult,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%lx,%p,%p,%p)\n", This, debugstr_w(pchHTML), dwFlags, ppContainerResult, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_ParseGlobal(IMarkupServices *iface,
    HGLOBAL hglobalHTML, DWORD dwFlags, IMarkupContainer **ppContainerResult,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%lx,%p,%p,%p)\n", This, debugstr_w(hglobalHTML), dwFlags, ppContainerResult, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_IsScopedElement(IMarkupServices *iface,
    IHTMLElement *pElement, BOOL *pfScoped)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElement, pfScoped);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetElementTagId(IMarkupServices *iface,
    IHTMLElement *pElement, ELEMENT_TAG_ID *ptagId)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElement, ptagId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetTagIDForName(IMarkupServices *iface,
    BSTR bstrName, ELEMENT_TAG_ID *ptagId)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%p)\n", This, debugstr_w(bstrName), ptagId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetNameForTagID(IMarkupServices *iface,
    ELEMENT_TAG_ID tagId, BSTR *pbstrName)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%d,%p)\n", This, tagId, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_MovePointersToRange(IMarkupServices *iface,
    IHTMLTxtRange *pIRange, IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pIRange, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_MoveRangeToPointers(IMarkupServices *iface,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish, IHTMLTxtRange *pIRange)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerStart, pPointerFinish, pIRange);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_BeginUndoUnit(IMarkupServices *iface, OLECHAR *pchTitle)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pchTitle));
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_EndUndoUnit(IMarkupServices *iface)
{
    HTMLDocument *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IMarkupServicesVtbl MarkupServicesVtbl = {
    MarkupServices_QueryInterface,
    MarkupServices_AddRef,
    MarkupServices_Release,
    MarkupServices_CreateMarkupPointer,
    MarkupServices_CreateMarkupContainer,
    MarkupServices_CreateElement,
    MarkupServices_CloneElement,
    MarkupServices_InsertElement,
    MarkupServices_RemoveElement,
    MarkupServices_Remove,
    MarkupServices_Copy,
    MarkupServices_Move,
    MarkupServices_InsertText,
    MarkupServices_ParseString,
    MarkupServices_ParseGlobal,
    MarkupServices_IsScopedElement,
    MarkupServices_GetElementTagId,
    MarkupServices_GetTagIDForName,
    MarkupServices_GetNameForTagID,
    MarkupServices_MovePointersToRange,
    MarkupServices_MoveRangeToPointers,
    MarkupServices_BeginUndoUnit,
    MarkupServices_EndUndoUnit
};

/**********************************************************
 * IMarkupContainer implementation
 */
static inline HTMLDocument *impl_from_IMarkupContainer(IMarkupContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IMarkupContainer_iface);
}

static HRESULT WINAPI MarkupContainer_QueryInterface(IMarkupContainer *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = impl_from_IMarkupContainer(iface);
    return htmldoc_query_interface(This, riid, ppvObject);
}

static ULONG WINAPI MarkupContainer_AddRef(IMarkupContainer *iface)
{
    HTMLDocument *This = impl_from_IMarkupContainer(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI MarkupContainer_Release(IMarkupContainer *iface)
{
    HTMLDocument *This = impl_from_IMarkupContainer(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI MarkupContainer_OwningDoc(IMarkupContainer *iface, IHTMLDocument2 **ppDoc)
{
    HTMLDocument *This = impl_from_IMarkupContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppDoc);
    return E_NOTIMPL;
}

static const IMarkupContainerVtbl MarkupContainerVtbl = {
    MarkupContainer_QueryInterface,
    MarkupContainer_AddRef,
    MarkupContainer_Release,
    MarkupContainer_OwningDoc
};

/**********************************************************
 * IDisplayServices implementation
 */
static inline HTMLDocument *impl_from_IDisplayServices(IDisplayServices *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IDisplayServices_iface);
}

static HRESULT WINAPI DisplayServices_QueryInterface(IDisplayServices *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    return htmldoc_query_interface(This, riid, ppvObject);
}

static ULONG WINAPI DisplayServices_AddRef(IDisplayServices *iface)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI DisplayServices_Release(IDisplayServices *iface)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI DisplayServices_CreateDisplayPointer(IDisplayServices *iface, IDisplayPointer **ppDispPointer)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p)\n", This, ppDispPointer);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_TransformRect(IDisplayServices *iface,
    RECT *pRect, COORD_SYSTEM eSource, COORD_SYSTEM eDestination, IHTMLElement *pIElement)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%s,%d,%d,%p)\n", This, wine_dbgstr_rect(pRect), eSource, eDestination, pIElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_TransformPoint(IDisplayServices *iface,
    POINT *pPoint, COORD_SYSTEM eSource, COORD_SYSTEM eDestination, IHTMLElement *pIElement)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%s,%d,%d,%p)\n", This, wine_dbgstr_point(pPoint), eSource, eDestination, pIElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_GetCaret(IDisplayServices *iface, IHTMLCaret **ppCaret)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p)\n", This, ppCaret);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_GetComputedStyle(IDisplayServices *iface,
    IMarkupPointer *pPointer, IHTMLComputedStyle **ppComputedStyle)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pPointer, ppComputedStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_ScrollRectIntoView(IDisplayServices *iface,
    IHTMLElement *pIElement, RECT rect)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%s)\n", This, pIElement, wine_dbgstr_rect(&rect));
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_HasFlowLayout(IDisplayServices *iface,
    IHTMLElement *pIElement, BOOL *pfHasFlowLayout)
{
    HTMLDocument *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pIElement, pfHasFlowLayout);
    return E_NOTIMPL;
}

static const IDisplayServicesVtbl DisplayServicesVtbl = {
    DisplayServices_QueryInterface,
    DisplayServices_AddRef,
    DisplayServices_Release,
    DisplayServices_CreateDisplayPointer,
    DisplayServices_TransformRect,
    DisplayServices_TransformPoint,
    DisplayServices_GetCaret,
    DisplayServices_GetComputedStyle,
    DisplayServices_ScrollRectIntoView,
    DisplayServices_HasFlowLayout
};

/**********************************************************
 * IDocumentRange implementation
 */
static inline HTMLDocument *impl_from_IDocumentRange(IDocumentRange *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IDocumentRange_iface);
}

static HRESULT WINAPI DocumentRange_QueryInterface(IDocumentRange *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI DocumentRange_AddRef(IDocumentRange *iface)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return htmldoc_addref(This);
}

static ULONG WINAPI DocumentRange_Release(IDocumentRange *iface)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return htmldoc_release(This);
}

static HRESULT WINAPI DocumentRange_GetTypeInfoCount(IDocumentRange *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI DocumentRange_GetTypeInfo(IDocumentRange *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocumentRange_GetIDsOfNames(IDocumentRange *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
    LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI DocumentRange_Invoke(IDocumentRange *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DocumentRange_createRange(IDocumentRange *iface, IHTMLDOMRange **p)
{
    HTMLDocument *This = impl_from_IDocumentRange(iface);
    nsIDOMRange *nsrange;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsIDOMHTMLDocument_CreateRange(This->doc_node->nsdoc, &nsrange)))
        return E_FAIL;

    hres = create_dom_range(nsrange, dispex_compat_mode(&This->doc_node->node.event_target.dispex), p);
    nsIDOMRange_Release(nsrange);
    return hres;
}

static const IDocumentRangeVtbl DocumentRangeVtbl = {
    DocumentRange_QueryInterface,
    DocumentRange_AddRef,
    DocumentRange_Release,
    DocumentRange_GetTypeInfoCount,
    DocumentRange_GetTypeInfo,
    DocumentRange_GetIDsOfNames,
    DocumentRange_Invoke,
    DocumentRange_createRange,
};

static BOOL htmldoc_qi(HTMLDocument *This, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = &This->IHTMLDocument2_iface;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &This->IDispatchEx_iface;
    else if(IsEqualGUID(&IID_IDispatchEx, riid))
        *ppv = &This->IDispatchEx_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument, riid))
        *ppv = &This->IHTMLDocument2_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument2, riid))
        *ppv = &This->IHTMLDocument2_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument3, riid))
        *ppv = &This->IHTMLDocument3_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument4, riid))
        *ppv = &This->IHTMLDocument4_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument5, riid))
        *ppv = &This->IHTMLDocument5_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument6, riid))
        *ppv = &This->IHTMLDocument6_iface;
    else if(IsEqualGUID(&IID_IHTMLDocument7, riid))
        *ppv = &This->IHTMLDocument7_iface;
    else if(IsEqualGUID(&IID_IDocumentSelector, riid))
        *ppv = &This->IDocumentSelector_iface;
    else if(IsEqualGUID(&IID_IDocumentEvent, riid))
        *ppv = &This->IDocumentEvent_iface;
    else if(IsEqualGUID(&IID_IPersist, riid))
        *ppv = &This->IPersistFile_iface;
    else if(IsEqualGUID(&IID_IPersistMoniker, riid))
        *ppv = &This->IPersistMoniker_iface;
    else if(IsEqualGUID(&IID_IPersistFile, riid))
        *ppv = &This->IPersistFile_iface;
    else if(IsEqualGUID(&IID_IMonikerProp, riid))
        *ppv = &This->IMonikerProp_iface;
    else if(IsEqualGUID(&IID_IOleObject, riid))
        *ppv = &This->IOleObject_iface;
    else if(IsEqualGUID(&IID_IOleDocument, riid))
        *ppv = &This->IOleDocument_iface;
    else if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid))
        *ppv = &This->IOleInPlaceActiveObject_iface;
    else if(IsEqualGUID(&IID_IOleWindow, riid))
        *ppv = &This->IOleInPlaceActiveObject_iface;
    else if(IsEqualGUID(&IID_IOleInPlaceObject, riid))
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    else if(IsEqualGUID(&IID_IOleInPlaceObjectWindowless, riid))
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &This->IServiceProvider_iface;
    else if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        *ppv = &This->IOleCommandTarget_iface;
    else if(IsEqualGUID(&IID_IOleControl, riid))
        *ppv = &This->IOleControl_iface;
    else if(IsEqualGUID(&IID_IHlinkTarget, riid))
        *ppv = &This->IHlinkTarget_iface;
    else if(IsEqualGUID(&IID_IConnectionPointContainer, riid))
        *ppv = &This->cp_container.IConnectionPointContainer_iface;
    else if(IsEqualGUID(&IID_IPersistStreamInit, riid))
        *ppv = &This->IPersistStreamInit_iface;
    else if(IsEqualGUID(&DIID_DispHTMLDocument, riid))
        *ppv = &This->IHTMLDocument2_iface;
    else if(IsEqualGUID(&IID_ISupportErrorInfo, riid))
        *ppv = &This->ISupportErrorInfo_iface;
    else if(IsEqualGUID(&IID_IPersistHistory, riid))
        *ppv = &This->IPersistHistory_iface;
    else if(IsEqualGUID(&IID_IObjectWithSite, riid))
        *ppv = &This->IObjectWithSite_iface;
    else if(IsEqualGUID(&IID_IOleContainer, riid))
        *ppv = &This->IOleContainer_iface;
    else if(IsEqualGUID(&IID_IObjectSafety, riid))
        *ppv = &This->IObjectSafety_iface;
    else if(IsEqualGUID(&IID_IProvideClassInfo, riid))
        *ppv = &This->IProvideMultipleClassInfo_iface;
    else if(IsEqualGUID(&IID_IProvideClassInfo2, riid))
        *ppv = &This->IProvideMultipleClassInfo_iface;
    else if(IsEqualGUID(&IID_IProvideMultipleClassInfo, riid))
        *ppv = &This->IProvideMultipleClassInfo_iface;
    else if(IsEqualGUID(&IID_IMarkupServices, riid))
        *ppv = &This->IMarkupServices_iface;
    else if(IsEqualGUID(&IID_IMarkupContainer, riid))
        *ppv = &This->IMarkupContainer_iface;
    else if(IsEqualGUID(&IID_IDisplayServices, riid))
        *ppv = &This->IDisplayServices_iface;
    else if(IsEqualGUID(&IID_IDocumentRange, riid))
        *ppv = &This->IDocumentRange_iface;
    else if(IsEqualGUID(&CLSID_CMarkup, riid)) {
        FIXME("(%p)->(CLSID_CMarkup %p)\n", This, ppv);
        *ppv = NULL;
    }else if(IsEqualGUID(&IID_IRunnableObject, riid)) {
        TRACE("(%p)->(IID_IRunnableObject %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else if(IsEqualGUID(&IID_IPersistPropertyBag, riid)) {
        TRACE("(%p)->(IID_IPersistPropertyBag %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else if(IsEqualGUID(&IID_IExternalConnection, riid)) {
        TRACE("(%p)->(IID_IExternalConnection %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else if(IsEqualGUID(&IID_IStdMarshalInfo, riid)) {
        TRACE("(%p)->(IID_IStdMarshalInfo %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else {
        return FALSE;
    }

    if(*ppv)
        IUnknown_AddRef((IUnknown*)*ppv);
    return TRUE;
}

static cp_static_data_t HTMLDocumentEvents_data = { HTMLDocumentEvents_tid, HTMLDocument_on_advise };
static cp_static_data_t HTMLDocumentEvents2_data = { HTMLDocumentEvents2_tid, HTMLDocument_on_advise, TRUE };

static const cpc_entry_t HTMLDocument_cpc[] = {
    {&IID_IDispatch, &HTMLDocumentEvents_data},
    {&IID_IPropertyNotifySink},
    {&DIID_HTMLDocumentEvents, &HTMLDocumentEvents_data},
    {&DIID_HTMLDocumentEvents2, &HTMLDocumentEvents2_data},
    {NULL}
};

static void init_doc(HTMLDocument *doc, IUnknown *outer, IDispatchEx *dispex)
{
    doc->IHTMLDocument2_iface.lpVtbl = &HTMLDocumentVtbl;
    doc->IHTMLDocument3_iface.lpVtbl = &HTMLDocument3Vtbl;
    doc->IHTMLDocument4_iface.lpVtbl = &HTMLDocument4Vtbl;
    doc->IHTMLDocument5_iface.lpVtbl = &HTMLDocument5Vtbl;
    doc->IHTMLDocument6_iface.lpVtbl = &HTMLDocument6Vtbl;
    doc->IHTMLDocument7_iface.lpVtbl = &HTMLDocument7Vtbl;
    doc->IDispatchEx_iface.lpVtbl = &DocDispatchExVtbl;
    doc->IDocumentSelector_iface.lpVtbl = &DocumentSelectorVtbl;
    doc->IDocumentEvent_iface.lpVtbl = &DocumentEventVtbl;
    doc->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    doc->IProvideMultipleClassInfo_iface.lpVtbl = &ProvideMultipleClassInfoVtbl;
    doc->IMarkupServices_iface.lpVtbl = &MarkupServicesVtbl;
    doc->IMarkupContainer_iface.lpVtbl = &MarkupContainerVtbl;
    doc->IDisplayServices_iface.lpVtbl = &DisplayServicesVtbl;
    doc->IDocumentRange_iface.lpVtbl = &DocumentRangeVtbl;

    doc->outer_unk = outer;
    doc->dispex = dispex;

    HTMLDocument_Persist_Init(doc);
    HTMLDocument_OleCmd_Init(doc);
    HTMLDocument_OleObj_Init(doc);
    HTMLDocument_Service_Init(doc);

    ConnectionPointContainer_Init(&doc->cp_container, (IUnknown*)&doc->IHTMLDocument2_iface, HTMLDocument_cpc);
}

static inline HTMLDocumentNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, node);
}

static HRESULT HTMLDocumentNode_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(htmldoc_qi(&This->basedoc, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;

    if(IsEqualGUID(&IID_IInternetHostSecurityManager, riid))
        *ppv = &This->IInternetHostSecurityManager_iface;
    else
        return HTMLDOMNode_QI(&This->node, riid, ppv);

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

void detach_document_node(HTMLDocumentNode *doc)
{
    unsigned i;

    while(!list_empty(&doc->plugin_hosts))
        detach_plugin_host(LIST_ENTRY(list_head(&doc->plugin_hosts), PluginHost, entry));

    if(doc->dom_implementation) {
        detach_dom_implementation(doc->dom_implementation);
        IHTMLDOMImplementation_Release(doc->dom_implementation);
        doc->dom_implementation = NULL;
    }

    if(doc->namespaces) {
        IHTMLNamespaceCollection_Release(doc->namespaces);
        doc->namespaces = NULL;
    }

    detach_events(doc);
    detach_selection(doc);
    detach_ranges(doc);

    for(i=0; i < doc->elem_vars_cnt; i++)
        heap_free(doc->elem_vars[i]);
    heap_free(doc->elem_vars);
    doc->elem_vars_cnt = doc->elem_vars_size = 0;
    doc->elem_vars = NULL;

    if(doc->catmgr) {
        ICatInformation_Release(doc->catmgr);
        doc->catmgr = NULL;
    }

    if(!doc->nsdoc && doc->window) {
        /* document fragments own reference to inner window */
        IHTMLWindow2_Release(&doc->window->base.IHTMLWindow2_iface);
        doc->window = NULL;
    }

    if(doc->browser) {
        list_remove(&doc->browser_entry);
        doc->browser = NULL;
    }
}

static void HTMLDocumentNode_destructor(HTMLDOMNode *iface)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);

    TRACE("(%p)\n", This);

    heap_free(This->event_vector);
    ConnectionPointContainer_Destroy(&This->basedoc.cp_container);
}

static HRESULT HTMLDocumentNode_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static void HTMLDocumentNode_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);

    if(This->nsdoc)
        note_cc_edge((nsISupports*)This->nsdoc, "This->nsdoc", cb);
}

static void HTMLDocumentNode_unlink(HTMLDOMNode *iface)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);

    if(This->nsdoc) {
        nsIDOMHTMLDocument *nsdoc = This->nsdoc;

        release_document_mutation(This);
        detach_document_node(This);
        This->nsdoc = NULL;
        nsIDOMHTMLDocument_Release(nsdoc);
        This->window = NULL;
    }
}

static const NodeImplVtbl HTMLDocumentNodeImplVtbl = {
    &CLSID_HTMLDocument,
    HTMLDocumentNode_QI,
    HTMLDocumentNode_destructor,
    HTMLDocument_cpc,
    HTMLDocumentNode_clone,
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
    HTMLDocumentNode_traverse,
    HTMLDocumentNode_unlink
};

static HRESULT HTMLDocumentFragment_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);
    HTMLDocumentNode *new_node;
    HRESULT hres;

    hres = create_document_fragment(nsnode, This->node.doc, &new_node);
    if(FAILED(hres))
        return hres;

    *ret = &new_node->node;
    return S_OK;
}

static inline HTMLDocumentNode *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, node.event_target.dispex);
}

static HRESULT HTMLDocumentNode_get_name(DispatchEx *dispex, DISPID id, BSTR *name)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;

    if(!This->nsdoc || idx >= This->elem_vars_cnt)
        return DISP_E_MEMBERNOTFOUND;

    return (*name = SysAllocString(This->elem_vars[idx])) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT HTMLDocumentNode_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    nsIDOMElement *nselem;
    HTMLDOMNode *node;
    unsigned i;
    HRESULT hres;

    if(flags != DISPATCH_PROPERTYGET && flags != (DISPATCH_METHOD|DISPATCH_PROPERTYGET))
        return MSHTML_E_INVALID_PROPERTY;

    i = id - MSHTML_DISPID_CUSTOM_MIN;

    if(!This->nsdoc || i >= This->elem_vars_cnt)
        return DISP_E_MEMBERNOTFOUND;

    hres = get_elem_by_name_or_id(This->nsdoc, This->elem_vars[i], &nselem);
    if(FAILED(hres))
        return hres;
    if(!nselem)
        return DISP_E_MEMBERNOTFOUND;

    hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)&node->IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT HTMLDocumentNode_next_dispid(DispatchEx *dispex, DISPID id, DISPID *pid)
{
    DWORD idx = (id == DISPID_STARTENUM) ? 0 : id - MSHTML_DISPID_CUSTOM_MIN + 1;
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    nsIDOMNodeList *node_list;
    const PRUnichar *name;
    nsIDOMElement *nselem;
    nsIDOMNode *nsnode;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;
    UINT32 i;

    if(!This->nsdoc)
        return S_FALSE;

    while(idx < This->elem_vars_cnt) {
        hres = has_elem_name(This->nsdoc, This->elem_vars[idx]);
        if(SUCCEEDED(hres)) {
            *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
            return S_OK;
        }
        if(hres != DISP_E_UNKNOWNNAME)
            return hres;
        idx++;
    }

    /* Populate possibly missing DISPIDs */
    nsAString_InitDepend(&nsstr, L":-moz-any(applet,embed,form,iframe,img,object)[name]");
    nsres = nsIDOMHTMLDocument_QuerySelectorAll(This->nsdoc, &nsstr, &node_list);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    for(i = 0, hres = S_OK; SUCCEEDED(hres); i++) {
        nsres = nsIDOMNodeList_Item(node_list, i, &nsnode);
        if(NS_FAILED(nsres)) {
            hres = map_nsresult(nsres);
            break;
        }
        if(!nsnode)
            break;

        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
        nsIDOMNode_Release(nsnode);
        if(nsres != S_OK)
            continue;

        nsres = get_elem_attr_value(nselem, L"name", &nsstr, &name);
        nsIDOMElement_Release(nselem);
        if(NS_FAILED(nsres))
            hres = map_nsresult(nsres);
        else {
            hres = dispid_from_elem_name(This, name, &id);
            nsAString_Finish(&nsstr);
        }
    }
    nsIDOMNodeList_Release(node_list);
    if(FAILED(hres))
        return hres;

    if(idx >= This->elem_vars_cnt)
        return S_FALSE;

    *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
    return S_OK;
}

static compat_mode_t HTMLDocumentNode_get_compat_mode(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    TRACE("(%p) returning %u\n", This, This->document_mode);

    return lock_document_mode(This);
}

static nsISupports *HTMLDocumentNode_get_gecko_target(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    return (nsISupports*)This->node.nsnode;
}

static void HTMLDocumentNode_bind_event(DispatchEx *dispex, eventid_t eid)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    ensure_doc_nsevent_handler(This, This->node.nsnode, eid);
}

static EventTarget *HTMLDocumentNode_get_parent_event_target(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    if(!This->window)
        return NULL;
    IHTMLWindow2_AddRef(&This->window->base.IHTMLWindow2_iface);
    return &This->window->event_target;
}

static ConnectionPointContainer *HTMLDocumentNode_get_cp_container(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    ConnectionPointContainer *container = This->basedoc.doc_obj
        ? &This->basedoc.doc_obj->basedoc.cp_container : &This->basedoc.cp_container;
    IConnectionPointContainer_AddRef(&container->IConnectionPointContainer_iface);
    return container;
}

static IHTMLEventObj *HTMLDocumentNode_set_current_event(DispatchEx *dispex, IHTMLEventObj *event)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    return default_set_current_event(This->window, event);
}

static HRESULT HTMLDocumentNode_location_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    return IHTMLDocument2_location_hook(&impl_from_DispatchEx(dispex)->basedoc, flags, dp, res, ei, caller);
}

static const event_target_vtbl_t HTMLDocumentNode_event_target_vtbl = {
    {
        NULL,
        NULL,
        HTMLDocumentNode_get_name,
        HTMLDocumentNode_invoke,
        NULL,
        HTMLDocumentNode_next_dispid,
        HTMLDocumentNode_get_compat_mode,
        NULL
    },
    HTMLDocumentNode_get_gecko_target,
    HTMLDocumentNode_bind_event,
    HTMLDocumentNode_get_parent_event_target,
    NULL,
    HTMLDocumentNode_get_cp_container,
    HTMLDocumentNode_set_current_event
};

static const NodeImplVtbl HTMLDocumentFragmentImplVtbl = {
    &CLSID_HTMLDocument,
    HTMLDocumentNode_QI,
    HTMLDocumentNode_destructor,
    HTMLDocument_cpc,
    HTMLDocumentFragment_clone
};

static const tid_t HTMLDocumentNode_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLDocument4_tid,
    IHTMLDocument5_tid,
    IDocumentSelector_tid,
    0
};

static void HTMLDocumentNode_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t document2_hooks[] = {
        {DISPID_IHTMLDOCUMENT2_URL,      NULL, L"URL"},
        {DISPID_IHTMLDOCUMENT2_LOCATION, HTMLDocumentNode_location_hook},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t document6_ie9_hooks[] = {
        {DISPID_IHTMLDOCUMENT6_ONSTORAGE},
        {DISPID_UNKNOWN}
    };

    HTMLDOMNode_init_dispex_info(info, mode);

    if(mode >= COMPAT_MODE_IE9) {
        dispex_info_add_interface(info, IHTMLDocument7_tid, NULL);
        dispex_info_add_interface(info, IDocumentEvent_tid, NULL);
    }

    /* Depending on compatibility version, we add interfaces in different order
     * so that the right getElementById implementation is used. */
    if(mode < COMPAT_MODE_IE8) {
        dispex_info_add_interface(info, IHTMLDocument3_tid, NULL);
        dispex_info_add_interface(info, IHTMLDocument6_tid, NULL);
    }else {
        dispex_info_add_interface(info, IHTMLDocument6_tid, mode >= COMPAT_MODE_IE9 ? document6_ie9_hooks : NULL);
        dispex_info_add_interface(info, IHTMLDocument3_tid, NULL);
    }
    dispex_info_add_interface(info, IHTMLDocument2_tid, document2_hooks);
}

static dispex_static_data_t HTMLDocumentNode_dispex = {
    L"HTMLDocument",
    &HTMLDocumentNode_event_target_vtbl.dispex_vtbl,
    DispHTMLDocument_tid,
    HTMLDocumentNode_iface_tids,
    HTMLDocumentNode_init_dispex_info
};

static HTMLDocumentNode *alloc_doc_node(HTMLDocumentObj *doc_obj, HTMLInnerWindow *window)
{
    HTMLDocumentNode *doc;

    doc = heap_alloc_zero(sizeof(HTMLDocumentNode));
    if(!doc)
        return NULL;

    doc->ref = 1;
    doc->basedoc.doc_node = doc;
    doc->basedoc.doc_obj = doc_obj;
    doc->basedoc.window = window ? window->base.outer_window : NULL;
    doc->window = window;

    init_doc(&doc->basedoc, (IUnknown*)&doc->node.IHTMLDOMNode_iface,
            &doc->node.event_target.dispex.IDispatchEx_iface);
    HTMLDocumentNode_SecMgr_Init(doc);

    list_init(&doc->selection_list);
    list_init(&doc->range_list);
    list_init(&doc->plugin_hosts);

    return doc;
}

HRESULT create_document_node(nsIDOMHTMLDocument *nsdoc, GeckoBrowser *browser, HTMLInnerWindow *window,
                             compat_mode_t parent_mode, HTMLDocumentNode **ret)
{
    HTMLDocumentObj *doc_obj = browser->doc;
    HTMLDocumentNode *doc;

    doc = alloc_doc_node(doc_obj, window);
    if(!doc)
        return E_OUTOFMEMORY;

    if(parent_mode >= COMPAT_MODE_IE9) {
        TRACE("using parent mode %u\n", parent_mode);
        doc->document_mode = parent_mode;
        lock_document_mode(doc);
    }

    if(!doc_obj->basedoc.window || (window && is_main_content_window(window->base.outer_window)))
        doc->basedoc.cp_container.forward_container = &doc_obj->basedoc.cp_container;

    HTMLDOMNode_Init(doc, &doc->node, (nsIDOMNode*)nsdoc, &HTMLDocumentNode_dispex);

    nsIDOMHTMLDocument_AddRef(nsdoc);
    doc->nsdoc = nsdoc;

    init_document_mutation(doc);
    doc_init_events(doc);

    doc->node.vtbl = &HTMLDocumentNodeImplVtbl;

    list_add_head(&browser->document_nodes, &doc->browser_entry);
    doc->browser = browser;

    if(browser->usermode == EDITMODE) {
        nsAString mode_str;
        nsresult nsres;

        nsAString_InitDepend(&mode_str, L"on");
        nsres = nsIDOMHTMLDocument_SetDesignMode(doc->nsdoc, &mode_str);
        nsAString_Finish(&mode_str);
        if(NS_FAILED(nsres))
            ERR("SetDesignMode failed: %08lx\n", nsres);
    }

    *ret = doc;
    return S_OK;
}

static HRESULT create_document_fragment(nsIDOMNode *nsnode, HTMLDocumentNode *doc_node, HTMLDocumentNode **ret)
{
    HTMLDocumentNode *doc_frag;

    doc_frag = alloc_doc_node(doc_node->basedoc.doc_obj, doc_node->window);
    if(!doc_frag)
        return E_OUTOFMEMORY;

    IHTMLWindow2_AddRef(&doc_frag->window->base.IHTMLWindow2_iface);

    HTMLDOMNode_Init(doc_node, &doc_frag->node, nsnode, &HTMLDocumentNode_dispex);
    doc_frag->node.vtbl = &HTMLDocumentFragmentImplVtbl;
    doc_frag->document_mode = lock_document_mode(doc_node);

    *ret = doc_frag;
    return S_OK;
}

HRESULT get_document_node(nsIDOMDocument *dom_document, HTMLDocumentNode **ret)
{
    HTMLDOMNode *node;
    HRESULT hres;

    hres = get_node((nsIDOMNode*)dom_document, FALSE, &node);
    if(FAILED(hres))
        return hres;

    if(!node) {
        ERR("document not initialized\n");
        return E_FAIL;
    }

    assert(node->vtbl == &HTMLDocumentNodeImplVtbl);
    *ret = impl_from_HTMLDOMNode(node);
    return S_OK;
}

static inline HTMLDocumentObj *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IUnknown_inner);
}

static HRESULT WINAPI HTMLDocumentObj_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_inner;
    }else if(htmldoc_qi(&This->basedoc, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_ICustomDoc, riid)) {
        *ppv = &This->ICustomDoc_iface;
    }else if(IsEqualGUID(&IID_IOleDocumentView, riid)) {
        *ppv = &This->IOleDocumentView_iface;
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObjectEx, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_ITargetContainer, riid)) {
        *ppv = &This->ITargetContainer_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        FIXME("Unimplemented interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLDocumentObj_AddRef(IUnknown *iface)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDocumentObj_Release(IUnknown *iface)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

    if(!ref) {
        if(This->basedoc.doc_node) {
            This->basedoc.doc_node->basedoc.doc_obj = NULL;
            htmldoc_release(&This->basedoc.doc_node->basedoc);
        }
        if(This->basedoc.window)
            IHTMLWindow2_Release(&This->basedoc.window->base.IHTMLWindow2_iface);
        if(This->advise_holder)
            IOleAdviseHolder_Release(This->advise_holder);

        if(This->view_sink)
            IAdviseSink_Release(This->view_sink);
        if(This->client)
            IOleObject_SetClientSite(&This->basedoc.IOleObject_iface, NULL);
        if(This->hostui)
            ICustomDoc_SetUIHandler(&This->ICustomDoc_iface, NULL);
        if(This->in_place_active)
            IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->basedoc.IOleInPlaceObjectWindowless_iface);
        if(This->ipsite)
            IOleDocumentView_SetInPlaceSite(&This->IOleDocumentView_iface, NULL);
        if(This->undomgr)
            IOleUndoManager_Release(This->undomgr);
        if(This->editsvcs)
            IHTMLEditServices_Release(This->editsvcs);
        if(This->tooltips_hwnd)
            DestroyWindow(This->tooltips_hwnd);

        if(This->hwnd)
            DestroyWindow(This->hwnd);
        heap_free(This->mime);

        remove_target_tasks(This->task_magic);
        ConnectionPointContainer_Destroy(&This->basedoc.cp_container);
        release_dispex(&This->dispex);

        if(This->nscontainer)
            detach_gecko_browser(This->nscontainer);
        heap_free(This);
    }

    return ref;
}

static const IUnknownVtbl HTMLDocumentObjVtbl = {
    HTMLDocumentObj_QueryInterface,
    HTMLDocumentObj_AddRef,
    HTMLDocumentObj_Release
};

/**********************************************************
 * ICustomDoc implementation
 */

static inline HTMLDocumentObj *impl_from_ICustomDoc(ICustomDoc *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, ICustomDoc_iface);
}

static HRESULT WINAPI CustomDoc_QueryInterface(ICustomDoc *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return htmldoc_query_interface(&This->basedoc, riid, ppv);
}

static ULONG WINAPI CustomDoc_AddRef(ICustomDoc *iface)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return htmldoc_addref(&This->basedoc);
}

static ULONG WINAPI CustomDoc_Release(ICustomDoc *iface)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return htmldoc_release(&This->basedoc);
}

static HRESULT WINAPI CustomDoc_SetUIHandler(ICustomDoc *iface, IDocHostUIHandler *pUIHandler)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pUIHandler);

    if(This->custom_hostui && This->hostui == pUIHandler)
        return S_OK;

    This->custom_hostui = TRUE;

    if(This->hostui)
        IDocHostUIHandler_Release(This->hostui);
    if(pUIHandler)
        IDocHostUIHandler_AddRef(pUIHandler);
    This->hostui = pUIHandler;
    if(!pUIHandler)
        return S_OK;

    hres = IDocHostUIHandler_QueryInterface(pUIHandler, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        FIXME("custom UI handler supports IOleCommandTarget\n");
        IOleCommandTarget_Release(cmdtrg);
    }

    return S_OK;
}

static const ICustomDocVtbl CustomDocVtbl = {
    CustomDoc_QueryInterface,
    CustomDoc_AddRef,
    CustomDoc_Release,
    CustomDoc_SetUIHandler
};

static HRESULT HTMLDocumentObj_location_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    return IHTMLDocument2_location_hook(&CONTAINING_RECORD(dispex, HTMLDocumentObj, dispex)->basedoc, flags, dp, res, ei, caller);
}

static const tid_t HTMLDocumentObj_iface_tids[] = {
    IHTMLDocument3_tid,
    IHTMLDocument4_tid,
    IHTMLDocument5_tid,
    0
};

static void HTMLDocumentObj_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t document2_hooks[] = {
        {DISPID_IHTMLDOCUMENT2_URL,      NULL, L"URL"},
        {DISPID_IHTMLDOCUMENT2_LOCATION, HTMLDocumentObj_location_hook},
        {DISPID_UNKNOWN}
    };
    dispex_info_add_interface(info, IHTMLDocument2_tid, document2_hooks);
}

static dispex_static_data_t HTMLDocumentObj_dispex = {
    L"HTMLDocumentObj",
    NULL,
    DispHTMLDocument_tid,
    HTMLDocumentObj_iface_tids,
    HTMLDocumentObj_init_dispex_info
};

static HRESULT create_document_object(BOOL is_mhtml, IUnknown *outer, REFIID riid, void **ppv)
{
    HTMLDocumentObj *doc;
    HRESULT hres;

    if(outer && !IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = NULL;
        return E_INVALIDARG;
    }

    /* ensure that security manager is initialized */
    if(!get_security_manager())
        return E_OUTOFMEMORY;

    doc = heap_alloc_zero(sizeof(HTMLDocumentObj));
    if(!doc)
        return E_OUTOFMEMORY;

    doc->ref = 1;
    doc->IUnknown_inner.lpVtbl = &HTMLDocumentObjVtbl;
    doc->ICustomDoc_iface.lpVtbl = &CustomDocVtbl;

    doc->basedoc.doc_obj = doc;

    init_dispatch(&doc->dispex, (IUnknown*)&doc->ICustomDoc_iface, &HTMLDocumentObj_dispex, COMPAT_MODE_QUIRKS);
    init_doc(&doc->basedoc, outer ? outer : &doc->IUnknown_inner, &doc->dispex.IDispatchEx_iface);
    TargetContainer_Init(doc);
    doc->is_mhtml = is_mhtml;

    doc->task_magic = get_task_target_magic();

    HTMLDocument_View_Init(doc);

    hres = create_gecko_browser(doc, &doc->nscontainer);
    if(FAILED(hres)) {
        ERR("Failed to init Gecko, returning CLASS_E_CLASSNOTAVAILABLE\n");
        htmldoc_release(&doc->basedoc);
        return hres;
    }

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &doc->IUnknown_inner;
    }else {
        hres = htmldoc_query_interface(&doc->basedoc, riid, ppv);
        htmldoc_release(&doc->basedoc);
        if(FAILED(hres))
            return hres;
    }

    doc->basedoc.window = doc->nscontainer->content_window;
    IHTMLWindow2_AddRef(&doc->basedoc.window->base.IHTMLWindow2_iface);

    if(!doc->basedoc.doc_node && doc->basedoc.window->base.inner_window->doc) {
        doc->basedoc.doc_node = doc->basedoc.window->base.inner_window->doc;
        htmldoc_addref(&doc->basedoc.doc_node->basedoc);
    }

    get_thread_hwnd();

    return S_OK;
}

HRESULT HTMLDocument_Create(IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_mshtml_guid(riid), ppv);
    return create_document_object(FALSE, outer, riid, ppv);
}

HRESULT MHTMLDocument_Create(IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_mshtml_guid(riid), ppv);
    return create_document_object(TRUE, outer, riid, ppv);
}
