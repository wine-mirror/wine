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

#include <math.h>
#include <assert.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

struct _compiler_ctx_t {
    parser_ctx_t *parser;
    bytecode_t *code;

    unsigned code_off;
    unsigned code_size;
};

static HRESULT compile_expression(compiler_ctx_t*,expression_t*);

static inline void *compiler_alloc(bytecode_t *code, size_t size)
{
    return jsheap_alloc(&code->heap, size);
}

static WCHAR *compiler_alloc_string(bytecode_t *code, const WCHAR *str)
{
    size_t size;
    WCHAR *ret;

    size = (strlenW(str)+1)*sizeof(WCHAR);
    ret = compiler_alloc(code, size);
    if(ret)
        memcpy(ret, str, size);
    return ret;
}

static BSTR compiler_alloc_bstr(compiler_ctx_t *ctx, const WCHAR *str)
{
    if(!ctx->code->bstr_pool_size) {
        ctx->code->bstr_pool = heap_alloc(8 * sizeof(BSTR));
        if(!ctx->code->bstr_pool)
            return NULL;
        ctx->code->bstr_pool_size = 8;
    }else if(ctx->code->bstr_pool_size == ctx->code->bstr_cnt) {
        BSTR *new_pool;

        new_pool = heap_realloc(ctx->code->bstr_pool, ctx->code->bstr_pool_size*2*sizeof(BSTR));
        if(!new_pool)
            return NULL;

        ctx->code->bstr_pool = new_pool;
        ctx->code->bstr_pool_size *= 2;
    }

    ctx->code->bstr_pool[ctx->code->bstr_cnt] = SysAllocString(str);
    if(!ctx->code->bstr_pool[ctx->code->bstr_cnt])
        return NULL;

    return ctx->code->bstr_pool[ctx->code->bstr_cnt++];
}

static unsigned push_instr(compiler_ctx_t *ctx, jsop_t op)
{
    assert(ctx->code_size >= ctx->code_off);

    if(!ctx->code_size) {
        ctx->code->instrs = heap_alloc(64 * sizeof(instr_t));
        if(!ctx->code->instrs)
            return -1;
        ctx->code_size = 64;
    }else if(ctx->code_size == ctx->code_off) {
        instr_t *new_instrs;

        new_instrs = heap_realloc(ctx->code->instrs, ctx->code_size*2*sizeof(instr_t));
        if(!new_instrs)
            return -1;

        ctx->code->instrs = new_instrs;
        ctx->code_size *= 2;
    }

    ctx->code->instrs[ctx->code_off].op = op;
    return ctx->code_off++;
}

static inline instr_t *instr_ptr(compiler_ctx_t *ctx, unsigned off)
{
    assert(off < ctx->code_off);
    return ctx->code->instrs + off;
}

static HRESULT push_instr_int(compiler_ctx_t *ctx, jsop_t op, LONG arg)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.lng = arg;
    return S_OK;
}

static HRESULT push_instr_str(compiler_ctx_t *ctx, jsop_t op, const WCHAR *arg)
{
    unsigned instr;
    WCHAR *str;

    str = compiler_alloc_string(ctx->code, arg);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.str = str;
    return S_OK;
}

static HRESULT push_instr_bstr(compiler_ctx_t *ctx, jsop_t op, const WCHAR *arg)
{
    unsigned instr;
    WCHAR *str;

    str = compiler_alloc_bstr(ctx, arg);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.bstr = str;
    return S_OK;
}

static HRESULT push_instr_double(compiler_ctx_t *ctx, jsop_t op, double arg)
{
    unsigned instr;
    DOUBLE *dbl;

    dbl = compiler_alloc(ctx->code, sizeof(arg));
    if(!dbl)
        return E_OUTOFMEMORY;
    *dbl = arg;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.dbl = dbl;
    return S_OK;
}

static HRESULT compile_binary_expression(compiler_ctx_t *ctx, binary_expression_t *expr, jsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1);
    if(FAILED(hres))
        return hres;

    hres = compile_expression(ctx, expr->expression2);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) == -1 ? E_OUTOFMEMORY : S_OK;
}

