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

typedef enum {
    EXPR_ADD,
    EXPR_BOOL,
    EXPR_CONCAT,
    EXPR_DOUBLE,
    EXPR_EMPTY,
    EXPR_EQUAL,
    EXPR_MEMBER,
    EXPR_NEG,
    EXPR_NEQUAL,
    EXPR_NOT,
    EXPR_NULL,
    EXPR_STRING,
    EXPR_SUB,
    EXPR_ULONG,
    EXPR_USHORT
} expression_type_t;

typedef struct _expression_t {
    expression_type_t type;
    struct _expression_t *next;
} expression_t;

typedef struct {
    expression_t expr;
    VARIANT_BOOL value;
} bool_expression_t;

typedef struct {
    expression_t expr;
    LONG value;
} int_expression_t;

typedef struct {
    expression_t expr;
    double value;
} double_expression_t;

typedef struct {
    expression_t expr;
    const WCHAR *value;
} string_expression_t;

typedef struct {
    expression_t expr;
    expression_t *subexpr;
} unary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *left;
    expression_t *right;
} binary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *obj_expr;
    const WCHAR *identifier;
    expression_t *args;
} member_expression_t;

typedef enum {
    STAT_ASSIGN,
    STAT_CALL,
    STAT_DIM
} statement_type_t;

typedef struct _statement_t {
    statement_type_t type;
    struct _statement_t *next;
} statement_t;

typedef struct {
    statement_t stat;
    member_expression_t *expr;
} call_statement_t;

typedef struct {
    statement_t stat;
    member_expression_t *member_expr;
    expression_t *value_expr;
} assign_statement_t;

typedef struct _dim_decl_t {
    const WCHAR *name;
    struct _dim_decl_t *next;
} dim_decl_t;

typedef struct _dim_statement_t {
    statement_t stat;
    dim_decl_t *dim_decls;
} dim_statement_t;

typedef struct {
    const WCHAR *code;
    const WCHAR *ptr;
    const WCHAR *end;

    BOOL option_explicit;
    BOOL parse_complete;
    HRESULT hres;

    int last_token;
    unsigned last_nl;

    statement_t *stats;
    statement_t *stats_tail;

    vbsheap_t heap;
} parser_ctx_t;

HRESULT parse_script(parser_ctx_t*,const WCHAR*) DECLSPEC_HIDDEN;
void parser_release(parser_ctx_t*) DECLSPEC_HIDDEN;
int parser_lex(void*,parser_ctx_t*) DECLSPEC_HIDDEN;
void *parser_alloc(parser_ctx_t*,size_t) DECLSPEC_HIDDEN;
