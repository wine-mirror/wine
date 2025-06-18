/*
 * Copyright 2008,2011 Jacek Caban for CodeWeavers
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

#define OP_LIST                            \
    X(add,        1, 0,0)                  \
    X(and,        1, 0,0)                  \
    X(array,      1, 0,0)                  \
    X(assign,     1, 0,0)                  \
    X(assign_call,1, ARG_UINT,   0)        \
    X(bool,       1, ARG_INT,    0)        \
    X(bneg,       1, 0,0)                  \
    X(call,       1, ARG_UINT,   ARG_UINT) \
    X(call_eval,  1, ARG_UINT,   ARG_UINT) \
    X(call_member,1, ARG_UINT,   ARG_UINT) \
    X(carray,     1, ARG_UINT,   0)        \
    X(carray_set, 1, ARG_UINT,   0)        \
    X(case,       0, ARG_ADDR,   0)        \
    X(cnd_nz,     0, ARG_ADDR,   0)        \
    X(cnd_z,      0, ARG_ADDR,   0)        \
    X(delete,     1, 0,0)                  \
    X(delete_ident,1,ARG_BSTR,   0)        \
    X(div,        1, 0,0)                  \
    X(double,     1, ARG_DBL,    0)        \
    X(end_finally,0, 0,0)                  \
    X(enter_catch,1, ARG_BSTR,   0)        \
    X(eq,         1, 0,0)                  \
    X(eq2,        1, 0,0)                  \
    X(forin,      0, ARG_ADDR,   0)        \
    X(func,       1, ARG_UINT,   0)        \
    X(gt,         1, 0,0)                  \
    X(gteq,       1, 0,0)                  \
    X(ident,      1, ARG_BSTR,   0)        \
    X(identid,    1, ARG_BSTR,   ARG_INT)  \
    X(in,         1, 0,0)                  \
    X(instanceof, 1, 0,0)                  \
    X(int,        1, ARG_INT,    0)        \
    X(jmp,        0, ARG_ADDR,   0)        \
    X(jmp_z,      0, ARG_ADDR,   0)        \
    X(local,      1, ARG_INT,    0)        \
    X(local_ref,  1, ARG_INT,    ARG_UINT) \
    X(lshift,     1, 0,0)                  \
    X(lt,         1, 0,0)                  \
    X(lteq,       1, 0,0)                  \
    X(member,     1, ARG_BSTR,   0)        \
    X(memberid,   1, ARG_UINT,   0)        \
    X(minus,      1, 0,0)                  \
    X(mod,        1, 0,0)                  \
    X(mul,        1, 0,0)                  \
    X(neg,        1, 0,0)                  \
    X(neq,        1, 0,0)                  \
    X(neq2,       1, 0,0)                  \
    X(new,        1, ARG_UINT,   0)        \
    X(new_obj,    1, 0,0)                  \
    X(null,       1, 0,0)                  \
    X(obj_prop,   1, ARG_STR,    ARG_UINT) \
    X(or,         1, 0,0)                  \
    X(pop,        1, ARG_UINT,   0)        \
    X(pop_except, 0, ARG_ADDR,   0)        \
    X(pop_scope,  1, 0,0)                  \
    X(postinc,    1, ARG_INT,    0)        \
    X(preinc,     1, ARG_INT,    0)        \
    X(push_acc,   1, 0,0)                  \
    X(push_except,1, ARG_ADDR,   ARG_UINT) \
    X(push_block_scope, 1, ARG_UINT, 0)    \
    X(push_with_scope,  1, 0,0)            \
    X(regexp,     1, ARG_STR,    ARG_UINT) \
    X(rshift,     1, 0,0)                  \
    X(rshift2,    1, 0,0)                  \
    X(str,        1, ARG_STR,    0)        \
    X(this,       1, 0,0)                  \
    X(throw,      0, 0,0)                  \
    X(throw_ref,  0, ARG_UINT,   0)        \
    X(throw_type, 0, ARG_UINT,   ARG_STR)  \
    X(tonum,      1, 0,0)                  \
    X(typeof,     1, 0,0)                  \
    X(typeofid,   1, 0,0)                  \
    X(typeofident,1, 0,0)                  \
    X(refval,     1, 0,0)                  \
    X(ret,        0, ARG_UINT,   0)        \
    X(set_member, 1, 0,0)                  \
    X(setret,     1, 0,0)                  \
    X(sub,        1, 0,0)                  \
    X(to_string,  1, 0,0)                  \
    X(undefined,  1, 0,0)                  \
    X(void,       1, 0,0)                  \
    X(xor,        1, 0,0)

typedef enum {
#define X(x,a,b,c) OP_##x,
OP_LIST
#undef X
    OP_LAST
} jsop_t;

typedef struct _bytecode_t bytecode_t;

typedef union {
    BSTR bstr;
    LONG lng;
    jsstr_t *str;
    unsigned uint;
} instr_arg_t;

typedef enum {
    ARG_NONE = 0,
    ARG_ADDR,
    ARG_BSTR,
    ARG_DBL,
    ARG_FUNC,
    ARG_INT,
    ARG_STR,
    ARG_UINT
} instr_arg_type_t;

typedef struct {
    jsop_t op;
    unsigned loc;
    union {
        instr_arg_t arg[2];
        double dbl;
    } u;
} instr_t;

typedef enum {
    PROPERTY_DEFINITION_VALUE,
    PROPERTY_DEFINITION_GETTER,
    PROPERTY_DEFINITION_SETTER
} property_definition_type_t;

typedef struct {
    BSTR name;
    int ref;
} local_ref_t;

#define INVALID_LOCAL_REF 0x7fffffff

typedef struct {
    unsigned locals_cnt;
    local_ref_t *locals;
} local_ref_scopes_t;

typedef struct _function_code_t {
    BSTR name;
    int local_ref;
    BSTR event_target;
    unsigned instr_off;

    const WCHAR *source;
    unsigned source_len;

    unsigned func_cnt;
    struct _function_code_t *funcs;

    unsigned var_cnt;
    struct {
        BSTR name;
        int func_id; /* -1 if not a function */
    } *variables;

    unsigned param_cnt;
    BSTR *params;

    local_ref_scopes_t *local_scopes;
    unsigned local_scope_count;

    unsigned int scope_index; /* index of scope in the parent function where the function is defined */

    bytecode_t *bytecode;
} function_code_t;

