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
} RegExpInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static HRESULT RegExp_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void RegExp_destructor(DispatchEx *dispex)
{
    heap_free(dispex);
}

static const builtin_prop_t RegExp_props[] = {
    {hasOwnPropertyW,        RegExp_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         RegExp_isPrototypeOf,         PROPF_METHOD},
    {propertyIsEnumerableW,  RegExp_propertyIsEnumerable,  PROPF_METHOD},
    {toLocaleStringW,        RegExp_toLocaleString,        PROPF_METHOD},
    {toStringW,              RegExp_toString,              PROPF_METHOD}
};

static const builtin_info_t RegExp_info = {
    JSCLASS_REGEXP,
    {NULL, RegExp_value, 0},
    sizeof(RegExp_props)/sizeof(*RegExp_props),
    RegExp_props,
    RegExp_destructor,
    NULL
};

static HRESULT alloc_regexp(script_ctx_t *ctx, BOOL use_constr, RegExpInstance **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    regexp = heap_alloc_zero(sizeof(RegExpInstance));
    if(!regexp)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&regexp->dispex, ctx, &RegExp_info, ctx->regexp_constr);
    else
        hres = init_dispex(&regexp->dispex, ctx, &RegExp_info, NULL);

    if(FAILED(hres)) {
        heap_free(regexp);
        return hres;
    }

    *ret = regexp;
    return S_OK;
}

static HRESULT RegExpConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT create_regexp_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    hres = alloc_regexp(ctx, FALSE, &regexp);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, RegExpConstr_value, PROPF_CONSTR, &regexp->dispex, ret);

    jsdisp_release(&regexp->dispex);
    return hres;
}
