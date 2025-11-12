/*
 * Copyright 2024 Gabriel Ivăncescu for CodeWeavers
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


#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    DWORD size;
    DECLSPEC_ALIGN(sizeof(double)) BYTE buf[];
} ArrayBufferInstance;

typedef struct {
    jsdisp_t dispex;

    ArrayBufferInstance *buffer;
    DWORD offset;
    DWORD size;
} DataViewInstance;

typedef struct {
    jsdisp_t dispex;

    ArrayBufferInstance *buffer;
    DWORD offset;
    DWORD length;
} TypedArrayInstance;

static inline ArrayBufferInstance *arraybuf_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, ArrayBufferInstance, dispex);
}

static inline DataViewInstance *dataview_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, DataViewInstance, dispex);
}

static inline TypedArrayInstance *typedarr_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, TypedArrayInstance, dispex);
}

static inline ArrayBufferInstance *arraybuf_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_ARRAYBUFFER)) ? arraybuf_from_jsdisp(jsdisp) : NULL;
}

static HRESULT create_arraybuf(script_ctx_t*,DWORD,ArrayBufferInstance**);

static HRESULT ArrayBuffer_get_byteLength(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(arraybuf_from_jsdisp(jsthis)->size);
    return S_OK;
}

static HRESULT ArrayBuffer_slice(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    ArrayBufferInstance *arraybuf, *ret;
    DWORD begin = 0, end, size;
    HRESULT hres;
    double n;

    TRACE("\n");

    if(!(arraybuf = arraybuf_this(vthis)))
        return JS_E_ARRAYBUFFER_EXPECTED;
    end = arraybuf->size;
    if(!r)
        return S_OK;

    if(argc) {
        hres = to_integer(ctx, argv[0], &n);
        if(FAILED(hres))
            return hres;
        if(n < 0.0)
            n += arraybuf->size;
        if(n >= 0.0 && n < arraybuf->size) {
            begin = n;
            if(argc > 1 && !is_undefined(argv[1])) {
                hres = to_integer(ctx, argv[1], &n);
                if(FAILED(hres))
                    return hres;
                if(n < 0.0)
                    n += arraybuf->size;
                if(n >= 0.0) {
                    end = n < arraybuf->size ? n : arraybuf->size;
                    end = end < begin ? begin : end;
                }else
                    end = begin;
            }
        }else
            end = 0;
    }

    size = end - begin;
    hres = create_arraybuf(ctx, size, &ret);
    if(FAILED(hres))
        return hres;
    memcpy(ret->buf, arraybuf->buf + begin, size);

    *r = jsval_obj(&ret->dispex);
    return S_OK;
}

static const builtin_prop_t ArrayBuffer_props[] = {
    {L"byteLength",            NULL, 0,                    ArrayBuffer_get_byteLength},
    {L"slice",                 ArrayBuffer_slice,          PROPF_METHOD|2},
};

static const builtin_info_t ArrayBuffer_info = {
    .class     = JSCLASS_ARRAYBUFFER,
    .props_cnt = ARRAY_SIZE(ArrayBuffer_props),
    .props     = ArrayBuffer_props,
};

static const builtin_prop_t ArrayBufferInst_props[] = {
    {L"byteLength",            NULL, 0,                    ArrayBuffer_get_byteLength},
};

static const builtin_info_t ArrayBufferInst_info = {
    .class     = JSCLASS_ARRAYBUFFER,
    .props_cnt = ARRAY_SIZE(ArrayBufferInst_props),
    .props     = ArrayBufferInst_props,
};

static HRESULT create_arraybuf(script_ctx_t *ctx, DWORD size, ArrayBufferInstance **ret)
{
    ArrayBufferInstance *arraybuf;
    HRESULT hres;

    if(!(arraybuf = calloc(1, FIELD_OFFSET(ArrayBufferInstance, buf[size]))))
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(&arraybuf->dispex, ctx, &ArrayBufferInst_info, ctx->arraybuf_constr);
    if(FAILED(hres)) {
        free(arraybuf);
        return hres;
    }
    arraybuf->size = size;

    *ret = arraybuf;
    return S_OK;
}

HRESULT create_arraybuffer(script_ctx_t *ctx, DWORD size, IWineJSDispatch **ret, void **data)
{
    ArrayBufferInstance *buf;
    HRESULT hres;

    hres = create_arraybuf(ctx, size, &buf);
    if(FAILED(hres))
        return hres;

    *ret = &buf->dispex.IWineJSDispatch_iface;
    *data = buf->buf;
    return S_OK;
}

static HRESULT ArrayBufferConstr_isView(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BOOL ret = FALSE;
    jsdisp_t *obj;

    TRACE("\n");

    if(!r)
        return S_OK;

    if(argc && is_object_instance(argv[0]) && (obj = to_jsdisp(get_object(argv[0]))) &&
       (obj->builtin_info->class == JSCLASS_DATAVIEW ||
        (obj->builtin_info->class >= FIRST_TYPEDARRAY_JSCLASS && obj->builtin_info->class <= LAST_TYPEDARRAY_JSCLASS)))
        ret = TRUE;

    *r = jsval_bool(ret);
    return S_OK;
}

static HRESULT ArrayBufferConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    ArrayBufferInstance *arraybuf;
    DWORD size = 0;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(argc) {
            double n;
            hres = to_integer(ctx, argv[0], &n);
            if(FAILED(hres))
                return hres;
            if(n < 0.0)
                return JS_E_INVALID_LENGTH;
            if(n > (UINT_MAX - FIELD_OFFSET(ArrayBufferInstance, buf[0])))
                return E_OUTOFMEMORY;
            size = n;
        }

        if(r) {
            hres = create_arraybuf(ctx, size, &arraybuf);
            if(FAILED(hres))
                return hres;
            *r = jsval_obj(&arraybuf->dispex);
        }
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t ArrayBufferConstr_props[] = {
    {L"isView",                ArrayBufferConstr_isView,   PROPF_METHOD|1},
};

static const builtin_info_t ArrayBufferConstr_info = {
    .class     = JSCLASS_FUNCTION,
    .call      = Function_value,
    .props_cnt = ARRAY_SIZE(ArrayBufferConstr_props),
    .props     = ArrayBufferConstr_props,
};

static inline DataViewInstance *dataview_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_DATAVIEW)) ? dataview_from_jsdisp(jsdisp) : NULL;
}

static HRESULT DataView_get_buffer(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DataViewInstance *view;

    TRACE("\n");

    if(!(view = dataview_this(vthis)))
        return JS_E_NOT_DATAVIEW;
    if(r) *r = jsval_obj(jsdisp_addref(&view->buffer->dispex));
    return S_OK;
}

static HRESULT DataView_get_byteLength(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DataViewInstance *view;

    TRACE("\n");

    if(!(view = dataview_this(vthis)))
        return JS_E_NOT_DATAVIEW;
    if(r) *r = jsval_number(view->size);
    return S_OK;
}

static HRESULT DataView_get_byteOffset(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DataViewInstance *view;

    TRACE("\n");

    if(!(view = dataview_this(vthis)))
        return JS_E_NOT_DATAVIEW;
    if(r) *r = jsval_number(view->offset);
    return S_OK;
}

static inline void copy_type_data(void *dst, const void *src, unsigned type_size, BOOL little_endian)
{
    const BYTE *in = src;
    BYTE *out = dst;
    unsigned i;

    if(little_endian)
        memcpy(out, in, type_size);
    else
        for(i = 0; i < type_size; i++)
            out[i] = in[type_size - i - 1];
}

static HRESULT get_data(script_ctx_t *ctx, jsval_t vthis, unsigned argc, jsval_t *argv, unsigned type_size, void *ret)
{
    BOOL little_endian = FALSE;
    DataViewInstance *view;
    HRESULT hres;
    DWORD offset;
    BYTE *data;
    double n;

    if(!(view = dataview_this(vthis)))
        return JS_E_NOT_DATAVIEW;
    if(!argc || is_undefined(argv[0]))
        return JS_E_DATAVIEW_NO_ARGUMENT;

    hres = to_integer(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    if(n < 0.0 || n + type_size > view->size)
        return JS_E_DATAVIEW_INVALID_ACCESS;

    offset = n;
    data = &view->buffer->buf[view->offset + offset];

    if(type_size == 1) {
        *(BYTE*)ret = data[0];
        return S_OK;
    }

    if(argc > 1) {
        hres = to_boolean(argv[1], &little_endian);
        if(FAILED(hres))
            return hres;
    }

    copy_type_data(ret, data, type_size, little_endian);
    return S_OK;
}

static HRESULT set_data(script_ctx_t *ctx, jsval_t vthis, unsigned argc, jsval_t *argv, unsigned type_size, const void *val)
{
    BOOL little_endian = FALSE;
    DataViewInstance *view;
    HRESULT hres;
    DWORD offset;
    BYTE *data;
    double n;

    if(!(view = dataview_this(vthis)))
        return JS_E_NOT_DATAVIEW;
    if(is_undefined(argv[0]) || is_undefined(argv[1]))
        return JS_E_DATAVIEW_NO_ARGUMENT;

    hres = to_integer(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    if(n < 0.0 || n + type_size > view->size)
        return JS_E_DATAVIEW_INVALID_ACCESS;

    offset = n;
    data = &view->buffer->buf[view->offset + offset];

    if(type_size == 1) {
        data[0] = *(const BYTE*)val;
        return S_OK;
    }

    if(argc > 2) {
        hres = to_boolean(argv[2], &little_endian);
        if(FAILED(hres))
            return hres;
    }

    copy_type_data(data, val, type_size, little_endian);
    return S_OK;
}

static HRESULT DataView_getFloat32(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    float v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getFloat64(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    double v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getInt8(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT8 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getInt16(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT16 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getInt32(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT32 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getUint8(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    UINT8 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getUint16(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    UINT16 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_getUint32(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    UINT32 v;

    TRACE("\n");

    hres = get_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_number(v);
    return S_OK;
}

static HRESULT DataView_setFloat32(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    double n;
    float v;

    TRACE("\n");

    if(argc < 2)
        return JS_E_DATAVIEW_NO_ARGUMENT;
    hres = to_number(ctx, argv[1], &n);
    if(FAILED(hres))
        return hres;
    v = n;  /* FIXME: don't assume rounding mode is round-to-nearest ties-to-even */

    hres = set_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT DataView_setFloat64(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    double v;

    TRACE("\n");

    if(argc < 2)
        return JS_E_DATAVIEW_NO_ARGUMENT;
    hres = to_number(ctx, argv[1], &v);
    if(FAILED(hres))
        return hres;

    hres = set_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT DataView_setInt8(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT32 n;
    INT8 v;

    TRACE("\n");

    if(argc < 2)
        return JS_E_DATAVIEW_NO_ARGUMENT;
    hres = to_int32(ctx, argv[1], &n);
    if(FAILED(hres))
        return hres;
    v = n;

    hres = set_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT DataView_setInt16(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT32 n;
    INT16 v;

    TRACE("\n");

    if(argc < 2)
        return JS_E_DATAVIEW_NO_ARGUMENT;
    hres = to_int32(ctx, argv[1], &n);
    if(FAILED(hres))
        return hres;
    v = n;

    hres = set_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT DataView_setInt32(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;
    INT32 v;

    TRACE("\n");

    if(argc < 2)
        return JS_E_DATAVIEW_NO_ARGUMENT;
    hres = to_int32(ctx, argv[1], &v);
    if(FAILED(hres))
        return hres;

    hres = set_data(ctx, vthis, argc, argv, sizeof(v), &v);
    if(FAILED(hres))
        return hres;
    if(r) *r = jsval_undefined();
    return S_OK;
}

static const builtin_prop_t DataView_props[] = {
    {L"getFloat32",            DataView_getFloat32,        PROPF_METHOD|1},
    {L"getFloat64",            DataView_getFloat64,        PROPF_METHOD|1},
    {L"getInt16",              DataView_getInt16,          PROPF_METHOD|1},
    {L"getInt32",              DataView_getInt32,          PROPF_METHOD|1},
    {L"getInt8",               DataView_getInt8,           PROPF_METHOD|1},
    {L"getUint16",             DataView_getUint16,         PROPF_METHOD|1},
    {L"getUint32",             DataView_getUint32,         PROPF_METHOD|1},
    {L"getUint8",              DataView_getUint8,          PROPF_METHOD|1},
    {L"setFloat32",            DataView_setFloat32,        PROPF_METHOD|1},
    {L"setFloat64",            DataView_setFloat64,        PROPF_METHOD|1},
    {L"setInt16",              DataView_setInt16,          PROPF_METHOD|1},
    {L"setInt32",              DataView_setInt32,          PROPF_METHOD|1},
    {L"setInt8",               DataView_setInt8,           PROPF_METHOD|1},
    {L"setUint16",             DataView_setInt16,          PROPF_METHOD|1},
    {L"setUint32",             DataView_setInt32,          PROPF_METHOD|1},
    {L"setUint8",              DataView_setInt8,           PROPF_METHOD|1},
};

static void DataView_destructor(jsdisp_t *dispex)
{
    DataViewInstance *view = dataview_from_jsdisp(dispex);
    if(view->buffer)
        jsdisp_release(&view->buffer->dispex);
}

static HRESULT DataView_gc_traverse(struct gc_ctx *gc_ctx, enum gc_traverse_op op, jsdisp_t *dispex)
{
    DataViewInstance *view = dataview_from_jsdisp(dispex);
    return gc_process_linked_obj(gc_ctx, op, dispex, &view->buffer->dispex, (void**)&view->buffer);
}

static const builtin_info_t DataView_info = {
    .class       = JSCLASS_DATAVIEW,
    .props_cnt   = ARRAY_SIZE(DataView_props),
    .props       = DataView_props,
    .destructor  = DataView_destructor,
    .gc_traverse = DataView_gc_traverse
};

static const builtin_info_t DataViewInst_info = {
    .class       = JSCLASS_DATAVIEW,
    .destructor  = DataView_destructor,
    .gc_traverse = DataView_gc_traverse
};

static HRESULT DataViewConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    ArrayBufferInstance *arraybuf;
    DataViewInstance *view;
    DWORD offset = 0, size;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(!argc || !(arraybuf = arraybuf_this(argv[0])))
            return JS_E_DATAVIEW_NO_ARGUMENT;
        size = arraybuf->size;

        if(argc > 1) {
            double offs, len, maxsize = size;
            hres = to_integer(ctx, argv[1], &offs);
            if(FAILED(hres))
                return hres;
            if(offs < 0.0 || offs > maxsize)
                return JS_E_DATAVIEW_INVALID_OFFSET;
            offset = offs;

            if(argc > 2 && !is_undefined(argv[2])) {
                hres = to_integer(ctx, argv[2], &len);
                if(FAILED(hres))
                    return hres;
                if(len < 0.0 || offs+len > maxsize)
                    return JS_E_DATAVIEW_INVALID_OFFSET;
                size = len;
            }else
                size -= offset;
        }

        if(!r)
            return S_OK;

        if(!(view = calloc(1, sizeof(DataViewInstance))))
            return E_OUTOFMEMORY;

        hres = init_dispex_from_constr(&view->dispex, ctx, &DataViewInst_info, ctx->dataview_constr);
        if(FAILED(hres)) {
            free(view);
            return hres;
        }

        jsdisp_addref(&arraybuf->dispex);
        view->buffer = arraybuf;
        view->offset = offset;
        view->size = size;

        *r = jsval_obj(&view->dispex);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_info_t DataViewConstr_info = {
    .class = JSCLASS_FUNCTION,
    .call  = Function_value,
};

/* NOTE: Keep in sync with the JSCLASS ordering of typed arrays */
#define ALL_TYPED_ARRAYS \
    X(Int8Array)         \
    X(Int16Array)        \
    X(Int32Array)        \
    X(Uint8Array)        \
    X(Uint8ClampedArray) \
    X(Uint16Array)       \
    X(Uint32Array)       \
    X(Float32Array)      \
    X(Float64Array)

enum {
#define X(name) name ##_desc_idx,
ALL_TYPED_ARRAYS
#undef X
};

struct typed_array_desc {
    unsigned size;
    double (*get)(const void*);
    void (*set)(void*,double);
};

static double get_s8(const void *p) { return *(const INT8 *)p; }
static double get_u8(const void *p) { return *(const UINT8 *)p; }
static void set_u8(void *p, double v) { *(UINT8 *)p = double_to_int32(v); }
static double get_s16(const void *p) { return *(const INT16 *)p; }
static double get_u16(const void *p) { return *(const UINT16 *)p; }
static void set_u16(void *p, double v) { *(UINT16 *)p = double_to_int32(v); }
static double get_s32(const void *p) { return *(const INT32 *)p; }
static double get_u32(const void *p) { return *(const UINT32 *)p; }
static void set_u32(void *p, double v) { *(UINT32 *)p = double_to_int32(v); }
static double get_f32(const void *p) { return *(const float *)p; }
static void set_f32(void *p, double v) { *(float *)p = v; }
static double get_f64(const void *p) { return *(const double *)p; }
static void set_f64(void *p, double v) { *(double *)p = v; }

static void set_clamped_u8(void *p, double v)
{
    *(UINT8 *)p = v >= 255.0 ? 255 : v > 0 ? lround(v) : 0;
}

static const struct typed_array_desc typed_array_descs[NUM_TYPEDARRAY_TYPES] = {
    [Int8Array_desc_idx]         = { 1, get_s8,  set_u8         },
    [Int16Array_desc_idx]        = { 2, get_s16, set_u16        },
    [Int32Array_desc_idx]        = { 4, get_s32, set_u32        },
    [Uint8Array_desc_idx]        = { 1, get_u8,  set_u8         },
    [Uint8ClampedArray_desc_idx] = { 1, get_u8,  set_clamped_u8 },
    [Uint16Array_desc_idx]       = { 2, get_u16, set_u16        },
    [Uint32Array_desc_idx]       = { 4, get_u32, set_u32        },
    [Float32Array_desc_idx]      = { 4, get_f32, set_f32        },
    [Float64Array_desc_idx]      = { 8, get_f64, set_f64        }
};

static inline TypedArrayInstance *typedarr_this(jsval_t vthis, jsclass_t class)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, class)) ? typedarr_from_jsdisp(jsdisp) : NULL;
}

