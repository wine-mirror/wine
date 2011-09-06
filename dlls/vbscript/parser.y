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

%{

#include "vbscript.h"
#include "parse.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);


#define YYLEX_PARAM ctx
#define YYPARSE_PARAM ctx

static int parser_error(const char*);

static void parse_complete(parser_ctx_t*);

%}

%pure_parser
%start Program

%token tEOF

%union {
    const WCHAR *string;
}

%token tNL

%%

Program : tEOF      { parse_complete(ctx); }

%%

static int parser_error(const char *str)
{
    return 0;
}

static void parse_complete(parser_ctx_t *ctx)
{
    ctx->parse_complete = TRUE;
}

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code)
{
    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + strlenW(ctx->code);

    ctx->parse_complete = FALSE;
    ctx->hres = S_OK;

    ctx->last_token = tNL;
    ctx->last_nl = 0;

    parser_parse(ctx);

    if(FAILED(ctx->hres))
        return ctx->hres;
    if(!ctx->parse_complete) {
        FIXME("parser failed on parsing %s\n", debugstr_w(ctx->ptr));
        return E_FAIL;
    }

    return S_OK;
}
