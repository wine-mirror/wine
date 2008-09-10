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
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    DispatchEx dispex;
    builtin_invoke_t value_proc;
    DWORD flags;
    source_elements_t *source;
    parameter_t *parameters;
    scope_chain_t *scope_chain;
    parser_ctx_t *parser;
    DWORD length;
} FunctionInstance;

static const WCHAR prototypeW[] = {'p','r','o','t','o','t', 'y', 'p','e',0};

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR applyW[] = {'a','p','p','l','y',0};
static const WCHAR callW[] = {'c','a','l','l',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] = {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static HRESULT Function_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FunctionInstance *This = (FunctionInstance*)dispex;

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

static HRESULT Function_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_apply(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_call(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void Function_destructor(DispatchEx *dispex)
{
    FunctionInstance *This = (FunctionInstance*)dispex;

    if(This->parser)
        parser_release(This->parser);
    if(This->scope_chain)
        scope_release(This->scope_chain);
    heap_free(This);
}

static const builtin_prop_t Function_props[] = {
    {applyW,                 Function_apply,                 PROPF_METHOD},
    {callW,                  Function_call,                  PROPF_METHOD},
    {hasOwnPropertyW,        Function_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Function_isPrototypeOf,         PROPF_METHOD},
    {lengthW,                Function_length,                0},
    {propertyIsEnumerableW,  Function_propertyIsEnumerable,  PROPF_METHOD},
    {toLocaleStringW,        Function_toLocaleString,        PROPF_METHOD},
    {toStringW,              Function_toString,              PROPF_METHOD},
    {valueOfW,               Function_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Function_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(Function_props)/sizeof(*Function_props),
    Function_props,
    Function_destructor,
    NULL
};

static HRESULT create_function(script_ctx_t *ctx, DWORD flags, DispatchEx *prototype, FunctionInstance **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    function = heap_alloc_zero(sizeof(FunctionInstance));
    if(!function)
        return E_OUTOFMEMORY;

    hres = init_dispex(&function->dispex, ctx, &Function_info, NULL);
    if(FAILED(hres))
        return hres;

    function->flags = flags;
    function->length = flags & PROPF_ARGMASK;

    if(prototype) {
        jsexcept_t jsexcept;
        VARIANT var;

        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(prototype);
        memset(&jsexcept, 0, sizeof(jsexcept));

        hres = jsdisp_propput_name(&function->dispex, prototypeW, ctx->lcid, &var, &jsexcept, NULL/*FIXME*/);
        if(FAILED(hres)) {
            IDispatchEx_Release(_IDispatchEx_(&function->dispex));
            return hres;
        }
    }

    *ret = function;
    return S_OK;
}

HRESULT create_builtin_function(script_ctx_t *ctx, builtin_invoke_t value_proc, DWORD flags,
        DispatchEx *prototype, DispatchEx **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    hres = create_function(ctx, flags, prototype, &function);
    if(FAILED(hres))
        return hres;

    function->value_proc = value_proc;

    *ret = &function->dispex;
    return S_OK;
}

HRESULT create_source_function(parser_ctx_t *ctx, parameter_t *parameters, source_elements_t *source,
        scope_chain_t *scope_chain, DispatchEx **ret)
{
    FunctionInstance *function;
    parameter_t *iter;
    DWORD length = 0;
    HRESULT hres;

    hres = create_function(ctx->script, PROPF_CONSTR, NULL, &function);
    if(FAILED(hres))
        return hres;

    function->source = source;
    function->parameters = parameters;

    if(scope_chain) {
        scope_addref(scope_chain);
        function->scope_chain = scope_chain;
    }

    parser_addref(ctx);
    function->parser = ctx;

    for(iter = parameters; iter; iter = iter->next)
        length++;
    function->length = length;

    *ret = &function->dispex;
    return S_OK;
}
