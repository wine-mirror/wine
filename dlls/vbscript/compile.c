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

#include <assert.h>

#include "vbscript.h"
#include "parse.h"
#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

typedef struct {
    parser_ctx_t parser;

    unsigned instr_cnt;
    unsigned instr_size;
    vbscode_t *code;
} compile_ctx_t;

static inline instr_t *instr_ptr(compile_ctx_t *ctx, unsigned id)
{
    assert(id < ctx->instr_cnt);
    return ctx->code->instrs + id;
}

static unsigned push_instr(compile_ctx_t *ctx, vbsop_t op)
{
    assert(ctx->instr_size && ctx->instr_size >= ctx->instr_cnt);

    if(ctx->instr_size == ctx->instr_cnt) {
        instr_t *new_instr;

        new_instr = heap_realloc(ctx->code->instrs, ctx->instr_size*2*sizeof(instr_t));
        if(!new_instr)
            return -1;

        ctx->code->instrs = new_instr;
        ctx->instr_size *= 2;
    }

    ctx->code->instrs[ctx->instr_cnt].op = op;
    return ctx->instr_cnt++;
}

static HRESULT push_instr_str(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.str = arg;
    return S_OK;
}

static HRESULT compile_member_expression(compile_ctx_t *ctx, member_expression_t *expr)
{
    HRESULT hres;

    if(expr->args) {
        FIXME("arguments not implemented\n");
        return E_NOTIMPL;
    }

    if(expr->obj_expr) {
        FIXME("obj_expr not implemented\n");
        hres = E_NOTIMPL;
    }else {
        hres = push_instr_str(ctx, OP_icallv, expr->identifier);
    }

    return hres;
}

static HRESULT compile_statement(compile_ctx_t *ctx, statement_t *stat)
{
    HRESULT hres;

    while(stat) {
        switch(stat->type) {
        case STAT_CALL:
            hres = compile_member_expression(ctx, ((call_statement_t*)stat)->expr);
            break;
        default:
            FIXME("Unimplemented statement type %d\n", stat->type);
            hres = E_NOTIMPL;
        }

        if(FAILED(hres))
            return hres;
        stat = stat->next;
    }

    return S_OK;
}

static HRESULT compile_func(compile_ctx_t *ctx, statement_t *stat, function_t *func)
{
    HRESULT hres;

    func->code_off = ctx->instr_cnt;

    hres = compile_statement(ctx, stat);
    if(FAILED(hres))
        return hres;

    if(push_instr(ctx, OP_ret) == -1)
        return E_OUTOFMEMORY;

    return S_OK;
}

void release_vbscode(vbscode_t *code)
{
    list_remove(&code->entry);
    heap_free(code->source);
    heap_free(code->instrs);
    heap_free(code);
}

static vbscode_t *alloc_vbscode(compile_ctx_t *ctx, const WCHAR *source)
{
    vbscode_t *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->source = heap_strdupW(source);
    if(!ret->source) {
        heap_free(ret);
        return NULL;
    }

    ret->instrs = heap_alloc(32*sizeof(instr_t));
    if(!ret->instrs) {
        release_vbscode(ret);
        return NULL;
    }

    ctx->instr_cnt = 0;
    ctx->instr_size = 32;

    ret->global_code.code_ctx = ret;

    list_init(&ret->entry);
    return ret;
}

HRESULT compile_script(script_ctx_t *script, const WCHAR *src, vbscode_t **ret)
{
    compile_ctx_t ctx;
    HRESULT hres;

    hres = parse_script(&ctx.parser, src);
    if(FAILED(hres))
        return hres;

    ctx.code = alloc_vbscode(&ctx, src);
    if(!ctx.code)
        return E_OUTOFMEMORY;

    hres = compile_func(&ctx, ctx.parser.stats, &ctx.code->global_code);
    if(FAILED(hres)) {
        release_vbscode(ctx.code);
        return hres;
    }

    list_add_tail(&script->code_list, &ctx.code->entry);
    *ret = ctx.code;
    return S_OK;
}
