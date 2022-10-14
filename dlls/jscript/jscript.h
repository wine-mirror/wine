/*
 * Copyright 2008-2009 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "resource.h"

#include "wine/heap.h"
#include "wine/list.h"

/*
 * This is Wine jscript extension for ES5 compatible mode. Native IE9+ implements
 * a separated JavaScript enging in side MSHTML. We implement its features here
 * and enable it when HTML flag is specified in SCRIPTPROP_INVOKEVERSIONING property.
 */
#define SCRIPTLANGUAGEVERSION_HTML 0x400

/*
 * This is Wine jscript extension for ES5 and ES6 compatible mode. Allowed only in HTML mode.
 */
#define SCRIPTLANGUAGEVERSION_ES5  0x102
#define SCRIPTLANGUAGEVERSION_ES6  0x103

typedef struct _jsval_t jsval_t;
typedef struct _jsstr_t jsstr_t;
typedef struct _jsexcept_t jsexcept_t;
typedef struct _script_ctx_t script_ctx_t;
typedef struct _dispex_prop_t dispex_prop_t;
typedef struct _property_desc_t property_desc_t;

typedef struct {
    void **blocks;
    DWORD block_cnt;
    DWORD last_block;
    DWORD offset;
    BOOL mark;
    struct list custom_blocks;
} heap_pool_t;

