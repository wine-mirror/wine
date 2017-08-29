/*
 * Kernel32 undocumented and private functions definition
 *
 * Copyright 2003 Eric Pouech
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

#ifndef __WINE_KERNEL_PRIVATE_H
#define __WINE_KERNEL_PRIVATE_H

#include "wine/server.h"

HANDLE  WINAPI OpenConsoleW(LPCWSTR, DWORD, BOOL, DWORD);
BOOL    WINAPI VerifyConsoleIoHandle(HANDLE);
HANDLE  WINAPI DuplicateConsoleHandle(HANDLE, DWORD, BOOL, DWORD);
BOOL    WINAPI CloseConsoleHandle(HANDLE handle);
HANDLE  WINAPI GetConsoleInputWaitHandle(void);
BOOL           CONSOLE_Init(RTL_USER_PROCESS_PARAMETERS *params) DECLSPEC_HIDDEN;
BOOL           CONSOLE_Exit(void) DECLSPEC_HIDDEN;

static inline BOOL is_console_handle(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE && ((UINT_PTR)h & 3) == 3;
}

/* map a real wineserver handle onto a kernel32 console handle */
static inline HANDLE console_handle_map(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE ? (HANDLE)((UINT_PTR)h ^ 3) : INVALID_HANDLE_VALUE;
}

/* map a kernel32 console handle onto a real wineserver handle */
static inline obj_handle_t console_handle_unmap(HANDLE h)
{
    return wine_server_obj_handle( h != INVALID_HANDLE_VALUE ? (HANDLE)((UINT_PTR)h ^ 3) : INVALID_HANDLE_VALUE );
}

/* Some Wine specific values for Console inheritance (params->ConsoleHandle) */
#define KERNEL32_CONSOLE_ALLOC          ((HANDLE)1)
#define KERNEL32_CONSOLE_SHELL          ((HANDLE)2)

extern HMODULE kernel32_handle DECLSPEC_HIDDEN;
extern SYSTEM_BASIC_INFORMATION system_info DECLSPEC_HIDDEN;

extern const WCHAR *DIR_Windows DECLSPEC_HIDDEN;
extern const WCHAR *DIR_System DECLSPEC_HIDDEN;
extern const WCHAR *DIR_SysWow64 DECLSPEC_HIDDEN;

extern void FILE_SetDosError(void) DECLSPEC_HIDDEN;
extern WCHAR *FILE_name_AtoW( LPCSTR name, BOOL alloc ) DECLSPEC_HIDDEN;
extern DWORD FILE_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen ) DECLSPEC_HIDDEN;

/* return values for MODULE_GetBinaryType */
enum binary_type
{
    BINARY_UNKNOWN = 0,
    BINARY_PE,
    BINARY_WIN16,
    BINARY_OS216,
    BINARY_DOS,
    BINARY_UNIX_EXE,
    BINARY_UNIX_LIB
};

#define BINARY_FLAG_DLL     0x01
#define BINARY_FLAG_64BIT   0x02
#define BINARY_FLAG_FAKEDLL 0x04

struct binary_info
{
    enum binary_type type;
    DWORD            arch;
    DWORD            flags;
    void            *res_start;
    void            *res_end;
};

/* module.c */
extern WCHAR *MODULE_get_dll_load_path( LPCWSTR module, int safe_mode ) DECLSPEC_HIDDEN;
extern void MODULE_get_binary_info( HANDLE hfile, struct binary_info *info ) DECLSPEC_HIDDEN;

extern BOOL NLS_IsUnicodeOnlyLcid(LCID) DECLSPEC_HIDDEN;

/* environ.c */
extern void ENV_CopyStartupInformation(void) DECLSPEC_HIDDEN;

/* computername.c */
extern void COMPUTERNAME_Init(void) DECLSPEC_HIDDEN;

/* locale.c */
extern void LOCALE_Init(void) DECLSPEC_HIDDEN;
extern void LOCALE_InitRegistry(void) DECLSPEC_HIDDEN;

/* oldconfig.c */
extern void convert_old_config(void) DECLSPEC_HIDDEN;

/* returns directory handle for named objects */
extern HANDLE get_BaseNamedObjects_handle(void) DECLSPEC_HIDDEN;

#endif
