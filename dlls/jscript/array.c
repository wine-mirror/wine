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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    DispatchEx dispex;

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
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR unshiftW[] = {'u','n','s','h','i','f','t',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

const WCHAR default_separatorW[] = {',',0};

static HRESULT Array_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    ArrayInstance *This = (ArrayInstance*)dispex;

    TRACE("%p %d\n", This, This->length);

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        V_VT(retv) = VT_I4;
        V_I4(retv) = This->length;
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT Array_concat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT array_join(DispatchEx *array, LCID lcid, DWORD length, const WCHAR *sep, VARIANT *retv,
        jsexcept_t *ei, IServiceProvider *caller)
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
        hres = jsdisp_propget_idx(array, i, lcid, &var, ei, caller);
        if(FAILED(hres))
            break;

        if(V_VT(&var) != VT_EMPTY && V_VT(&var) != VT_NULL)
            hres = to_string(array->ctx, &var, ei, str_tab+i);
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
static HRESULT Array_join(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DWORD length;
    HRESULT hres;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_ARRAY)) {
        length = ((ArrayInstance*)dispex)->length;
    }else {
        FIXME("dispid is not Array\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp)) {
        BSTR sep;

        hres = to_string(dispex->ctx, dp->rgvarg + dp->cArgs-1, ei, &sep);
        if(FAILED(hres))
            return hres;

        hres = array_join(dispex, lcid, length, sep, retv, ei, caller);

        SysFreeString(sep);
    }else {
        hres = array_join(dispex, lcid, length, default_separatorW, retv, ei, caller);
    }

    return hres;
}

static HRESULT Array_pop(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.4.4.7 */
static HRESULT Array_push(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DWORD length = 0;
    int i, n;
    HRESULT hres;

    TRACE("\n");

    if(dispex->builtin_info->class == JSCLASS_ARRAY) {
        length = ((ArrayInstance*)dispex)->length;
    }else {
        FIXME("not Array this\n");
        return E_NOTIMPL;
    }

    n = dp->cArgs - dp->cNamedArgs;
    for(i=0; i < n; i++) {
        hres = jsdisp_propput_idx(dispex, length+i, lcid, get_arg(dp, i), ei, sp);
        if(FAILED(hres))
            return hres;
    }

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = length+n;
    }
    return S_OK;
}

static HRESULT Array_reverse(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_shift(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_slice(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_sort(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_splice(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.4.4.2 */
static HRESULT Array_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_ARRAY)) {
        WARN("not Array object\n");
        return E_FAIL;
    }

    return array_join(dispex, lcid, ((ArrayInstance*)dispex)->length, default_separatorW, retv, ei, sp);
}

static HRESULT Array_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_unshift(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void Array_destructor(DispatchEx *dispex)
{
    heap_free(dispex);
}

static void Array_on_put(DispatchEx *dispex, const WCHAR *name)
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
    {concatW,                Array_concat,               PROPF_METHOD},
    {hasOwnPropertyW,        Array_hasOwnProperty,       PROPF_METHOD},
    {isPrototypeOfW,         Array_isPrototypeOf,        PROPF_METHOD},
    {joinW,                  Array_join,                 PROPF_METHOD},
    {lengthW,                Array_length,               0},
    {popW,                   Array_pop,                  PROPF_METHOD},
    {propertyIsEnumerableW,  Array_propertyIsEnumerable, PROPF_METHOD},
    {pushW,                  Array_push,                 PROPF_METHOD},
    {reverseW,               Array_reverse,              PROPF_METHOD},
    {shiftW,                 Array_shift,                PROPF_METHOD},
    {sliceW,                 Array_slice,                PROPF_METHOD},
    {sortW,                  Array_sort,                 PROPF_METHOD},
    {spliceW,                Array_splice,               PROPF_METHOD},
    {toLocaleStringW,        Array_toLocaleString,       PROPF_METHOD},
    {toStringW,              Array_toString,             PROPF_METHOD},
    {unshiftW,               Array_unshift,              PROPF_METHOD},
    {valueOfW,               Array_valueOf,              PROPF_METHOD}
};

static const builtin_info_t Array_info = {
    JSCLASS_ARRAY,
    {NULL, Array_value, 0},
    sizeof(Array_props)/sizeof(*Array_props),
    Array_props,
    Array_destructor,
    Array_on_put
};

static HRESULT ArrayConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *obj;
    VARIANT *arg_var;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        if(arg_cnt(dp) == 1 && V_VT((arg_var = get_arg(dp, 0))) == VT_I4) {
            if(V_I4(arg_var) < 0) {
                FIXME("throw RangeError\n");
                return E_FAIL;
            }

            hres = create_array(dispex->ctx, V_I4(arg_var), &obj);
            if(FAILED(hres))
                return hres;

            V_VT(retv) = VT_DISPATCH;
            V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(obj);
            return S_OK;
        }

        hres = create_array(dispex->ctx, arg_cnt(dp), &obj);
        if(FAILED(hres))
            return hres;

        for(i=0; i < arg_cnt(dp); i++) {
            hres = jsdisp_propput_idx(obj, i, lcid, get_arg(dp, i), ei, caller);
            if(FAILED(hres))
                break;
        }
        if(FAILED(hres)) {
            jsdisp_release(obj);
            return hres;
        }

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(obj);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_array(script_ctx_t *ctx, BOOL use_constr, ArrayInstance **ret)
{
    ArrayInstance *array = heap_alloc_zero(sizeof(ArrayInstance));
    HRESULT hres;

    if(use_constr)
        hres = init_dispex_from_constr(&array->dispex, ctx, &Array_info, ctx->array_constr);
    else
        hres = init_dispex(&array->dispex, ctx, &Array_info, NULL);

    if(FAILED(hres)) {
        heap_free(array);
        return hres;
    }

    *ret = array;
    return S_OK;
}

HRESULT create_array_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, FALSE, &array);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, ArrayConstr_value, PROPF_CONSTR, &array->dispex, ret);

    IDispatchEx_Release(_IDispatchEx_(&array->dispex));
    return hres;
}

HRESULT create_array(script_ctx_t *ctx, DWORD length, DispatchEx **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, TRUE, &array);
    if(FAILED(hres))
        return hres;

    array->length = length;

    *ret = &array->dispex;
    return S_OK;
}
