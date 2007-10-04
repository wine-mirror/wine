/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLInputElementVtbl *lpHTMLInputElementVtbl;

    nsIDOMHTMLInputElement *nsinput;
} HTMLInputElement;

#define HTMLINPUT(x)  ((IHTMLInputElement*)  &(x)->lpHTMLInputElementVtbl)

#define HTMLINPUT_THIS(iface) DEFINE_THIS(HTMLInputElement, HTMLInputElement, iface)

static HRESULT WINAPI HTMLInputElement_QueryInterface(IHTMLInputElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLInputElement_AddRef(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLInputElement_Release(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfoCount(IHTMLInputElement *iface, UINT *pctinfo)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfo(IHTMLInputElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_GetIDsOfNames(IHTMLInputElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_Invoke(IHTMLInputElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_type(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_type(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString type_str;
    const PRUnichar *type;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetType(This->nsinput, &type_str);

    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&type_str, &type, NULL);
        *p = SysAllocString(type);
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }

    nsAString_Finish(&type_str);

    TRACE("type=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_value(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_value(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString value_str;
    const PRUnichar *value = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLInputElement_GetValue(This->nsinput, &value_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value, NULL);
        *p = SysAllocString(value);
    }else {
        ERR("GetValue failed: %08x\n", nsres);
    }

    nsAString_Finish(&value_str);

    TRACE("value=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_name(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_name(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString name_str;
    const PRUnichar *name;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);

    nsres = nsIDOMHTMLInputElement_GetName(This->nsinput, &name_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&name_str, &name, NULL);
        *p = SysAllocString(name);
    }else {
        ERR("GetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Finish(&name_str);

    TRACE("name=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_status(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_status(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_disabled(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_disabled(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_form(IHTMLInputElement *iface, IHTMLFormElement **p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_size(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_size(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_maxLength(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_maxLength(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_select(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onchange(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onchange(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onselect(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onselect(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultValue(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_defaultValue(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_readOnly(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_readOnly(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_createTextRange(IHTMLInputElement *iface, IHTMLTxtRange **range)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_checked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_checked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    PRBool checked;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetChecked(This->nsinput, &checked);
    if(NS_FAILED(nsres)) {
        ERR("GetChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = checked ? VARIANT_TRUE : VARIANT_FALSE;
    TRACE("checked=%x\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_border(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_border(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vspace(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vspace(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_hspace(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_hspace(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_alt(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_alt(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_src(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_src(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_lowsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_lowsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vrml(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vrml(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_dynsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_dynsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_readyState(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_complete(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_loop(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_loop(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_align(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_align(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onload(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onload(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onerror(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onerror(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onabort(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onabort(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_width(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_width(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_height(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_height(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_start(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_start(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLINPUT_THIS

static const IHTMLInputElementVtbl HTMLInputElementVtbl = {
    HTMLInputElement_QueryInterface,
    HTMLInputElement_AddRef,
    HTMLInputElement_Release,
    HTMLInputElement_GetTypeInfoCount,
    HTMLInputElement_GetTypeInfo,
    HTMLInputElement_GetIDsOfNames,
    HTMLInputElement_Invoke,
    HTMLInputElement_put_type,
    HTMLInputElement_get_type,
    HTMLInputElement_put_value,
    HTMLInputElement_get_value,
    HTMLInputElement_put_name,
    HTMLInputElement_get_name,
    HTMLInputElement_put_status,
    HTMLInputElement_get_status,
    HTMLInputElement_put_disabled,
    HTMLInputElement_get_disabled,
    HTMLInputElement_get_form,
    HTMLInputElement_put_size,
    HTMLInputElement_get_size,
    HTMLInputElement_put_maxLength,
    HTMLInputElement_get_maxLength,
    HTMLInputElement_select,
    HTMLInputElement_put_onchange,
    HTMLInputElement_get_onchange,
    HTMLInputElement_put_onselect,
    HTMLInputElement_get_onselect,
    HTMLInputElement_put_defaultValue,
    HTMLInputElement_get_defaultValue,
    HTMLInputElement_put_readOnly,
    HTMLInputElement_get_readOnly,
    HTMLInputElement_createTextRange,
    HTMLInputElement_put_indeterminate,
    HTMLInputElement_get_indeterminate,
    HTMLInputElement_put_defaultChecked,
    HTMLInputElement_get_defaultChecked,
    HTMLInputElement_put_checked,
    HTMLInputElement_get_checked,
    HTMLInputElement_put_border,
    HTMLInputElement_get_border,
    HTMLInputElement_put_vspace,
    HTMLInputElement_get_vspace,
    HTMLInputElement_put_hspace,
    HTMLInputElement_get_hspace,
    HTMLInputElement_put_alt,
    HTMLInputElement_get_alt,
    HTMLInputElement_put_src,
    HTMLInputElement_get_src,
    HTMLInputElement_put_lowsrc,
    HTMLInputElement_get_lowsrc,
    HTMLInputElement_put_vrml,
    HTMLInputElement_get_vrml,
    HTMLInputElement_put_dynsrc,
    HTMLInputElement_get_dynsrc,
    HTMLInputElement_get_readyState,
    HTMLInputElement_get_complete,
    HTMLInputElement_put_loop,
    HTMLInputElement_get_loop,
    HTMLInputElement_put_align,
    HTMLInputElement_get_align,
    HTMLInputElement_put_onload,
    HTMLInputElement_get_onload,
    HTMLInputElement_put_onerror,
    HTMLInputElement_get_onerror,
    HTMLInputElement_put_onabort,
    HTMLInputElement_get_onabort,
    HTMLInputElement_put_width,
    HTMLInputElement_get_width,
    HTMLInputElement_put_height,
    HTMLInputElement_get_height,
    HTMLInputElement_put_start,
    HTMLInputElement_get_start
};

#define HTMLINPUT_NODE_THIS(iface) DEFINE_THIS2(HTMLInputElement, element.node, iface)

static HRESULT HTMLInputElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLInputElement *This = HTMLINPUT_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }else if(IsEqualGUID(&IID_IHTMLInputElement, riid)) {
        TRACE("(%p)->(IID_IHTMLInputElement %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLInputElement_destructor(HTMLDOMNode *iface)
{
    HTMLInputElement *This = HTMLINPUT_NODE_THIS(iface);

    nsIDOMHTMLInputElement_Release(This->nsinput);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLINPUT_NODE_THIS

static const NodeImplVtbl HTMLInputElementImplVtbl = {
    HTMLInputElement_QI,
    HTMLInputElement_destructor
};

HTMLElement *HTMLInputElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLInputElement *ret = mshtml_alloc(sizeof(HTMLInputElement));
    nsresult nsres;

    ret->lpHTMLInputElementVtbl = &HTMLInputElementVtbl;
    ret->element.node.vtbl = &HTMLInputElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLInputElement,
                                             (void**)&ret->nsinput);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLInputElement interface: %08x\n", nsres);

    return &ret->element;
}
