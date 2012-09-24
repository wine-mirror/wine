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

#include "config.h"
#include "wine/port.h"

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#define UINT32_MAX 0xffffffff

typedef struct {
    jsdisp_t dispex;

    WCHAR *str;
    DWORD length;
} StringInstance;

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR anchorW[] = {'a','n','c','h','o','r',0};
static const WCHAR bigW[] = {'b','i','g',0};
static const WCHAR blinkW[] = {'b','l','i','n','k',0};
static const WCHAR boldW[] = {'b','o','l','d',0};
static const WCHAR charAtW[] = {'c','h','a','r','A','t',0};
static const WCHAR charCodeAtW[] = {'c','h','a','r','C','o','d','e','A','t',0};
static const WCHAR concatW[] = {'c','o','n','c','a','t',0};
static const WCHAR fixedW[] = {'f','i','x','e','d',0};
static const WCHAR fontcolorW[] = {'f','o','n','t','c','o','l','o','r',0};
static const WCHAR fontsizeW[] = {'f','o','n','t','s','i','z','e',0};
static const WCHAR indexOfW[] = {'i','n','d','e','x','O','f',0};
static const WCHAR italicsW[] = {'i','t','a','l','i','c','s',0};
static const WCHAR lastIndexOfW[] = {'l','a','s','t','I','n','d','e','x','O','f',0};
static const WCHAR linkW[] = {'l','i','n','k',0};
static const WCHAR matchW[] = {'m','a','t','c','h',0};
static const WCHAR replaceW[] = {'r','e','p','l','a','c','e',0};
static const WCHAR searchW[] = {'s','e','a','r','c','h',0};
static const WCHAR sliceW[] = {'s','l','i','c','e',0};
static const WCHAR smallW[] = {'s','m','a','l','l',0};
static const WCHAR splitW[] = {'s','p','l','i','t',0};
static const WCHAR strikeW[] = {'s','t','r','i','k','e',0};
static const WCHAR subW[] = {'s','u','b',0};
static const WCHAR substringW[] = {'s','u','b','s','t','r','i','n','g',0};
static const WCHAR substrW[] = {'s','u','b','s','t','r',0};
static const WCHAR supW[] = {'s','u','p',0};
static const WCHAR toLowerCaseW[] = {'t','o','L','o','w','e','r','C','a','s','e',0};
static const WCHAR toUpperCaseW[] = {'t','o','U','p','p','e','r','C','a','s','e',0};
static const WCHAR toLocaleLowerCaseW[] = {'t','o','L','o','c','a','l','e','L','o','w','e','r','C','a','s','e',0};
static const WCHAR toLocaleUpperCaseW[] = {'t','o','L','o','c','a','l','e','U','p','p','e','r','C','a','s','e',0};
static const WCHAR localeCompareW[] = {'l','o','c','a','l','e','C','o','m','p','a','r','e',0};
static const WCHAR fromCharCodeW[] = {'f','r','o','m','C','h','a','r','C','o','d','e',0};

static inline StringInstance *string_from_vdisp(vdisp_t *vdisp)
{
    return (StringInstance*)vdisp->u.jsdisp;
}

static inline StringInstance *string_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_STRING) ? string_from_vdisp(jsthis) : NULL;
}

static HRESULT get_string_val(script_ctx_t *ctx, vdisp_t *jsthis, const WCHAR **str, DWORD *len, BSTR *val_str)
{
    StringInstance *string;
    HRESULT hres;

    if((string = string_this(jsthis))) {
        *str = string->str;
        *len = string->length;
        *val_str = NULL;
        return S_OK;
    }

    hres = to_string(ctx, jsval_disp(jsthis->u.disp), val_str);
    if(FAILED(hres))
        return hres;

    *str = *val_str;
    *len = SysStringLen(*val_str);
    return S_OK;
}

static HRESULT String_length(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("%p\n", jsthis);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        StringInstance *string = string_from_vdisp(jsthis);

        *r = jsval_number(string->length);
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT stringobj_to_string(vdisp_t *jsthis, jsval_t *r)
{
    StringInstance *string;

    if(!(string = string_this(jsthis))) {
        WARN("this is not a string object\n");
        return E_FAIL;
    }

    if(r) {
        BSTR str = SysAllocString(string->str);
        if(!str)
            return E_OUTOFMEMORY;

        *r = jsval_string(str);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, r);
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, r);
}

