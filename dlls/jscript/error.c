/*
 * Copyright 2009 Piotr Caban
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

#include "config.h"
#include "wine/port.h"

#include <math.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR descriptionW[] = {'d','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR messageW[] = {'m','e','s','s','a','g','e',0};
static const WCHAR nameW[] = {'n','a','m','e',0};
static const WCHAR numberW[] = {'n','u','m','b','e','r',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};

/* ECMA-262 3rd Edition    15.11.4.4 */
static HRESULT Error_toString(script_ctx_t *ctx, vdisp_t *vthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsthis;
    BSTR name = NULL, msg = NULL, ret = NULL;
    VARIANT v;
    HRESULT hres;

    static const WCHAR object_errorW[] = {'[','o','b','j','e','c','t',' ','E','r','r','o','r',']',0};

    TRACE("\n");

    jsthis = get_jsdisp(vthis);
    if(!jsthis || ctx->version < 2) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(object_errorW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    hres = jsdisp_propget_name(jsthis, nameW, &v, ei);
    if(FAILED(hres))
        return hres;

    if(V_VT(&v) != VT_EMPTY) {
        hres = to_string(ctx, &v, ei, &name);
        VariantClear(&v);
        if(FAILED(hres))
            return hres;
        if(!*name) {
            SysFreeString(name);
            name = NULL;
        }
    }

    hres = jsdisp_propget_name(jsthis, messageW, &v, ei);
    if(SUCCEEDED(hres)) {
        if(V_VT(&v) != VT_EMPTY) {
            hres = to_string(ctx, &v, ei, &msg);
            VariantClear(&v);
            if(SUCCEEDED(hres) && !*msg) {
                SysFreeString(msg);
                msg = NULL;
            }
        }
    }

    if(SUCCEEDED(hres)) {
        if(name && msg) {
            DWORD name_len, msg_len;

            name_len = SysStringLen(name);
            msg_len = SysStringLen(msg);

            ret = SysAllocStringLen(NULL, name_len + msg_len + 2);
            if(ret) {
                memcpy(ret, name, name_len*sizeof(WCHAR));
                ret[name_len] = ':';
                ret[name_len+1] = ' ';
                memcpy(ret+name_len+2, msg, msg_len*sizeof(WCHAR));
            }
        }else if(name) {
            ret = name;
            name = NULL;
        }else if(msg) {
            ret = msg;
            msg = NULL;
        }else {
            ret = SysAllocString(object_errorW);
        }
    }

    SysFreeString(msg);
    SysFreeString(name);
    if(FAILED(hres))
        return hres;
    if(!ret)
        return E_OUTOFMEMORY;

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }else {
        SysFreeString(ret);
    }

    return S_OK;
}

static HRESULT Error_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t Error_props[] = {
    {toStringW,                 Error_toString,                     PROPF_METHOD}
};

static const builtin_info_t Error_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    sizeof(Error_props)/sizeof(*Error_props),
    Error_props,
    NULL,
    NULL
};

static const builtin_info_t ErrorInst_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    0,
    NULL,
    NULL,
    NULL
};

static HRESULT alloc_error(script_ctx_t *ctx, jsdisp_t *prototype,
        jsdisp_t *constr, jsdisp_t **ret)
{
    jsdisp_t *err;
    HRESULT hres;

    err = heap_alloc_zero(sizeof(*err));
    if(!err)
        return E_OUTOFMEMORY;

    if(prototype)
        hres = init_dispex(err, ctx, &Error_info, prototype);
    else
        hres = init_dispex_from_constr(err, ctx, &ErrorInst_info,
            constr ? constr : ctx->error_constr);
    if(FAILED(hres)) {
        heap_free(err);
        return hres;
    }

    *ret = err;
    return S_OK;
}

static HRESULT create_error(script_ctx_t *ctx, jsdisp_t *constr,
        UINT number, const WCHAR *msg, jsdisp_t **ret)
{
    jsdisp_t *err;
    VARIANT v;
    HRESULT hres;

    hres = alloc_error(ctx, NULL, constr, &err);
    if(FAILED(hres))
        return hres;

    num_set_int(&v, number);
    hres = jsdisp_propput_name(err, numberW, &v, NULL/*FIXME*/);
    if(FAILED(hres)) {
        jsdisp_release(err);
        return hres;
    }

    V_VT(&v) = VT_BSTR;
    if(msg) V_BSTR(&v) = SysAllocString(msg);
    else V_BSTR(&v) = SysAllocStringLen(NULL, 0);
    if(V_BSTR(&v)) {
        hres = jsdisp_propput_name(err, messageW, &v, NULL/*FIXME*/);
        if(SUCCEEDED(hres))
            hres = jsdisp_propput_name(err, descriptionW, &v, NULL/*FIXME*/);
        SysFreeString(V_BSTR(&v));
    }else {
        hres = E_OUTOFMEMORY;
    }
    if(FAILED(hres)) {
        jsdisp_release(err);
        return hres;
    }

    *ret = err;
    return S_OK;
}

