/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "kernel_private.h"
#include "psapi.h"

#include "wine/list.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);


/****************************************************************************
 *              GetDllDirectoryA   (KERNEL32.@)
 */
DWORD WINAPI GetDllDirectoryA( DWORD buf_len, LPSTR buffer )
{
    UNICODE_STRING str;
    NTSTATUS status;
    WCHAR data[MAX_PATH];
    DWORD len;

    str.Buffer = data;
    str.MaximumLength = sizeof(data);

    for (;;)
    {
        status = LdrGetDllDirectory( &str );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        if (str.Buffer != data) HeapFree( GetProcessHeap(), 0, str.Buffer );
        str.MaximumLength = str.Length;
        if (!(str.Buffer = HeapAlloc( GetProcessHeap(), 0, str.MaximumLength )))
        {
            status = STATUS_NO_MEMORY;
            break;
        }
    }

    if (!set_ntstatus( status )) return 0;

    len = FILE_name_WtoA( str.Buffer, str.Length / sizeof(WCHAR), NULL, 0 );
    if (buffer && buf_len > len)
    {
        FILE_name_WtoA( str.Buffer, -1, buffer, buf_len );
    }
    else
    {
        len++;  /* for terminating null */
        if (buffer) *buffer = 0;
    }
    if (str.Buffer != data) HeapFree( GetProcessHeap(), 0, str.Buffer );
    return len;
}


/****************************************************************************
 *              GetDllDirectoryW   (KERNEL32.@)
 */
DWORD WINAPI GetDllDirectoryW( DWORD buf_len, LPWSTR buffer )
{
    UNICODE_STRING str;
    NTSTATUS status;

    str.Buffer = buffer;
    str.MaximumLength = min( buf_len, UNICODE_STRING_MAX_CHARS ) * sizeof(WCHAR);
    status = LdrGetDllDirectory( &str );
    if (status == STATUS_BUFFER_TOO_SMALL) status = STATUS_SUCCESS;
    if (!set_ntstatus( status )) return 0;
    return str.Length / sizeof(WCHAR);
}


/****************************************************************************
 *              SetDllDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI SetDllDirectoryA( LPCSTR dir )
{
    WCHAR *dirW = NULL;
    BOOL ret;

    if (dir && !(dirW = FILE_name_AtoW( dir, TRUE ))) return FALSE;
    ret = SetDllDirectoryW( dirW );
    HeapFree( GetProcessHeap(), 0, dirW );
    return ret;
}


/****************************************************************************
 *              SetDllDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI SetDllDirectoryW( LPCWSTR dir )
{
    UNICODE_STRING str;

    RtlInitUnicodeString( &str, dir );
    return set_ntstatus( LdrSetDllDirectory( &str ));
}


/***********************************************************************
 *             GetBinaryTypeW                     [KERNEL32.@]
 *
 * Determine whether a file is executable, and if so, what kind.
 *
 * PARAMS
 *  lpApplicationName [I] Path of the file to check
 *  lpBinaryType      [O] Destination for the binary type
 *
 * RETURNS
 *  TRUE, if the file is an executable, in which case lpBinaryType is set.
 *  FALSE, if the file is not an executable or if the function fails.
 *
 * NOTES
 *  The type of executable is a property that determines which subsystem an
 *  executable file runs under. lpBinaryType can be set to one of the following
 *  values:
 *   SCS_32BIT_BINARY: A Win32 based application
 *   SCS_64BIT_BINARY: A Win64 based application
 *   SCS_DOS_BINARY: An MS-Dos based application
 *   SCS_WOW_BINARY: A Win16 based application
 *   SCS_PIF_BINARY: A PIF file that executes an MS-Dos based app
 *   SCS_POSIX_BINARY: A POSIX based application ( Not implemented )
 *   SCS_OS216_BINARY: A 16bit OS/2 based application
 *
 *  To find the binary type, this function reads in the files header information.
 *  If extended header information is not present it will assume that the file
 *  is a DOS executable. If extended header information is present it will
 *  determine if the file is a 16, 32 or 64 bit Windows executable by checking the
 *  flags in the header.
 *
 *  ".com" and ".pif" files are only recognized by their file name extension,
 *  as per native Windows.
 */