static HRESULT do_attributeless_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, jsval_t *r, const WCHAR *tagname)
{
    const WCHAR *str;
    DWORD length;
    BSTR val_str = NULL;
    HRESULT hres;

    static const WCHAR tagfmt[] = {'<','%','s','>','%','s','<','/','%','s','>',0};

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(r) {
        BSTR ret = SysAllocStringLen(NULL, length + 2*strlenW(tagname) + 5);
        if(!ret) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        sprintfW(ret, tagfmt, tagname, str, tagname);
        *r = jsval_string(ret);
    }

    SysFreeString(val_str);
    return S_OK;
}

static HRESULT do_attribute_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, unsigned argc, jsval_t *argv, jsval_t *r,
        const WCHAR *tagname, const WCHAR *attr)
{
    static const WCHAR tagfmtW[]
        = {'<','%','s',' ','%','s','=','\"','%','s','\"','>','%','s','<','/','%','s','>',0};
    static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};

    StringInstance *string;
    const WCHAR *str;
    DWORD length;
    BSTR attr_value, val_str = NULL;
    HRESULT hres;

    if(!(string = string_this(jsthis))) {
        hres = to_string(ctx, jsval_disp(jsthis->u.disp), &val_str);
        if(FAILED(hres))
            return hres;

        str = val_str;
        length = SysStringLen(val_str);
    }
    else {
        str = string->str;
        length = string->length;
    }

    if(argc) {
        hres = to_string(ctx, argv[0], &attr_value);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }
    else {
        attr_value = SysAllocString(undefinedW);
        if(!attr_value) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }
    }

    if(r) {
        BSTR ret = SysAllocStringLen(NULL, length + 2*strlenW(tagname)
                + strlenW(attr) + SysStringLen(attr_value) + 9);
        if(!ret) {
            SysFreeString(attr_value);
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        sprintfW(ret, tagfmtW, tagname, attr, attr_value, str, tagname);
        *r = jsval_string(ret);
    }

    SysFreeString(attr_value);
    SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_anchor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'N','A','M','E',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_big(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR bigtagW[] = {'B','I','G',0};
    return do_attributeless_tag_format(ctx, jsthis, r, bigtagW);
}

static HRESULT String_blink(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR blinktagW[] = {'B','L','I','N','K',0};
    return do_attributeless_tag_format(ctx, jsthis, r, blinktagW);
}

static HRESULT String_bold(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR boldtagW[] = {'B',0};
    return do_attributeless_tag_format(ctx, jsthis, r, boldtagW);
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str;
    DWORD length;
    BSTR ret, val_str;
    INT pos = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc) {
        double d;

        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
        pos = is_int32(d) ? d : -1;
    }

    if(!r) {
        SysFreeString(val_str);
        return S_OK;
    }

    if(0 <= pos && pos < length)
        ret = SysAllocStringLen(str+pos, 1);
    else
        ret = SysAllocStringLen(NULL, 0);
    SysFreeString(val_str);
    if(!ret) {
        return E_OUTOFMEMORY;
    }

    *r = jsval_string(ret);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charCodeAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str;
    BSTR val_str;
    DWORD length, idx = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc > 0) {
        double d;

        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(!is_int32(d) || d < 0 || d >= length) {
            SysFreeString(val_str);
            if(r)
                *r = jsval_number(NAN);
            return S_OK;
        }

        idx = d;
    }

    if(r)
        *r = jsval_number(str[idx]);

    SysFreeString(val_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.6 */
static HRESULT String_concat(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BSTR *strs = NULL, ret = NULL;
    DWORD len = 0, i, l, str_cnt;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("\n");

    str_cnt = argc+1;
    strs = heap_alloc_zero(str_cnt * sizeof(BSTR));
    if(!strs)
        return E_OUTOFMEMORY;

    hres = to_string(ctx, jsval_disp(jsthis->u.disp), strs);
    if(SUCCEEDED(hres)) {
        for(i=0; i < argc; i++) {
            hres = to_string(ctx, argv[i], strs+i+1);
            if(FAILED(hres))
                break;
        }
    }

    if(SUCCEEDED(hres)) {
        for(i=0; i < str_cnt; i++)
            len += SysStringLen(strs[i]);

        ptr = ret = SysAllocStringLen(NULL, len);

        for(i=0; i < str_cnt; i++) {
            l = SysStringLen(strs[i]);
            memcpy(ptr, strs[i], l*sizeof(WCHAR));
            ptr += l;
        }
    }

    for(i=0; i < str_cnt; i++)
        SysFreeString(strs[i]);
    heap_free(strs);

    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_string(ret);
    else
        SysFreeString(ret);
    return S_OK;
}

static HRESULT String_fixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fixedtagW[] = {'T','T',0};
    return do_attributeless_tag_format(ctx, jsthis, r, fixedtagW);
}

