/*
 * Copyright 2024 Gabriel Ivăncescu for CodeWeavers
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


#include <limits.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    DWORD size;
    DECLSPEC_ALIGN(sizeof(double)) BYTE buf[];
} ArrayBufferInstance;

static inline ArrayBufferInstance *arraybuf_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, ArrayBufferInstance, dispex);
}

static HRESULT ArrayBuffer_get_byteLength(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(arraybuf_from_jsdisp(jsthis)->size);
    return S_OK;
}

static HRESULT ArrayBuffer_slice(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("not implemented\n");

    return E_NOTIMPL;
}

static const builtin_prop_t ArrayBuffer_props[] = {
    {L"byteLength",            NULL, 0,                    ArrayBuffer_get_byteLength},
    {L"slice",                 ArrayBuffer_slice,          PROPF_METHOD|2},
};

static const builtin_info_t ArrayBuffer_info = {
    JSCLASS_ARRAYBUFFER,
    NULL,
    ARRAY_SIZE(ArrayBuffer_props),
    ArrayBuffer_props,
    NULL,
    NULL
};

static const builtin_prop_t ArrayBufferInst_props[] = {
    {L"byteLength",            NULL, 0,                    ArrayBuffer_get_byteLength},
};

static const builtin_info_t ArrayBufferInst_info = {
    JSCLASS_ARRAYBUFFER,
    NULL,
    ARRAY_SIZE(ArrayBufferInst_props),
    ArrayBufferInst_props,
    NULL,
    NULL
};

static HRESULT create_arraybuf(script_ctx_t *ctx, DWORD size, ArrayBufferInstance **ret)
{
    ArrayBufferInstance *arraybuf;
    HRESULT hres;

    if(!(arraybuf = calloc(1, FIELD_OFFSET(ArrayBufferInstance, buf[size]))))
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(&arraybuf->dispex, ctx, &ArrayBufferInst_info, ctx->arraybuf_constr);
    if(FAILED(hres)) {
        free(arraybuf);
        return hres;
    }
    arraybuf->size = size;

    *ret = arraybuf;
    return S_OK;
}

static HRESULT ArrayBufferConstr_isView(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("not implemented\n");

    return E_NOTIMPL;
}

static HRESULT ArrayBufferConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    ArrayBufferInstance *arraybuf;
    DWORD size = 0;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(argc) {
            double n;
            hres = to_integer(ctx, argv[0], &n);
            if(FAILED(hres))
                return hres;
            if(n < 0.0)
                return JS_E_INVALID_LENGTH;
            if(n > (UINT_MAX - FIELD_OFFSET(ArrayBufferInstance, buf[0])))
                return E_OUTOFMEMORY;
            size = n;
        }

        if(r) {
            hres = create_arraybuf(ctx, size, &arraybuf);
            if(FAILED(hres))
                return hres;
            *r = jsval_obj(&arraybuf->dispex);
        }
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t ArrayBufferConstr_props[] = {
    {L"isView",                ArrayBufferConstr_isView,   PROPF_METHOD|1},
};

static const builtin_info_t ArrayBufferConstr_info = {
    JSCLASS_FUNCTION,
    Function_value,
    ARRAY_SIZE(ArrayBufferConstr_props),
    ArrayBufferConstr_props,
    NULL,
    NULL
};

HRESULT init_arraybuf_constructors(script_ctx_t *ctx)
{
    ArrayBufferInstance *arraybuf;
    HRESULT hres;

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES5)
        return S_OK;

    if(!(arraybuf = calloc(1, FIELD_OFFSET(ArrayBufferInstance, buf[0]))))
        return E_OUTOFMEMORY;

    hres = init_dispex(&arraybuf->dispex, ctx, &ArrayBuffer_info, ctx->object_prototype);
    if(FAILED(hres)) {
        free(arraybuf);
        return hres;
    }

    hres = create_builtin_constructor(ctx, ArrayBufferConstr_value, L"ArrayBuffer", &ArrayBufferConstr_info,
                                      PROPF_CONSTR|1, &arraybuf->dispex, &ctx->arraybuf_constr);
    jsdisp_release(&arraybuf->dispex);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"ArrayBuffer", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->arraybuf_constr));

    return hres;
}
