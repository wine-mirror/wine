/*
 * Copyright 2026 Jacek Caban for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "wine/asm.h"

#ifdef __i386__
/* Offset of ThreadLocalStoragePointer in the TEB, the compiler uses %fs:_tls_array to access it. */
asm( ".globl " __ASM_NAME("_tls_array") "\n\t"
     __ASM_NAME("_tls_array") "=44" );
#endif

int _tls_index = 0;

__attribute__((section(".tls")))     char _tls_start = 0;
__attribute__((section(".tls$ZZZ"))) char _tls_end = 0;

__attribute__((section(".CRT$XLA"))) void *__xl_a = 0;
__attribute__((section(".CRT$XLZ"))) void *__xl_z = 0;

const struct
{
    void *StartAddressOfRawData;
    void *EndAddressOfRawData;
    void *AddressOfIndex;
    void *AddressOfCallBacks;
    ULONG SizeOfZeroFill;
    ULONG Characteristics;
} _tls_used = {
    &_tls_start,
    &_tls_end,
    &_tls_index,
    &__xl_a + 1,
};

#endif
