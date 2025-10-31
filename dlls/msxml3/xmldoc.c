/*
 * XML Document implementation
 *
 * Copyright 2007 James Hawkins
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
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "wininet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "ocidl.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static HRESULT XMLElementCollection_create( xmlNodePtr node, LPVOID *ppObj );

/**********************************************************************
 * IXMLElement2
 */
typedef struct _xmlelem
{
    IXMLElement2 IXMLElement2_iface;
    LONG ref;
    xmlNodePtr node;
    BOOL own;
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
        if (This->own) xmlFreeNode(This->node);
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
    xmlelem *This = impl_from_IXMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    if (*This->node->name) {
        *p = bstr_from_xmlChar(This->node->name);
        CharUpperBuffW(*p, SysStringLen(*p));
    }else {
        *p = NULL;
    }

    TRACE("returning %s\n", debugstr_w(*p));

    return S_OK;
}

static HRESULT WINAPI xmlelem_put_tagName(IXMLElement2 *iface, BSTR p)
{
    xmlelem *This = impl_from_IXMLElement2(iface);

    FIXME("(%p)->(%s): stub\n", This, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT XMLElement_create(xmlNodePtr node, LPVOID *ppObj, BOOL own);

static HRESULT WINAPI xmlelem_get_parent(IXMLElement2 *iface, IXMLElement2 **parent)
{
    xmlelem *This = impl_from_IXMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, parent);

    if (!parent)
        return E_INVALIDARG;

    *parent = NULL;

    if (!This->node->parent)
        return S_FALSE;

    return XMLElement_create(This->node->parent, (LPVOID *)parent, FALSE);
}

static HRESULT WINAPI xmlelem_setAttribute(IXMLElement2 *iface, BSTR strPropertyName,
                                            VARIANT PropertyValue)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlChar *name, *value;
    xmlAttrPtr attr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(strPropertyName), debugstr_variant(&PropertyValue));

    if (!strPropertyName || V_VT(&PropertyValue) != VT_BSTR)
        return E_INVALIDARG;

    name = xmlchar_from_wchar(strPropertyName);
    value = xmlchar_from_wchar(V_BSTR(&PropertyValue));
    attr = xmlSetProp(This->node, name, value);

    free(name);
    free(value);
    return (attr) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_getAttribute(IXMLElement2 *iface, BSTR name,
    VARIANT *value)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlChar *val = NULL;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(name), value);

    if (!value)
        return E_INVALIDARG;

    VariantInit(value);
    V_BSTR(value) = NULL;

    if (!name)
        return E_INVALIDARG;

    /* case for xml:lang attribute */
    if (!lstrcmpiW(name, L"xml:lang"))
    {
        xmlNsPtr ns;
        ns = xmlSearchNs(This->node->doc, This->node, (xmlChar*)"xml");
        val = xmlGetNsProp(This->node, (xmlChar*)"lang", ns->href);
    }
    else
    {
        xmlAttrPtr attr;
        xmlChar *xml_name;

        xml_name = xmlchar_from_wchar(name);
        attr = This->node->properties;
        while (attr)
        {
            BSTR attr_name;

            attr_name = bstr_from_xmlChar(attr->name);
            if (!lstrcmpiW(name, attr_name))
            {
                val = xmlNodeListGetString(attr->doc, attr->children, 1);
                SysFreeString(attr_name);
                break;
            }

            attr = attr->next;
            SysFreeString(attr_name);
        }

        free(xml_name);
    }

    if (val)
    {
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar(val);
    }

    xmlFree(val);
    TRACE("returning %s\n", debugstr_w(V_BSTR(value)));
    return (val) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_removeAttribute(IXMLElement2 *iface, BSTR strPropertyName)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlChar *name;
    xmlAttrPtr attr;
    int res;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%s)\n", This, debugstr_w(strPropertyName));

    if (!strPropertyName)
        return E_INVALIDARG;

    name = xmlchar_from_wchar(strPropertyName);
    attr = xmlHasProp(This->node, name);
    if (!attr)
        goto done;

    res = xmlRemoveProp(attr);

    if (res == 0)
        hr = S_OK;

