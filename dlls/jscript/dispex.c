/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "jscript.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef enum {
    PROP_VARIANT,
    PROP_BUILTIN,
    PROP_PROTREF,
    PROP_DELETED
} prop_type_t;

struct _dispex_prop_t {
    WCHAR *name;
    prop_type_t type;
    DWORD flags;

    union {
        VARIANT var;
        const builtin_prop_t *p;
        DWORD ref;
    } u;
};

static inline DISPID prop_to_id(DispatchEx *This, dispex_prop_t *prop)
{
    return prop - This->props;
}

static const builtin_prop_t *find_builtin_prop(DispatchEx *This, const WCHAR *name)
{
    int min = 0, max, i, r;

    max = This->builtin_info->props_cnt-1;
    while(min <= max) {
        i = (min+max)/2;

        r = strcmpW(name, This->builtin_info->props[i].name);
        if(!r)
            return This->builtin_info->props + i;

        if(r < 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

static dispex_prop_t *alloc_prop(DispatchEx *This, const WCHAR *name, prop_type_t type, DWORD flags)
{
    dispex_prop_t *ret;

    if(This->buf_size == This->prop_cnt) {
        dispex_prop_t *tmp = heap_realloc(This->props, (This->buf_size<<=1)*sizeof(*This->props));
        if(!tmp)
            return NULL;
        This->props = tmp;
    }

    ret = This->props + This->prop_cnt++;
    ret->type = type;
    ret->flags = flags;
    ret->name = heap_strdupW(name);
    if(!ret->name)
        return NULL;

    return ret;
}

static dispex_prop_t *alloc_protref(DispatchEx *This, const WCHAR *name, DWORD ref)
{
    dispex_prop_t *ret;

    ret = alloc_prop(This, name, PROP_PROTREF, 0);
    if(!ret)
        return NULL;

    ret->u.ref = ref;
    return ret;
}

static HRESULT find_prop_name(DispatchEx *This, const WCHAR *name, dispex_prop_t **ret)
{
    const builtin_prop_t *builtin;
    dispex_prop_t *prop;

    for(prop = This->props; prop < This->props+This->prop_cnt; prop++) {
        if(prop->name && !strcmpW(prop->name, name)) {
            *ret = prop;
            return S_OK;
        }
    }

    builtin = find_builtin_prop(This, name);
    if(builtin) {
        prop = alloc_prop(This, name, PROP_BUILTIN, builtin->flags);
        if(!prop)
            return E_OUTOFMEMORY;

        prop->u.p = builtin;
        *ret = prop;
        return S_OK;
    }

    *ret = NULL;
    return S_OK;
}

static HRESULT find_prop_name_prot(DispatchEx *This, const WCHAR *name, BOOL alloc, dispex_prop_t **ret)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = find_prop_name(This, name, &prop);
    if(FAILED(hres))
        return hres;
    if(prop) {
        *ret = prop;
        return S_OK;
    }

    if(This->prototype) {
        hres = find_prop_name_prot(This->prototype, name, FALSE, &prop);
        if(FAILED(hres))
            return hres;
        if(prop) {
            prop = alloc_protref(This, prop->name, prop - This->prototype->props);
            if(!prop)
                return E_OUTOFMEMORY;
            *ret = prop;
            return S_OK;
        }
    }

    if(alloc) {
        TRACE("creating prop %s\n", debugstr_w(name));

        prop = alloc_prop(This, name, PROP_VARIANT, PROPF_ENUM);
        if(!prop)
            return E_OUTOFMEMORY;
        VariantInit(&prop->u.var);
    }

    *ret = prop;
    return S_OK;
}

#define DISPATCHEX_THIS(iface) DEFINE_THIS(DispatchEx, IDispatchEx, iface)

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = _IDispatchEx_(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = _IDispatchEx_(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = _IDispatchEx_(This);
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        dispex_prop_t *prop;

        for(prop = This->props; prop < This->props+This->prop_cnt; prop++) {
            if(prop->type == PROP_VARIANT)
                VariantClear(&prop->u.var);
            heap_free(prop->name);
        }
        heap_free(This->props);
        script_release(This->ctx);

        if(This->builtin_info->destructor)
            This->builtin_info->destructor(This);
        else
            heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        hres = IDispatchEx_GetDispID(_IDispatchEx_(This), rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(_IDispatchEx_(This), dispIdMember, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    dispex_prop_t *prop;
    HRESULT hres;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameEnsure|fdexNameImplicit)) {
        FIXME("Unsupported grfdex %x\n", grfdex);
        return E_NOTIMPL;
    }

    hres = find_prop_name_prot(This, bstrName, (grfdex&fdexNameEnsure) != 0, &prop);
    if(FAILED(hres))
        return hres;
    if(prop) {
        *pid = prop_to_id(This, prop);
        return S_OK;
    }

    TRACE("not found %s\n", debugstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %x %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

#undef DISPATCHEX_THIS

static IDispatchExVtbl DispatchExVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static HRESULT jsdisp_set_prot_prop(DispatchEx *dispex, DispatchEx *prototype)
{
    VARIANT *var;

    if(!dispex->props[1].name)
        return E_OUTOFMEMORY;

    dispex->props[1].type = PROP_VARIANT;
    dispex->props[1].flags = 0;

    var = &dispex->props[1].u.var;
    V_VT(var) = VT_DISPATCH;
    V_DISPATCH(var) = (IDispatch*)_IDispatchEx_(prototype);

    return S_OK;
}

static HRESULT init_dispex(DispatchEx *dispex, script_ctx_t *ctx, const builtin_info_t *builtin_info, DispatchEx *prototype)
{
    static const WCHAR prototypeW[] = {'p','r','o','t','o','t','y','p','e',0};

    TRACE("%p (%p)\n", dispex, prototype);

    dispex->lpIDispatchExVtbl = &DispatchExVtbl;
    dispex->ref = 1;
    dispex->builtin_info = builtin_info;

    dispex->props = heap_alloc((dispex->buf_size=4) * sizeof(dispex_prop_t));
    if(!dispex->props)
        return E_OUTOFMEMORY;

    dispex->prototype = prototype;
    if(prototype)
        IDispatchEx_AddRef(_IDispatchEx_(prototype));

    dispex->prop_cnt = 2;
    dispex->props[0].name = NULL;
    dispex->props[0].flags = 0;
    if(builtin_info->value_prop.invoke) {
        dispex->props[0].type = PROP_BUILTIN;
        dispex->props[0].u.p = &builtin_info->value_prop;
    }else {
        dispex->props[0].type = PROP_DELETED;
    }

    dispex->props[1].type = PROP_DELETED;
    dispex->props[1].name = SysAllocString(prototypeW);
    dispex->props[1].flags = 0;

    if(prototype) {
        HRESULT hres;

        hres = jsdisp_set_prot_prop(dispex, prototype);
        if(FAILED(hres)) {
            IDispatchEx_Release(_IDispatchEx_(dispex));
            return hres;
        }
    }

    script_addref(ctx);
    dispex->ctx = ctx;

    return S_OK;
}

static const builtin_info_t dispex_info = {
    JSCLASS_NONE,
    {NULL, NULL, 0},
    0, NULL,
    NULL,
    NULL
};

HRESULT create_dispex(script_ctx_t *ctx, const builtin_info_t *builtin_info, DispatchEx *prototype, DispatchEx **dispex)
{
    DispatchEx *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(DispatchEx));
    if(!ret)
        return E_OUTOFMEMORY;

    hres = init_dispex(ret, ctx, builtin_info ? builtin_info : &dispex_info, prototype);
    if(FAILED(hres))
        return hres;

    *dispex = ret;
    return S_OK;
}
