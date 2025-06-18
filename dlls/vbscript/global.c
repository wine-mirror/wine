/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include <assert.h>
#include <math.h>

#include "vbscript.h"
#include "vbscript_defs.h"

#include "mshtmhst.h"
#include "objsafe.h"
#include "wchar.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#define VB_E_CANNOT_CREATE_OBJ 0x800a01ad
#define VB_E_MK_PARSE_ERROR    0x800a01b0

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

#define BP_GET      1
#define BP_GETPUT   2

typedef struct {
    UINT16 len;
    WCHAR buf[7];
} string_constant_t;

struct _builtin_prop_t {
    const WCHAR *name;
    HRESULT (*proc)(BuiltinDisp*,VARIANT*,unsigned,VARIANT*);
    DWORD flags;
    unsigned min_args;
    UINT_PTR max_args;
};

static inline BuiltinDisp *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, BuiltinDisp, IDispatch_iface);
}

static HRESULT WINAPI Builtin_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else {
        if(!IsEqualGUID(riid, &IID_IDispatchEx))
            WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Builtin_AddRef(IDispatch *iface)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI Builtin_Release(IDispatch *iface)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        assert(!This->ctx);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI Builtin_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 0;
    return S_OK;
}

static HRESULT WINAPI Builtin_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return DISP_E_BADINDEX;
}

HRESULT get_builtin_id(BuiltinDisp *disp, const WCHAR *name, DISPID *id)
{
    size_t min = 1, max = disp->member_cnt - 1, i;
    int r;

    while(min <= max) {
        i = (min + max) / 2;
        r = wcsicmp(disp->members[i].name, name);
        if(!r) {
            *id = i;
            return S_OK;
        }
        if(r < 0)
            min = i+1;
        else
            max = i-1;
    }

    return DISP_E_MEMBERNOTFOUND;

}

static HRESULT WINAPI Builtin_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names, UINT name_cnt,
                                            LCID lcid, DISPID *ids)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    unsigned i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), names, name_cnt, lcid, ids);

    if(!This->ctx) {
        FIXME("NULL context\n");
        return E_UNEXPECTED;
    }

    for(i = 0; i < name_cnt; i++) {
        hres = get_builtin_id(This, names[i], &ids[i]);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI Builtin_Invoke(IDispatch *iface, DISPID id, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, UINT *err)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    const builtin_prop_t *prop;
    VARIANT args_buf[8], *args;
    unsigned argn, i;
    HRESULT hres;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, id, debugstr_guid(riid), lcid, flags, dp, res, ei, err);

    if(!This->ctx) {
        FIXME("NULL context\n");
        return E_UNEXPECTED;
    }

    if(id >= This->member_cnt || (!This->members[id].proc && !This->members[id].flags))
        return DISP_E_MEMBERNOTFOUND;
    prop = This->members + id;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        if(!(prop->flags & (BP_GET|BP_GETPUT))) {
            FIXME("property does not support DISPATCH_PROPERTYGET\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
        if(!prop->proc && prop->flags == BP_GET) {
            const int vt = prop->min_args, val = prop->max_args;
            switch(vt) {
            case VT_I2:
                V_VT(res) = VT_I2;
                V_I2(res) = val;
                break;
            case VT_I4:
                V_VT(res) = VT_I4;
                V_I4(res) = val;
                break;
            case VT_BSTR: {
                const string_constant_t *str = (const string_constant_t*)prop->max_args;
                BSTR ret;

                ret = SysAllocStringLen(str->buf, str->len);
                if(!ret)
                    return E_OUTOFMEMORY;

                V_VT(res) = VT_BSTR;
                V_BSTR(res) = ret;
                break;
            }
            DEFAULT_UNREACHABLE;
            }
            return S_OK;
        }
        break;
    case DISPATCH_METHOD:
        if(prop->flags & (BP_GET|BP_GETPUT)) {
            FIXME("Call on property\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYPUT:
        if(!(prop->flags & BP_GETPUT)) {
            FIXME("property does not support DISPATCH_PROPERTYPUT\n");
            return E_FAIL;
        }

        FIXME("call put\n");
        return E_NOTIMPL;
    default:
        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    argn = arg_cnt(dp);

    if(argn < prop->min_args || argn > (prop->max_args ? prop->max_args : prop->min_args)) {
        WARN("invalid number of arguments\n");
        return MAKE_VBSERROR(VBSE_FUNC_ARITY_MISMATCH);
    }

    if(argn <= ARRAY_SIZE(args_buf)) {
        args = args_buf;
    }else {
        args = malloc(argn * sizeof(*args));
        if(!args)
            return E_OUTOFMEMORY;
    }

    for(i=0; i < argn; i++) {
        if(V_VT(dp->rgvarg+dp->cArgs-i-1) == (VT_BYREF|VT_VARIANT))
            args[i] = *V_VARIANTREF(dp->rgvarg+dp->cArgs-i-1);
        else
            args[i] = dp->rgvarg[dp->cArgs-i-1];
    }

    hres = prop->proc(This, args, dp->cArgs, res);
    if(args != args_buf)
        free(args);
    return hres;
}

static const IDispatchVtbl BuiltinDispVtbl = {
    Builtin_QueryInterface,
    Builtin_AddRef,
    Builtin_Release,
    Builtin_GetTypeInfoCount,
    Builtin_GetTypeInfo,
    Builtin_GetIDsOfNames,
    Builtin_Invoke
};

static HRESULT create_builtin_dispatch(script_ctx_t *ctx, const builtin_prop_t *members, size_t member_cnt, BuiltinDisp **ret)
{
    BuiltinDisp *disp;

    if(!(disp = malloc(sizeof(*disp))))
        return E_OUTOFMEMORY;

    disp->IDispatch_iface.lpVtbl = &BuiltinDispVtbl;
    disp->ref = 1;
    disp->members = members;
    disp->member_cnt = member_cnt;
    disp->ctx = ctx;

    *ret = disp;
    return S_OK;
}

static IInternetHostSecurityManager *get_sec_mgr(script_ctx_t *ctx)
{
    IInternetHostSecurityManager *secmgr;
    IServiceProvider *sp;
    HRESULT hres;

    if(!ctx->site)
        return NULL;

    if(ctx->secmgr)
        return ctx->secmgr;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IServiceProvider, (void**)&sp);
    if(FAILED(hres))
        return NULL;

    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager, &IID_IInternetHostSecurityManager,
            (void**)&secmgr);
    IServiceProvider_Release(sp);
    if(FAILED(hres))
        return NULL;

    return ctx->secmgr = secmgr;
}

static HRESULT return_string(VARIANT *res, const WCHAR *str)
{
    BSTR ret;

    if(!res)
        return S_OK;

    ret = SysAllocString(str);
    if(!ret)
        return E_OUTOFMEMORY;

    V_VT(res) = VT_BSTR;
    V_BSTR(res) = ret;
    return S_OK;
}

static HRESULT return_bstr(VARIANT *res, BSTR str)
{
    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT return_bool(VARIANT *res, BOOL val)
{
    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = val ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT return_short(VARIANT *res, short val)
{
    if(res) {
        V_VT(res) = VT_I2;
        V_I2(res) = val;
    }

    return S_OK;
}

static HRESULT return_int(VARIANT *res, int val)
{
    if(res) {
        V_VT(res) = VT_I4;
        V_I4(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_double(VARIANT *res, double val)
{
    if(res) {
        V_VT(res) = VT_R8;
        V_R8(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_float(VARIANT *res, float val)
{
    if(res) {
        V_VT(res) = VT_R4;
        V_R4(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_null(VARIANT *res)
{
    if(res)
        V_VT(res) = VT_NULL;
    return S_OK;
}

static inline HRESULT return_date(VARIANT *res, double date)
{
    if(res) {
        V_VT(res) = VT_DATE;
        V_DATE(res) = date;
    }
    return S_OK;
}

HRESULT to_int(VARIANT *v, int *ret)
{
    VARIANT r;
    HRESULT hres;

    V_VT(&r) = VT_EMPTY;
    hres = VariantChangeType(&r, v, 0, VT_I4);
    if(FAILED(hres))
        return hres;

    *ret = V_I4(&r);
    return S_OK;
}

static HRESULT to_double(VARIANT *v, double *ret)
{
    VARIANT dst;
    HRESULT hres;

    V_VT(&dst) = VT_EMPTY;
    hres = VariantChangeType(&dst, v, 0, VT_R8);
    if(FAILED(hres))
        return hres;

    *ret = V_R8(&dst);
    return S_OK;
}

static HRESULT to_float(VARIANT *v, float *ret)
{
    VARIANT dst;
    HRESULT hres;

    V_VT(&dst) = VT_EMPTY;
    hres = VariantChangeType(&dst, v, 0, VT_R4);
    if(FAILED(hres))
        return hres;

    *ret = V_R4(&dst);
    return S_OK;
}

static HRESULT to_string(VARIANT *v, BSTR *ret)
{
    VARIANT dst;
    HRESULT hres;

    V_VT(&dst) = VT_EMPTY;
    hres = VariantChangeType(&dst, v, VARIANT_LOCALBOOL, VT_BSTR);
    if(FAILED(hres))
        return hres;

    *ret = V_BSTR(&dst);
    return S_OK;
}

static HRESULT to_system_time(VARIANT *v, SYSTEMTIME *st)
{
    VARIANT date;
    HRESULT hres;

    V_VT(&date) = VT_EMPTY;
    hres = VariantChangeType(&date, v, 0, VT_DATE);
    if(FAILED(hres))
        return hres;

    return VariantTimeToSystemTime(V_DATE(&date), st);
}

static HRESULT set_object_site(script_ctx_t *ctx, IUnknown *obj)
{
    IObjectWithSite *obj_site;
    IUnknown *ax_site;
    HRESULT hres;

    hres = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void**)&obj_site);
    if(FAILED(hres))
        return S_OK;

    ax_site = create_ax_site(ctx);
    if(ax_site) {
        hres = IObjectWithSite_SetSite(obj_site, ax_site);
        IUnknown_Release(ax_site);
    }
    else
        hres = E_OUTOFMEMORY;
    IObjectWithSite_Release(obj_site);
    return hres;
}

static IUnknown *create_object(script_ctx_t *ctx, const WCHAR *progid)
{
    IInternetHostSecurityManager *secmgr = NULL;
    struct CONFIRMSAFETY cs;
    IClassFactoryEx *cfex;
    IClassFactory *cf;
    DWORD policy_size;
    BYTE *bpolicy;
    IUnknown *obj;
    DWORD policy;
    GUID guid;
    HRESULT hres;

    hres = CLSIDFromProgID(progid, &guid);
    if(FAILED(hres))
        return NULL;

    TRACE("GUID %s\n", debugstr_guid(&guid));

    if(ctx->safeopt & INTERFACE_USES_SECURITY_MANAGER) {
        secmgr = get_sec_mgr(ctx);
        if(!secmgr)
            return NULL;

        policy = 0;
        hres = IInternetHostSecurityManager_ProcessUrlAction(secmgr, URLACTION_ACTIVEX_RUN,
                (BYTE*)&policy, sizeof(policy), (BYTE*)&guid, sizeof(GUID), 0, 0);
        if(FAILED(hres) || policy != URLPOLICY_ALLOW)
            return NULL;
    }

    hres = CoGetClassObject(&guid, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, NULL, &IID_IClassFactory, (void**)&cf);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IClassFactoryEx, (void**)&cfex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IClassFactoryEx\n");
        IClassFactoryEx_Release(cfex);
    }

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
    if(FAILED(hres))
        return NULL;

    if(secmgr) {
        cs.clsid = guid;
        cs.pUnk = obj;
        cs.dwFlags = 0;
        hres = IInternetHostSecurityManager_QueryCustomPolicy(secmgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
                &bpolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
        if(SUCCEEDED(hres)) {
            policy = policy_size >= sizeof(DWORD) ? *(DWORD*)bpolicy : URLPOLICY_DISALLOW;
            CoTaskMemFree(bpolicy);
        }

        if(FAILED(hres) || policy != URLPOLICY_ALLOW) {
            IUnknown_Release(obj);
            return NULL;
        }
    }

    hres = set_object_site(ctx, obj);
    if(FAILED(hres)) {
        IUnknown_Release(obj);
        return NULL;
    }

    return obj;
}

static HRESULT show_msgbox(script_ctx_t *ctx, BSTR prompt, unsigned type, BSTR orig_title, VARIANT *res)
{
    SCRIPTUICHANDLING uic_handling = SCRIPTUICHANDLING_ALLOW;
    IActiveScriptSiteUIControl *ui_control;
    IActiveScriptSiteWindow *acts_window;
    WCHAR *title_buf = NULL;
    const WCHAR *title;
    HWND hwnd = NULL;
    int ret = 0;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IActiveScriptSiteUIControl, (void**)&ui_control);
    if(SUCCEEDED(hres)) {
        hres = IActiveScriptSiteUIControl_GetUIBehavior(ui_control, SCRIPTUICITEM_MSGBOX, &uic_handling);
        IActiveScriptSiteUIControl_Release(ui_control);
        if(FAILED(hres))
            uic_handling = SCRIPTUICHANDLING_ALLOW;
    }

    switch(uic_handling) {
    case SCRIPTUICHANDLING_ALLOW:
        break;
    case SCRIPTUICHANDLING_NOUIDEFAULT:
        return return_short(res, 0);
    default:
        FIXME("blocked\n");
        return E_FAIL;
    }

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IActiveScriptSiteWindow, (void**)&acts_window);
    if(FAILED(hres)) {
        FIXME("No IActiveScriptSiteWindow\n");
        return hres;
    }

    if(ctx->safeopt & INTERFACE_USES_SECURITY_MANAGER) {
        if(orig_title && *orig_title) {
            WCHAR *ptr;

            title = title_buf = malloc(sizeof(L"VBScript") + (lstrlenW(orig_title)+2)*sizeof(WCHAR));
            if(!title)
                return E_OUTOFMEMORY;

            memcpy(title_buf, L"VBScript", sizeof(L"VBScript"));
            ptr = title_buf + ARRAY_SIZE(L"VBScript")-1;

            *ptr++ = ':';
            *ptr++ = ' ';
            lstrcpyW(ptr, orig_title);
        }else {
            title = L"VBScript";
        }
    }else {
        title = orig_title ? orig_title : L"";
    }

    hres = IActiveScriptSiteWindow_GetWindow(acts_window, &hwnd);
    if(SUCCEEDED(hres)) {
        hres = IActiveScriptSiteWindow_EnableModeless(acts_window, FALSE);
        if(SUCCEEDED(hres)) {
            ret = MessageBoxW(hwnd, prompt, title, type);
            hres = IActiveScriptSiteWindow_EnableModeless(acts_window, TRUE);
        }
    }

    free(title_buf);
    IActiveScriptSiteWindow_Release(acts_window);
    if(FAILED(hres)) {
        FIXME("failed: %08lx\n", hres);
        return hres;
    }

    return return_short(res, ret);
}

static HRESULT Global_CCur(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_CY);
    if(FAILED(hres))
        return hres;

    if(!res) {
        VariantClear(&v);
        return DISP_E_BADVARTYPE;
    }

    *res = v;
    return S_OK;
}

static HRESULT Global_CInt(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_I2);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;
    else {
        *res = v;
        return S_OK;
    }
}

static HRESULT Global_CLng(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    int i;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = to_int(arg, &i);
    if(FAILED(hres))
        return hres;
    if(!res)
        return DISP_E_BADVARTYPE;

    return return_int(res, i);
}

static HRESULT Global_CBool(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, VARIANT_LOCALBOOL, VT_BOOL);
    if(FAILED(hres))
        return hres;

    if(res)
        *res = v;
    else
        VariantClear(&v);
    return S_OK;
}

static HRESULT Global_CByte(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, VARIANT_LOCALBOOL, VT_UI1);
    if(FAILED(hres))
        return hres;

    if(!res) {
        VariantClear(&v);
        return DISP_E_BADVARTYPE;
    }

    *res = v;
    return S_OK;
}

static HRESULT Global_CDate(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_DATE);
    if(FAILED(hres)) {
        hres = VariantChangeType(&v, arg, 0, VT_R8);
        if(FAILED(hres))
            return hres;
        hres = VariantChangeType(&v, &v, 0, VT_DATE);
        if(FAILED(hres))
            return hres;
    }

    if(!res)
        return DISP_E_BADVARTYPE;
    *res = v;
    return S_OK;
}

static HRESULT Global_CDbl(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_R8);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;
    else {
        *res = v;
        return S_OK;
    }
}

static HRESULT Global_CSng(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_R4);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;

   *res = v;
   return S_OK;
}

static HRESULT Global_CStr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, str);
}

