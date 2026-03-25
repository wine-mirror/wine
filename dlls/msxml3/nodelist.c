/*
 *    Node list implementation
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml2did.h"

#include "msxml_private.h"

#include "wine/debug.h"

/* This file implements the object returned by childNodes property. Note that this is
 * not the IXMLDOMNodeList returned by XPath queries - it's implemented in selection.c.
 * They are different because the list returned by childNodes:
 *  - is "live" - changes to the XML tree are automatically reflected in the list
 *  - doesn't supports IXMLDOMSelection
 *  - note that an attribute node have a text child in DOM but not in the XPath data model
 *    thus the child is inaccessible by an XPath query
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    DispatchEx dispex;
    IXMLDOMNodeList IXMLDOMNodeList_iface;
    LONG refcount;
    struct domnode *parent;
    struct domnode *current;
    IEnumVARIANT *enumvariant;
} xmlnodelist;

static HRESULT nodelist_get_item(IUnknown *iface, LONG index, VARIANT *item)
{
    V_VT(item) = VT_DISPATCH;
    return IXMLDOMNodeList_get_item((IXMLDOMNodeList*)iface, index, (IXMLDOMNode**)&V_DISPATCH(item));
}

static const struct enumvariant_funcs nodelist_enumvariant = {
    nodelist_get_item,
    NULL
};

static inline xmlnodelist *impl_from_IXMLDOMNodeList( IXMLDOMNodeList *iface )
{
    return CONTAINING_RECORD(iface, xmlnodelist, IXMLDOMNodeList_iface);
}

static HRESULT WINAPI xmlnodelist_QueryInterface(IXMLDOMNodeList *iface, REFIID riid, void **ppvObject)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) )
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID( riid, &IID_IEnumVARIANT ))
    {
        if (!list->enumvariant)
        {
            HRESULT hr = create_enumvariant((IUnknown*)iface, FALSE, &nodelist_enumvariant, &list->enumvariant);
            if (FAILED(hr)) return hr;
        }

        return IEnumVARIANT_QueryInterface(list->enumvariant, &IID_IEnumVARIANT, ppvObject);
    }
    else if (dispex_query_interface(&list->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMNodeList_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlnodelist_AddRef(IXMLDOMNodeList *iface)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    ULONG refcount = InterlockedIncrement(&list->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void xmlnode_list_release_current(xmlnodelist *list)
{
    if (list->current && list->current != list->parent)
        domnode_release(list->current);
    list->current = NULL;
}

static ULONG WINAPI xmlnodelist_Release(IXMLDOMNodeList *iface)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    ULONG refcount = InterlockedDecrement(&list->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        xmlnode_list_release_current(list);
        domnode_release(list->parent);
        if (list->enumvariant) IEnumVARIANT_Release(list->enumvariant);
        free(list);
    }

    return refcount;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfoCount(IXMLDOMNodeList *iface, UINT *count)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    return IDispatchEx_GetTypeInfoCount(&list->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI xmlnodelist_GetTypeInfo(IXMLDOMNodeList *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    return IDispatchEx_GetTypeInfo(&list->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI xmlnodelist_GetIDsOfNames(IXMLDOMNodeList *iface, REFIID riid,
        LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    return IDispatchEx_GetIDsOfNames(&list->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI xmlnodelist_Invoke(IXMLDOMNodeList *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result,
        EXCEPINFO *ei, UINT *puArgErr)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    return IDispatchEx_Invoke(&list->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            flags, params, result, ei, puArgErr);
}

static HRESULT WINAPI xmlnodelist_get_item(IXMLDOMNodeList *iface, LONG index, IXMLDOMNode **node)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    struct list *p;
    LONG i = 0;

    TRACE("%p, %ld, %p.\n", iface, index, node);

    if (!node)
        return E_INVALIDARG;

    *node = NULL;

    if (index < 0)
        return S_FALSE;

    p = list_head(&list->parent->children);
    while (p)
    {
        if (i++ == index) break;
        p = list_next(&list->parent->children, p);
    }
    if (!p)
        return S_FALSE;

    return create_node(LIST_ENTRY(p, struct domnode, entry), node);
}

static HRESULT WINAPI xmlnodelist_get_length(IXMLDOMNodeList *iface, LONG *length)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);

    TRACE("%p, %p.\n", iface, length);

    if (!length)
        return E_INVALIDARG;

    *length = list_count(&list->parent->children);
    return S_OK;
}

static HRESULT WINAPI xmlnodelist_nextNode(IXMLDOMNodeList *iface, IXMLDOMNode **node)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, node);

    if (!node)
        return E_INVALIDARG;

    *node = NULL;

    /* First iteration */
    if (list->current == list->parent)
    {
        list->current = domnode_get_first_child(list->parent);
        if (list->current) domnode_addref(list->current);
    }

    if (!list->current)
        return S_FALSE;

    hr = create_node(list->current, node);
    domnode_release(list->current);
    list->current = domnode_get_next_sibling(list->current);
    if (list->current) domnode_addref(list->current);

    return hr;
}

