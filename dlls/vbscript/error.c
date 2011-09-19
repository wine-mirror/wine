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

HRESULT init_err(script_ctx_t *ctx)
{
    HRESULT hres;

    ctx->err_desc.ctx = ctx;

    hres = get_typeinfo(ErrObj_tid, &ctx->err_desc.typeinfo);
    if(FAILED(hres))
        return hres;

    return create_vbdisp(&ctx->err_desc, &ctx->err_obj);
}
