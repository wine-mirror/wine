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

#include "config.h"
#include "wine/port.h"

#include <math.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    DWORD length;
} ArrayInstance;

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR concatW[] = {'c','o','n','c','a','t',0};
static const WCHAR joinW[] = {'j','o','i','n',0};
static const WCHAR popW[] = {'p','o','p',0};
static const WCHAR pushW[] = {'p','u','s','h',0};
static const WCHAR reverseW[] = {'r','e','v','e','r','s','e',0};
static const WCHAR shiftW[] = {'s','h','i','f','t',0};
static const WCHAR sliceW[] = {'s','l','i','c','e',0};
static const WCHAR sortW[] = {'s','o','r','t',0};
static const WCHAR spliceW[] = {'s','p','l','i','c','e',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR unshiftW[] = {'u','n','s','h','i','f','t',0};

static const WCHAR default_separatorW[] = {',',0};

static inline ArrayInstance *array_from_vdisp(vdisp_t *vdisp)
{
    return (ArrayInstance*)vdisp->u.jsdisp;
}

static inline ArrayInstance *array_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_ARRAY) ? array_from_vdisp(jsthis) : NULL;
}

static HRESULT get_length(script_ctx_t *ctx, vdisp_t *vdisp, jsexcept_t *ei, jsdisp_t **jsthis, DWORD *ret)
{
    ArrayInstance *array;
    VARIANT var;
    HRESULT hres;

    array = array_this(vdisp);
    if(array) {
        *jsthis = &array->dispex;
        *ret = array->length;
        return S_OK;
    }

    if(!is_jsdisp(vdisp))
        return throw_type_error(ctx, ei, JS_E_JSCRIPT_EXPECTED, NULL);

    hres = jsdisp_propget_name(vdisp->u.jsdisp, lengthW, &var, ei);
    if(FAILED(hres))
        return hres;

    hres = to_uint32(ctx, &var, ei, ret);
    VariantClear(&var);
    if(FAILED(hres))
        return hres;

    *jsthis = vdisp->u.jsdisp;
    return S_OK;
}

static HRESULT set_length(jsdisp_t *obj, jsexcept_t *ei, DWORD length)
{
    VARIANT var;

    if(is_class(obj, JSCLASS_ARRAY)) {
        ((ArrayInstance*)obj)->length = length;
        return S_OK;
    }

    num_set_int(&var, length);
    return jsdisp_propput_name(obj, lengthW, &var, ei);
}

static WCHAR *idx_to_str(DWORD idx, WCHAR *ptr)
{
    if(!idx) {
        *ptr = '0';
        return ptr;
    }

    while(idx) {
        *ptr-- = '0' + (idx%10);
        idx /= 10;
    }

    return ptr+1;
}

