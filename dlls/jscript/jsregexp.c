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

#include <math.h>

#include "jscript.h"
#include "regexp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    JSRegExp *jsregexp;
    jsstr_t *str;
    INT last_index;
    jsval_t last_index_val;
} RegExpInstance;

static const WCHAR sourceW[] = {'s','o','u','r','c','e',0};
static const WCHAR globalW[] = {'g','l','o','b','a','l',0};
static const WCHAR ignoreCaseW[] = {'i','g','n','o','r','e','C','a','s','e',0};
static const WCHAR multilineW[] = {'m','u','l','t','i','l','i','n','e',0};
static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR execW[] = {'e','x','e','c',0};
static const WCHAR testW[] = {'t','e','s','t',0};

static const WCHAR leftContextW[] =
    {'l','e','f','t','C','o','n','t','e','x','t',0};
static const WCHAR rightContextW[] =
    {'r','i','g','h','t','C','o','n','t','e','x','t',0};

static const WCHAR idx1W[] = {'$','1',0};
static const WCHAR idx2W[] = {'$','2',0};
static const WCHAR idx3W[] = {'$','3',0};
static const WCHAR idx4W[] = {'$','4',0};
static const WCHAR idx5W[] = {'$','5',0};
static const WCHAR idx6W[] = {'$','6',0};
static const WCHAR idx7W[] = {'$','7',0};
static const WCHAR idx8W[] = {'$','8',0};
static const WCHAR idx9W[] = {'$','9',0};

static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
static const WCHAR emptyW[] = {0};

static inline RegExpInstance *regexp_from_vdisp(vdisp_t *vdisp)
{
    return (RegExpInstance*)vdisp->u.jsdisp;
}

static void set_last_index(RegExpInstance *This, DWORD last_index)
{
    This->last_index = last_index;
    jsval_release(This->last_index_val);
    This->last_index_val = jsval_number(last_index);
}

static HRESULT do_regexp_match_next(script_ctx_t *ctx, RegExpInstance *regexp, DWORD rem_flags,
        jsstr_t *str, const WCHAR **cp, match_result_t **parens, DWORD *parens_size,
        DWORD *parens_cnt, match_result_t *ret)
{
    REMatchState *result;
    DWORD matchlen;
    HRESULT hres;

    hres = MatchRegExpNext(regexp->jsregexp, str->str, jsstr_length(str),
            cp, &ctx->tmp_heap, &result, &matchlen);
    if(FAILED(hres))
        return hres;
    if(hres == S_FALSE) {
        if(rem_flags & REM_RESET_INDEX)
            set_last_index(regexp, 0);
        return S_FALSE;
    }

    if(parens) {
        if(regexp->jsregexp->parenCount > *parens_size) {
            match_result_t *new_parens;

            if(*parens)
                new_parens = heap_realloc(*parens, sizeof(match_result_t)*regexp->jsregexp->parenCount);
            else
                new_parens = heap_alloc(sizeof(match_result_t)*regexp->jsregexp->parenCount);
            if(!new_parens)
                return E_OUTOFMEMORY;

            *parens_size = regexp->jsregexp->parenCount;
            *parens = new_parens;
        }
    }

    if(!(rem_flags & REM_NO_CTX_UPDATE) && ctx->last_match != str) {
        jsstr_release(ctx->last_match);
        ctx->last_match = jsstr_addref(str);
    }

    if(parens) {
        DWORD i;

        *parens_cnt = regexp->jsregexp->parenCount;

        for(i=0; i < regexp->jsregexp->parenCount; i++) {
            if(result->parens[i].index == -1) {
                (*parens)[i].str = NULL;
                (*parens)[i].len = 0;
            }else {
                (*parens)[i].str = str->str + result->parens[i].index;
                (*parens)[i].len = result->parens[i].length;
            }
        }
    }

    if(!(rem_flags & REM_NO_CTX_UPDATE)) {
        DWORD i, n = min(sizeof(ctx->match_parens)/sizeof(ctx->match_parens[0]), regexp->jsregexp->parenCount);

        for(i=0; i < n; i++) {
            if(result->parens[i].index == -1) {
                ctx->match_parens[i].str = NULL;
                ctx->match_parens[i].len = 0;
            }else {
                ctx->match_parens[i].str = ctx->last_match->str + result->parens[i].index;
                ctx->match_parens[i].len = result->parens[i].length;
            }
        }

        if(n < sizeof(ctx->match_parens)/sizeof(ctx->match_parens[0]))
            memset(ctx->match_parens+n, 0, sizeof(ctx->match_parens) - n*sizeof(ctx->match_parens[0]));
    }

    ret->str = result->cp-matchlen;
    ret->len = matchlen;
    set_last_index(regexp, result->cp-str->str);

    if(!(rem_flags & REM_NO_CTX_UPDATE)) {
        ctx->last_match_index = ret->str-str->str;
        ctx->last_match_length = matchlen;
    }

    return S_OK;
}

