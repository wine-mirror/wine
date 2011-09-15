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

 static void parse_complete(parser_ctx_t*,BOOL);

static void source_add_statement(parser_ctx_t*,statement_t*);
static void source_add_class(parser_ctx_t*,class_decl_t*);

static void *new_expression(parser_ctx_t*,expression_type_t,size_t);
static expression_t *new_bool_expression(parser_ctx_t*,VARIANT_BOOL);
static expression_t *new_string_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_long_expression(parser_ctx_t*,expression_type_t,LONG);
static expression_t *new_double_expression(parser_ctx_t*,double);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);

static member_expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);

static void *new_statement(parser_ctx_t*,statement_type_t,size_t);
static statement_t *new_call_statement(parser_ctx_t*,member_expression_t*);
static statement_t *new_assign_statement(parser_ctx_t*,member_expression_t*,expression_t*);
static statement_t *new_dim_statement(parser_ctx_t*,dim_decl_t*);
static statement_t *new_if_statement(parser_ctx_t*,expression_t*,statement_t*,elseif_decl_t*,statement_t*);
static statement_t *new_function_statement(parser_ctx_t*,function_decl_t*);

static dim_decl_t *new_dim_decl(parser_ctx_t*,const WCHAR*,dim_decl_t*);
static elseif_decl_t *new_elseif_decl(parser_ctx_t*,expression_t*,statement_t*);
static function_decl_t *new_function_decl(parser_ctx_t*,const WCHAR*,function_type_t,arg_decl_t*,statement_t*);
static arg_decl_t *new_argument_decl(parser_ctx_t*,const WCHAR*,BOOL);
static class_decl_t *new_class_decl(parser_ctx_t*);

#define CHECK_ERROR if(((parser_ctx_t*)ctx)->hres != S_OK) YYABORT

%}

%pure_parser
%start Program

%union {
    const WCHAR *string;
    statement_t *statement;
    expression_t *expression;
    member_expression_t *member;
    elseif_decl_t *elseif;
    dim_decl_t *dim_decl;
    function_decl_t *func_decl;
    arg_decl_t *arg_decl;
    class_decl_t *class_decl;
    LONG lng;
    BOOL bool;
    double dbl;
}

%token tEOF tNL tREM tEMPTYBRACKETS
%token tTRUE tFALSE
%token tNOT tAND tOR tXOR tEQV tIMP tNEQ
%token tIS tLTEQ tGTEQ tMOD
%token tCALL tDIM tSUB tFUNCTION tPROPERTY tGET tLET
%token tIF tELSE tELSEIF tEND tTHEN tEXIT
%token tWHILE tWEND tDO tLOOP tUNTIL
%token tBYREF tBYVAL
%token tOPTION tEXPLICIT
%token tSTOP
%token tNOTHING tEMPTY tNULL
%token tCLASS tSET tNEW tPUBLIC tPRIVATE tDEFAULT tME
%token tERROR tNEXT tON tRESUME tGOTO
%token <string> tIdentifier tString
%token <lng> tLong tShort
%token <dbl> tDouble

%type <statement> Statement StatementNl StatementsNl StatementsNl_opt IfStatement Else_opt
%type <expression> Expression LiteralExpression PrimaryExpression EqualityExpression CallExpression
%type <expression> ConcatExpression AdditiveExpression ModExpression IntdivExpression MultiplicativeExpression ExpExpression
%type <expression> NotExpression UnaryExpression AndExpression OrExpression XorExpression EqvExpression
%type <member> MemberExpression
%type <expression> Arguments_opt ArgumentList_opt ArgumentList
%type <bool> OptionExplicit_opt
%type <arg_decl> ArgumentsDecl_opt ArgumentDeclList ArgumentDecl
%type <func_decl> FunctionDecl
%type <elseif> ElseIfs_opt ElseIfs ElseIf
%type <class_decl> ClassDeclaration ClassBody
%type <dim_decl> DimDeclList

%%

Program
    : OptionExplicit_opt SourceElements tEOF    { parse_complete(ctx, $1); }

OptionExplicit_opt
    : /* empty */                { $$ = FALSE; }
    | tOPTION tEXPLICIT tNL      { $$ = TRUE; }

SourceElements
    : /* empty */
    | SourceElements StatementNl            { source_add_statement(ctx, $2); }
    | SourceElements ClassDeclaration       { source_add_class(ctx, $2); }

StatementsNl_opt
    : /* empty */                           { $$ = NULL; }
    | StatementsNl                          { $$ = $1; }

StatementsNl
    : StatementNl                           { $$ = $1; }
    | StatementNl StatementsNl              { $1->next = $2; $$ = $1; }

StatementNl
    : Statement tNL                 { $$ = $1; }

