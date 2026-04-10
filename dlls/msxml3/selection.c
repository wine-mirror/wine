/*
 *    XPath/XSLPattern query result node list implementation
 *
 * Copyright 2005 Mike McCormack
 * Copyright 2007 Mikolaj Zalewski
 * Copyright 2010 Adam Martinson for CodeWeavers
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
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlsave.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml2did.h"

#include "msxml_private.h"

#include "wine/debug.h"

/* This file implements the object returned by a XPath query. Note that this is
 * not the IXMLDOMNodeList returned by childNodes - it's implemented in nodelist.c.
 * They are different because the list returned by XPath queries:
 *  - is static - gives the results for the XML tree as it existed during the
 *    execution of the query
 *  - supports IXMLDOMSelection
 *
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

int registerNamespaces(struct domnode *node, xmlXPathContextPtr ctxt);
xmlChar* XSLPattern_to_XPath(xmlXPathContextPtr ctxt, xmlChar const* xslpat_str);

typedef struct
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    IUnknown *outer;
    BOOL own;

    LONG pos;

    const struct enumvariant_funcs *funcs;
} enumvariant;

struct selected_list
{
    struct domnode **nodes;
    LONG count;
    LONG pos;
};

typedef struct
{
    DispatchEx dispex;
    IXMLDOMSelection IXMLDOMSelection_iface;
    LONG refcount;
    struct domnode *node;

    struct selected_list result;

    IEnumVARIANT *enumvariant;
} domselection;

static HRESULT selection_get_item(IUnknown *iface, LONG index, VARIANT* item)
{
    V_VT(item) = VT_DISPATCH;
    return IXMLDOMSelection_get_item((IXMLDOMSelection*)iface, index, (IXMLDOMNode**)&V_DISPATCH(item));
}

static HRESULT selection_next(IUnknown *iface)
{
    IXMLDOMNode *node;
    HRESULT hr = IXMLDOMSelection_nextNode((IXMLDOMSelection*)iface, &node);
    if (hr == S_OK) IXMLDOMNode_Release(node);
    return hr;
}

static const struct enumvariant_funcs selection_enumvariant =
{
    selection_get_item,
    selection_next
};

static inline domselection *impl_from_IXMLDOMSelection( IXMLDOMSelection *iface )
{
    return CONTAINING_RECORD(iface, domselection, IXMLDOMSelection_iface);
}

static inline enumvariant *impl_from_IEnumVARIANT( IEnumVARIANT *iface )
{
    return CONTAINING_RECORD(iface, enumvariant, IEnumVARIANT_iface);
}

static HRESULT WINAPI domselection_QueryInterface(IXMLDOMSelection *iface, REFIID riid, void **obj)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (!obj)
        return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXMLDOMNodeList) ||
        IsEqualGUID(riid, &IID_IXMLDOMSelection))
    {
        *obj = &selection->IXMLDOMSelection_iface;
    }
    else if (IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        if (!selection->enumvariant)
        {
            HRESULT hr = create_enumvariant((IUnknown *)iface, FALSE, &selection_enumvariant, &selection->enumvariant);
            if (FAILED(hr)) return hr;
        }

        return IEnumVARIANT_QueryInterface(selection->enumvariant, &IID_IEnumVARIANT, obj);
    }
    else if (dispex_query_interface(&selection->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMSelection_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI domselection_AddRef(IXMLDOMSelection *iface)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    ULONG refcount = InterlockedIncrement(&selection->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI domselection_Release(IXMLDOMSelection *iface)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    ULONG refcount = InterlockedDecrement(&selection->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (selection->enumvariant)
            IEnumVARIANT_Release(selection->enumvariant);

        for (int i = 0; i < selection->result.count; ++i)
            domnode_release(selection->result.nodes[i]);
        free(selection->result.nodes);
        free(selection);
    }

    return refcount;
}

static HRESULT WINAPI domselection_GetTypeInfoCount(IXMLDOMSelection *iface, UINT *count)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    return IDispatchEx_GetTypeInfoCount(&selection->dispex.IDispatchEx_iface, count);
}

static HRESULT WINAPI domselection_GetTypeInfo(IXMLDOMSelection *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    return IDispatchEx_GetTypeInfo(&selection->dispex.IDispatchEx_iface, index, lcid, ti);
}

static HRESULT WINAPI domselection_GetIDsOfNames(IXMLDOMSelection *iface, REFIID riid, LPOLESTR *names,
        UINT name_count, LCID lcid, DISPID *dispid)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    return IDispatchEx_GetIDsOfNames(&selection->dispex.IDispatchEx_iface, riid, names, name_count, lcid, dispid);
}

static HRESULT WINAPI domselection_Invoke(IXMLDOMSelection *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *argerr)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);
    return IDispatchEx_Invoke(&selection->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, flags,
            params, result, ei, argerr);
}

static HRESULT WINAPI domselection_get_item(IXMLDOMSelection *iface, LONG index, IXMLDOMNode **item)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);

    TRACE("%p, %ld, %p.\n", iface, index, item);

    if (!item)
        return E_INVALIDARG;

    *item = NULL;

    if (index < 0 || index >= selection->result.count)
        return S_FALSE;

    selection->result.pos = index + 1;

    return create_node(selection->result.nodes[index], item);
}

static HRESULT WINAPI domselection_get_length(IXMLDOMSelection *iface, LONG *length)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);

    TRACE("%p, %p.\n", iface, length);

    if (!length)
        return E_INVALIDARG;

    *length = selection->result.count;
    return S_OK;
}

static HRESULT WINAPI domselection_nextNode(IXMLDOMSelection *iface, IXMLDOMNode **node)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);

    TRACE("%p, %p.\n", iface, node);

    if (!node)
        return E_INVALIDARG;

    *node = NULL;

    if (selection->result.pos >= selection->result.count)
        return S_FALSE;

    return create_node(selection->result.nodes[selection->result.pos++], node);
}

static HRESULT WINAPI domselection_reset(IXMLDOMSelection *iface)
{
    domselection *selection = impl_from_IXMLDOMSelection(iface);

    TRACE("%p.\n", iface);

    selection->result.pos = 0;
    return S_OK;
}

static HRESULT WINAPI domselection_get__newEnum(IXMLDOMSelection *iface, IUnknown **enumv)
{
    TRACE("%p, %p.\n", iface, enumv);

    return create_enumvariant((IUnknown *)iface, TRUE, &selection_enumvariant, (IEnumVARIANT**)enumv);
}

static HRESULT WINAPI domselection_get_expr(IXMLDOMSelection *iface, BSTR *p)
{
    FIXME("%p, %p.\n", iface, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_put_expr(IXMLDOMSelection *iface, BSTR p)
{
    FIXME("%p, %s.\n", iface, debugstr_w(p));

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_get_context(IXMLDOMSelection *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_putref_context(IXMLDOMSelection *iface, IXMLDOMNode *node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_peekNode(IXMLDOMSelection *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_matches(IXMLDOMSelection *iface, IXMLDOMNode *node, IXMLDOMNode **out_node)
{
    FIXME("%p, %p, %p.\n", iface, node, out_node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_removeNext(IXMLDOMSelection *iface, IXMLDOMNode **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_removeAll(IXMLDOMSelection *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_clone(IXMLDOMSelection *iface, IXMLDOMSelection **node)
{
    FIXME("%p, %p.\n", iface, node);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_getProperty(IXMLDOMSelection *iface, BSTR p, VARIANT *var)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_w(p), var);

    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_setProperty(IXMLDOMSelection *iface, BSTR p, VARIANT var)
{
    FIXME("%p, %s, %s.\n", iface, debugstr_w(p), debugstr_variant(&var));

    return E_NOTIMPL;
}

static const struct IXMLDOMSelectionVtbl domselection_vtbl =
{
    domselection_QueryInterface,
    domselection_AddRef,
    domselection_Release,
    domselection_GetTypeInfoCount,
    domselection_GetTypeInfo,
    domselection_GetIDsOfNames,
    domselection_Invoke,
    domselection_get_item,
    domselection_get_length,
    domselection_nextNode,
    domselection_reset,
    domselection_get__newEnum,
    domselection_get_expr,
    domselection_put_expr,
    domselection_get_context,
    domselection_putref_context,
    domselection_peekNode,
    domselection_matches,
    domselection_removeNext,
    domselection_removeAll,
    domselection_clone,
    domselection_getProperty,
    domselection_setProperty
};

static HRESULT WINAPI enumvariant_QueryInterface(
    IEnumVARIANT *iface,
    REFIID riid,
    void** ppvObject )
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID( riid, &IID_IUnknown ))
    {
        if (This->own)
            *ppvObject = &This->IEnumVARIANT_iface;
        else
            return IUnknown_QueryInterface(This->outer, riid, ppvObject);
    }
    else if (IsEqualGUID( riid, &IID_IEnumVARIANT ))
    {
        *ppvObject = &This->IEnumVARIANT_iface;
    }
    else
        return IUnknown_QueryInterface(This->outer, riid, ppvObject);

    IEnumVARIANT_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI enumvariant_AddRef(IEnumVARIANT *iface )
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI enumvariant_Release(IEnumVARIANT *iface)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        if (This->own) IUnknown_Release(This->outer);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI enumvariant_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ret_count = 0;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, var, fetched);

    if (fetched) *fetched = 0;

    if (celt && !var) return E_INVALIDARG;

    for (; celt > 0; celt--, var++, This->pos++)
    {
        HRESULT hr = This->funcs->get_item(This->outer, This->pos, var);
        if (hr != S_OK)
        {
            V_VT(var) = VT_EMPTY;
            break;
        }
        ret_count++;
    }

    if (fetched) *fetched = ret_count;

    /* we need to advance one step more for some reason */
    if (ret_count)
    {
        if (This->funcs->next)
            This->funcs->next(This->outer);
    }

    return celt == 0 ? S_OK : S_FALSE;
}

