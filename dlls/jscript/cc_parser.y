/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#include "jscript.h"
#include "engine.h"
#include "parser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

%}

%lex-param { parser_ctx_t *ctx }
%parse-param { parser_ctx_t *ctx }
%pure-parser
%start CCExpr

%union {
    ccval_t ccval;
}

%token tNEQ
%token <ccval> tCCValue

%type <ccval> CCUnaryExpression CCEqualityExpression CCAdditiveExpression CCMultiplicativeExpression

%{

static int cc_parser_error(parser_ctx_t *ctx, const char *str)
{
    if(SUCCEEDED(ctx->hres)) {
        WARN("%s\n", str);
        ctx->hres = JS_E_SYNTAX;
    }

    return 0;
}

static int cc_parser_lex(void *lval, parser_ctx_t *ctx)
{
    int r;

    r = try_parse_ccval(ctx, lval);
    if(r)
        return r > 0 ? tCCValue : -1;

    switch(*ctx->ptr) {
    case '(':
    case ')':
    case '+':
    case '*':
    case '/':
        return *ctx->ptr++;
    case '!':
        if(*++ctx->ptr == '=') {
            ctx->ptr++;
            return tNEQ;
        }

        return '!';
    }

    WARN("Failed to interpret %s\n", debugstr_w(ctx->ptr));
    return -1;
}

%}

%%

/* FIXME: Implement missing expressions. */

CCExpr
    : CCUnaryExpression { ctx->ccval = $1; YYACCEPT; }

CCUnaryExpression
    : tCCValue                      { $$ = $1; }
    | '(' CCEqualityExpression ')'  { $$ = $2; }
    | '!' CCUnaryExpression         { $$ = ccval_bool(!get_ccbool($2)); };

CCEqualityExpression
    : CCAdditiveExpression          { $$ = $1; }
    | CCEqualityExpression tNEQ CCAdditiveExpression
                                    { $$ = ccval_bool(get_ccnum($1) != get_ccnum($3)); }

CCAdditiveExpression
    : CCMultiplicativeExpression    { $$ = $1; }
    | CCAdditiveExpression '+' CCMultiplicativeExpression
                                    { $$ = ccval_num(get_ccnum($1) + get_ccnum($3)); }

CCMultiplicativeExpression
    : CCUnaryExpression             { $$ = $1; }
    | CCMultiplicativeExpression '*' CCUnaryExpression
                                    { $$ = ccval_num(get_ccnum($1) * get_ccnum($3)); }
    | CCMultiplicativeExpression '/' CCUnaryExpression
                                    { $$ = ccval_num(get_ccnum($1) / get_ccnum($3)); }

%%

BOOL parse_cc_expr(parser_ctx_t *ctx)
{
    ctx->hres = S_OK;
    cc_parser_parse(ctx);
    return SUCCEEDED(ctx->hres);
}
