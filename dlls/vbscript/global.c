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

#include "vbscript.h"
#include "vbscript_defs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static HRESULT Global_IsObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    if(V_VT(arg) == (VT_VARIANT|VT_BYREF))
        arg = V_VARIANTREF(arg);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_DISPATCH ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static const builtin_prop_t global_props[] = {
    {DISPID_GLOBAL_ISOBJECT,                  Global_IsObject, 0, 1}
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

    return S_OK;
}