static HRESULT String_fontcolor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'C','O','L','O','R',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_fontsize(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'S','I','Z','E',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_indexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DWORD length, pos = 0;
    const WCHAR *str;
    BSTR search_str, val_str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_number(-1);
        SysFreeString(val_str);
        return S_OK;
    }

    hres = to_string(ctx, argv[0], &search_str);
    if(FAILED(hres)) {
        SysFreeString(val_str);
        return hres;
    }

    if(argc >= 2) {
        double d;

        hres = to_integer(ctx, argv[1], &d);
        if(SUCCEEDED(hres) && d > 0.0)
            pos = is_int32(d) ? min(length, d) : length;
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *ptr;

        ptr = strstrW(str+pos, search_str);
        if(ptr)
            ret = ptr - str;
        else
            ret = -1;
    }

    SysFreeString(search_str);
    SysFreeString(val_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT String_italics(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR italicstagW[] = {'I',0};
    return do_attributeless_tag_format(ctx, jsthis, r, italicstagW);
}

/* ECMA-262 3rd Edition    15.5.4.8 */
static HRESULT String_lastIndexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BSTR search_str, val_str;
    DWORD length, pos = 0, search_len;
    const WCHAR *str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_number(-1);
        SysFreeString(val_str);
        return S_OK;
    }

    hres = to_string(ctx, argv[0], &search_str);
    if(FAILED(hres)) {
        SysFreeString(val_str);
        return hres;
    }

    search_len = SysStringLen(search_str);

    if(argc >= 2) {
        double d;

        hres = to_integer(ctx, argv[1], &d);
        if(SUCCEEDED(hres) && d > 0)
            pos = is_int32(d) ? min(length, d) : length;
    }else {
        pos = length;
    }

    if(SUCCEEDED(hres) && length >= search_len) {
        const WCHAR *ptr;

        for(ptr = str+min(pos, length-search_len); ptr >= str; ptr--) {
            if(!memcmp(ptr, search_str, search_len*sizeof(WCHAR))) {
                ret = ptr-str;
                break;
            }
        }
    }

    SysFreeString(search_str);
    SysFreeString(val_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT String_link(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'H','R','E','F',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

/* ECMA-262 3rd Edition    15.5.4.10 */
static HRESULT String_match(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *regexp = NULL;
    const WCHAR *str;
    DWORD length;
    BSTR val_str = NULL;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_null();
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        BSTR match_str;

        hres = to_string(ctx, argv[0], &match_str);
        if(FAILED(hres))
            return hres;

        hres = create_regexp(ctx, match_str, SysStringLen(match_str), 0, &regexp);
        SysFreeString(match_str);
        if(FAILED(hres))
            return hres;
    }

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(SUCCEEDED(hres)) {
        if(!val_str)
            val_str = SysAllocStringLen(str, length);
        if(val_str)
            hres = regexp_string_match(ctx, regexp, val_str, r);
        else
            hres = E_OUTOFMEMORY;
    }

    jsdisp_release(regexp);
    SysFreeString(val_str);
    return hres;
}

typedef struct {
    WCHAR *buf;
    DWORD size;
    DWORD len;
} strbuf_t;

static HRESULT strbuf_append(strbuf_t *buf, const WCHAR *str, DWORD len)
{
    if(!len)
        return S_OK;

    if(len + buf->len > buf->size) {
        WCHAR *new_buf;
        DWORD new_size;

        new_size = buf->size ? buf->size<<1 : 16;
        if(new_size < buf->len+len)
            new_size = buf->len+len;
        if(buf->buf)
            new_buf = heap_realloc(buf->buf, new_size*sizeof(WCHAR));
        else
            new_buf = heap_alloc(new_size*sizeof(WCHAR));
        if(!new_buf)
            return E_OUTOFMEMORY;

        buf->buf = new_buf;
        buf->size = new_size;
    }

    memcpy(buf->buf+buf->len, str, len*sizeof(WCHAR));
    buf->len += len;
    return S_OK;
}

static HRESULT rep_call(script_ctx_t *ctx, jsdisp_t *func, const WCHAR *str, match_result_t *match,
        match_result_t *parens, DWORD parens_cnt, BSTR *ret)
{
    jsval_t *argv;
    unsigned argc;
    jsval_t val;
    BSTR tmp_str;
    DWORD i;
    HRESULT hres = S_OK;

    argc = parens_cnt+3;
    argv = heap_alloc_zero(sizeof(*argv)*argc);
    if(!argv)
        return E_OUTOFMEMORY;

    tmp_str = SysAllocStringLen(match->str, match->len);
    if(!tmp_str)
        hres = E_OUTOFMEMORY;
    argv[0] = jsval_string(tmp_str);

    if(SUCCEEDED(hres)) {
        for(i=0; i < parens_cnt; i++) {
            tmp_str = SysAllocStringLen(parens[i].str, parens[i].len);
            if(!tmp_str) {
               hres = E_OUTOFMEMORY;
               break;
            }
            argv[i+1] = jsval_string(tmp_str);
        }
    }

    if(SUCCEEDED(hres)) {
        argv[parens_cnt+1] = jsval_number(match->str - str);

        tmp_str = SysAllocString(str);
        if(!tmp_str)
            hres = E_OUTOFMEMORY;
        argv[parens_cnt+2] = jsval_string(tmp_str);
    }

    if(SUCCEEDED(hres))
        hres = jsdisp_call_value(func, NULL, DISPATCH_METHOD, argc, argv, &val);

    for(i=0; i < parens_cnt+3; i++) {
        if(i != parens_cnt+1)
            SysFreeString(get_string(argv[i]));
    }
    heap_free(argv);

    if(FAILED(hres))
        return hres;

    hres = to_string(ctx, val, ret);
    jsval_release(val);
    return hres;
}

/* ECMA-262 3rd Edition    15.5.4.11 */
static HRESULT String_replace(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str;
    DWORD parens_cnt = 0, parens_size=0, rep_len=0, length;
    BSTR rep_str = NULL, match_str = NULL, ret_str, val_str;
    jsdisp_t *rep_func = NULL, *regexp = NULL;
    match_result_t *parens = NULL, match = {NULL,0}, **parens_ptr = &parens;
    strbuf_t ret = {NULL,0,0};
    DWORD re_flags = REM_NO_CTX_UPDATE;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r) {
            if(!val_str) {
                val_str = SysAllocStringLen(str, length);
                if(!val_str)
                    return E_OUTOFMEMORY;
            }

            *r = jsval_string(val_str);
        }
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        hres = to_string(ctx, argv[0], &match_str);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }

    if(argc >= 2) {
        if(is_object_instance(argv[1])) {
            rep_func = iface_to_jsdisp((IUnknown*)get_object(argv[1]));
            if(rep_func && !is_class(rep_func, JSCLASS_FUNCTION)) {
                jsdisp_release(rep_func);
                rep_func = NULL;
            }
        }

        if(!rep_func) {
            hres = to_string(ctx, argv[1], &rep_str);
            if(SUCCEEDED(hres)) {
                rep_len = SysStringLen(rep_str);
                if(!strchrW(rep_str, '$'))
                    parens_ptr = NULL;
            }
        }
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *cp, *ecp;

        cp = ecp = str;

        while(1) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, re_flags, str, length, &cp, parens_ptr,
                        &parens_size, &parens_cnt, &match);
                re_flags |= REM_CHECK_GLOBAL;

                if(hres == S_FALSE) {
                    hres = S_OK;
                    break;
                }
                if(FAILED(hres))
                    break;

                if(!match.len)
                    cp++;
            }else {
                match.str = strstrW(cp, match_str);
                if(!match.str)
                    break;
                match.len = SysStringLen(match_str);
                cp = match.str+match.len;
            }

            hres = strbuf_append(&ret, ecp, match.str-ecp);
            ecp = match.str+match.len;
            if(FAILED(hres))
                break;

            if(rep_func) {
                BSTR cstr;

                hres = rep_call(ctx, rep_func, str, &match, parens, parens_cnt, &cstr);
                if(FAILED(hres))
                    break;

                hres = strbuf_append(&ret, cstr, SysStringLen(cstr));
                SysFreeString(cstr);
                if(FAILED(hres))
                    break;
            }else if(rep_str && regexp) {
                const WCHAR *ptr = rep_str, *ptr2;

                while((ptr2 = strchrW(ptr, '$'))) {
                    hres = strbuf_append(&ret, ptr, ptr2-ptr);
                    if(FAILED(hres))
                        break;

                    switch(ptr2[1]) {
                    case '$':
                        hres = strbuf_append(&ret, ptr2, 1);
                        ptr = ptr2+2;
                        break;
                    case '&':
                        hres = strbuf_append(&ret, match.str, match.len);
                        ptr = ptr2+2;
                        break;
                    case '`':
                        hres = strbuf_append(&ret, str, match.str-str);
                        ptr = ptr2+2;
                        break;
                    case '\'':
                        hres = strbuf_append(&ret, ecp, (str+length)-ecp);
                        ptr = ptr2+2;
                        break;
                    default: {
                        DWORD idx;

                        if(!isdigitW(ptr2[1])) {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        idx = ptr2[1] - '0';
                        if(isdigitW(ptr2[2]) && idx*10 + (ptr2[2]-'0') <= parens_cnt) {
                            idx = idx*10 + (ptr[2]-'0');
                            ptr = ptr2+3;
                        }else if(idx && idx <= parens_cnt) {
                            ptr = ptr2+2;
                        }else {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        hres = strbuf_append(&ret, parens[idx-1].str, parens[idx-1].len);
                    }
                    }

                    if(FAILED(hres))
                        break;
                }

                if(SUCCEEDED(hres))
                    hres = strbuf_append(&ret, ptr, (rep_str+rep_len)-ptr);
                if(FAILED(hres))
                    break;
            }else if(rep_str) {
                hres = strbuf_append(&ret, rep_str, rep_len);
                if(FAILED(hres))
                    break;
            }else {
                static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d'};

                hres = strbuf_append(&ret, undefinedW, sizeof(undefinedW)/sizeof(WCHAR));
                if(FAILED(hres))
                    break;
            }

            if(!regexp)
                break;
        }

        if(SUCCEEDED(hres))
            hres = strbuf_append(&ret, ecp, (str+length)-ecp);
    }

    if(rep_func)
        jsdisp_release(rep_func);
    SysFreeString(rep_str);
    SysFreeString(match_str);
    heap_free(parens);

    if(SUCCEEDED(hres) && match.str && regexp) {
        if(!val_str)
            val_str = SysAllocStringLen(str, length);
        if(val_str) {
            SysFreeString(ctx->last_match);
            ctx->last_match = val_str;
            val_str = NULL;
            ctx->last_match_index = match.str-str;
            ctx->last_match_length = match.len;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    if(regexp)
        jsdisp_release(regexp);
    SysFreeString(val_str);

    if(SUCCEEDED(hres) && r) {
        ret_str = SysAllocStringLen(ret.buf, ret.len);
        if(!ret_str)
            return E_OUTOFMEMORY;

        TRACE("= %s\n", debugstr_w(ret_str));
        *r = jsval_string(ret_str);
    }

    heap_free(ret.buf);
    return hres;
}

static HRESULT String_search(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *regexp = NULL;
    const WCHAR *str, *cp;
    match_result_t match;
    DWORD length;
    BSTR val_str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_null();
        SysFreeString(val_str);
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        hres = create_regexp_var(ctx, argv[0], NULL, &regexp);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }

    cp = str;
    hres = regexp_match_next(ctx, regexp, REM_RESET_INDEX, str, length, &cp, NULL, NULL, NULL, &match);
    SysFreeString(val_str);
    jsdisp_release(regexp);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(hres == S_OK ? match.str-str : -1);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.13 */
static HRESULT String_slice(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str;
    BSTR val_str;
    DWORD length;
    INT start=0, end;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(is_int32(d)) {
            start = d;
            if(start < 0) {
                start = length + start;
                if(start < 0)
                    start = 0;
            }else if(start > length) {
                start = length;
            }
        }else if(d > 0) {
            start = length;
        }
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(is_int32(d)) {
            end = d;
            if(end < 0) {
                end = length + end;
                if(end < 0)
                    end = 0;
            }else if(end > length) {
                end = length;
            }
        }else {
            end = d < 0.0 ? 0 : length;
        }
    }else {
        end = length;
    }

    if(end < start)
        end = start;

    if(r) {
        BSTR retstr = SysAllocStringLen(str+start, end-start);
        if(!retstr) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        *r = jsval_string(retstr);
    }

    SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_small(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR smalltagW[] = {'S','M','A','L','L',0};
    return do_attributeless_tag_format(ctx, jsthis, r, smalltagW);
}

static HRESULT String_split(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_result_t match_result;
    DWORD length, i, match_len = 0;
    const WCHAR *str, *ptr, *ptr2, *cp;
    unsigned limit = UINT32_MAX;
    jsdisp_t *array, *regexp = NULL;
    BSTR val_str, match_str = NULL, tmp_str;
    HRESULT hres;

    TRACE("\n");

    if(argc != 1 && argc != 2) {
        FIXME("unsupported argc %u\n", argc);
        return E_NOTIMPL;
    }

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc > 1 && !is_undefined(argv[1])) {
        hres = to_uint32(ctx, argv[1], &limit);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
        if(regexp) {
            if(!is_class(regexp, JSCLASS_REGEXP)) {
                jsdisp_release(regexp);
                regexp = NULL;
            }
        }
    }

    if(!regexp) {
        hres = to_string(ctx, argv[0], &match_str);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        match_len = SysStringLen(match_str);
        if(!match_len) {
            SysFreeString(match_str);
            match_str = NULL;
        }
    }

    hres = create_array(ctx, 0, &array);

    if(SUCCEEDED(hres)) {
        ptr = cp = str;
        for(i=0; i<limit; i++) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, 0, str, length, &cp, NULL, NULL, NULL, &match_result);
                if(hres != S_OK)
                    break;
                ptr2 = match_result.str;
            }else if(match_str) {
                ptr2 = strstrW(ptr, match_str);
                if(!ptr2)
                    break;
            }else {
                if(!*ptr)
                    break;
                ptr2 = ptr+1;
            }

            tmp_str = SysAllocStringLen(ptr, ptr2-ptr);
            if(!tmp_str) {
                hres = E_OUTOFMEMORY;
                break;
            }

            hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
            SysFreeString(tmp_str);
            if(FAILED(hres))
                break;

            if(regexp)
                ptr = cp;
            else if(match_str)
                ptr = ptr2 + match_len;
            else
                ptr++;
        }
    }

    if(SUCCEEDED(hres) && (match_str || regexp) && i<limit) {
        DWORD len = (str+length) - ptr;

        if(len || match_str) {
            tmp_str = SysAllocStringLen(ptr, len);

            if(tmp_str) {
                hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
                SysFreeString(tmp_str);
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    if(regexp)
        jsdisp_release(regexp);
    SysFreeString(match_str);
    SysFreeString(val_str);

    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);

    return hres;
}

static HRESULT String_strike(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR striketagW[] = {'S','T','R','I','K','E',0};
    return do_attributeless_tag_format(ctx, jsthis, r, striketagW);
}

static HRESULT String_sub(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR subtagW[] = {'S','U','B',0};
    return do_attributeless_tag_format(ctx, jsthis, r, subtagW);
}

/* ECMA-262 3rd Edition    15.5.4.15 */
static HRESULT String_substring(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str;
    BSTR val_str;
    INT start=0, end;
    DWORD length;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc >= 1) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(d >= 0)
            start = is_int32(d) ? min(length, d) : length;
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(d >= 0)
            end = is_int32(d) ? min(length, d) : length;
        else
            end = 0;
    }else {
        end = length;
    }

    if(start > end) {
        INT tmp = start;
        start = end;
        end = tmp;
    }

    if(r) {
        BSTR ret = SysAllocStringLen(str+start, end-start);
        if(!ret) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }
        *r = jsval_string(ret);
    }
    SysFreeString(val_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    B.2.3 */
static HRESULT String_substr(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BSTR val_str;
    const WCHAR *str;
    INT start=0, len;
    DWORD length;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(argc >= 1) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(d >= 0)
            start = is_int32(d) ? min(length, d) : length;
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(d >= 0.0)
            len = is_int32(d) ? min(length-start, d) : length-start;
        else
            len = 0;
    }else {
        len = length-start;
    }

    hres = S_OK;
    if(r) {
        BSTR ret = SysAllocStringLen(str+start, len);
        if(ret)
            *r = jsval_string(ret);
        else
            hres = E_OUTOFMEMORY;
    }

    SysFreeString(val_str);
    return hres;
}