done:
    free(name);
    return hr;
}

static HRESULT WINAPI xmlelem_get_children(IXMLElement2 *iface, IXMLElementCollection **p)
{
    xmlelem *This = impl_from_IXMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    return XMLElementCollection_create(This->node, (LPVOID *)p);
}

static LONG type_libxml_to_msxml(xmlElementType type)
{
    switch (type)
    {
        case XML_ELEMENT_NODE:
            return XMLELEMTYPE_ELEMENT;
        case XML_TEXT_NODE:
            return XMLELEMTYPE_TEXT;
        case XML_COMMENT_NODE:
            return XMLELEMTYPE_COMMENT;
        case XML_DOCUMENT_NODE:
            return XMLELEMTYPE_DOCUMENT;
        case XML_DTD_NODE:
            return XMLELEMTYPE_DTD;
        case XML_PI_NODE:
            return XMLELEMTYPE_PI;
        default:
            break;
    }

    return XMLELEMTYPE_OTHER;
}

static HRESULT WINAPI xmlelem_get_type(IXMLElement2 *iface, LONG *p)
{
    xmlelem *This = impl_from_IXMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    *p = type_libxml_to_msxml(This->node->type);
    TRACE("returning %ld\n", *p);
    return S_OK;
}

static HRESULT WINAPI xmlelem_get_text(IXMLElement2 *iface, BSTR *p)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlChar *content;

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    content = xmlNodeGetContent(This->node);
    *p = bstr_from_xmlChar(content);
    TRACE("returning %s\n", debugstr_w(*p));

    xmlFree(content);
    return S_OK;
}

static HRESULT WINAPI xmlelem_put_text(IXMLElement2 *iface, BSTR p)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlChar *content;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    /* FIXME: test which types can be used */
    if (This->node->type == XML_ELEMENT_NODE)
        return E_NOTIMPL;

    content = xmlchar_from_wchar(p);
    xmlNodeSetContent(This->node, content);

    free(content);

    return S_OK;
}

