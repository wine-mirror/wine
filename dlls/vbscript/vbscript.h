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

typedef struct {
    IActiveScriptSite *site;
    LCID lcid;

    IDispatch *host_global;

    vbdisp_t *script_obj;

    struct list named_items;
} script_ctx_t;

HRESULT init_global(script_ctx_t*);

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
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
