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

void WINAPI LdrInitializeThunk(CONTEXT*,void**,ULONG_PTR,ULONG_PTR);

void CDECL mmap_add_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
void CDECL mmap_remove_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_is_in_reserved_area( void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
int  CDECL mmap_enum_reserved_areas( int (CDECL *enum_func)(void *base, SIZE_T size, void *arg), void *arg,
                                     int top_down ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL virtual_map_section( HANDLE handle, PVOID *addr_ptr, unsigned short zero_bits_64, SIZE_T commit_size,
                                           const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr, ULONG alloc_type,
                                           ULONG protect, pe_image_info_t *image_info ) DECLSPEC_HIDDEN;
extern void CDECL virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL virtual_create_builtin_view( void *module ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL virtual_alloc_thread_stack( INITIAL_TEB *stack, SIZE_T reserve_size, SIZE_T commit_size, SIZE_T *pthread_size ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL virtual_handle_fault( LPCVOID addr, DWORD err, BOOL on_signal_stack ) DECLSPEC_HIDDEN;
extern unsigned int CDECL virtual_locked_server_call( void *req_ptr ) DECLSPEC_HIDDEN;
extern ssize_t CDECL virtual_locked_read( int fd, void *addr, size_t size ) DECLSPEC_HIDDEN;
extern ssize_t CDECL virtual_locked_pread( int fd, void *addr, size_t size, off_t offset ) DECLSPEC_HIDDEN;
extern ssize_t CDECL virtual_locked_recvmsg( int fd, struct msghdr *hdr, int flags ) DECLSPEC_HIDDEN;
extern BOOL CDECL virtual_is_valid_code_address( const void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
extern int CDECL virtual_handle_stack_fault( void *addr ) DECLSPEC_HIDDEN;
extern BOOL CDECL virtual_check_buffer_for_read( const void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern BOOL CDECL virtual_check_buffer_for_write( void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern SIZE_T CDECL virtual_uninterrupted_read_memory( const void *addr, void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL virtual_uninterrupted_write_memory( void *addr, const void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern void CDECL virtual_set_force_exec( BOOL enable ) DECLSPEC_HIDDEN;
extern void CDECL virtual_release_address_space(void) DECLSPEC_HIDDEN;
extern void CDECL virtual_set_large_address_space(void) DECLSPEC_HIDDEN;

extern unsigned int CDECL server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                                         timeout_t abs_timeout, CONTEXT *context, RTL_CRITICAL_SECTION *cs,
                                         user_apc_t *user_apc ) DECLSPEC_HIDDEN;
extern unsigned int CDECL server_wait( const select_op_t *select_op, data_size_t size, UINT flags,
                                       const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
extern void CDECL server_send_fd( int fd ) DECLSPEC_HIDDEN;
extern int CDECL server_get_unix_fd( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                                     int *needs_close, enum server_fd_type *type,
                                     unsigned int *options ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL server_fd_to_handle( int fd, unsigned int access, unsigned int attributes,
                                           HANDLE *handle ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                                           unsigned int *options ) DECLSPEC_HIDDEN;
extern void CDECL server_release_fd( HANDLE handle, int unix_fd ) DECLSPEC_HIDDEN;
extern void CDECL server_init_process_done(void) DECLSPEC_HIDDEN;
extern TEB * CDECL init_threading( int *nb_threads_ptr, struct ldt_copy **ldt_copy, SIZE_T *size,
                                   BOOL *suspend, unsigned int *cpus, BOOL *wow64,
                                   timeout_t *start_time ) DECLSPEC_HIDDEN;
extern void CDECL DECLSPEC_NORETURN start_process( PRTL_THREAD_START_ROUTINE entry, BOOL suspend, void *relay ) DECLSPEC_HIDDEN;
extern void CDECL DECLSPEC_NORETURN abort_thread( int status ) DECLSPEC_HIDDEN;
extern void CDECL DECLSPEC_NORETURN exit_thread( int status ) DECLSPEC_HIDDEN;
extern void CDECL DECLSPEC_NORETURN exit_process( int status ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len ) DECLSPEC_HIDDEN;

extern const char *data_dir DECLSPEC_HIDDEN;
extern const char *build_dir DECLSPEC_HIDDEN;
extern const char *config_dir DECLSPEC_HIDDEN;
extern unsigned int server_cpus DECLSPEC_HIDDEN;
extern BOOL is_wow64 DECLSPEC_HIDDEN;
extern HANDLE keyed_event DECLSPEC_HIDDEN;
extern timeout_t server_start_time DECLSPEC_HIDDEN;
extern sigset_t server_block_set DECLSPEC_HIDDEN;
extern SIZE_T signal_stack_size DECLSPEC_HIDDEN;
extern SIZE_T signal_stack_mask DECLSPEC_HIDDEN;

extern unsigned int server_call_unlocked( void *req_ptr ) DECLSPEC_HIDDEN;
extern void server_enter_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern void server_leave_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern void start_server( BOOL debug ) DECLSPEC_HIDDEN;
extern void server_init_process(void) DECLSPEC_HIDDEN;
extern size_t server_init_thread( void *entry_point, BOOL *suspend ) DECLSPEC_HIDDEN;
extern int server_pipe( int fd[2] ) DECLSPEC_HIDDEN;

extern NTSTATUS context_to_server( context_t *to, const CONTEXT *from ) DECLSPEC_HIDDEN;
extern NTSTATUS context_from_server( CONTEXT *to, const context_t *from ) DECLSPEC_HIDDEN;
extern void wait_suspend( CONTEXT *context ) DECLSPEC_HIDDEN;
extern NTSTATUS set_thread_context( HANDLE handle, const context_t *context, BOOL *self ) DECLSPEC_HIDDEN;
extern NTSTATUS get_thread_context( HANDLE handle, context_t *context, unsigned int flags, BOOL *self ) DECLSPEC_HIDDEN;
extern unsigned int server_queue_process_apc( HANDLE process, const apc_call_t *call,
                                              apc_result_t *result ) DECLSPEC_HIDDEN;
extern NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
                                         data_size_t *ret_len ) DECLSPEC_HIDDEN;

extern void virtual_init(void) DECLSPEC_HIDDEN;
extern TEB *virtual_alloc_first_teb(void) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_alloc_teb( TEB **ret_teb ) DECLSPEC_HIDDEN;
extern void virtual_free_teb( TEB *teb ) DECLSPEC_HIDDEN;
extern void virtual_map_user_shared_data(void) DECLSPEC_HIDDEN;

extern void signal_init_threading(void) DECLSPEC_HIDDEN;
extern NTSTATUS signal_alloc_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_free_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_init_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry, void *arg,
                                                   BOOL suspend, void *relay, TEB *teb ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_exit_thread( int status, void (*func)(int) ) DECLSPEC_HIDDEN;

extern void dbg_init(void) DECLSPEC_HIDDEN;

#endif /* __NTDLL_UNIX_PRIVATE_H */
