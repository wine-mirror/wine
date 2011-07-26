/*
 * Header includes for shdocvw.dll
 *
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"

#include "wine/unicode.h"

HRESULT WINAPI CUrlHistory_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI InternetShortcut_Create(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

extern LONG module_ref DECLSPEC_HIDDEN;

static inline void lock_module(void) {
    InterlockedIncrement(&module_ref);
}

static inline void unlock_module(void) {
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

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR co_strdupW(LPCWSTR str)
{
    WCHAR *ret = CoTaskMemAlloc((strlenW(str) + 1)*sizeof(WCHAR));
    if (ret)
        lstrcpyW(ret, str);
    return ret;
}

static inline LPWSTR co_strdupAtoW(LPCSTR str)
{
    INT len;
    WCHAR *ret;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = CoTaskMemAlloc(len*sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static inline LPSTR co_strdupWtoA(LPCWSTR str)
{
    INT len;
    CHAR *ret;
    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, 0, 0);
    ret = CoTaskMemAlloc(len);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, 0, 0);
    return ret;
}
