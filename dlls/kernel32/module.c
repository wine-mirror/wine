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

#include "config.h"
#include "wine/port.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "kernel_private.h"
#include "psapi.h"

#include "wine/exception.h"
#include "wine/list.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#define NE_FFLAGS_LIBMODULE 0x8000

/* to keep track of LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE file handles */
struct exclusive_datafile
{
    struct list entry;
    HMODULE     module;
    HANDLE      file;
};
static struct list exclusive_datafile_list = LIST_INIT( exclusive_datafile_list );

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
    static const WCHAR comW[] = { '.','c','o','m',0 };
    static const WCHAR pifW[] = { '.','p','i','f',0 };
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
            switch (info.Machine)
            {
            case IMAGE_FILE_MACHINE_I386:
            case IMAGE_FILE_MACHINE_ARM:
            case IMAGE_FILE_MACHINE_THUMB:
            case IMAGE_FILE_MACHINE_ARMNT:
            case IMAGE_FILE_MACHINE_POWERPC:
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
        if ((ptr = strrchrW( name, '.' )))
        {
            if (!strcmpiW( ptr, comW ))
            {
                *type = SCS_DOS_BINARY;
                return TRUE;
            }
            if (!strcmpiW( ptr, pifW ))
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

/******************************************************************
 *		load_library_as_datafile
 */
static BOOL load_library_as_datafile( LPCWSTR load_path, DWORD flags, LPCWSTR name, HMODULE *hmod )
{
    static const WCHAR dotDLL[] = {'.','d','l','l',0};

    WCHAR filenameW[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE mapping;
    HMODULE module = 0;
    DWORD protect = PAGE_READONLY;
    DWORD sharing = FILE_SHARE_READ | FILE_SHARE_DELETE;

    *hmod = 0;

    if (flags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) protect |= SEC_IMAGE;

    if (SearchPathW( load_path, name, dotDLL, ARRAY_SIZE( filenameW ), filenameW, NULL ))
    {
        hFile = CreateFileW( filenameW, GENERIC_READ, sharing, NULL, OPEN_EXISTING, 0, 0 );
    }
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    mapping = CreateFileMappingW( hFile, NULL, protect, 0, 0, NULL );
    if (!mapping) goto failed;

    module = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!module) goto failed;

    if (!(flags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))
    {
        /* make sure it's a valid PE file */
        if (!RtlImageNtHeader( module )) goto failed;
        *hmod = (HMODULE)((char *)module + 1); /* set bit 0 for data file module */

        if (flags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)
        {
            struct exclusive_datafile *file = HeapAlloc( GetProcessHeap(), 0, sizeof(*file) );
            if (!file) goto failed;
            file->module = *hmod;
            file->file   = hFile;
            list_add_head( &exclusive_datafile_list, &file->entry );
            TRACE( "delaying close %p for module %p\n", file->file, file->module );
            return TRUE;
        }
    }
    else *hmod = (HMODULE)((char *)module + 2); /* set bit 1 for image resource module */

    CloseHandle( hFile );
    return TRUE;

failed:
    if (module) UnmapViewOfFile( module );
    CloseHandle( hFile );
    return FALSE;
}


/******************************************************************
 *		load_library
 *
 * Helper for LoadLibraryExA/W.
 */
static HMODULE load_library( const UNICODE_STRING *libname, DWORD flags )
{
    NTSTATUS nts;
    HMODULE hModule;
    WCHAR *load_path, *dummy;
    const DWORD unsupported_flags = (LOAD_IGNORE_CODE_AUTHZ_LEVEL |
                                     LOAD_LIBRARY_REQUIRE_SIGNED_TARGET);

    if( flags & unsupported_flags)
        FIXME("unsupported flag(s) used (flags: 0x%08x)\n", flags);

    if (!set_ntstatus( LdrGetDllPath( libname->Buffer, flags, &load_path, &dummy ))) return 0;

    if (flags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE))
    {
        ULONG_PTR magic;

        LdrLockLoaderLock( 0, NULL, &magic );
        if (!LdrGetDllHandle( load_path, flags, libname, &hModule ))
        {
            LdrAddRefDll( 0, hModule );
            LdrUnlockLoaderLock( 0, magic );
            goto done;
        }
        if (load_library_as_datafile( load_path, flags, libname->Buffer, &hModule ))
        {
            LdrUnlockLoaderLock( 0, magic );
            goto done;
        }
        LdrUnlockLoaderLock( 0, magic );
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    nts = LdrLoadDll( load_path, flags, libname, &hModule );
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        if (nts == STATUS_DLL_NOT_FOUND && (GetVersion() & 0x80000000))
            SetLastError( ERROR_DLL_NOT_FOUND );
        else
            SetLastError( RtlNtStatusToDosError( nts ) );
    }
done:
    RtlReleasePath( load_path );
    return hModule;
}


/******************************************************************
 *		LoadLibraryExA          (KERNEL32.@)
 *
 * Load a dll file into the process address space.
 *
 * PARAMS
 *  libname [I] Name of the file to load
 *  hfile   [I] Reserved, must be 0.
 *  flags   [I] Flags for loading the dll
 *
 * RETURNS
 *  Success: A handle to the loaded dll.
 *  Failure: A NULL handle. Use GetLastError() to determine the cause.
 *
 * NOTES
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module representations.
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
    WCHAR *libnameW;

    if (!(libnameW = FILE_name_AtoW( libname, FALSE ))) return 0;
    return LoadLibraryExW( libnameW, hfile, flags );
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32.@)
 *
 * Unicode version of LoadLibraryExA.
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryExW(LPCWSTR libnameW, HANDLE hfile, DWORD flags)
{
    UNICODE_STRING      wstr;
    HMODULE             res;

    if (!libnameW)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    RtlInitUnicodeString( &wstr, libnameW );
    if (wstr.Buffer[wstr.Length/sizeof(WCHAR) - 1] != ' ')
        return load_library( &wstr, flags );

    /* Library name has trailing spaces */
    RtlCreateUnicodeString( &wstr, libnameW );
    while (wstr.Length > sizeof(WCHAR) &&
           wstr.Buffer[wstr.Length/sizeof(WCHAR) - 1] == ' ')
    {
        wstr.Length -= sizeof(WCHAR);
    }
    wstr.Buffer[wstr.Length/sizeof(WCHAR)] = '\0';
    res = load_library( &wstr, flags );
    RtlFreeUnicodeString( &wstr );
    return res;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32.@)
 *
 * Load a dll file into the process address space.
 *
 * PARAMS
 *  libname [I] Name of the file to load
 *
 * RETURNS
 *  Success: A handle to the loaded dll.
 *  Failure: A NULL handle. Use GetLastError() to determine the cause.
 *
 * NOTES
 * See LoadLibraryExA().
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryA(LPCSTR libname)
{
    return LoadLibraryExA(libname, 0, 0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32.@)
 *
 * Unicode version of LoadLibraryA.
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW, 0, 0);
}

