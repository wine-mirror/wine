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

#include "msvcp.h"

#include "windef.h"
#include "winbase.h"

/* ?deallocate@?$allocator@D@std@@QAEXPADI@Z */
/* ?deallocate@?$allocator@D@std@@QEAAXPEAD_K@Z */
void MSVCP_allocator_char_deallocate(void *this, char *ptr, MSVCP_size_t size)
{
    MSVCRT_operator_delete(ptr);
}

/* ?allocate@?$allocator@D@std@@QAEPADI@Z */
/* ?allocate@?$allocator@D@std@@QEAAPEAD_K@Z */
char* MSVCP_allocator_char_allocate(void *this, MSVCP_size_t count)
{
    return MSVCRT_operator_new(count);
}

/* ?max_size@?$allocator@D@std@@QBEIXZ */
/* ?max_size@?$allocator@D@std@@QEBA_KXZ */
MSVCP_size_t MSVCP_allocator_char_max_size(void *this)
{
    return UINT_MAX/sizeof(char);
}


/* allocator<wchar_t> */
/* ?deallocate@?$allocator@_W@std@@QAEXPA_WI@Z */
/* ?deallocate@?$allocator@_W@std@@QEAAXPEA_W_K@Z */
void MSVCP_allocator_wchar_deallocate(void *this,
        wchar_t *ptr, MSVCP_size_t size)
{
    MSVCRT_operator_delete(ptr);
}

/* ?allocate@?$allocator@_W@std@@QAEPA_WI@Z */
/* ?allocate@?$allocator@_W@std@@QEAAPEA_W_K@Z */
wchar_t* MSVCP_allocator_wchar_allocate(void *this, MSVCP_size_t count)
{
    if(UINT_MAX/count < sizeof(wchar_t)) {
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
        return NULL;
    }

    return MSVCRT_operator_new(count * sizeof(wchar_t));
}

/* ?max_size@?$allocator@_W@std@@QBEIXZ */
/* ?max_size@?$allocator@_W@std@@QEBA_KXZ */
MSVCP_size_t MSVCP_allocator_wchar_max_size(void *this)
{
    return UINT_MAX/sizeof(wchar_t);
}

/* allocator<void> */
/* ??0?$allocator@X@std@@QAE@XZ */
/* ??0?$allocator@X@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_ctor, 4)
void* __thiscall MSVCP_allocator_void_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@X@std@@QAE@ABV01@@Z */
/* ??0?$allocator@X@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_copy_ctor, 8)
void* __thiscall MSVCP_allocator_void_copy_ctor(void *this, void *copy)
{
    return this;
}

/* ??4?$allocator@X@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@X@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_assign, 8)
void* __thiscall MSVCP_allocator_void_assign(void *this, void *assign)
{
    return this;
}
