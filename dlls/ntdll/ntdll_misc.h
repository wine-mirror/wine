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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include <stdarg.h>

#include "windef.h"
#include "winnt.h"
#include "ntstatus.h"
#include "winternl.h"
#include "wine/server.h"

/* The per-thread signal stack size */
#ifdef __i386__
#define SIGNAL_STACK_SIZE  4096
#else
#define SIGNAL_STACK_SIZE  0  /* we don't need a signal stack on non-i386 */
#endif

#define MAX_NT_PATH_LENGTH 277

extern void WINAPI __regs_RtlRaiseException( PEXCEPTION_RECORD, PCONTEXT );

/* debug helper */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);

extern void NTDLL_get_server_timeout( abs_time_t *when, const LARGE_INTEGER *timeout );
extern void NTDLL_from_server_timeout( LARGE_INTEGER *timeout, const abs_time_t *when );
extern NTSTATUS NTDLL_wait_for_multiple_objects( UINT count, const HANDLE *handles, UINT flags,
                                                 const LARGE_INTEGER *timeout, HANDLE signal_object );

/* init routines */
extern BOOL SIGNAL_Init(void);
extern void debug_init(void);
extern void thread_init(void);
extern void virtual_init(void);

/* server support */
extern time_t server_start_time;
extern void server_init_process(void);
extern size_t server_init_thread( int unix_pid, int unix_tid, void *entry_point );
extern void DECLSPEC_NORETURN server_protocol_error( const char *err, ... );
extern void DECLSPEC_NORETURN server_protocol_perror( const char *err );
extern void DECLSPEC_NORETURN server_abort_thread( int status );

/* module handling */
extern FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, const WCHAR *user );
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal, const WCHAR *user );
extern void RELAY_SetupDLL( HMODULE hmod );
extern void SNOOP_SetupDLL( HMODULE hmod );
extern UNICODE_STRING system_dir;

/* redefine these to make sure we don't reference kernel symbols */
#define GetProcessHeap()       (NtCurrentTeb()->Peb->ProcessHeap)
#define GetCurrentProcessId()  ((DWORD)NtCurrentTeb()->ClientId.UniqueProcess)
#define GetCurrentThreadId()   ((DWORD)NtCurrentTeb()->ClientId.UniqueThread)

/* Device IO */
extern NTSTATUS CDROM_DeviceIoControl(HANDLE hDevice, 
                                      HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                      PVOID UserApcContext, 
                                      PIO_STATUS_BLOCK piosb, 
                                      ULONG IoControlCode,
                                      LPVOID lpInBuffer, DWORD nInBufferSize,
                                      LPVOID lpOutBuffer, DWORD nOutBufferSize);

/* file I/O */
extern NTSTATUS FILE_GetNtStatus(void);
extern BOOL DIR_is_hidden_file( const UNICODE_STRING *name );

/* virtual memory */
extern NTSTATUS VIRTUAL_HandleFault(LPCVOID addr);
extern BOOL VIRTUAL_HasMapping( LPCVOID addr );
extern void VIRTUAL_UseLargeAddressSpace(void);

extern BOOL is_current_process( HANDLE handle );

/* code pages */
extern int ntdll_umbstowcs(DWORD flags, const char* src, int srclen, WCHAR* dst, int dstlen);
extern int ntdll_wcstoumbs(DWORD flags, const WCHAR* src, int srclen, char* dst, int dstlen,
                           const char* defchar, int *used );

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

struct ntdll_thread_data
{
    DWORD              teb_sel;       /* selector to TEB */
    struct debug_info *debug_info;    /* info for debugstr functions */
    int                request_fd;    /* fd for sending server requests */
    int                reply_fd;      /* fd for receiving server replies */
    int                wait_fd[2];    /* fd for sleeping server requests */
    void              *vm86_ptr;      /* data for vm86 mode */

    void              *pad[3];        /* change this if you add fields! */
};

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)NtCurrentTeb()->SystemReserved2;
}

#endif