/***********************************************************************
 *           FreeLibrary   (KERNEL32.@)
 *
 * Free a dll loaded into the process address space.
 *
 * PARAMS
 *  hLibModule [I] Handle to the dll returned by LoadLibraryA().
 *
 * RETURNS
 *  Success: TRUE. The dll is removed if it is not still in use.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeLibrary(HINSTANCE hLibModule)
{
    if (!hLibModule)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if ((ULONG_PTR)hLibModule & 3) /* this is a datafile module */
    {
        void *ptr = (void *)((ULONG_PTR)hLibModule & ~3);
        if (!RtlImageNtHeader( ptr ))
        {
            SetLastError( ERROR_BAD_EXE_FORMAT );
            return FALSE;
        }
        if ((ULONG_PTR)hLibModule & 1)
        {
            struct exclusive_datafile *file;
            ULONG_PTR magic;

            LdrLockLoaderLock( 0, NULL, &magic );
            LIST_FOR_EACH_ENTRY( file, &exclusive_datafile_list, struct exclusive_datafile, entry )
            {
                if (file->module != hLibModule) continue;
                TRACE( "closing %p for module %p\n", file->file, file->module );
                CloseHandle( file->file );
                list_remove( &file->entry );
                HeapFree( GetProcessHeap(), 0, file );
                break;
            }
            LdrUnlockLoaderLock( 0, magic );
        }
        return UnmapViewOfFile( ptr );
    }

    return set_ntstatus( LdrUnloadDll( hLibModule ));
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

