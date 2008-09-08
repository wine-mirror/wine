/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#define EXPR_NOVAL   0x0001
#define EXPR_NEWREF  0x0002
#define EXPR_STRREF  0x0004

static inline HRESULT stat_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    return stat->eval(ctx, stat, rt, ret);
}

static inline HRESULT expr_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    return _expr->eval(ctx, _expr, flags, ei, ret);
}

static void exprval_release(exprval_t *val)
{
    switch(val->type) {
    case EXPRVAL_VARIANT:
        VariantClear(&val->u.var);
        return;
    case EXPRVAL_IDREF:
        if(val->u.idref.disp)
            IDispatch_Release(val->u.idref.disp);
        return;
    case EXPRVAL_NAMEREF:
        if(val->u.nameref.disp)
            IDispatch_Release(val->u.nameref.disp);
        SysFreeString(val->u.nameref.name);
    }
}

/* ECMA-262 3rd Edition    8.7.1 */
static HRESULT exprval_value(script_ctx_t *ctx, exprval_t *val, jsexcept_t *ei, VARIANT *ret)
{
    V_VT(ret) = VT_EMPTY;

    switch(val->type) {
    case EXPRVAL_VARIANT:
        return VariantCopy(ret, &val->u.var);
    case EXPRVAL_IDREF:
        if(!val->u.idref.disp) {
            FIXME("throw ReferenceError\n");
            return E_FAIL;
        }

        return disp_propget(val->u.idref.disp, val->u.idref.id, ctx->lcid, ret, ei, NULL/*FIXME*/);
    default:
        ERR("type %d\n", val->type);
        return E_FAIL;
    }
}

static HRESULT exprval_to_value(script_ctx_t *ctx, exprval_t *val, jsexcept_t *ei, VARIANT *ret)
{
    if(val->type == EXPRVAL_VARIANT) {
        *ret = val->u.var;
        V_VT(&val->u.var) = VT_EMPTY;
        return S_OK;
    }

    return exprval_value(ctx, val, ei, ret);
}

static HRESULT exprval_to_boolean(script_ctx_t *ctx, exprval_t *exprval, jsexcept_t *ei, VARIANT_BOOL *b)
{
    if(exprval->type != EXPRVAL_VARIANT) {
        VARIANT val;
        HRESULT hres;

        hres = exprval_to_value(ctx, exprval, ei, &val);
        if(FAILED(hres))
            return hres;

        hres = to_boolean(&val, b);
        VariantClear(&val);
        return hres;
    }

    return to_boolean(&exprval->u.var, b);
}

static void exprval_set_idref(exprval_t *val, IDispatch *disp, DISPID id)
{
    val->type = EXPRVAL_IDREF;
    val->u.idref.disp = disp;
    val->u.idref.id = id;

    if(disp)
        IDispatch_AddRef(disp);
}

void scope_release(scope_chain_t *scope)
{
    if(--scope->ref)
        return;

    if(scope->next)
        scope_release(scope->next);

    IDispatchEx_Release(_IDispatchEx_(scope->obj));
    heap_free(scope);
}

HRESULT create_exec_ctx(DispatchEx *var_disp, scope_chain_t *scope, exec_ctx_t **ret)
{
    exec_ctx_t *ctx;

    ctx = heap_alloc_zero(sizeof(exec_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    IDispatchEx_AddRef(_IDispatchEx_(var_disp));
    ctx->var_disp = var_disp;

    if(scope) {
        scope_addref(scope);
        ctx->scope_chain = scope;
    }

    *ret = ctx;
    return S_OK;
}

void exec_release(exec_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    if(ctx->scope_chain)
        scope_release(ctx->scope_chain);
    if(ctx->var_disp)
        IDispatchEx_Release(_IDispatchEx_(ctx->var_disp));
    heap_free(ctx);
}

static HRESULT dispex_get_id(IDispatchEx *dispex, BSTR name, DWORD flags, DISPID *id)
{
    *id = 0;

    return IDispatchEx_GetDispID(dispex, name, flags|fdexNameCaseSensitive, id);
}

static HRESULT disp_get_id(IDispatch *disp, BSTR name, DWORD flags, DISPID *id)
{
    IDispatchEx *dispex;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        TRACE("unsing IDispatch\n");

        *id = 0;
        return IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, id);
    }

    hres = dispex_get_id(dispex, name, flags, id);
    IDispatchEx_Release(dispex);
    return hres;
}

/* ECMA-262 3rd Edition    8.7.2 */
static HRESULT put_value(script_ctx_t *ctx, exprval_t *ref, VARIANT *v, jsexcept_t *ei)
{
    if(ref->type != EXPRVAL_IDREF) {
        FIXME("throw ReferemceError\n");
        return E_FAIL;
    }

    return disp_propput(ref->u.idref.disp, ref->u.idref.id, ctx->lcid, v, ei, NULL/*FIXME*/);
}