static HRESULT create_typedarr(const builtin_info_t*,script_ctx_t*,ArrayBufferInstance*,DWORD,DWORD,jsdisp_t**);

static HRESULT TypedArray_get_buffer(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_obj(jsdisp_addref(&typedarr_from_jsdisp(jsthis)->buffer->dispex));
    return S_OK;
}

static HRESULT TypedArray_get_byteLength(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(typedarr_from_jsdisp(jsthis)->length * typed_array_descs[jsthis->builtin_info->class - FIRST_TYPEDARRAY_JSCLASS].size);
    return S_OK;
}

static HRESULT TypedArray_get_byteOffset(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(typedarr_from_jsdisp(jsthis)->offset);
    return S_OK;
}

static HRESULT TypedArray_get_length(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(typedarr_from_jsdisp(jsthis)->length);
    return S_OK;
}

static HRESULT fill_typedarr_data_from_object(const struct typed_array_desc *desc, script_ctx_t *ctx, BYTE *data, jsdisp_t *obj, DWORD length)
{
    HRESULT hres;
    jsval_t val;
    UINT32 i;

    for(i = 0; i < length; i++) {
        double n;

        hres = jsdisp_get_idx(obj, i, &val);
        if(FAILED(hres)) {
            if(hres != DISP_E_UNKNOWNNAME)
                return hres;
            val = jsval_undefined();
        }

        hres = to_number(ctx, val, &n);
        jsval_release(val);
        if(FAILED(hres))
            return hres;
        desc->set(&data[i * desc->size], n);
    }

    return S_OK;
}

