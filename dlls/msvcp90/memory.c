/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <limits.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

/* ?address@?$allocator@D@std@@QBEPADAAD@Z */
/* ?address@?$allocator@D@std@@QEBAPEADAEAD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_address, 8)
char* __stdcall MSVCP_allocator_char_address(void *this, char *ptr)
{
    return ptr;
}

/* ?address@?$allocator@D@std@@QBEPBDABD@Z */
/* ?address@?$allocator@D@std@@QEBAPEBDAEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_const_address, 8)
const char* __stdcall MSVCP_allocator_char_const_address(void *this, const char *ptr)
{
    return ptr;
}

/* ??0?$allocator@D@std@@QAE@XZ */
/* ??0?$allocator@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_ctor, 4)
void* __stdcall MSVCP_allocator_char_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@D@std@@QAE@ABV01@@Z */
/* ??0?$allocator@D@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_copy_ctor, 8)
void* __stdcall MSVCP_allocator_char_copy_ctor(void *this, void *copy)
{
    return this;
}

/* ??4?$allocator@D@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@D@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_assign, 8);
void* __stdcall MSVCP_allocator_char_assign(void *this, void *assign)
{
    return this;
}

/* ?deallocate@?$allocator@D@std@@QAEXPADI@Z */
/* ?deallocate@?$allocator@D@std@@QEAAXPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_deallocate, 12);
void __stdcall MSVCP_allocator_char_deallocate(void *this, char *ptr, size_t size)
{
    MSVCRT_operator_delete(ptr);
}

/* ?allocate@?$allocator@D@std@@QAEPADI@Z */
/* ?allocate@?$allocator@D@std@@QEAAPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_allocate, 8);
char* __stdcall MSVCP_allocator_char_allocate(void *this, size_t count)
{
    return MSVCRT_operator_new(sizeof(char[count]));
}

/* ?allocate@?$allocator@D@std@@QAEPADIPBX@Z */
/* ?allocate@?$allocator@D@std@@QEAAPEAD_KPEBX@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_allocate_hint, 12);
char* __stdcall MSVCP_allocator_char_allocate_hint(void *this,
        size_t count, const void *hint)
{
    /* Native ignores hint */
    return MSVCP_allocator_char_allocate(this, count);
}

/* ?construct@?$allocator@D@std@@QAEXPADABD@Z */
/* ?construct@?$allocator@D@std@@QEAAXPEADAEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_construct, 12);
void __stdcall MSVCP_allocator_char_construct(void *this, char *ptr, const char *val)
{
    *ptr = *val;
}

/* ?destroy@?$allocator@D@std@@QAEXPAD@Z */
/* ?destroy@?$allocator@D@std@@QEAAXPEAD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_destroy, 8);
void __stdcall MSVCP_allocator_char_destroy(void *this, char *ptr)
{
}

/* ?max_size@?$allocator@D@std@@QBEIXZ */
/* ?max_size@?$allocator@D@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_max_size, 4);
size_t __stdcall MSVCP_allocator_char_max_size(void *this)
{
    return UINT_MAX/sizeof(char);
}
