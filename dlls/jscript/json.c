/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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
#include "parser.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR parseW[] = {'p','a','r','s','e',0};
static const WCHAR stringifyW[] = {'s','t','r','i','n','g','i','f','y',0};

static const WCHAR nullW[] = {'n','u','l','l',0};
static const WCHAR trueW[] = {'t','r','u','e',0};
static const WCHAR falseW[] = {'f','a','l','s','e',0};

typedef struct {
    const WCHAR *ptr;
    const WCHAR *end;
    script_ctx_t *ctx;
} json_parse_ctx_t;

static BOOL is_json_space(WCHAR c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static WCHAR skip_spaces(json_parse_ctx_t *ctx)
{
    while(is_json_space(*ctx->ptr))
        ctx->ptr++;
    return *ctx->ptr;
}

static BOOL is_keyword(json_parse_ctx_t *ctx, const WCHAR *keyword)
{
    unsigned i;
    for(i=0; keyword[i]; i++) {
        if(!ctx->ptr[i] || keyword[i] != ctx->ptr[i])
            return FALSE;
    }
    if(is_identifier_char(ctx->ptr[i]))
        return FALSE;
    ctx->ptr += i;
    return TRUE;
}

/* ECMA-262 5.1 Edition    15.12.1.1 */
static HRESULT parse_json_string(json_parse_ctx_t *ctx, WCHAR **r)
{
    const WCHAR *ptr = ++ctx->ptr;
    size_t len;
    WCHAR *buf;

    while(*ctx->ptr && *ctx->ptr != '"') {
        if(*ctx->ptr++ == '\\')
            ctx->ptr++;
    }
    if(!*ctx->ptr) {
        FIXME("unterminated string\n");
        return E_FAIL;
    }

    len = ctx->ptr-ptr;
    buf = heap_alloc((len+1)*sizeof(WCHAR));
    if(!buf)
        return E_OUTOFMEMORY;
    if(len)
        memcpy(buf, ptr, len*sizeof(WCHAR));
    buf[len] = 0;

    if(!unescape(buf)) {
        FIXME("unescape failed\n");
        heap_free(buf);
        return E_FAIL;
    }

    ctx->ptr++;
    *r = buf;
    return S_OK;
}

/* ECMA-262 5.1 Edition    15.12.1.2 */
static HRESULT parse_json_value(json_parse_ctx_t *ctx, jsval_t *r)
{
    HRESULT hres;

    switch(skip_spaces(ctx)) {

    /* JSONNullLiteral */
    case 'n':
        if(!is_keyword(ctx, nullW))
            break;
        *r = jsval_null();
        return S_OK;

    /* JSONBooleanLiteral */
    case 't':
        if(!is_keyword(ctx, trueW))
            break;
        *r = jsval_bool(TRUE);
        return S_OK;
    case 'f':
        if(!is_keyword(ctx, falseW))
            break;
        *r = jsval_bool(FALSE);
        return S_OK;

    /* JSONObject */
    case '{': {
        WCHAR *prop_name;
        jsdisp_t *obj;
        jsval_t val;

        hres = create_object(ctx->ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        ctx->ptr++;
        if(skip_spaces(ctx) == '}') {
            ctx->ptr++;
            *r = jsval_obj(obj);
            return S_OK;
        }

        while(1) {
            if(*ctx->ptr != '"')
                break;
            hres = parse_json_string(ctx, &prop_name);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) != ':') {
                FIXME("missing ':'\n");
                heap_free(prop_name);
                break;
            }

            ctx->ptr++;
            hres = parse_json_value(ctx, &val);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_name(obj, prop_name, val);
                jsval_release(val);
            }
            heap_free(prop_name);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) == '}') {
                ctx->ptr++;
                *r = jsval_obj(obj);
                return S_OK;
            }

            if(*ctx->ptr++ != ',') {
                FIXME("expected ','\n");
                break;
            }
            skip_spaces(ctx);
        }

        jsdisp_release(obj);
        break;
    }

    /* JSONString */
    case '"': {
        WCHAR *string;
        jsstr_t *str;

        hres = parse_json_string(ctx, &string);
        if(FAILED(hres))
            return hres;

        /* FIXME: avoid reallocation */
        str = jsstr_alloc(string);
        heap_free(string);
        if(!str)
            return E_OUTOFMEMORY;

        *r = jsval_string(str);
        return S_OK;
    }

    /* JSONArray */
    case '[': {
        jsdisp_t *array;
        unsigned i = 0;
        jsval_t val;

        hres = create_array(ctx->ctx, 0, &array);
        if(FAILED(hres))
            return hres;

        ctx->ptr++;
        if(skip_spaces(ctx) == ']') {
            ctx->ptr++;
            *r = jsval_obj(array);
            return S_OK;
        }

        while(1) {
            hres = parse_json_value(ctx, &val);
            if(FAILED(hres))
                break;

            hres = jsdisp_propput_idx(array, i, val);
            jsval_release(val);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) == ']') {
                ctx->ptr++;
                *r = jsval_obj(array);
                return S_OK;
            }

            if(*ctx->ptr != ',') {
                FIXME("expected ','\n");
                break;
            }

            ctx->ptr++;
            i++;
        }

        jsdisp_release(array);
        break;
    }

    /* JSONNumber */
    default: {
        int sign = 1;
        double n;

        if(*ctx->ptr == '-') {
            sign = -1;
            ctx->ptr++;
            skip_spaces(ctx);
        }

        if(!isdigitW(*ctx->ptr))
            break;

        if(*ctx->ptr == '0') {
            ctx->ptr++;
            n = 0;
            if(is_identifier_char(*ctx->ptr))
                break;
        }else {
            hres = parse_decimal(&ctx->ptr, ctx->end, &n);
            if(FAILED(hres))
                return hres;
        }

        *r = jsval_number(sign*n);
        return S_OK;
    }
    }

    FIXME("Syntax error at %s\n", debugstr_w(ctx->ptr));
    return E_FAIL;
}