static HRESULT compile_unary_expression(compiler_ctx_t *ctx, unary_expression_t *expr, jsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) == -1 ? E_OUTOFMEMORY : S_OK;
}

/* ECMA-262 3rd Edition    11.14 */
static HRESULT compile_comma_expression(compiler_ctx_t *ctx, binary_expression_t *expr)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1);
    if(FAILED(hres))
        return hres;

    if(push_instr(ctx, OP_pop) == -1)
        return E_OUTOFMEMORY;

    return compile_expression(ctx, expr->expression2);
}

/* ECMA-262 3rd Edition    11.11 */
static HRESULT compile_logical_expression(compiler_ctx_t *ctx, binary_expression_t *expr, jsop_t op)
{
    unsigned instr;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1);
    if(FAILED(hres))
        return hres;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, expr->expression2);
    if(FAILED(hres))
        return hres;

    instr_ptr(ctx, instr)->arg1.uint = ctx->code_off;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.12 */
static HRESULT compile_conditional_expression(compiler_ctx_t *ctx, conditional_expression_t *expr)
{
    unsigned jmp_false, jmp_end;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression);
    if(FAILED(hres))
        return hres;

    jmp_false = push_instr(ctx, OP_jmp_z);
    if(jmp_false == -1)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, expr->true_expression);
    if(FAILED(hres))
        return hres;

    jmp_end = push_instr(ctx, OP_jmp);
    if(jmp_end == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, jmp_false)->arg1.uint = ctx->code_off;
    if(push_instr(ctx, OP_pop) == -1)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, expr->false_expression);
    if(FAILED(hres))
        return hres;

    instr_ptr(ctx, jmp_end)->arg1.uint = ctx->code_off;
    return S_OK;
}

static HRESULT compile_new_expression(compiler_ctx_t *ctx, call_expression_t *expr)
{
    unsigned arg_cnt = 0;
    argument_t *arg;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression);
    if(FAILED(hres))
        return hres;

    for(arg = expr->argument_list; arg; arg = arg->next) {
        hres = compile_expression(ctx, arg->expr);
        if(FAILED(hres))
            return hres;
        arg_cnt++;
    }

    return push_instr_int(ctx, OP_new, arg_cnt);
}

static HRESULT compile_interp_fallback(compiler_ctx_t *ctx, expression_t *expr)
{
    unsigned instr;

    instr = push_instr(ctx, OP_tree);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.expr = expr;
    return S_OK;
}

static HRESULT compile_delete_expression(compiler_ctx_t *ctx, unary_expression_t *expr)
{
    HRESULT hres;

    switch(expr->expression->type) {
    case EXPR_ARRAY: {
        array_expression_t *array_expr = (array_expression_t*)expr->expression;

        hres = compile_expression(ctx, array_expr->member_expr);
        if(FAILED(hres))
            return hres;

        hres = compile_expression(ctx, array_expr->expression);
        if(FAILED(hres))
            return hres;

        if(push_instr(ctx, OP_delete) == -1)
            return E_OUTOFMEMORY;
        break;
    }
    case EXPR_MEMBER: {
        member_expression_t *member_expr = (member_expression_t*)expr->expression;

        hres = compile_expression(ctx, member_expr->expression);
        if(FAILED(hres))
            return hres;

        /* FIXME: Potential optimization */
        hres = push_instr_str(ctx, OP_str, member_expr->identifier);
        if(FAILED(hres))
            return hres;

        if(push_instr(ctx, OP_delete) == -1)
            return E_OUTOFMEMORY;
        break;
    }
    default:
        expr->expr.eval = delete_expression_eval;
        return compile_interp_fallback(ctx, &expr->expr);
    }

    return S_OK;
}

