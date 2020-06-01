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

#include "wine/server.h"
#include "wine/debug.h"

struct ldt_copy;

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 13

struct unix_funcs
{
    /* Nt* functions */
    NTSTATUS      (WINAPI *NtClose)( HANDLE handle );
    TEB *         (WINAPI *NtCurrentTeb)(void);
    NTSTATUS      (WINAPI *NtDuplicateObject)( HANDLE source_process, HANDLE source,
                                               HANDLE dest_process, HANDLE *dest,
                                               ACCESS_MASK access, ULONG attributes, ULONG options );
    NTSTATUS      (WINAPI *NtSetLdtEntries)( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 );

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

    /* virtual memory functions */
    NTSTATUS      (CDECL *map_so_dll)( const IMAGE_NT_HEADERS *nt_descr, HMODULE module );
    void          (CDECL *mmap_add_reserved_area)( void *addr, SIZE_T size );
    void          (CDECL *mmap_remove_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_is_in_reserved_area)( void *addr, SIZE_T size );
    int           (CDECL *mmap_enum_reserved_areas)( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg),
                                                     void *arg, int top_down );

    /* thread/process functions */
    void          (CDECL *init_threading)( int *nb_threads, struct ldt_copy **ldt_copy );
    NTSTATUS      (CDECL *alloc_thread)( TEB *teb );
    void          (CDECL *free_thread)( TEB *teb );
    void          (CDECL *init_thread)( TEB *teb );
    void          (CDECL *abort_thread)( int status );
    void          (CDECL *exit_thread)( int status );
    void          (CDECL *exit_process)( int status );
    NTSTATUS      (CDECL *get_thread_ldt_entry)( HANDLE handle, void *data, ULONG len, ULONG *ret_len );

    /* server functions */
    unsigned int  (CDECL *server_call_unlocked)( void *req_ptr );
    unsigned int  (CDECL *server_call)( void *req_ptr );
    unsigned int  (CDECL *server_select)( const select_op_t *select_op, data_size_t size, UINT flags,
                                          timeout_t abs_timeout, CONTEXT *context, RTL_CRITICAL_SECTION *cs,
                                          user_apc_t *user_apc );
    unsigned int  (CDECL *server_wait)( const select_op_t *select_op, data_size_t size, UINT flags,
                                        const LARGE_INTEGER *timeout );
    unsigned int  (CDECL *server_queue_process_apc)( HANDLE process, const apc_call_t *call, apc_result_t *result );
    void          (CDECL *server_send_fd)( int fd );
    int           (CDECL *server_get_unix_fd)( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                                               int *needs_close, enum server_fd_type *type, unsigned int *options );
    NTSTATUS      (CDECL *server_fd_to_handle)( int fd, unsigned int access, unsigned int attributes,
                                                HANDLE *handle );
    NTSTATUS      (CDECL *server_handle_to_fd)( HANDLE handle, unsigned int access, int *unix_fd,
                                                unsigned int *options );
    void          (CDECL *server_release_fd)( HANDLE handle, int unix_fd );
    int           (CDECL *server_pipe)( int fd[2] );
    void          (CDECL *server_init_process)(void);
    void          (CDECL *server_init_process_done)(void);
    size_t        (CDECL *server_init_thread)( void *entry_point, BOOL *suspend, unsigned int *cpus,
                                               BOOL *wow64, timeout_t *start_time );

    /* debugging functions */
    void          (CDECL *dbg_init)(void);
    unsigned char (CDECL *dbg_get_channel_flags)( struct __wine_debug_channel *channel );
    const char *  (CDECL *dbg_strdup)( const char *str );
    int           (CDECL *dbg_output)( const char *str );
    int           (CDECL *dbg_header)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                       const char *function );
};

#endif /* __NTDLL_UNIXLIB_H */