static HRESULT Array_length(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    ArrayInstance *This = array_from_vdisp(jsthis);

    TRACE("%p %d\n", This, This->length);

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        num_set_int(retv, This->length);
        break;
    case DISPATCH_PROPERTYPUT: {
        DOUBLE len = -1;
        DWORD i;
        HRESULT hres;

        hres = to_number(ctx, argv, ei, &len);
        if(FAILED(hres))
            return hres;

        len = floor(len);
        if(len!=(DWORD)len)
            return throw_range_error(ctx, ei, JS_E_INVALID_LENGTH, NULL);

        for(i=len; i<This->length; i++) {
            hres = jsdisp_delete_idx(&This->dispex, i);
            if(FAILED(hres))
                return hres;
        }

        This->length = len;
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT concat_array(jsdisp_t *array, ArrayInstance *obj, DWORD *len, jsexcept_t *ei)
{
    VARIANT var;
    DWORD i;
    HRESULT hres;

    for(i=0; i < obj->length; i++) {
        hres = jsdisp_get_idx(&obj->dispex, i, &var, ei);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            return hres;

        hres = jsdisp_propput_idx(array, *len+i, &var, ei);
        VariantClear(&var);
        if(FAILED(hres))
            return hres;
    }

    *len += obj->length;
    return S_OK;
}

static HRESULT concat_obj(jsdisp_t *array, IDispatch *obj, DWORD *len, jsexcept_t *ei)
{
    jsdisp_t *jsobj;
    VARIANT var;
    HRESULT hres;

    jsobj = iface_to_jsdisp((IUnknown*)obj);
    if(jsobj) {
        if(is_class(jsobj, JSCLASS_ARRAY)) {
            hres = concat_array(array, (ArrayInstance*)jsobj, len, ei);
            jsdisp_release(jsobj);
            return hres;
        }
        jsdisp_release(jsobj);
    }

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = obj;
    return jsdisp_propput_idx(array, (*len)++, &var, ei);
}

static HRESULT Array_concat(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *ret;
    DWORD len = 0;
    HRESULT hres;

    TRACE("\n");

    hres = create_array(ctx, 0, &ret);
    if(FAILED(hres))
        return hres;

    hres = concat_obj(ret, jsthis->u.disp, &len, ei);
    if(SUCCEEDED(hres)) {
        VARIANT *arg;
        DWORD i;

        for(i=0; i < argc; i++) {
            arg = argv+i;
            if(V_VT(arg) == VT_DISPATCH)
                hres = concat_obj(ret, V_DISPATCH(arg), &len, ei);
            else
                hres = jsdisp_propput_idx(ret, len++, arg, ei);
            if(FAILED(hres))
                break;
        }
    }

    if(FAILED(hres))
        return hres;

    if(retv)
        var_set_jsdisp(retv, ret);
    else
        jsdisp_release(ret);
    return S_OK;
}

static HRESULT array_join(script_ctx_t *ctx, jsdisp_t *array, DWORD length, const WCHAR *sep, VARIANT *retv, jsexcept_t *ei)
{
    BSTR *str_tab, ret = NULL;
    VARIANT var;
    DWORD i;
    HRESULT hres = E_FAIL;

    if(!length) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocStringLen(NULL, 0);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    str_tab = heap_alloc_zero(length * sizeof(BSTR));
    if(!str_tab)
        return E_OUTOFMEMORY;

    for(i=0; i < length; i++) {
        hres = jsdisp_get_idx(array, i, &var, ei);
        if(hres == DISP_E_UNKNOWNNAME) {
            hres = S_OK;
            continue;
        } else if(FAILED(hres))
            break;

        if(V_VT(&var) != VT_EMPTY && V_VT(&var) != VT_NULL)
            hres = to_string(ctx, &var, ei, str_tab+i);
        VariantClear(&var);
        if(FAILED(hres))
            break;
    }

    if(SUCCEEDED(hres)) {
        DWORD seplen = 0, len = 0;
        WCHAR *ptr;

        seplen = strlenW(sep);

        if(str_tab[0])
            len = SysStringLen(str_tab[0]);
        for(i=1; i < length; i++)
            len += seplen + SysStringLen(str_tab[i]);

        ret = SysAllocStringLen(NULL, len);
        if(ret) {
            DWORD tmplen = 0;

            if(str_tab[0]) {
                tmplen = SysStringLen(str_tab[0]);
                memcpy(ret, str_tab[0], tmplen*sizeof(WCHAR));
            }

            ptr = ret + tmplen;
            for(i=1; i < length; i++) {
                if(seplen) {
                    memcpy(ptr, sep, seplen*sizeof(WCHAR));
                    ptr += seplen;
                }

                if(str_tab[i]) {
                    tmplen = SysStringLen(str_tab[i]);
                    memcpy(ptr, str_tab[i], tmplen*sizeof(WCHAR));
                    ptr += tmplen;
                }
            }
            *ptr=0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    for(i=0; i < length; i++)
        SysFreeString(str_tab[i]);
    heap_free(str_tab);
    if(FAILED(hres))
        return hres;

    TRACE("= %s\n", debugstr_w(ret));

    if(retv) {
        if(!ret) {
            ret = SysAllocStringLen(NULL, 0);
            if(!ret)
                return E_OUTOFMEMORY;
        }

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }else {
        SysFreeString(ret);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.5 */
static HRESULT Array_join(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    DWORD length;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        BSTR sep;

        hres = to_string(ctx, argv, ei, &sep);
        if(FAILED(hres))
            return hres;

        hres = array_join(ctx, jsthis, length, sep, retv, ei);

        SysFreeString(sep);
    }else {
        hres = array_join(ctx, jsthis, length, default_separatorW, retv, ei);
    }

    return hres;
}

static HRESULT Array_pop(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    VARIANT val;
    DWORD length;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(!length) {
        hres = set_length(jsthis, ei, 0);
        if(FAILED(hres))
            return hres;

        if(retv)
            V_VT(retv) = VT_EMPTY;
        return S_OK;
    }

    length--;
    hres = jsdisp_get_idx(jsthis, length, &val, ei);
    if(SUCCEEDED(hres)) {
        hres = jsdisp_delete_idx(jsthis, length);
    } else if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(&val) = VT_EMPTY;
        hres = S_OK;
    } else
        return hres;

    if(SUCCEEDED(hres))
        hres = set_length(jsthis, ei, length);

    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(retv)
        *retv = val;
    else
        VariantClear(&val);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.7 */
static HRESULT Array_push(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    DWORD length = 0;
    unsigned i;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    for(i=0; i < argc; i++) {
        hres = jsdisp_propput_idx(jsthis, length+i, argv+i, ei);
        if(FAILED(hres))
            return hres;
    }

    hres = set_length(jsthis, ei, length+argc);
    if(FAILED(hres))
        return hres;

    if(retv)
        num_set_int(retv, length+argc);
    return S_OK;
}

static HRESULT Array_reverse(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    DWORD length, k, l;
    VARIANT v1, v2;
    HRESULT hres1, hres2;

    TRACE("\n");

    hres1 = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres1))
        return hres1;

    for(k=0; k<length/2; k++) {
        l = length-k-1;

        hres1 = jsdisp_get_idx(jsthis, k, &v1, ei);
        if(FAILED(hres1) && hres1!=DISP_E_UNKNOWNNAME)
            return hres1;

        hres2 = jsdisp_get_idx(jsthis, l, &v2, ei);
        if(FAILED(hres2) && hres2!=DISP_E_UNKNOWNNAME) {
            VariantClear(&v1);
            return hres2;
        }

        if(hres1 == DISP_E_UNKNOWNNAME)
            hres1 = jsdisp_delete_idx(jsthis, l);
        else
            hres1 = jsdisp_propput_idx(jsthis, l, &v1, ei);

        if(FAILED(hres1)) {
            VariantClear(&v1);
            VariantClear(&v2);
            return hres1;
        }

        if(hres2 == DISP_E_UNKNOWNNAME)
            hres2 = jsdisp_delete_idx(jsthis, k);
        else
            hres2 = jsdisp_propput_idx(jsthis, k, &v2, ei);

        if(FAILED(hres2)) {
            VariantClear(&v2);
            return hres2;
        }
    }

    if(retv) {
        jsdisp_addref(jsthis);
        var_set_jsdisp(retv, jsthis);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.9 */
static HRESULT Array_shift(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    DWORD length = 0, i;
    VARIANT v, ret;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(!length) {
        hres = set_length(jsthis, ei, 0);
        if(FAILED(hres))
            return hres;
    }

    if(!length) {
        if(retv)
            V_VT(retv) = VT_EMPTY;
        return S_OK;
    }

    hres = jsdisp_get_idx(jsthis, 0, &ret, ei);
    if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(&ret) = VT_EMPTY;
        hres = S_OK;
    }

    for(i=1; SUCCEEDED(hres) && i<length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &v, ei);
        if(hres == DISP_E_UNKNOWNNAME)
            hres = jsdisp_delete_idx(jsthis, i-1);
        else if(SUCCEEDED(hres))
            hres = jsdisp_propput_idx(jsthis, i-1, &v, ei);
    }

    if(SUCCEEDED(hres)) {
        hres = jsdisp_delete_idx(jsthis, length-1);
        if(SUCCEEDED(hres))
            hres = set_length(jsthis, ei, length-1);
    }

    if(SUCCEEDED(hres) && retv)
        *retv = ret;
    else
        VariantClear(&ret);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.10 */
static HRESULT Array_slice(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *arr, *jsthis;
    DOUBLE range;
    DWORD length, start, end, idx;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_number(ctx, argv, ei, &range);
        if(FAILED(hres))
            return hres;

        range = floor(range);
        if(-range>length || isnan(range)) start = 0;
        else if(range < 0) start = range+length;
        else if(range <= length) start = range;
        else start = length;
    }
    else start = 0;

    if(argc > 1) {
        hres = to_number(ctx, argv+1, ei, &range);
        if(FAILED(hres))
            return hres;

        range = floor(range);
        if(-range>length) end = 0;
        else if(range < 0) end = range+length;
        else if(range <= length) end = range;
        else end = length;
    }
    else end = length;

    hres = create_array(ctx, (end>start)?end-start:0, &arr);
    if(FAILED(hres))
        return hres;

    for(idx=start; idx<end; idx++) {
        VARIANT v;

        hres = jsdisp_get_idx(jsthis, idx, &v, ei);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;

        if(SUCCEEDED(hres)) {
            hres = jsdisp_propput_idx(arr, idx-start, &v, ei);
            VariantClear(&v);
        }

        if(FAILED(hres)) {
            jsdisp_release(arr);
            return hres;
        }
    }

    if(retv)
        var_set_jsdisp(retv, arr);
    else
        jsdisp_release(arr);

    return S_OK;
}

static HRESULT sort_cmp(script_ctx_t *ctx, jsdisp_t *cmp_func, VARIANT *v1, VARIANT *v2, jsexcept_t *ei, INT *cmp)
{
    HRESULT hres;

    if(cmp_func) {
        VARIANTARG args[2];
        double n;
        VARIANT res;

        args[0] = *v1;
        args[1] = *v2;

        hres = jsdisp_call_value(cmp_func, NULL, DISPATCH_METHOD, 2, args, &res, ei);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, &res, ei, &n);
        VariantClear(&res);
        if(FAILED(hres))
            return hres;

        if(n == 0)
            *cmp = 0;
        *cmp = n > 0.0 ? 1 : -1;
    }else if(V_VT(v1) == VT_EMPTY) {
        *cmp = V_VT(v2) == VT_EMPTY ? 0 : 1;
    }else if(V_VT(v2) == VT_EMPTY) {
        *cmp = -1;
    }else if(is_num_vt(V_VT(v1)) && is_num_vt(V_VT(v2))) {
        DOUBLE d = num_val(v1)-num_val(v2);
        if(d > 0.0)
            *cmp = 1;
        else
            *cmp = d < -0.0 ? -1 : 0;
    }else {
        BSTR x, y;

        hres = to_string(ctx, v1, ei, &x);
        if(FAILED(hres))
            return hres;

        hres = to_string(ctx, v2, ei, &y);
        if(SUCCEEDED(hres)) {
            *cmp = strcmpW(x, y);
            SysFreeString(y);
        }
        SysFreeString(x);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.11 */
static HRESULT Array_sort(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis, *cmp_func = NULL;
    VARIANT *vtab, **sorttab = NULL;
    DWORD length;
    DWORD i;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        WARN("invalid arg_cnt %d\n", argc);
        return E_FAIL;
    }

    if(argc == 1) {
        if(V_VT(argv) != VT_DISPATCH) {
            WARN("arg is not dispatch\n");
            return E_FAIL;
        }


        cmp_func = iface_to_jsdisp((IUnknown*)V_DISPATCH(argv));
        if(!cmp_func || !is_class(cmp_func, JSCLASS_FUNCTION)) {
            WARN("cmp_func is not a function\n");
            if(cmp_func)
                jsdisp_release(cmp_func);
            return E_FAIL;
        }
    }

    if(!length) {
        if(cmp_func)
            jsdisp_release(cmp_func);
        if(retv) {
            jsdisp_addref(jsthis);
            var_set_jsdisp(retv, jsthis);
        }
        return S_OK;
    }

    vtab = heap_alloc_zero(length * sizeof(VARIANT));
    if(vtab) {
        for(i=0; i<length; i++) {
            hres = jsdisp_get_idx(jsthis, i, vtab+i, ei);
            if(hres == DISP_E_UNKNOWNNAME) {
                V_VT(vtab+i) = VT_EMPTY;
                hres = S_OK;
            } else if(FAILED(hres)) {
                WARN("Could not get elem %d: %08x\n", i, hres);
                break;
            }
        }
    }else {
        hres = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hres)) {
        sorttab = heap_alloc(length*2*sizeof(VARIANT*));
        if(!sorttab)
            hres = E_OUTOFMEMORY;
    }

    /* merge-sort */
    if(SUCCEEDED(hres)) {
        VARIANT *tmpv, **tmpbuf;
        INT cmp;

        tmpbuf = sorttab + length;
        for(i=0; i < length; i++)
            sorttab[i] = vtab+i;

        for(i=0; i < length/2; i++) {
            hres = sort_cmp(ctx, cmp_func, sorttab[2*i+1], sorttab[2*i], ei, &cmp);
            if(FAILED(hres))
                break;

            if(cmp < 0) {
                tmpv = sorttab[2*i];
                sorttab[2*i] = sorttab[2*i+1];
                sorttab[2*i+1] = tmpv;
            }
        }

        if(SUCCEEDED(hres)) {
            DWORD k, a, b, bend;

            for(k=2; k < length; k *= 2) {
                for(i=0; i+k < length; i += 2*k) {
                    a = b = 0;
                    if(i+2*k <= length)
                        bend = k;
                    else
                        bend = length - (i+k);

                    memcpy(tmpbuf, sorttab+i, k*sizeof(VARIANT*));

                    while(a < k && b < bend) {
                        hres = sort_cmp(ctx, cmp_func, tmpbuf[a], sorttab[i+k+b], ei, &cmp);
                        if(FAILED(hres))
                            break;

                        if(cmp < 0) {
                            sorttab[i+a+b] = tmpbuf[a];
                            a++;
                        }else {
                            sorttab[i+a+b] = sorttab[i+k+b];
                            b++;
                        }
                    }

                    if(FAILED(hres))
                        break;

                    if(a < k)
                        memcpy(sorttab+i+a+b, tmpbuf+a, (k-a)*sizeof(VARIANT*));
                }

                if(FAILED(hres))
                    break;
            }
        }

        for(i=0; SUCCEEDED(hres) && i < length; i++)
            hres = jsdisp_propput_idx(jsthis, i, sorttab[i], ei);
    }

    if(vtab) {
        for(i=0; i < length; i++)
            VariantClear(vtab+i);
        heap_free(vtab);
    }
    heap_free(sorttab);
    if(cmp_func)
        jsdisp_release(cmp_func);

    if(FAILED(hres))
        return hres;

    if(retv) {
        jsdisp_addref(jsthis);
        var_set_jsdisp(retv, jsthis);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.12 */
static HRESULT Array_splice(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    DWORD length, start=0, delete_cnt=0, i, add_args = 0;
    jsdisp_t *ret_array = NULL, *jsthis;
    VARIANT v;
    double d;
    int n;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_integer(ctx, argv, ei, &d);
        if(FAILED(hres))
            return hres;

        if(is_int32(d)) {
            if((n = d) >= 0)
                start = min(n, length);
            else
                start = -n > length ? 0 : length + n;
        }else {
            start = d < 0.0 ? 0 : length;
        }
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv+1, ei, &d);
        if(FAILED(hres))
            return hres;

        if(is_int32(d)) {
            if((n = d) > 0)
                delete_cnt = min(n, length-start);
        }else if(d > 0.0) {
            delete_cnt = length-start;
        }

        add_args = argc-2;
    }

    if(retv) {
        hres = create_array(ctx, 0, &ret_array);
        if(FAILED(hres))
            return hres;

        for(i=0; SUCCEEDED(hres) && i < delete_cnt; i++) {
            hres = jsdisp_get_idx(jsthis, start+i, &v, ei);
            if(hres == DISP_E_UNKNOWNNAME)
                hres = S_OK;
            else if(SUCCEEDED(hres))
                hres = jsdisp_propput_idx(ret_array, i, &v, ei);
        }

        if(SUCCEEDED(hres)) {
            num_set_int(&v, delete_cnt);
            hres = jsdisp_propput_name(ret_array, lengthW, &v, ei);
        }
    }

    if(add_args < delete_cnt) {
        for(i = start; SUCCEEDED(hres) && i < length-delete_cnt; i++) {
            hres = jsdisp_get_idx(jsthis, i+delete_cnt, &v, ei);
            if(hres == DISP_E_UNKNOWNNAME)
                hres = jsdisp_delete_idx(jsthis, i+add_args);
            else if(SUCCEEDED(hres))
                hres = jsdisp_propput_idx(jsthis, i+add_args, &v, ei);
        }

        for(i=length; SUCCEEDED(hres) && i != length-delete_cnt+add_args; i--)
            hres = jsdisp_delete_idx(jsthis, i-1);
    }else if(add_args > delete_cnt) {
        for(i=length-delete_cnt; SUCCEEDED(hres) && i != start; i--) {
            hres = jsdisp_get_idx(jsthis, i+delete_cnt-1, &v, ei);
            if(hres == DISP_E_UNKNOWNNAME)
                hres = jsdisp_delete_idx(jsthis, i+add_args-1);
            else if(SUCCEEDED(hres))
                hres = jsdisp_propput_idx(jsthis, i+add_args-1, &v, ei);
        }
    }

    for(i=0; SUCCEEDED(hres) && i < add_args; i++)
        hres = jsdisp_propput_idx(jsthis, start+i, argv+i+2, ei);

    if(SUCCEEDED(hres)) {
        num_set_int(&v, length-delete_cnt+add_args);
        hres = jsdisp_propput_name(jsthis, lengthW, &v, ei);
    }

    if(FAILED(hres)) {
        if(ret_array)
            jsdisp_release(ret_array);
        return hres;
    }

    if(retv)
        var_set_jsdisp(retv, ret_array);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.2 */
static HRESULT Array_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    ArrayInstance *array;

    TRACE("\n");

    array = array_this(jsthis);
    if(!array)
        return throw_type_error(ctx, ei, JS_E_ARRAY_EXPECTED, NULL);

    return array_join(ctx, &array->dispex, array->length, default_separatorW, retv, ei);
}

static HRESULT Array_toLocaleString(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.4.4.13 */
static HRESULT Array_unshift(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    WCHAR buf[14], *buf_end, *str;
    DWORD i, length;
    VARIANT var;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, ei, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        buf_end = buf + sizeof(buf)/sizeof(WCHAR)-1;
        *buf_end-- = 0;
        i = length;

        while(i--) {
            str = idx_to_str(i, buf_end);

            hres = jsdisp_get_id(jsthis, str, 0, &id);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propget(jsthis, id, &var, ei);
                if(FAILED(hres))
                    return hres;

                hres = jsdisp_propput_idx(jsthis, i+argc, &var, ei);
                VariantClear(&var);
            }else if(hres == DISP_E_UNKNOWNNAME) {
                hres = IDispatchEx_DeleteMemberByDispID(vthis->u.dispex, id);
            }
        }

        if(FAILED(hres))
            return hres;
    }

    for(i=0; i<argc; i++) {
        hres = jsdisp_propput_idx(jsthis, i, argv+i, ei);
        if(FAILED(hres))
            return hres;
    }

    if(argc) {
        length += argc;
        hres = set_length(jsthis, ei, length);
        if(FAILED(hres))
            return hres;
    }

    if(retv) {
        if(ctx->version < 2)
            V_VT(retv) = VT_EMPTY;
        else
            num_set_int(retv, length);
    }
    return S_OK;
}

static HRESULT Array_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);
    case INVOKE_PROPERTYGET:
        return array_join(ctx, jsthis->u.jsdisp, array_from_vdisp(jsthis)->length, default_separatorW, retv, ei);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Array_destructor(jsdisp_t *dispex)
{
    heap_free(dispex);
}

