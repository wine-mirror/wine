/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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

#include <math.h>

#include "jscript.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR parseW[] = {'p','a','r','s','e',0};
static const WCHAR stringifyW[] = {'s','t','r','i','n','g','i','f','y',0};

static HRESULT JSON_parse(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSON_stringify(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t JSON_props[] = {
    {parseW,     JSON_parse,     PROPF_METHOD|2},
    {stringifyW, JSON_stringify, PROPF_METHOD|3}
};

static const builtin_info_t JSON_info = {
    JSCLASS_JSON,
    {NULL, NULL, 0},
    sizeof(JSON_props)/sizeof(*JSON_props),
    JSON_props,
    NULL,
    NULL
};

HRESULT create_json(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *json;
    HRESULT hres;

    json = heap_alloc_zero(sizeof(*json));
    if(!json)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(json, ctx, &JSON_info, ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(json);
        return hres;
    }

    *ret = json;
    return S_OK;
}
