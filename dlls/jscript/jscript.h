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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "wine/unicode.h"
#include "wine/list.h"

typedef struct _script_ctx_t script_ctx_t;
typedef struct _exec_ctx_t exec_ctx_t;
typedef struct _dispex_prop_t dispex_prop_t;

typedef struct {
    EXCEPINFO ei;
    VARIANT var;
} jsexcept_t;

typedef struct DispatchEx DispatchEx;

#define PROPF_ARGMASK 0x00ff
#define PROPF_METHOD  0x0100
#define PROPF_ENUM    0x0200
#define PROPF_CONSTR  0x0400

typedef enum {
    JSCLASS_NONE,
    JSCLASS_ARRAY,
    JSCLASS_FUNCTION,
    JSCLASS_GLOBAL,
    JSCLASS_OBJECT,
    JSCLASS_STRING
} jsclass_t;

typedef HRESULT (*builtin_invoke_t)(DispatchEx*,LCID,WORD,DISPPARAMS*,VARIANT*,jsexcept_t*,IServiceProvider*);

typedef struct {
    const WCHAR *name;
    builtin_invoke_t invoke;
    DWORD flags;
} builtin_prop_t;

typedef struct {
    jsclass_t class;
    builtin_prop_t value_prop;
    DWORD props_cnt;
    const builtin_prop_t *props;
    void (*destructor)(DispatchEx*);
    void (*on_put)(DispatchEx*,const WCHAR*);
} builtin_info_t;

struct DispatchEx {
    const IDispatchExVtbl  *lpIDispatchExVtbl;

    LONG ref;

    DWORD buf_size;
    DWORD prop_cnt;
    dispex_prop_t *props;
    script_ctx_t *ctx;

    DispatchEx *prototype;

    const builtin_info_t *builtin_info;
};

#define _IDispatchEx_(x) ((IDispatchEx*) &(x)->lpIDispatchExVtbl)

static inline void jsdisp_release(DispatchEx *jsdisp)
{
    IDispatchEx_Release(_IDispatchEx_(jsdisp));
}

HRESULT create_dispex(script_ctx_t*,const builtin_info_t*,DispatchEx*,DispatchEx**);
HRESULT init_dispex(DispatchEx*,script_ctx_t*,const builtin_info_t*,DispatchEx*);
HRESULT init_dispex_from_constr(DispatchEx*,script_ctx_t*,const builtin_info_t*,DispatchEx*);
DispatchEx *iface_to_jsdisp(IUnknown*);

HRESULT disp_call(IDispatch*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,jsexcept_t*,IServiceProvider*);
HRESULT jsdisp_call_value(DispatchEx*,LCID,WORD,DISPPARAMS*,VARIANT*,jsexcept_t*,IServiceProvider*);
HRESULT disp_propget(IDispatch*,DISPID,LCID,VARIANT*,jsexcept_t*,IServiceProvider*);
HRESULT disp_propput(IDispatch*,DISPID,LCID,VARIANT*,jsexcept_t*,IServiceProvider*);
HRESULT jsdisp_propput_name(DispatchEx*,const WCHAR*,LCID,VARIANT*,jsexcept_t*,IServiceProvider*);
HRESULT jsdisp_set_prototype(DispatchEx*,DispatchEx*);

HRESULT create_builtin_function(script_ctx_t*,builtin_invoke_t,DWORD,DispatchEx*,DispatchEx**);

HRESULT to_boolean(VARIANT*,VARIANT_BOOL*);
HRESULT to_object(exec_ctx_t*,VARIANT*,IDispatch**);

typedef struct named_item_t {
    IDispatch *disp;
    DWORD flags;

    struct named_item_t *next;
} named_item_t;

struct _script_ctx_t {
    LONG ref;

    SCRIPTSTATE state;
    exec_ctx_t *exec_ctx;
    named_item_t *named_items;
    LCID lcid;

    DispatchEx *script_disp;
    DispatchEx *global;
    DispatchEx *array_constr;
    DispatchEx *object_constr;
    DispatchEx *string_constr;
};

void script_release(script_ctx_t*);

static inline void script_addref(script_ctx_t *ctx)
{
    ctx->ref++;
}

HRESULT init_global(script_ctx_t*);

HRESULT create_array_constr(script_ctx_t*,DispatchEx**);
HRESULT create_object_constr(script_ctx_t*,DispatchEx**);
HRESULT create_string_constr(script_ctx_t*,DispatchEx**);

const char *debugstr_variant(const VARIANT*);

HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);

typedef struct {
    void **blocks;
    DWORD block_cnt;
    DWORD last_block;
    DWORD offset;
    struct list custom_blocks;
} jsheap_t;

void jsheap_init(jsheap_t*);
void *jsheap_alloc(jsheap_t*,DWORD);
void jsheap_clear(jsheap_t*);
void jsheap_free(jsheap_t*);

extern LONG module_ref;

static inline void lock_module(void)
{
    InterlockedIncrement(&module_ref);
}

static inline void unlock_module(void)
{
    InterlockedDecrement(&module_ref);
}

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
        memcpy(ret, str, size);
    }

    return ret;
}

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))
