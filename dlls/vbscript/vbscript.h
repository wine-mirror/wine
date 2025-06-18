/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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
#include <stdint.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"
#include "activdbg.h"

#include "vbscript_classes.h"
#include "vbscript_defs.h"

#include "wine/list.h"

typedef struct {
    void **blocks;
    DWORD block_cnt;
    DWORD last_block;
    DWORD offset;
    BOOL mark;
    struct list custom_blocks;
} heap_pool_t;

void heap_pool_init(heap_pool_t*);
void *heap_pool_alloc(heap_pool_t*,size_t) __WINE_ALLOC_SIZE(2);
void *heap_pool_grow(heap_pool_t*,void*,DWORD,DWORD);
void heap_pool_clear(heap_pool_t*);
void heap_pool_free(heap_pool_t*);
heap_pool_t *heap_pool_mark(heap_pool_t*);

typedef struct _function_t function_t;
typedef struct _vbscode_t vbscode_t;
typedef struct _script_ctx_t script_ctx_t;
typedef struct _vbdisp_t vbdisp_t;

typedef enum {
    VBDISP_CALLGET,
    VBDISP_LET,
    VBDISP_SET,
    VBDISP_ANY
} vbdisp_invoke_type_t;

typedef struct {
    unsigned dim_cnt;
    SAFEARRAYBOUND *bounds;
} array_desc_t;

typedef struct {
    BOOL is_public;
    BOOL is_array;
    const WCHAR *name;
} vbdisp_prop_desc_t;

typedef struct {
    const WCHAR *name;
    BOOL is_public;
    BOOL is_array;
    function_t *entries[VBDISP_ANY];
} vbdisp_funcprop_desc_t;

typedef struct _class_desc_t {
    const WCHAR *name;
    script_ctx_t *ctx;

    unsigned class_initialize_id;
    unsigned class_terminate_id;
    unsigned func_cnt;
    vbdisp_funcprop_desc_t *funcs;

    unsigned prop_cnt;
    vbdisp_prop_desc_t *props;

    unsigned array_cnt;
    array_desc_t *array_descs;

    function_t *value_func;

    struct _class_desc_t *next;
} class_desc_t;

struct _vbdisp_t {
    IDispatchEx IDispatchEx_iface;

    LONG ref;
    BOOL terminator_ran;
    struct list entry;

    const class_desc_t *desc;
    SAFEARRAY **arrays;
    VARIANT props[1];
};

typedef struct _dynamic_var_t {
    struct _dynamic_var_t *next;
    VARIANT v;
    const WCHAR *name;
    BOOL is_const;
    SAFEARRAY *array;
} dynamic_var_t;

typedef struct {
    IDispatchEx IDispatchEx_iface;
    LONG ref;

    dynamic_var_t **global_vars;
    size_t global_vars_cnt;
    size_t global_vars_size;

    function_t **global_funcs;
    size_t global_funcs_cnt;
    size_t global_funcs_size;

    class_desc_t *classes;

    script_ctx_t *ctx;
    heap_pool_t heap;

    unsigned int rnd;
} ScriptDisp;

typedef struct _builtin_prop_t builtin_prop_t;

typedef struct {
    IDispatch IDispatch_iface;
    LONG ref;
    size_t member_cnt;
    const builtin_prop_t *members;
    script_ctx_t *ctx;
} BuiltinDisp;

typedef struct named_item_t {
    ScriptDisp *script_obj;
    IDispatch *disp;
    unsigned ref;
    DWORD flags;
    LPWSTR name;

    struct list entry;
} named_item_t;

HRESULT create_vbdisp(const class_desc_t*,vbdisp_t**);
HRESULT disp_get_id(IDispatch*,BSTR,vbdisp_invoke_type_t,BOOL,DISPID*);
HRESULT vbdisp_get_id(vbdisp_t*,BSTR,vbdisp_invoke_type_t,BOOL,DISPID*);
HRESULT disp_call(script_ctx_t*,IDispatch*,DISPID,DISPPARAMS*,VARIANT*);
HRESULT disp_propput(script_ctx_t*,IDispatch*,DISPID,WORD,DISPPARAMS*);
HRESULT get_disp_value(script_ctx_t*,IDispatch*,VARIANT*);
void collect_objects(script_ctx_t*);
HRESULT create_script_disp(script_ctx_t*,ScriptDisp**);

HRESULT to_int(VARIANT*,int*);

static inline unsigned arg_cnt(const DISPPARAMS *dp)
{
    return dp->cArgs - dp->cNamedArgs;
}