HRESULT regexp_match_next(script_ctx_t *ctx, jsdisp_t *dispex, DWORD rem_flags, jsstr_t *str,
        const WCHAR **cp, match_result_t **parens, DWORD *parens_size, DWORD *parens_cnt,
        match_result_t *ret)
{
    RegExpInstance *regexp = (RegExpInstance*)dispex;
    heap_pool_t *mark;
    HRESULT hres;

    if((rem_flags & REM_CHECK_GLOBAL) && !(regexp->jsregexp->flags & JSREG_GLOB))
        return S_FALSE;

    mark = heap_pool_mark(&ctx->tmp_heap);

    hres = do_regexp_match_next(ctx, regexp, rem_flags, str, cp, parens, parens_size, parens_cnt, ret);

    heap_pool_clear(mark);
    return hres;
}

static HRESULT regexp_match(script_ctx_t *ctx, jsdisp_t *dispex, jsstr_t *str, BOOL gflag,
        match_result_t **match_result, DWORD *result_cnt)
{
    RegExpInstance *This = (RegExpInstance*)dispex;
    match_result_t *ret = NULL, cres;
    const WCHAR *cp = str->str;
    DWORD i=0, ret_size = 0;
    heap_pool_t *mark;
    HRESULT hres;

    mark = heap_pool_mark(&ctx->tmp_heap);

    while(1) {
        hres = do_regexp_match_next(ctx, This, 0, str, &cp, NULL, NULL, NULL, &cres);
        if(hres == S_FALSE) {
            hres = S_OK;
            break;
        }

        if(FAILED(hres))
            break;

        if(ret_size == i) {
            if(ret) {
                match_result_t *old_ret = ret;

                ret = heap_realloc(old_ret, (ret_size <<= 1) * sizeof(match_result_t));
                if(!ret)
                    heap_free(old_ret);
            }else {
                ret = heap_alloc((ret_size=4) * sizeof(match_result_t));
            }
            if(!ret) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }

        ret[i++] = cres;

        if(!gflag && !(This->jsregexp->flags & JSREG_GLOB)) {
            hres = S_OK;
            break;
        }
    }

    heap_pool_clear(mark);
    if(FAILED(hres)) {
        heap_free(ret);
        return hres;
    }

    *match_result = ret;
    *result_cnt = i;
    return S_OK;
}