BOOL WINAPI GetBinaryTypeW( LPCWSTR name, LPDWORD type )
{
    HANDLE hfile, mapping;
    NTSTATUS status;
    const WCHAR *ptr;

    TRACE("%s\n", debugstr_w(name) );

    if (type == NULL) return FALSE;

    hfile = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return FALSE;

    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY,
                              NULL, NULL, PAGE_READONLY, SEC_IMAGE, hfile );
    CloseHandle( hfile );

    switch (status)
    {
    case STATUS_SUCCESS:
        {
            SECTION_IMAGE_INFORMATION info;

            status = NtQuerySection( mapping, SectionImageInformation, &info, sizeof(info), NULL );
            CloseHandle( mapping );
            if (status) return FALSE;
            if (info.ImageCharacteristics & IMAGE_FILE_DLL) return FALSE;
            switch (info.Machine)
            {
            case IMAGE_FILE_MACHINE_I386:
            case IMAGE_FILE_MACHINE_ARMNT:
                *type = SCS_32BIT_BINARY;
                return TRUE;
            case IMAGE_FILE_MACHINE_AMD64:
            case IMAGE_FILE_MACHINE_ARM64:
                *type = SCS_64BIT_BINARY;
                return TRUE;
            }
            return FALSE;
        }
    case STATUS_INVALID_IMAGE_WIN_16:
        *type = SCS_WOW_BINARY;
        return TRUE;
    case STATUS_INVALID_IMAGE_WIN_32:
        *type = SCS_32BIT_BINARY;
        return TRUE;
    case STATUS_INVALID_IMAGE_WIN_64:
        *type = SCS_64BIT_BINARY;
        return TRUE;
    case STATUS_INVALID_IMAGE_NE_FORMAT:
        *type = SCS_OS216_BINARY;
        return TRUE;
    case STATUS_INVALID_IMAGE_PROTECT:
        *type = SCS_DOS_BINARY;
        return TRUE;
    case STATUS_INVALID_IMAGE_NOT_MZ:
        if ((ptr = wcsrchr( name, '.' )))
        {
            if (!wcsicmp( ptr, L".com" ))
            {
                *type = SCS_DOS_BINARY;
                return TRUE;
            }
            if (!wcsicmp( ptr, L".pif" ))
            {
                *type = SCS_PIF_BINARY;
                return TRUE;
            }
        }
        return FALSE;
    default:
        return FALSE;
    }
}

/***********************************************************************
 *             GetBinaryTypeA                     [KERNEL32.@]
 *             GetBinaryType                      [KERNEL32.@]
 *
 * See GetBinaryTypeW.
 */
BOOL WINAPI GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    ANSI_STRING app_nameA;

    TRACE("%s\n", debugstr_a(lpApplicationName));

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    RtlInitAnsiString(&app_nameA, lpApplicationName);
    if (!set_ntstatus( RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString,
                                                     &app_nameA, FALSE )))
        return FALSE;
    return GetBinaryTypeW(NtCurrentTeb()->StaticUnicodeString.Buffer, lpBinaryType);
}

/***********************************************************************
 *           GetProcAddress   		(KERNEL32.@)
 *
 * Find the address of an exported symbol in a loaded dll.
 *
 * PARAMS
 *  hModule  [I] Handle to the dll returned by LoadLibraryA().
 *  function [I] Name of the symbol, or an integer ordinal number < 16384
 *
 * RETURNS
 *  Success: A pointer to the symbol in the process address space.
 *  Failure: NULL. Use GetLastError() to determine the cause.
 */
FARPROC get_proc_address( HMODULE hModule, LPCSTR function )
{
    FARPROC     fp;

    if (!hModule) hModule = NtCurrentTeb()->Peb->ImageBaseAddress;

    if ((ULONG_PTR)function >> 16)
    {
        ANSI_STRING     str;

        RtlInitAnsiString( &str, function );
        if (!set_ntstatus( LdrGetProcedureAddress( hModule, &str, 0, (void**)&fp ))) return NULL;
    }
    else
        if (!set_ntstatus( LdrGetProcedureAddress( hModule, NULL, LOWORD(function), (void**)&fp )))
            return NULL;

    return fp;
}

/*
 * Work around a Delphi bug on x86_64.  When delay loading a symbol,
 * Delphi saves rcx, rdx, r8 and r9 to the stack.  It then calls
 * GetProcAddress(), pops the saved registers and calls the function.
 * This works fine if all of the parameters are ints.  However, since
 * it does not save xmm0 - 3, it relies on GetProcAddress() preserving
 * these registers if the function takes floating point parameters.
 * This wrapper saves xmm0 - 3 to the stack.
 */
#ifdef __arm64ec__
FARPROC WINAPI __attribute__((naked)) GetProcAddress( HMODULE module, LPCSTR function )
{
    asm( ".seh_proc \"#GetProcAddress\"\n\t"
         "stp x29, x30, [sp, #-48]!\n\t"
         ".seh_save_fplr_x 48\n\t"
         ".seh_endprologue\n\t"
         "stp d0, d1, [sp, #16]\n\t"
         "stp d2, d3, [sp, #32]\n\t"
         "bl \"#get_proc_address\"\n\t"
         "ldp d0, d1, [sp, #16]\n\t"
         "ldp d2, d3, [sp, #32]\n\t"
         "ldp x29, x30, [sp], #48\n\t"
         "ret\n\t"
         ".seh_endproc" );
}
#elif defined(__x86_64__)
__ASM_GLOBAL_FUNC( GetProcAddress,
                   ".byte 0x48\n\t"  /* hotpatch prolog */
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "subq $0x60,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movaps %xmm0,0x20(%rsp)\n\t"
                   "movaps %xmm1,0x30(%rsp)\n\t"
                   "movaps %xmm2,0x40(%rsp)\n\t"
                   "movaps %xmm3,0x50(%rsp)\n\t"
                   "call " __ASM_NAME("get_proc_address") "\n\t"
                   "movaps 0x50(%rsp), %xmm3\n\t"
                   "movaps 0x40(%rsp), %xmm2\n\t"
                   "movaps 0x30(%rsp), %xmm1\n\t"
                   "movaps 0x20(%rsp), %xmm0\n\t"
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )
#else /* __x86_64__ */

FARPROC WINAPI GetProcAddress( HMODULE module, LPCSTR function )
{
    return get_proc_address( module, function );
}

#endif /* __x86_64__ */