static inline VARIANT *get_arg(DISPPARAMS *dp, DWORD i)
{
    return dp->rgvarg + dp->cArgs-i-1;
}

#define SP_CALLER_UNINITIALIZED ((IServiceProvider*)IntToPtr(-1))

struct vbcaller {
    IServiceProvider IServiceProvider_iface;

    LONG ref;

    script_ctx_t *ctx;
    IServiceProvider *caller;
};

struct _script_ctx_t {
    IActiveScriptSite *site;
    LCID lcid;
    UINT codepage;

    IInternetHostSecurityManager *secmgr;
    struct vbcaller *vbcaller;
    DWORD safeopt;

    ScriptDisp *script_obj;

    BuiltinDisp *global_obj;
    BuiltinDisp *err_obj;

    EXCEPINFO ei;
    vbscode_t *error_loc_code;
    unsigned error_loc_offset;

    struct list objects;
    struct list code_list;
    struct list named_items;
};

HRESULT init_global(script_ctx_t*);
HRESULT init_err(script_ctx_t*);

IUnknown *create_ax_site(script_ctx_t*);

typedef enum {
    ARG_NONE = 0,
    ARG_STR,
    ARG_BSTR,
    ARG_INT,
    ARG_UINT,
    ARG_ADDR,
    ARG_DOUBLE,
    ARG_DATE
} instr_arg_type_t;

#define OP_LIST                                   \
    X(add,            1, 0,           0)          \
    X(and,            1, 0,           0)          \
    X(assign_ident,   1, ARG_BSTR,    ARG_UINT)   \
    X(assign_member,  1, ARG_BSTR,    ARG_UINT)   \
    X(bool,           1, ARG_INT,     0)          \
    X(catch,          1, ARG_ADDR,    ARG_UINT)   \
    X(case,           0, ARG_ADDR,    0)          \
    X(concat,         1, 0,           0)          \
    X(const,          1, ARG_BSTR,    0)          \
    X(date,           1, ARG_DATE,    0)          \
    X(deref,          1, 0,           0)          \
    X(dim,            1, ARG_BSTR,    ARG_UINT)   \
    X(div,            1, 0,           0)          \
    X(double,         1, ARG_DOUBLE,  0)          \
    X(empty,          1, 0,           0)          \
    X(enumnext,       0, ARG_ADDR,    ARG_BSTR)   \
    X(equal,          1, 0,           0)          \
    X(hres,           1, ARG_UINT,    0)          \
    X(errmode,        1, ARG_INT,     0)          \
    X(eqv,            1, 0,           0)          \
    X(exp,            1, 0,           0)          \
    X(gt,             1, 0,           0)          \
    X(gteq,           1, 0,           0)          \
    X(icall,          1, ARG_BSTR,    ARG_UINT)   \
    X(icallv,         1, ARG_BSTR,    ARG_UINT)   \
    X(ident,          1, ARG_BSTR,    0)          \
    X(idiv,           1, 0,           0)          \
    X(imp,            1, 0,           0)          \
    X(incc,           1, ARG_BSTR,    0)          \
    X(int,            1, ARG_INT,     0)          \
    X(is,             1, 0,           0)          \
    X(jmp,            0, ARG_ADDR,    0)          \
    X(jmp_false,      0, ARG_ADDR,    0)          \
    X(jmp_true,       0, ARG_ADDR,    0)          \
    X(lt,             1, 0,           0)          \
    X(lteq,           1, 0,           0)          \
    X(mcall,          1, ARG_BSTR,    ARG_UINT)   \
    X(mcallv,         1, ARG_BSTR,    ARG_UINT)   \
    X(me,             1, 0,           0)          \
    X(mod,            1, 0,           0)          \
    X(mul,            1, 0,           0)          \
    X(neg,            1, 0,           0)          \
    X(nequal,         1, 0,           0)          \
    X(new,            1, ARG_STR,     0)          \
    X(newenum,        1, 0,           0)          \
    X(not,            1, 0,           0)          \
    X(nothing,        1, 0,           0)          \
    X(null,           1, 0,           0)          \
    X(numval,         1, 0,           0)          \
    X(or,             1, 0,           0)          \
    X(pop,            1, ARG_UINT,    0)          \
    X(redim,          1, ARG_BSTR,    ARG_UINT)   \
    X(redim_preserve, 1, ARG_BSTR,    ARG_UINT)   \
    X(ret,            0, 0,           0)          \
    X(retval,         1, 0,           0)          \
    X(set_ident,      1, ARG_BSTR,    ARG_UINT)   \
    X(set_member,     1, ARG_BSTR,    ARG_UINT)   \
    X(stack,          1, ARG_UINT,    0)          \
    X(step,           0, ARG_ADDR,    ARG_BSTR)   \
    X(stop,           1, 0,           0)          \
    X(string,         1, ARG_STR,     0)          \
    X(sub,            1, 0,           0)          \
    X(val,            1, 0,           0)          \
    X(vcall,          1, ARG_UINT,    0)          \
    X(vcallv,         1, ARG_UINT,    0)          \
    X(xor,            1, 0,           0)

