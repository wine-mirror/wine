/*
 *    Node map implementation
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
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml2did.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    DispatchEx dispex;
    IXMLDOMNamedNodeMap IXMLDOMNamedNodeMap_iface;
    ISupportErrorInfo ISupportErrorInfo_iface;
    LONG refcount;

    struct domnode *node;
    LONG iterator;
    IEnumVARIANT *enumvariant;

    const struct nodemap_funcs *funcs;
} xmlnodemap;

static HRESULT nodemap_get_item(IUnknown *iface, LONG index, VARIANT *item)
{
    V_VT(item) = VT_DISPATCH;
    return IXMLDOMNamedNodeMap_get_item((IXMLDOMNamedNodeMap*)iface, index, (IXMLDOMNode**)&V_DISPATCH(item));
}

static const struct enumvariant_funcs nodemap_enumvariant = {
    nodemap_get_item,
    NULL
};

static inline xmlnodemap *impl_from_IXMLDOMNamedNodeMap( IXMLDOMNamedNodeMap *iface )
{
    return CONTAINING_RECORD(iface, xmlnodemap, IXMLDOMNamedNodeMap_iface);
}

static inline xmlnodemap *impl_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return CONTAINING_RECORD(iface, xmlnodemap, ISupportErrorInfo_iface);
}

static HRESULT WINAPI xmlnodemap_QueryInterface(IXMLDOMNamedNodeMap *iface, REFIID riid, void **obj)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID( riid, &IID_IUnknown) ||
        IsEqualGUID( riid, &IID_IDispatch) ||
        IsEqualGUID( riid, &IID_IXMLDOMNamedNodeMap))
    {
        *obj = iface;
    }
    else if (IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        if (!map->enumvariant)
        {
            HRESULT hr = create_enumvariant((IUnknown*)iface, FALSE, &nodemap_enumvariant, &map->enumvariant);
            if (FAILED(hr)) return hr;
        }

        return IEnumVARIANT_QueryInterface(map->enumvariant, &IID_IEnumVARIANT, obj);
    }
    else if (dispex_query_interface(&map->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else if (IsEqualGUID( riid, &IID_ISupportErrorInfo))
    {
        *obj = &map->ISupportErrorInfo_iface;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMNamedNodeMap_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI xmlnodemap_AddRef(IXMLDOMNamedNodeMap *iface)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    ULONG refcount = InterlockedIncrement(&map->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI xmlnodemap_Release(IXMLDOMNamedNodeMap *iface)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    ULONG refcount = InterlockedDecrement(&map->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        domnode_release(map->node);
        if (map->enumvariant)
            IEnumVARIANT_Release(map->enumvariant);
        free(map);
    }

    return refcount;
}

static HRESULT WINAPI xmlnodemap_GetTypeInfoCount(IXMLDOMNamedNodeMap *iface, UINT *count)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    return IDispatchEx_GetTypeInfoCount(&map->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI xmlnodemap_GetTypeInfo(IXMLDOMNamedNodeMap *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    return IDispatchEx_GetTypeInfo(&map->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI xmlnodemap_GetIDsOfNames(IXMLDOMNamedNodeMap *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    return IDispatchEx_GetIDsOfNames(&map->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI xmlnodemap_Invoke(IXMLDOMNamedNodeMap *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *puArgErr)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);
    return IDispatchEx_Invoke(&map->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags, params, result, ei, puArgErr);
}

static HRESULT WINAPI xmlnodemap_getNamedItem(IXMLDOMNamedNodeMap *iface, BSTR name, IXMLDOMNode **item)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), item);

    return map->funcs->get_named_item(map->node, name, item);
}

static HRESULT WINAPI xmlnodemap_setNamedItem(IXMLDOMNamedNodeMap *iface, IXMLDOMNode *newItem, IXMLDOMNode **namedItem)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %p, %p.\n", iface, newItem, namedItem );

    return map->funcs->set_named_item(map->node, newItem, namedItem);
}

static HRESULT WINAPI xmlnodemap_removeNamedItem(IXMLDOMNamedNodeMap *iface, BSTR name, IXMLDOMNode **namedItem)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_w(name), namedItem);

    return map->funcs->remove_named_item(map->node, name, namedItem);
}

static HRESULT WINAPI xmlnodemap_get_item(IXMLDOMNamedNodeMap *iface, LONG index, IXMLDOMNode **item)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %ld, %p.\n", iface, index, item);

    return map->funcs->get_item(map->node, index, item);
}

static HRESULT WINAPI xmlnodemap_get_length(IXMLDOMNamedNodeMap *iface, LONG *length)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %p.\n", iface, length);

    return map->funcs->get_length(map->node, length);
}

static HRESULT WINAPI xmlnodemap_getQualifiedItem(IXMLDOMNamedNodeMap *iface,
        BSTR baseName, BSTR namespaceURI, IXMLDOMNode **item)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_w(baseName), debugstr_w(namespaceURI), item);

    return map->funcs->get_qualified_item(map->node, baseName, namespaceURI, item);
}

static HRESULT WINAPI xmlnodemap_removeQualifiedItem(IXMLDOMNamedNodeMap *iface, BSTR baseName,
        BSTR namespaceURI, IXMLDOMNode **item)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_w(baseName), debugstr_w(namespaceURI), item);

    return map->funcs->remove_qualified_item(map->node, baseName, namespaceURI, item);
}

static HRESULT WINAPI xmlnodemap_nextNode(IXMLDOMNamedNodeMap *iface, IXMLDOMNode **nextItem)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %p, %ld.\n", iface, nextItem, map->iterator);

    return map->funcs->next_node(map->node, &map->iterator, nextItem);
}

static HRESULT WINAPI xmlnodemap_reset(IXMLDOMNamedNodeMap *iface)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap(iface);

    TRACE("%p, %ld.\n", iface, map->iterator);

    map->iterator = 0;

    return S_OK;
}

static HRESULT WINAPI xmlnodemap__newEnum(IXMLDOMNamedNodeMap *iface, IUnknown **enumv)
{
    TRACE("%p, %p.\n", iface, enumv);

    return create_enumvariant((IUnknown *)iface, TRUE, &nodemap_enumvariant, (IEnumVARIANT **)enumv);
}

static const struct IXMLDOMNamedNodeMapVtbl XMLDOMNamedNodeMapVtbl =
{
    xmlnodemap_QueryInterface,
    xmlnodemap_AddRef,
    xmlnodemap_Release,
    xmlnodemap_GetTypeInfoCount,
    xmlnodemap_GetTypeInfo,
    xmlnodemap_GetIDsOfNames,
    xmlnodemap_Invoke,
    xmlnodemap_getNamedItem,
    xmlnodemap_setNamedItem,
    xmlnodemap_removeNamedItem,
    xmlnodemap_get_item,
    xmlnodemap_get_length,
    xmlnodemap_getQualifiedItem,
    xmlnodemap_removeQualifiedItem,
    xmlnodemap_nextNode,
    xmlnodemap_reset,
    xmlnodemap__newEnum,
};

static HRESULT WINAPI support_error_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void** ppvObject )
{
    xmlnodemap *This = impl_from_ISupportErrorInfo( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);
    return IXMLDOMNamedNodeMap_QueryInterface(&This->IXMLDOMNamedNodeMap_iface, riid, ppvObject);
}

static ULONG WINAPI support_error_AddRef(
    ISupportErrorInfo *iface )
{
    xmlnodemap *This = impl_from_ISupportErrorInfo( iface );
    return IXMLDOMNamedNodeMap_AddRef(&This->IXMLDOMNamedNodeMap_iface);
}

static ULONG WINAPI support_error_Release(
    ISupportErrorInfo *iface )
{
    xmlnodemap *This = impl_from_ISupportErrorInfo( iface );
    return IXMLDOMNamedNodeMap_Release(&This->IXMLDOMNamedNodeMap_iface);
}

static HRESULT WINAPI support_error_InterfaceSupportsErrorInfo(
    ISupportErrorInfo *iface,
    REFIID riid )
{
    xmlnodemap *This = impl_from_ISupportErrorInfo( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));
    return IsEqualGUID(riid, &IID_IXMLDOMNamedNodeMap) ? S_OK : S_FALSE;
}

static const struct ISupportErrorInfoVtbl SupportErrorInfoVtbl =
{
    support_error_QueryInterface,
    support_error_AddRef,
    support_error_Release,
    support_error_InterfaceSupportsErrorInfo
};

static HRESULT xmlnodemap_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
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

static HRESULT xmlnodemap_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    xmlnodemap *map = impl_from_IXMLDOMNamedNodeMap((IXMLDOMNamedNodeMap *)iface);

    TRACE("%p, %ld, %lx, %x, %p, %p, %p.\n", iface, id, lcid, flags, params, res, ei);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = NULL;

    if (id < DISPID_DOM_COLLECTION_BASE || id > DISPID_DOM_COLLECTION_MAX)
        return DISP_E_UNKNOWNNAME;

    switch(flags)
    {
        case INVOKE_PROPERTYGET:
        {
            IXMLDOMNode *disp = NULL;

            IXMLDOMNamedNodeMap_get_item(&map->IXMLDOMNamedNodeMap_iface, id - DISPID_DOM_COLLECTION_BASE, &disp);
            V_DISPATCH(res) = (IDispatch*)disp;
            break;
        }
        default:
        {
            FIXME("unimplemented flags %x\n", flags);
            break;
        }
    }

    TRACE("ret %p\n", V_DISPATCH(res));

    return S_OK;
}

static const dispex_static_data_vtbl_t xmlnodemap_dispex_vtbl =
{
    xmlnodemap_get_dispid,
    xmlnodemap_invoke
};

static const tid_t xmlnodemap_iface_tids[] =
{
    IXMLDOMNamedNodeMap_tid,
    0
};

static dispex_static_data_t xmlnodemap_dispex =
{
    &xmlnodemap_dispex_vtbl,
    IXMLDOMNamedNodeMap_tid,
    NULL,
    xmlnodemap_iface_tids
};

HRESULT create_nodemap(struct domnode *node, const struct nodemap_funcs *funcs, IXMLDOMNamedNodeMap **map)
{
    xmlnodemap *object;

    *map = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMNamedNodeMap_iface.lpVtbl = &XMLDOMNamedNodeMapVtbl;
    object->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    object->refcount = 1;
    object->funcs = funcs;
    object->node = domnode_addref(node);

    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMNamedNodeMap_iface, &xmlnodemap_dispex);

    *map = &object->IXMLDOMNamedNodeMap_iface;

    return S_OK;
}
