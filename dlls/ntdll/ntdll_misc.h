/*
 * Copyright 2000 Juergen Schmied
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

#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include <stdarg.h>
#include <signal.h>

#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/server.h"

#define MAX_NT_PATH_LENGTH 277

/* exceptions */
extern void wait_suspend( CONTEXT *context );
extern void WINAPI __regs_RtlRaiseException( PEXCEPTION_RECORD, PCONTEXT );
extern void get_cpu_context( CONTEXT *context );
extern void set_cpu_context( const CONTEXT *context );

/* debug helper */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);

extern NTSTATUS NTDLL_queue_process_apc( HANDLE process, const apc_call_t *call, apc_result_t *result );
extern NTSTATUS NTDLL_wait_for_multiple_objects( UINT count, const HANDLE *handles, UINT flags,
                                                 const LARGE_INTEGER *timeout, HANDLE signal_object );

/* init routines */
extern BOOL SIGNAL_Init(void);
extern size_t get_signal_stack_total_size(void);
extern void version_init( const WCHAR *appname );
extern void debug_init(void);
extern HANDLE thread_init(void);
extern void virtual_init(void);
extern void virtual_init_threading(void);

/* server support */
extern timeout_t server_start_time;
extern void server_init_process(void);
extern NTSTATUS server_init_process_done(void);
extern size_t server_init_thread( int unix_pid, int unix_tid, void *entry_point );
extern void DECLSPEC_NORETURN server_protocol_error( const char *err, ... );
extern void DECLSPEC_NORETURN server_protocol_perror( const char *err );
extern void DECLSPEC_NORETURN server_exit_thread( int status );
extern void DECLSPEC_NORETURN server_abort_thread( int status );
extern sigset_t server_block_set;
extern void server_enter_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset );
extern void server_leave_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset );
extern int server_remove_fd_from_cache( obj_handle_t handle );
extern int server_get_unix_fd( obj_handle_t handle, unsigned int access, int *unix_fd,
                               int *needs_close, enum server_fd_type *type, unsigned int *options );

/* module handling */
extern NTSTATUS MODULE_DllThreadAttach( LPVOID lpReserved );
extern FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user );
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal, const WCHAR *user );
extern void RELAY_SetupDLL( HMODULE hmod );
extern void SNOOP_SetupDLL( HMODULE hmod );
extern UNICODE_STRING windows_dir;
extern UNICODE_STRING system_dir;

/* redefine these to make sure we don't reference kernel symbols */
#define GetProcessHeap()       (NtCurrentTeb()->Peb->ProcessHeap)
#define GetCurrentProcessId()  (HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess))
#define GetCurrentThreadId()   (HandleToULong(NtCurrentTeb()->ClientId.UniqueThread))

/* Device IO */
extern NTSTATUS CDROM_DeviceIoControl(HANDLE hDevice, 
                                      HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                      PVOID UserApcContext, 
                                      PIO_STATUS_BLOCK piosb, 
                                      ULONG IoControlCode,
                                      LPVOID lpInBuffer, DWORD nInBufferSize,
                                      LPVOID lpOutBuffer, DWORD nOutBufferSize);
extern NTSTATUS COMM_DeviceIoControl(HANDLE hDevice, 
                                     HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                     PVOID UserApcContext, 
                                     PIO_STATUS_BLOCK piosb, 
                                     ULONG IoControlCode,
                                     LPVOID lpInBuffer, DWORD nInBufferSize,
                                     LPVOID lpOutBuffer, DWORD nOutBufferSize);
extern NTSTATUS TAPE_DeviceIoControl(HANDLE hDevice, 
                                     HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                     PVOID UserApcContext, 
                                     PIO_STATUS_BLOCK piosb, 
                                     ULONG IoControlCode,
                                     LPVOID lpInBuffer, DWORD nInBufferSize,
                                     LPVOID lpOutBuffer, DWORD nOutBufferSize);

/* file I/O */
extern NTSTATUS FILE_GetNtStatus(void);
extern BOOL DIR_is_hidden_file( const UNICODE_STRING *name );
extern NTSTATUS DIR_unmount_device( HANDLE handle );
extern NTSTATUS DIR_get_unix_cwd( char **cwd );

/* virtual memory */
extern NTSTATUS VIRTUAL_HandleFault(LPCVOID addr);
extern void VIRTUAL_SetForceExec( BOOL enable );
extern void VIRTUAL_UseLargeAddressSpace(void);
extern struct _KUSER_SHARED_DATA *user_shared_data;

/* code pages */
extern int ntdll_umbstowcs(DWORD flags, const char* src, int srclen, WCHAR* dst, int dstlen);
extern int ntdll_wcstoumbs(DWORD flags, const WCHAR* src, int srclen, char* dst, int dstlen,
                           const char* defchar, int *used );

/* load order */

enum loadorder
{
    LO_INVALID,
    LO_DISABLED,
    LO_NATIVE,
    LO_BUILTIN,
    LO_NATIVE_BUILTIN,  /* native then builtin */
    LO_BUILTIN_NATIVE,  /* builtin then native */
    LO_DEFAULT          /* nothing specified, use default strategy */
};

extern enum loadorder get_load_order( const WCHAR *app_name, const WCHAR *path );

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

/* thread private data, stored in NtCurrentTeb()->SystemReserved2 */
struct ntdll_thread_data
{
    DWORD              fs;            /* 1d4 TEB selector */
    DWORD              gs;            /* 1d8 libc selector; update winebuild if you move this! */
    struct debug_info *debug_info;    /* 1dc info for debugstr functions */
    int                request_fd;    /* 1e0 fd for sending server requests */
    int                reply_fd;      /* 1e4 fd for receiving server replies */
    int                wait_fd[2];    /* 1e8 fd for sleeping server requests */
    void              *vm86_ptr;      /* 1f0 data for vm86 mode */

    void              *pad[2];        /* 1f4 change this if you add fields! */
};

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)NtCurrentTeb()->SystemReserved2;
}

/* thread debug_registers, stored in NtCurrentTeb()->SpareBytes1 */
struct ntdll_thread_regs
{
    DWORD dr0;
    DWORD dr1;
    DWORD dr2;
    DWORD dr3;
    DWORD dr6;
    DWORD dr7;
};

static inline struct ntdll_thread_regs *ntdll_get_thread_regs(void)
{
    return (struct ntdll_thread_regs *)NtCurrentTeb()->SpareBytes1;
}

#endif