static HRESULT String_sup(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR suptagW[] = {'S','U','P',0};
    return do_attributeless_tag_format(ctx, jsthis, r, suptagW);
}

static HRESULT String_toLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR* str;
    DWORD length;
    BSTR val_str;
    HRESULT  hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(r) {
        if(!val_str) {
            val_str = SysAllocStringLen(str, length);
            if(!val_str)
                return E_OUTOFMEMORY;
        }

        strlwrW(val_str);
        *r = jsval_string(val_str);
    }
    else SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_toUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR* str;
    DWORD length;
    BSTR val_str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(r) {
        if(!val_str) {
            val_str = SysAllocStringLen(str, length);
            if(!val_str)
                return E_OUTOFMEMORY;
        }

        struprW(val_str);
        *r = jsval_string(val_str);
    }
    else SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_toLocaleLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_toLocaleUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_localeCompare(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    StringInstance *This = string_from_vdisp(jsthis);

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, JS_E_FUNCTION_EXPECTED, NULL);
    case DISPATCH_PROPERTYGET: {
        BSTR str = SysAllocString(This->str);
        if(!str)
            return E_OUTOFMEMORY;

        *r = jsval_string(str);
        break;
    }
    default:
        FIXME("flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void String_destructor(jsdisp_t *dispex)
{
    StringInstance *This = (StringInstance*)dispex;

    heap_free(This->str);
    heap_free(This);
}

static const builtin_prop_t String_props[] = {
    {anchorW,                String_anchor,                PROPF_METHOD|1},
    {bigW,                   String_big,                   PROPF_METHOD},
    {blinkW,                 String_blink,                 PROPF_METHOD},
    {boldW,                  String_bold,                  PROPF_METHOD},
    {charAtW,                String_charAt,                PROPF_METHOD|1},
    {charCodeAtW,            String_charCodeAt,            PROPF_METHOD|1},
    {concatW,                String_concat,                PROPF_METHOD|1},
    {fixedW,                 String_fixed,                 PROPF_METHOD},
    {fontcolorW,             String_fontcolor,             PROPF_METHOD|1},
    {fontsizeW,              String_fontsize,              PROPF_METHOD|1},
    {indexOfW,               String_indexOf,               PROPF_METHOD|2},
    {italicsW,               String_italics,               PROPF_METHOD},
    {lastIndexOfW,           String_lastIndexOf,           PROPF_METHOD|2},
    {lengthW,                String_length,                0},
    {linkW,                  String_link,                  PROPF_METHOD|1},
    {localeCompareW,         String_localeCompare,         PROPF_METHOD|1},
    {matchW,                 String_match,                 PROPF_METHOD|1},
    {replaceW,               String_replace,               PROPF_METHOD|1},
    {searchW,                String_search,                PROPF_METHOD},
    {sliceW,                 String_slice,                 PROPF_METHOD},
    {smallW,                 String_small,                 PROPF_METHOD},
    {splitW,                 String_split,                 PROPF_METHOD|2},
    {strikeW,                String_strike,                PROPF_METHOD},
    {subW,                   String_sub,                   PROPF_METHOD},
    {substrW,                String_substr,                PROPF_METHOD|2},
    {substringW,             String_substring,             PROPF_METHOD|2},
    {supW,                   String_sup,                   PROPF_METHOD},
    {toLocaleLowerCaseW,     String_toLocaleLowerCase,     PROPF_METHOD},
    {toLocaleUpperCaseW,     String_toLocaleUpperCase,     PROPF_METHOD},
    {toLowerCaseW,           String_toLowerCase,           PROPF_METHOD},
    {toStringW,              String_toString,              PROPF_METHOD},
    {toUpperCaseW,           String_toUpperCase,           PROPF_METHOD},
    {valueOfW,               String_valueOf,               PROPF_METHOD}
};

static const builtin_info_t String_info = {
    JSCLASS_STRING,
    {NULL, String_value, 0},
    sizeof(String_props)/sizeof(*String_props),
    String_props,
    String_destructor,
    NULL
};

static const builtin_prop_t StringInst_props[] = {
    {lengthW,                String_length,                0}
};

static const builtin_info_t StringInst_info = {
    JSCLASS_STRING,
    {NULL, String_value, 0},
    sizeof(StringInst_props)/sizeof(*StringInst_props),
    StringInst_props,
    String_destructor,
    NULL
};

/* ECMA-262 3rd Edition    15.5.3.2 */
static HRESULT StringConstr_fromCharCode(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    DWORD i, code;
    BSTR ret;
    HRESULT hres;

    TRACE("\n");

    ret = SysAllocStringLen(NULL, argc);
    if(!ret)
        return E_OUTOFMEMORY;

    for(i=0; i<argc; i++) {
        hres = to_uint32(ctx, argv[i], &code);
        if(FAILED(hres)) {
            SysFreeString(ret);
            return hres;
        }

        ret[i] = code;
    }

    if(r)
        *r = jsval_string(ret);
    else
        SysFreeString(ret);
    return S_OK;
}

static HRESULT StringConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC: {
        BSTR str;

        if(argc) {
            hres = to_string(ctx, argv[0], &str);
            if(FAILED(hres))
                return hres;
        }else {
            str = SysAllocStringLen(NULL, 0);
            if(!str)
                return E_OUTOFMEMORY;
        }

        *r = jsval_string(str);
        break;
    }
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *ret;

        if(argc) {
            BSTR str;

            hres = to_string(ctx, argv[0], &str);
            if(FAILED(hres))
                return hres;

            hres = create_string(ctx, str, SysStringLen(str), &ret);
            SysFreeString(str);
        }else {
            hres = create_string(ctx, NULL, 0, &ret);
        }

        if(FAILED(hres))
            return hres;

        *r = jsval_obj(ret);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT string_alloc(script_ctx_t *ctx, jsdisp_t *object_prototype, StringInstance **ret)
{
    StringInstance *string;
    HRESULT hres;

    string = heap_alloc_zero(sizeof(StringInstance));
    if(!string)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&string->dispex, ctx, &String_info, object_prototype);
    else
        hres = init_dispex_from_constr(&string->dispex, ctx, &StringInst_info, ctx->string_constr);
    if(FAILED(hres)) {
        heap_free(string);
        return hres;
    }

    *ret = string;
    return S_OK;
}