static HRESULT TypedArray_set(const builtin_info_t *info, script_ctx_t *ctx, jsval_t vthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    const struct typed_array_desc *desc = &typed_array_descs[info->class - FIRST_TYPEDARRAY_JSCLASS];
    TypedArrayInstance *typedarr;
    DWORD begin = 0, size;
    BYTE *dest, *data;
    IDispatch *disp;
    jsdisp_t *obj;
    HRESULT hres;
    jsval_t val;
    UINT32 len;
    double n;

    if(!(typedarr = typedarr_this(vthis, info->class)))
        return JS_E_NOT_TYPEDARRAY;
    if(!argc)
        return JS_E_TYPEDARRAY_INVALID_SOURCE;

    hres = to_object(ctx, argv[0], &disp);
    if(FAILED(hres))
        return JS_E_TYPEDARRAY_INVALID_SOURCE;

    if(!(obj = to_jsdisp(disp))) {
        FIXME("Non-JS array object\n");
        hres = JS_E_TYPEDARRAY_INVALID_SOURCE;
        goto done;
    }

    hres = jsdisp_propget_name(obj, L"length", &val);
    if(FAILED(hres))
        goto done;

    hres = to_uint32(ctx, val, &len);
    jsval_release(val);
    if(FAILED(hres))
        goto done;

    if(argc > 1) {
        hres = to_integer(ctx, argv[1], &n);
        if(FAILED(hres))
            goto done;
        if(n < 0.0 || n > typedarr->length) {
            hres = JS_E_TYPEDARRAY_INVALID_OFFSLEN;
            goto done;
        }
        begin = n;
    }

    if(len > typedarr->length - begin) {
        hres = JS_E_TYPEDARRAY_INVALID_OFFSLEN;
        goto done;
    }
    size = len * desc->size;
    dest = data = &typedarr->buffer->buf[typedarr->offset + begin * desc->size];

    /* If they overlap, make a temporary copy */
    if(obj->builtin_info->class >= FIRST_TYPEDARRAY_JSCLASS && obj->builtin_info->class <= LAST_TYPEDARRAY_JSCLASS) {
        TypedArrayInstance *src_arr = typedarr_from_jsdisp(obj);
        const BYTE *src = src_arr->buffer->buf + src_arr->offset;

        if(dest < src + len * typed_array_descs[obj->builtin_info->class - FIRST_TYPEDARRAY_JSCLASS].size &&
           dest + size > src) {
            if(!(data = malloc(size))) {
                hres = E_OUTOFMEMORY;
                goto done;
            }
        }
    }

    hres = fill_typedarr_data_from_object(desc, ctx, data, obj, len);
    if(SUCCEEDED(hres) && dest != data) {
        memcpy(dest, data, size);
        free(data);
    }

done:
    IDispatch_Release(disp);
    return hres;
}