static HRESULT RegExp_source(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        RegExpInstance *This = regexp_from_vdisp(jsthis);
        *r = jsval_string(jsstr_addref(This->str));
        break;
    }
    default:
        FIXME("Unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT RegExp_global(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_ignoreCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_multiline(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static INT index_from_val(script_ctx_t *ctx, jsval_t v)
{
    double n;
    HRESULT hres;

    hres = to_number(ctx, v, &n);
    if(FAILED(hres)) {
        clear_ei(ctx); /* FIXME: Move ignoring exceptions to to_primitive */
        return 0;
    }

    n = floor(n);
    return is_int32(n) ? n : 0;
}

static HRESULT RegExp_lastIndex(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        RegExpInstance *regexp = regexp_from_vdisp(jsthis);

        return jsval_copy(regexp->last_index_val, r);
    }
    case DISPATCH_PROPERTYPUT: {
        RegExpInstance *regexp = regexp_from_vdisp(jsthis);
        HRESULT hres;

        hres = jsval_copy(argv[0], &regexp->last_index_val);
        if(FAILED(hres))
            return hres;

        regexp->last_index = index_from_val(ctx, argv[0]);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT RegExp_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT create_match_array(script_ctx_t *ctx, jsstr_t *input, const match_result_t *result,
        const match_result_t *parens, DWORD parens_cnt, IDispatch **ret)
{
    jsdisp_t *array;
    jsstr_t *str;
    DWORD i;
    HRESULT hres = S_OK;

    static const WCHAR indexW[] = {'i','n','d','e','x',0};
    static const WCHAR inputW[] = {'i','n','p','u','t',0};
    static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};
    static const WCHAR zeroW[] = {'0',0};

    hres = create_array(ctx, parens_cnt+1, &array);
    if(FAILED(hres))
        return hres;

    for(i=0; i < parens_cnt; i++) {
        str = jsstr_alloc_len(parens[i].str, parens[i].len);
        if(!str) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = jsdisp_propput_idx(array, i+1, jsval_string(str));
        jsstr_release(str);
        if(FAILED(hres))
            break;
    }

    while(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(array, indexW, jsval_number(result->str-input->str));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, lastIndexW, jsval_number(result->str-input->str+result->len));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, inputW, jsval_string(jsstr_addref(input)));
        if(FAILED(hres))
            break;

        str = jsstr_alloc_len(result->str, result->len);
        if(!str) {
            hres = E_OUTOFMEMORY;
            break;
        }
        hres = jsdisp_propput_name(array, zeroW, jsval_string(str));
        jsstr_release(str);
        break;
    }

    if(FAILED(hres)) {
        jsdisp_release(array);
        return hres;
    }

    *ret = to_disp(array);
    return S_OK;
}

static HRESULT run_exec(script_ctx_t *ctx, vdisp_t *jsthis, jsval_t arg, jsstr_t **input,
        match_result_t *match, match_result_t **parens, DWORD *parens_cnt, BOOL *ret)
{
    RegExpInstance *regexp;
    DWORD parens_size = 0, last_index = 0;
    const WCHAR *cp;
    jsstr_t *string;
    HRESULT hres;

    if(!is_vclass(jsthis, JSCLASS_REGEXP)) {
        FIXME("Not a RegExp\n");
        return E_NOTIMPL;
    }

    regexp = regexp_from_vdisp(jsthis);

    hres = to_string(ctx, arg, &string);
    if(FAILED(hres))
        return hres;

    if(regexp->jsregexp->flags & JSREG_GLOB) {
        if(regexp->last_index < 0) {
            jsstr_release(string);
            set_last_index(regexp, 0);
            *ret = FALSE;
            if(input)
                *input = jsstr_empty();
            return S_OK;
        }

        last_index = regexp->last_index;
    }

    cp = string->str + last_index;
    hres = regexp_match_next(ctx, &regexp->dispex, REM_RESET_INDEX, string, &cp, parens,
            parens ? &parens_size : NULL, parens_cnt, match);
    if(FAILED(hres)) {
        jsstr_release(string);
        return hres;
    }

    *ret = hres == S_OK;
    if(input)
        *input = string;
    else
        jsstr_release(string);
    return S_OK;
}

static HRESULT RegExp_exec(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_result_t *parens = NULL, match;
    DWORD parens_cnt = 0;
    BOOL b;
    jsstr_t *string;
    HRESULT hres;

    TRACE("\n");

    hres = run_exec(ctx, jsthis, argc ? argv[0] : jsval_string(jsstr_empty()), &string, &match, &parens, &parens_cnt, &b);
    if(FAILED(hres)) {
        heap_free(parens);
        return hres;
    }

    if(r) {
        if(b) {
            IDispatch *ret;

            hres = create_match_array(ctx, string, &match, parens, parens_cnt, &ret);
            if(SUCCEEDED(hres))
                *r = jsval_disp(ret);
        }else {
            *r = jsval_null();
        }
    }

    heap_free(parens);
    jsstr_release(string);
    return hres;
}