static HRESULT compile_literal(compiler_ctx_t *ctx, literal_t *literal)
{
    switch(literal->type) {
    case LT_BOOL:
        return push_instr_int(ctx, OP_bool, literal->u.bval);
    case LT_DOUBLE:
        return push_instr_double(ctx, OP_double, literal->u.dval);
    case LT_INT:
        return push_instr_int(ctx, OP_int, literal->u.lval);
    case LT_NULL:
        return push_instr(ctx, OP_null);
    case LT_STRING:
        return push_instr_str(ctx, OP_str, literal->u.wstr);
    case LT_REGEXP: {
        unsigned instr;
        WCHAR *str;

        str = compiler_alloc(ctx->code, (literal->u.regexp.str_len+1)*sizeof(WCHAR));
        if(!str)
            return E_OUTOFMEMORY;
        memcpy(str, literal->u.regexp.str, literal->u.regexp.str_len*sizeof(WCHAR));
        str[literal->u.regexp.str_len] = 0;

        instr = push_instr(ctx, OP_regexp);
        if(instr == -1)
            return E_OUTOFMEMORY;

        instr_ptr(ctx, instr)->arg1.str = str;
        instr_ptr(ctx, instr)->arg2.lng = literal->u.regexp.flags;
        return S_OK;
    }
    default:
        assert(0);
    }
}

static HRESULT compile_expression(compiler_ctx_t *ctx, expression_t *expr)
{
    switch(expr->type) {
    case EXPR_AND:
        return compile_logical_expression(ctx, (binary_expression_t*)expr, OP_jmp_z);
    case EXPR_ADD:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_add);
    case EXPR_BITNEG:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_bneg);
    case EXPR_BOR:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_or);
    case EXPR_COMMA:
        return compile_comma_expression(ctx, (binary_expression_t*)expr);
    case EXPR_COND:
        return compile_conditional_expression(ctx, (conditional_expression_t*)expr);
    case EXPR_DELETE:
        return compile_delete_expression(ctx, (unary_expression_t*)expr);
    case EXPR_DIV:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_div);
    case EXPR_EQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eq);
    case EXPR_EQEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eq2);
    case EXPR_IDENT:
        return push_instr_bstr(ctx, OP_ident, ((identifier_expression_t*)expr)->identifier);
    case EXPR_IN:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_in);
    case EXPR_LESS:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lt);
    case EXPR_LESSEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lteq);
    case EXPR_LITERAL:
        return compile_literal(ctx, ((literal_expression_t*)expr)->literal);
    case EXPR_LOGNEG:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_neg);
    case EXPR_MINUS:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_minus);
    case EXPR_MOD:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mod);
    case EXPR_MUL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mul);
    case EXPR_NEW:
        return compile_new_expression(ctx, (call_expression_t*)expr);
    case EXPR_NOTEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_neq);
    case EXPR_NOTEQEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_neq2);
    case EXPR_OR:
        return compile_logical_expression(ctx, (binary_expression_t*)expr, OP_jmp_nz);
    case EXPR_PLUS:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_tonum);
    case EXPR_SUB:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_sub);
    case EXPR_THIS:
        return push_instr(ctx, OP_this) == -1 ? E_OUTOFMEMORY : S_OK;
    case EXPR_VOID:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_void);
    case EXPR_BXOR:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_xor);
    default:
        assert(expr->eval != compiled_expression_eval);
        return compile_interp_fallback(ctx, expr);
    }

    return S_OK;
}

void release_bytecode(bytecode_t *code)
{
    unsigned i;

    for(i=0; i < code->bstr_cnt; i++)
        SysFreeString(code->bstr_pool[i]);

    jsheap_free(&code->heap);
    heap_free(code->bstr_pool);
    heap_free(code->instrs);
    heap_free(code);
}

void release_compiler(compiler_ctx_t *ctx)
{
    heap_free(ctx);
}

HRESULT compile_subscript(parser_ctx_t *parser, expression_t *expr, unsigned *ret_off)
{
    HRESULT hres;

    if(!parser->code) {
        parser->code = heap_alloc_zero(sizeof(bytecode_t));
        if(!parser->code)
            return E_OUTOFMEMORY;
        jsheap_init(&parser->code->heap);
    }

    if(!parser->compiler) {
        parser->compiler = heap_alloc_zero(sizeof(compiler_ctx_t));
        if(!parser->compiler)
            return E_OUTOFMEMORY;

        parser->compiler->parser = parser;
        parser->compiler->code = parser->code;
    }

    *ret_off = parser->compiler->code_off;
    hres = compile_expression(parser->compiler, expr);
    if(FAILED(hres))
        return hres;

    return push_instr(parser->compiler, OP_ret) == -1 ? E_OUTOFMEMORY : S_OK;
}