#ifdef __x86_64__
/*
 * Work around a Delphi bug on x86_64.  When delay loading a symbol,
 * Delphi saves rcx, rdx, r8 and r9 to the stack.  It then calls
 * GetProcAddress(), pops the saved registers and calls the function.
 * This works fine if all of the parameters are ints.  However, since
 * it does not save xmm0 - 3, it relies on GetProcAddress() preserving
 * these registers if the function takes floating point parameters.
 * This wrapper saves xmm0 - 3 to the stack.
 */
extern FARPROC get_proc_address_wrapper( HMODULE module, LPCSTR function );

__ASM_GLOBAL_FUNC( get_proc_address_wrapper,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "subq $0x40,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x40\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movaps %xmm0,-0x10(%rbp)\n\t"
                   "movaps %xmm1,-0x20(%rbp)\n\t"
                   "movaps %xmm2,-0x30(%rbp)\n\t"
                   "movaps %xmm3,-0x40(%rbp)\n\t"
                   "call " __ASM_NAME("get_proc_address") "\n\t"
                   "movaps -0x40(%rbp), %xmm3\n\t"
                   "movaps -0x30(%rbp), %xmm2\n\t"
                   "movaps -0x20(%rbp), %xmm1\n\t"
                   "movaps -0x10(%rbp), %xmm0\n\t"
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )
#else /* __x86_64__ */

static inline FARPROC get_proc_address_wrapper( HMODULE module, LPCSTR function )
{
    return get_proc_address( module, function );
}

#endif /* __x86_64__ */

FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    return get_proc_address_wrapper( hModule, function );
}

typedef struct _PEB32
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN SpareBool;
    DWORD   Mutant;
    DWORD   ImageBaseAddress;
    DWORD   LdrData;
} PEB32;

typedef struct _LIST_ENTRY32
{
  DWORD Flink;
  DWORD Blink;
} LIST_ENTRY32;

typedef struct _PEB_LDR_DATA32
{
    ULONG        Length;
    BOOLEAN      Initialized;
    DWORD        SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
} PEB_LDR_DATA32;

typedef struct _UNICODE_STRING32
{
  USHORT Length;
  USHORT MaximumLength;
  DWORD  Buffer;
} UNICODE_STRING32;

typedef struct _LDR_MODULE32
{
    LIST_ENTRY32        InLoadOrderModuleList;
    LIST_ENTRY32        InMemoryOrderModuleList;
    LIST_ENTRY32        InInitializationOrderModuleList;
    DWORD               BaseAddress;
    DWORD               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING32    FullDllName;
    UNICODE_STRING32    BaseDllName;
} LDR_MODULE32;

typedef struct {
    HANDLE process;
    PLIST_ENTRY head, current;
    LDR_MODULE ldr_module;
    BOOL wow64;
    LDR_MODULE32 ldr_module32;
} MODULE_ITERATOR;

