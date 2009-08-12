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
#include <sys/types.h>
#include <pthread.h>

#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/server.h"

#define MAX_NT_PATH_LENGTH 277

#define MAX_DOS_DRIVES 26

struct drive_info
{
    dev_t dev;
    ino_t ino;
};

/* exceptions */
extern void wait_suspend( CONTEXT *context );
extern NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, int first_chance, CONTEXT *context );
extern LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context );
extern void raise_status( NTSTATUS status, EXCEPTION_RECORD *rec ) DECLSPEC_NORETURN;
extern void set_cpu_context( const CONTEXT *context );
extern void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags );
extern NTSTATUS context_to_server( context_t *to, const CONTEXT *from );
extern NTSTATUS context_from_server( CONTEXT *to, const context_t *from );
extern void call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg ) DECLSPEC_NORETURN;

/* debug helpers */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern LPCSTR debugstr_ObjectAttributes(const OBJECT_ATTRIBUTES *oa);

extern NTSTATUS NTDLL_queue_process_apc( HANDLE process, const apc_call_t *call, apc_result_t *result );
extern NTSTATUS NTDLL_wait_for_multiple_objects( UINT count, const HANDLE *handles, UINT flags,
                                                 const LARGE_INTEGER *timeout, HANDLE signal_object );

/* init routines */
extern void signal_init_thread( TEB *teb );
extern void signal_init_process(void);
extern size_t get_signal_stack_total_size(void);
extern void version_init( const WCHAR *appname );
extern void debug_init(void);
extern HANDLE thread_init(void);
extern void actctx_init(void);
extern void virtual_init(void);
extern void virtual_init_threading(void);

/* server support */
extern timeout_t server_start_time;
extern unsigned int server_cpus;
extern int is_wow64;
extern void server_init_process(void);
extern NTSTATUS server_init_process_done(void);
extern size_t server_init_thread( void *entry_point );
extern void DECLSPEC_NORETURN server_protocol_error( const char *err, ... );
extern void DECLSPEC_NORETURN server_protocol_perror( const char *err );
extern void DECLSPEC_NORETURN abort_thread( int status );
extern void DECLSPEC_NORETURN exit_thread( int status );
extern sigset_t server_block_set;
extern void server_enter_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset );
extern void server_leave_uninterrupted_section( RTL_CRITICAL_SECTION *cs, sigset_t *sigset );
extern int server_remove_fd_from_cache( HANDLE handle );
extern int server_get_unix_fd( HANDLE handle, unsigned int access, int *unix_fd,
                               int *needs_close, enum server_fd_type *type, unsigned int *options );
extern int server_pipe( int fd[2] );

/* security descriptors */
NTSTATUS NTDLL_create_struct_sd(PSECURITY_DESCRIPTOR nt_sd, struct security_descriptor **server_sd,
                                data_size_t *server_sd_len);
void NTDLL_free_struct_sd(struct security_descriptor *server_sd);

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

typedef LONG (WINAPI *PUNHANDLED_EXCEPTION_FILTER)(PEXCEPTION_POINTERS);
extern PUNHANDLED_EXCEPTION_FILTER unhandled_exception_filter;

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
extern unsigned int DIR_get_drives_info( struct drive_info info[MAX_DOS_DRIVES] );

/* virtual memory */
extern void virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info );
extern NTSTATUS virtual_create_system_view( void *base, SIZE_T size, DWORD vprot );
extern NTSTATUS virtual_alloc_thread_stack( TEB *teb, SIZE_T reserve_size, SIZE_T commit_size );
extern void virtual_clear_thread_stack(void);
extern BOOL virtual_handle_stack_fault( void *addr );
extern NTSTATUS virtual_handle_fault( LPCVOID addr, DWORD err );
extern BOOL virtual_check_buffer_for_read( const void *ptr, SIZE_T size );
extern BOOL virtual_check_buffer_for_write( void *ptr, SIZE_T size );
extern void VIRTUAL_SetForceExec( BOOL enable );
extern void virtual_release_address_space( BOOL free_high_mem );
extern struct _KUSER_SHARED_DATA *user_shared_data;

/* completion */
extern NTSTATUS NTDLL_AddCompletion( HANDLE hFile, ULONG_PTR CompletionValue,
                                     NTSTATUS CompletionStatus, ULONG Information );

/* code pages */
extern int ntdll_umbstowcs(DWORD flags, const char* src, int srclen, WCHAR* dst, int dstlen);
extern int ntdll_wcstoumbs(DWORD flags, const WCHAR* src, int srclen, char* dst, int dstlen,
                           const char* defchar, int *used );

extern int CDECL NTDLL__vsnprintf( char *str, SIZE_T len, const char *format, __ms_va_list args );
extern int CDECL NTDLL__vsnwprintf( WCHAR *str, SIZE_T len, const WCHAR *format, __ms_va_list args );

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
    DWORD              fs;            /* 1d4/300 TEB selector */
    DWORD              gs;            /* 1d8/304 libc selector; update winebuild if you move this! */
    struct debug_info *debug_info;    /* 1dc/308 info for debugstr functions */
    int                request_fd;    /* 1e0/310 fd for sending server requests */
    int                reply_fd;      /* 1e4/314 fd for receiving server replies */
    int                wait_fd[2];    /* 1e8/318 fd for sleeping server requests */
    BOOL               wow64_redir;   /* 1f0/320 Wow64 filesystem redirection flag */
    void              *vm86_ptr;      /* 1f4/328 data for vm86 mode */
    pthread_t          pthread_id;    /* 1f8/330 pthread thread id */
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

/* Register functions */

#ifdef __i386__
#define DEFINE_REGS_ENTRYPOINT( name, args ) \
    __ASM_GLOBAL_FUNC( name, \
                       ".byte 0x68\n\t"  /* pushl $__regs_func */       \
                       ".long " __ASM_NAME("__regs_") #name "-.-11\n\t" \
                       ".byte 0x6a," #args "\n\t" /* pushl $args */     \
                       "call " __ASM_NAME("__wine_call_from_32_regs") "\n\t" \
                       "ret $(4*" #args ")" ) /* fake ret to make copy protections happy */
#elif defined(__x86_64__)
#define DEFINE_REGS_ENTRYPOINT( name, args ) \
    __ASM_GLOBAL_FUNC( name, \
                       "movq %rcx,8(%rsp)\n\t"  \
                       "movq %rdx,16(%rsp)\n\t" \
                       "movq $" #args ",%rcx\n\t" \
                       "leaq " __ASM_NAME("__regs_") #name "(%rip),%rdx\n\t" \
                       "call " __ASM_NAME("__wine_call_from_regs"))
#endif

#endif
