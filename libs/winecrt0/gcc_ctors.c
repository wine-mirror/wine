/*
 * Copyright 2026 Yuxuan Shui for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public License,
 * the authors give you unlimited permission to link the compiled version
 * of this file with other programs, and to distribute those programs
 * without any restriction coming from the use of this file.  (The GNU
 * Lesser General Public License restrictions do apply in other respects;
 * for example, they cover modification of the file, and distribution when
 * not linked into another program.)
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

#ifdef __WINE_PE_BUILD

#include <stdarg.h>
#include <process.h>
#include "windef.h"
#include "winbase.h"
#include "wine/asm.h"

extern _PVFV __CTOR_LIST__[];
extern _PVFV __DTOR_LIST__[];

void __cdecl __wine_call_gcc_ctors(void)
{
    ULONG_PTR n = (ULONG_PTR)__CTOR_LIST__[0], i;
    if (n == (ULONG_PTR)-1) for (n = 0; __CTOR_LIST__[n + 1]; n++);
    for (i = n; i >= 1; i--) __CTOR_LIST__[i]();
}

__ASM_SECTION_POINTER( ".section .CRT$XCB", __wine_call_gcc_ctors )

void __cdecl __wine_call_gcc_dtors(void)
{
    size_t i;
    for (i = 1; __DTOR_LIST__[i]; i++) __DTOR_LIST__[i]();
}

__ASM_SECTION_POINTER( ".section .CRT$XTB", __wine_call_gcc_dtors )

#endif
