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

#include "wine/debug.h"

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 7

struct unix_funcs
{
    /* environment functions */
    void          (CDECL *get_main_args)( int *argc, char **argv[], char **envp[] );
    void          (CDECL *get_paths)( const char **builddir, const char **datadir, const char **configdir );
    void          (CDECL *get_dll_path)( const char ***paths, SIZE_T *maxlen );
    void          (CDECL *get_unix_codepage)( CPTABLEINFO *table );
    const char *  (CDECL *get_version)(void);
    const char *  (CDECL *get_build_id)(void);
    void          (CDECL *get_host_version)( const char **sysname, const char **release );

    /* loader functions */
    NTSTATUS      (CDECL *exec_wineloader)( char **argv, int socketfd, int is_child_64bit,
                                            ULONGLONG res_start, ULONGLONG res_end );
    void          (CDECL *start_server)( BOOL debug );

    /* virtual memory functions */
    NTSTATUS      (CDECL *map_so_dll)( const IMAGE_NT_HEADERS *nt_descr, HMODULE module );
    void          (CDECL *mmap_add_reserved_area)( void *addr, SIZE_T size );
    void          (CDECL *mmap_remove_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_is_in_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_enum_reserved_areas)( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg),
                                                     void *arg, int top_down );

    /* debugging functions */
    void          (CDECL *dbg_init)(void);
    unsigned char (CDECL *dbg_get_channel_flags)( struct __wine_debug_channel *channel );
    const char *  (CDECL *dbg_strdup)( const char *str );
    int           (CDECL *dbg_output)( const char *str );
    int           (CDECL *dbg_header)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                       const char *function );
};

#endif /* __NTDLL_UNIXLIB_H */
