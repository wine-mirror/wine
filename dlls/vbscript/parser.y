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

static void source_add_statement(parser_ctx_t*,statement_t*);

static member_expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);

static statement_t *new_call_statement(parser_ctx_t*,member_expression_t*);

#define CHECK_ERROR if(((parser_ctx_t*)ctx)->hres != S_OK) YYABORT

%}

%pure_parser
%start Program

%union {
    const WCHAR *string;
    statement_t *statement;
    expression_t *expression;
    member_expression_t *member;
}

%token tEOF tNL blah
%token tCALL
%token <string> tIdentifier

%type <statement> Statement StatementNl
%type <member> MemberExpression
%type <expression> Arguments_opt ArgumentList_opt

%%

Program
    : SourceElements tEOF           { parse_complete(ctx); }

SourceElements
    : /* empty */
    | SourceElements StatementNl    { source_add_statement(ctx, $2); }

StatementNl
    : Statement tNL                 { $$ = $1; }

Statement
    : MemberExpression Arguments_opt        { $1->args = $2; $$ = new_call_statement(ctx, $1); CHECK_ERROR; }
    | tCALL MemberExpression Arguments_opt  { $2->args = $3; $$ = new_call_statement(ctx, $2); CHECK_ERROR; }

MemberExpression
    : tIdentifier                   { $$ = new_member_expression(ctx, NULL, $1); CHECK_ERROR; }
    /* FIXME: MemberExpressionArgs '.' tIdentifier */

Arguments_opt
    : /* empty */                   { $$ = NULL; }
    | '(' ArgumentList_opt ')'      { $$ = $2; }

ArgumentList_opt
    : /* empty */                   { $$ = NULL; }
 /* | ArgumentList                  { $$ = $1; } */

%%

static int parser_error(const char *str)
{
    return 0;
}

static void source_add_statement(parser_ctx_t *ctx, statement_t *stat)
{
    if(ctx->stats) {
        ctx->stats_tail->next = stat;
        ctx->stats_tail = stat;
    }else {
        ctx->stats = ctx->stats_tail = stat;
    }
}

static void parse_complete(parser_ctx_t *ctx)
{
    ctx->parse_complete = TRUE;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, unsigned size)
{
    expression_t *expr;

    expr = parser_alloc(ctx, size ? size : sizeof(*expr));
    if(expr) {
        expr->type = type;
        expr->next = NULL;
    }

    return expr;
}

static member_expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *obj_expr, const WCHAR *identifier)
{
    member_expression_t *expr;

    expr = new_expression(ctx, EXPR_MEMBER, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->obj_expr = obj_expr;
    expr->identifier = identifier;
    expr->args = NULL;
    return expr;
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, unsigned size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size);
    if(stat) {
        stat->type = type;
        stat->next = NULL;
    }

    return stat;
}

static statement_t *new_call_statement(parser_ctx_t *ctx, member_expression_t *expr)
{
    call_statement_t *stat;

    stat = new_statement(ctx, STAT_CALL, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    return &stat->stat;
}

void *parser_alloc(parser_ctx_t *ctx, size_t size)
{
    void *ret;

    /* FIXME: leaks! */
    ret = heap_alloc(size);
    if(!ret)
        ctx->hres = E_OUTOFMEMORY;
    return ret;
}

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code)
{
    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + strlenW(ctx->code);

    ctx->parse_complete = FALSE;
    ctx->hres = S_OK;

    ctx->last_token = tNL;
    ctx->last_nl = 0;
    ctx->stats = ctx->stats_tail = NULL;

    parser_parse(ctx);

    if(FAILED(ctx->hres))
        return ctx->hres;
    if(!ctx->parse_complete) {
        FIXME("parser failed on parsing %s\n", debugstr_w(ctx->ptr));
        return E_FAIL;
    }

    return S_OK;
}
