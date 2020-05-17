/*
 * Ntdll Unix interface
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

#ifndef __NTDLL_UNIXLIB_H
#define __NTDLL_UNIXLIB_H

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 2

struct unix_funcs
{
    /* virtual memory functions */
    NTSTATUS      (CDECL *map_so_dll)( const IMAGE_NT_HEADERS *nt_descr, HMODULE module );
    void          (CDECL *mmap_add_reserved_area)( void *addr, SIZE_T size );
    void          (CDECL *mmap_remove_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_is_in_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_enum_reserved_areas)( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg),
                                                     void *arg, int top_down );
};

#endif /* __NTDLL_UNIXLIB_H */