static HRESULT TypedArray_subarray(const builtin_info_t *info, script_ctx_t *ctx, jsval_t vthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TypedArrayInstance *typedarr;
    DWORD begin = 0, end;
    jsdisp_t *obj;
    HRESULT hres;
    double n;

    if(!(typedarr = typedarr_this(vthis, info->class)))
        return JS_E_NOT_TYPEDARRAY;
    if(!argc)
        return JS_E_TYPEDARRAY_INVALID_SUBARRAY;
    if(!r)
        return S_OK;

    hres = to_integer(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;
    end = typedarr->length;
    if(n < 0.0)
        n += typedarr->length;
    if(n >= 0.0)
        begin = n < typedarr->length ? n : typedarr->length;

    if(argc > 1 && !is_undefined(argv[1])) {
        hres = to_integer(ctx, argv[1], &n);
        if(FAILED(hres))
            return hres;
        if(n < 0.0)
            n += typedarr->length;
        if(n >= 0.0) {
            end = n < typedarr->length ? n : typedarr->length;
            end = end < begin ? begin : end;
        }else
            end = begin;
    }

    hres = create_typedarr(info, ctx, typedarr->buffer,
                           typedarr->offset + begin * typed_array_descs[info->class - FIRST_TYPEDARRAY_JSCLASS].size,
                           end - begin, &obj);
    if(FAILED(hres))
        return hres;

    *r = jsval_obj(obj);
    return S_OK;
}

static void TypedArray_destructor(jsdisp_t *dispex)
{
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);
    if(typedarr->buffer)
        jsdisp_release(&typedarr->buffer->dispex);
}