static HRESULT WINAPI enumvariant_Skip(IEnumVARIANT *iface, ULONG celt)
{
    FIXME("%p, %lu: stub\n", iface, celt);

    return E_NOTIMPL;
}

static HRESULT WINAPI enumvariant_Reset(IEnumVARIANT *iface)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );

    TRACE("%p\n", This);
    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI enumvariant_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppenum)
{
    FIXME("%p, %p: stub\n", iface, ppenum);

    return E_NOTIMPL;
}

static const struct IEnumVARIANTVtbl EnumVARIANTVtbl =
{
    enumvariant_QueryInterface,
    enumvariant_AddRef,
    enumvariant_Release,
    enumvariant_Next,
    enumvariant_Skip,
    enumvariant_Reset,
    enumvariant_Clone
};

HRESULT create_enumvariant(IUnknown *outer, BOOL own, const struct enumvariant_funcs *funcs, IEnumVARIANT **penum)
{
    enumvariant *This;

    This = malloc(sizeof(enumvariant));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumVARIANT_iface.lpVtbl = &EnumVARIANTVtbl;
    This->ref = 0;
    This->outer = outer;
    This->own = own;
    This->pos = 0;
    This->funcs = funcs;

    if (This->own)
        IUnknown_AddRef(This->outer);

    *penum = &This->IEnumVARIANT_iface;
    IEnumVARIANT_AddRef(*penum);
    return S_OK;
}

