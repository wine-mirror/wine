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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);


typedef struct {
    vbscode_t *code;
    instr_t *instr;
    script_ctx_t *script;

    unsigned stack_size;
    unsigned top;
    VARIANT *stack;
} exec_ctx_t;

typedef HRESULT (*instr_func_t)(exec_ctx_t*);

typedef enum {
    REF_NONE,
    REF_DISP
} ref_type_t;

typedef struct {
    ref_type_t type;
    union {
        struct {
            IDispatch *disp;
            DISPID id;
        } d;
    } u;
} ref_t;

static HRESULT lookup_identifier(exec_ctx_t *ctx, BSTR name, ref_t *ref)
{
    named_item_t *item;
    DISPID id;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(item, &ctx->script->named_items, named_item_t, entry) {
        if(item->flags & SCRIPTITEM_GLOBALMEMBERS) {
            hres = disp_get_id(item->disp, name, &id);
            if(SUCCEEDED(hres)) {
                ref->type = REF_DISP;
                ref->u.d.disp = item->disp;
                ref->u.d.id = id;
                return S_OK;
            }
        }
    }

    FIXME("create if no option explicit\n");

    ref->type = REF_NONE;
    return S_OK;
}

static inline VARIANT *stack_pop(exec_ctx_t *ctx)
{
    assert(ctx->top);
    return ctx->stack + --ctx->top;
}

static HRESULT stack_push(exec_ctx_t *ctx, VARIANT *v)
{
    if(ctx->stack_size == ctx->top) {
        VARIANT *new_stack;

        new_stack = heap_realloc(ctx->stack, ctx->stack_size*2);
        if(!new_stack) {
            VariantClear(v);
            return E_OUTOFMEMORY;
        }

        ctx->stack = new_stack;
        ctx->stack_size *= 2;
    }

    ctx->stack[ctx->top++] = *v;
    return S_OK;
}

static void stack_popn(exec_ctx_t *ctx, unsigned n)
{
    while(n--)
        VariantClear(stack_pop(ctx));
}

static HRESULT interp_icallv(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    DISPPARAMS dp = {0};
    ref_t ref;
    HRESULT hres;

    TRACE("\n");

    if(arg_cnt) {
        FIXME("arguments not implemented\n");
        return E_NOTIMPL;
    }

    hres = lookup_identifier(ctx, identifier, &ref);
    if(FAILED(hres))
        return hres;

    switch(ref.type) {
    case REF_DISP:
        hres = disp_call(ctx->script, ref.u.d.disp, ref.u.d.id, &dp, NULL);
        if(FAILED(hres))
            return hres;
        break;
    default:
        FIXME("%s not found\n", debugstr_w(identifier));
        return DISP_E_UNKNOWNNAME;
    }

    return S_OK;
}

static HRESULT interp_ret(exec_ctx_t *ctx)
{
    TRACE("\n");

    ctx->instr = NULL;
    return S_OK;
}

static HRESULT interp_bool(exec_ctx_t *ctx)
{
    const VARIANT_BOOL arg = ctx->instr->arg1.lng;
    VARIANT v;

    TRACE("%s\n", arg ? "true" : "false");

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_string(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(ctx->instr->arg1.str);
    if(!V_BSTR(&v))
        return E_OUTOFMEMORY;

    return stack_push(ctx, &v);
}

static const instr_func_t op_funcs[] = {
#define X(x,n,a,b) interp_ ## x,
OP_LIST
#undef X
};

static const unsigned op_move[] = {
#define X(x,n,a,b) n,
OP_LIST
#undef X
};

HRESULT exec_script(script_ctx_t *ctx, function_t *func)
{
    exec_ctx_t exec;
    vbsop_t op;
    HRESULT hres = S_OK;

    exec.stack_size = 16;
    exec.top = 0;
    exec.stack = heap_alloc(exec.stack_size * sizeof(VARIANT));
    if(!exec.stack)
        return E_OUTOFMEMORY;

    exec.code = func->code_ctx;
    exec.instr = exec.code->instrs + func->code_off;
    exec.script = ctx;

    while(exec.instr) {
        op = exec.instr->op;
        hres = op_funcs[op](&exec);
        if(FAILED(hres)) {
            FIXME("Failed %08x\n", hres);
            stack_popn(&exec, exec.top);
            break;
        }

        exec.instr += op_move[op];
    }

    assert(!exec.top);
    heap_free(exec.stack);

    return hres;
}
