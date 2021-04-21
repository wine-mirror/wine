/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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
    jsdisp_t dispex;
} SetInstance;

static HRESULT Set_add(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT Set_clear(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT Set_delete(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT Set_forEach(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT Set_has(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT Set_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t Set_props[] = {
    {L"add",        Set_add,       PROPF_METHOD|1},
    {L"clear",      Set_clear,     PROPF_METHOD},
    {L"delete" ,    Set_delete,    PROPF_METHOD|1},
    {L"forEach",    Set_forEach,   PROPF_METHOD|1},
    {L"has",        Set_has,       PROPF_METHOD|1},
};

static const builtin_info_t Set_prototype_info = {
    JSCLASS_SET,
    {NULL, Set_value, 0},
    ARRAY_SIZE(Set_props),
    Set_props,
    NULL,
    NULL
};

static const builtin_info_t Set_info = {
    JSCLASS_SET,
    {NULL, Set_value, 0},
    0, NULL,
    NULL,
    NULL
};

static HRESULT Set_constructor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    SetInstance *set;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!(set = heap_alloc_zero(sizeof(*set))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&set->dispex, ctx, &Set_info, ctx->set_prototype);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(&set->dispex);
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

HRESULT init_set_constructor(script_ctx_t *ctx)
{
    jsdisp_t *constructor;
    HRESULT hres;

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES6)
        return S_OK;

    hres = create_dispex(ctx, &Set_prototype_info, ctx->object_prototype, &ctx->set_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, Set_constructor, L"Set", NULL,
                                      PROPF_CONSTR, ctx->set_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Set", PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    return hres;
}
