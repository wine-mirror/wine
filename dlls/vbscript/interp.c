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
    function_t *func;

    unsigned stack_size;
    unsigned top;
    VARIANT *stack;
} exec_ctx_t;

typedef HRESULT (*instr_func_t)(exec_ctx_t*);

typedef enum {
    REF_NONE,
    REF_DISP,
    REF_VAR
} ref_type_t;

typedef struct {
    ref_type_t type;
    union {
        struct {
            IDispatch *disp;
            DISPID id;
        } d;
        VARIANT *v;
    } u;
} ref_t;

typedef struct {
    VARIANT *v;
    VARIANT store;
    BOOL owned;
} variant_val_t;

static BOOL lookup_dynamic_vars(dynamic_var_t *var, const WCHAR *name, ref_t *ref)
{
    while(var) {
        if(!strcmpiW(var->name, name)) {
            ref->type = REF_VAR;
            ref->u.v = &var->v;
            return TRUE;
        }

        var = var->next;
    }

    return FALSE;
}

static HRESULT lookup_identifier(exec_ctx_t *ctx, BSTR name, ref_t *ref)
{
    named_item_t *item;
    DISPID id;
    HRESULT hres;

    if(lookup_dynamic_vars(ctx->script->global_vars, name, ref))
        return S_OK;

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

    if(!ctx->func->code_ctx->option_explicit)
        FIXME("create an attempt to set\n");

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

static HRESULT stack_pop_val(exec_ctx_t *ctx, variant_val_t *v)
{
    VARIANT *var;

    var = stack_pop(ctx);

    if(V_VT(var) == (VT_BYREF|VT_VARIANT)) {
        v->owned = FALSE;
        var = V_VARIANTREF(var);
    }else {
        v->owned = TRUE;
    }

    if(V_VT(var) == VT_DISPATCH) {
        FIXME("got dispatch - get its default value\n");
        return E_NOTIMPL;
    }else {
        v->v = var;
    }

    return S_OK;
}

static inline void release_val(variant_val_t *v)
{
    if(v->owned)
        VariantClear(v->v);
}

static HRESULT stack_pop_disp(exec_ctx_t *ctx, IDispatch **ret)
{
    VARIANT *v = stack_pop(ctx);

    if(V_VT(v) == VT_DISPATCH) {
        *ret = V_DISPATCH(v);
        return S_OK;
    }

    if(V_VT(v) != (VT_VARIANT|VT_BYREF)) {
        FIXME("not supported type: %s\n", debugstr_variant(v));
        VariantClear(v);
        return E_FAIL;
    }

    v = V_BYREF(v);
    if(V_VT(v) != VT_DISPATCH) {
        FIXME("not disp %s\n", debugstr_variant(v));
        return E_FAIL;
    }

    if(V_DISPATCH(v))
        IDispatch_AddRef(V_DISPATCH(v));
    *ret = V_DISPATCH(v);
    return S_OK;
}

static inline void instr_jmp(exec_ctx_t *ctx, unsigned addr)
{
    ctx->instr = ctx->code->instrs + addr;
}

static void vbstack_to_dp(exec_ctx_t *ctx, unsigned arg_cnt, DISPPARAMS *dp)
{
    dp->cArgs = arg_cnt;
    dp->rgdispidNamedArgs = NULL;
    dp->cNamedArgs = 0;

    if(arg_cnt) {
        VARIANT tmp;
        unsigned i;

        assert(ctx->top >= arg_cnt);

        for(i=1; i*2 <= arg_cnt; i++) {
            tmp = ctx->stack[ctx->top-i];
            ctx->stack[ctx->top-i] = ctx->stack[ctx->top-arg_cnt+i-1];
            ctx->stack[ctx->top-arg_cnt+i-1] = tmp;
        }

        dp->rgvarg = ctx->stack + ctx->top-arg_cnt;
    }else {
        dp->rgvarg = NULL;
    }
}

static HRESULT do_icall(exec_ctx_t *ctx, VARIANT *res)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    ref_t ref = {0};
    DISPPARAMS dp;
    HRESULT hres;

    hres = lookup_identifier(ctx, identifier, &ref);
    if(FAILED(hres))
        return hres;

    vbstack_to_dp(ctx, arg_cnt, &dp);

    switch(ref.type) {
    case REF_VAR:
        if(!res) {
            FIXME("REF_VAR no res\n");
            return E_NOTIMPL;
        }

        if(arg_cnt) {
            FIXME("arguments not implemented\n");
            return E_NOTIMPL;
        }

        V_VT(res) = VT_BYREF|VT_VARIANT;
        V_BYREF(res) = V_VT(ref.u.v) == (VT_VARIANT|VT_BYREF) ? V_VARIANTREF(ref.u.v) : ref.u.v;
        break;
    case REF_DISP:
        hres = disp_call(ctx->script, ref.u.d.disp, ref.u.d.id, &dp, res);
        if(FAILED(hres))
            return hres;
        break;
    case REF_NONE:
        FIXME("%s not found\n", debugstr_w(identifier));
        return DISP_E_UNKNOWNNAME;
    }

    stack_popn(ctx, arg_cnt);
    return S_OK;
}