IDispatch *lookup_global_host(script_ctx_t*);
local_ref_t *lookup_local(const function_code_t*,const WCHAR*,unsigned int);

struct _bytecode_t {
    LONG ref;
    BOOL is_persistent;

    instr_t *instrs;
    heap_pool_t heap;

    function_code_t global_code;
    named_item_t *named_item;

    WCHAR *source;
    UINT64 source_context;
    unsigned start_line;

    BSTR *bstr_pool;
    unsigned bstr_pool_size;
    unsigned bstr_cnt;

    jsstr_t **str_pool;
    unsigned str_pool_size;
    unsigned str_cnt;

    struct list entry;
};

HRESULT compile_script(script_ctx_t*,const WCHAR*,UINT64,unsigned,const WCHAR*,const WCHAR*,BOOL,BOOL,named_item_t*,bytecode_t**);
void release_bytecode(bytecode_t*);

unsigned get_location_line(bytecode_t *code, unsigned loc, unsigned *char_pos);

static inline bytecode_t *bytecode_addref(bytecode_t *code)
{
    code->ref++;
    return code;
}

struct vars_buffer {
    function_code_t *func_code;
    unsigned argc;
    jsval_t var[];
};

typedef struct _scope_chain_t {
    jsdisp_t dispex;
    IDispatch *obj;
    unsigned int scope_index;
    struct vars_buffer *detached_vars;
    struct _call_frame_t *frame;
    struct _scope_chain_t *next;
} scope_chain_t;

static inline scope_chain_t *scope_addref(scope_chain_t *scope)
{
    jsdisp_addref(&scope->dispex);
    return scope;
}

static inline void scope_release(scope_chain_t *scope)
{
    jsdisp_release(&scope->dispex);
}

struct _jsexcept_t {
    HRESULT error;

    BOOL valid_value;
    jsval_t value;

    jsstr_t *source;
    jsstr_t *message;
    jsstr_t *line;

    bytecode_t *code;
    unsigned loc;

    BOOL enter_notified;
    jsexcept_t *prev;
};

void enter_script(script_ctx_t*,jsexcept_t*);
HRESULT leave_script(script_ctx_t*,HRESULT);
void reset_ei(jsexcept_t*);
void set_error_location(jsexcept_t*,bytecode_t*,unsigned,unsigned,jsstr_t*);

typedef struct _except_frame_t except_frame_t;
struct _parser_ctx_t;

typedef struct _call_frame_t {
    unsigned ip;
    except_frame_t *except_frame;
    unsigned stack_base;
    scope_chain_t *scope;
    scope_chain_t *base_scope;

    jsval_t ret;

    IDispatch *this_obj;
    jsdisp_t *function_instance;
    jsdisp_t *variable_obj;
    jsdisp_t *arguments_obj;
    DWORD flags;

    unsigned argc;
    unsigned pop_locals;
    unsigned arguments_off;
    unsigned variables_off;
    unsigned pop_variables;

    bytecode_t *bytecode;
    function_code_t *function;

    struct _call_frame_t *prev_frame;
} call_frame_t;

#define EXEC_GLOBAL            0x0001
#define EXEC_CONSTRUCTOR       0x0002
#define EXEC_RETURN_TO_INTERP  0x0004
#define EXEC_EVAL              0x0008

HRESULT exec_source(script_ctx_t*,DWORD,bytecode_t*,function_code_t*,scope_chain_t*,IDispatch*,
        jsdisp_t*,unsigned,jsval_t*,jsval_t*);

HRESULT create_source_function(script_ctx_t*,bytecode_t*,function_code_t*,scope_chain_t*,jsdisp_t**);
HRESULT setup_arguments_object(script_ctx_t*,call_frame_t*);
void detach_arguments_object(call_frame_t*);