static HRESULT error_constr(script_ctx_t *ctx, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei, jsdisp_t *constr) {
    jsdisp_t *err;
    UINT num = 0;
    BSTR msg = NULL;
    HRESULT hres;

    if(argc) {
        double n;

        hres = to_number(ctx, argv, ei, &n);
        if(FAILED(hres)) /* FIXME: really? */
            n = NAN;
        if(isnan(n))
            hres = to_string(ctx, argv, ei, &msg);
        if(FAILED(hres))
            return hres;
        num = n;
    }

    if(argc>1 && !msg) {
        hres = to_string(ctx, argv+1, ei, &msg);
        if(FAILED(hres))
            return hres;
    }

    switch(flags) {
    case INVOKE_FUNC:
    case DISPATCH_CONSTRUCT:
        hres = create_error(ctx, constr, num, msg, &err);
        SysFreeString(msg);

        if(FAILED(hres))
            return hres;

        if(retv)
            var_set_jsdisp(retv, err);
        else
            jsdisp_release(err);

        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT ErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->error_constr);
}

static HRESULT EvalErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->eval_error_constr);
}

static HRESULT RangeErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->range_error_constr);
}

static HRESULT ReferenceErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->reference_error_constr);
}

static HRESULT RegExpErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->regexp_error_constr);
}

static HRESULT SyntaxErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->syntax_error_constr);
}

static HRESULT TypeErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->type_error_constr);
}

static HRESULT URIErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, retv, ei, ctx->uri_error_constr);
}

HRESULT init_error_constr(script_ctx_t *ctx, jsdisp_t *object_prototype)
{
    static const WCHAR ErrorW[] = {'E','r','r','o','r',0};
    static const WCHAR EvalErrorW[] = {'E','v','a','l','E','r','r','o','r',0};
    static const WCHAR RangeErrorW[] = {'R','a','n','g','e','E','r','r','o','r',0};
    static const WCHAR ReferenceErrorW[] = {'R','e','f','e','r','e','n','c','e','E','r','r','o','r',0};
    static const WCHAR RegExpErrorW[] = {'R','e','g','E','x','p','E','r','r','o','r',0};
    static const WCHAR SyntaxErrorW[] = {'S','y','n','t','a','x','E','r','r','o','r',0};
    static const WCHAR TypeErrorW[] = {'T','y','p','e','E','r','r','o','r',0};
    static const WCHAR URIErrorW[] = {'U','R','I','E','r','r','o','r',0};
    static const WCHAR *names[] = {ErrorW, EvalErrorW, RangeErrorW,
        ReferenceErrorW, RegExpErrorW, SyntaxErrorW, TypeErrorW, URIErrorW};
    jsdisp_t **constr_addr[] = {&ctx->error_constr, &ctx->eval_error_constr,
        &ctx->range_error_constr, &ctx->reference_error_constr, &ctx->regexp_error_constr,
        &ctx->syntax_error_constr, &ctx->type_error_constr,
        &ctx->uri_error_constr};
    static builtin_invoke_t constr_val[] = {ErrorConstr_value, EvalErrorConstr_value,
        RangeErrorConstr_value, ReferenceErrorConstr_value, RegExpErrorConstr_value,
        SyntaxErrorConstr_value, TypeErrorConstr_value, URIErrorConstr_value};

    jsdisp_t *err;
    INT i;
    VARIANT v;
    HRESULT hres;

    for(i=0; i < sizeof(names)/sizeof(names[0]); i++) {
        hres = alloc_error(ctx, i==0 ? object_prototype : NULL, NULL, &err);
        if(FAILED(hres))
            return hres;

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(names[i]);
        if(!V_BSTR(&v)) {
            jsdisp_release(err);
            return E_OUTOFMEMORY;
        }

        hres = jsdisp_propput_name(err, nameW, &v, NULL/*FIXME*/);

        if(SUCCEEDED(hres))
            hres = create_builtin_constructor(ctx, constr_val[i], names[i], NULL,
                    PROPF_CONSTR|1, err, constr_addr[i]);

        jsdisp_release(err);
        VariantClear(&v);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT throw_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str, jsdisp_t *constr)
{
    WCHAR buf[1024], *pos = NULL;
    jsdisp_t *err;
    HRESULT hres;

    if(!is_jscript_error(error))
        return error;

    buf[0] = '\0';
    LoadStringW(jscript_hinstance, HRESULT_CODE(error),  buf, sizeof(buf)/sizeof(WCHAR));

    if(str) pos = strchrW(buf, '|');
    if(pos) {
        int len = strlenW(str);
        memmove(pos+len, pos+1, (strlenW(pos+1)+1)*sizeof(WCHAR));
        memcpy(pos, str, len*sizeof(WCHAR));
    }

    WARN("%s\n", debugstr_w(buf));

    hres = create_error(ctx, constr, error, buf, &err);
    if(FAILED(hres))
        return hres;

    if(ei)
        var_set_jsdisp(&ei->var, err);
    return error;
}

HRESULT throw_generic_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->error_constr);
}

HRESULT throw_range_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->range_error_constr);
}

HRESULT throw_reference_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->reference_error_constr);
}

HRESULT throw_regexp_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->regexp_error_constr);
}

HRESULT throw_syntax_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->syntax_error_constr);
}

HRESULT throw_type_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->type_error_constr);
}

HRESULT throw_uri_error(script_ctx_t *ctx, jsexcept_t *ei, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, ei, error, str, ctx->uri_error_constr);
}