static HRESULT RegExp_test(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_result_t match;
    jsstr_t *undef_str;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        undef_str = jsstr_alloc(undefinedW);
        if(!undef_str)
            return E_OUTOFMEMORY;
    }

    hres = run_exec(ctx, jsthis, argc ? argv[0] : jsval_string(undef_str), NULL, &match, NULL, NULL, &b);
    if(!argc)
        jsstr_release(undef_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_bool(b);
    return S_OK;
}

static HRESULT RegExp_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, JS_E_FUNCTION_EXPECTED, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void RegExp_destructor(jsdisp_t *dispex)
{
    RegExpInstance *This = (RegExpInstance*)dispex;

    if(This->jsregexp)
        js_DestroyRegExp(This->jsregexp);
    jsval_release(This->last_index_val);
    jsstr_release(This->str);
    heap_free(This);
}

static const builtin_prop_t RegExp_props[] = {
    {execW,                  RegExp_exec,                  PROPF_METHOD|1},
    {globalW,                RegExp_global,                0},
    {ignoreCaseW,            RegExp_ignoreCase,            0},
    {lastIndexW,             RegExp_lastIndex,             0},
    {multilineW,             RegExp_multiline,             0},
    {sourceW,                RegExp_source,                0},
    {testW,                  RegExp_test,                  PROPF_METHOD|1},
    {toStringW,              RegExp_toString,              PROPF_METHOD}
};

static const builtin_info_t RegExp_info = {
    JSCLASS_REGEXP,
    {NULL, RegExp_value, 0},
    sizeof(RegExp_props)/sizeof(*RegExp_props),
    RegExp_props,
    RegExp_destructor,
    NULL
};

static const builtin_prop_t RegExpInst_props[] = {
    {globalW,                RegExp_global,                0},
    {ignoreCaseW,            RegExp_ignoreCase,            0},
    {lastIndexW,             RegExp_lastIndex,             0},
    {multilineW,             RegExp_multiline,             0},
    {sourceW,                RegExp_source,                0}
};

static const builtin_info_t RegExpInst_info = {
    JSCLASS_REGEXP,
    {NULL, RegExp_value, 0},
    sizeof(RegExpInst_props)/sizeof(*RegExpInst_props),
    RegExpInst_props,
    RegExp_destructor,
    NULL
};

static HRESULT alloc_regexp(script_ctx_t *ctx, jsdisp_t *object_prototype, RegExpInstance **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    regexp = heap_alloc_zero(sizeof(RegExpInstance));
    if(!regexp)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&regexp->dispex, ctx, &RegExp_info, object_prototype);
    else
        hres = init_dispex_from_constr(&regexp->dispex, ctx, &RegExpInst_info, ctx->regexp_constr);

    if(FAILED(hres)) {
        heap_free(regexp);
        return hres;
    }

    *ret = regexp;
    return S_OK;
}

HRESULT create_regexp(script_ctx_t *ctx, jsstr_t *src, DWORD flags, jsdisp_t **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    TRACE("%s %x\n", debugstr_jsstr(src), flags);

    hres = alloc_regexp(ctx, NULL, &regexp);
    if(FAILED(hres))
        return hres;

    regexp->str = jsstr_addref(src);
    regexp->last_index_val = jsval_number(0);

    regexp->jsregexp = js_NewRegExp(ctx, &ctx->tmp_heap, regexp->str->str,
            jsstr_length(regexp->str), flags, FALSE);
    if(!regexp->jsregexp) {
        WARN("js_NewRegExp failed\n");
        jsdisp_release(&regexp->dispex);
        return E_FAIL;
    }

    *ret = &regexp->dispex;
    return S_OK;
}

