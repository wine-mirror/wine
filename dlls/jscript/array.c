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


static HRESULT Array_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_concat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_join(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_pop(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_push(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
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

static HRESULT Array_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
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
    FIXME("\n");
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
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
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
