/*
 * Ntdll Unix private interface
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#ifndef __NTDLL_UNIX_PRIVATE_H
#define __NTDLL_UNIX_PRIVATE_H

#include "unixlib.h"

void CDECL mmap_add_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
void CDECL mmap_remove_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_is_in_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_enum_reserved_areas( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg), void *arg,
                                     int top_down ) DECLSPEC_HIDDEN;

extern void virtual_init(void) DECLSPEC_HIDDEN;

extern void CDECL dbg_init(void) DECLSPEC_HIDDEN;

#endif /* __NTDLL_UNIX_PRIVATE_H */