static HRESULT TypedArray_lookup_prop(jsdisp_t *dispex, const WCHAR *name, unsigned flags, struct property_info *desc)
{
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);

    /* Typed Arrays override every positive index */
    return jsdisp_index_lookup(&typedarr->dispex, name, INT_MAX, desc);
}

static inline HRESULT TypedArray_prop_get(const builtin_info_t *info, jsdisp_t *dispex, unsigned idx, jsval_t *r)
{
    const struct typed_array_desc *desc = &typed_array_descs[info->class - FIRST_TYPEDARRAY_JSCLASS];
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);

    if(idx >= typedarr->length)
        *r = jsval_undefined();
    else
        *r = jsval_number(desc->get(&typedarr->buffer->buf[typedarr->offset + idx * desc->size]));
    return S_OK;
}

static inline HRESULT TypedArray_prop_put(const builtin_info_t *info, jsdisp_t *dispex, unsigned idx, jsval_t val)
{
    const struct typed_array_desc *desc = &typed_array_descs[info->class - FIRST_TYPEDARRAY_JSCLASS];
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);
    HRESULT hres;
    double n;

    if(idx >= typedarr->length)
        return S_OK;

    hres = to_number(typedarr->dispex.ctx, val, &n);
    if(SUCCEEDED(hres))
        desc->set(&typedarr->buffer->buf[typedarr->offset + idx * desc->size], n);
    return hres;
}