Statement
    : MemberExpression ArgumentList_opt     { $1->args = $2; $$ = new_call_statement(ctx, $1); CHECK_ERROR; }
    | tCALL MemberExpression Arguments_opt  { $2->args = $3; $$ = new_call_statement(ctx, $2); CHECK_ERROR; }
    | MemberExpression Arguments_opt '=' Expression
                                            { $1->args = $2; $$ = new_assign_statement(ctx, $1, $4); CHECK_ERROR; }
    | tDIM DimDeclList                      { $$ = new_dim_statement(ctx, $2); CHECK_ERROR; }
    | IfStatement                           { $$ = $1; }
    | FunctionDecl                          { $$ = new_function_statement(ctx, $1); CHECK_ERROR; }
    | tEXIT tFUNCTION                       { $$ = new_statement(ctx, STAT_EXITFUNC, 0); CHECK_ERROR; }
    | tEXIT tSUB                            { $$ = new_statement(ctx, STAT_EXITSUB, 0); CHECK_ERROR; }

MemberExpression
    : tIdentifier                           { $$ = new_member_expression(ctx, NULL, $1); CHECK_ERROR; }
    | CallExpression '.' tIdentifier        { $$ = new_member_expression(ctx, $1, $3); CHECK_ERROR; }

DimDeclList /* FIXME: Support arrays */
    : tIdentifier                           { $$ = new_dim_decl(ctx, $1, NULL); CHECK_ERROR; }
    | tIdentifier ',' DimDeclList           { $$ = new_dim_decl(ctx, $1, $3); CHECK_ERROR; }

IfStatement
    : tIF Expression tTHEN tNL StatementsNl ElseIfs_opt Else_opt tEND tIF
                                            { $$ = new_if_statement(ctx, $2, $5, $6, $7); CHECK_ERROR; }
    /* FIXME: short if statement */

ElseIfs_opt
    : /* empty */                           { $$ = NULL; }
    | ElseIfs                               { $$ = $1; }

ElseIfs
    : ElseIf                                { $$ = $1; }
    | ElseIf ElseIfs                        { $1->next = $2; $$ = $1; }

ElseIf
    : tELSEIF Expression tTHEN tNL StatementsNl
                                            { $$ = new_elseif_decl(ctx, $2, $5); }

Else_opt
    : /* empty */                           { $$ = NULL; }
    | tELSE tNL StatementsNl                { $$ = $3; }

Arguments_opt
    : EmptyBrackets_opt             { $$ = NULL; }
    | '(' ArgumentList ')'          { $$ = $2; }

ArgumentList_opt
    : EmptyBrackets_opt             { $$ = NULL; }
    | ArgumentList                  { $$ = $1; }

ArgumentList
    : Expression                    { $$ = $1; }
    | Expression ',' ArgumentList   { $1->next = $3; $$ = $1; }

EmptyBrackets_opt
    : /* empty */
    | tEMPTYBRACKETS

Expression
    : EqvExpression                             { $$ = $1; }
    | Expression tIMP EqvExpression             { $$ = new_binary_expression(ctx, EXPR_IMP, $1, $3); CHECK_ERROR; }

EqvExpression
    : XorExpression                             { $$ = $1; }
    | EqvExpression tEQV XorExpression          { $$ = new_binary_expression(ctx, EXPR_EQV, $1, $3); CHECK_ERROR; }

XorExpression
    : OrExpression                              { $$ = $1; }
    | XorExpression tXOR OrExpression           { $$ = new_binary_expression(ctx, EXPR_XOR, $1, $3); CHECK_ERROR; }

OrExpression
    : AndExpression                             { $$ = $1; }
    | OrExpression tOR AndExpression            { $$ = new_binary_expression(ctx, EXPR_OR, $1, $3); CHECK_ERROR; }

AndExpression
    : NotExpression                             { $$ = $1; }
    | AndExpression tAND NotExpression          { $$ = new_binary_expression(ctx, EXPR_AND, $1, $3); CHECK_ERROR; }

NotExpression
    : EqualityExpression            { $$ = $1; }
    | tNOT NotExpression            { $$ = new_unary_expression(ctx, EXPR_NOT, $2); CHECK_ERROR; }

EqualityExpression
    : ConcatExpression                          { $$ = $1; }
    | EqualityExpression '=' ConcatExpression   { $$ = new_binary_expression(ctx, EXPR_EQUAL, $1, $3); CHECK_ERROR; }
    | EqualityExpression tNEQ ConcatExpression  { $$ = new_binary_expression(ctx, EXPR_NEQUAL, $1, $3); CHECK_ERROR; }

ConcatExpression
    : AdditiveExpression                        { $$ = $1; }
    | ConcatExpression '&' AdditiveExpression   { $$ = new_binary_expression(ctx, EXPR_CONCAT, $1, $3); CHECK_ERROR; }

