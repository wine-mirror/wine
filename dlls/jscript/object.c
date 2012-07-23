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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static const WCHAR default_valueW[] = {'[','o','b','j','e','c','t',' ','O','b','j','e','c','t',']',0};

static HRESULT Object_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *jsdisp;
    const WCHAR *str;

    static const WCHAR formatW[] = {'[','o','b','j','e','c','t',' ','%','s',']',0};

    static const WCHAR arrayW[] = {'A','r','r','a','y',0};
    static const WCHAR booleanW[] = {'B','o','o','l','e','a','n',0};
    static const WCHAR dateW[] = {'D','a','t','e',0};
    static const WCHAR errorW[] = {'E','r','r','o','r',0};
    static const WCHAR functionW[] = {'F','u','n','c','t','i','o','n',0};
    static const WCHAR mathW[] = {'M','a','t','h',0};
    static const WCHAR numberW[] = {'N','u','m','b','e','r',0};
    static const WCHAR objectW[] = {'O','b','j','e','c','t',0};
    static const WCHAR regexpW[] = {'R','e','g','E','x','p',0};
    static const WCHAR stringW[] = {'S','t','r','i','n','g',0};
    /* Keep in sync with jsclass_t enum */
    static const WCHAR *names[] = {objectW, arrayW, booleanW, dateW, errorW,
        functionW, NULL, mathW, numberW, objectW, regexpW, stringW, objectW, objectW};

    TRACE("\n");

    jsdisp = get_jsdisp(jsthis);
    if(!jsdisp) {
        str = objectW;
    }else if(names[jsdisp->builtin_info->class]) {
        str = names[jsdisp->builtin_info->class];
    }else {
        FIXME("jdisp->builtin_info->class = %d\n", jsdisp->builtin_info->class);
        return E_FAIL;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocStringLen(NULL, 9+strlenW(str));
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;

        sprintfW(V_BSTR(retv), formatW, str);
    }

    return S_OK;
}

static HRESULT Object_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");

    if(!is_jsdisp(jsthis)) {
        FIXME("Host object this\n");
        return E_FAIL;
    }

    return jsdisp_call_name(jsthis->u.jsdisp, toStringW, DISPATCH_METHOD, 0, NULL, retv, ei);
}

static HRESULT Object_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");

    if(retv) {
        IDispatch_AddRef(jsthis->u.disp);

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = jsthis->u.disp;
    }

    return S_OK;
}

static HRESULT Object_hasOwnProperty(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    BSTR name;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(retv) {
            V_VT(retv) = VT_BOOL;
            V_BOOL(retv) = VARIANT_FALSE;
        }

        return S_OK;
    }

    hres = to_string(ctx, argv, ei, &name);
    if(FAILED(hres))
        return hres;

    if(is_jsdisp(jsthis)) {
        VARIANT_BOOL result;

        hres = jsdisp_is_own_prop(jsthis->u.jsdisp, name, &result);
        if(FAILED(hres))
            return hres;

        if(retv) {
            V_VT(retv) = VT_BOOL;
            V_BOOL(retv) = result;
        }

        return S_OK;
    }

    if(is_dispex(jsthis)) {
        hres = IDispatchEx_GetDispID(jsthis->u.dispex, name,
                make_grfdex(ctx, fdexNameCaseSensitive), &id);
    } else {
        hres = IDispatch_GetIDsOfNames(jsthis->u.disp, &IID_NULL,
                &name, 1, ctx->lcid, &id);
    }

    if(retv) {
        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = SUCCEEDED(hres) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Object_propertyIsEnumerable(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_isPrototypeOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);
    case DISPATCH_PROPERTYGET:
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocString(default_valueW);
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Object_destructor(jsdisp_t *dispex)
{
    heap_free(dispex);
}

static const builtin_prop_t Object_props[] = {
    {hasOwnPropertyW,        Object_hasOwnProperty,        PROPF_METHOD|1},
    {isPrototypeOfW,         Object_isPrototypeOf,         PROPF_METHOD|1},
    {propertyIsEnumerableW,  Object_propertyIsEnumerable,  PROPF_METHOD|1},
    {toLocaleStringW,        Object_toLocaleString,        PROPF_METHOD},
    {toStringW,              Object_toString,              PROPF_METHOD},
    {valueOfW,               Object_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Object_info = {
    JSCLASS_OBJECT,
    {NULL, Object_value, 0},
    sizeof(Object_props)/sizeof(*Object_props),
    Object_props,
    Object_destructor,
    NULL
};

static const builtin_info_t ObjectInst_info = {
    JSCLASS_OBJECT,
    {NULL, Object_value, 0},
    0, NULL,
    Object_destructor,
    NULL
};

static HRESULT ObjectConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc) {
            if(V_VT(argv) != VT_EMPTY && V_VT(argv) != VT_NULL && (V_VT(argv) != VT_DISPATCH || V_DISPATCH(argv))) {
                IDispatch *disp;

                hres = to_object(ctx, argv, &disp);
                if(FAILED(hres))
                    return hres;

                if(retv) {
                    V_VT(retv) = VT_DISPATCH;
                    V_DISPATCH(retv) = disp;
                }else {
                    IDispatch_Release(disp);
                }
                return S_OK;
            }
        }
        /* fall through */
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *obj;

        hres = create_object(ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        if(retv)
            var_set_jsdisp(retv, obj);
        else
            jsdisp_release(obj);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_object_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    static const WCHAR ObjectW[] = {'O','b','j','e','c','t',0};

    return create_builtin_constructor(ctx, ObjectConstr_value, ObjectW, NULL, PROPF_CONSTR,
            object_prototype, ret);
}

HRESULT create_object_prototype(script_ctx_t *ctx, jsdisp_t **ret)
{
    return create_dispex(ctx, &Object_info, NULL, ret);
}

HRESULT create_object(script_ctx_t *ctx, jsdisp_t *constr, jsdisp_t **ret)
{
    jsdisp_t *object;
    HRESULT hres;

    object = heap_alloc_zero(sizeof(jsdisp_t));
    if(!object)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(object, ctx, &ObjectInst_info, constr ? constr : ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(object);
        return hres;
    }

    *ret = object;
    return S_OK;
}