static HRESULT TypedArray_fill_props(jsdisp_t *dispex)
{
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);

    return jsdisp_fill_indices(&typedarr->dispex, typedarr->length);
}

static HRESULT TypedArray_gc_traverse(struct gc_ctx *gc_ctx, enum gc_traverse_op op, jsdisp_t *dispex)
{
    TypedArrayInstance *typedarr = typedarr_from_jsdisp(dispex);
    return gc_process_linked_obj(gc_ctx, op, dispex, &typedarr->buffer->dispex, (void**)&typedarr->buffer);
}

static const builtin_prop_t TypedArrayInst_props[] = {
    {L"buffer",                NULL, 0,                    TypedArray_get_buffer},
    {L"byteLength",            NULL, 0,                    TypedArray_get_byteLength},
    {L"byteOffset",            NULL, 0,                    TypedArray_get_byteOffset},
    {L"length",                NULL, 0,                    TypedArray_get_length},
};

static HRESULT create_typedarr(const builtin_info_t *info, script_ctx_t *ctx, ArrayBufferInstance *buffer,
        DWORD offset, DWORD length, jsdisp_t **ret)
{
    TypedArrayInstance *typedarr;
    HRESULT hres;

    if(!(typedarr = calloc(1, sizeof(TypedArrayInstance))))
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(&typedarr->dispex, ctx, info, ctx->typedarr_constr[info->class - FIRST_TYPEDARRAY_JSCLASS]);
    if(FAILED(hres)) {
        free(typedarr);
        return hres;
    }

    jsdisp_addref(&buffer->dispex);
    typedarr->buffer = buffer;
    typedarr->offset = offset;
    typedarr->length = length;

    *ret = &typedarr->dispex;
    return S_OK;
}