static HRESULT interp_icall(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = do_icall(ctx, &v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_icallv(exec_ctx_t *ctx)
{
    TRACE("\n");
    return do_icall(ctx, NULL);
}

static HRESULT assign_ident(exec_ctx_t *ctx, BSTR name, VARIANT *val, BOOL own_val)
{
    ref_t ref;
    HRESULT hres;

    hres = lookup_identifier(ctx, name, &ref);
    if(FAILED(hres))
        return hres;

    switch(ref.type) {
    case REF_VAR: {
        VARIANT *v = ref.u.v;

        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        if(own_val) {
            VariantClear(v);
            *v = *val;
            hres = S_OK;
        }else {
            hres = VariantCopy(v, val);
        }
        break;
    }
    case REF_DISP:
        hres = disp_propput(ctx->script, ref.u.d.disp, ref.u.d.id, val);
        if(own_val)
            VariantClear(val);
        break;
    case REF_NONE:
        FIXME("%s not found\n", debugstr_w(name));
        if(own_val)
            VariantClear(val);
        return DISP_E_UNKNOWNNAME;
    }

    return hres;
}

static HRESULT interp_assign_ident(exec_ctx_t *ctx)
{
    const BSTR arg = ctx->instr->arg1.bstr;
    variant_val_t v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    hres = stack_pop_val(ctx, &v);
    if(FAILED(hres))
        return hres;

    return assign_ident(ctx, arg, v.v, v.owned);
}

static HRESULT interp_assign_member(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    variant_val_t val;
    IDispatch *obj;
    DISPID id;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    hres = stack_pop_disp(ctx, &obj);
    if(FAILED(hres))
        return hres;

    if(!obj) {
        FIXME("NULL obj\n");
        return E_FAIL;
    }

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres)) {
        IDispatch_Release(obj);
        return hres;
    }

    hres = disp_get_id(obj, identifier, &id);
    if(SUCCEEDED(hres))
        hres = disp_propput(ctx->script, obj, id, val.v);

    release_val(&val);
    IDispatch_Release(obj);
    return hres;
}

static HRESULT interp_jmp(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;

    TRACE("%u\n", arg);

    instr_jmp(ctx, arg);
    return S_OK;
}

static HRESULT interp_jmp_false(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;
    variant_val_t val;
    HRESULT hres;

    TRACE("%u\n", arg);

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    if(V_VT(val.v) != VT_BOOL) {
        FIXME("unsupported for %s\n", debugstr_variant(val.v));
        release_val(&val);
        return E_NOTIMPL;
    }

    if(V_BOOL(val.v))
        ctx->instr++;
    else
        instr_jmp(ctx, ctx->instr->arg1.uint);
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

static HRESULT interp_long(exec_ctx_t *ctx)
{
    const LONG arg = ctx->instr->arg1.lng;
    VARIANT v;

    TRACE("%d\n", arg);

    V_VT(&v) = VT_I4;
    V_I4(&v) = arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_short(exec_ctx_t *ctx)
{
    const LONG arg = ctx->instr->arg1.lng;
    VARIANT v;

    TRACE("%d\n", arg);

    V_VT(&v) = VT_I2;
    V_I2(&v) = arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_double(exec_ctx_t *ctx)
{
    const DOUBLE *arg = ctx->instr->arg1.dbl;
    VARIANT v;

    TRACE("%lf\n", *arg);

    V_VT(&v) = VT_R8;
    V_R8(&v) = *arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_empty(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_EMPTY;
    return stack_push(ctx, &v);
}

static HRESULT interp_null(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_NULL;
    return stack_push(ctx, &v);
}

static HRESULT interp_not(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    hres = VarNot(val.v, &v);
    release_val(&val);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT cmp_oper(exec_ctx_t *ctx)
{
    variant_val_t l, r;
    HRESULT hres;

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        if(V_VT(l.v) == VT_NULL || V_VT(r.v) == VT_NULL) {
            FIXME("comparing nulls is not implemented\n");
            hres = E_NOTIMPL;
        }else {
            hres = VarCmp(l.v, r.v, ctx->script->lcid, 0);
        }
    }

    release_val(&r);
    release_val(&l);
    return hres;
}

static HRESULT interp_equal(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_nequal(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres != VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_concat(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarCat(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_add(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarAdd(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_sub(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarSub(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_mod(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarMod(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_idiv(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarIdiv(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_div(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarDiv(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_mul(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarMul(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_exp(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarPow(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_neg(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    hres = VarNeg(val.v, &v);
    release_val(&val);
    if(FAILED(hres))
        return hres;

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
    exec.func = func;

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