static BOOL init_module_iterator(MODULE_ITERATOR *iter, HANDLE process)
{
    PROCESS_BASIC_INFORMATION pbi;
    PPEB_LDR_DATA ldr_data;
    NTSTATUS status;

    if (!IsWow64Process(process, &iter->wow64))
        return FALSE;

    /* Get address of PEB */
    status = NtQueryInformationProcess(process, ProcessBasicInformation,
                                       &pbi, sizeof(pbi), NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    if (sizeof(void *) == 8 && iter->wow64)
    {
        PEB_LDR_DATA32 *ldr_data32_ptr;
        DWORD ldr_data32, first_module;
        PEB32 *peb32;

        peb32 = (PEB32 *)(DWORD_PTR)pbi.PebBaseAddress;

        if (!ReadProcessMemory(process, &peb32->LdrData, &ldr_data32,
                               sizeof(ldr_data32), NULL))
            return FALSE;
        ldr_data32_ptr = (PEB_LDR_DATA32 *)(DWORD_PTR) ldr_data32;

        if (!ReadProcessMemory(process,
                               &ldr_data32_ptr->InLoadOrderModuleList.Flink,
                               &first_module, sizeof(first_module), NULL))
            return FALSE;
        iter->head = (LIST_ENTRY *)&ldr_data32_ptr->InLoadOrderModuleList;
        iter->current = (LIST_ENTRY *)(DWORD_PTR) first_module;
        iter->process = process;

        return TRUE;
    }

    /* Read address of LdrData from PEB */
    if (!ReadProcessMemory(process, &pbi.PebBaseAddress->LdrData,
                           &ldr_data, sizeof(ldr_data), NULL))
        return FALSE;

    /* Read address of first module from LdrData */
    if (!ReadProcessMemory(process,
                           &ldr_data->InLoadOrderModuleList.Flink,
                           &iter->current, sizeof(iter->current), NULL))
        return FALSE;

    iter->head = &ldr_data->InLoadOrderModuleList;
    iter->process = process;

    return TRUE;
}

static int module_iterator_next(MODULE_ITERATOR *iter)
{
    if (iter->current == iter->head)
        return 0;

    if (sizeof(void *) == 8 && iter->wow64)
    {
        LIST_ENTRY32 *entry32 = (LIST_ENTRY32 *)iter->current;

        if (!ReadProcessMemory(iter->process,
                               CONTAINING_RECORD(entry32, LDR_MODULE32, InLoadOrderModuleList),
                               &iter->ldr_module32, sizeof(iter->ldr_module32), NULL))
            return -1;

        iter->current = (LIST_ENTRY *)(DWORD_PTR) iter->ldr_module32.InLoadOrderModuleList.Flink;
        return 1;
    }

    if (!ReadProcessMemory(iter->process,
                           CONTAINING_RECORD(iter->current, LDR_MODULE, InLoadOrderModuleList),
                           &iter->ldr_module, sizeof(iter->ldr_module), NULL))
         return -1;

    iter->current = iter->ldr_module.InLoadOrderModuleList.Flink;
    return 1;
}

static BOOL get_ldr_module(HANDLE process, HMODULE module, LDR_MODULE *ldr_module)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    while ((ret = module_iterator_next(&iter)) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || module == iter.ldr_module.BaseAddress)
        {
            *ldr_module = iter.ldr_module;
            return TRUE;
        }

    if (ret == 0)
        SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

static BOOL get_ldr_module32(HANDLE process, HMODULE module, LDR_MODULE32 *ldr_module)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    while ((ret = module_iterator_next(&iter)) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || (DWORD)(DWORD_PTR) module == iter.ldr_module32.BaseAddress)
        {
            *ldr_module = iter.ldr_module32;
            return TRUE;
        }

    if (ret == 0)
        SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

/***********************************************************************
 *           K32EnumProcessModules (KERNEL32.@)
 *
 * NOTES
 *  Returned list is in load order.
 */
BOOL WINAPI K32EnumProcessModules(HANDLE process, HMODULE *lphModule,
                                  DWORD cb, DWORD *needed)
{
    MODULE_ITERATOR iter;
    DWORD size = 0;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    if (cb && !lphModule)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    while ((ret = module_iterator_next(&iter)) > 0)
    {
        if (cb >= sizeof(HMODULE))
        {
            if (sizeof(void *) == 8 && iter.wow64)
                *lphModule++ = (HMODULE) (DWORD_PTR)iter.ldr_module32.BaseAddress;
            else
                *lphModule++ = iter.ldr_module.BaseAddress;
            cb -= sizeof(HMODULE);
        }
        size += sizeof(HMODULE);
    }

    if (!needed)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }
    *needed = size;

    return ret == 0;
}

/***********************************************************************
 *           K32EnumProcessModulesEx (KERNEL32.@)
 *
 * NOTES
 *  Returned list is in load order.
 */
BOOL WINAPI K32EnumProcessModulesEx(HANDLE process, HMODULE *lphModule,
                                    DWORD cb, DWORD *needed, DWORD filter)
{
    FIXME("(%p, %p, %d, %p, %d) semi-stub\n",
          process, lphModule, cb, needed, filter);
    return K32EnumProcessModules(process, lphModule, cb, needed);
}