typedef enum {
#define X(x,n,a,b) OP_##x,
OP_LIST
#undef X
    OP_LAST
} vbsop_t;

typedef union {
    const WCHAR *str;
    BSTR bstr;
    unsigned uint;
    LONG lng;
    double *dbl;
    DATE *date;
} instr_arg_t;

typedef struct {
    vbsop_t op;
    unsigned loc;
    instr_arg_t arg1;
    instr_arg_t arg2;
} instr_t;

typedef struct {
    const WCHAR *name;
    BOOL by_ref;
} arg_desc_t;

typedef enum {
    FUNC_GLOBAL,
    FUNC_FUNCTION,
    FUNC_SUB,
    FUNC_PROPGET,
    FUNC_PROPLET,
    FUNC_PROPSET,
} function_type_t;

typedef struct {
    const WCHAR *name;
} var_desc_t;

struct _function_t {
    function_type_t type;
    const WCHAR *name;
    BOOL is_public;
    arg_desc_t *args;
    unsigned arg_cnt;
    var_desc_t *vars;
    unsigned var_cnt;
    array_desc_t *array_descs;
    unsigned array_cnt;
    unsigned code_off;
    vbscode_t *code_ctx;
    function_t *next;
};

struct _vbscode_t {
    instr_t *instrs;
    unsigned ref;

    WCHAR *source;
    DWORD_PTR cookie;
    unsigned start_line;

    BOOL option_explicit;

    BOOL pending_exec;
    BOOL is_persistent;
    function_t main_code;
    named_item_t *named_item;

    BSTR *bstr_pool;
    unsigned bstr_pool_size;
    unsigned bstr_cnt;
    heap_pool_t heap;

    function_t *funcs;
    class_desc_t *classes;
    class_desc_t *last_class;

    struct list entry;
};

static inline void grab_vbscode(vbscode_t *code)
{
    code->ref++;
}

void release_vbscode(vbscode_t*);
HRESULT compile_script(script_ctx_t*,const WCHAR*,const WCHAR*,const WCHAR*,DWORD_PTR,unsigned,DWORD,vbscode_t**);
HRESULT compile_procedure(script_ctx_t*,const WCHAR*,const WCHAR*,const WCHAR*,DWORD_PTR,unsigned,DWORD,class_desc_t**);
HRESULT exec_script(script_ctx_t*,BOOL,function_t*,vbdisp_t*,DISPPARAMS*,VARIANT*);
void release_dynamic_var(dynamic_var_t*);
named_item_t *lookup_named_item(script_ctx_t*,const WCHAR*,unsigned);
void release_named_item(named_item_t*);
void clear_ei(EXCEPINFO*);
HRESULT report_script_error(script_ctx_t*,const vbscode_t*,unsigned);
void detach_global_objects(script_ctx_t*);
HRESULT get_builtin_id(BuiltinDisp*,const WCHAR*,DISPID*);
HRESULT array_access(SAFEARRAY *array, DISPPARAMS *dp, VARIANT **ret);

void release_regexp_typelib(void);
HRESULT get_dispatch_typeinfo(ITypeInfo**);

static inline BOOL is_int32(double d)
{
    return INT32_MIN <= d && d <= INT32_MAX && (double)(int)d == d;
}

static inline BOOL is_digit(WCHAR c)
{
    return '0' <= c && c <= '9';
}

HRESULT create_regexp(IDispatch**);
BSTR string_replace(BSTR,BSTR,BSTR,int,int,int);

void map_vbs_exception(EXCEPINFO *);

HRESULT create_safearray_iter(SAFEARRAY *sa, BOOL owned, IEnumVARIANT **ev);

#define FACILITY_VBS 0xa
#define MAKE_VBSERROR(code) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_VBS, code)

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);
HRESULT WINAPI VBScriptRegExpFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);

BSTR get_vbscript_string(int);

#define VBSCRIPT_BUILD_VERSION 16978
#define VBSCRIPT_MAJOR_VERSION 5
#define VBSCRIPT_MINOR_VERSION 8