AdditiveExpression
    : ModExpression                             { $$ = $1; }
    | AdditiveExpression '+' ModExpression      { $$ = new_binary_expression(ctx, EXPR_ADD, $1, $3); CHECK_ERROR; }
    | AdditiveExpression '-' ModExpression      { $$ = new_binary_expression(ctx, EXPR_SUB, $1, $3); CHECK_ERROR; }

ModExpression
    : IntdivExpression                          { $$ = $1; }
    | ModExpression tMOD IntdivExpression       { $$ = new_binary_expression(ctx, EXPR_MOD, $1, $3); CHECK_ERROR; }

IntdivExpression
    : MultiplicativeExpression                  { $$ = $1; }
    | IntdivExpression '\\' MultiplicativeExpression
                                                { $$ = new_binary_expression(ctx, EXPR_IDIV, $1, $3); CHECK_ERROR; }

MultiplicativeExpression
    : ExpExpression                             { $$ = $1; }
    | MultiplicativeExpression '*' ExpExpression
                                                { $$ = new_binary_expression(ctx, EXPR_MUL, $1, $3); CHECK_ERROR; }
    | MultiplicativeExpression '/' ExpExpression
                                                { $$ = new_binary_expression(ctx, EXPR_DIV, $1, $3); CHECK_ERROR; }

ExpExpression
    : UnaryExpression                           { $$ = $1; }
    | ExpExpression '^' UnaryExpression         { $$ = new_binary_expression(ctx, EXPR_EXP, $1, $3); CHECK_ERROR; }

UnaryExpression
    : LiteralExpression             { $$ = $1; }
    | CallExpression                { $$ = $1; }
    | '-' UnaryExpression           { $$ = new_unary_expression(ctx, EXPR_NEG, $2); CHECK_ERROR; }

CallExpression
    : PrimaryExpression                 { $$ = $1; }
    | MemberExpression Arguments_opt    { $1->args = $2; $$ = &$1->expr; }

LiteralExpression
    : tTRUE                         { $$ = new_bool_expression(ctx, VARIANT_TRUE); CHECK_ERROR; }
    | tFALSE                        { $$ = new_bool_expression(ctx, VARIANT_FALSE); CHECK_ERROR; }
    | tString                       { $$ = new_string_expression(ctx, $1); CHECK_ERROR; }
    | tShort                        { $$ = new_long_expression(ctx, EXPR_USHORT, $1); CHECK_ERROR; }
    | '0'                           { $$ = new_long_expression(ctx, EXPR_USHORT, 0); CHECK_ERROR; }
    | tLong                         { $$ = new_long_expression(ctx, EXPR_ULONG, $1); CHECK_ERROR; }
    | tDouble                       { $$ = new_double_expression(ctx, $1); CHECK_ERROR; }
    | tEMPTY                        { $$ = new_expression(ctx, EXPR_EMPTY, 0); CHECK_ERROR; }
    | tNULL                         { $$ = new_expression(ctx, EXPR_NULL, 0); CHECK_ERROR; }

PrimaryExpression
    : '(' Expression ')'            { $$ = $2; }

ClassDeclaration
    : tCLASS tIdentifier tNL ClassBody tEND tCLASS tNL      { $4->name = $2; $$ = $4; }

ClassBody
    : /* empty */                               { $$ = new_class_decl(ctx); }

FunctionDecl
    : /* Storage_opt */ tSUB tIdentifier ArgumentsDecl_opt tNL StatementsNl_opt tEND tSUB
                                    { $$ = new_function_decl(ctx, $2, FUNC_SUB, $3, $5); CHECK_ERROR; }
    | /* Storage_opt */ tFUNCTION tIdentifier ArgumentsDecl_opt tNL StatementsNl_opt tEND tFUNCTION
                                    { $$ = new_function_decl(ctx, $2, FUNC_FUNCTION, $3, $5); CHECK_ERROR; }

ArgumentsDecl_opt
    : EmptyBrackets_opt                         { $$ = NULL; }
    | '(' ArgumentDeclList ')'                  { $$ = $2; }

ArgumentDeclList
    : ArgumentDecl                              { $$ = $1; }
    | ArgumentDecl ',' ArgumentDeclList         { $1->next = $3; $$ = $1; }

ArgumentDecl
    : tIdentifier                               { $$ = new_argument_decl(ctx, $1, TRUE); }
    | tBYREF tIdentifier                        { $$ = new_argument_decl(ctx, $2, TRUE); }
    | tBYVAL tIdentifier                        { $$ = new_argument_decl(ctx, $2, FALSE); }

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