static inline WCHAR hex_char(unsigned n)
{
    return n < 10 ? '0'+n : 'A'+n-10;
}

static HRESULT Global_Hex(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR buf[17], *ptr;
    DWORD n;
    HRESULT hres;
    int ret;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_NULL:
        return return_null(res);
    default:
        hres = to_int(arg, &ret);
        if(FAILED(hres))
            return hres;
        else
            n = ret;
    }

    buf[16] = 0;
    ptr = buf+15;

    if(n) {
        do {
            *ptr-- = hex_char(n & 0xf);
            n >>= 4;
        }while(n);
        ptr++;
    }else {
        *ptr = '0';
    }

    return return_string(res, ptr);
}

static HRESULT Global_Oct(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    WCHAR buf[23], *ptr;
    DWORD n;
    int ret;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_NULL:
        return return_null(res);
    default:
        hres = to_int(arg, &ret);
        if(FAILED(hres))
            return hres;
        else
            n = ret;
    }

    buf[22] = 0;
    ptr = buf + 21;

    if(n) {
        do {
            *ptr-- = '0' + (n & 0x7);
            n >>= 3;
        }while(n);
        ptr++;
    }else {
        *ptr = '0';
    }

    return return_string(res, ptr);
}

static HRESULT Global_VarType(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARTYPE vt;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    vt = V_VT(arg) & ~VT_BYREF;
    if(vt & ~(VT_TYPEMASK | VT_ARRAY)) {
        FIXME("not supported %s\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    return return_short(res, vt);
}

static HRESULT Global_IsDate(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    return return_bool(res, V_VT(arg) == VT_DATE);
}

static HRESULT Global_IsEmpty(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);
    return return_bool(res, V_VT(arg) == VT_EMPTY);
}

static HRESULT Global_IsNull(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    return return_bool(res, V_VT(arg) == VT_NULL);
}

static HRESULT Global_IsNumeric(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = to_double(arg, &d);

    return return_bool(res, SUCCEEDED(hres));
}

static HRESULT Global_IsArray(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    return return_bool(res, V_ISARRAY(arg));
}

static HRESULT Global_IsObject(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    return return_bool(res, V_VT(arg) == VT_DISPATCH);
}

static HRESULT Global_Atn(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, atan(d));
}

static HRESULT Global_Cos(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, cos(d));
}

static HRESULT Global_Sin(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, sin(d));
}

static HRESULT Global_Tan(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, tan(d));
}

static HRESULT Global_Exp(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, exp(d));
}

