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

    unsigned *labels;
    unsigned labels_size;
    unsigned labels_cnt;

    dim_decl_t *dim_decls;
    dynamic_var_t *global_vars;
} compile_ctx_t;

static HRESULT compile_expression(compile_ctx_t*,expression_t*);
static HRESULT compile_statement(compile_ctx_t*,statement_t*);

static const struct {
    instr_arg_type_t arg1_type;
    instr_arg_type_t arg2_type;
} instr_info[] = {
#define X(n,a,b,c) {b,c},
OP_LIST
#undef X
};

static inline void *compiler_alloc(vbscode_t *vbscode, size_t size)
{
    return vbsheap_alloc(&vbscode->heap, size);
}

static WCHAR *compiler_alloc_string(vbscode_t *vbscode, const WCHAR *str)
{
    size_t size;
    WCHAR *ret;

    size = (strlenW(str)+1)*sizeof(WCHAR);
    ret = compiler_alloc(vbscode, size);
    if(ret)
        memcpy(ret, str, size);
    return ret;
}

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

static HRESULT push_instr_int(compile_ctx_t *ctx, vbsop_t op, LONG arg)
{
    unsigned ret;

    ret = push_instr(ctx, op);
    if(ret == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, ret)->arg1.lng = arg;
    return S_OK;
}

static HRESULT push_instr_addr(compile_ctx_t *ctx, vbsop_t op, unsigned arg)
{
    unsigned ret;

    ret = push_instr(ctx, op);
    if(ret == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, ret)->arg1.uint = arg;
    return S_OK;
}

static HRESULT push_instr_str(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg)
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

static HRESULT push_instr_double(compile_ctx_t *ctx, vbsop_t op, double arg)
{
    unsigned instr;
    double *d;

    d = compiler_alloc(ctx->code, sizeof(double));
    if(!d)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    *d = arg;
    instr_ptr(ctx, instr)->arg1.dbl = d;
    return S_OK;
}

static BSTR alloc_bstr_arg(compile_ctx_t *ctx, const WCHAR *str)
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

static HRESULT push_instr_bstr(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg)
{
    unsigned instr;
    BSTR bstr;

    bstr = alloc_bstr_arg(ctx, arg);
    if(!bstr)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.bstr = bstr;
    return S_OK;
}

static HRESULT push_instr_bstr_uint(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg1, unsigned arg2)
{
    unsigned instr;
    BSTR bstr;

    bstr = alloc_bstr_arg(ctx, arg1);
    if(!bstr)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(instr == -1)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.bstr = bstr;
    instr_ptr(ctx, instr)->arg2.uint = arg2;
    return S_OK;
}

#define LABEL_FLAG 0x80000000

static unsigned alloc_label(compile_ctx_t *ctx)
{
    if(!ctx->labels_size) {
        ctx->labels = heap_alloc(8 * sizeof(*ctx->labels));
        if(!ctx->labels)
            return -1;
        ctx->labels_size = 8;
    }else if(ctx->labels_size == ctx->labels_cnt) {
        unsigned *new_labels;

        new_labels = heap_realloc(ctx->labels, 2*ctx->labels_size*sizeof(*ctx->labels));
        if(!new_labels)
            return -1;

        ctx->labels = new_labels;
        ctx->labels_size *= 2;
    }

    return ctx->labels_cnt++ | LABEL_FLAG;
}

static inline void label_set_addr(compile_ctx_t *ctx, unsigned label)
{
    assert(label & LABEL_FLAG);
    ctx->labels[label & ~LABEL_FLAG] = ctx->instr_cnt;
}

static HRESULT compile_args(compile_ctx_t *ctx, expression_t *args, unsigned *ret)
{
    unsigned arg_cnt = 0;
    HRESULT hres;

    while(args) {
        hres = compile_expression(ctx, args);
        if(FAILED(hres))
            return hres;

        arg_cnt++;
        args = args->next;
    }

    *ret = arg_cnt;
    return S_OK;
}

static HRESULT compile_member_expression(compile_ctx_t *ctx, member_expression_t *expr, BOOL ret_val)
{
    unsigned arg_cnt = 0;
    HRESULT hres;

    hres = compile_args(ctx, expr->args, &arg_cnt);
    if(FAILED(hres))
        return hres;

    if(expr->obj_expr) {
        FIXME("obj_expr not implemented\n");
        hres = E_NOTIMPL;
    }else {
        hres = push_instr_bstr_uint(ctx, ret_val ? OP_icall : OP_icallv, expr->identifier, arg_cnt);
    }

    return hres;
}

static HRESULT compile_unary_expression(compile_ctx_t *ctx, unary_expression_t *expr, vbsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->subexpr);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) == -1 ? E_OUTOFMEMORY : S_OK;
}