static void Array_on_put(jsdisp_t *dispex, const WCHAR *name)
{
    ArrayInstance *array = (ArrayInstance*)dispex;
    const WCHAR *ptr = name;
    DWORD id = 0;

    if(!isdigitW(*ptr))
        return;

    while(*ptr && isdigitW(*ptr)) {
        id = id*10 + (*ptr-'0');
        ptr++;
    }

    if(*ptr)
        return;

    if(id >= array->length)
        array->length = id+1;
}

static const builtin_prop_t Array_props[] = {
    {concatW,                Array_concat,               PROPF_METHOD|1},
    {joinW,                  Array_join,                 PROPF_METHOD|1},
    {lengthW,                Array_length,               0},
    {popW,                   Array_pop,                  PROPF_METHOD},
    {pushW,                  Array_push,                 PROPF_METHOD|1},
    {reverseW,               Array_reverse,              PROPF_METHOD},
    {shiftW,                 Array_shift,                PROPF_METHOD},
    {sliceW,                 Array_slice,                PROPF_METHOD|2},
    {sortW,                  Array_sort,                 PROPF_METHOD|1},
    {spliceW,                Array_splice,               PROPF_METHOD|2},
    {toLocaleStringW,        Array_toLocaleString,       PROPF_METHOD},
    {toStringW,              Array_toString,             PROPF_METHOD},
    {unshiftW,               Array_unshift,              PROPF_METHOD|1},
};