static HRESULT Global_Log(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    if(d <= 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else
        return return_double(res, log(d));
}

static HRESULT Global_Sqr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    if(d < 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else
        return return_double(res, sqrt(d));
}

static unsigned int get_next_rnd(int value)
{
    return (value * 0x43fd43fd + 0xc39ec3) & 0xffffff;
}

static HRESULT Global_Randomize(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    union
    {
        double d;
        unsigned int i[2];
    } dtoi;
    unsigned int seed;
    HRESULT hres;

    assert(args_cnt == 0 || args_cnt == 1);
    if (args_cnt == 1) {
        hres = to_double(arg, &dtoi.d);
        if (FAILED(hres))
            return hres;
    }
    else
        dtoi.d = GetTickCount() * 0.001;

    seed = dtoi.i[1];
    seed ^= (seed >> 16);
    seed = ((seed & 0xffff) << 8) | (This->ctx->script_obj->rnd & 0xff);
    This->ctx->script_obj->rnd = seed;

    return res ? DISP_E_TYPEMISMATCH : S_OK;
}

static HRESULT Global_Rnd(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    static const float modulus = 16777216.0f;
    unsigned int value;
    HRESULT hres;
    float f;

    assert(args_cnt == 0 || args_cnt == 1);

    value = This->ctx->script_obj->rnd;
    if (args_cnt == 1)
    {
        hres = to_float(arg, &f);
        if (FAILED(hres))
            return hres;

        if (f < 0.0f)
        {
            value = *(unsigned int *)&f;
            This->ctx->script_obj->rnd = value = get_next_rnd(value + (value >> 24));
        }
        else if (f == 0.0f)
            value = This->ctx->script_obj->rnd;
        else
            This->ctx->script_obj->rnd = value = get_next_rnd(value);
    }
    else
    {
        This->ctx->script_obj->rnd = value = get_next_rnd(value);
    }

    return return_float(res, (float)value / modulus);
}

static HRESULT Global_Timer(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double sec;

    GetLocalTime(&lt);
    sec = lt.wHour * 3600 + lt.wMinute * 60 + lt.wSecond + lt.wMilliseconds / 1000.0;
    return return_float(res, sec);

}

static HRESULT Global_LBound(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SAFEARRAY *sa;
    HRESULT hres;
    LONG lbound;
    int dim;

    assert(args_cnt == 1 || args_cnt == 2);

    TRACE("%s %s\n", debugstr_variant(arg), args_cnt == 2 ? debugstr_variant(arg + 1) : "1");

    switch(V_VT(arg)) {
    case VT_VARIANT|VT_ARRAY:
        sa = V_ARRAY(arg);
        break;
    case VT_VARIANT|VT_ARRAY|VT_BYREF:
        sa = *V_ARRAYREF(arg);
        break;
    case VT_EMPTY:
    case VT_NULL:
        return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
    default:
        FIXME("arg %s not supported\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    if(args_cnt == 2) {
        hres = to_int(arg + 1, &dim);
        if(FAILED(hres))
            return hres;
    }else {
        dim = 1;
    }

    hres = SafeArrayGetLBound(sa, dim, &lbound);
    if(FAILED(hres))
        return hres;

    return return_int(res, lbound);
}

static HRESULT Global_UBound(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SAFEARRAY *sa;
    HRESULT hres;
    LONG ubound;
    int dim;

    assert(args_cnt == 1 || args_cnt == 2);

    TRACE("%s %s\n", debugstr_variant(arg), args_cnt == 2 ? debugstr_variant(arg + 1) : "1");

    switch(V_VT(arg)) {
    case VT_VARIANT|VT_ARRAY:
        sa = V_ARRAY(arg);
        break;
    case VT_VARIANT|VT_ARRAY|VT_BYREF:
        sa = *V_ARRAYREF(arg);
        break;
    case VT_EMPTY:
    case VT_NULL:
        return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
    default:
        FIXME("arg %s not supported\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    if(args_cnt == 2) {
        hres = to_int(arg + 1, &dim);
        if(FAILED(hres))
            return hres;
    }else {
        dim = 1;
    }

    hres = SafeArrayGetUBound(sa, dim, &ubound);
    if(FAILED(hres))
        return hres;

    return return_int(res, ubound);
}

static HRESULT Global_RGB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    int i, color[3];

    TRACE("%s %s %s\n", debugstr_variant(arg), debugstr_variant(arg + 1), debugstr_variant(arg + 2));

    assert(args_cnt == 3);

    for(i = 0; i < 3; i++) {
        hres = to_int(arg + i, color + i);
        if(FAILED(hres))
            return hres;
        if(color[i] > 255)
            color[i] = 255;
        if(color[i] < 0)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    return return_int(res, RGB(color[0], color[1], color[2]));
}

static HRESULT Global_Len(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    DWORD len;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL)
        return return_null(res);

    if(V_VT(arg) != VT_BSTR) {
        BSTR str;

        hres = to_string(arg, &str);
        if(FAILED(hres))
            return hres;

        len = SysStringLen(str);
        SysFreeString(str);
    }else {
        len = SysStringLen(V_BSTR(arg));
    }

    return return_int(res, len);
}

static HRESULT Global_LenB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Left(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str, ret, conv_str = NULL;
    int len, str_len;
    HRESULT hres;

    TRACE("(%s %s)\n", debugstr_variant(args+1), debugstr_variant(args));

    if(V_VT(args) == VT_BSTR) {
        str = V_BSTR(args);
    }else {
        hres = to_string(args, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    hres = to_int(args+1, &len);
    if(FAILED(hres))
        return hres;

    if(len < 0) {
        FIXME("len = %d\n", len);
        return E_FAIL;
    }

    str_len = SysStringLen(str);
    if(len > str_len)
        len = str_len;

    ret = SysAllocStringLen(str, len);
    SysFreeString(conv_str);
    if(!ret)
        return E_OUTOFMEMORY;

    return return_bstr(res, ret);
}

static HRESULT Global_LeftB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Right(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str, ret, conv_str = NULL;
    int len, str_len;
    HRESULT hres;

    TRACE("(%s %s)\n", debugstr_variant(args), debugstr_variant(args+1));

    if(V_VT(args+1) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_int(args+1, &len);
    if(FAILED(hres))
        return hres;

    if(len < 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

    if(V_VT(args) == VT_NULL)
        return return_null(res);

    if(V_VT(args) == VT_BSTR) {
        str = V_BSTR(args);
    }else {
        hres = to_string(args, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    str_len = SysStringLen(str);
    if(len > str_len)
        len = str_len;

    ret = SysAllocStringLen(str+str_len-len, len);
    SysFreeString(conv_str);
    if(!ret)
        return E_OUTOFMEMORY;

    return return_bstr(res, ret);
}

static HRESULT Global_RightB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Mid(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int len = -1, start, str_len;
    BSTR str, conv_str = NULL;
    HRESULT hres;

    TRACE("(%s %s ...)\n", debugstr_variant(args), debugstr_variant(args+1));

    assert(args_cnt == 2 || args_cnt == 3);

    if(V_VT(args+1) == VT_NULL || (args_cnt == 3 && V_VT(args+2) == VT_NULL))
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    if(V_VT(args+1) == VT_EMPTY)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

    hres = to_int(args+1, &start);
    if(FAILED(hres))
        return hres;

    if(start <= 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

    if(args_cnt == 3) {
        if(V_VT(args+2) == VT_EMPTY)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

        hres = to_int(args+2, &len);
        if(FAILED(hres))
            return hres;

        if(len < 0)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if(V_VT(args) == VT_EMPTY)
        return return_string(res, L"");

    if(V_VT(args) == VT_NULL)
        return return_null(res);

    if(V_VT(args) == VT_BSTR) {
        str = V_BSTR(args);
    }else {
        hres = to_string(args, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    str_len = SysStringLen(str);
    start--;
    if(start > str_len)
        start = str_len;

    if(len == -1)
        len = str_len-start;
    else if(len > str_len-start)
        len = str_len-start;

    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = SysAllocStringLen(str+start, len);
        if(!V_BSTR(res))
            hres = E_OUTOFMEMORY;
    }

    SysFreeString(conv_str);

    return hres;
}

static HRESULT Global_MidB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrComp(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR left, right;
    int mode, ret;
    HRESULT hres;
    short val;

    TRACE("(%s %s ...)\n", debugstr_variant(args), debugstr_variant(args+1));

    assert(args_cnt == 2 || args_cnt == 3);

    if (args_cnt == 3) {
        hres = to_int(args+2, &mode);
        if(FAILED(hres))
            return hres;

        if (mode != 0 && mode != 1) {
            FIXME("unknown compare mode = %d\n", mode);
            return E_FAIL;
        }
    }
    else
        mode = 0;

    hres = to_string(args, &left);
    if(FAILED(hres))
        return hres;

    hres = to_string(args+1, &right);
    if(FAILED(hres))
    {
        SysFreeString(left);
        return hres;
    }

    ret = mode ? wcsicmp(left, right) : wcscmp(left, right);
    val = ret < 0 ? -1 : (ret > 0 ? 1 : 0);

    SysFreeString(left);
    SysFreeString(right);
    return return_short(res, val);
}

static HRESULT Global_LCase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        return return_null(res);
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = towlower(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_UCase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        return return_null(res);
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = towupper(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_LTrim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(ptr = str; *ptr && iswspace(*ptr); ptr++);

    str = SysAllocString(ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_RTrim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(ptr = str+SysStringLen(str); ptr-1 > str && iswspace(*(ptr-1)); ptr--);

    str = SysAllocStringLen(str, ptr-str);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Trim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *begin_ptr, *end_ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(begin_ptr = str; *begin_ptr && iswspace(*begin_ptr); begin_ptr++);
    for(end_ptr = str+SysStringLen(str); end_ptr-1 > begin_ptr && iswspace(*(end_ptr-1)); end_ptr--);

    str = SysAllocStringLen(begin_ptr, end_ptr-begin_ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Space(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    int n, i;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_int(arg, &n);
    if(FAILED(hres))
        return hres;

    if(n < 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

    if(!res)
        return S_OK;

    str = SysAllocStringLen(NULL, n);
    if(!str)
        return E_OUTOFMEMORY;

    for(i=0; i<n; i++)
        str[i] = ' ';

    V_VT(res) = VT_BSTR;
    V_BSTR(res) = str;
    return S_OK;
}

static HRESULT Global_String(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    WCHAR ch;
    int cnt;
    HRESULT hres;

    TRACE("%s %s\n", debugstr_variant(args), debugstr_variant(args + 1));

    hres = to_int(args, &cnt);
    if(FAILED(hres))
        return hres;
    if(cnt < 0)
        return E_INVALIDARG;

    if(V_VT(args + 1) != VT_BSTR) {
        FIXME("Unsupported argument %s\n", debugstr_variant(args+1));
        return E_NOTIMPL;
    }
    if(!SysStringLen(V_BSTR(args + 1)))
        return E_INVALIDARG;
    ch = V_BSTR(args + 1)[0];

    if(res) {
        BSTR str = SysAllocStringLen(NULL, cnt);
        if(!str)
            return E_OUTOFMEMORY;
        wmemset(str, ch, cnt);
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }
    return S_OK;
}

static HRESULT Global_InStr(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    VARIANT *startv, *str1v, *str2v;
    BSTR str1, str2;
    int ret = -1, start = 0, mode = 0;
    HRESULT hres;

    TRACE("args_cnt=%u\n", args_cnt);

    assert(2 <= args_cnt && args_cnt <= 4);

    switch(args_cnt) {
    case 2:
        startv = NULL;
        str1v = args;
        str2v = args+1;
        break;
    case 3:
        startv = args;
        str1v = args+1;
        str2v = args+2;
        break;
    case 4:
        startv = args;
        str1v = args+1;
        str2v = args+2;

        if(V_VT(args+3) == VT_NULL)
            return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

        hres = to_int(args+3, &mode);
        if(FAILED(hres))
            return hres;

        if (mode != 0 && mode != 1)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);

        break;
    DEFAULT_UNREACHABLE;
    }

    if(startv) {
       if(V_VT(startv) == VT_NULL)
            return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

        hres = to_int(startv, &start);
        if(FAILED(hres))
            return hres;
        if(--start < 0)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if(V_VT(str1v) == VT_NULL || V_VT(str2v) == VT_NULL)
        return return_null(res);

    if(V_VT(str1v) != VT_BSTR) {
        hres = to_string(str1v, &str1);
        if(FAILED(hres))
            return hres;
    }
    else
        str1 = V_BSTR(str1v);

    if(V_VT(str2v) != VT_BSTR) {
        hres = to_string(str2v, &str2);
        if(FAILED(hres)){
            if(V_VT(str1v) != VT_BSTR)
                SysFreeString(str1);
            return hres;
        }
    }
    else
        str2 = V_BSTR(str2v);

    if(start < SysStringLen(str1)) {
        ret = FindStringOrdinal(FIND_FROMSTART, str1 + start, SysStringLen(str1)-start,
                                str2, SysStringLen(str2), mode);
    }

    if(V_VT(str1v) != VT_BSTR)
        SysFreeString(str1);
    if(V_VT(str2v) != VT_BSTR)
        SysFreeString(str2);
    return return_int(res, ++ret ? ret+start : 0);
}

static HRESULT Global_InStrB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_AscB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Asc(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR conv_str = NULL, str;
    HRESULT hres = S_OK;

    TRACE("(%s)\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_NULL:
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
    case VT_EMPTY:
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    case VT_BSTR:
        str = V_BSTR(arg);
        break;
    default:
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    if(!SysStringLen(str))
        hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else if (This->ctx->codepage == CP_UTF8)
        hres = return_short(res, *str);
    else {
        unsigned char buf[2];
        short val = 0;
        int n = WideCharToMultiByte(This->ctx->codepage, 0, str, 1, (char*)buf, sizeof(buf), NULL, NULL);
        switch(n) {
        case 1:
            val = buf[0];
            break;
        case 2:
            val = (buf[0] << 8) | buf[1];
            break;
        default:
            WARN("Failed to convert %x\n", *str);
            hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
        }
        if(SUCCEEDED(hres))
            hres = return_short(res, val);
    }

    SysFreeString(conv_str);
    return hres;
}

static HRESULT Global_Chr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    int cp, c, len = 0;
    CPINFO cpi;
    WCHAR ch;
    char buf[2];
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_int(arg, &c);
    if(FAILED(hres))
        return hres;

    cp = This->ctx->codepage;
    if(!GetCPInfo(cp, &cpi))
        cpi.MaxCharSize = 1;

    if((c!=(short)c && c!=(unsigned short)c) ||
            (unsigned short)c>=(cpi.MaxCharSize>1 ? 0x10000 : 0x100)) {
        WARN("invalid arg %d\n", c);
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if (cp == CP_UTF8)
    {
        ch = c;
    }
    else
    {
        if(c>>8)
            buf[len++] = c>>8;
        if(!len || IsDBCSLeadByteEx(cp, buf[0]))
            buf[len++] = c;
        if(!MultiByteToWideChar(cp, 0, buf, len, &ch, 1)) {
            WARN("invalid arg %d, cp %d\n", c, cp);
            return E_FAIL;
        }
    }

    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = SysAllocStringLen(&ch, 1);
        if(!V_BSTR(res))
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

static HRESULT Global_AscW(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrW(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Abs(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarAbs(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Fix(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarFix(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Int(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarInt(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Sgn(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    double v;
    short val;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_double(arg, &v);
    if (FAILED(hres))
        return hres;

    val = v == 0 ? 0 : (v > 0 ? 1 : -1);
    return return_short(res, val);
}

static HRESULT Global_Now(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double date;

    TRACE("\n");

    GetLocalTime(&lt);
    SystemTimeToVariantTime(&lt, &date);
    return return_date(res, date);
}

static HRESULT Global_Date(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    UDATE ud;
    DATE date;
    HRESULT hres;

    TRACE("\n");

    GetLocalTime(&lt);
    ud.st = lt;
    ud.wDayOfYear = 0;
    hres = VarDateFromUdateEx(&ud, 0, VAR_DATEVALUEONLY, &date);
    if(FAILED(hres))
        return hres;
    return return_date(res, date);
}

static HRESULT Global_Time(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    UDATE ud;
    DATE time;
    HRESULT hres;

    TRACE("\n");

    GetLocalTime(&lt);
    ud.st = lt;
    ud.wDayOfYear = 0;
    hres = VarDateFromUdateEx(&ud, 0, VAR_TIMEVALUEONLY, &time);
    if(FAILED(hres))
        return hres;
    return return_date(res, time);
}

static HRESULT Global_Day(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wDay);
}

static HRESULT Global_Month(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wMonth);
}

static HRESULT Global_Weekday(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres = S_OK;
    int first_day = 0;
    SYSTEMTIME st;

    TRACE("(%s)\n", debugstr_variant(args));

    assert(args_cnt == 1 || args_cnt == 2);

    /* [vbSunday = 1, vbSaturday = 7] -> wDayOfWeek [0, 6] */
    if (args_cnt == 2)
    {
        if (V_VT(args + 1) == VT_NULL)
            return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

        hres = to_int(args + 1, &first_day);
        if (SUCCEEDED(hres))
        {
            if (!first_day)
            {
                /* vbUseSystemDayOfWeek */
                GetLocaleInfoW(This->ctx->lcid, LOCALE_RETURN_NUMBER | LOCALE_IFIRSTDAYOFWEEK, (LPWSTR)&first_day,
                        sizeof(first_day) / sizeof(WCHAR));
                first_day = (first_day + 1) % 7;
            }
            else if (first_day >= 1 && first_day <= 7)
            {
                first_day--;
            }
            else
                return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
        }
    }

    if (FAILED(hres))
        return hres;

    if (V_VT(args) == VT_NULL)
        return return_null(res);

    if (FAILED(hres = to_system_time(args, &st))) return hres;

    return return_short(res, 1 + (7 - first_day + st.wDayOfWeek) % 7);
}

static HRESULT Global_Year(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wYear);
}

static HRESULT Global_Hour(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wHour);
}

static HRESULT Global_Minute(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wMinute);
}

static HRESULT Global_Second(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wSecond);
}

static HRESULT Global_SetLocale(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateValue(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeValue(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateSerial(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int year, month, day;
    UDATE ud = {{ 0 }};
    HRESULT hres;
    double date;

    TRACE("\n");

    assert(args_cnt == 3);

    if (V_VT(args) == VT_NULL || V_VT(args + 1) == VT_NULL || V_VT(args + 2) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_int(args, &year);
    if (SUCCEEDED(hres))
        hres = to_int(args + 1, &month);
    if (SUCCEEDED(hres))
        hres = to_int(args + 2, &day);

    if (SUCCEEDED(hres))
    {
        ud.st.wYear = year;
        ud.st.wMonth = month;
        ud.st.wDay = day;
        hres = VarDateFromUdateEx(&ud, 0, 0, &date);
    }

    if (SUCCEEDED(hres))
        hres = return_date(res, date);

    return hres;
}

static HRESULT Global_TimeSerial(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int hour, minute, second;
    UDATE ud = {{ 0 }};
    HRESULT hres;
    double date;

    TRACE("\n");

    assert(args_cnt == 3);

    if (V_VT(args) == VT_NULL || V_VT(args + 1) == VT_NULL || V_VT(args + 2) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_int(args, &hour);
    if (SUCCEEDED(hres))
        hres = to_int(args + 1, &minute);
    if (SUCCEEDED(hres))
        hres = to_int(args + 2, &second);

    if (SUCCEEDED(hres))
    {
        ud.st.wYear = 1899;
        ud.st.wMonth = 12;
        ud.st.wDay = 30;
        ud.st.wHour = hour;
        ud.st.wMinute = minute;
        ud.st.wSecond = second;
        hres = VarDateFromUdateEx(&ud, 0, 0, &date);
    }

    if (SUCCEEDED(hres))
        hres = return_date(res, date);

    return hres;
}

static HRESULT Global_InputBox(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_MsgBox(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR prompt, title = NULL;
    int type = MB_OK;
    HRESULT hres;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 5);

    hres = to_string(args, &prompt);
    if(FAILED(hres))
        return hres;

    if(args_cnt > 1)
        hres = to_int(args+1, &type);

    if(SUCCEEDED(hres) && args_cnt > 2)
        hres = to_string(args+2, &title);

    if(SUCCEEDED(hres) && args_cnt > 3) {
        FIXME("unsupported arg_cnt %d\n", args_cnt);
        hres = E_NOTIMPL;
    }

    if(SUCCEEDED(hres))
        hres = show_msgbox(This->ctx, prompt, type, title, res);

    SysFreeString(prompt);
    SysFreeString(title);
    return hres;
}

static HRESULT Global_CreateObject(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    IUnknown *obj;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    if(V_VT(arg) != VT_BSTR) {
        FIXME("non-bstr arg\n");
        return E_INVALIDARG;
    }

    obj = create_object(This->ctx, V_BSTR(arg));
    if(!obj)
        return VB_E_CANNOT_CREATE_OBJ;

    if(res) {
        hres = IUnknown_QueryInterface(obj, &IID_IDispatch, (void**)&V_DISPATCH(res));
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_DISPATCH;
    }

    IUnknown_Release(obj);
    return S_OK;
}

static HRESULT Global_GetObject(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    IBindCtx *bind_ctx;
    IUnknown *obj_unk;
    IDispatch *disp;
    ULONG eaten = 0;
    IMoniker *mon;
    HRESULT hres;

    TRACE("%s %s\n", args_cnt ? debugstr_variant(args) : "", args_cnt > 1 ? debugstr_variant(args+1) : "");

    if(args_cnt != 1 || V_VT(args) != VT_BSTR) {
        FIXME("unsupported args\n");
        return E_NOTIMPL;
    }

    if(This->ctx->safeopt & (INTERFACE_USES_SECURITY_MANAGER|INTERFACESAFE_FOR_UNTRUSTED_DATA)) {
        WARN("blocked in current safety mode\n");
        return VB_E_CANNOT_CREATE_OBJ;
    }

    hres = CreateBindCtx(0, &bind_ctx);
    if(FAILED(hres))
        return hres;

    hres = MkParseDisplayName(bind_ctx, V_BSTR(args), &eaten, &mon);
    if(SUCCEEDED(hres)) {
        hres = IMoniker_BindToObject(mon, bind_ctx, NULL, &IID_IUnknown, (void**)&obj_unk);
        IMoniker_Release(mon);
    }else {
        hres = MK_E_SYNTAX;
    }
    IBindCtx_Release(bind_ctx);
    if(FAILED(hres))
        return hres;

    hres = set_object_site(This->ctx, obj_unk);
    if(FAILED(hres)) {
        IUnknown_Release(obj_unk);
        return hres;
    }

    hres = IUnknown_QueryInterface(obj_unk, &IID_IDispatch, (void**)&disp);
    if(SUCCEEDED(hres)) {
        if(res) {
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = disp;
        }else {
            IDispatch_Release(disp);
        }
    }else {
        FIXME("object does not support IDispatch\n");
    }

    return hres;
}

static HRESULT Global_DateAdd(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR interval = NULL;
    UDATE ud = {{ 0 }};
    HRESULT hres;
    double date;
    int count;

    TRACE("\n");

    assert(args_cnt == 3);

    if (V_VT(args) == VT_NULL || V_VT(args + 1) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    if (V_VT(args + 2) == VT_NULL)
        return return_null(res);

    hres = to_string(args, &interval);
    if (SUCCEEDED(hres))
        hres = to_int(args + 1, &count);
    if (SUCCEEDED(hres))
        hres = to_system_time(args + 2, &ud.st);
    if (SUCCEEDED(hres))
    {
        if (!wcsicmp(interval, L"yyyy"))
            ud.st.wYear += count;
        else if (!wcsicmp(interval, L"q"))
            ud.st.wMonth += 3 * count;
        else if (!wcsicmp(interval, L"m"))
            ud.st.wMonth += count;
        else if (!wcsicmp(interval, L"y")
                || !wcsicmp(interval, L"d")
                || !wcsicmp(interval, L"w"))
        {
            ud.st.wDay += count;
        }
        else if (!wcsicmp(interval, L"ww"))
            ud.st.wDay += 7 * count;
        else if (!wcsicmp(interval, L"h"))
            ud.st.wHour += count;
        else if (!wcsicmp(interval, L"n"))
            ud.st.wMinute += count;
        else if (!wcsicmp(interval, L"s"))
            ud.st.wSecond += count;
        else
        {
            WARN("Unrecognized interval %s.\n", debugstr_w(interval));
            hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
        }
    }

    SysFreeString(interval);

    if (SUCCEEDED(hres))
        hres = VarDateFromUdateEx(&ud, 0, 0, &date);

    if (SUCCEEDED(hres))
        hres = return_date(res, date);

    return hres;
}

static HRESULT Global_DateDiff(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DatePart(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TypeName(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    ITypeInfo *typeinfo;
    BSTR name = NULL;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if (V_ISARRAY(arg))
        return return_string(res, L"Variant()");

    switch(V_VT(arg)) {
        case VT_UI1:
            return return_string(res, L"Byte");
        case VT_I2:
            return return_string(res, L"Integer");
        case VT_I4:
            return return_string(res, L"Long");
        case VT_R4:
            return return_string(res, L"Single");
        case VT_R8:
            return return_string(res, L"Double");
        case VT_CY:
            return return_string(res, L"Currency");
        case VT_DECIMAL:
            return return_string(res, L"Decimal");
        case VT_DATE:
            return return_string(res, L"Date");
        case VT_BSTR:
            return return_string(res, L"String");
        case VT_BOOL:
            return return_string(res, L"Boolean");
        case VT_EMPTY:
            return return_string(res, L"Empty");
        case VT_NULL:
            return return_string(res, L"Null");
        case VT_DISPATCH:
            if (!V_DISPATCH(arg))
                return return_string(res, L"Nothing");
            if (SUCCEEDED(IDispatch_GetTypeInfo(V_DISPATCH(arg), 0, GetUserDefaultLCID(), &typeinfo)))
            {
                hres = ITypeInfo_GetDocumentation(typeinfo, MEMBERID_NIL, &name, NULL, NULL, NULL);
                ITypeInfo_Release(typeinfo);

                if (SUCCEEDED(hres) && name && *name)
                    return return_bstr(res, name);

                SysFreeString(name);
            }
            return return_string(res, L"Object");
        default:
            FIXME("arg %s not supported\n", debugstr_variant(arg));
            return E_NOTIMPL;
        }
}

static HRESULT Global_Array(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SAFEARRAYBOUND bounds;
    SAFEARRAY *sa;
    VARIANT *data;
    HRESULT hres;
    unsigned i;

    TRACE("arg_cnt=%u\n", args_cnt);

    bounds.lLbound = 0;
    bounds.cElements = args_cnt;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    if(!sa)
        return E_OUTOFMEMORY;

    hres = SafeArrayAccessData(sa, (void**)&data);
    if(FAILED(hres)) {
        SafeArrayDestroy(sa);
        return hres;
    }

    for(i=0; i<args_cnt; i++) {
        hres = VariantCopyInd(data+i, arg+i);
        if(FAILED(hres)) {
            SafeArrayUnaccessData(sa);
            SafeArrayDestroy(sa);
            return hres;
        }
    }
    SafeArrayUnaccessData(sa);

    if(res) {
        V_VT(res) = VT_ARRAY|VT_VARIANT;
        V_ARRAY(res) = sa;
    }else {
        SafeArrayDestroy(sa);
    }

    return S_OK;
}

static HRESULT Global_Erase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Filter(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Join(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR free_delimiter = NULL, output = NULL, str = NULL, *str_array = NULL;
    UINT total_len = 0, delimiter_len = 1, str_len;
    const WCHAR *delimiter = L" ";
    LONG lbound, ubound, count;
    WCHAR *output_ptr;
    unsigned int i;
    SAFEARRAY *sa;
    VARIANT *data;
    HRESULT hres;

    assert(1 <= args_cnt && args_cnt <= 2);

    switch(V_VT(args)) {
        case VT_NULL:
            return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        case VT_DISPATCH:
            return MAKE_VBSERROR(VBSE_OLE_NO_PROP_OR_METHOD);
        case VT_VARIANT|VT_ARRAY:
            sa = V_ARRAY(args);
            break;
        case VT_VARIANT|VT_ARRAY|VT_BYREF:
            sa = *V_ARRAYREF(args);
            break;
        default:
            return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
    }

    if (args_cnt == 2) {
        if (V_VT(args + 1) == VT_NULL)
            return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        if (V_VT(args + 1) != VT_BSTR) {
            hres = to_string(args + 1, &free_delimiter);
            if (FAILED(hres))
                return hres;
            delimiter = free_delimiter;
            delimiter_len = SysStringLen(free_delimiter);
        } else {
            delimiter = V_BSTR(args + 1);
            delimiter_len = SysStringLen(V_BSTR(args + 1));
        }
    }

    if (SafeArrayGetDim(sa) != 1) {
        hres = MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
        goto cleanup;
    }

    hres = SafeArrayGetLBound(sa, 1, &lbound);
    if (FAILED(hres))
        goto cleanup;

    hres = SafeArrayGetUBound(sa, 1, &ubound);
    if (FAILED(hres))
        goto cleanup;
    count = ubound - lbound + 1;

    hres = SafeArrayAccessData(sa, (void**)&data);
    if (FAILED(hres))
        goto cleanup;

    str_array = calloc(count, sizeof(BSTR));
    if (!str_array) {
        hres = E_OUTOFMEMORY;
        goto cleanup_data;
    }
    for (i = 0; i < count; i++) {
        if (V_VT(&data[i]) != VT_BSTR) {
            hres = to_string(&data[i], &str);
            if (FAILED(hres))
                goto cleanup_data;
        } else {
            str = V_BSTR(&data[i]);
        }

        str_array[i] = str;
        total_len += SysStringLen(str);
        if (i)
            total_len += delimiter_len;
    }

    output = SysAllocStringLen(NULL, total_len);
    if (!output) {
        hres = E_OUTOFMEMORY;
        goto cleanup_data;
    }

    output_ptr = output;

    for (i = 0; i < count; i++) {
        if (i) {
            memcpy(output_ptr, delimiter, delimiter_len * sizeof(WCHAR));
            output_ptr += delimiter_len;
        }
        str = str_array[i];
        str_len = SysStringLen(str);
        memcpy(output_ptr, str, str_len * sizeof(WCHAR));
        output_ptr += str_len;
    }

    *output_ptr = L'\0';
    hres = return_bstr(res, output);

cleanup_data:
    for (i = 0; i < count; i++) {
        if (V_VT(&data[i]) != VT_BSTR)
            SysFreeString(str_array[i]);
    }
    SafeArrayUnaccessData(sa);
cleanup:
    SysFreeString(free_delimiter);
    free(str_array);
    return hres;
}

static HRESULT Global_Split(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR string, delimiter = NULL;
    int count, max, mode, len, start, end, ret, delimiterlen = 1;
    int i, *indices = NULL, *new_indices, indices_max = 8;
    SAFEARRAYBOUND bounds;
    SAFEARRAY *sa = NULL;
    VARIANT *data;
    HRESULT hres = S_OK;

    TRACE("%s %u...\n", debugstr_variant(args), args_cnt);

    assert(1 <= args_cnt && args_cnt <= 4);

    if(V_VT(args) == VT_NULL || (args_cnt > 1 && V_VT(args+1) == VT_NULL) || (args_cnt > 2 && V_VT(args+2) == VT_NULL)
       || (args_cnt == 4 && V_VT(args+3) == VT_NULL))
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    if(V_VT(args) != VT_BSTR) {
        hres = to_string(args, &string);
        if(FAILED(hres))
            return hres;
    }else {
        string = V_BSTR(args);
    }

    if(args_cnt > 1) {
        if(V_VT(args+1) != VT_BSTR) {
            hres = to_string(args+1, &delimiter);
            if(FAILED(hres))
                goto error;
        }else {
            delimiter = V_BSTR(args+1);
        }
        delimiterlen = SysStringLen(delimiter);
    }

    if(args_cnt > 2) {
        hres = to_int(args+2, &max);
        if(FAILED(hres))
            goto error;
        if (max < -1) {
            hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
            goto error;
       }
    }else {
        max = -1;
    }

    if(args_cnt == 4) {
        hres = to_int(args+3, &mode);
        if(FAILED(hres))
            goto error;
        if (mode != 0 && mode != 1) {
            hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
            goto error;
        }
    }else {
        mode = 0;
    }

    start = 0;

    len = SysStringLen(string);
    count = 0;

    indices = malloc( indices_max * sizeof(int));
    if(!indices) {
        hres = E_OUTOFMEMORY;
        goto error;
    }

    while(1) {
        ret = -1;
        if (delimiterlen) {
            ret = FindStringOrdinal(FIND_FROMSTART, string + start, len - start,
                                    delimiter ? delimiter : L" ", delimiterlen, mode);
        }

        if (ret == -1) {
            end = len;
        }else {
            end = start + ret;
        }

        if (count == indices_max) {
            new_indices = realloc(indices, indices_max * 2 * sizeof(int));
            if(!new_indices) {
                hres = E_OUTOFMEMORY;
                goto error;
            }
            indices = new_indices;
            indices_max *= 2;
        }
        indices[count++] = end;

        if (ret == -1 || count == max) break;
        start = start + ret + delimiterlen;
        if (start > len) break;
    }

    bounds.lLbound = 0;
    bounds.cElements = count;
    sa = SafeArrayCreate( VT_VARIANT, 1, &bounds);
    if (!sa) {
        hres = E_OUTOFMEMORY;
        goto error;
    }
    hres = SafeArrayAccessData(sa, (void**)&data);
    if(FAILED(hres)) {
        goto error;
    }

    start = 0;
    for (i = 0; i < count; i++) {
        V_VT(&data[i]) = VT_BSTR;
        V_BSTR(&data[i]) = SysAllocStringLen(string + start, indices[i] - start);

        if (!V_BSTR(&data[i])) {
            hres = E_OUTOFMEMORY;
            break;
        }
        start = indices[i]+delimiterlen;
    }
    SafeArrayUnaccessData(sa);

error:
    if(SUCCEEDED(hres) && res) {
        V_VT(res) = VT_ARRAY|VT_VARIANT;
        V_ARRAY(res) = sa;
    }else {
        SafeArrayDestroy(sa);
    }

    free(indices);
    if(V_VT(args) != VT_BSTR)
        SysFreeString(string);
    if(args_cnt > 1 && V_VT(args+1) != VT_BSTR)
        SysFreeString(delimiter);
    return hres;
}

static HRESULT Global_Replace(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR string, find = NULL, replace = NULL, ret;
    int from = 1, cnt = -1, mode = 0;
    HRESULT hres = S_OK;

    TRACE("%s %s %s %u...\n", debugstr_variant(args), debugstr_variant(args+1), debugstr_variant(args+2), args_cnt);

    assert(3 <= args_cnt && args_cnt <= 6);

   if(V_VT(args) == VT_NULL || V_VT(args+1) == VT_NULL || (V_VT(args+2) == VT_NULL)
        || (args_cnt >= 4 && V_VT(args+3) == VT_NULL) || (args_cnt >= 5 && V_VT(args+4) == VT_NULL)
        || (args_cnt == 6 && V_VT(args+5) == VT_NULL))
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);


    if(V_VT(args) != VT_BSTR) {
        hres = to_string(args, &string);
        if(FAILED(hres))
            return hres;
    }else {
        string = V_BSTR(args);
    }

    if(V_VT(args+1) != VT_BSTR) {
        hres = to_string(args+1, &find);
        if(FAILED(hres))
            goto error;
    }else {
        find = V_BSTR(args+1);
    }

    if(V_VT(args+2) != VT_BSTR) {
        hres = to_string(args+2, &replace);
        if(FAILED(hres))
            goto error;
    }else {
        replace = V_BSTR(args+2);
    }

    if(args_cnt >= 4) {
        hres = to_int(args+3, &from);
        if(FAILED(hres))
            goto error;
        if(from < 1) {
            hres = E_INVALIDARG;
            goto error;
        }
    }

    if(args_cnt >= 5) {
        hres = to_int(args+4, &cnt);
        if(FAILED(hres))
            goto error;
        if(cnt < -1) {
            hres = E_INVALIDARG;
            goto error;
        }
    }

    if(args_cnt == 6) {
        hres = to_int(args+5, &mode);
        if(FAILED(hres))
            goto error;
        if (mode != 0 && mode != 1) {
            hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
            goto error;
        }
    }

    ret = string_replace(string, find, replace, from - 1, cnt, mode);
    if(!ret) {
        hres = E_OUTOFMEMORY;
    }else if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = ret;
    }else {
        SysFreeString(ret);
    }

error:
    if(V_VT(args) != VT_BSTR)
        SysFreeString(string);
    if(V_VT(args+1) != VT_BSTR)
        SysFreeString(find);
    if(V_VT(args+2) != VT_BSTR)
        SysFreeString(replace);
    return hres;
}

static HRESULT Global_StrReverse(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR *ptr1, *ptr2, ch;
    BSTR ret;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_string(arg, &ret);
    if(FAILED(hres))
        return hres;

    ptr1 = ret;
    ptr2 = ret + SysStringLen(ret)-1;
    while(ptr1 < ptr2) {
        ch = *ptr1;
        *ptr1++ = *ptr2;
        *ptr2-- = ch;
    }

    return return_bstr(res, ret);
}

static HRESULT Global_InStrRev(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int start = -1, ret = -1, mode = 0;
    BSTR str1, str2;
    size_t len1, len2;
    HRESULT hres;

    TRACE("%s %s arg_cnt=%u\n", debugstr_variant(args), debugstr_variant(args+1), args_cnt);

    assert(2 <= args_cnt && args_cnt <= 4);

    if(V_VT(args) == VT_NULL || V_VT(args+1) == VT_NULL || (args_cnt > 2 && V_VT(args+2) == VT_NULL)
       || (args_cnt == 4 && V_VT(args+3) == VT_NULL))
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    if(args_cnt == 4) {
        hres = to_int(args+3, &mode);
        if(FAILED(hres))
            return hres;
        if (mode != 0 && mode != 1)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if(args_cnt >= 3) {
        hres = to_int(args+2, &start);
        if(FAILED(hres))
            return hres;
        if(!start || start < -1)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if(V_VT(args) != VT_BSTR) {
        hres = to_string(args, &str1);
        if(FAILED(hres))
            return hres;
    }
    else
        str1 = V_BSTR(args);

    if(V_VT(args+1) != VT_BSTR) {
        hres = to_string(args+1, &str2);
        if(FAILED(hres)) {
            if(V_VT(args) != VT_BSTR)
                SysFreeString(str1);
            return hres;
        }
    }
    else
        str2 = V_BSTR(args+1);

    len1 = SysStringLen(str1);
    if(!len1) {
        ret = 0;
        goto end;
    }

    if(start == -1)
        start = len1;

    len2 = SysStringLen(str2);
    if(!len2) {
        ret = start;
        goto end;
    }

    if(start >= len2 && start <= len1) {
        ret = FindStringOrdinal(FIND_FROMEND, str1, start,
                                str2, len2, mode);
    }
    ret++;

end:
    if(V_VT(args) != VT_BSTR)
        SysFreeString(str1);
    if(V_VT(args+1) != VT_BSTR)
        SysFreeString(str2);
    return return_int(res, ret);
}

static HRESULT Global_LoadPicture(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngine(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_string(res, L"VBScript");
}

static HRESULT Global_ScriptEngineMajorVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MAJOR_VERSION);
}

static HRESULT Global_ScriptEngineMinorVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MINOR_VERSION);
}

static HRESULT Global_ScriptEngineBuildVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_BUILD_VERSION);
}

static HRESULT Global_FormatNumber(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    union
    {
        struct
        {
            int num_dig, inc_lead, use_parens, group;
        } s;
        int val[4];
    } int_args = { .s.num_dig = -1, .s.inc_lead = -2, .s.use_parens = -2, .s.group = -2 };
    HRESULT hres;
    BSTR str;
    int i;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 5);

    for (i = 1; i < args_cnt; ++i)
    {
        if (V_VT(args+i) == VT_ERROR) continue;
        if (V_VT(args+i) == VT_NULL) return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        if (FAILED(hres = to_int(args+i, &int_args.val[i-1]))) return hres;
    }

    hres = VarFormatNumber(args, int_args.s.num_dig, int_args.s.inc_lead, int_args.s.use_parens,
        int_args.s.group, 0, &str);
    if (FAILED(hres)) return hres;

    return return_bstr(res, str);
}

static HRESULT Global_FormatCurrency(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    union
    {
        struct
        {
            int num_dig, inc_lead, use_parens, group;
        } s;
        int val[4];
    } int_args = { .s.num_dig = -1, .s.inc_lead = -2, .s.use_parens = -2, .s.group = -2 };
    HRESULT hres;
    BSTR str;
    int i;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 5);

    for (i = 1; i < args_cnt; ++i)
    {
        if (V_VT(args+i) == VT_ERROR) continue;
        if (V_VT(args+i) == VT_NULL) return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        if (FAILED(hres = to_int(args+i, &int_args.val[i-1]))) return hres;
    }

    hres = VarFormatCurrency(args, int_args.s.num_dig, int_args.s.inc_lead, int_args.s.use_parens,
        int_args.s.group, 0, &str);
    if (FAILED(hres)) return hres;

    return return_bstr(res, str);
}

static HRESULT Global_FormatPercent(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    union
    {
        struct
        {
            int num_dig, inc_lead, use_parens, group;
        } s;
        int val[4];
    } int_args = { .s.num_dig = -1, .s.inc_lead = -2, .s.use_parens = -2, .s.group = -2 };
    HRESULT hres;
    BSTR str;
    int i;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 5);

    for (i = 1; i < args_cnt; ++i)
    {
        if (V_VT(args+i) == VT_ERROR) continue;
        if (V_VT(args+i) == VT_NULL) return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        if (FAILED(hres = to_int(args+i, &int_args.val[i-1]))) return hres;
    }

    hres = VarFormatPercent(args, int_args.s.num_dig, int_args.s.inc_lead, int_args.s.use_parens,
        int_args.s.group, 0, &str);
    if (FAILED(hres)) return hres;

    return return_bstr(res, str);
}

static HRESULT Global_GetLocale(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatDateTime(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int format = 0;
    HRESULT hres;
    BSTR str;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 2);

    if (V_VT(args) == VT_NULL)
        return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);

    if (args_cnt == 2)
    {
        if (V_VT(args+1) == VT_NULL) return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
        if (V_VT(args+1) != VT_ERROR)
        {
            if (FAILED(hres = to_int(args+1, &format))) return hres;
        }
    }

    hres = VarFormatDateTime(args, format, 0, &str);
    if (FAILED(hres)) return hres;

    return return_bstr(res, str);
}

static HRESULT Global_WeekdayName(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int weekday, first_day = 1, abbrev = 0;
    BSTR ret;
    HRESULT hres;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 3);

    hres = to_int(args, &weekday);
    if(FAILED(hres))
        return hres;

    if(args_cnt > 1) {
        hres = to_int(args+1, &abbrev);
        if(FAILED(hres))
            return hres;

        if(args_cnt == 3) {
            hres = to_int(args+2, &first_day);
            if(FAILED(hres))
                return hres;
        }
    }

    hres = VarWeekdayName(weekday, abbrev, first_day, 0, &ret);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, ret);
}

static HRESULT Global_MonthName(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int month, abbrev = 0;
    BSTR ret;
    HRESULT hres;

    TRACE("\n");

    assert(args_cnt == 1 || args_cnt == 2);

    if(V_VT(args) == VT_NULL || (args_cnt == 2 && V_VT(args+1) == VT_NULL))
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_int(args, &month);
    if(FAILED(hres))
        return hres;

    if(args_cnt == 2) {
        hres = to_int(args+1, &abbrev);
        if(FAILED(hres))
            return hres;
    }

    hres = VarMonthName(month, abbrev, 0, &ret);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, ret);
}

static HRESULT Global_Round(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int decimal_places = 0;
    double d;
    HRESULT hres;

    TRACE("%s %s\n", debugstr_variant(args), args_cnt == 2 ? debugstr_variant(args + 1) : "0");

    assert(args_cnt == 1 || args_cnt == 2);

    if(!res)
        return S_OK;

    if(args_cnt == 2) {
       if (V_VT(args + 1) != VT_ERROR) {
           hres = to_int(args + 1, &decimal_places);
           if (FAILED(hres))
              return hres;
       }
    }

    switch(V_VT(args)) {
    case VT_I2:
    case VT_I4:
    case VT_BOOL:
        *res = *args;
        return S_OK;
    case VT_R8:
        d = V_R8(args);
        break;
    default:
        hres = to_double(args, &d);
        if(FAILED(hres))
            return hres;
    }

    hres = VarR8Round(d, decimal_places, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, d);
}

static HRESULT Global_Escape(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Unescape(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Eval(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Execute(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ExecuteGlobal(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_GetRef(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Err(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    if(args_cnt) {
        FIXME("Setter not supported\n");
        return E_NOTIMPL;
    }

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = &This->ctx->err_obj->IDispatch_iface;
    IDispatch_AddRef(V_DISPATCH(res));
    return S_OK;
}

static const string_constant_t vbCr          = {1, {'\r'}};
static const string_constant_t vbCrLf        = {2, {'\r','\n'}};
static const string_constant_t vbNewLine     = {2, {'\r','\n'}};
static const string_constant_t vbFormFeed    = {1, {0xc}};
static const string_constant_t vbLf          = {1, {'\n'}};
static const string_constant_t vbNullChar    = {1};
static const string_constant_t vbNullString  = {0};
static const string_constant_t vbTab         = {1, {'\t'}};
static const string_constant_t vbVerticalTab = {1, {0xb}};

static const builtin_prop_t global_props[] = {
    {NULL}, /* no default value */
    {L"Abs",                       Global_Abs, 0, 1},
    {L"Array",                     Global_Array, 0, 0, MAXDWORD},
    {L"Asc",                       Global_Asc, 0, 1},
    {L"AscB",                      Global_AscB, 0, 1},
    {L"AscW",                      Global_AscW, 0, 1},
    {L"Atn",                       Global_Atn, 0, 1},
    {L"CBool",                     Global_CBool, 0, 1},
    {L"CByte",                     Global_CByte, 0, 1},
    {L"CCur",                      Global_CCur, 0, 1},
    {L"CDate",                     Global_CDate, 0, 1},
    {L"CDbl",                      Global_CDbl, 0, 1},
    {L"Chr",                       Global_Chr, 0, 1},
    {L"ChrB",                      Global_ChrB, 0, 1},
    {L"ChrW",                      Global_ChrW, 0, 1},
    {L"CInt",                      Global_CInt, 0, 1},
    {L"CLng",                      Global_CLng, 0, 1},
    {L"Cos",                       Global_Cos, 0, 1},
    {L"CreateObject",              Global_CreateObject, 0, 1},
    {L"CSng",                      Global_CSng, 0, 1},
    {L"CStr",                      Global_CStr, 0, 1},
    {L"Date",                      Global_Date, 0, 0},
    {L"DateAdd",                   Global_DateAdd, 0, 3},
    {L"DateDiff",                  Global_DateDiff, 0, 3, 5},
    {L"DatePart",                  Global_DatePart, 0, 2, 4},
    {L"DateSerial",                Global_DateSerial, 0, 3},
    {L"DateValue",                 Global_DateValue, 0, 1},
    {L"Day",                       Global_Day, 0, 1},
    {L"Erase",                     Global_Erase, 0, 1},
    {L"Err",                       Global_Err, BP_GETPUT},
    {L"Escape",                    Global_Escape, 0, 1},
    {L"Eval",                      Global_Eval, 0, 1},
    {L"Execute",                   Global_Execute, 0, 1},
    {L"ExecuteGlobal",             Global_ExecuteGlobal, 0, 1},
    {L"Exp",                       Global_Exp, 0, 1},
    {L"Filter",                    Global_Filter, 0, 2, 4},
    {L"Fix",                       Global_Fix, 0, 1},
    {L"FormatCurrency",            Global_FormatCurrency, 0, 1, 5},
    {L"FormatDateTime",            Global_FormatDateTime, 0, 1, 2},
    {L"FormatNumber",              Global_FormatNumber, 0, 1, 5},
    {L"FormatPercent",             Global_FormatPercent, 0, 1, 5},
    {L"GetLocale",                 Global_GetLocale, 0, 0},
    {L"GetObject",                 Global_GetObject, 0, 0, 2},
    {L"GetRef",                    Global_GetRef, 0, 1},
    {L"Hex",                       Global_Hex, 0, 1},
    {L"Hour",                      Global_Hour, 0, 1},
    {L"InputBox",                  Global_InputBox, 0, 1, 7},
    {L"InStr",                     Global_InStr, 0, 2, 4},
    {L"InStrB",                    Global_InStrB, 0, 3, 4},
    {L"InStrRev",                  Global_InStrRev, 0, 2, 4},
    {L"Int",                       Global_Int, 0, 1},
    {L"IsArray",                   Global_IsArray, 0, 1},
    {L"IsDate",                    Global_IsDate, 0, 1},
    {L"IsEmpty",                   Global_IsEmpty, 0, 1},
    {L"IsNull",                    Global_IsNull, 0, 1},
    {L"IsNumeric",                 Global_IsNumeric, 0, 1},
    {L"IsObject",                  Global_IsObject, 0, 1},
    {L"Join",                      Global_Join, 0, 1, 2},
    {L"LBound",                    Global_LBound, 0, 1, 2},
    {L"LCase",                     Global_LCase, 0, 1},
    {L"Left",                      Global_Left, 0, 2},
    {L"LeftB",                     Global_LeftB, 0, 2},
    {L"Len",                       Global_Len, 0, 1},
    {L"LenB",                      Global_LenB, 0, 1},
    {L"LoadPicture",               Global_LoadPicture, 0, 1},
    {L"Log",                       Global_Log, 0, 1},
    {L"LTrim",                     Global_LTrim, 0, 1},
    {L"Mid",                       Global_Mid, 0, 2, 3},
    {L"MidB",                      Global_MidB, 0, 2, 3},
    {L"Minute",                    Global_Minute, 0, 1},
    {L"Month",                     Global_Month, 0, 1},
    {L"MonthName",                 Global_MonthName, 0, 1, 2},
    {L"MsgBox",                    Global_MsgBox, 0, 1, 5},
    {L"Now",                       Global_Now, 0, 0},
    {L"Oct",                       Global_Oct, 0, 1},
    {L"Randomize",                 Global_Randomize, 0, 0, 1},
    {L"Replace",                   Global_Replace, 0, 3, 6},
    {L"RGB",                       Global_RGB, 0, 3},
    {L"Right",                     Global_Right, 0, 2},
    {L"RightB",                    Global_RightB, 0, 2},
    {L"Rnd",                       Global_Rnd, 0, 0, 1},
    {L"Round",                     Global_Round, 0, 1, 2},
    {L"RTrim",                     Global_RTrim, 0, 1},
    {L"ScriptEngine",              Global_ScriptEngine, 0, 0},
    {L"ScriptEngineBuildVersion",  Global_ScriptEngineBuildVersion, 0, 0},
    {L"ScriptEngineMajorVersion",  Global_ScriptEngineMajorVersion, 0, 0},
    {L"ScriptEngineMinorVersion",  Global_ScriptEngineMinorVersion, 0, 0},
    {L"Second",                    Global_Second, 0, 1},
    {L"SetLocale",                 Global_SetLocale, 0, 0, 1},
    {L"Sgn",                       Global_Sgn, 0, 1},
    {L"Sin",                       Global_Sin, 0, 1},
    {L"Space",                     Global_Space, 0, 1},
    {L"Split",                     Global_Split, 0, 1, 4},
    {L"Sqr",                       Global_Sqr, 0, 1},
    {L"StrComp",                   Global_StrComp, 0, 2, 3},
    {L"String",                    Global_String, 0, 0, 2},
    {L"StrReverse",                Global_StrReverse, 0, 1},
    {L"Tan",                       Global_Tan, 0, 1},
    {L"Time",                      Global_Time, 0, 0},
    {L"Timer",                     Global_Timer, 0, 0},
    {L"TimeSerial",                Global_TimeSerial, 0, 3},
    {L"TimeValue",                 Global_TimeValue, 0, 1},
    {L"Trim",                      Global_Trim, 0, 1},
    {L"TypeName",                  Global_TypeName, 0, 1},
    {L"UBound",                    Global_UBound, 0, 1, 2},
    {L"UCase",                     Global_UCase, 0, 1},
    {L"Unescape",                  Global_Unescape, 0, 1},
    {L"VarType",                   Global_VarType, 0, 1},
    {L"vbAbort",                   NULL, BP_GET, VT_I2, IDABORT},
    {L"vbAbortRetryIgnore",        NULL, BP_GET, VT_I2, MB_ABORTRETRYIGNORE},
    {L"vbApplicationModal",        NULL, BP_GET, VT_I2, MB_APPLMODAL},
    {L"vbArray",                   NULL, BP_GET, VT_I2, VT_ARRAY},
    {L"vbBinaryCompare",           NULL, BP_GET, VT_I2, 0},
    {L"vbBlack",                   NULL, BP_GET, VT_I4, 0x000000},
    {L"vbBlue",                    NULL, BP_GET, VT_I4, 0xff0000},
    {L"vbBoolean",                 NULL, BP_GET, VT_I2, VT_BOOL},
    {L"vbByte",                    NULL, BP_GET, VT_I2, VT_UI1},
    {L"vbCancel",                  NULL, BP_GET, VT_I2, IDCANCEL},
    {L"vbCr",                      NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCr},
    {L"vbCritical",                NULL, BP_GET, VT_I2, MB_ICONHAND},
    {L"vbCrLf",                    NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCrLf},
    {L"vbCurrency",                NULL, BP_GET, VT_I2, VT_CY},
    {L"vbCyan",                    NULL, BP_GET, VT_I4, 0xffff00},
    {L"vbDatabaseCompare",         NULL, BP_GET, VT_I2, 2},
    {L"vbDataObject",              NULL, BP_GET, VT_I2, VT_UNKNOWN},
    {L"vbDate",                    NULL, BP_GET, VT_I2, VT_DATE},
    {L"vbDecimal",                 NULL, BP_GET, VT_I2, VT_DECIMAL},
    {L"vbDefaultButton1",          NULL, BP_GET, VT_I2, MB_DEFBUTTON1},
    {L"vbDefaultButton2",          NULL, BP_GET, VT_I2, MB_DEFBUTTON2},
    {L"vbDefaultButton3",          NULL, BP_GET, VT_I2, MB_DEFBUTTON3},
    {L"vbDefaultButton4",          NULL, BP_GET, VT_I2, MB_DEFBUTTON4},
    {L"vbDouble",                  NULL, BP_GET, VT_I2, VT_R8},
    {L"vbEmpty",                   NULL, BP_GET, VT_I2, VT_EMPTY},
    {L"vbError",                   NULL, BP_GET, VT_I2, VT_ERROR},
    {L"vbExclamation",             NULL, BP_GET, VT_I2, MB_ICONEXCLAMATION},
    {L"vbFalse",                   NULL, BP_GET, VT_I2, VARIANT_FALSE},
    {L"vbFirstFourDays",           NULL, BP_GET, VT_I2, 2},
    {L"vbFirstFullWeek",           NULL, BP_GET, VT_I2, 3},
    {L"vbFirstJan1",               NULL, BP_GET, VT_I2, 1},
    {L"vbFormFeed",                NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbFormFeed},
    {L"vbFriday",                  NULL, BP_GET, VT_I2, 6},
    {L"vbGeneralDate",             NULL, BP_GET, VT_I2, 0},
    {L"vbGreen",                   NULL, BP_GET, VT_I4, 0x00ff00},
    {L"vbIgnore",                  NULL, BP_GET, VT_I2, IDIGNORE},
    {L"vbInformation",             NULL, BP_GET, VT_I2, MB_ICONASTERISK},
    {L"vbInteger",                 NULL, BP_GET, VT_I2, VT_I2},
    {L"vbLf",                      NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbLf},
    {L"vbLong",                    NULL, BP_GET, VT_I2, VT_I4},
    {L"vbLongDate",                NULL, BP_GET, VT_I2, 1},
    {L"vbLongTime",                NULL, BP_GET, VT_I2, 3},
    {L"vbMagenta",                 NULL, BP_GET, VT_I4, 0xff00ff},
    {L"vbMonday",                  NULL, BP_GET, VT_I2, 2},
    {L"vbMsgBoxHelpButton",        NULL, BP_GET, VT_I4, MB_HELP},
    {L"vbMsgBoxRight",             NULL, BP_GET, VT_I4, MB_RIGHT},
    {L"vbMsgBoxRtlReading",        NULL, BP_GET, VT_I4, MB_RTLREADING},
    {L"vbMsgBoxSetForeground",     NULL, BP_GET, VT_I4, MB_SETFOREGROUND},
    {L"vbNewLine",                 NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNewLine},
    {L"vbNo",                      NULL, BP_GET, VT_I2, IDNO},
    {L"vbNull",                    NULL, BP_GET, VT_I2, VT_NULL},
    {L"vbNullChar",                NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullChar},
    {L"vbNullString",              NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullString},
    {L"vbObject",                  NULL, BP_GET, VT_I2, VT_DISPATCH},
    {L"vbObjectError",             NULL, BP_GET, VT_I4, 0x80040000},
    {L"vbOK",                      NULL, BP_GET, VT_I2, IDOK},
    {L"vbOKCancel",                NULL, BP_GET, VT_I2, MB_OKCANCEL},
    {L"vbOKOnly",                  NULL, BP_GET, VT_I2, MB_OK},
    {L"vbQuestion",                NULL, BP_GET, VT_I2, MB_ICONQUESTION},
    {L"vbRed",                     NULL, BP_GET, VT_I4, 0x0000ff},
    {L"vbRetry",                   NULL, BP_GET, VT_I2, IDRETRY},
    {L"vbRetryCancel",             NULL, BP_GET, VT_I2, MB_RETRYCANCEL},
    {L"vbSaturday",                NULL, BP_GET, VT_I2, 7},
    {L"vbShortDate",               NULL, BP_GET, VT_I2, 2},
    {L"vbShortTime",               NULL, BP_GET, VT_I2, 4},
    {L"vbSingle",                  NULL, BP_GET, VT_I2, VT_R4},
    {L"vbString",                  NULL, BP_GET, VT_I2, VT_BSTR},
    {L"vbSunday",                  NULL, BP_GET, VT_I2, 1},
    {L"vbSystemModal",             NULL, BP_GET, VT_I2, MB_SYSTEMMODAL},
    {L"vbTab",                     NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbTab},
    {L"vbTextCompare",             NULL, BP_GET, VT_I2, 1},
    {L"vbThursday",                NULL, BP_GET, VT_I2, 5},
    {L"vbTrue",                    NULL, BP_GET, VT_I2, VARIANT_TRUE},
    {L"vbTuesday",                 NULL, BP_GET, VT_I2, 3},
    {L"vbUseDefault",              NULL, BP_GET, VT_I2, -2},
    {L"vbUseSystem",               NULL, BP_GET, VT_I2, 0},
    {L"vbUseSystemDayOfWeek",      NULL, BP_GET, VT_I2, 0},
    {L"vbVariant",                 NULL, BP_GET, VT_I2, VT_VARIANT},
    {L"vbVerticalTab",             NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbVerticalTab},
    {L"vbWednesday",               NULL, BP_GET, VT_I2, 4},
    {L"vbWhite",                   NULL, BP_GET, VT_I4, 0xffffff},
    {L"vbYellow",                  NULL, BP_GET, VT_I4, 0x00ffff},
    {L"vbYes",                     NULL, BP_GET, VT_I2, IDYES},
    {L"vbYesNo",                   NULL, BP_GET, VT_I2, MB_YESNO},
    {L"vbYesNoCancel",             NULL, BP_GET, VT_I2, MB_YESNOCANCEL},
    {L"Weekday",                   Global_Weekday, 0, 1, 2},
    {L"WeekdayName",               Global_WeekdayName, 0, 1, 3},
    {L"Year",                      Global_Year, 0, 1}
};

static HRESULT err_string_prop(BSTR *prop, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    if(!args_cnt)
        return return_string(res, *prop ? *prop : L"");

    hres = to_string(args, &str);
    if(FAILED(hres))
        return hres;

    SysFreeString(*prop);
    *prop = str;
    return S_OK;
}

static HRESULT Err_Description(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrDescription, args, args_cnt, res);
}

static HRESULT Err_HelpContext(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    if(args_cnt) {
        FIXME("setter not implemented\n");
        return E_NOTIMPL;
    }

    return return_int(res, This->ctx->ei.dwHelpContext);
}

static HRESULT Err_HelpFile(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrHelpFile, args, args_cnt, res);
}

static HRESULT Err_Number(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;

    TRACE("\n");

    if(args_cnt) {
        FIXME("setter not implemented\n");
        return E_NOTIMPL;
    }

    hres = This->ctx->ei.scode;
    return return_int(res, HRESULT_FACILITY(hres) == FACILITY_VBS ? HRESULT_CODE(hres) : hres);
}

static HRESULT Err_Source(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrSource, args, args_cnt, res);
}

static HRESULT Err_Clear(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    clear_ei(&This->ctx->ei);
    return S_OK;
}

static HRESULT Err_Raise(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR source = NULL, description = NULL, helpfile = NULL;
    int code,  helpcontext = 0;
    HRESULT hres;

    TRACE("%s %u...\n", debugstr_variant(args), args_cnt);

    hres = to_int(args, &code);
    if(FAILED(hres))
        return hres;
    if(code == 0 || code > 0xffff)
        return E_INVALIDARG;

    if(args_cnt >= 2)
        hres = to_string(args + 1, &source);
    if(args_cnt >= 3 && SUCCEEDED(hres))
        hres = to_string(args + 2, &description);
    if(args_cnt >= 4 && SUCCEEDED(hres))
        hres = to_string(args + 3, &helpfile);
    if(args_cnt >= 5 && SUCCEEDED(hres))
        hres = to_int(args + 4, &helpcontext);

    if(SUCCEEDED(hres)) {
        script_ctx_t *ctx = This->ctx;

        if(source) {
            SysFreeString(ctx->ei.bstrSource);
            ctx->ei.bstrSource = source;
        }
        if(description) {
            SysFreeString(ctx->ei.bstrDescription);
            ctx->ei.bstrDescription = description;
        }
        if(helpfile) {
            SysFreeString(ctx->ei.bstrHelpFile);
            ctx->ei.bstrHelpFile = helpfile;
        }
        if(args_cnt >= 5)
            ctx->ei.dwHelpContext = helpcontext;

        ctx->ei.scode = (code & ~0xffff) ? code : MAKE_VBSERROR(code);
        map_vbs_exception(&ctx->ei);

        hres = SCRIPT_E_RECORDED;
    }else {
        SysFreeString(source);
        SysFreeString(description);
        SysFreeString(helpfile);
    }

    return hres;
}

static const builtin_prop_t err_props[] = {
    {NULL,            Err_Number, BP_GETPUT},
    {L"Clear",        Err_Clear},
    {L"Description",  Err_Description, BP_GETPUT},
    {L"HelpContext",  Err_HelpContext, BP_GETPUT},
    {L"HelpFile",     Err_HelpFile, BP_GETPUT},
    {L"Number",       Err_Number, BP_GETPUT},
    {L"Raise",        Err_Raise, 0, 1, 5},
    {L"Source",       Err_Source, BP_GETPUT}
};

void detach_global_objects(script_ctx_t *ctx)
{
    if(ctx->err_obj) {
        ctx->err_obj->ctx = NULL;
        IDispatch_Release(&ctx->err_obj->IDispatch_iface);
        ctx->err_obj = NULL;
    }

    if(ctx->global_obj) {
        ctx->global_obj->ctx = NULL;
        IDispatch_Release(&ctx->global_obj->IDispatch_iface);
        ctx->global_obj = NULL;
    }
}

HRESULT init_global(script_ctx_t *ctx)
{
    HRESULT hres;

    hres = create_builtin_dispatch(ctx, global_props, ARRAY_SIZE(global_props), &ctx->global_obj);
    if(FAILED(hres))
        return hres;

    return create_builtin_dispatch(ctx, err_props, ARRAY_SIZE(err_props), &ctx->err_obj);
}