static HRESULT compile_binary_expression(compile_ctx_t *ctx, binary_expression_t *expr, vbsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->left);
    if(FAILED(hres))
        return hres;

    hres = compile_expression(ctx, expr->right);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) == -1 ? E_OUTOFMEMORY : S_OK;
}

static HRESULT compile_expression(compile_ctx_t *ctx, expression_t *expr)
{
    switch(expr->type) {
    case EXPR_ADD:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_add);
    case EXPR_BOOL:
        return push_instr_int(ctx, OP_bool, ((bool_expression_t*)expr)->value);
    case EXPR_CONCAT:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_concat);
    case EXPR_DOUBLE:
        return push_instr_double(ctx, OP_double, ((double_expression_t*)expr)->value);
    case EXPR_EMPTY:
        return push_instr(ctx, OP_empty) != -1 ? S_OK : E_OUTOFMEMORY;
    case EXPR_EQUAL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_equal);
    case EXPR_MEMBER:
        return compile_member_expression(ctx, (member_expression_t*)expr, TRUE);
    case EXPR_NEG:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_neg);
    case EXPR_NEQUAL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_nequal);
    case EXPR_NOT:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_not);
    case EXPR_NULL:
        return push_instr(ctx, OP_null) != -1 ? S_OK : E_OUTOFMEMORY;
    case EXPR_STRING:
        return push_instr_str(ctx, OP_string, ((string_expression_t*)expr)->value);
    case EXPR_SUB:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_sub);
    case EXPR_USHORT:
        return push_instr_int(ctx, OP_short, ((int_expression_t*)expr)->value);
    case EXPR_ULONG:
        return push_instr_int(ctx, OP_long, ((int_expression_t*)expr)->value);
    default:
        FIXME("Unimplemented expression type %d\n", expr->type);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT compile_if_statement(compile_ctx_t *ctx, if_statement_t *stat)
{
    unsigned cnd_jmp, endif_label = -1;
    elseif_decl_t *elseif_decl;
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    cnd_jmp = push_instr(ctx, OP_jmp_false);
    if(cnd_jmp == -1)
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, stat->if_stat);
    if(FAILED(hres))
        return hres;

    if(stat->else_stat || stat->elseifs) {
        endif_label = alloc_label(ctx);
        if(endif_label == -1)
            return E_OUTOFMEMORY;

        hres = push_instr_addr(ctx, OP_jmp, endif_label);
        if(FAILED(hres))
            return hres;
    }

    for(elseif_decl = stat->elseifs; elseif_decl; elseif_decl = elseif_decl->next) {
        instr_ptr(ctx, cnd_jmp)->arg1.uint = ctx->instr_cnt;

        hres = compile_expression(ctx, elseif_decl->expr);
        if(FAILED(hres))
            return hres;

        cnd_jmp = push_instr(ctx, OP_jmp_false);
        if(cnd_jmp == -1)
            return E_OUTOFMEMORY;

        hres = compile_statement(ctx, elseif_decl->stat);
        if(FAILED(hres))
            return hres;

        hres = push_instr_addr(ctx, OP_jmp, endif_label);
        if(FAILED(hres))
            return hres;
    }

    instr_ptr(ctx, cnd_jmp)->arg1.uint = ctx->instr_cnt;

    if(stat->else_stat) {
        hres = compile_statement(ctx, stat->else_stat);
        if(FAILED(hres))
            return hres;
    }

    if(endif_label != -1)
        label_set_addr(ctx, endif_label);
    return S_OK;
}

static HRESULT compile_assign_statement(compile_ctx_t *ctx, assign_statement_t *stat)
{
    HRESULT hres;

    hres = compile_expression(ctx, stat->value_expr);
    if(FAILED(hres))
        return hres;

    if(stat->member_expr->args) {
        FIXME("arguments support not implemented\n");
        return E_NOTIMPL;
    }

    if(stat->member_expr->obj_expr) {
        hres = compile_expression(ctx, stat->member_expr->obj_expr);
        if(FAILED(hres))
            return hres;

        hres = push_instr_bstr(ctx, OP_assign_member, stat->member_expr->identifier);
    }else {
        hres = push_instr_bstr(ctx, OP_assign_ident, stat->member_expr->identifier);
    }

    return hres;
}

static BOOL lookup_dim_decls(compile_ctx_t *ctx, const WCHAR *name)
{
    dim_decl_t *dim_decl;

    for(dim_decl = ctx->dim_decls; dim_decl; dim_decl = dim_decl->next) {
        if(!strcmpiW(dim_decl->name, name))
            return TRUE;
    }

    return FALSE;
}

