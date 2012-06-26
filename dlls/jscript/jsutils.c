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

#include <math.h>
#include <assert.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);
WINE_DECLARE_DEBUG_CHANNEL(heap);

const char *debugstr_variant(const VARIANT *v)
{
    if(!v)
        return "(null)";

    switch(V_VT(v)) {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_NULL:
        return "{VT_NULL}";
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_UI4:
        return wine_dbg_sprintf("{VT_UI4: %u}", V_UI4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}

#define MIN_BLOCK_SIZE  128
#define ARENA_FREE_FILLER  0xaa

static inline DWORD block_size(DWORD block)
{
    return MIN_BLOCK_SIZE << block;
}

void jsheap_init(jsheap_t *heap)
{
    memset(heap, 0, sizeof(*heap));
    list_init(&heap->custom_blocks);
}

void *jsheap_alloc(jsheap_t *heap, DWORD size)
{
    struct list *list;
    void *tmp;

    if(!heap->block_cnt) {
        if(!heap->blocks) {
            heap->blocks = heap_alloc(sizeof(void*));
            if(!heap->blocks)
                return NULL;
        }

        tmp = heap_alloc(block_size(0));
        if(!tmp)
            return NULL;

        heap->blocks[0] = tmp;
        heap->block_cnt = 1;
    }

    if(heap->offset + size <= block_size(heap->last_block)) {
        tmp = ((BYTE*)heap->blocks[heap->last_block])+heap->offset;
        heap->offset += size;
        return tmp;
    }

    if(size <= block_size(heap->last_block+1)) {
        if(heap->last_block+1 == heap->block_cnt) {
            tmp = heap_realloc(heap->blocks, (heap->block_cnt+1)*sizeof(void*));
            if(!tmp)
                return NULL;

            heap->blocks = tmp;
            heap->blocks[heap->block_cnt] = heap_alloc(block_size(heap->block_cnt));
            if(!heap->blocks[heap->block_cnt])
                return NULL;

            heap->block_cnt++;
        }

        heap->last_block++;
        heap->offset = size;
        return heap->blocks[heap->last_block];
    }

    list = heap_alloc(size + sizeof(struct list));
    if(!list)
        return NULL;

    list_add_head(&heap->custom_blocks, list);
    return list+1;
}

void *jsheap_grow(jsheap_t *heap, void *mem, DWORD size, DWORD inc)
{
    void *ret;

    if(mem == (BYTE*)heap->blocks[heap->last_block] + heap->offset-size
       && heap->offset+inc < block_size(heap->last_block)) {
        heap->offset += inc;
        return mem;
    }

    ret = jsheap_alloc(heap, size+inc);
    if(ret) /* FIXME: avoid copying for custom blocks */
        memcpy(ret, mem, size);
    return ret;
}

void jsheap_clear(jsheap_t *heap)
{
    struct list *tmp;

    if(!heap)
        return;

    while((tmp = list_next(&heap->custom_blocks, &heap->custom_blocks))) {
        list_remove(tmp);
        heap_free(tmp);
    }

    if(WARN_ON(heap)) {
        DWORD i;

        for(i=0; i < heap->block_cnt; i++)
            memset(heap->blocks[i], ARENA_FREE_FILLER, block_size(i));
    }

    heap->last_block = heap->offset = 0;
    heap->mark = FALSE;
}

void jsheap_free(jsheap_t *heap)
{
    DWORD i;

    jsheap_clear(heap);

    for(i=0; i < heap->block_cnt; i++)
        heap_free(heap->blocks[i]);
    heap_free(heap->blocks);

    jsheap_init(heap);
}

jsheap_t *jsheap_mark(jsheap_t *heap)
{
    if(heap->mark)
        return NULL;

    heap->mark = TRUE;
    return heap;
}

/* ECMA-262 3rd Edition    9.1 */
HRESULT to_primitive(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, VARIANT *ret, hint_t hint)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
    case VT_BOOL:
    case VT_I4:
    case VT_R8:
        *ret = *v;
        break;
    case VT_BSTR:
        V_VT(ret) = VT_BSTR;
        V_BSTR(ret) = SysAllocString(V_BSTR(v));
        break;
    case VT_DISPATCH: {
        jsdisp_t *jsdisp;
        DISPID id;
        HRESULT hres;

        static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
        static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

        if(!V_DISPATCH(v)) {
            V_VT(ret) = VT_NULL;
            break;
        }

        jsdisp = iface_to_jsdisp((IUnknown*)V_DISPATCH(v));
        if(!jsdisp) {
            V_VT(ret) = VT_EMPTY;
            return disp_propget(ctx, V_DISPATCH(v), DISPID_VALUE, ret, ei);
        }

        if(hint == NO_HINT)
            hint = is_class(jsdisp, JSCLASS_DATE) ? HINT_STRING : HINT_NUMBER;

        /* Native implementation doesn't throw TypeErrors, returns strange values */

        hres = jsdisp_get_id(jsdisp, hint == HINT_STRING ? toStringW : valueOfW, 0, &id);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_call(jsdisp, id, DISPATCH_METHOD, 0, NULL, ret, ei);
            if(FAILED(hres)) {
                WARN("call error - forwarding exception\n");
                jsdisp_release(jsdisp);
                return hres;
            }
            else if(V_VT(ret) != VT_DISPATCH) {
                jsdisp_release(jsdisp);
                return S_OK;
            }
            else
                IDispatch_Release(V_DISPATCH(ret));
        }

        hres = jsdisp_get_id(jsdisp, hint == HINT_STRING ? valueOfW : toStringW, 0, &id);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_call(jsdisp, id, DISPATCH_METHOD, 0, NULL, ret, ei);
            if(FAILED(hres)) {
                WARN("call error - forwarding exception\n");
                jsdisp_release(jsdisp);
                return hres;
            }
            else if(V_VT(ret) != VT_DISPATCH) {
                jsdisp_release(jsdisp);
                return S_OK;
            }
            else
                IDispatch_Release(V_DISPATCH(ret));
        }

        jsdisp_release(jsdisp);

        WARN("failed\n");
        return throw_type_error(ctx, ei, JS_E_TO_PRIMITIVE, NULL);
    }
    case VT_I2:
    case VT_INT:
        assert(0);
    default:
        FIXME("Unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    9.2 */
HRESULT to_boolean(VARIANT *v, VARIANT_BOOL *b)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
        *b = VARIANT_FALSE;
        break;
    case VT_I4:
        *b = V_I4(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_R8:
        if(isnan(V_R8(v))) *b = VARIANT_FALSE;
        else *b = V_R8(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_BSTR:
        *b = V_BSTR(v) && *V_BSTR(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_DISPATCH:
        *b = V_DISPATCH(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_BOOL:
        *b = V_BOOL(v);
        break;
    default:
        FIXME("unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

static int hex_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c-'0';

    if('a' <= c && c <= 'f')
        return c-'a'+10;

    if('A' <= c && c <= 'F')
        return c-'A'+10;

    return -1;
}

/* ECMA-262 3rd Edition    9.3.1 */
static HRESULT str_to_number(BSTR str, double *ret)
{
    const WCHAR *ptr = str;
    BOOL neg = FALSE;
    DOUBLE d = 0.0;

    static const WCHAR infinityW[] = {'I','n','f','i','n','i','t','y'};

    if(!ptr) {
        *ret = 0;
        return S_OK;
    }

    while(isspaceW(*ptr))
        ptr++;

    if(*ptr == '-') {
        neg = TRUE;
        ptr++;
    }else if(*ptr == '+') {
        ptr++;
    }

    if(!strncmpW(ptr, infinityW, sizeof(infinityW)/sizeof(WCHAR))) {
        ptr += sizeof(infinityW)/sizeof(WCHAR);
        while(*ptr && isspaceW(*ptr))
            ptr++;

        if(*ptr)
            *ret = NAN;
        else
            *ret = neg ? -INFINITY : INFINITY;
        return S_OK;
    }

    if(*ptr == '0' && ptr[1] == 'x') {
        DWORD l = 0;

        ptr += 2;
        while((l = hex_to_int(*ptr)) != -1) {
            d = d*16 + l;
            ptr++;
        }

        *ret = d;
        return S_OK;
    }

    while(isdigitW(*ptr))
        d = d*10 + (*ptr++ - '0');

    if(*ptr == 'e' || *ptr == 'E') {
        BOOL eneg = FALSE;
        LONG l = 0;

        ptr++;
        if(*ptr == '-') {
            ptr++;
            eneg = TRUE;
        }else if(*ptr == '+') {
            ptr++;
        }

        while(isdigitW(*ptr))
            l = l*10 + (*ptr++ - '0');
        if(eneg)
            l = -l;

        d *= pow(10, l);
    }else if(*ptr == '.') {
        DOUBLE dec = 0.1;

        ptr++;
        while(isdigitW(*ptr)) {
            d += dec * (*ptr++ - '0');
            dec *= 0.1;
        }
    }

    while(isspaceW(*ptr))
        ptr++;

    if(*ptr) {
        *ret = NAN;
        return S_OK;
    }

    if(neg)
        d = -d;

    *ret = d;
    return S_OK;
}

/* ECMA-262 3rd Edition    9.3 */
HRESULT to_number(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, double *ret)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        *ret = NAN;
        break;
    case VT_NULL:
        *ret = 0;
        break;
    case VT_I4:
        *ret = V_I4(v);
        break;
    case VT_R8:
        *ret = V_R8(v);
        break;
    case VT_BSTR:
        return str_to_number(V_BSTR(v), ret);
    case VT_DISPATCH: {
        VARIANT prim;
        HRESULT hres;

        hres = to_primitive(ctx, v, ei, &prim, HINT_NUMBER);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, &prim, ei, ret);
        VariantClear(&prim);
        return hres;
    }
    case VT_BOOL:
        *ret = V_BOOL(v) ? 1 : 0;
        break;
    case VT_I2:
    case VT_INT:
        assert(0);
    default:
        FIXME("unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    9.4 */
HRESULT to_integer(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, double *ret)
{
    double n;
    HRESULT hres;

    if(V_VT(v) == VT_I4) {
        *ret = V_I4(v);
        return S_OK;
    }

    hres = to_number(ctx, v, ei, &n);
    if(FAILED(hres))
        return hres;

    if(isnan(n))
        *ret = 0;
    else
        *ret = n >= 0.0 ? floor(n) : -floor(-n);
    return S_OK;
}

/* ECMA-262 3rd Edition    9.5 */
HRESULT to_int32(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, INT *ret)
{
    double n;
    HRESULT hres;

    if(V_VT(v) == VT_I4) {
        *ret = V_I4(v);
        return S_OK;
    }

    hres = to_number(ctx, v, ei, &n);
    if(FAILED(hres))
        return hres;

    *ret = isnan(n) || isinf(n) ? 0 : n;
    return S_OK;
}

/* ECMA-262 3rd Edition    9.6 */
HRESULT to_uint32(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, DWORD *ret)
{
    double n;
    HRESULT hres;

    if(V_VT(v) == VT_I4) {
        *ret = V_I4(v);
        return S_OK;
    }

    hres = to_number(ctx, v, ei, &n);
    if(FAILED(hres))
        return hres;

    *ret = isnan(n) || isinf(n) ? 0 : n;
    return S_OK;
}

static BSTR int_to_bstr(int i)
{
    WCHAR buf[12], *p;
    BOOL neg = FALSE;

    if(!i) {
        static const WCHAR zeroW[] = {'0',0};
        return SysAllocString(zeroW);
    }

    if(i < 0) {
        neg = TRUE;
        i = -i;
    }

    p = buf + sizeof(buf)/sizeof(*buf)-1;
    *p-- = 0;
    while(i) {
        *p-- = i%10 + '0';
        i /= 10;
    }

    if(neg)
        *p = '-';
    else
        p++;

    return SysAllocString(p);
}

HRESULT double_to_bstr(double n, BSTR *str)
{
    const WCHAR NaNW[] = {'N','a','N',0};
    const WCHAR InfinityW[] = {'-','I','n','f','i','n','i','t','y',0};

    if(isnan(n)) {
       *str = SysAllocString(NaNW);
    }else if(isinf(n)) {
        *str = SysAllocString(n<0 ? InfinityW : InfinityW+1);
    }else {
        VARIANT strv, v;
        HRESULT hres;

        V_VT(&v) = VT_R8;
        V_R8(&v) = n;
        V_VT(&strv) = VT_EMPTY;
        hres = VariantChangeTypeEx(&strv, &v, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, VT_BSTR);
        if(FAILED(hres))
            return hres;

        *str = V_BSTR(&strv);
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    9.8 */
HRESULT to_string(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, BSTR *str)
{
    const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
    const WCHAR nullW[] = {'n','u','l','l',0};
    const WCHAR trueW[] = {'t','r','u','e',0};
    const WCHAR falseW[] = {'f','a','l','s','e',0};

    switch(V_VT(v)) {
    case VT_EMPTY:
        *str = SysAllocString(undefinedW);
        break;
    case VT_NULL:
        *str = SysAllocString(nullW);
        break;
    case VT_I4:
        *str = int_to_bstr(V_I4(v));
        break;
    case VT_R8:
        return double_to_bstr(V_R8(v), str);
    case VT_BSTR:
        *str = SysAllocString(V_BSTR(v));
        break;
    case VT_DISPATCH: {
        VARIANT prim;
        HRESULT hres;

        hres = to_primitive(ctx, v, ei, &prim, HINT_STRING);
        if(FAILED(hres))
            return hres;

        hres = to_string(ctx, &prim, ei, str);
        VariantClear(&prim);
        return hres;
    }
    case VT_BOOL:
        *str = SysAllocString(V_BOOL(v) ? trueW : falseW);
        break;
    default:
        FIXME("unsupported vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    9.9 */
HRESULT to_object(script_ctx_t *ctx, VARIANT *v, IDispatch **disp)
{
    jsdisp_t *dispex;
    HRESULT hres;

    switch(V_VT(v)) {
    case VT_BSTR:
        hres = create_string(ctx, V_BSTR(v), SysStringLen(V_BSTR(v)), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case VT_I4:
    case VT_R8:
        hres = create_number(ctx, num_val(v), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case VT_DISPATCH:
        if(V_DISPATCH(v)) {
            IDispatch_AddRef(V_DISPATCH(v));
            *disp = V_DISPATCH(v);
        }else {
            jsdisp_t *obj;

            hres = create_object(ctx, NULL, &obj);
            if(FAILED(hres))
                return hres;

            *disp = to_disp(obj);
        }
        break;
    case VT_BOOL:
        hres = create_bool(ctx, V_BOOL(v), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case VT_ARRAY|VT_VARIANT:
        hres = create_vbarray(ctx, V_ARRAY(v), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    default:
        FIXME("unsupported vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT variant_change_type(script_ctx_t *ctx, VARIANT *dst, VARIANT *src, VARTYPE vt)
{
    jsexcept_t ei;
    HRESULT hres;

    memset(&ei, 0, sizeof(ei));

    switch(vt) {
    case VT_I2:
    case VT_I4: {
        INT i;

        hres = to_int32(ctx, src, &ei, &i);
        if(SUCCEEDED(hres)) {
            if(vt == VT_I4)
                V_I4(dst) = i;
            else
                V_I2(dst) = i;
        }
        break;
    }
    case VT_R8: {
        double n;
        hres = to_number(ctx, src, &ei, &n);
        if(SUCCEEDED(hres))
            V_R8(dst) = n;
        break;
    }
    case VT_R4: {
        double n;

        hres = to_number(ctx, src, &ei, &n);
        if(SUCCEEDED(hres))
            V_R4(dst) = n;
        break;
    }
    case VT_BOOL: {
        VARIANT_BOOL b;

        hres = to_boolean(src, &b);
        if(SUCCEEDED(hres))
            V_BOOL(dst) = b;
        break;
    }
    case VT_BSTR: {
        BSTR str;

        hres = to_string(ctx, src, &ei, &str);
        if(SUCCEEDED(hres))
            V_BSTR(dst) = str;
        break;
    }
    case VT_EMPTY:
        hres = V_VT(src) == VT_EMPTY ? S_OK : E_NOTIMPL;
        break;
    case VT_NULL:
        hres = V_VT(src) == VT_NULL ? S_OK : E_NOTIMPL;
        break;
    default:
        FIXME("vt %d not implemented\n", vt);
        hres = E_NOTIMPL;
    }

    if(FAILED(hres)) {
        VariantClear(&ei.var);
        return hres;
    }

    V_VT(dst) = vt;
    return S_OK;
}

static inline JSCaller *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, JSCaller, IServiceProvider_iface);
}

static HRESULT WINAPI JSCaller_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    JSCaller *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI JSCaller_AddRef(IServiceProvider *iface)
{
    JSCaller *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI JSCaller_Release(IServiceProvider *iface)
{
    JSCaller *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->ctx);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI JSCaller_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    JSCaller *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(guidService, &SID_VariantConversion) && This->ctx && This->ctx->active_script) {
        TRACE("(%p)->(SID_VariantConversion)\n", This);
        return IActiveScript_QueryInterface(This->ctx->active_script, riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    JSCaller_QueryInterface,
    JSCaller_AddRef,
    JSCaller_Release,
    JSCaller_QueryService
};

HRESULT create_jscaller(script_ctx_t *ctx)
{
    JSCaller *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    ret->ref = 1;
    ret->ctx = ctx;

    ctx->jscaller = ret;
    return S_OK;
}