void heap_pool_init(heap_pool_t*) DECLSPEC_HIDDEN;
void *heap_pool_alloc(heap_pool_t*,DWORD) __WINE_ALLOC_SIZE(2) DECLSPEC_HIDDEN;
void *heap_pool_grow(heap_pool_t*,void*,DWORD,DWORD) DECLSPEC_HIDDEN;
void heap_pool_clear(heap_pool_t*) DECLSPEC_HIDDEN;
void heap_pool_free(heap_pool_t*) DECLSPEC_HIDDEN;
heap_pool_t *heap_pool_mark(heap_pool_t*) DECLSPEC_HIDDEN;

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (lstrlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

typedef struct jsdisp_t jsdisp_t;

extern HINSTANCE jscript_hinstance DECLSPEC_HIDDEN;
HRESULT get_dispatch_typeinfo(ITypeInfo**) DECLSPEC_HIDDEN;

#define PROPF_ARGMASK       0x00ff
#define PROPF_METHOD        0x0100
#define PROPF_CONSTR        0x0200

#define PROPF_ENUMERABLE    0x0400
#define PROPF_WRITABLE      0x0800
#define PROPF_CONFIGURABLE  0x1000
#define PROPF_ALL           (PROPF_ENUMERABLE | PROPF_WRITABLE | PROPF_CONFIGURABLE)

#define PROPF_VERSION_MASK  0x01ff0000
#define PROPF_VERSION_SHIFT 16
#define PROPF_HTML          (SCRIPTLANGUAGEVERSION_HTML << PROPF_VERSION_SHIFT)
#define PROPF_ES5           ((SCRIPTLANGUAGEVERSION_HTML|SCRIPTLANGUAGEVERSION_ES5) << PROPF_VERSION_SHIFT)
#define PROPF_ES6           ((SCRIPTLANGUAGEVERSION_HTML|SCRIPTLANGUAGEVERSION_ES6) << PROPF_VERSION_SHIFT)

/*
 * This is our internal dispatch flag informing calee that it's called directly from interpreter.
 * If calee is executed as interpreted function, we may let already running interpreter to take
 * of execution.
 */
#define DISPATCH_JSCRIPT_CALLEREXECSSOURCE  0x8000
#define DISPATCH_JSCRIPT_INTERNAL_MASK      DISPATCH_JSCRIPT_CALLEREXECSSOURCE

/* NOTE: Keep in sync with names in Object.toString implementation */
typedef enum {
    JSCLASS_NONE,
    JSCLASS_ARRAY,
    JSCLASS_BOOLEAN,
    JSCLASS_DATE,
    JSCLASS_ENUMERATOR,
    JSCLASS_ERROR,
    JSCLASS_FUNCTION,
    JSCLASS_GLOBAL,
    JSCLASS_MATH,
    JSCLASS_NUMBER,
    JSCLASS_OBJECT,
    JSCLASS_REGEXP,
    JSCLASS_STRING,
    JSCLASS_ARGUMENTS,
    JSCLASS_VBARRAY,
    JSCLASS_JSON,
    JSCLASS_MAP,
    JSCLASS_SET,
} jsclass_t;

jsdisp_t *iface_to_jsdisp(IDispatch*) DECLSPEC_HIDDEN;

typedef HRESULT (*builtin_invoke_t)(script_ctx_t*,jsval_t,WORD,unsigned,jsval_t*,jsval_t*);
typedef HRESULT (*builtin_getter_t)(script_ctx_t*,jsdisp_t*,jsval_t*);
typedef HRESULT (*builtin_setter_t)(script_ctx_t*,jsdisp_t*,jsval_t);

HRESULT builtin_set_const(script_ctx_t*,jsdisp_t*,jsval_t) DECLSPEC_HIDDEN;

typedef struct named_item_t {
    jsdisp_t *script_obj;
    IDispatch *disp;
    unsigned ref;
    DWORD flags;
    LPWSTR name;

    struct list entry;
} named_item_t;

HRESULT create_named_item_script_obj(script_ctx_t*,named_item_t*) DECLSPEC_HIDDEN;
named_item_t *lookup_named_item(script_ctx_t*,const WCHAR*,unsigned) DECLSPEC_HIDDEN;
void release_named_item(named_item_t*) DECLSPEC_HIDDEN;

typedef struct {
    const WCHAR *name;
    builtin_invoke_t invoke;
    DWORD flags;
    builtin_getter_t getter;
    builtin_setter_t setter;
} builtin_prop_t;

typedef struct {
    jsclass_t class;
    builtin_invoke_t call;
    DWORD props_cnt;
    const builtin_prop_t *props;
    void (*destructor)(jsdisp_t*);
    void (*on_put)(jsdisp_t*,const WCHAR*);
    unsigned (*idx_length)(jsdisp_t*);
    HRESULT (*idx_get)(jsdisp_t*,unsigned,jsval_t*);
    HRESULT (*idx_put)(jsdisp_t*,unsigned,jsval_t);
} builtin_info_t;

struct jsdisp_t {
    IDispatchEx IDispatchEx_iface;

    LONG ref;

    DWORD buf_size;
    DWORD prop_cnt;
    dispex_prop_t *props;
    script_ctx_t *ctx;
    BOOL extensible;

    jsdisp_t *prototype;

    const builtin_info_t *builtin_info;
};

static inline IDispatch *to_disp(jsdisp_t *jsdisp)
{
    return (IDispatch*)&jsdisp->IDispatchEx_iface;
}

jsdisp_t *as_jsdisp(IDispatch*) DECLSPEC_HIDDEN;
jsdisp_t *to_jsdisp(IDispatch*) DECLSPEC_HIDDEN;
void jsdisp_free(jsdisp_t*) DECLSPEC_HIDDEN;

#ifndef TRACE_REFCNT

/*
 * We do a lot of refcount calls during script execution, so having an inline
 * version is a nice perf win. Define TRACE_REFCNT macro when debugging
 * refcount bugs to have traces. Also, since jsdisp_t is not thread safe anyways,
 * there is no point in using atomic operations.
 */
static inline jsdisp_t *jsdisp_addref(jsdisp_t *jsdisp)
{
    jsdisp->ref++;
    return jsdisp;
}

static inline void jsdisp_release(jsdisp_t *jsdisp)
{
    if(!--jsdisp->ref)
        jsdisp_free(jsdisp);
}

#else

jsdisp_t *jsdisp_addref(jsdisp_t*) DECLSPEC_HIDDEN;
void jsdisp_release(jsdisp_t*) DECLSPEC_HIDDEN;

#endif

enum jsdisp_enum_type {
    JSDISP_ENUM_ALL,
    JSDISP_ENUM_OWN,
    JSDISP_ENUM_OWN_ENUMERABLE
};

HRESULT create_dispex(script_ctx_t*,const builtin_info_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT init_dispex(jsdisp_t*,script_ctx_t*,const builtin_info_t*,jsdisp_t*) DECLSPEC_HIDDEN;
HRESULT init_dispex_from_constr(jsdisp_t*,script_ctx_t*,const builtin_info_t*,jsdisp_t*) DECLSPEC_HIDDEN;

HRESULT disp_call(script_ctx_t*,IDispatch*,DISPID,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT disp_call_name(script_ctx_t*,IDispatch*,const WCHAR*,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT disp_call_value(script_ctx_t*,IDispatch*,IDispatch*,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_call_value(jsdisp_t*,IDispatch*,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_call(jsdisp_t*,DISPID,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_call_name(jsdisp_t*,const WCHAR*,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT disp_propget(script_ctx_t*,IDispatch*,DISPID,jsval_t*) DECLSPEC_HIDDEN;
HRESULT disp_propput(script_ctx_t*,IDispatch*,DISPID,jsval_t) DECLSPEC_HIDDEN;
HRESULT disp_propput_name(script_ctx_t*,IDispatch*,const WCHAR*,jsval_t) DECLSPEC_HIDDEN;
HRESULT jsdisp_propget(jsdisp_t*,DISPID,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_propput(jsdisp_t*,const WCHAR*,DWORD,BOOL,jsval_t) DECLSPEC_HIDDEN;
HRESULT jsdisp_propput_name(jsdisp_t*,const WCHAR*,jsval_t) DECLSPEC_HIDDEN;
HRESULT jsdisp_propput_idx(jsdisp_t*,DWORD,jsval_t) DECLSPEC_HIDDEN;
HRESULT jsdisp_propget_name(jsdisp_t*,LPCWSTR,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_get_idx(jsdisp_t*,DWORD,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_get_id(jsdisp_t*,const WCHAR*,DWORD,DISPID*) DECLSPEC_HIDDEN;
HRESULT disp_delete(IDispatch*,DISPID,BOOL*) DECLSPEC_HIDDEN;
HRESULT disp_delete_name(script_ctx_t*,IDispatch*,jsstr_t*,BOOL*) DECLSPEC_HIDDEN;
HRESULT jsdisp_delete_idx(jsdisp_t*,DWORD) DECLSPEC_HIDDEN;
HRESULT jsdisp_get_own_property(jsdisp_t*,const WCHAR*,BOOL,property_desc_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_define_property(jsdisp_t*,const WCHAR*,property_desc_t*) DECLSPEC_HIDDEN;
HRESULT jsdisp_define_data_property(jsdisp_t*,const WCHAR*,unsigned,jsval_t) DECLSPEC_HIDDEN;
HRESULT jsdisp_next_prop(jsdisp_t*,DISPID,enum jsdisp_enum_type,DISPID*) DECLSPEC_HIDDEN;
HRESULT jsdisp_get_prop_name(jsdisp_t*,DISPID,jsstr_t**);
HRESULT jsdisp_change_prototype(jsdisp_t*,jsdisp_t*) DECLSPEC_HIDDEN;
void jsdisp_freeze(jsdisp_t*,BOOL) DECLSPEC_HIDDEN;
BOOL jsdisp_is_frozen(jsdisp_t*,BOOL) DECLSPEC_HIDDEN;

HRESULT create_builtin_function(script_ctx_t*,builtin_invoke_t,const WCHAR*,const builtin_info_t*,DWORD,
        jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_builtin_constructor(script_ctx_t*,builtin_invoke_t,const WCHAR*,const builtin_info_t*,DWORD,
        jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT Function_invoke(jsdisp_t*,IDispatch*,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;

HRESULT Function_value(script_ctx_t*,jsval_t,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT Function_get_value(script_ctx_t*,jsdisp_t*,jsval_t*) DECLSPEC_HIDDEN;
struct _function_code_t *Function_get_code(jsdisp_t*) DECLSPEC_HIDDEN;

HRESULT throw_error(script_ctx_t*,HRESULT,const WCHAR*) DECLSPEC_HIDDEN;
jsdisp_t *create_builtin_error(script_ctx_t *ctx) DECLSPEC_HIDDEN;

HRESULT create_object(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_math(script_ctx_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_array(script_ctx_t*,DWORD,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_regexp(script_ctx_t*,jsstr_t*,DWORD,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_regexp_var(script_ctx_t*,jsval_t,jsval_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_string(script_ctx_t*,jsstr_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_bool(script_ctx_t*,BOOL,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_number(script_ctx_t*,double,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_vbarray(script_ctx_t*,SAFEARRAY*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_json(script_ctx_t*,jsdisp_t**) DECLSPEC_HIDDEN;

typedef enum {
    NO_HINT,
    HINT_STRING,
    HINT_NUMBER
} hint_t;

HRESULT to_primitive(script_ctx_t*,jsval_t,jsval_t*, hint_t) DECLSPEC_HIDDEN;
HRESULT to_boolean(jsval_t,BOOL*) DECLSPEC_HIDDEN;
HRESULT to_number(script_ctx_t*,jsval_t,double*) DECLSPEC_HIDDEN;
HRESULT to_integer(script_ctx_t*,jsval_t,double*) DECLSPEC_HIDDEN;
HRESULT to_int32(script_ctx_t*,jsval_t,INT*) DECLSPEC_HIDDEN;
HRESULT to_long(script_ctx_t*,jsval_t,LONG*) DECLSPEC_HIDDEN;
HRESULT to_uint32(script_ctx_t*,jsval_t,UINT32*) DECLSPEC_HIDDEN;
HRESULT to_string(script_ctx_t*,jsval_t,jsstr_t**) DECLSPEC_HIDDEN;
HRESULT to_flat_string(script_ctx_t*,jsval_t,jsstr_t**,const WCHAR**) DECLSPEC_HIDDEN;
HRESULT to_object(script_ctx_t*,jsval_t,IDispatch**) DECLSPEC_HIDDEN;

HRESULT jsval_strict_equal(jsval_t,jsval_t,BOOL*) DECLSPEC_HIDDEN;

HRESULT variant_change_type(script_ctx_t*,VARIANT*,VARIANT*,VARTYPE) DECLSPEC_HIDDEN;
HRESULT variant_date_to_number(double,double*) DECLSPEC_HIDDEN;
HRESULT variant_date_to_string(script_ctx_t*,double,jsstr_t**) DECLSPEC_HIDDEN;

HRESULT decode_source(WCHAR*) DECLSPEC_HIDDEN;

HRESULT double_to_string(double,jsstr_t**) DECLSPEC_HIDDEN;
WCHAR *idx_to_str(DWORD,WCHAR*) DECLSPEC_HIDDEN;

static inline BOOL is_digit(WCHAR c)
{
    return '0' <= c && c <= '9';
}

typedef struct _cc_var_t cc_var_t;

typedef struct {
    cc_var_t *vars;
} cc_ctx_t;

void release_cc(cc_ctx_t*) DECLSPEC_HIDDEN;

typedef struct {
    IServiceProvider IServiceProvider_iface;

    LONG ref;

    script_ctx_t *ctx;
} JSCaller;

#include "jsval.h"

struct _property_desc_t {
    unsigned flags;
    unsigned mask;
    BOOL explicit_value;
    jsval_t value;
    BOOL explicit_getter;
    jsdisp_t *getter;
    BOOL explicit_setter;
    jsdisp_t *setter;
};

typedef struct {
    unsigned index;
    unsigned length;
} match_result_t;

struct _script_ctx_t {
    LONG ref;

    SCRIPTSTATE state;
    IActiveScript *active_script;

    struct _call_frame_t *call_ctx;
    struct list named_items;
    IActiveScriptSite *site;
    IInternetHostSecurityManager *secmgr;
    DWORD safeopt;
    DWORD version;
    BOOL html_mode;
    LCID lcid;
    cc_ctx_t *cc;
    JSCaller *jscaller;
    jsexcept_t *ei;

    heap_pool_t tmp_heap;

    jsval_t *stack;
    unsigned stack_top;
    jsval_t acc;

    jsstr_t *last_match;
    match_result_t match_parens[9];
    DWORD last_match_index;
    DWORD last_match_length;

    union {
        struct {
            jsdisp_t *global;
            jsdisp_t *function_constr;
            jsdisp_t *array_constr;
            jsdisp_t *bool_constr;
            jsdisp_t *date_constr;
            jsdisp_t *enumerator_constr;
            jsdisp_t *error_constr;
            jsdisp_t *eval_error_constr;
            jsdisp_t *range_error_constr;
            jsdisp_t *reference_error_constr;
            jsdisp_t *regexp_error_constr;
            jsdisp_t *syntax_error_constr;
            jsdisp_t *type_error_constr;
            jsdisp_t *uri_error_constr;
            jsdisp_t *number_constr;
            jsdisp_t *object_constr;
            jsdisp_t *object_prototype;
            jsdisp_t *regexp_constr;
            jsdisp_t *string_constr;
            jsdisp_t *vbarray_constr;
            jsdisp_t *map_prototype;
            jsdisp_t *set_prototype;
        };
        jsdisp_t *global_objects[22];
    };
};
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(script_ctx_t, set_prototype) == RTL_SIZEOF_THROUGH_FIELD(script_ctx_t, global_objects));

void script_release(script_ctx_t*) DECLSPEC_HIDDEN;

static inline void script_addref(script_ctx_t *ctx)
{
    ctx->ref++;
}

HRESULT init_global(script_ctx_t*) DECLSPEC_HIDDEN;
HRESULT init_function_constr(script_ctx_t*,jsdisp_t*) DECLSPEC_HIDDEN;
HRESULT create_object_prototype(script_ctx_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT init_set_constructor(script_ctx_t*) DECLSPEC_HIDDEN;

HRESULT create_activex_constr(script_ctx_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_array_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_bool_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_date_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT init_error_constr(script_ctx_t*,jsdisp_t*) DECLSPEC_HIDDEN;
HRESULT create_enumerator_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_number_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_object_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_regexp_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_string_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;
HRESULT create_vbarray_constr(script_ctx_t*,jsdisp_t*,jsdisp_t**) DECLSPEC_HIDDEN;

IUnknown *create_ax_site(script_ctx_t*) DECLSPEC_HIDDEN;
HRESULT create_jscaller(script_ctx_t*) DECLSPEC_HIDDEN;

#define REM_CHECK_GLOBAL   0x0001
#define REM_RESET_INDEX    0x0002
#define REM_NO_CTX_UPDATE  0x0004
#define REM_ALLOC_RESULT   0x0008
#define REM_NO_PARENS      0x0010
struct match_state_t;
HRESULT regexp_match_next(script_ctx_t*,jsdisp_t*,DWORD,jsstr_t*,struct match_state_t**) DECLSPEC_HIDDEN;
HRESULT parse_regexp_flags(const WCHAR*,DWORD,DWORD*) DECLSPEC_HIDDEN;
HRESULT regexp_string_match(script_ctx_t*,jsdisp_t*,jsstr_t*,jsval_t*) DECLSPEC_HIDDEN;

BOOL bool_obj_value(jsdisp_t*) DECLSPEC_HIDDEN;
unsigned array_get_length(jsdisp_t*) DECLSPEC_HIDDEN;
HRESULT localize_number(script_ctx_t*,DOUBLE,BOOL,jsstr_t**) DECLSPEC_HIDDEN;

HRESULT JSGlobal_eval(script_ctx_t*,jsval_t,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT Object_get_proto_(script_ctx_t*,jsval_t,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT Object_set_proto_(script_ctx_t*,jsval_t,WORD,unsigned,jsval_t*,jsval_t*) DECLSPEC_HIDDEN;

static inline BOOL is_class(jsdisp_t *jsdisp, jsclass_t class)
{
    return jsdisp->builtin_info->class == class;
}

static inline BOOL is_int32(double d)
{
    return INT32_MIN <= d && d <= INT32_MAX && (double)(int)d == d;
}

static inline DWORD make_grfdex(script_ctx_t *ctx, DWORD flags)
{
    return ((ctx->version & 0xff) << 28) | flags;
}

#define FACILITY_JSCRIPT 10

#define MAKE_JSERROR(code) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_JSCRIPT, code)

#define JS_E_TO_PRIMITIVE            MAKE_JSERROR(IDS_TO_PRIMITIVE)
#define JS_E_INVALIDARG              MAKE_JSERROR(IDS_INVALID_CALL_ARG)
#define JS_E_SUBSCRIPT_OUT_OF_RANGE  MAKE_JSERROR(IDS_SUBSCRIPT_OUT_OF_RANGE)
#define JS_E_STACK_OVERFLOW          MAKE_JSERROR(IDS_STACK_OVERFLOW)
#define JS_E_OBJECT_REQUIRED         MAKE_JSERROR(IDS_OBJECT_REQUIRED)
#define JS_E_CANNOT_CREATE_OBJ       MAKE_JSERROR(IDS_CREATE_OBJ_ERROR)
#define JS_E_INVALID_PROPERTY        MAKE_JSERROR(IDS_NO_PROPERTY)
#define JS_E_INVALID_ACTION          MAKE_JSERROR(IDS_UNSUPPORTED_ACTION)
#define JS_E_MISSING_ARG             MAKE_JSERROR(IDS_ARG_NOT_OPT)
#define JS_E_SYNTAX                  MAKE_JSERROR(IDS_SYNTAX_ERROR)
#define JS_E_MISSING_SEMICOLON       MAKE_JSERROR(IDS_SEMICOLON)
#define JS_E_MISSING_LBRACKET        MAKE_JSERROR(IDS_LBRACKET)
#define JS_E_MISSING_RBRACKET        MAKE_JSERROR(IDS_RBRACKET)
#define JS_E_EXPECTED_IDENTIFIER     MAKE_JSERROR(IDS_EXPECTED_IDENTIFIER)
#define JS_E_EXPECTED_ASSIGN         MAKE_JSERROR(IDS_EXPECTED_ASSIGN)
#define JS_E_INVALID_CHAR            MAKE_JSERROR(IDS_INVALID_CHAR)
#define JS_E_UNTERMINATED_STRING     MAKE_JSERROR(IDS_UNTERMINATED_STR)
#define JS_E_MISPLACED_RETURN        MAKE_JSERROR(IDS_MISPLACED_RETURN)
#define JS_E_INVALID_BREAK           MAKE_JSERROR(IDS_INVALID_BREAK)
#define JS_E_INVALID_CONTINUE        MAKE_JSERROR(IDS_INVALID_CONTINUE)
#define JS_E_LABEL_REDEFINED         MAKE_JSERROR(IDS_LABEL_REDEFINED)
#define JS_E_LABEL_NOT_FOUND         MAKE_JSERROR(IDS_LABEL_NOT_FOUND)
#define JS_E_EXPECTED_CCEND          MAKE_JSERROR(IDS_EXPECTED_CCEND)
#define JS_E_DISABLED_CC             MAKE_JSERROR(IDS_DISABLED_CC)
#define JS_E_EXPECTED_AT             MAKE_JSERROR(IDS_EXPECTED_AT)
#define JS_E_FUNCTION_EXPECTED       MAKE_JSERROR(IDS_NOT_FUNC)
#define JS_E_DATE_EXPECTED           MAKE_JSERROR(IDS_NOT_DATE)
#define JS_E_NUMBER_EXPECTED         MAKE_JSERROR(IDS_NOT_NUM)
#define JS_E_OBJECT_EXPECTED         MAKE_JSERROR(IDS_OBJECT_EXPECTED)
#define JS_E_ILLEGAL_ASSIGN          MAKE_JSERROR(IDS_ILLEGAL_ASSIGN)
#define JS_E_UNDEFINED_VARIABLE      MAKE_JSERROR(IDS_UNDEFINED)
#define JS_E_BOOLEAN_EXPECTED        MAKE_JSERROR(IDS_NOT_BOOL)
#define JS_E_VBARRAY_EXPECTED        MAKE_JSERROR(IDS_NOT_VBARRAY)
#define JS_E_INVALID_DELETE          MAKE_JSERROR(IDS_INVALID_DELETE)
#define JS_E_JSCRIPT_EXPECTED        MAKE_JSERROR(IDS_JSCRIPT_EXPECTED)
#define JS_E_ENUMERATOR_EXPECTED     MAKE_JSERROR(IDS_ENUMERATOR_EXPECTED)
#define JS_E_REGEXP_EXPECTED         MAKE_JSERROR(IDS_REGEXP_EXPECTED)
#define JS_E_REGEXP_SYNTAX           MAKE_JSERROR(IDS_REGEXP_SYNTAX_ERROR)
#define JS_E_EXCEPTION_THROWN        MAKE_JSERROR(IDS_EXCEPTION_THROWN)
#define JS_E_INVALID_URI_CODING      MAKE_JSERROR(IDS_URI_INVALID_CODING)
#define JS_E_INVALID_URI_CHAR        MAKE_JSERROR(IDS_URI_INVALID_CHAR)
#define JS_E_FRACTION_DIGITS_OUT_OF_RANGE MAKE_JSERROR(IDS_FRACTION_DIGITS_OUT_OF_RANGE)
#define JS_E_PRECISION_OUT_OF_RANGE  MAKE_JSERROR(IDS_PRECISION_OUT_OF_RANGE)
#define JS_E_INVALID_LENGTH          MAKE_JSERROR(IDS_INVALID_LENGTH)
#define JS_E_ARRAY_EXPECTED          MAKE_JSERROR(IDS_ARRAY_EXPECTED)
#define JS_E_CYCLIC_PROTO_VALUE      MAKE_JSERROR(IDS_CYCLIC_PROTO_VALUE)
#define JS_E_CANNOT_CREATE_FOR_NONEXTENSIBLE MAKE_JSERROR(IDS_CREATE_FOR_NONEXTENSIBLE)
#define JS_E_OBJECT_NONEXTENSIBLE    MAKE_JSERROR(IDS_OBJECT_NONEXTENSIBLE)
#define JS_E_NONCONFIGURABLE_REDEFINED MAKE_JSERROR(IDS_NONCONFIGURABLE_REDEFINED)
#define JS_E_NONWRITABLE_MODIFIED    MAKE_JSERROR(IDS_NONWRITABLE_MODIFIED)
#define JS_E_WRONG_THIS              MAKE_JSERROR(IDS_WRONG_THIS)
#define JS_E_PROP_DESC_MISMATCH      MAKE_JSERROR(IDS_PROP_DESC_MISMATCH)
#define JS_E_INVALID_WRITABLE_PROP_DESC MAKE_JSERROR(IDS_INVALID_WRITABLE_PROP_DESC)

static inline BOOL is_jscript_error(HRESULT hres)
{
    return HRESULT_FACILITY(hres) == FACILITY_JSCRIPT;
}

const char *debugstr_jsval(const jsval_t) DECLSPEC_HIDDEN;

HRESULT create_jscript_object(BOOL,REFIID,void**) DECLSPEC_HIDDEN;

extern LONG module_ref DECLSPEC_HIDDEN;

static inline void lock_module(void)
{
    InterlockedIncrement(&module_ref);
}

static inline void unlock_module(void)
{
    InterlockedDecrement(&module_ref);
}
