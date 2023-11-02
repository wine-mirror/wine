/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "pluginhost.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLObjectElement {
    HTMLPluginContainer plugin_container;

    IHTMLObjectElement IHTMLObjectElement_iface;
    IHTMLObjectElement2 IHTMLObjectElement2_iface;

    nsIDOMHTMLObjectElement *nsobject;
};

static inline HTMLObjectElement *impl_from_IHTMLObjectElement(IHTMLObjectElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLObjectElement, IHTMLObjectElement_iface);
}

static HRESULT WINAPI HTMLObjectElement_QueryInterface(IHTMLObjectElement *iface,
        REFIID riid, void **ppv)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->plugin_container.element.node.IHTMLDOMNode_iface,
            riid, ppv);
}

static ULONG WINAPI HTMLObjectElement_AddRef(IHTMLObjectElement *iface)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    return IHTMLDOMNode_AddRef(&This->plugin_container.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLObjectElement_Release(IHTMLObjectElement *iface)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    return IHTMLDOMNode_Release(&This->plugin_container.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLObjectElement_GetTypeInfoCount(IHTMLObjectElement *iface, UINT *pctinfo)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLObjectElement_GetTypeInfo(IHTMLObjectElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    return IDispatchEx_GetTypeInfo(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLObjectElement_GetIDsOfNames(IHTMLObjectElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLObjectElement_Invoke(IHTMLObjectElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    return IDispatchEx_Invoke(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLObjectElement_get_object(IHTMLObjectElement *iface, IDispatch **p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_plugin_disp(&This->plugin_container, p);
}

static HRESULT WINAPI HTMLObjectElement_get_classid(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLObjectElement2_get_classid(&This->IHTMLObjectElement2_iface, p);
}

static HRESULT WINAPI HTMLObjectElement_get_data(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLObjectElement2_get_data(&This->IHTMLObjectElement2_iface, p);
}

static HRESULT WINAPI HTMLObjectElement_put_recordset(IHTMLObjectElement *iface, IDispatch *v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_recordset(IHTMLObjectElement *iface, IDispatch **p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_align(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_align(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_name(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLObjectElement_SetName(This->nsobject, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLObjectElement_get_name(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLObjectElement_GetName(This->nsobject, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLObjectElement_put_codeBase(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_codeBase(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_codeType(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_codeType(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_code(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_code(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_BaseHref(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_type(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_type(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_form(IHTMLObjectElement *iface, IHTMLFormElement **p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_width(IHTMLObjectElement *iface, VARIANT v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString width_str;
    PRUnichar buf[12];
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_I4: {
        swprintf(buf, ARRAY_SIZE(buf), L"%d", V_I4(&v));
        break;
    }
    default:
        FIXME("unimplemented for arg %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&width_str, buf);
    nsres = nsIDOMHTMLObjectElement_SetWidth(This->nsobject, &width_str);
    nsAString_Finish(&width_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetWidth failed: %08lx\n", nsres);
        return E_FAIL;
    }

    notif_container_change(&This->plugin_container, DISPID_UNKNOWN);
    return S_OK;
}

static HRESULT WINAPI HTMLObjectElement_get_width(IHTMLObjectElement *iface, VARIANT *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString width_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&width_str, NULL);
    nsres = nsIDOMHTMLObjectElement_GetWidth(This->nsobject, &width_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *width;

        nsAString_GetData(&width_str, &width);
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = SysAllocString(width);
        hres = V_BSTR(p) ? S_OK : E_OUTOFMEMORY;
    }else {
        ERR("GetWidth failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&width_str);
    return hres;
}

static HRESULT WINAPI HTMLObjectElement_put_height(IHTMLObjectElement *iface, VARIANT v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString height_str;
    PRUnichar buf[12];
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_I4: {
        swprintf(buf, ARRAY_SIZE(buf), L"%d", V_I4(&v));
        break;
    }
    default:
        FIXME("unimplemented for arg %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&height_str, buf);
    nsres = nsIDOMHTMLObjectElement_SetHeight(This->nsobject, &height_str);
    nsAString_Finish(&height_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetHeight failed: %08lx\n", nsres);
        return E_FAIL;
    }

    notif_container_change(&This->plugin_container, DISPID_UNKNOWN);
    return S_OK;
}

static HRESULT WINAPI HTMLObjectElement_get_height(IHTMLObjectElement *iface, VARIANT *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsAString height_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&height_str, NULL);
    nsres = nsIDOMHTMLObjectElement_GetHeight(This->nsobject, &height_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *height;

        nsAString_GetData(&height_str, &height);
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = SysAllocString(height);
        hres = V_BSTR(p) ? S_OK : E_OUTOFMEMORY;
    }else {
        ERR("GetHeight failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&height_str);
    return hres;
}

static HRESULT WINAPI HTMLObjectElement_get_readyState(IHTMLObjectElement *iface, LONG *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_onreadystatechange(IHTMLObjectElement *iface, VARIANT v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_onreadystatechange(IHTMLObjectElement *iface, VARIANT *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_onerror(IHTMLObjectElement *iface, VARIANT v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_onerror(IHTMLObjectElement *iface, VARIANT *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_altHtml(IHTMLObjectElement *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_altHtml(IHTMLObjectElement *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_put_vspace(IHTMLObjectElement *iface, LONG v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_vspace(IHTMLObjectElement *iface, LONG *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLObjectElement_GetVspace(This->nsobject, p);
    if(NS_FAILED(nsres)) {
        ERR("GetVspace failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLObjectElement_put_hspace(IHTMLObjectElement *iface, LONG v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement_get_hspace(IHTMLObjectElement *iface, LONG *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLObjectElementVtbl HTMLObjectElementVtbl = {
    HTMLObjectElement_QueryInterface,
    HTMLObjectElement_AddRef,
    HTMLObjectElement_Release,
    HTMLObjectElement_GetTypeInfoCount,
    HTMLObjectElement_GetTypeInfo,
    HTMLObjectElement_GetIDsOfNames,
    HTMLObjectElement_Invoke,
    HTMLObjectElement_get_object,
    HTMLObjectElement_get_classid,
    HTMLObjectElement_get_data,
    HTMLObjectElement_put_recordset,
    HTMLObjectElement_get_recordset,
    HTMLObjectElement_put_align,
    HTMLObjectElement_get_align,
    HTMLObjectElement_put_name,
    HTMLObjectElement_get_name,
    HTMLObjectElement_put_codeBase,
    HTMLObjectElement_get_codeBase,
    HTMLObjectElement_put_codeType,
    HTMLObjectElement_get_codeType,
    HTMLObjectElement_put_code,
    HTMLObjectElement_get_code,
    HTMLObjectElement_get_BaseHref,
    HTMLObjectElement_put_type,
    HTMLObjectElement_get_type,
    HTMLObjectElement_get_form,
    HTMLObjectElement_put_width,
    HTMLObjectElement_get_width,
    HTMLObjectElement_put_height,
    HTMLObjectElement_get_height,
    HTMLObjectElement_get_readyState,
    HTMLObjectElement_put_onreadystatechange,
    HTMLObjectElement_get_onreadystatechange,
    HTMLObjectElement_put_onerror,
    HTMLObjectElement_get_onerror,
    HTMLObjectElement_put_altHtml,
    HTMLObjectElement_get_altHtml,
    HTMLObjectElement_put_vspace,
    HTMLObjectElement_get_vspace,
    HTMLObjectElement_put_hspace,
    HTMLObjectElement_get_hspace
};

static inline HTMLObjectElement *impl_from_IHTMLObjectElement2(IHTMLObjectElement2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLObjectElement, IHTMLObjectElement2_iface);
}

static HRESULT WINAPI HTMLObjectElement2_QueryInterface(IHTMLObjectElement2 *iface,
        REFIID riid, void **ppv)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);

    return IHTMLDOMNode_QueryInterface(&This->plugin_container.element.node.IHTMLDOMNode_iface,
            riid, ppv);
}

static ULONG WINAPI HTMLObjectElement2_AddRef(IHTMLObjectElement2 *iface)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);

    return IHTMLDOMNode_AddRef(&This->plugin_container.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLObjectElement2_Release(IHTMLObjectElement2 *iface)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);

    return IHTMLDOMNode_Release(&This->plugin_container.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLObjectElement2_GetTypeInfoCount(IHTMLObjectElement2 *iface, UINT *pctinfo)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLObjectElement2_GetTypeInfo(IHTMLObjectElement2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    return IDispatchEx_GetTypeInfo(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLObjectElement2_GetIDsOfNames(IHTMLObjectElement2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    return IDispatchEx_GetIDsOfNames(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLObjectElement2_Invoke(IHTMLObjectElement2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    return IDispatchEx_Invoke(&This->plugin_container.element.node.event_target.dispex.IDispatchEx_iface,
            dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLObjectElement2_namedRecordset(IHTMLObjectElement2 *iface, BSTR dataMember,
        VARIANT *hierarchy, IDispatch **ppRecordset)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(dataMember), hierarchy, ppRecordset);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement2_put_classid(IHTMLObjectElement2 *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    HRESULT hres;

    FIXME("(%p)->(%s) semi-stub\n", This, debugstr_w(v));

    hres = elem_string_attr_setter(&This->plugin_container.element, L"classid", v);
    if(FAILED(hres))
        return hres;

    if(This->plugin_container.plugin_host) {
        FIXME("Host already associated.\n");
        return E_NOTIMPL;
    }

    /*
     * NOTE:
     * If the element is not yet in DOM tree, we should embed it as soon as it's added.
     * However, Gecko for some reason decides not to create NP plugin in this case,
     * so this won't work.
     */

    return create_plugin_host(This->plugin_container.element.node.doc, &This->plugin_container);
}

static HRESULT WINAPI HTMLObjectElement2_get_classid(IHTMLObjectElement2 *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement2_put_data(IHTMLObjectElement2 *iface, BSTR v)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLObjectElement2_get_data(IHTMLObjectElement2 *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_IHTMLObjectElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLObjectElement2Vtbl HTMLObjectElement2Vtbl = {
    HTMLObjectElement2_QueryInterface,
    HTMLObjectElement2_AddRef,
    HTMLObjectElement2_Release,
    HTMLObjectElement2_GetTypeInfoCount,
    HTMLObjectElement2_GetTypeInfo,
    HTMLObjectElement2_GetIDsOfNames,
    HTMLObjectElement2_Invoke,
    HTMLObjectElement2_namedRecordset,
    HTMLObjectElement2_put_classid,
    HTMLObjectElement2_get_classid,
    HTMLObjectElement2_put_data,
    HTMLObjectElement2_get_data
};

static inline HTMLObjectElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLObjectElement, plugin_container.element.node);
}

static HRESULT HTMLObjectElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLObjectElement *This = impl_from_HTMLDOMNode(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static inline HTMLObjectElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLObjectElement, plugin_container.element.node.event_target.dispex);
}

static void *HTMLObjectElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);
    void *elem_iface;

    if(IsEqualGUID(&IID_IHTMLObjectElement, riid))
        return &This->IHTMLObjectElement_iface;
    if(IsEqualGUID(&IID_IHTMLObjectElement2, riid))
        return &This->IHTMLObjectElement2_iface;
    if(IsEqualGUID(&IID_HTMLPluginContainer, riid)) {
        /* Special pseudo-interface returning HTMLPluginContainer struct. */
        return &This->plugin_container;
    }

    elem_iface = HTMLElement_query_interface(&This->plugin_container.element.node.event_target.dispex, riid);
    if(!elem_iface && This->plugin_container.plugin_host && This->plugin_container.plugin_host->plugin_unk) {
        IUnknown *plugin_iface, *ret;
        HRESULT hres = IUnknown_QueryInterface(This->plugin_container.plugin_host->plugin_unk, riid, (void**)&plugin_iface);

        if(hres == S_OK) {
            hres = wrap_iface(plugin_iface, (IUnknown*)&This->IHTMLObjectElement_iface, &ret);
            IUnknown_Release(plugin_iface);
            if(FAILED(hres)) {
                ERR("wrap_iface failed: %08lx\n", hres);
                return NULL;
            }

            TRACE("returning plugin iface %p wrapped to %p\n", plugin_iface, ret);
            return ret;
        }
    }

    return elem_iface;
}

static void HTMLObjectElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsobject)
        note_cc_edge((nsISupports*)This->nsobject, "nsobject", cb);
}

static void HTMLObjectElement_unlink(DispatchEx *dispex)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsobject);
}

static void HTMLObjectElement_destructor(DispatchEx *dispex)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);

    if(This->plugin_container.plugin_host)
        detach_plugin_host(This->plugin_container.plugin_host);

    HTMLElement_destructor(&This->plugin_container.element.node.event_target.dispex);
}

static HRESULT HTMLObjectElement_get_dispid(DispatchEx *dispex, BSTR name, DWORD grfdex, DISPID *dispid)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);

    TRACE("(%p)->(%s %lx %p)\n", This, debugstr_w(name), grfdex, dispid);

    return get_plugin_dispid(&This->plugin_container, name, dispid);
}

static HRESULT HTMLObjectElement_dispex_get_name(DispatchEx *dispex, DISPID id, BSTR *name)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);

    FIXME("(%p)->(%lx %p)\n", This, id, name);

    return E_NOTIMPL;
}

static HRESULT HTMLObjectElement_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLObjectElement *This = impl_from_DispatchEx(dispex);

    TRACE("(%p)->(%ld)\n", This, id);

    return invoke_plugin_prop(&This->plugin_container, id, lcid, flags, params, res, ei);
}

static const NodeImplVtbl HTMLObjectElementImplVtbl = {
    .clsid                 = &CLSID_HTMLObjectElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .get_readystate        = HTMLObjectElement_get_readystate,
};

static const event_target_vtbl_t HTMLObjectElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLObjectElement_query_interface,
        .destructor     = HTMLObjectElement_destructor,
        .traverse       = HTMLObjectElement_traverse,
        .unlink         = HTMLObjectElement_unlink,
        .get_dispid     = HTMLObjectElement_get_dispid,
        .get_name       = HTMLObjectElement_dispex_get_name,
        .invoke         = HTMLObjectElement_invoke
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLObjectElement_iface_tids[] = {
    IHTMLObjectElement2_tid,
    IHTMLObjectElement_tid,
    HTMLELEMENT_TIDS,
    0
};
static dispex_static_data_t HTMLObjectElement_dispex = {
    "HTMLObjectElement",
    &HTMLObjectElement_event_target_vtbl.dispex_vtbl,
    DispHTMLObjectElement_tid,
    HTMLObjectElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLObjectElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLObjectElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLObjectElement_iface.lpVtbl = &HTMLObjectElementVtbl;
    ret->IHTMLObjectElement2_iface.lpVtbl = &HTMLObjectElement2Vtbl;
    ret->plugin_container.element.node.vtbl = &HTMLObjectElementImplVtbl;

    HTMLElement_Init(&ret->plugin_container.element, doc, nselem, &HTMLObjectElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLObjectElement, (void**)&ret->nsobject);
    assert(nsres == NS_OK);

    *elem = &ret->plugin_container.element;
    return S_OK;
}

struct HTMLEmbed {
    HTMLElement element;

    IHTMLEmbedElement IHTMLEmbedElement_iface;
};

static inline HTMLEmbed *impl_from_IHTMLEmbedElement(IHTMLEmbedElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLEmbed, IHTMLEmbedElement_iface);
}

static HRESULT WINAPI HTMLEmbedElement_QueryInterface(IHTMLEmbedElement *iface,
        REFIID riid, void **ppv)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLEmbedElement_AddRef(IHTMLEmbedElement *iface)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLEmbedElement_Release(IHTMLEmbedElement *iface)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfoCount(IHTMLEmbedElement *iface, UINT *pctinfo)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfo(IHTMLEmbedElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetIDsOfNames(IHTMLEmbedElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLEmbedElement_Invoke(IHTMLEmbedElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLEmbedElement_put_hidden(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_hidden(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_palette(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_pluginspage(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_src(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_src(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_units(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_units(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_name(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_name(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_width(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_width(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_height(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_height(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbed *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLEmbedElementVtbl HTMLEmbedElementVtbl = {
    HTMLEmbedElement_QueryInterface,
    HTMLEmbedElement_AddRef,
    HTMLEmbedElement_Release,
    HTMLEmbedElement_GetTypeInfoCount,
    HTMLEmbedElement_GetTypeInfo,
    HTMLEmbedElement_GetIDsOfNames,
    HTMLEmbedElement_Invoke,
    HTMLEmbedElement_put_hidden,
    HTMLEmbedElement_get_hidden,
    HTMLEmbedElement_get_palette,
    HTMLEmbedElement_get_pluginspage,
    HTMLEmbedElement_put_src,
    HTMLEmbedElement_get_src,
    HTMLEmbedElement_put_units,
    HTMLEmbedElement_get_units,
    HTMLEmbedElement_put_name,
    HTMLEmbedElement_get_name,
    HTMLEmbedElement_put_width,
    HTMLEmbedElement_get_width,
    HTMLEmbedElement_put_height,
    HTMLEmbedElement_get_height
};

static inline HTMLEmbed *embed_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLEmbed, element.node.event_target.dispex);
}

static void *HTMLEmbedElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLEmbed *This = embed_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLEmbedElement, riid))
        return &This->IHTMLEmbedElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLEmbedElementImplVtbl = {
    .clsid                 = &CLSID_HTMLEmbed,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLEmbedElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLEmbedElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLEmbedElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLEmbedElement_tid,
    0
};
static dispex_static_data_t HTMLEmbedElement_dispex = {
    "HTMLEmbedElement",
    &HTMLEmbedElement_event_target_vtbl.dispex_vtbl,
    DispHTMLEmbed_tid,
    HTMLEmbedElement_iface_tids,
    HTMLElement_init_dispex_info
};

HRESULT HTMLEmbedElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLEmbed *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLEmbedElement_iface.lpVtbl = &HTMLEmbedElementVtbl;
    ret->element.node.vtbl = &HTMLEmbedElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLEmbedElement_dispex);
    *elem = &ret->element;
    return S_OK;
}
