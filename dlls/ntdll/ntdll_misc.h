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

#include "winnt.h"
#include "winternl.h"
#include "module.h"
#include "thread.h"
#include "wine/server.h"

/* debug helper */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);

extern void NTDLL_get_server_timeout( abs_time_t *when, const LARGE_INTEGER *timeout );

/* module handling */
extern WINE_MODREF *MODULE_AllocModRef( HMODULE hModule, LPCSTR filename );

extern FARPROC RELAY_GetProcAddress( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, const char *user );
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal );
extern void RELAY_SetupDLL( const char *module );

static inline HANDLE ntdll_get_process_heap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}

static inline RTL_USER_PROCESS_PARAMETERS* ntdll_get_process_pmts(void)
{
    return NtCurrentTeb()->Peb->ProcessParameters;
}

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