HRESULT create_regexp_var(script_ctx_t *ctx, jsval_t src_arg, jsval_t *flags_arg, jsdisp_t **ret)
{
    jsstr_t *src, *opt = NULL;
    DWORD flags;
    HRESULT hres;

    if(is_object_instance(src_arg)) {
        jsdisp_t *obj;

        obj = iface_to_jsdisp((IUnknown*)get_object(src_arg));
        if(obj) {
            if(is_class(obj, JSCLASS_REGEXP)) {
                RegExpInstance *regexp = (RegExpInstance*)obj;

                hres = create_regexp(ctx, regexp->str, regexp->jsregexp->flags, ret);
                jsdisp_release(obj);
                return hres;
            }

            jsdisp_release(obj);
        }
    }

    if(!is_string(src_arg)) {
        FIXME("src_arg = %s\n", debugstr_jsval(src_arg));
        return E_NOTIMPL;
    }

    src = get_string(src_arg);

    if(flags_arg) {
        if(!is_string(*flags_arg)) {
            FIXME("unimplemented for %s\n", debugstr_jsval(*flags_arg));
            return E_NOTIMPL;
        }

        opt = get_string(*flags_arg);
    }

    hres = parse_regexp_flags(opt ? opt->str : NULL, opt ? jsstr_length(opt) : 0, &flags);
    if(FAILED(hres))
        return hres;

    return create_regexp(ctx, src, flags, ret);
}

HRESULT regexp_string_match(script_ctx_t *ctx, jsdisp_t *re, jsstr_t *str, jsval_t *r)
{
    static const WCHAR indexW[] = {'i','n','d','e','x',0};
    static const WCHAR inputW[] = {'i','n','p','u','t',0};
    static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};

    RegExpInstance *regexp = (RegExpInstance*)re;
    match_result_t *match_result;
    unsigned match_cnt, i;
    jsdisp_t *array;
    HRESULT hres;

    if(!(regexp->jsregexp->flags & JSREG_GLOB)) {
        match_result_t match, *parens = NULL;
        DWORD parens_cnt, parens_size = 0;
        const WCHAR *cp = str->str;

        hres = regexp_match_next(ctx, &regexp->dispex, 0, str, &cp, &parens, &parens_size, &parens_cnt, &match);
        if(FAILED(hres)) {
            heap_free(parens);
            return hres;
        }

        if(r) {
            if(hres == S_OK) {
                IDispatch *ret;

                hres = create_match_array(ctx, str, &match, parens, parens_cnt, &ret);
                if(SUCCEEDED(hres))
                    *r = jsval_disp(ret);
            }else {
                *r = jsval_null();
            }
        }

        heap_free(parens);
        return S_OK;
    }

    hres = regexp_match(ctx, &regexp->dispex, str, FALSE, &match_result, &match_cnt);
    if(FAILED(hres))
        return hres;

    if(!match_cnt) {
        TRACE("no match\n");

        if(r)
            *r = jsval_null();
        return S_OK;
    }

    hres = create_array(ctx, match_cnt, &array);
    if(FAILED(hres))
        return hres;

    for(i=0; i < match_cnt; i++) {
        jsstr_t *tmp_str;

        tmp_str = jsstr_alloc_len(match_result[i].str, match_result[i].len);
        if(!tmp_str) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
        jsstr_release(tmp_str);
        if(FAILED(hres))
            break;
    }

    while(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(array, indexW, jsval_number(match_result[match_cnt-1].str-str->str));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, lastIndexW,
                jsval_number(match_result[match_cnt-1].str-str->str+match_result[match_cnt-1].len));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, inputW, jsval_string(str));
        break;
    }

    heap_free(match_result);

    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);
    return hres;
}

static HRESULT global_idx(script_ctx_t *ctx, DWORD flags, DWORD idx, jsval_t *r)
{
    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        jsstr_t *ret;

        ret = jsstr_alloc_len(ctx->match_parens[idx].str, ctx->match_parens[idx].len);
        if(!ret)
            return E_OUTOFMEMORY;

        *r = jsval_string(ret);
        break;
    }
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        FIXME("unsupported flags\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT RegExpConstr_idx1(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 0, r);
}

static HRESULT RegExpConstr_idx2(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 1, r);
}

static HRESULT RegExpConstr_idx3(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 2, r);
}