/* ECMA-262 5.1 Edition    15.12.2 */
static HRESULT JSON_parse(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    json_parse_ctx_t parse_ctx;
    const WCHAR *buf;
    jsstr_t *str;
    jsval_t ret;
    HRESULT hres;

    if(argc != 1) {
        FIXME("Unsupported args\n");
        return E_INVALIDARG;
    }

    hres = to_flat_string(ctx, argv[0], &str, &buf);
    if(FAILED(hres))
        return hres;

    TRACE("%s\n", debugstr_w(buf));

    parse_ctx.ptr = buf;
    parse_ctx.end = buf + jsstr_length(str);
    parse_ctx.ctx = ctx;
    hres = parse_json_value(&parse_ctx, &ret);
    jsstr_release(str);
    if(FAILED(hres))
        return hres;

    if(skip_spaces(&parse_ctx)) {
        FIXME("syntax error\n");
        jsval_release(ret);
        return E_FAIL;
    }

    if(r)
        *r = ret;
    else
        jsval_release(ret);
    return S_OK;
}

static HRESULT JSON_stringify(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t JSON_props[] = {
    {parseW,     JSON_parse,     PROPF_METHOD|2},
    {stringifyW, JSON_stringify, PROPF_METHOD|3}
};

static const builtin_info_t JSON_info = {
    JSCLASS_JSON,
    {NULL, NULL, 0},
    sizeof(JSON_props)/sizeof(*JSON_props),
    JSON_props,
    NULL,
    NULL
};

HRESULT create_json(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *json;
    HRESULT hres;

    json = heap_alloc_zero(sizeof(*json));
    if(!json)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(json, ctx, &JSON_info, ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(json);
        return hres;
    }

    *ret = json;
    return S_OK;
}