static HRESULT domselection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
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

static HRESULT domselection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    domselection *selection = impl_from_IXMLDOMSelection((IXMLDOMSelection *)iface);

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

            IXMLDOMSelection_get_item(&selection->IXMLDOMSelection_iface, id - DISPID_DOM_COLLECTION_BASE, &disp);
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

static const dispex_static_data_vtbl_t domselection_dispex_vtbl =
{
    domselection_get_dispid,
    domselection_invoke
};

static const tid_t domselection_iface_tids[] =
{
    IXMLDOMSelection_tid,
    0
};

static dispex_static_data_t domselection_dispex =
{
    &domselection_dispex_vtbl,
    IXMLDOMSelection_tid,
    NULL,
    domselection_iface_tids
};

#define XSLPATTERN_CHECK_ARGS(n) \
    if (nargs != n) { \
        FIXME("XSLPattern syntax error: Expected %i arguments, got %i\n", n, nargs); \
        xmlXPathSetArityError(pctx); \
        return; \
    }

static void XSLPattern_index(xmlXPathParserContextPtr pctx, int nargs)
{
    XSLPATTERN_CHECK_ARGS(0);

    xmlXPathPositionFunction(pctx, 0);
    xmlXPathReturnNumber(pctx, xmlXPathPopNumber(pctx) - 1.0);
}

static void XSLPattern_end(xmlXPathParserContextPtr pctx, int nargs)
{
    double pos, last;
    XSLPATTERN_CHECK_ARGS(0);

    xmlXPathPositionFunction(pctx, 0);
    pos = xmlXPathPopNumber(pctx);
    xmlXPathLastFunction(pctx, 0);
    last = xmlXPathPopNumber(pctx);
    xmlXPathReturnBoolean(pctx, pos == last);
}

static void XSLPattern_nodeType(xmlXPathParserContextPtr pctx, int nargs)
{
    XSLPATTERN_CHECK_ARGS(0);
    xmlXPathReturnNumber(pctx, pctx->context->node->type);
}

static void XSLPattern_OP_IEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) == 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_INEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) != 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_ILt(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) < 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_ILEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) <= 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_IGt(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) > 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_IGEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) >= 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void query_serror(void* ctx, const xmlError* err)
{
    LIBXML2_CALLBACK_SERROR(domselection_create, err);
}