static const builtin_prop_t StringConstr_props[] = {
    {fromCharCodeW,    StringConstr_fromCharCode,    PROPF_METHOD},
};

static const builtin_info_t StringConstr_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(StringConstr_props)/sizeof(*StringConstr_props),
    StringConstr_props,
    NULL,
    NULL
};

HRESULT create_string_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    StringInstance *string;
    HRESULT hres;

    static const WCHAR StringW[] = {'S','t','r','i','n','g',0};

    hres = string_alloc(ctx, object_prototype, &string);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, StringConstr_value, StringW, &StringConstr_info,
            PROPF_CONSTR|1, &string->dispex, ret);

    jsdisp_release(&string->dispex);
    return hres;
}

HRESULT create_string(script_ctx_t *ctx, const WCHAR *str, DWORD len, jsdisp_t **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, NULL, &string);
    if(FAILED(hres))
        return hres;

    if(len == -1)
        len = strlenW(str);

    string->length = len;
    string->str = heap_alloc((len+1)*sizeof(WCHAR));
    if(!string->str) {
        jsdisp_release(&string->dispex);
        return E_OUTOFMEMORY;
    }

    memcpy(string->str, str, len*sizeof(WCHAR));
    string->str[len] = 0;

    *ret = &string->dispex;
    return S_OK;

}