static HRESULT literal_to_var(literal_t *literal, VARIANT *v)
{
    V_VT(v) = literal->vt;

    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
        break;
    case VT_I4:
        V_I4(v) = literal->u.lval;
        break;
    case VT_R8:
        V_R8(v) = literal->u.dval;
        break;
    case VT_BSTR:
        V_BSTR(v) = SysAllocString(literal->u.wstr);
        break;
    case VT_BOOL:
        V_BOOL(v) = literal->u.bval;
        break;
    case VT_DISPATCH:
        IDispatch_AddRef(literal->u.disp);
        V_DISPATCH(v) = literal->u.disp;
        break;
    default:
        ERR("wrong type %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT exec_source(exec_ctx_t *ctx, parser_ctx_t *parser, source_elements_t *source, jsexcept_t *ei, VARIANT *retv)
{
    script_ctx_t *script = parser->script;
    parser_ctx_t *prev_parser;
    VARIANT val, tmp;
    statement_t *stat;
    exec_ctx_t *prev_ctx;
    return_type_t rt;
    HRESULT hres = S_OK;

    prev_ctx = script->exec_ctx;
    script->exec_ctx = ctx;

    prev_parser = ctx->parser;
    ctx->parser = parser;

    V_VT(&val) = VT_EMPTY;
    memset(&rt, 0, sizeof(rt));
    rt.type = RT_NORMAL;

    for(stat = source->statement; stat; stat = stat->next) {
        hres = stat_eval(ctx, stat, &rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&val);
        val = tmp;
        if(rt.type != RT_NORMAL)
            break;
    }

    script->exec_ctx = prev_ctx;
    ctx->parser = prev_parser;

    if(rt.type != RT_NORMAL && rt.type != RT_RETURN) {
        FIXME("wrong rt %d\n", rt.type);
        hres = E_FAIL;
    }

    *ei = rt.ei;
    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(retv)
        *retv = val;
    else
        VariantClear(&val);
    return S_OK;
}

/* ECMA-262 3rd Edition    10.1.4 */
static HRESULT identifier_eval(exec_ctx_t *ctx, BSTR identifier, DWORD flags, exprval_t *ret)
{
    scope_chain_t *scope;
    named_item_t *item;
    DISPID id = 0;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    for(scope = ctx->scope_chain; scope; scope = scope->next) {
        hres = dispex_get_id(_IDispatchEx_(scope->obj), identifier, 0, &id);
        if(SUCCEEDED(hres))
            break;
    }

    if(scope) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(scope->obj), id);
        return S_OK;
    }

    hres = dispex_get_id(_IDispatchEx_(ctx->parser->script->global), identifier, 0, &id);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->parser->script->global), id);
        return S_OK;
    }

    for(item = ctx->parser->script->named_items; item; item = item->next) {
        hres = disp_get_id(item->disp, identifier, 0, &id);
        if(SUCCEEDED(hres))
            break;
    }

    if(item) {
        exprval_set_idref(ret, (IDispatch*)item->disp, id);
        return S_OK;
    }

    hres = dispex_get_id(_IDispatchEx_(ctx->parser->script->script_disp), identifier, 0, &id);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->parser->script->script_disp), id);
        return S_OK;
    }

    if(flags & EXPR_NEWREF) {
        hres = dispex_get_id(_IDispatchEx_(ctx->var_disp), identifier, fdexNameEnsure, &id);
        if(FAILED(hres))
            return hres;

        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->var_disp), id);
        return S_OK;
    }

    WARN("Could not find identifier %s\n", debugstr_w(identifier));
    return E_FAIL;
}

HRESULT block_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    12.2 */
static HRESULT variable_list_eval(exec_ctx_t *ctx, variable_declaration_t *var_list, jsexcept_t *ei)
{
    variable_declaration_t *iter;
    HRESULT hres;

    for(iter = var_list; iter; iter = iter->next) {
        VARIANT val;

        if(iter->expr) {
            exprval_t exprval;

            hres = expr_eval(ctx, iter->expr, 0, ei, &exprval);
            if(FAILED(hres))
                break;

            hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
            exprval_release(&exprval);
            if(FAILED(hres))
                break;
        }else {
            V_VT(&val) = VT_EMPTY;
        }

        hres = jsdisp_propput_name(ctx->var_disp, iter->identifier, ctx->parser->script->lcid, &val, ei, NULL/*FIXME*/);
        VariantClear(&val);
        if(FAILED(hres))
            break;
    }

    return hres;
}

/* ECMA-262 3rd Edition    12.2 */
HRESULT var_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    var_statement_t *stat = (var_statement_t*)_stat;
    HRESULT hres;

    TRACE("\n");

    hres = variable_list_eval(ctx, stat->variable_list, &rt->ei);
    if(FAILED(hres))
        return hres;

    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.3 */
HRESULT empty_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    TRACE("\n");

    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.4 */
