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

struct debug_info
{
    unsigned int str_pos;       /* current position in strings buffer */
    unsigned int out_pos;       /* current position in output buffer */
    char         strings[1024]; /* buffer for temporary strings */
    char         output[1024];  /* current output line */
};

/* thread private data, stored in NtCurrentTeb()->GdiTebBatch */
struct ntdll_thread_data
{
    struct debug_info *debug_info;    /* info for debugstr functions */
    void              *start_stack;   /* stack for thread startup */
    int                request_fd;    /* fd for sending server requests */
    int                reply_fd;      /* fd for receiving server replies */
    int                wait_fd[2];    /* fd for sleeping server requests */
    BOOL               wow64_redir;   /* Wow64 filesystem redirection flag */
    pthread_t          pthread_id;    /* pthread thread id */
};

C_ASSERT( sizeof(struct ntdll_thread_data) <= sizeof(((TEB *)0)->GdiTebBatch) );

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)&NtCurrentTeb()->GdiTebBatch;
}

void CDECL mmap_add_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
void CDECL mmap_remove_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_is_in_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_enum_reserved_areas( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg), void *arg,
                                     int top_down ) DECLSPEC_HIDDEN;

extern void virtual_init(void) DECLSPEC_HIDDEN;

extern void CDECL dbg_init(void) DECLSPEC_HIDDEN;

extern void CDECL server_send_fd( int fd ) DECLSPEC_HIDDEN;
extern int CDECL server_remove_fd_from_cache( HANDLE handle ) DECLSPEC_HIDDEN;
extern int CDECL server_get_unix_fd( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                                     int *needs_close, enum server_fd_type *type,
                                     unsigned int *options ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL server_fd_to_handle( int fd, unsigned int access, unsigned int attributes,
                                           HANDLE *handle ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                                           unsigned int *options ) DECLSPEC_HIDDEN;
extern void CDECL server_release_fd( HANDLE handle, int unix_fd ) DECLSPEC_HIDDEN;
extern int CDECL server_pipe( int fd[2] ) DECLSPEC_HIDDEN;
extern void CDECL server_init_process(void) DECLSPEC_HIDDEN;
extern void CDECL server_init_process_done(void) DECLSPEC_HIDDEN;
extern size_t CDECL server_init_thread( void *entry_point, BOOL *suspend, unsigned int *cpus,
                                        BOOL *wow64, timeout_t *start_time ) DECLSPEC_HIDDEN;

extern const char *data_dir DECLSPEC_HIDDEN;
extern const char *build_dir DECLSPEC_HIDDEN;
extern const char *config_dir DECLSPEC_HIDDEN;

extern void start_server( BOOL debug ) DECLSPEC_HIDDEN;

#endif /* __NTDLL_UNIX_PRIVATE_H */