static HRESULT compile_dim_statement(compile_ctx_t *ctx, dim_statement_t *stat)
{
    dim_decl_t *dim_decl = stat->dim_decls;

    while(1) {
        if(lookup_dim_decls(ctx, dim_decl->name)) {
            FIXME("dim %s name redefined\n", debugstr_w(dim_decl->name));
            return E_FAIL;
        }

        if(!dim_decl->next)
            break;
        dim_decl = dim_decl->next;
    }

    dim_decl->next = ctx->dim_decls;
    ctx->dim_decls = stat->dim_decls;
    return S_OK;
}

static HRESULT compile_statement(compile_ctx_t *ctx, statement_t *stat)
{
    HRESULT hres;

    while(stat) {
        switch(stat->type) {
        case STAT_ASSIGN:
            hres = compile_assign_statement(ctx, (assign_statement_t*)stat);
            break;
        case STAT_CALL:
            hres = compile_member_expression(ctx, ((call_statement_t*)stat)->expr, FALSE);
            break;
        case STAT_DIM:
            hres = compile_dim_statement(ctx, (dim_statement_t*)stat);
            break;
        case STAT_IF:
            hres = compile_if_statement(ctx, (if_statement_t*)stat);
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

static void resolve_labels(compile_ctx_t *ctx)
{
    instr_t *instr;

    for(instr = ctx->code->instrs; instr < ctx->code->instrs+ctx->instr_cnt; instr++) {
        if(instr_info[instr->op].arg1_type == ARG_ADDR && (instr->arg1.uint & LABEL_FLAG)) {
            assert((instr->arg1.uint & ~LABEL_FLAG) < ctx->labels_cnt);
            instr->arg1.uint = ctx->labels[instr->arg1.uint & ~LABEL_FLAG];
        }
        assert(instr_info[instr->op].arg2_type != ARG_ADDR);
    }

    ctx->labels_cnt = 0;
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

    resolve_labels(ctx);

    if(ctx->dim_decls) {
        dim_decl_t *dim_decl;
        dynamic_var_t *new_var;

        for(dim_decl = ctx->dim_decls; dim_decl; dim_decl = dim_decl->next) {
            new_var = compiler_alloc(ctx->code, sizeof(*new_var));
            if(!new_var)
                return E_OUTOFMEMORY;

            new_var->name = compiler_alloc_string(ctx->code, dim_decl->name);
            if(!new_var->name)
                return E_OUTOFMEMORY;

            V_VT(&new_var->v) = VT_EMPTY;

            new_var->next = ctx->global_vars;
            ctx->global_vars = new_var;
        }
    }

    return S_OK;
}

static BOOL lookup_script_identifier(script_ctx_t *script, const WCHAR *identifier)
{
    dynamic_var_t *var;

    for(var = script->global_vars; var; var = var->next) {
        if(!strcmpiW(var->name, identifier))
            return TRUE;
    }

    return FALSE;
}

static HRESULT check_script_collisions(compile_ctx_t *ctx, script_ctx_t *script)
{
    dynamic_var_t *var;

    for(var = ctx->global_vars; var; var = var->next) {
        if(lookup_script_identifier(script, var->name)) {
            FIXME("%s: redefined\n", debugstr_w(var->name));
            return E_FAIL;
        }
    }

    return S_OK;
}

void release_vbscode(vbscode_t *code)
{
    unsigned i;

    list_remove(&code->entry);

    for(i=0; i < code->bstr_cnt; i++)
        SysFreeString(code->bstr_pool[i]);

    vbsheap_free(&code->heap);

    heap_free(code->bstr_pool);
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
    vbsheap_init(&ret->heap);

    ret->option_explicit = ctx->parser.option_explicit;

    ret->bstr_pool = NULL;
    ret->bstr_pool_size = 0;
    ret->bstr_cnt = 0;

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

    ctx.global_vars = NULL;
    ctx.dim_decls = NULL;
    ctx.labels = NULL;
    ctx.labels_cnt = ctx.labels_size = 0;

    hres = compile_func(&ctx, ctx.parser.stats, &ctx.code->global_code);
    if(FAILED(hres)) {
        release_vbscode(ctx.code);
        return hres;
    }

    hres = check_script_collisions(&ctx, script);
    if(FAILED(hres)) {
        release_vbscode(ctx.code);
        return hres;
    }

    if(ctx.global_vars) {
        dynamic_var_t *var;

        for(var = ctx.global_vars; var->next; var = var->next);

        var->next = script->global_vars;
        script->global_vars = ctx.global_vars;
    }

    parser_release(&ctx.parser);

    list_add_tail(&script->code_list, &ctx.code->entry);
    *ret = ctx.code;
    return S_OK;
}