static HRESULT WINAPI xmlnodelist_reset(IXMLDOMNodeList *iface)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList(iface);

    TRACE("%p.\n", iface);

    xmlnode_list_release_current(list);
    list->current = list->parent;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist__newEnum(IXMLDOMNodeList *iface, IUnknown **enumv)
{
    TRACE("%p, %p.\n", iface, enumv);

    return create_enumvariant((IUnknown*)iface, TRUE, &nodelist_enumvariant, (IEnumVARIANT**)enumv);
}

static const struct IXMLDOMNodeListVtbl xmlnodelist_vtbl =
{
    xmlnodelist_QueryInterface,
    xmlnodelist_AddRef,
    xmlnodelist_Release,
    xmlnodelist_GetTypeInfoCount,
    xmlnodelist_GetTypeInfo,
    xmlnodelist_GetIDsOfNames,
    xmlnodelist_Invoke,
    xmlnodelist_get_item,
    xmlnodelist_get_length,
    xmlnodelist_nextNode,
    xmlnodelist_reset,
    xmlnodelist__newEnum,
};

static HRESULT xmlnodelist_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    WCHAR *ptr;
    int idx = 0;

    for(ptr = name; *ptr >= '0' && *ptr <= '9'; ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_DOM_COLLECTION_BASE + idx;
    TRACE("ret %lx\n", *dispid);
    return S_OK;
}

static HRESULT xmlnodelist_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    xmlnodelist *list = impl_from_IXMLDOMNodeList((IXMLDOMNodeList *)iface);

    TRACE("%p, %ld, %lx, %x, %p, %p, %p.\n", iface, id, lcid, flags, params, res, ei);

    if (id >= DISPID_DOM_COLLECTION_BASE && id <= DISPID_DOM_COLLECTION_MAX)
    {
        switch(flags)
        {
            case DISPATCH_PROPERTYGET:
            {
                IXMLDOMNode *disp = NULL;

                V_VT(res) = VT_DISPATCH;
                IXMLDOMNodeList_get_item(&list->IXMLDOMNodeList_iface, id - DISPID_DOM_COLLECTION_BASE, &disp);
                V_DISPATCH(res) = (IDispatch*)disp;
                break;
            }
            default:
            {
                FIXME("unimplemented flags %x\n", flags);
                break;
            }
        }
    }
    else if (id == DISPID_VALUE)
    {
        switch(flags)
        {
            case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
            case DISPATCH_PROPERTYGET:
            case DISPATCH_METHOD:
            {
                IXMLDOMNode *item;
                VARIANT index;
                HRESULT hr;

                if (params->cArgs - params->cNamedArgs != 1) return DISP_E_BADPARAMCOUNT;

                VariantInit(&index);
                hr = VariantChangeType(&index, params->rgvarg, 0, VT_I4);
                if(FAILED(hr))
                {
                    FIXME("failed to convert arg, %s\n", debugstr_variant(params->rgvarg));
                    return hr;
                }

                IXMLDOMNodeList_get_item(&list->IXMLDOMNodeList_iface, V_I4(&index), &item);
                V_VT(res) = VT_DISPATCH;
                V_DISPATCH(res) = (IDispatch*)item;
                break;
            }
            default:
            {
                FIXME("DISPID_VALUE: unimplemented flags %x\n", flags);
                break;
            }
        }
    }
    else
        return DISP_E_UNKNOWNNAME;

    TRACE("ret %p\n", V_DISPATCH(res));

    return S_OK;
}

static const dispex_static_data_vtbl_t xmlnodelist_dispex_vtbl =
{
    xmlnodelist_get_dispid,
    xmlnodelist_invoke
};

static const tid_t xmlnodelist_iface_tids[] =
{
    IXMLDOMNodeList_tid,
    0
};

static dispex_static_data_t xmlnodelist_dispex =
{
    &xmlnodelist_dispex_vtbl,
    IXMLDOMNodeList_tid,
    NULL,
    xmlnodelist_iface_tids
};

HRESULT create_children_nodelist(struct domnode *node, IXMLDOMNodeList **list)
{
    xmlnodelist *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMNodeList_iface.lpVtbl = &xmlnodelist_vtbl;
    object->refcount = 1;
    object->current = object->parent = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMNodeList_iface, &xmlnodelist_dispex);

    *list = &object->IXMLDOMNodeList_iface;

    return S_OK;
}