HRESULT expression_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    expression_statement_t *stat = (expression_statement_t*)_stat;
    exprval_t exprval;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, EXPR_NOVAL, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    *ret = val;
    TRACE("= %s\n", debugstr_variant(ret));
    return S_OK;
}

HRESULT if_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT dowhile_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT while_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT for_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT forin_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT continue_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT break_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT return_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT with_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT labelled_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT switch_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT throw_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT try_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT return_bool(exprval_t *ret, DWORD b)
{
    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_BOOL;
    V_BOOL(&ret->u.var) = b ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

HRESULT function_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT conditional_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT array_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT member_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void free_dp(DISPPARAMS *dp)
{
    DWORD i;

    for(i=0; i < dp->cArgs; i++)
        VariantClear(dp->rgvarg+i);
    heap_free(dp->rgvarg);
}

static HRESULT args_to_param(exec_ctx_t *ctx, argument_t *args, jsexcept_t *ei, DISPPARAMS *dp)
{
    VARIANTARG *vargs;
    exprval_t exprval;
    argument_t *iter;
    DWORD cnt = 0, i;
    HRESULT hres = S_OK;

    memset(dp, 0, sizeof(*dp));

    for(iter = args; iter; iter = iter->next)
        cnt++;
    if(!cnt)
        return S_OK;

    vargs = heap_alloc_zero(cnt * sizeof(*vargs));
    if(!vargs)
        return E_OUTOFMEMORY;

    for(i = cnt, iter = args; iter; iter = iter->next) {
        hres = expr_eval(ctx, iter->expr, 0, ei, &exprval);
        if(FAILED(hres))
            break;

        hres = exprval_to_value(ctx->parser->script, &exprval, ei, vargs + (--i));
        exprval_release(&exprval);
        if(FAILED(hres))
            break;
    }

    if(FAILED(hres)) {
        free_dp(dp);
        return hres;
    }

    dp->rgvarg = vargs;
    dp->cArgs = cnt;
    return S_OK;
}

HRESULT member_new_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT call_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    call_expression_t *expr = (call_expression_t*)_expr;
    VARIANT func, var;
    exprval_t exprval;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = args_to_param(ctx, expr->argument_list, ei, &dp);
    if(SUCCEEDED(hres)) {
        switch(exprval.type) {
        case EXPRVAL_IDREF:
            hres = disp_call(exprval.u.idref.disp, exprval.u.idref.id, ctx->parser->script->lcid, DISPATCH_METHOD,
                    &dp, flags & EXPR_NOVAL ? NULL : &var, ei, NULL/*FIXME*/);
            if(flags & EXPR_NOVAL)
                V_VT(&var) = VT_EMPTY;
            break;
        default:
            FIXME("unimplemented type %d\n", V_VT(&func));
            hres = E_NOTIMPL;
        }

        free_dp(&dp);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    TRACE("= %s\n", debugstr_variant(&var));
    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

HRESULT this_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    10.1.4 */
HRESULT identifier_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    identifier_expression_t *expr = (identifier_expression_t*)_expr;
    BSTR identifier;
    HRESULT hres;

    TRACE("\n");

    identifier = SysAllocString(expr->identifier);
    if(!identifier)
        return E_OUTOFMEMORY;

    hres = identifier_eval(ctx, identifier, flags, ret);

    SysFreeString(identifier);
    return hres;
}

/* ECMA-262 3rd Edition    7.8 */
HRESULT literal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    literal_expression_t *expr = (literal_expression_t*)_expr;
    VARIANT var;
    HRESULT hres;

    TRACE("\n");

    hres = literal_to_var(expr->literal, &var);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

HRESULT array_literal_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT property_value_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT comma_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT logical_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT logical_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_xor_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT instanceof_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT in_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT add_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT sub_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT mul_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT div_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT mod_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT delete_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT void_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT typeof_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT minus_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT plus_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT post_increment_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT post_decrement_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT pre_increment_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT pre_decrement_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT new_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT equal_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT equal2_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT not_equal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{

    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT not_equal2_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT less_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT lesseq_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT greater_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT greatereq_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_negation_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    11.4.9 */
HRESULT logical_negation_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT_BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_boolean(ctx->parser->script, &exprval, ei, &b);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, !b);
}

HRESULT left_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT right_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT right2_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    11.13.1 */
HRESULT assign_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    exprval_t exprval, exprvalr;
    VARIANT rval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression1, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression2, 0, ei, &exprvalr);
    if(SUCCEEDED(hres)) {
        hres = exprval_to_value(ctx->parser->script, &exprvalr, ei, &rval);
        exprval_release(&exprvalr);
    }

    if(SUCCEEDED(hres))
        hres = put_value(ctx->parser->script, &exprval, &rval, ei);

    exprval_release(&exprval);
    if(FAILED(hres)) {
        VariantClear(&rval);
        return hres;
    }

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = rval;
    return S_OK;
}

HRESULT assign_lshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_rshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_rrshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_add_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_sub_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_mul_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_div_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_mod_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_xor_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}