static HRESULT WINAPI xmlelem_addChild(IXMLElement2 *iface, IXMLElement2 *pChildElem,
                                       LONG lIndex, LONG lreserved)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlelem *childElem = impl_from_IXMLElement2(pChildElem);
    xmlNodePtr child;

    TRACE("%p, %p, %ld, %ld.\n", iface, pChildElem, lIndex, lreserved);

    if (lIndex == 0)
        child = xmlAddChild(This->node, childElem->node);
    else
        child = xmlAddNextSibling(This->node, childElem->node->last);

    /* parent is responsible for child data */
    if (child) childElem->own = FALSE;

    return (child) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_removeChild(IXMLElement2 *iface, IXMLElement2 *pChildElem)
{
    xmlelem *This = impl_from_IXMLElement2(iface);
    xmlelem *childElem = impl_from_IXMLElement2(pChildElem);

    TRACE("(%p)->(%p)\n", This, childElem);

    if (!pChildElem)
        return E_INVALIDARG;

    /* only supported for This is childElem parent case */
    if (This->node != childElem->node->parent)
        return E_INVALIDARG;

    xmlUnlinkNode(childElem->node);
    /* standalone element now */
    childElem->own = TRUE;

    return S_OK;
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

static HRESULT XMLElement_create(xmlNodePtr node, LPVOID *ppObj, BOOL own)
{
    xmlelem *elem;

    TRACE("(%p)\n", ppObj);

    if (!ppObj)
        return E_INVALIDARG;

    *ppObj = NULL;

    elem = malloc(sizeof(*elem));
    if(!elem)
        return E_OUTOFMEMORY;

    elem->IXMLElement2_iface.lpVtbl = &xmlelem_vtbl;
    elem->ref = 1;
    elem->node = node;
    elem->own  = own;

    *ppObj = &elem->IXMLElement2_iface;

    TRACE("returning iface %p\n", *ppObj);
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
    xmlNodePtr node;

    /* IEnumVARIANT members */
    xmlNodePtr current;
} xmlelem_collection;

static inline LONG xmlelem_collection_updatelength(xmlelem_collection *collection)
{
    xmlNodePtr ptr = collection->node->children;

    collection->length = 0;
    while (ptr)
    {
        collection->length++;
        ptr = ptr->next;
    }
    return collection->length;
}

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
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        free(This);
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
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    *p = xmlelem_collection_updatelength(This);
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
                                              VARIANT var2, IDispatch **ppDisp)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    xmlNodePtr ptr = This->node->children;
    int index, i;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&var1), debugstr_variant(&var2), ppDisp);

    if (!ppDisp)
        return E_INVALIDARG;

    *ppDisp = NULL;

    index = V_I4(&var1);
    if (index < 0)
        return E_INVALIDARG;

    xmlelem_collection_updatelength(This);
    if (index >= This->length)
        return E_FAIL;

    for (i = 0; i < index; i++)
        ptr = ptr->next;

    return XMLElement_create(ptr, (LPVOID *)ppDisp, FALSE);
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
    IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *fetched)
{
    xmlelem_collection *This = impl_from_IEnumVARIANT(iface);
    HRESULT hr;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, rgVar, fetched);

    if (!rgVar)
        return E_INVALIDARG;

    if (fetched) *fetched = 0;

    if (!This->current)
    {
        V_VT(rgVar) = VT_EMPTY;
        return S_FALSE;
    }

    while (celt > 0 && This->current)
    {
        V_VT(rgVar) = VT_DISPATCH;
        hr = XMLElement_create(This->current, (void **)&V_DISPATCH(rgVar), FALSE);
        if (FAILED(hr)) return hr;
        This->current = This->current->next;
        if (fetched) ++*fetched;
        rgVar++;
        celt--;
    }
    if (!celt) return S_OK;
    V_VT(rgVar) = VT_EMPTY;
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
    xmlelem_collection *This = impl_from_IEnumVARIANT(iface);
    TRACE("(%p)\n", This);
    This->current = This->node->children;
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