static HRESULT RegExpConstr_idx4(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 3, r);
}

static HRESULT RegExpConstr_idx5(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 4, r);
}

static HRESULT RegExpConstr_idx6(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 5, r);
}

static HRESULT RegExpConstr_idx7(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 6, r);
}

static HRESULT RegExpConstr_idx8(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 7, r);
}

static HRESULT RegExpConstr_idx9(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, flags, 8, r);
}

static HRESULT RegExpConstr_leftContext(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        jsstr_t *ret;

        ret = jsstr_alloc_len(ctx->last_match->str, ctx->last_match_index);
        if(!ret)
            return E_OUTOFMEMORY;

        *r = jsval_string(ret);
        break;
    }
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        FIXME("unsupported flags\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT RegExpConstr_rightContext(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
         unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        jsstr_t *ret;

        ret = jsstr_alloc(ctx->last_match->str+ctx->last_match_index+ctx->last_match_length);
        if(!ret)
            return E_OUTOFMEMORY;

        *r = jsval_string(ret);
        break;
    }
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        FIXME("unsupported flags\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT RegExpConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc) {
            if(is_object_instance(argv[0])) {
                jsdisp_t *jsdisp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
                if(jsdisp) {
                    if(is_class(jsdisp, JSCLASS_REGEXP)) {
                        if(argc > 1 && !is_undefined(argv[1])) {
                            jsdisp_release(jsdisp);
                            return throw_regexp_error(ctx, JS_E_REGEXP_SYNTAX, NULL);
                        }

                        if(r)
                            *r = jsval_obj(jsdisp);
                        else
                            jsdisp_release(jsdisp);
                        return S_OK;
                    }
                    jsdisp_release(jsdisp);
                }
            }
        }
        /* fall through */
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *ret;
        HRESULT hres;

        if(!argc) {
            FIXME("no args\n");
            return E_NOTIMPL;
        }

        hres = create_regexp_var(ctx, argv[0], argc > 1 ? argv+1 : NULL, &ret);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_obj(ret);
        else
            jsdisp_release(ret);
        return S_OK;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t RegExpConstr_props[] = {
    {idx1W,           RegExpConstr_idx1,           0},
    {idx2W,           RegExpConstr_idx2,           0},
    {idx3W,           RegExpConstr_idx3,           0},
    {idx4W,           RegExpConstr_idx4,           0},
    {idx5W,           RegExpConstr_idx5,           0},
    {idx6W,           RegExpConstr_idx6,           0},
    {idx7W,           RegExpConstr_idx7,           0},
    {idx8W,           RegExpConstr_idx8,           0},
    {idx9W,           RegExpConstr_idx9,           0},
    {leftContextW,    RegExpConstr_leftContext,    0},
    {rightContextW,   RegExpConstr_rightContext,   0}
};

static const builtin_info_t RegExpConstr_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(RegExpConstr_props)/sizeof(*RegExpConstr_props),
    RegExpConstr_props,
    NULL,
    NULL
};

HRESULT create_regexp_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    static const WCHAR RegExpW[] = {'R','e','g','E','x','p',0};

    hres = alloc_regexp(ctx, object_prototype, &regexp);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, RegExpConstr_value, RegExpW, &RegExpConstr_info,
            PROPF_CONSTR|2, &regexp->dispex, ret);

    jsdisp_release(&regexp->dispex);
    return hres;
}

HRESULT parse_regexp_flags(const WCHAR *str, DWORD str_len, DWORD *ret)
{
    const WCHAR *p;
    DWORD flags = 0;

    for (p = str; p < str+str_len; p++) {
        switch (*p) {
        case 'g':
            flags |= JSREG_GLOB;
            break;
        case 'i':
            flags |= JSREG_FOLD;
            break;
        case 'm':
            flags |= JSREG_MULTILINE;
            break;
        case 'y':
            flags |= JSREG_STICKY;
            break;
        default:
            WARN("wrong flag %c\n", *p);
            return E_FAIL;
        }
    }

    *ret = flags;
    return S_OK;
}