static HRESULT select_nodes(struct domnode *node, BSTR query, bool xpath, struct selected_list *list)
{
    xmlXPathObjectPtr result;
    xmlXPathContextPtr ctxt;
    xmlNodePtr xmlnode;
    xmlChar *xmlquery;
    xmlDocPtr xmldoc;
    HRESULT hr;

    xmlquery = xmlchar_from_wchar(query);
    xmldoc = create_xmldoc_from_domdoc(node, &xmlnode);

    ctxt = xmlXPathNewContext(xmldoc);
    ctxt->node = xmlnode;

    ctxt->error = query_serror;
    registerNamespaces(node, ctxt);
    xmlXPathContextSetCache(ctxt, 1, -1, 0);

    if (xpath)
    {
        xmlXPathRegisterAllFunctions(ctxt);
        result = xmlXPathEvalExpression(xmlquery, ctxt);
    }
    else
    {
        xmlChar* pattern_query = XSLPattern_to_XPath(ctxt, xmlquery);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"not", xmlXPathNotFunction);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"boolean", xmlXPathBooleanFunction);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"index", XSLPattern_index);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"end", XSLPattern_end);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"nodeType", XSLPattern_nodeType);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IEq", XSLPattern_OP_IEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_INEq", XSLPattern_OP_INEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_ILt", XSLPattern_OP_ILt);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_ILEq", XSLPattern_OP_ILEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IGt", XSLPattern_OP_IGt);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IGEq", XSLPattern_OP_IGEq);

        result = xmlXPathEvalExpression(pattern_query, ctxt);
        xmlFree(pattern_query);
    }

    if (result && result->type == XPATH_NODESET)
    {
        list->count = xmlXPathNodeSetGetLength(result->nodesetval);
        list->nodes = calloc(list->count, sizeof(*list->nodes));

        for (int i = 0; i < list->count; ++i)
        {
            xmlnode = xmlXPathNodeSetItem(result->nodesetval, i);

            if (xmlnode->type == XML_NAMESPACE_DECL)
            {
                FIXME("Not implemented for namespace nodes.\n");
                list->count--;
                continue;
            }
            else
            {
                node = xmlnode->_private2;
            }

            list->nodes[i] = domnode_addref(node);
        }

        TRACE("found %ld matches\n", list->count);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(ctxt);
    xmlFreeDoc(xmldoc);
    free(xmlquery);

    return hr;
}

HRESULT create_selection(struct domnode *node, BSTR query, bool xpath, IXMLDOMNodeList **out)
{
    domselection *object;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", node, debugstr_w(query), out);

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = select_nodes(node, query, xpath, &object->result)))
    {
        free(object);
        return hr;
    }

    object->IXMLDOMSelection_iface.lpVtbl = &domselection_vtbl;
    object->refcount = 1;
    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMSelection_iface, &domselection_dispex);

    *out = (IXMLDOMNodeList *)&object->IXMLDOMSelection_iface;

    return S_OK;
}

HRESULT create_selection_from_nodeset(xmlXPathObjectPtr result, IXMLDOMNodeList **out)
{
    struct domnode *node;
    xmlNodePtr xmlnode;
    domselection *obj;

    TRACE("%p, %p.\n", result, out);

    *out = NULL;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    obj->IXMLDOMSelection_iface.lpVtbl = &domselection_vtbl;
    obj->refcount = 1;
    init_dispex(&obj->dispex, (IUnknown *)&obj->IXMLDOMSelection_iface, &domselection_dispex);

    if (result && result->type == XPATH_NODESET)
    {
        obj->result.count = xmlXPathNodeSetGetLength(result->nodesetval);
        obj->result.nodes = calloc(obj->result.count, sizeof(*obj->result.nodes));

        for (int i = 0; i < obj->result.count; ++i)
        {
            xmlnode = xmlXPathNodeSetItem(result->nodesetval, i);

            if (xmlnode->type == XML_NAMESPACE_DECL)
            {
                FIXME("Not implemented for namespace nodes.\n");
                obj->result.count--;
                continue;
            }
            else if (!xmlnode->_private2)
            {
                WARN("Skipping untracked node.\n");
                obj->result.count--;
                continue;
            }
            else
            {
                node = xmlnode->_private2;
                obj->result.nodes[i] = domnode_addref(node);
            }
        }
    }

    *out = (IXMLDOMNodeList *)&obj->IXMLDOMSelection_iface;

    return S_OK;
}