/***********************************************************************
 *           K32GetModuleBaseNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleBaseNameW(HANDLE process, HMODULE module,
                                   LPWSTR base_name, DWORD size)
{
    LDR_MODULE ldr_module;
    BOOL wow64;

    if (!IsWow64Process(process, &wow64))
        return 0;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return 0;

        size = min(ldr_module32.BaseDllName.Length / sizeof(WCHAR), size);
        if (!ReadProcessMemory(process, (void *)(DWORD_PTR)ldr_module32.BaseDllName.Buffer,
                               base_name, size * sizeof(WCHAR), NULL))
            return 0;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return 0;

        size = min(ldr_module.BaseDllName.Length / sizeof(WCHAR), size);
        if (!ReadProcessMemory(process, ldr_module.BaseDllName.Buffer,
                               base_name, size * sizeof(WCHAR), NULL))
            return 0;
    }

    base_name[size] = 0;
    return size;
}

/***********************************************************************
 *           K32GetModuleBaseNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleBaseNameA(HANDLE process, HMODULE module,
                                   LPSTR base_name, DWORD size)
{
    WCHAR *base_name_w;
    DWORD len, ret = 0;

    if(!base_name || !size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    base_name_w = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    if(!base_name_w)
        return 0;

    len = K32GetModuleBaseNameW(process, module, base_name_w, size);
    TRACE("%d, %s\n", len, debugstr_w(base_name_w));
    if (len)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, base_name_w, len,
                                  base_name, size, NULL, NULL);
        if (ret < size) base_name[ret] = 0;
    }
    HeapFree(GetProcessHeap(), 0, base_name_w);
    return ret;
}

/***********************************************************************
 *           K32GetModuleFileNameExW (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleFileNameExW(HANDLE process, HMODULE module,
                                     LPWSTR file_name, DWORD size)
{
    LDR_MODULE ldr_module;
    BOOL wow64;
    DWORD len;

    if (!size) return 0;

    if (!IsWow64Process(process, &wow64))
        return 0;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return 0;

        len = ldr_module32.FullDllName.Length / sizeof(WCHAR);
        if (!ReadProcessMemory(process, (void *)(DWORD_PTR)ldr_module32.FullDllName.Buffer,
                               file_name, min( len, size ) * sizeof(WCHAR), NULL))
            return 0;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return 0;

        len = ldr_module.FullDllName.Length / sizeof(WCHAR);
        if (!ReadProcessMemory(process, ldr_module.FullDllName.Buffer,
                               file_name, min( len, size ) * sizeof(WCHAR), NULL))
            return 0;
    }

    if (len < size)
    {
        file_name[len] = 0;
        return len;
    }
    else
    {
        file_name[size - 1] = 0;
        return size;
    }
}

/***********************************************************************
 *           K32GetModuleFileNameExA (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleFileNameExA(HANDLE process, HMODULE module,
                                     LPSTR file_name, DWORD size)
{
    WCHAR *ptr;
    DWORD len;

    TRACE("(hProcess=%p, hModule=%p, %p, %d)\n", process, module, file_name, size);

    if (!file_name || !size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if ( process == GetCurrentProcess() )
    {
        len = GetModuleFileNameA( module, file_name, size );
        file_name[size - 1] = '\0';
        return len;
    }

    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR)))) return 0;

    len = K32GetModuleFileNameExW(process, module, ptr, size);
    if (!len)
    {
        file_name[0] = '\0';
    }
    else
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ptr, -1, file_name, size, NULL, NULL ))
        {
            file_name[size - 1] = 0;
            len = size;
        }
        else if (len < size) len = strlen( file_name );
    }

    HeapFree(GetProcessHeap(), 0, ptr);
    return len;
}

/***********************************************************************
 *           K32GetModuleInformation (KERNEL32.@)
 */
BOOL WINAPI K32GetModuleInformation(HANDLE process, HMODULE module,
                                    MODULEINFO *modinfo, DWORD cb)
{
    LDR_MODULE ldr_module;
    BOOL wow64;

    if (cb < sizeof(MODULEINFO))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!IsWow64Process(process, &wow64))
        return FALSE;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return FALSE;

        modinfo->lpBaseOfDll = (void *)(DWORD_PTR)ldr_module32.BaseAddress;
        modinfo->SizeOfImage = ldr_module32.SizeOfImage;
        modinfo->EntryPoint  = (void *)(DWORD_PTR)ldr_module32.EntryPoint;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return FALSE;

        modinfo->lpBaseOfDll = ldr_module.BaseAddress;
        modinfo->SizeOfImage = ldr_module.SizeOfImage;
        modinfo->EntryPoint  = ldr_module.EntryPoint;
    }
    return TRUE;
}

#ifdef __i386__

/***********************************************************************
 *           __wine_dll_register_16 (KERNEL32.@)
 *
 * No longer used.
 */
void __wine_dll_register_16( const IMAGE_DOS_HEADER *header, const char *file_name )
{
    ERR( "loading old style 16-bit dll %s no longer supported\n", file_name );
}


/***********************************************************************
 *           __wine_dll_unregister_16 (KERNEL32.@)
 *
 * No longer used.
 */
void __wine_dll_unregister_16( const IMAGE_DOS_HEADER *header )
{
}

#endif
