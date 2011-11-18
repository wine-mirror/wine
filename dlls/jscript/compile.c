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

static HRESULT compile_interp_fallback(compiler_ctx_t *ctx, expression_t *expr)
{
    unsigned instr;

    instr = push_instr(ctx, OP_tree);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.expr = expr;
    return S_OK;
}

static HRESULT compile_expression(compiler_ctx_t *ctx, expression_t *expr)
{
    switch(expr->type) {
    case EXPR_EQEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eq2);
    default:
        return compile_interp_fallback(ctx, expr);
    }

    return S_OK;
}

void release_bytecode(bytecode_t *code)
{
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