static void source_add_class(parser_ctx_t *ctx, class_decl_t *class_decl)
{
    class_decl->next = ctx->class_decls;
    ctx->class_decls = class_decl;
}

static void parse_complete(parser_ctx_t *ctx, BOOL option_explicit)
{
    ctx->parse_complete = TRUE;
    ctx->option_explicit = option_explicit;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *expr;

    expr = parser_alloc(ctx, size ? size : sizeof(*expr));
    if(expr) {
        expr->type = type;
        expr->next = NULL;
    }

    return expr;
}

static expression_t *new_bool_expression(parser_ctx_t *ctx, VARIANT_BOOL value)
{
    bool_expression_t *expr;

    expr = new_expression(ctx, EXPR_BOOL, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_string_expression(parser_ctx_t *ctx, const WCHAR *value)
{
    string_expression_t *expr;

    expr = new_expression(ctx, EXPR_STRING, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_long_expression(parser_ctx_t *ctx, expression_type_t type, LONG value)
{
    int_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_double_expression(parser_ctx_t *ctx, double value)
{
    double_expression_t *expr;

    expr = new_expression(ctx, EXPR_DOUBLE, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *subexpr)
{
    unary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->subexpr = subexpr;
    return &expr->expr;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *left, expression_t *right)
{
    binary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->left = left;
    expr->right = right;
    return &expr->expr;
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

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
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

static statement_t *new_assign_statement(parser_ctx_t *ctx, member_expression_t *left, expression_t *right)
{
    assign_statement_t *stat;

    stat = new_statement(ctx, STAT_ASSIGN, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->member_expr = left;
    stat->value_expr = right;
    return &stat->stat;
}

static dim_decl_t *new_dim_decl(parser_ctx_t *ctx, const WCHAR *name, dim_decl_t *next)
{
    dim_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->next = next;
    return decl;
}

static statement_t *new_dim_statement(parser_ctx_t *ctx, dim_decl_t *decls)
{
    dim_statement_t *stat;

    stat = new_statement(ctx, STAT_DIM, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->dim_decls = decls;
    return &stat->stat;
}

static elseif_decl_t *new_elseif_decl(parser_ctx_t *ctx, expression_t *expr, statement_t *stat)
{
    elseif_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->expr = expr;
    decl->stat = stat;
    decl->next = NULL;
    return decl;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, elseif_decl_t *elseif_decl,
        statement_t *else_stat)
{
    if_statement_t *stat;

    stat = new_statement(ctx, STAT_IF, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->if_stat = if_stat;
    stat->elseifs = elseif_decl;
    stat->else_stat = else_stat;
    return &stat->stat;
}

static arg_decl_t *new_argument_decl(parser_ctx_t *ctx, const WCHAR *name, BOOL by_ref)
{
    arg_decl_t *arg_decl;

    arg_decl = parser_alloc(ctx, sizeof(*arg_decl));
    if(!arg_decl)
        return NULL;

    arg_decl->name = name;
    arg_decl->by_ref = by_ref;
    arg_decl->next = NULL;
    return arg_decl;
}

static function_decl_t *new_function_decl(parser_ctx_t *ctx, const WCHAR *name, function_type_t type,
        arg_decl_t *arg_decl, statement_t *body)
{
    function_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->type = type;
    decl->args = arg_decl;
    decl->body = body;
    return decl;
}

static statement_t *new_function_statement(parser_ctx_t *ctx, function_decl_t *decl)
{
    function_statement_t *stat;

    stat = new_statement(ctx, STAT_FUNC, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->func_decl = decl;
    return &stat->stat;
}

static class_decl_t *new_class_decl(parser_ctx_t *ctx)
{
    class_decl_t *class_decl;

    class_decl = parser_alloc(ctx, sizeof(*class_decl));
    if(!class_decl)
        return NULL;

    class_decl->next = NULL;
    return class_decl;
}

void *parser_alloc(parser_ctx_t *ctx, size_t size)
{
    void *ret;

    ret = vbsheap_alloc(&ctx->heap, size);
    if(!ret)
        ctx->hres = E_OUTOFMEMORY;
    return ret;
}

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code)
{
    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + strlenW(ctx->code);

    vbsheap_init(&ctx->heap);

    ctx->parse_complete = FALSE;
    ctx->hres = S_OK;

    ctx->last_token = tNL;
    ctx->last_nl = 0;
    ctx->stats = ctx->stats_tail = NULL;
    ctx->class_decls = NULL;
    ctx->option_explicit = FALSE;

    parser_parse(ctx);

    if(FAILED(ctx->hres))
        return ctx->hres;
    if(!ctx->parse_complete) {
        FIXME("parser failed on parsing %s\n", debugstr_w(ctx->ptr));
        return E_FAIL;
    }

    return S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    vbsheap_free(&ctx->heap);
}
