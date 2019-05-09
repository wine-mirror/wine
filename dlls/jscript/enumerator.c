/*
 * Copyright 2019 Andreas Maier
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

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
} EnumeratorInstance;

static const WCHAR atEndW[] = {'a','t','E','n','d',0};
static const WCHAR itemW[] = {'i','t','e','m',0};
static const WCHAR moveFirstW[] = {'m','o','v','e','F','i','r','s','t',0};
static const WCHAR moveNextW[] = {'m','o','v','e','N','e','x','t',0};

static void Enumerator_destructor(jsdisp_t *dispex)
{
    TRACE("Enumerator_destructor\n");

    heap_free(dispex);
}

static HRESULT Enumerator_atEnd(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("Enumerator_atEnd\n");

    return E_NOTIMPL;
}

static HRESULT Enumerator_item(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("Enumerator_item\n");

    return E_NOTIMPL;
}

static HRESULT Enumerator_moveFirst(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("Enumerator_moveFirst\n");

    return E_NOTIMPL;
}

static HRESULT Enumerator_moveNext(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("Enumerator_moveNext\n");

    return E_NOTIMPL;
}

static const builtin_prop_t Enumerator_props[] = {
    {atEndW,     Enumerator_atEnd,     PROPF_METHOD},
    {itemW,      Enumerator_item,      PROPF_METHOD},
    {moveFirstW, Enumerator_moveFirst, PROPF_METHOD},
    {moveNextW,  Enumerator_moveNext,  PROPF_METHOD},
};

static const builtin_info_t Enumerator_info = {
    JSCLASS_OBJECT,
    {NULL, NULL, 0},
    ARRAY_SIZE(Enumerator_props),
    Enumerator_props,
    NULL,
    NULL
};

static const builtin_info_t EnumeratorInst_info = {
    JSCLASS_OBJECT,
    {NULL, NULL, 0, NULL},
    0,
    NULL,
    Enumerator_destructor,
    NULL
};

static HRESULT EnumeratorConstr_value(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *obj;
    HRESULT hres;

    TRACE("EnumeratorConstr_value\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        if (argc != 1)
            return throw_syntax_error(ctx, JS_E_MISSING_ARG, NULL);

        hres = create_enumerator(ctx, &argv[0], &obj);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(obj);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_enumerator(script_ctx_t *ctx, jsdisp_t *object_prototype, EnumeratorInstance **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;

    enumerator = heap_alloc_zero(sizeof(EnumeratorInstance));
    if(!enumerator)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&enumerator->dispex, ctx, &Enumerator_info, object_prototype);
    else
        hres = init_dispex_from_constr(&enumerator->dispex, ctx, &EnumeratorInst_info,
                                       ctx->enumerator_constr);

    if(FAILED(hres)) {
        heap_free(enumerator);
        return hres;
    }

    *ret = enumerator;
    return S_OK;
}

static const builtin_info_t EnumeratorConstr_info = {
    JSCLASS_FUNCTION,
    DEFAULT_FUNCTION_VALUE,
    0,
    NULL,
    NULL,
    NULL
};

HRESULT create_enumerator_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;
    static const WCHAR EnumeratorW[] = {'E','n','u','m','e','r','a','t','o','r',0};

    hres = alloc_enumerator(ctx, object_prototype, &enumerator);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, EnumeratorConstr_value,
                                     EnumeratorW, &EnumeratorConstr_info,
                                     PROPF_CONSTR|7, &enumerator->dispex, ret);
    jsdisp_release(&enumerator->dispex);

    return hres;
}

HRESULT create_enumerator(script_ctx_t *ctx, jsval_t *argv, jsdisp_t **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;

    hres = alloc_enumerator(ctx, NULL, &enumerator);
    if(FAILED(hres))
        return hres;

    *ret = &enumerator->dispex;
    return S_OK;
}