static const builtin_info_t Array_info = {
    JSCLASS_ARRAY,
    {NULL, Array_value, 0},
    sizeof(Array_props)/sizeof(*Array_props),
    Array_props,
    Array_destructor,
    Array_on_put
};

static const builtin_prop_t ArrayInst_props[] = {
    {lengthW,                Array_length,               0}
};

static const builtin_info_t ArrayInst_info = {
    JSCLASS_ARRAY,
    {NULL, Array_value, 0},
    sizeof(ArrayInst_props)/sizeof(*ArrayInst_props),
    ArrayInst_props,
    Array_destructor,
    Array_on_put
};

static HRESULT ArrayConstr_value(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *obj;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(argc == 1 && V_VT(argv) == VT_I4) {
            if(V_I4(argv) < 0)
                return throw_range_error(ctx, ei, JS_E_INVALID_LENGTH, NULL);

            hres = create_array(ctx, V_I4(argv), &obj);
            if(FAILED(hres))
                return hres;

            var_set_jsdisp(retv, obj);
            return S_OK;
        }

        hres = create_array(ctx, argc, &obj);
        if(FAILED(hres))
            return hres;

        for(i=0; i < argc; i++) {
            hres = jsdisp_propput_idx(obj, i, argv+i, ei);
            if(FAILED(hres))
                break;
        }
        if(FAILED(hres)) {
            jsdisp_release(obj);
            return hres;
        }

        var_set_jsdisp(retv, obj);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_array(script_ctx_t *ctx, jsdisp_t *object_prototype, ArrayInstance **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    array = heap_alloc_zero(sizeof(ArrayInstance));
    if(!array)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&array->dispex, ctx, &Array_info, object_prototype);
    else
        hres = init_dispex_from_constr(&array->dispex, ctx, &ArrayInst_info, ctx->array_constr);

    if(FAILED(hres)) {
        heap_free(array);
        return hres;
    }

    *ret = array;
    return S_OK;
}

HRESULT create_array_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    static const WCHAR ArrayW[] = {'A','r','r','a','y',0};

    hres = alloc_array(ctx, object_prototype, &array);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, ArrayConstr_value, ArrayW, NULL, PROPF_CONSTR|1, &array->dispex, ret);

    jsdisp_release(&array->dispex);
    return hres;
}

HRESULT create_array(script_ctx_t *ctx, DWORD length, jsdisp_t **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, NULL, &array);
    if(FAILED(hres))
        return hres;

    array->length = length;

    *ret = &array->dispex;
    return S_OK;
}
