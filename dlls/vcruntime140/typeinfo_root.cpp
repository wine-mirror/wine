/*
 * Copyright 2026 Jacek Caban for CodeWeavers
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

#if 0
#pragma makedep implib
#endif

#ifdef _MSC_VER

#include "windef.h"
#include "winnt.h"
#include "wine/asm.h"

struct __type_info_node
{
    SLIST_HEADER list;
};

__type_info_node __type_info_root_node = {};

extern "C" void __cdecl __std_type_info_destroy_list(SLIST_HEADER*);

extern "C" void __cdecl __wine_destroy_type_info_list(void)
{
    __std_type_info_destroy_list(&__type_info_root_node.list);
}

__ASM_SECTION_POINTER( ".section .CRT$XTY", __wine_destroy_type_info_list )

#endif
