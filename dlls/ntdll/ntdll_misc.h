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
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "ntstatus.h"
#include "winternl.h"
#include "module.h"
#include "thread.h"
#include "wine/server.h"

/* The per-thread signal stack size */
#ifdef __i386__
#define SIGNAL_STACK_SIZE  4096
#else
#define SIGNAL_STACK_SIZE  0  /* we don't need a signal stack on non-i386 */
#endif

/* debug helper */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);

extern void NTDLL_get_server_timeout( abs_time_t *when, const LARGE_INTEGER *timeout );
extern NTSTATUS NTDLL_wait_for_multiple_objects( UINT count, const HANDLE *handles, UINT flags,
                                                 const LARGE_INTEGER *timeout );

/* init routines */
extern BOOL SIGNAL_Init(void);
extern void thread_init(void);

/* server support */
extern void server_init_process(void);
extern void server_init_thread(void);
extern void DECLSPEC_NORETURN server_protocol_error( const char *err, ... );
extern void DECLSPEC_NORETURN server_protocol_perror( const char *err );
extern void DECLSPEC_NORETURN server_abort_thread( int status );

/* module handling */
extern BOOL MODULE_GetSystemDirectory( UNICODE_STRING *sysdir );
extern void RELAY_InitDebugLists(void);
extern FARPROC RELAY_GetProcAddress( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, const WCHAR *user );
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal );
extern void RELAY_SetupDLL( HMODULE hmod );
extern void SNOOP_SetupDLL( HMODULE hmod );

static inline HANDLE ntdll_get_process_heap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}
/* redefine these to make sure we don't reference kernel symbols */
#define GetProcessHeap()       (NtCurrentTeb()->Peb->ProcessHeap)
#define GetCurrentProcessId()  ((DWORD)NtCurrentTeb()->ClientId.UniqueProcess)
#define GetCurrentThreadId()   ((DWORD)NtCurrentTeb()->ClientId.UniqueThread)

static inline RTL_USER_PROCESS_PARAMETERS* ntdll_get_process_pmts(void)
{
    return NtCurrentTeb()->Peb->ProcessParameters;
}

/* hack: upcall to kernel */
extern HANDLE (WINAPI *pCreateFileW)( LPCWSTR filename, DWORD access, DWORD sharing,
                                      LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                      DWORD attributes, HANDLE template );

/* Device IO */
/* ntdll/cdrom.c.c */
extern NTSTATUS CDROM_DeviceIoControl(DWORD clientID, HANDLE hDevice, 
                                      HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                      PVOID UserApcContext, 
                                      PIO_STATUS_BLOCK piosb, 
                                      ULONG IoControlCode,
                                      LPVOID lpInBuffer, DWORD nInBufferSize,
                                      LPVOID lpOutBuffer, DWORD nOutBufferSize);

#endif
