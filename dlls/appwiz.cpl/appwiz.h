/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include "wine/heap.h"
#include "winnls.h"

typedef enum {
    ADDON_GECKO,
    ADDON_MONO
} addon_t;

BOOL install_addon(addon_t) DECLSPEC_HIDDEN;

extern HINSTANCE hInst DECLSPEC_HIDDEN;

static inline WCHAR *heap_strdupW(const WCHAR *str)
{
    WCHAR *ret;

    if(str) {
        size_t size = lstrlenW(str)+1;
        ret = heap_alloc(size*sizeof(WCHAR));
        if(ret)
            memcpy(ret, str, size*sizeof(WCHAR));
    }else {
        ret = NULL;
    }

    return ret;
}

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    WCHAR *ret = NULL;

    if(str) {
        size_t len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}
