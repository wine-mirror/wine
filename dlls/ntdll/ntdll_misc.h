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

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              hConsole;
    ULONG               ProcessGroup;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    UNICODE_STRING      CurrentDirectoryName;
    HANDLE              CurrentDirectoryHandle;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      DesktopInfo;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

static inline HANDLE ntdll_get_process_heap(void)
{
    HANDLE *pdb = (HANDLE *)NtCurrentTeb()->process;
    return pdb[0x18 / sizeof(HANDLE)];  /* get dword at offset 0x18 in pdb */
}

/* FIXME: this should be part of PEB, once it's defined */
extern RTL_USER_PROCESS_PARAMETERS process_pmts;
BOOL build_initial_environment(void);

static inline RTL_USER_PROCESS_PARAMETERS* ntdll_get_process_pmts(void)
{
    return &process_pmts;
}
#endif