static HRESULT TypedArrayConstr_value(const builtin_info_t *info, script_ctx_t *ctx, jsval_t vthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    const struct typed_array_desc *desc = &typed_array_descs[info->class - FIRST_TYPEDARRAY_JSCLASS];
    const unsigned elem_size = desc->size;
    ArrayBufferInstance *buffer = NULL;
    DWORD offset = 0, length = 0;
    jsdisp_t *typedarr;
    HRESULT hres;
    double n;

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(argc) {
            if(is_object_instance(argv[0])) {
                jsdisp_t *obj = to_jsdisp(get_object(argv[0]));

                if(!obj)
                    return JS_E_TYPEDARRAY_BAD_CTOR_ARG;

                if(obj->builtin_info->class == JSCLASS_ARRAYBUFFER) {
                    buffer = arraybuf_from_jsdisp(obj);
                    if(argc > 1) {
                        hres = to_integer(ctx, argv[1], &n);
                        if(FAILED(hres))
                            return hres;
                        if(n < 0.0 || n > buffer->size)
                            return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                        offset = n;
                        if(offset % elem_size)
                            return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                    }
                    if(argc > 2 && !is_undefined(argv[2])) {
                        hres = to_integer(ctx, argv[2], &n);
                        if(FAILED(hres))
                            return hres;
                        if(n < 0.0 || n > UINT_MAX)
                            return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                        length = n;
                        if(offset + length * elem_size > buffer->size)
                            return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                    }else {
                        length = buffer->size - offset;
                        if(length % elem_size)
                            return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                        length /= elem_size;
                    }
                    jsdisp_addref(&buffer->dispex);
                }else {
                    jsval_t val;
                    UINT32 len;
                    DWORD size;

                    hres = jsdisp_propget_name(obj, L"length", &val);
                    if(FAILED(hres))
                        return hres;
                    if(is_undefined(val))
                        return JS_E_TYPEDARRAY_BAD_CTOR_ARG;

                    hres = to_uint32(ctx, val, &len);
                    jsval_release(val);
                    if(FAILED(hres))
                        return hres;

                    length = len;
                    size = length * elem_size;
                    if(size < length || size > (UINT_MAX - FIELD_OFFSET(ArrayBufferInstance, buf[0])))
                        return E_OUTOFMEMORY;

                    hres = create_arraybuf(ctx, size, &buffer);
                    if(FAILED(hres))
                        return hres;

                    hres = fill_typedarr_data_from_object(desc, ctx, buffer->buf, obj, length);
                    if(FAILED(hres)) {
                        jsdisp_release(&buffer->dispex);
                        return hres;
                    }
                }
            }else if(is_number(argv[0])) {
                hres = to_integer(ctx, argv[0], &n);
                if(FAILED(hres))
                    return hres;
                if(n < 0.0)
                    return JS_E_TYPEDARRAY_INVALID_OFFSLEN;
                if(n * elem_size > (UINT_MAX - FIELD_OFFSET(ArrayBufferInstance, buf[0])))
                    return E_OUTOFMEMORY;
                length = n;
            }else
                return JS_E_TYPEDARRAY_BAD_CTOR_ARG;
        }

        if(!r)
            return S_OK;

        if(!buffer) {
            hres = create_arraybuf(ctx, length * elem_size, &buffer);
            if(FAILED(hres))
                return hres;
        }

        hres = create_typedarr(info, ctx, buffer, offset, length, &typedarr);
        jsdisp_release(&buffer->dispex);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(typedarr);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

#define X(name)                                                                         \
static const builtin_info_t name##_info;                                                \
                                                                                        \
static HRESULT name##_set(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r) \
{                                                                                       \
    TRACE("\n");                                                                        \
    return TypedArray_set(&name##_info, ctx, vthis, flags, argc, argv, r);              \
}                                                                                       \
                                                                                        \
static HRESULT name##_subarray(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r) \
{                                                                                       \
    TRACE("\n");                                                                        \
    return TypedArray_subarray(&name##_info, ctx, vthis, flags, argc, argv, r);         \
}                                                                                       \
                                                                                        \
static HRESULT name##_prop_get(jsdisp_t *dispex, unsigned idx, jsval_t *r)              \
{                                                                                       \
    TRACE("%p[%u]\n", dispex, idx);                                                     \
    return TypedArray_prop_get(&name##_info, dispex, idx, r);                           \
}                                                                                       \
                                                                                        \
static HRESULT name##_prop_put(jsdisp_t *dispex, unsigned idx, jsval_t val)             \
{                                                                                       \
    TRACE("%p[%u] = %s\n", dispex, idx, debugstr_jsval(val));                           \
    return TypedArray_prop_put(&name##_info, dispex, idx, val);                         \
}                                                                                       \
                                                                                        \
static const builtin_prop_t name##_props[] = {                                          \
    {L"buffer",                NULL, 0,                    TypedArray_get_buffer},      \
    {L"byteLength",            NULL, 0,                    TypedArray_get_byteLength},  \
    {L"byteOffset",            NULL, 0,                    TypedArray_get_byteOffset},  \
    {L"length",                NULL, 0,                    TypedArray_get_length},      \
    {L"set",                   name##_set,                 PROPF_METHOD|2},             \
    {L"subarray",              name##_subarray,            PROPF_METHOD|2},             \
};                                                                                      \
                                                                                        \
static const builtin_info_t name##_info =                                               \
{                                                                                       \
    .class          = FIRST_TYPEDARRAY_JSCLASS + name ##_desc_idx,                      \
    .props_cnt      = ARRAY_SIZE(name##_props),                                         \
    .props          = name##_props,                                                     \
    .destructor     = TypedArray_destructor,                                            \
    .lookup_prop    = TypedArray_lookup_prop,                                           \
    .prop_get       = name##_prop_get,                                                  \
    .prop_put       = name##_prop_put,                                                  \
    .fill_props     = TypedArray_fill_props,                                            \
    .gc_traverse    = TypedArray_gc_traverse,                                           \
};                                                                                      \
                                                                                        \
static const builtin_info_t name##Inst_info =                                           \
{                                                                                       \
    .class          = FIRST_TYPEDARRAY_JSCLASS + name##_desc_idx,                       \
    .props_cnt      = ARRAY_SIZE(TypedArrayInst_props),                                 \
    .props          = TypedArrayInst_props,                                             \
    .destructor     = TypedArray_destructor,                                            \
    .lookup_prop    = TypedArray_lookup_prop,                                           \
    .prop_get       = name##_prop_get,                                                  \
    .prop_put       = name##_prop_put,                                                  \
    .fill_props     = TypedArray_fill_props,                                            \
    .gc_traverse    = TypedArray_gc_traverse,                                           \
};                                                                                      \
static HRESULT name ## Constr_value(script_ctx_t *ctx, jsval_t jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r) \
{                                                                                       \
    TRACE("\n");                                                                        \
    return TypedArrayConstr_value(&name##Inst_info, ctx, jsthis, flags, argc, argv, r); \
}
ALL_TYPED_ARRAYS
#undef X

static const builtin_info_t TypedArrayConstr_info = {
    .class = JSCLASS_FUNCTION,
    .call  = Function_value,
};

static HRESULT init_typed_array_constructor(script_ctx_t *ctx, builtin_invoke_t func, const WCHAR *name, const builtin_info_t *info)
{
    const unsigned type_idx = info->class - FIRST_TYPEDARRAY_JSCLASS;
    TypedArrayInstance *prototype;
    HRESULT hres;

    if(!(prototype = calloc(1, sizeof(*prototype))))
        return E_OUTOFMEMORY;

    hres = create_arraybuf(ctx, 0, &prototype->buffer);
    if(FAILED(hres)) {
        free(prototype);
        return hres;
    }

    hres = init_dispex(&prototype->dispex, ctx, info, ctx->object_prototype);
    if(FAILED(hres)) {
        jsdisp_release(&prototype->buffer->dispex);
        free(prototype);
        return hres;
    }

    hres = create_builtin_constructor(ctx, func, name, &TypedArrayConstr_info, PROPF_CONSTR|1, &prototype->dispex, &ctx->typedarr_constr[type_idx]);
    jsdisp_release(&prototype->dispex);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->typedarr_constr[type_idx], L"BYTES_PER_ELEMENT", 0,
                                       jsval_number(typed_array_descs[type_idx].size));
    if(FAILED(hres))
        return hres;

    return jsdisp_define_data_property(ctx->global, name, PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->typedarr_constr[type_idx]));
}

HRESULT init_arraybuf_constructors(script_ctx_t *ctx)
{
    static const struct {
        const WCHAR *name;
        builtin_invoke_t constr_func;
        const builtin_info_t *prototype_info;
    } typed_arrays[] = {
        { L"Int8Array",     Int8ArrayConstr_value,     &Int8Array_info },
        { L"Int16Array",    Int16ArrayConstr_value,    &Int16Array_info },
        { L"Int32Array",    Int32ArrayConstr_value,    &Int32Array_info },
        { L"Uint8Array",    Uint8ArrayConstr_value,    &Uint8Array_info },
        { L"Uint16Array",   Uint16ArrayConstr_value,   &Uint16Array_info },
        { L"Uint32Array",   Uint32ArrayConstr_value,   &Uint32Array_info },
        { L"Float32Array",  Float32ArrayConstr_value,  &Float32Array_info },
        { L"Float64Array",  Float64ArrayConstr_value,  &Float64Array_info },
    };
    static const struct {
        const WCHAR *name;
        builtin_invoke_t get;
    } DataView_getters[] = {
        { L"buffer",        DataView_get_buffer },
        { L"byteLength",    DataView_get_byteLength },
        { L"byteOffset",    DataView_get_byteOffset },
    };
    ArrayBufferInstance *arraybuf;
    DataViewInstance *view;
    property_desc_t desc;
    HRESULT hres;
    unsigned i;

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES5_1)
        return S_OK;

    if(!(arraybuf = calloc(1, FIELD_OFFSET(ArrayBufferInstance, buf[0]))))
        return E_OUTOFMEMORY;

    hres = init_dispex(&arraybuf->dispex, ctx, &ArrayBuffer_info, ctx->object_prototype);
    if(FAILED(hres)) {
        free(arraybuf);
        return hres;
    }

    hres = create_builtin_constructor(ctx, ArrayBufferConstr_value, L"ArrayBuffer", &ArrayBufferConstr_info,
                                      PROPF_CONSTR|1, &arraybuf->dispex, &ctx->arraybuf_constr);
    jsdisp_release(&arraybuf->dispex);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"ArrayBuffer", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->arraybuf_constr));
    if(FAILED(hres))
        return hres;

    if(!(view = calloc(1, sizeof(DataViewInstance))))
        return E_OUTOFMEMORY;

    hres = create_arraybuf(ctx, 0, &view->buffer);
    if(FAILED(hres)) {
        free(view);
        return hres;
    }

    hres = init_dispex(&view->dispex, ctx, &DataView_info, ctx->object_prototype);
    if(FAILED(hres)) {
        jsdisp_release(&view->buffer->dispex);
        free(view);
        return hres;
    }

    desc.flags = PROPF_CONFIGURABLE;
    desc.mask  = PROPF_CONFIGURABLE | PROPF_ENUMERABLE;
    desc.explicit_getter = desc.explicit_setter = TRUE;
    desc.explicit_value = FALSE;
    desc.setter = NULL;

    /* FIXME: If we find we need builtin accessors in other places, we should consider a more generic solution */
    for(i = 0; i < ARRAY_SIZE(DataView_getters); i++) {
        hres = create_builtin_function(ctx, DataView_getters[i].get, NULL, NULL, PROPF_METHOD, NULL, &desc.getter);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_define_property(&view->dispex, DataView_getters[i].name, &desc);
            jsdisp_release(desc.getter);
        }
        if(FAILED(hres)) {
            jsdisp_release(&view->dispex);
            return hres;
        }
    }

    hres = create_builtin_constructor(ctx, DataViewConstr_value, L"DataView", &DataViewConstr_info,
                                      PROPF_CONSTR|1, &view->dispex, &ctx->dataview_constr);
    jsdisp_release(&view->dispex);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"DataView", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->dataview_constr));
    if(FAILED(hres))
        return hres;

    for(i = 0; i < ARRAY_SIZE(typed_arrays); i++) {
        hres = init_typed_array_constructor(ctx, typed_arrays[i].constr_func, typed_arrays[i].name, typed_arrays[i].prototype_info);
        if(FAILED(hres))
            return hres;
    }

    if(ctx->version >= SCRIPTLANGUAGEVERSION_ES6) {
        hres = init_typed_array_constructor(ctx, Uint8ClampedArrayConstr_value, L"Uint8ClampedArray", &Uint8ClampedArray_info);
        if(FAILED(hres))
            return hres;
    }

    return hres;
}

HRESULT typed_array_get_random_values(jsdisp_t *obj)
{
    TypedArrayInstance *typedarr;
    DWORD size;

    if(!obj || obj->builtin_info->class < FIRST_TYPEDARRAY_JSCLASS || obj->builtin_info->class > LAST_TYPEDARRAY_JSCLASS)
        return E_INVALIDARG;

    if(obj->builtin_info->class == JSCLASS_FLOAT32ARRAY || obj->builtin_info->class == JSCLASS_FLOAT64ARRAY) {
        /* FIXME: Return TypeMismatchError */
        return E_FAIL;
    }

    typedarr = typedarr_from_jsdisp(obj);
    size = typedarr->length * typed_array_descs[obj->builtin_info->class - FIRST_TYPEDARRAY_JSCLASS].size;
    if(size > 65536) {
        /* FIXME: Return QuotaExceededError */
        return E_FAIL;
    }

    if(!RtlGenRandom(&typedarr->buffer->buf[typedarr->offset], size))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}
