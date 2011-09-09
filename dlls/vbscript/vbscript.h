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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "vbscript_classes.h"

#include "wine/list.h"
#include "wine/unicode.h"

typedef struct _function_t function_t;
typedef struct _vbscode_t vbscode_t;
typedef struct _script_ctx_t script_ctx_t;

typedef struct named_item_t {
    IDispatch *disp;
    DWORD flags;
    LPWSTR name;

    struct list entry;
} named_item_t;

typedef struct {
    IDispatchEx IDispatchEx_iface;

    LONG ref;
} vbdisp_t;

HRESULT disp_get_id(IDispatch*,BSTR,DISPID*);
HRESULT disp_call(script_ctx_t*,IDispatch*,DISPID,DISPPARAMS*,VARIANT*);

struct _script_ctx_t {
    IActiveScriptSite *site;
    LCID lcid;

    IDispatch *host_global;

    vbdisp_t *script_obj;

    struct list code_list;
    struct list named_items;
};

HRESULT init_global(script_ctx_t*);

typedef enum {
    ARG_NONE = 0,
    ARG_STR,
    ARG_BSTR,
    ARG_INT,
    ARG_UINT
} instr_arg_type_t;

#define OP_LIST                                   \
    X(bool,           1, ARG_INT,     0)          \
    X(icallv,         1, ARG_BSTR,    ARG_UINT)   \
    X(ret,            0, 0,           0)          \
    X(string,         1, ARG_STR,     0)

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
} instr_arg_t;

typedef struct {
    vbsop_t op;
    instr_arg_t arg1;
    instr_arg_t arg2;
} instr_t;

struct _function_t {
    unsigned code_off;
    vbscode_t *code_ctx;
};

struct _vbscode_t {
    instr_t *instrs;
    WCHAR *source;

    BOOL option_explicit;

    BOOL global_executed;
    function_t global_code;

    BSTR *bstr_pool;
    unsigned bstr_pool_size;
    unsigned bstr_cnt;

    struct list entry;
};

void release_vbscode(vbscode_t*) DECLSPEC_HIDDEN;
HRESULT compile_script(script_ctx_t*,const WCHAR*,vbscode_t**) DECLSPEC_HIDDEN;
HRESULT exec_script(script_ctx_t*,function_t*) DECLSPEC_HIDDEN;

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}
