/*
 * Internal header for browseui.dll
 *
 * Copyright 2007 Mikolaj Zalewski
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

#ifndef __WINE_BROWSEUI_H
#define __WINE_BROWSEUI_H

extern LONG BROWSEUI_refCount DECLSPEC_HIDDEN;
extern HINSTANCE BROWSEUI_hinstance DECLSPEC_HIDDEN;

extern HRESULT ACLMulti_Constructor(IUnknown *punkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT ProgressDialog_Constructor(IUnknown *punkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT CompCatCacheDaemon_Constructor(IUnknown *punkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;
extern HRESULT ACLShellSource_Constructor(IUnknown *punkOuter, IUnknown **ppOut) DECLSPEC_HIDDEN;

extern const GUID CLSID_CompCatCacheDaemon;

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc_zero(size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void* __WINE_ALLOC_SIZE(2) heap_realloc(void *mem, size_t size)
{
    if (!mem) return heap_alloc(size);
    return HeapReAlloc(GetProcessHeap(), 0, mem, size);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#endif /* __WINE_BROWSEUI_H */
