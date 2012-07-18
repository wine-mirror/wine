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

#include "vbscript.h"
#include "vbscript_defs.h"

#include "mshtmhst.h"
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#define VB_E_CANNOT_CREATE_OBJ 0x800a01ad

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

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

static HRESULT return_bstr(VARIANT *res, const WCHAR *str)
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
    if((short)val == val)
        return return_short(res, val);

    if(res) {
        V_VT(res) = VT_I4;
        V_I4(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_null(VARIANT *res)
{
    if(res)
        V_VT(res) = VT_NULL;
    return S_OK;
}

static HRESULT to_int(VARIANT *v, int *ret)
{
    switch(V_VT(v)) {
    case VT_I2:
        *ret = V_I2(v);
        break;
    case VT_I4:
        *ret = V_I4(v);
        break;
    default:
        FIXME("not supported %s\n", debugstr_variant(v));
        return E_NOTIMPL;
    }

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

static IUnknown *create_object(script_ctx_t *ctx, const WCHAR *progid)
{
    IInternetHostSecurityManager *secmgr = NULL;
    IObjectWithSite *obj_site;
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

    hres = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void**)&obj_site);
    if(SUCCEEDED(hres)) {
        IUnknown *ax_site;

        ax_site = create_ax_site(ctx);
        if(ax_site) {
            hres = IObjectWithSite_SetSite(obj_site, ax_site);
            IUnknown_Release(ax_site);
        }
        IObjectWithSite_Release(obj_site);
        if(!ax_site || FAILED(hres)) {
            IUnknown_Release(obj);
            return NULL;
        }
    }

    return obj;
}

static HRESULT Global_CCur(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CInt(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CLng(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CBool(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CByte(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CDbl(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CSng(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CStr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static inline WCHAR hex_char(unsigned n)
{
    return n < 10 ? '0'+n : 'A'+n-10;
}

static HRESULT Global_Hex(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR buf[17], *ptr;
    DWORD n;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_I4:
        n = V_I4(arg);
        break;
    case VT_EMPTY:
        n = 0;
        break;
    case VT_NULL:
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        FIXME("unsupported type %s\n", debugstr_variant(arg));
        return E_NOTIMPL;
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

    return return_bstr(res, ptr);
}

static HRESULT Global_Oct(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_VarType(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsEmpty(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_EMPTY ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNull(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNumeric(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsArray(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_DISPATCH ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_Ant(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Cos(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Sin(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Tan(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Exp(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Log(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Sqr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Randomize(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Rnd(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Timer(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LBound(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_UBound(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_RGB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Len(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LenB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Left(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LeftB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Right(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_RightB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Mid(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int len = -1, start, str_len;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    switch(args_cnt) {
    case 3:
        hres = to_int(args, &len);
        if(FAILED(hres))
            return hres;

        if(len < 0) {
            FIXME("len = %d\n", len);
            return E_FAIL;
        }
        /* fallthrough */
    case 2:
        hres = to_int(args+args_cnt-2, &start);
        if(FAILED(hres))
            return hres;

        if(V_VT(args+args_cnt-1) != VT_BSTR) {
            FIXME("args[0] = %s\n", debugstr_variant(args+args_cnt-1));
            return E_NOTIMPL;
        }

        str = V_BSTR(args+args_cnt-1);
        break;
    default:
        assert(0);
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
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT Global_MidB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrComp(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LCase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = tolowerW(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_UCase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = toupperW(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_LTrim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_RTrim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Trim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Space(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_String(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InStr(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    VARIANT *startv, *str1v, *str2v;
    BSTR str1, str2;
    int start, ret;
    HRESULT hres;

    TRACE("\n");

    switch(args_cnt) {
    case 2:
        startv = NULL;
        str1v = args+1;
        str2v = args;
        break;
    case 3:
        startv = args+2;
        str1v = args+1;
        str2v = args;
        break;
    case 4:
        FIXME("unsupported compare argument %s\n", debugstr_variant(args));
        return E_NOTIMPL;
    default:
        assert(0);
    }

    if(startv) {
        hres = to_int(startv, &start);
        if(FAILED(hres))
            return hres;
        if(--start < 0) {
            FIXME("start %d\n", start);
            return E_FAIL;
        }
    }else {
        start = 0;
    }

    if(V_VT(str1v) == VT_NULL || V_VT(str2v) == VT_NULL)
        return return_null(res);

    if(V_VT(str1v) != VT_BSTR) {
        FIXME("Unsupported str1 type %s\n", debugstr_variant(str1v));
        return E_NOTIMPL;
    }
    str1 = V_BSTR(str1v);

    if(V_VT(str2v) != VT_BSTR) {
        FIXME("Unsupported str2 type %s\n", debugstr_variant(str2v));
        return E_NOTIMPL;
    }
    str2 = V_BSTR(str2v);

    if(start < SysStringLen(str1)) {
        WCHAR *ptr;

        ptr = strstrW(str1+start, str2);
        ret = ptr ? ptr-str1+1 : 0;
    }else {
        ret = 0;
    }

    return return_int(res, ret);
}

static HRESULT Global_InStrB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_AscB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Asc(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Chr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_AscW(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrW(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Abs(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Fix(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Int(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Sgn(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Now(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Date(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Time(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Day(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Month(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Weekday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Year(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Hour(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Minute(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Second(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateValue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeValue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateSerial(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeSerial(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InputBox(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_MsgBox(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CreateObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    IUnknown *obj;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    if(V_VT(arg) != VT_BSTR) {
        FIXME("non-bstr arg\n");
        return E_INVALIDARG;
    }

    obj = create_object(This->desc->ctx, V_BSTR(arg));
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

static HRESULT Global_GetObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateAdd(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateDiff(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DatePart(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TypeName(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Array(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Erase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Filter(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Join(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Split(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Replace(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrReverse(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InStrRev(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LoadPicture(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngine(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngineMajorVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngineMinorVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngineBuildVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatNumber(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatCurrency(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatPercent(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatDateTime(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_WeekdayName(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_MonthName(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Round(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Escape(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Unescape(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Eval(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Execute(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ExecuteGlobal(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_GetRef(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbUseSystem(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbUseSystemDayOfWeek(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbSunday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 1);
}

static HRESULT Global_vbMonday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 2);
}

static HRESULT Global_vbTuesday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 3);
}

static HRESULT Global_vbWednesday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 4);
}

static HRESULT Global_vbThursday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 5);
}

static HRESULT Global_vbFriday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 6);
}

static HRESULT Global_vbSaturday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return return_short(res, 7);
}

static HRESULT Global_vbFirstJan1(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbFirstFourDays(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbFirstFullWeek(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbOKOnly(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbOKCancel(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbAbortRetryIgnore(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbYesNoCancel(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbYesNo(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbRetryCancel(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCritical(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbQuestion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbExclamation(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbInformation(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDefaultButton1(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDefaultButton2(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDefaultButton3(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDefaultButton4(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbApplicationModal(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbSystemModal(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbOK(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCancel(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbAbort(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbRetry(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbIgnore(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbYes(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbNo(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbEmpty(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbNull(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbInteger(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbLong(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbSingle(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDouble(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCurrency(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbString(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbError(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbBoolean(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbVariant(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDataObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDecimal(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbByte(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbArray(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbTrue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbFalse(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbUseDefault(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbBinaryCompare(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbTextCompare(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbDatabaseCompare(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbGeneralDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbLongDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbShortDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbLongTime(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbShortTime(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbObjectError(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbBlack(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbBlue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCyan(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbGreen(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbMagenta(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbRed(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbWhite(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbYellow(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbCrLf(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbNewLine(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbFormFeed(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbLf(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbNullChar(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbNullString(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbTab(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbVerticalTab(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbMsgBoxHelpButton(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbMsgBoxSetForeground(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbMsgBoxRight(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_vbMsgBoxRtlReading(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t global_props[] = {
    {DISPID_GLOBAL_VBUSESYSTEM,        Global_vbUseSystem, BP_GET},
    {DISPID_GLOBAL_USESYSTEMDAYOFWEEK, Global_vbUseSystemDayOfWeek, BP_GET},
    {DISPID_GLOBAL_VBSUNDAY,           Global_vbSunday, BP_GET},
    {DISPID_GLOBAL_VBMONDAY,           Global_vbMonday, BP_GET},
    {DISPID_GLOBAL_VBTUESDAY,          Global_vbTuesday, BP_GET},
    {DISPID_GLOBAL_VBWEDNESDAY,        Global_vbWednesday, BP_GET},
    {DISPID_GLOBAL_VBTHURSDAY,         Global_vbThursday, BP_GET},
    {DISPID_GLOBAL_VBFRIDAY,           Global_vbFriday, BP_GET},
    {DISPID_GLOBAL_VBSATURDAY,         Global_vbSaturday, BP_GET},
    {DISPID_GLOBAL_VBFIRSTJAN1,        Global_vbFirstJan1, BP_GET},
    {DISPID_GLOBAL_VBFIRSTFOURDAYS,    Global_vbFirstFourDays, BP_GET},
    {DISPID_GLOBAL_VBFIRSTFULLWEEK,    Global_vbFirstFullWeek, BP_GET},
    {DISPID_GLOBAL_VBOKONLY,           Global_vbOKOnly, BP_GET},
    {DISPID_GLOBAL_VBOKCANCEL,         Global_vbOKCancel, BP_GET},
    {DISPID_GLOBAL_VBABORTRETRYIGNORE, Global_vbAbortRetryIgnore, BP_GET},
    {DISPID_GLOBAL_VBYESNOCANCEL,      Global_vbYesNoCancel, BP_GET},
    {DISPID_GLOBAL_VBYESNO,            Global_vbYesNo, BP_GET},
    {DISPID_GLOBAL_VBRETRYCANCEL,      Global_vbRetryCancel, BP_GET},
    {DISPID_GLOBAL_VBCRITICAL,         Global_vbCritical, BP_GET},
    {DISPID_GLOBAL_VBQUESTION,         Global_vbQuestion, BP_GET},
    {DISPID_GLOBAL_VBEXCLAMATION,      Global_vbExclamation, BP_GET},
    {DISPID_GLOBAL_VBINFORMATION,      Global_vbInformation, BP_GET},
    {DISPID_GLOBAL_VBDEFAULTBUTTON1,   Global_vbDefaultButton1, BP_GET},
    {DISPID_GLOBAL_VBDEFAULTBUTTON2,   Global_vbDefaultButton2, BP_GET},
    {DISPID_GLOBAL_VBDEFAULTBUTTON3,   Global_vbDefaultButton3, BP_GET},
    {DISPID_GLOBAL_VBDEFAULTBUTTON4,   Global_vbDefaultButton4, BP_GET},
    {DISPID_GLOBAL_VBAPPLICATIONMODAL, Global_vbApplicationModal, BP_GET},
    {DISPID_GLOBAL_VBSYSTEMMODAL,      Global_vbSystemModal, BP_GET},
    {DISPID_GLOBAL_VBOK,               Global_vbOK, BP_GET},
    {DISPID_GLOBAL_VBCANCEL,           Global_vbCancel, BP_GET},
    {DISPID_GLOBAL_VBABORT,            Global_vbAbort, BP_GET},
    {DISPID_GLOBAL_VBRETRY,            Global_vbRetry, BP_GET},
    {DISPID_GLOBAL_VBIGNORE,           Global_vbIgnore, BP_GET},
    {DISPID_GLOBAL_VBYES,              Global_vbYes, BP_GET},
    {DISPID_GLOBAL_VBNO,               Global_vbNo, BP_GET},
    {DISPID_GLOBAL_VBEMPTY,            Global_vbEmpty, BP_GET},
    {DISPID_GLOBAL_VBNULL,             Global_vbNull, BP_GET},
    {DISPID_GLOBAL_VBINTEGER,          Global_vbInteger, BP_GET},
    {DISPID_GLOBAL_VBLONG,             Global_vbLong, BP_GET},
    {DISPID_GLOBAL_VBSINGLE,           Global_vbSingle, BP_GET},
    {DISPID_GLOBAL_VBDOUBLE,           Global_vbDouble, BP_GET},
    {DISPID_GLOBAL_VBCURRENCY,         Global_vbCurrency, BP_GET},
    {DISPID_GLOBAL_VBDATE,             Global_vbDate, BP_GET},
    {DISPID_GLOBAL_VBSTRING,           Global_vbString, BP_GET},
    {DISPID_GLOBAL_VBOBJECT,           Global_vbObject, BP_GET},
    {DISPID_GLOBAL_VBERROR,            Global_vbError, BP_GET},
    {DISPID_GLOBAL_VBBOOLEAN,          Global_vbBoolean, BP_GET},
    {DISPID_GLOBAL_VBVARIANT,          Global_vbVariant, BP_GET},
    {DISPID_GLOBAL_VBDATAOBJECT,       Global_vbDataObject, BP_GET},
    {DISPID_GLOBAL_VBDECIMAL,          Global_vbDecimal, BP_GET},
    {DISPID_GLOBAL_VBBYTE,             Global_vbByte, BP_GET},
    {DISPID_GLOBAL_VBARRAY,            Global_vbArray, BP_GET},
    {DISPID_GLOBAL_VBTRUE,             Global_vbTrue, BP_GET},
    {DISPID_GLOBAL_VBFALSE,            Global_vbFalse, BP_GET},
    {DISPID_GLOBAL_VBUSEDEFAULT,       Global_vbUseDefault, BP_GET},
    {DISPID_GLOBAL_VBBINARYCOMPARE,    Global_vbBinaryCompare, BP_GET},
    {DISPID_GLOBAL_VBTEXTCOMPARE,      Global_vbTextCompare, BP_GET},
    {DISPID_GLOBAL_VBDATABASECOMPARE,  Global_vbDatabaseCompare, BP_GET},
    {DISPID_GLOBAL_VBGENERALDATE,      Global_vbGeneralDate, BP_GET},
    {DISPID_GLOBAL_VBLONGDATE,         Global_vbLongDate, BP_GET},
    {DISPID_GLOBAL_VBSHORTDATE,        Global_vbShortDate, BP_GET},
    {DISPID_GLOBAL_VBLONGTIME,         Global_vbLongTime, BP_GET},
    {DISPID_GLOBAL_VBSHORTTIME,        Global_vbShortTime, BP_GET},
    {DISPID_GLOBAL_VBOBJECTERROR,      Global_vbObjectError, BP_GET},
    {DISPID_GLOBAL_VBBLACK,            Global_vbBlack, BP_GET},
    {DISPID_GLOBAL_VBBLUE,             Global_vbBlue, BP_GET},
    {DISPID_GLOBAL_VBCYAN,             Global_vbCyan, BP_GET},
    {DISPID_GLOBAL_VBGREEN,            Global_vbGreen, BP_GET},
    {DISPID_GLOBAL_VBMAGENTA,          Global_vbMagenta, BP_GET},
    {DISPID_GLOBAL_VBRED,              Global_vbRed, BP_GET},
    {DISPID_GLOBAL_VBWHITE,            Global_vbWhite, BP_GET},
    {DISPID_GLOBAL_VBYELLOW,           Global_vbYellow, BP_GET},
    {DISPID_GLOBAL_VBCR,               Global_vbCr, BP_GET},
    {DISPID_GLOBAL_VBCRLF,             Global_vbCrLf, BP_GET},
    {DISPID_GLOBAL_VBNEWLINE,          Global_vbNewLine, BP_GET},
    {DISPID_GLOBAL_VBFORMFEED,         Global_vbFormFeed, BP_GET},
    {DISPID_GLOBAL_VBLF,               Global_vbLf, BP_GET},
    {DISPID_GLOBAL_VBNULLCHAR,         Global_vbNullChar, BP_GET},
    {DISPID_GLOBAL_VBNULLSTRING,       Global_vbNullString, BP_GET},
    {DISPID_GLOBAL_VBTAB,              Global_vbTab, BP_GET},
    {DISPID_GLOBAL_VBVERTICALTAB,      Global_vbVerticalTab, BP_GET},
    {DISPID_GLOBAL_CCUR,                      Global_CCur, 0, 1},
    {DISPID_GLOBAL_CINT,                      Global_CInt, 0, 1},
    {DISPID_GLOBAL_CLNG,                      Global_CLng, 0, 1},
    {DISPID_GLOBAL_CBOOL,                     Global_CBool, 0, 1},
    {DISPID_GLOBAL_CBYTE,                     Global_CByte, 0, 1},
    {DISPID_GLOBAL_CDATE,                     Global_CDate, 0, 1},
    {DISPID_GLOBAL_CDBL,                      Global_CDbl, 0, 1},
    {DISPID_GLOBAL_CSNG,                      Global_CSng, 0, 1},
    {DISPID_GLOBAL_CSTR,                      Global_CStr, 0, 1},
    {DISPID_GLOBAL_HEX,                       Global_Hex, 0, 1},
    {DISPID_GLOBAL_OCT,                       Global_Oct, 0, 1},
    {DISPID_GLOBAL_VARTYPE,                   Global_VarType, 0, 1},
    {DISPID_GLOBAL_ISDATE,                    Global_IsDate, 0, 1},
    {DISPID_GLOBAL_ISEMPTY,                   Global_IsEmpty, 0, 1},
    {DISPID_GLOBAL_ISNULL,                    Global_IsNull, 0, 1},
    {DISPID_GLOBAL_ISNUMERIC,                 Global_IsNumeric, 0, 1},
    {DISPID_GLOBAL_ISARRAY,                   Global_IsArray, 0, 1},
    {DISPID_GLOBAL_ISOBJECT,                  Global_IsObject, 0, 1},
    {DISPID_GLOBAL_ATN,                       Global_Ant, 0, 1},
    {DISPID_GLOBAL_COS,                       Global_Cos, 0, 1},
    {DISPID_GLOBAL_SIN,                       Global_Sin, 0, 1},
    {DISPID_GLOBAL_TAN,                       Global_Tan, 0, 1},
    {DISPID_GLOBAL_EXP,                       Global_Exp, 0, 1},
    {DISPID_GLOBAL_LOG,                       Global_Log, 0, 1},
    {DISPID_GLOBAL_SQR,                       Global_Sqr, 0, 1},
    {DISPID_GLOBAL_RANDOMIZE,                 Global_Randomize, 0, 1},
    {DISPID_GLOBAL_RND,                       Global_Rnd, 0, 1},
    {DISPID_GLOBAL_TIMER,                     Global_Timer, 0, 0},
    {DISPID_GLOBAL_LBOUND,                    Global_LBound, 0, 1},
    {DISPID_GLOBAL_UBOUND,                    Global_UBound, 0, 1},
    {DISPID_GLOBAL_RGB,                       Global_RGB, 0, 3},
    {DISPID_GLOBAL_LEN,                       Global_Len, 0, 1},
    {DISPID_GLOBAL_LENB,                      Global_LenB, 0, 1},
    {DISPID_GLOBAL_LEFT,                      Global_Left, 0, 2},
    {DISPID_GLOBAL_LEFTB,                     Global_LeftB, 0, 2},
    {DISPID_GLOBAL_RIGHT,                     Global_Right, 0, 2},
    {DISPID_GLOBAL_RIGHTB,                    Global_RightB, 0, 2},
    {DISPID_GLOBAL_MID,                       Global_Mid, 0, 2, 3},
    {DISPID_GLOBAL_MIDB,                      Global_MidB, 0, 2, 3},
    {DISPID_GLOBAL_STRCOMP,                   Global_StrComp, 0, 2, 3},
    {DISPID_GLOBAL_LCASE,                     Global_LCase, 0, 1},
    {DISPID_GLOBAL_UCASE,                     Global_UCase, 0, 1},
    {DISPID_GLOBAL_LTRIM,                     Global_LTrim, 0, 1},
    {DISPID_GLOBAL_RTRIM,                     Global_RTrim, 0, 1},
    {DISPID_GLOBAL_TRIM,                      Global_Trim, 0, 1},
    {DISPID_GLOBAL_SPACE,                     Global_Space, 0, 1},
    {DISPID_GLOBAL_STRING,                    Global_String, 0, 0, 2},
    {DISPID_GLOBAL_INSTR,                     Global_InStr, 0, 2, 4},
    {DISPID_GLOBAL_INSTRB,                    Global_InStrB, 0, 3, 4},
    {DISPID_GLOBAL_ASCB,                      Global_AscB, 0, 1},
    {DISPID_GLOBAL_CHRB,                      Global_ChrB, 0, 1},
    {DISPID_GLOBAL_ASC,                       Global_Asc, 0, 1},
    {DISPID_GLOBAL_CHR,                       Global_Chr, 0, 1},
    {DISPID_GLOBAL_ASCW,                      Global_AscW, 0, 1},
    {DISPID_GLOBAL_CHRW,                      Global_ChrW, 0, 1},
    {DISPID_GLOBAL_ABS,                       Global_Abs, 0, 1},
    {DISPID_GLOBAL_FIX,                       Global_Fix, 0, 1},
    {DISPID_GLOBAL_INT,                       Global_Int, 0, 1},
    {DISPID_GLOBAL_SGN,                       Global_Sgn, 0, 1},
    {DISPID_GLOBAL_NOW,                       Global_Now, 0, 0},
    {DISPID_GLOBAL_DATE,                      Global_Date, 0, 0},
    {DISPID_GLOBAL_TIME,                      Global_Time, 0, 0},
    {DISPID_GLOBAL_DAY,                       Global_Day, 0, 1},
    {DISPID_GLOBAL_MONTH,                     Global_Month, 0, 1},
    {DISPID_GLOBAL_WEEKDAY,                   Global_Weekday, 0, 1, 2},
    {DISPID_GLOBAL_YEAR,                      Global_Year, 0, 1},
    {DISPID_GLOBAL_HOUR,                      Global_Hour, 0, 1},
    {DISPID_GLOBAL_MINUTE,                    Global_Minute, 0, 1},
    {DISPID_GLOBAL_SECOND,                    Global_Second, 0, 1},
    {DISPID_GLOBAL_DATEVALUE,                 Global_DateValue, 0, 1},
    {DISPID_GLOBAL_TIMEVALUE,                 Global_TimeValue, 0, 1},
    {DISPID_GLOBAL_DATESERIAL,                Global_DateSerial, 0, 3},
    {DISPID_GLOBAL_TIMESERIAL,                Global_TimeSerial, 0, 3},
    {DISPID_GLOBAL_INPUTBOX,                  Global_InputBox, 0, 1, 7},
    {DISPID_GLOBAL_MSGBOX,                    Global_MsgBox, 0, 1, 5},
    {DISPID_GLOBAL_CREATEOBJECT,              Global_CreateObject, 0, 1},
    {DISPID_GLOBAL_GETOBJECT,                 Global_GetObject, 0, 0, 2},
    {DISPID_GLOBAL_DATEADD,                   Global_DateAdd, 0, 3},
    {DISPID_GLOBAL_DATEDIFF,                  Global_DateDiff, 0, 3, 5},
    {DISPID_GLOBAL_DATEPART,                  Global_DatePart, 0, 2, 4},
    {DISPID_GLOBAL_TYPENAME,                  Global_TypeName, 0, 1},
    {DISPID_GLOBAL_ARRAY,                     Global_Array, 0, 1},
    {DISPID_GLOBAL_ERASE,                     Global_Erase, 0, 1},
    {DISPID_GLOBAL_FILTER,                    Global_Filter, 0, 2, 4},
    {DISPID_GLOBAL_JOIN,                      Global_Join, 0, 1, 2},
    {DISPID_GLOBAL_SPLIT,                     Global_Split, 0, 1, 4},
    {DISPID_GLOBAL_REPLACE,                   Global_Replace, 0, 3, 6},
    {DISPID_GLOBAL_STRREVERSE,                Global_StrReverse, 0, 1},
    {DISPID_GLOBAL_INSTRREV,                  Global_InStrRev, 0, 2, 4},
    {DISPID_GLOBAL_LOADPICTURE,               Global_LoadPicture, 0, 1},
    {DISPID_GLOBAL_SCRIPTENGINE,              Global_ScriptEngine, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEMAJORVERSION,  Global_ScriptEngineMajorVersion, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEMINORVERSION,  Global_ScriptEngineMinorVersion, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEBUILDVERSION,  Global_ScriptEngineBuildVersion, 0, 0},
    {DISPID_GLOBAL_FORMATNUMBER,              Global_FormatNumber, 0, 1, 5},
    {DISPID_GLOBAL_FORMATCURRENCY,            Global_FormatCurrency, 0, 1, 5},
    {DISPID_GLOBAL_FORMATPERCENT,             Global_FormatPercent, 0, 1, 5},
    {DISPID_GLOBAL_FORMATDATETIME,            Global_FormatDateTime, 0, 1, 2},
    {DISPID_GLOBAL_WEEKDAYNAME,               Global_WeekdayName, 0, 3},
    {DISPID_GLOBAL_MONTHNAME,                 Global_MonthName, 0, 1, 2},
    {DISPID_GLOBAL_ROUND,                     Global_Round, 0, 1, 2},
    {DISPID_GLOBAL_ESCAPE,                    Global_Escape, 0, 1},
    {DISPID_GLOBAL_UNESCAPE,                  Global_Unescape, 0, 1},
    {DISPID_GLOBAL_EVAL,                      Global_Eval, 0, 1},
    {DISPID_GLOBAL_EXECUTE,                   Global_Execute, 0, 1},
    {DISPID_GLOBAL_EXECUTEGLOBAL,             Global_ExecuteGlobal, 0, 1},
    {DISPID_GLOBAL_GETREF,                    Global_GetRef, 0, 1},
    {DISPID_GLOBAL_VBMSGBOXHELPBUTTON,     Global_vbMsgBoxHelpButton, BP_GET},
    {DISPID_GLOBAL_VBMSGBOXSETFOREGROUND,  Global_vbMsgBoxSetForeground, BP_GET},
    {DISPID_GLOBAL_VBMSGBOXRIGHT,          Global_vbMsgBoxRight, BP_GET},
    {DISPID_GLOBAL_VBMSGBOXRTLREADING,     Global_vbMsgBoxRtlReading, BP_GET}
};

HRESULT init_global(script_ctx_t *ctx)
{
    HRESULT hres;

    ctx->global_desc.ctx = ctx;
    ctx->global_desc.builtin_prop_cnt = sizeof(global_props)/sizeof(*global_props);
    ctx->global_desc.builtin_props = global_props;

    hres = get_typeinfo(GlobalObj_tid, &ctx->global_desc.typeinfo);
    if(FAILED(hres))
        return hres;

    hres = create_vbdisp(&ctx->global_desc, &ctx->global_obj);
    if(FAILED(hres))
        return hres;

    ctx->script_desc.ctx = ctx;
    hres = create_vbdisp(&ctx->script_desc, &ctx->script_obj);
    if(FAILED(hres))
        return hres;

    return init_err(ctx);
}