static HRESULT XMLElementCollection_create(xmlNodePtr node, LPVOID *ppObj)
{
    xmlelem_collection *collection;

    TRACE("(%p)\n", ppObj);

    *ppObj = NULL;

    if (!node->children)
        return S_FALSE;

    collection = malloc(sizeof(*collection));
    if(!collection)
        return E_OUTOFMEMORY;

    collection->IXMLElementCollection_iface.lpVtbl = &xmlelem_collection_vtbl;
    collection->IEnumVARIANT_iface.lpVtbl = &xmlelem_collection_IEnumVARIANTvtbl;
    collection->ref = 1;
    collection->length = 0;
    collection->node = node;
    collection->current = node->children;
    xmlelem_collection_updatelength(collection);

    *ppObj = &collection->IXMLElementCollection_iface;

    TRACE("returning iface %p\n", *ppObj);
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
    HRESULT error;

    /* IXMLDocument */
    xmlDocPtr xmldoc;

    /* IPersistStream */
    IStream *stream;
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
    xmldoc *This = impl_from_IXMLDocument2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (ref == 0)
    {
        xmlFreeDoc(This->xmldoc);
        if (This->stream) IStream_Release(This->stream);
        free(This);
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
    xmldoc *This = impl_from_IXMLDocument2(iface);
    xmlNodePtr root;

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = NULL;

    if (!(root = xmlDocGetRootElement(This->xmldoc)))
        return E_FAIL;

    return XMLElement_create(root, (LPVOID *)p, FALSE);
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

typedef struct {
    IBindStatusCallback IBindStatusCallback_iface;
} bsc;

static HRESULT WINAPI bsc_QueryInterface(
    IBindStatusCallback *iface,
    REFIID riid,
    LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    TRACE("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI bsc_AddRef(
    IBindStatusCallback *iface )
{
    return 2;
}

static ULONG WINAPI bsc_Release(
    IBindStatusCallback *iface )
{
    return 1;
}

static HRESULT WINAPI bsc_OnStartBinding(
        IBindStatusCallback* iface,
        DWORD dwReserved,
        IBinding* pib)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetPriority(
        IBindStatusCallback* iface,
        LONG* pnPriority)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnLowResource(
        IBindStatusCallback* iface,
        DWORD reserved)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnProgress(
        IBindStatusCallback* iface,
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnStopBinding(
        IBindStatusCallback* iface,
        HRESULT hresult,
        LPCWSTR szError)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetBindInfo(
        IBindStatusCallback* iface,
        DWORD* grfBINDF,
        BINDINFO* pbindinfo)
{
    *grfBINDF = BINDF_RESYNCHRONIZE;

    return S_OK;
}

static HRESULT WINAPI bsc_OnDataAvailable(
        IBindStatusCallback* iface,
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC* pformatetc,
        STGMEDIUM* pstgmed)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnObjectAvailable(
        IBindStatusCallback* iface,
        REFIID riid,
        IUnknown* punk)
{
    return S_OK;
}

static const struct IBindStatusCallbackVtbl bsc_vtbl =
{
    bsc_QueryInterface,
    bsc_AddRef,
    bsc_Release,
    bsc_OnStartBinding,
    bsc_GetPriority,
    bsc_OnLowResource,
    bsc_OnProgress,
    bsc_OnStopBinding,
    bsc_GetBindInfo,
    bsc_OnDataAvailable,
    bsc_OnObjectAvailable
};

static bsc xmldoc_bsc = { { &bsc_vtbl } };

static HRESULT WINAPI xmldoc_put_URL(IXMLDocument2 *iface, BSTR p)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    IStream *stream;
    IBindCtx *bctx;
    IMoniker *moniker;
    IPersistStreamInit *persist;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    if (!PathIsURLW(p))
    {
        WCHAR fullpath[MAX_PATH];
        DWORD needed = ARRAY_SIZE(url);

        if (!PathSearchAndQualifyW(p, fullpath, ARRAY_SIZE(fullpath)))
        {
            ERR("can't find path\n");
            return E_FAIL;
        }

        if (FAILED(UrlCreateFromPathW(fullpath, url, &needed, 0)))
        {
            ERR("can't create url from path\n");
            return E_FAIL;
        }

        p = url;
    }

    hr = CreateURLMoniker(NULL, p, &moniker);
    if (FAILED(hr))
        return hr;

    CreateAsyncBindCtx(0, &xmldoc_bsc.IBindStatusCallback_iface, 0, &bctx);

    hr = IMoniker_BindToStorage(moniker, bctx, NULL, &IID_IStream, (LPVOID *)&stream);
    IBindCtx_Release(bctx);
    IMoniker_Release(moniker);
    if (FAILED(hr))
        return hr;

    hr = IXMLDocument2_QueryInterface(iface, &IID_IPersistStreamInit, (LPVOID *)&persist);
    if (FAILED(hr))
    {
        IStream_Release(stream);
        return hr;
    }

    hr = IPersistStreamInit_Load(persist, stream);
    IPersistStreamInit_Release(persist);
    IStream_Release(stream);

    return hr;
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
    xmldoc *This = impl_from_IXMLDocument2(iface);
    xmlDtd *dtd;

    TRACE("(%p, %p)\n", This, p);

    if (!p) return E_INVALIDARG;

    dtd = xmlGetIntSubset(This->xmldoc);
    if (!dtd) return S_FALSE;

    *p = bstr_from_xmlChar(dtd->name);
    CharUpperBuffW(*p, SysStringLen(*p));

    return S_OK;
}

static HRESULT WINAPI xmldoc_get_dtdURl(IXMLDocument2 *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static xmlElementType type_msxml_to_libxml(LONG type)
{
    switch (type)
    {
        case XMLELEMTYPE_ELEMENT:
            return XML_ELEMENT_NODE;
        case XMLELEMTYPE_TEXT:
            return XML_TEXT_NODE;
        case XMLELEMTYPE_COMMENT:
            return XML_COMMENT_NODE;
        case XMLELEMTYPE_DOCUMENT:
            return XML_DOCUMENT_NODE;
        case XMLELEMTYPE_DTD:
            return XML_DTD_NODE;
        case XMLELEMTYPE_PI:
            return XML_PI_NODE;
        default:
            break;
    }

    return -1; /* FIXME: what is OTHER in msxml? */
}

static HRESULT WINAPI xmldoc_createElement(IXMLDocument2 *iface, VARIANT vType,
                                           VARIANT var1, IXMLElement2 **ppElem)
{
    xmlNodePtr node;
    static const xmlChar empty[] = "\0";

    TRACE("(%p)->(%s %s %p)\n", iface, debugstr_variant(&vType),
        debugstr_variant(&var1), ppElem);

    if (!ppElem)
        return E_INVALIDARG;

    *ppElem = NULL;

    if (V_VT(&vType) != VT_I4)
        return E_INVALIDARG;

    if(type_msxml_to_libxml(V_I4(&vType)) == -1)
        return E_NOTIMPL;

    node = xmlNewNode(NULL, empty);
    node->type = type_msxml_to_libxml(V_I4(&vType));

    /* FIXME: create xmlNodePtr based on vType and var1 */
    return XMLElement_create(node, (LPVOID *)ppElem, TRUE);
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
    xmldoc_get_dtdURl,
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

static xmlDocPtr parse_xml(char *ptr, int len)
{
    return xmlReadMemory(ptr, len, NULL, NULL,
                         XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS);
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_Load(
    IPersistStreamInit *iface, LPSTREAM pStm)
{
    xmldoc *This = impl_from_IPersistStreamInit(iface);
    HRESULT hr;
    HGLOBAL hglobal;
    DWORD read, written, len;
    BYTE buf[4096];
    char *ptr;

    TRACE("(%p, %p)\n", iface, pStm);

    if (!pStm)
        return E_INVALIDARG;

    /* release previously allocated stream */
    if (This->stream) IStream_Release(This->stream);
    hr = CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
    if (FAILED(hr))
        return hr;

    do
    {
        IStream_Read(pStm, buf, sizeof(buf), &read);
        hr = IStream_Write(This->stream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    if (FAILED(hr))
    {
        ERR("Failed to copy stream\n");
        return hr;
    }

    hr = GetHGlobalFromStream(This->stream, &hglobal);
    if (FAILED(hr))
        return hr;

    len = GlobalSize(hglobal);
    ptr = GlobalLock(hglobal);
    if (len != 0)
    {
        xmlFreeDoc(This->xmldoc);
        This->xmldoc = parse_xml(ptr, len);
    }
    GlobalUnlock(hglobal);

    if (!This->xmldoc)
    {
        ERR("Failed to parse xml\n");
        return E_FAIL;
    }

    return S_OK;
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

    doc = malloc(sizeof(*doc));
    if(!doc)
        return E_OUTOFMEMORY;

    doc->IXMLDocument2_iface.lpVtbl = &xmldoc_vtbl;
    doc->IPersistStreamInit_iface.lpVtbl = &xmldoc_IPersistStreamInit_VTable;
    doc->ref = 1;
    doc->error = S_OK;
    doc->xmldoc = NULL;
    doc->stream = NULL;

    *ppObj = &doc->IXMLDocument2_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
