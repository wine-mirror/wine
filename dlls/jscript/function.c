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

#include <assert.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    builtin_invoke_t value_proc;
    const WCHAR *name;
    DWORD flags;
    scope_chain_t *scope_chain;
    bytecode_t *code;
    function_code_t *func_code;
    DWORD length;
    jsdisp_t *arguments;
} FunctionInstance;

static inline FunctionInstance *function_from_vdisp(vdisp_t *vdisp)
{
    return (FunctionInstance*)vdisp->u.jsdisp;
}

static inline FunctionInstance *function_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_FUNCTION) ? function_from_vdisp(jsthis) : NULL;
}

static const WCHAR prototypeW[] = {'p','r','o','t','o','t', 'y', 'p','e',0};

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR applyW[] = {'a','p','p','l','y',0};
static const WCHAR callW[] = {'c','a','l','l',0};
static const WCHAR argumentsW[] = {'a','r','g','u','m','e','n','t','s',0};

static HRESULT init_parameters(jsdisp_t *var_disp, FunctionInstance *function, unsigned argc, VARIANT *argv,
        jsexcept_t *ei)
{
    VARIANT var_empty;
    DWORD i=0;
    HRESULT hres;

    V_VT(&var_empty) = VT_EMPTY;

    for(i=0; i < function->func_code->param_cnt; i++) {
        hres = jsdisp_propput_name(var_disp, function->func_code->params[i],
                i < argc ? argv+i : &var_empty, ei);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT Arguments_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_info_t Arguments_info = {
    JSCLASS_ARGUMENTS,
    {NULL, Arguments_value, 0},
    0, NULL,
    NULL,
    NULL
};

static HRESULT create_arguments(script_ctx_t *ctx, IDispatch *calee, unsigned argc, VARIANT *argv,
        jsexcept_t *ei, jsdisp_t **ret)
{
    jsdisp_t *args;
    VARIANT var;
    DWORD i;
    HRESULT hres;

    static const WCHAR caleeW[] = {'c','a','l','l','e','e',0};

    args = heap_alloc_zero(sizeof(jsdisp_t));
    if(!args)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(args, ctx, &Arguments_info, ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(args);
        return hres;
    }

    for(i=0; i < argc; i++) {
        hres = jsdisp_propput_idx(args, i, argv+i, ei);
        if(FAILED(hres))
            break;
    }

    if(SUCCEEDED(hres)) {
        num_set_int(&var, argc);
        hres = jsdisp_propput_name(args, lengthW, &var, ei);

        if(SUCCEEDED(hres)) {
            V_VT(&var) = VT_DISPATCH;
            V_DISPATCH(&var) = calee;
            hres = jsdisp_propput_name(args, caleeW, &var, ei);
        }
    }

    if(FAILED(hres)) {
        jsdisp_release(args);
        return hres;
    }

    *ret = args;
    return S_OK;
}

static HRESULT create_var_disp(script_ctx_t *ctx, FunctionInstance *function, jsdisp_t *arg_disp,
        unsigned argc, VARIANT *argv, jsexcept_t *ei, jsdisp_t **ret)
{
    jsdisp_t *var_disp;
    VARIANT var;
    HRESULT hres;

    hres = create_dispex(ctx, NULL, NULL, &var_disp);
    if(FAILED(hres))
        return hres;

    var_set_jsdisp(&var, arg_disp);
    hres = jsdisp_propput_name(var_disp, argumentsW, &var, ei);
    if(SUCCEEDED(hres))
        hres = init_parameters(var_disp, function, argc, argv, ei);
    if(FAILED(hres)) {
        jsdisp_release(var_disp);
        return hres;
    }

    *ret = var_disp;
    return S_OK;
}

static HRESULT invoke_source(script_ctx_t *ctx, FunctionInstance *function, IDispatch *this_obj, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *var_disp, *arg_disp;
    exec_ctx_t *exec_ctx;
    scope_chain_t *scope;
    HRESULT hres;

    if(!function->func_code) {
        FIXME("no source\n");
        return E_FAIL;
    }

    hres = create_arguments(ctx, to_disp(&function->dispex), argc, argv, ei, &arg_disp);
    if(FAILED(hres))
        return hres;

    hres = create_var_disp(ctx, function, arg_disp, argc, argv, ei, &var_disp);
    if(FAILED(hres)) {
        jsdisp_release(arg_disp);
        return hres;
    }

    hres = scope_push(function->scope_chain, var_disp, &scope);
    if(SUCCEEDED(hres)) {
        hres = create_exec_ctx(ctx, this_obj, var_disp, scope, FALSE, &exec_ctx);
        scope_release(scope);
    }
    jsdisp_release(var_disp);
    if(SUCCEEDED(hres)) {
        jsdisp_t *prev_args;

        prev_args = function->arguments;
        function->arguments = arg_disp;
        hres = exec_source(exec_ctx, function->code, function->func_code, FALSE, ei, retv);
        function->arguments = prev_args;

        jsdisp_release(arg_disp);
        exec_release(exec_ctx);
    }

    return hres;
}

static HRESULT invoke_constructor(script_ctx_t *ctx, FunctionInstance *function, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    jsdisp_t *this_obj;
    VARIANT var;
    HRESULT hres;

    hres = create_object(ctx, &function->dispex, &this_obj);
    if(FAILED(hres))
        return hres;

    hres = invoke_source(ctx, function, to_disp(this_obj), argc, argv, &var, ei);
    if(FAILED(hres)) {
        jsdisp_release(this_obj);
        return hres;
    }

    if(V_VT(&var) == VT_DISPATCH) {
        jsdisp_release(this_obj);
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = V_DISPATCH(&var);
    }else {
        VariantClear(&var);
        var_set_jsdisp(retv, this_obj);
    }
    return S_OK;
}

static HRESULT invoke_value_proc(script_ctx_t *ctx, FunctionInstance *function, IDispatch *this_disp, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    vdisp_t vthis;
    HRESULT hres;

    if(this_disp)
        set_disp(&vthis, this_disp);
    else if(ctx->host_global)
        set_disp(&vthis, ctx->host_global);
    else
        set_jsdisp(&vthis, ctx->global);

    hres = function->value_proc(ctx, &vthis, flags, argc, argv, retv, ei);

    vdisp_release(&vthis);
    return hres;
}

static HRESULT call_function(script_ctx_t *ctx, FunctionInstance *function, IDispatch *this_obj,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    if(function->value_proc)
        return invoke_value_proc(ctx, function, this_obj, DISPATCH_METHOD, argc, argv, retv, ei);

    return invoke_source(ctx, function, this_obj, argc, argv, retv, ei);
}

static HRESULT function_to_string(FunctionInstance *function, BSTR *ret)
{
    BSTR str;

    static const WCHAR native_prefixW[] = {'\n','f','u','n','c','t','i','o','n',' '};
    static const WCHAR native_suffixW[] =
        {'(',')',' ','{','\n',' ',' ',' ',' ','[','n','a','t','i','v','e',' ','c','o','d','e',']','\n','}','\n'};

    if(function->value_proc) {
        DWORD name_len;

        name_len = strlenW(function->name);
        str = SysAllocStringLen(NULL, sizeof(native_prefixW) + name_len*sizeof(WCHAR) + sizeof(native_suffixW));
        if(!str)
            return E_OUTOFMEMORY;

        memcpy(str, native_prefixW, sizeof(native_prefixW));
        memcpy(str + sizeof(native_prefixW)/sizeof(WCHAR), function->name, name_len*sizeof(WCHAR));
        memcpy(str + sizeof(native_prefixW)/sizeof(WCHAR) + name_len, native_suffixW, sizeof(native_suffixW));
    }else {
        str = SysAllocStringLen(function->func_code->source, function->func_code->source_len);
        if(!str)
            return E_OUTOFMEMORY;
    }

    *ret = str;
    return S_OK;
}

HRESULT Function_invoke(jsdisp_t *func_this, IDispatch *jsthis, WORD flags, unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function;

    TRACE("func %p this %p\n", func_this, jsthis);

    assert(is_class(func_this, JSCLASS_FUNCTION));
    function = (FunctionInstance*)func_this;

    if(function->value_proc)
        return invoke_value_proc(function->dispex.ctx, function, jsthis, flags, argc, argv, retv, ei);

    if(flags == DISPATCH_CONSTRUCT)
        return invoke_constructor(function->dispex.ctx, function, argc, argv, retv, ei);

    assert(flags == DISPATCH_METHOD);
    return invoke_source(function->dispex.ctx, function, jsthis, argc, argv, retv, ei);
}

static HRESULT Function_length(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *This = function_from_vdisp(jsthis);

    TRACE("%p %d\n", This, This->length);

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        num_set_int(retv, This->length);
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT Function_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!(function = function_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);

    hres = function_to_string(function, &str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT array_to_args(script_ctx_t *ctx, jsdisp_t *arg_array, jsexcept_t *ei, unsigned *argc, VARIANT **ret)
{
    VARIANT var, *argv;
    DWORD length, i;
    HRESULT hres;

    hres = jsdisp_propget_name(arg_array, lengthW, &var, ei);
    if(FAILED(hres))
        return hres;

    hres = to_uint32(ctx, &var, ei, &length);
    VariantClear(&var);
    if(FAILED(hres))
        return hres;

    argv = heap_alloc(length * sizeof(VARIANT));
    if(!argv)
        return E_OUTOFMEMORY;

    for(i=0; i<length; i++) {
        hres = jsdisp_get_idx(arg_array, i, argv+i, ei);
        if(hres == DISP_E_UNKNOWNNAME)
            V_VT(argv+i) = VT_EMPTY;
        else if(FAILED(hres)) {
            while(i--)
                VariantClear(argv+i);
            heap_free(argv);
            return hres;
        }
    }

    *argc = length;
    *ret = argv;
    return S_OK;
}

static HRESULT Function_apply(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function;
    VARIANT *args = NULL;
    unsigned i, cnt = 0;
    IDispatch *this_obj = NULL;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(!(function = function_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);

    if(argc) {
        if(V_VT(argv) != VT_EMPTY && V_VT(argv) != VT_NULL) {
            hres = to_object(ctx, argv, &this_obj);
            if(FAILED(hres))
                return hres;
        }
    }

    if(argc >= 2) {
        jsdisp_t *arg_array = NULL;

        if(V_VT(argv+1) == VT_DISPATCH) {
            arg_array = iface_to_jsdisp((IUnknown*)V_DISPATCH(argv+1));
            if(arg_array &&
               (!is_class(arg_array, JSCLASS_ARRAY) && !is_class(arg_array, JSCLASS_ARGUMENTS) )) {
                jsdisp_release(arg_array);
                arg_array = NULL;
            }
        }

        if(arg_array) {
            hres = array_to_args(ctx, arg_array, ei, &cnt, &args);
            jsdisp_release(arg_array);
        }else {
            FIXME("throw TypeError\n");
            hres = E_FAIL;
        }
    }

    if(SUCCEEDED(hres))
        hres = call_function(ctx, function, this_obj, cnt, args, retv, ei);

    if(this_obj)
        IDispatch_Release(this_obj);
    for(i=0; i < cnt; i++)
        VariantClear(args+i);
    heap_free(args);
    return hres;
}

static HRESULT Function_call(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function;
    IDispatch *this_obj = NULL;
    unsigned cnt = 0;
    HRESULT hres;

    TRACE("\n");

    if(!(function = function_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);

    if(argc) {
        if(V_VT(argv) != VT_EMPTY && V_VT(argv) != VT_NULL) {
            hres = to_object(ctx, argv, &this_obj);
            if(FAILED(hres))
                return hres;
        }

        cnt = argc-1;
    }

    hres = call_function(ctx, function, this_obj, cnt, argv+1, retv, ei);

    if(this_obj)
        IDispatch_Release(this_obj);
    return hres;
}

HRESULT Function_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function;

    TRACE("\n");

    if(!is_vclass(jsthis, JSCLASS_FUNCTION)) {
        ERR("dispex is not a function\n");
        return E_FAIL;
    }

    function = (FunctionInstance*)jsthis->u.jsdisp;

    switch(flags) {
    case DISPATCH_METHOD:
        assert(function->value_proc != NULL);
        return invoke_value_proc(ctx, function, NULL, flags, argc, argv, retv, ei);

    case DISPATCH_PROPERTYGET: {
        HRESULT hres;
        BSTR str;

        hres = function_to_string(function, &str);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
        break;
    }

    case DISPATCH_CONSTRUCT:
        assert(function->value_proc != NULL);
        return invoke_value_proc(ctx, function, NULL, flags, argc, argv, retv, ei);

    default:
        FIXME("not implemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT Function_arguments(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, VARIANT *argv, VARIANT *retv, jsexcept_t *ei)
{
    FunctionInstance *function = (FunctionInstance*)jsthis->u.jsdisp;
    HRESULT hres = S_OK;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        if(function->arguments) {
            jsdisp_addref(function->arguments);
            var_set_jsdisp(retv, function->arguments);
        }else {
            V_VT(retv) = VT_NULL;
        }
        break;
    }
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static void Function_destructor(jsdisp_t *dispex)
{
    FunctionInstance *This = (FunctionInstance*)dispex;

    if(This->code)
        release_bytecode(This->code);
    if(This->scope_chain)
        scope_release(This->scope_chain);
    heap_free(This);
}

static const builtin_prop_t Function_props[] = {
    {applyW,                 Function_apply,                 PROPF_METHOD|2},
    {argumentsW,             Function_arguments,             0},
    {callW,                  Function_call,                  PROPF_METHOD|1},
    {lengthW,                Function_length,                0},
    {toStringW,              Function_toString,              PROPF_METHOD}
};

static const builtin_info_t Function_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(Function_props)/sizeof(*Function_props),
    Function_props,
    Function_destructor,
    NULL
};

static const builtin_prop_t FunctionInst_props[] = {
    {argumentsW,             Function_arguments,             0},
    {lengthW,                Function_length,                0}
};

static const builtin_info_t FunctionInst_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(FunctionInst_props)/sizeof(*FunctionInst_props),
    FunctionInst_props,
    Function_destructor,
    NULL
};

static HRESULT create_function(script_ctx_t *ctx, const builtin_info_t *builtin_info, DWORD flags,
        BOOL funcprot, jsdisp_t *prototype, FunctionInstance **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    function = heap_alloc_zero(sizeof(FunctionInstance));
    if(!function)
        return E_OUTOFMEMORY;

    if(funcprot)
        hres = init_dispex(&function->dispex, ctx, builtin_info, prototype);
    else if(builtin_info)
        hres = init_dispex_from_constr(&function->dispex, ctx, builtin_info, ctx->function_constr);
    else
        hres = init_dispex_from_constr(&function->dispex, ctx, &FunctionInst_info, ctx->function_constr);
    if(FAILED(hres))
        return hres;

    function->flags = flags;
    function->length = flags & PROPF_ARGMASK;

    *ret = function;
    return S_OK;
}

static HRESULT set_prototype(script_ctx_t *ctx, jsdisp_t *dispex, jsdisp_t *prototype)
{
    jsexcept_t jsexcept;
    VARIANT var;

    var_set_jsdisp(&var, prototype);
    memset(&jsexcept, 0, sizeof(jsexcept));

    return jsdisp_propput_name(dispex, prototypeW, &var, &jsexcept);
}

HRESULT create_builtin_function(script_ctx_t *ctx, builtin_invoke_t value_proc, const WCHAR *name,
        const builtin_info_t *builtin_info, DWORD flags, jsdisp_t *prototype, jsdisp_t **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    hres = create_function(ctx, builtin_info, flags, FALSE, NULL, &function);
    if(FAILED(hres))
        return hres;

    if(builtin_info) {
        VARIANT var;

        num_set_int(&var, function->length);
        hres = jsdisp_propput_const(&function->dispex, lengthW, &var);
    }

    if(SUCCEEDED(hres))
        hres = set_prototype(ctx, &function->dispex, prototype);
    if(FAILED(hres)) {
        jsdisp_release(&function->dispex);
        return hres;
    }

    function->value_proc = value_proc;
    function->name = name;

    *ret = &function->dispex;
    return S_OK;
}

static HRESULT set_constructor_prop(script_ctx_t *ctx, jsdisp_t *constr, jsdisp_t *prot)
{
    VARIANT v;

    static const WCHAR constructorW[] = {'c','o','n','s','t','r','u','c','t','o','r',0};

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = to_disp(constr);
    return jsdisp_propput_dontenum(prot, constructorW, &v);
}

HRESULT create_builtin_constructor(script_ctx_t *ctx, builtin_invoke_t value_proc, const WCHAR *name,
        const builtin_info_t *builtin_info, DWORD flags, jsdisp_t *prototype, jsdisp_t **ret)
{
    jsdisp_t *constr;
    HRESULT hres;

    hres = create_builtin_function(ctx, value_proc, name, builtin_info, flags, prototype, &constr);
    if(FAILED(hres))
        return hres;

    hres = set_constructor_prop(ctx, constr, prototype);
    if(FAILED(hres)) {
        jsdisp_release(constr);
        return hres;
    }

    *ret = constr;
    return S_OK;
}

HRESULT create_source_function(script_ctx_t *ctx, bytecode_t *code, function_code_t *func_code,
        scope_chain_t *scope_chain, jsdisp_t **ret)
{
    FunctionInstance *function;
    jsdisp_t *prototype;
    HRESULT hres;

    hres = create_object(ctx, NULL, &prototype);
    if(FAILED(hres))
        return hres;

    hres = create_function(ctx, NULL, PROPF_CONSTR, FALSE, NULL, &function);
    if(SUCCEEDED(hres)) {
        hres = set_prototype(ctx, &function->dispex, prototype);
        if(SUCCEEDED(hres))
            hres = set_constructor_prop(ctx, &function->dispex, prototype);
        if(FAILED(hres))
            jsdisp_release(&function->dispex);
    }
    jsdisp_release(prototype);
    if(FAILED(hres))
        return hres;

    if(scope_chain) {
        scope_addref(scope_chain);
        function->scope_chain = scope_chain;
    }

    bytecode_addref(code);
    function->code = code;
    function->func_code = func_code;
    function->length = function->func_code->param_cnt;

    *ret = &function->dispex;
    return S_OK;
}

static HRESULT construct_function(script_ctx_t *ctx, unsigned argc, VARIANT *argv, jsexcept_t *ei, IDispatch **ret)
{
    WCHAR *str = NULL, *ptr;
    DWORD len = 0, l;
    bytecode_t *code;
    jsdisp_t *function;
    BSTR *params = NULL;
    int i=0, j=0;
    HRESULT hres = S_OK;

    static const WCHAR function_anonymousW[] = {'f','u','n','c','t','i','o','n',' ','a','n','o','n','y','m','o','u','s','('};
    static const WCHAR function_beginW[] = {')',' ','{','\n'};
    static const WCHAR function_endW[] = {'\n','}',0};

    if(argc) {
        params = heap_alloc(argc*sizeof(BSTR));
        if(!params)
            return E_OUTOFMEMORY;

        if(argc > 2)
            len = (argc-2)*2; /* separating commas */
        for(i=0; i < argc; i++) {
            hres = to_string(ctx, argv+i, ei, params+i);
            if(FAILED(hres))
                break;
            len += SysStringLen(params[i]);
        }
    }

    if(SUCCEEDED(hres)) {
        len += (sizeof(function_anonymousW) + sizeof(function_beginW) + sizeof(function_endW)) / sizeof(WCHAR);
        str = heap_alloc(len*sizeof(WCHAR));
        if(str) {
            memcpy(str, function_anonymousW, sizeof(function_anonymousW));
            ptr = str + sizeof(function_anonymousW)/sizeof(WCHAR);
            if(argc > 1) {
                while(1) {
                    l = SysStringLen(params[j]);
                    memcpy(ptr, params[j], l*sizeof(WCHAR));
                    ptr += l;
                    if(++j == argc-1)
                        break;
                    *ptr++ = ',';
                    *ptr++ = ' ';
                }
            }
            memcpy(ptr, function_beginW, sizeof(function_beginW));
            ptr += sizeof(function_beginW)/sizeof(WCHAR);
            if(argc) {
                l = SysStringLen(params[argc-1]);
                memcpy(ptr, params[argc-1], l*sizeof(WCHAR));
                ptr += l;
            }
            memcpy(ptr, function_endW, sizeof(function_endW));

            TRACE("%s\n", debugstr_w(str));
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    while(--i >= 0)
        SysFreeString(params[i]);
    heap_free(params);
    if(FAILED(hres))
        return hres;

    hres = compile_script(ctx, str, NULL, FALSE, FALSE, &code);
    heap_free(str);
    if(FAILED(hres))
        return hres;

    if(code->global_code.func_cnt != 1 || code->global_code.var_cnt) {
        ERR("Invalid parser result!\n");
        release_bytecode(code);
        return E_UNEXPECTED;
    }

    hres = create_source_function(ctx, code, code->global_code.funcs, NULL, &function);
    release_bytecode(code);
    if(FAILED(hres))
        return hres;

    *ret = to_disp(function);
    return S_OK;
}

static HRESULT FunctionConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        IDispatch *ret;

        hres = construct_function(ctx, argc, argv, ei, &ret);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = ret;
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT FunctionProt_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, VARIANT *argv,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT init_function_constr(script_ctx_t *ctx, jsdisp_t *object_prototype)
{
    FunctionInstance *prot, *constr;
    HRESULT hres;

    static const WCHAR FunctionW[] = {'F','u','n','c','t','i','o','n',0};

    hres = create_function(ctx, &Function_info, PROPF_CONSTR, TRUE, object_prototype, &prot);
    if(FAILED(hres))
        return hres;

    prot->value_proc = FunctionProt_value;
    prot->name = prototypeW;

    hres = create_function(ctx, &FunctionInst_info, PROPF_CONSTR|1, TRUE, &prot->dispex, &constr);
    if(SUCCEEDED(hres)) {
        constr->value_proc = FunctionConstr_value;
        constr->name = FunctionW;
        hres = set_prototype(ctx, &constr->dispex, &prot->dispex);
        if(SUCCEEDED(hres))
            hres = set_constructor_prop(ctx, &constr->dispex, &prot->dispex);
        if(FAILED(hres))
            jsdisp_release(&constr->dispex);
    }
    jsdisp_release(&prot->dispex);
    if(FAILED(hres))
        return hres;

    ctx->function_constr = &constr->dispex;
    return S_OK;
}
