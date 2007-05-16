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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/winbase16.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "thread.h"
#include "kernel_private.h"

#include "wine/exception.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

static WCHAR *dll_directory;  /* extra path for SetDllDirectoryW */

static CRITICAL_SECTION dlldir_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dlldir_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dlldir_section") }
};
static CRITICAL_SECTION dlldir_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/****************************************************************************
 *              GetDllDirectoryA   (KERNEL32.@)
 */
DWORD WINAPI GetDllDirectoryA( DWORD buf_len, LPSTR buffer )
{
    DWORD len;

    RtlEnterCriticalSection( &dlldir_section );
    len = dll_directory ? FILE_name_WtoA( dll_directory, strlenW(dll_directory), NULL, 0 ) : 0;
    if (buffer && buf_len > len)
    {
        if (dll_directory) FILE_name_WtoA( dll_directory, -1, buffer, buf_len );
        else *buffer = 0;
    }
    else
    {
        len++;  /* for terminating null */
        if (buffer) *buffer = 0;
    }
    RtlLeaveCriticalSection( &dlldir_section );
    return len;
}


/****************************************************************************
 *              GetDllDirectoryW   (KERNEL32.@)
 */
DWORD WINAPI GetDllDirectoryW( DWORD buf_len, LPWSTR buffer )
{
    DWORD len;

    RtlEnterCriticalSection( &dlldir_section );
    len = dll_directory ? strlenW( dll_directory ) : 0;
    if (buffer && buf_len > len)
    {
        if (dll_directory) memcpy( buffer, dll_directory, (len + 1) * sizeof(WCHAR) );
        else *buffer = 0;
    }
    else
    {
        len++;  /* for terminating null */
        if (buffer) *buffer = 0;
    }
    RtlLeaveCriticalSection( &dlldir_section );
    return len;
}


/****************************************************************************
 *              SetDllDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI SetDllDirectoryA( LPCSTR dir )
{
    WCHAR *dirW;
    BOOL ret;

    if (!(dirW = FILE_name_AtoW( dir, TRUE ))) return FALSE;
    ret = SetDllDirectoryW( dirW );
    HeapFree( GetProcessHeap(), 0, dirW );
    return ret;
}


/****************************************************************************
 *              SetDllDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI SetDllDirectoryW( LPCWSTR dir )
{
    WCHAR *newdir = NULL;

    if (dir)
    {
        DWORD len = (strlenW(dir) + 1) * sizeof(WCHAR);
        if (!(newdir = HeapAlloc( GetProcessHeap(), 0, len )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        memcpy( newdir, dir, len );
    }

    RtlEnterCriticalSection( &dlldir_section );
    HeapFree( GetProcessHeap(), 0, dll_directory );
    dll_directory = newdir;
    RtlLeaveCriticalSection( &dlldir_section );
    return TRUE;
}


/****************************************************************************
 *              DisableThreadLibraryCalls (KERNEL32.@)
 *
 * Inform the module loader that thread notifications are not required for a dll.
 *
 * PARAMS
 *  hModule [I] Module handle to skip calls for
 *
 * RETURNS
 *  Success: TRUE. Thread attach and detach notifications will not be sent
 *           to hModule.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  This is typically called from the dll entry point of a dll during process
 *  attachment, for dlls that do not need to process thread notifications.
 */
BOOL WINAPI DisableThreadLibraryCalls( HMODULE hModule )
{
    NTSTATUS    nts = LdrDisableThreadCalloutsForDll( hModule );
    if (nts == STATUS_SUCCESS) return TRUE;

    SetLastError( RtlNtStatusToDosError( nts ) );
    return FALSE;
}


/* Check whether a file is an OS/2 or a very old Windows executable
 * by testing on import of KERNEL.
 *
 * FIXME: is reading the module imports the only way of discerning
 *        old Windows binaries from OS/2 ones ? At least it seems so...
 */
static enum binary_type MODULE_Decide_OS2_OldWin(HANDLE hfile, const IMAGE_DOS_HEADER *mz,
                                                 const IMAGE_OS2_HEADER *ne)
{
    DWORD currpos = SetFilePointer( hfile, 0, NULL, SEEK_CUR);
    enum binary_type ret = BINARY_OS216;
    LPWORD modtab = NULL;
    LPSTR nametab = NULL;
    DWORD len;
    int i;

    /* read modref table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_modtab, NULL, SEEK_SET ) == -1)
      || (!(modtab = HeapAlloc( GetProcessHeap(), 0, ne->ne_cmod*sizeof(WORD))))
      || (!(ReadFile(hfile, modtab, ne->ne_cmod*sizeof(WORD), &len, NULL)))
      || (len != ne->ne_cmod*sizeof(WORD)) )
	goto broken;

    /* read imported names table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_imptab, NULL, SEEK_SET ) == -1)
      || (!(nametab = HeapAlloc( GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab)))
      || (!(ReadFile(hfile, nametab, ne->ne_enttab - ne->ne_imptab, &len, NULL)))
      || (len != ne->ne_enttab - ne->ne_imptab) )
	goto broken;

    for (i=0; i < ne->ne_cmod; i++)
    {
	LPSTR module = &nametab[modtab[i]];
	TRACE("modref: %.*s\n", module[0], &module[1]);
	if (!(strncmp(&module[1], "KERNEL", module[0])))
	{ /* very old Windows file */
	    MESSAGE("This seems to be a very old (pre-3.0) Windows executable. Expect crashes, especially if this is a real-mode binary !\n");
            ret = BINARY_WIN16;
	    goto good;
	}
    }

broken:
    ERR("Hmm, an error occurred. Is this binary file broken?\n");

good:
    HeapFree( GetProcessHeap(), 0, modtab);
    HeapFree( GetProcessHeap(), 0, nametab);
    SetFilePointer( hfile, currpos, NULL, SEEK_SET); /* restore filepos */
    return ret;
}

/***********************************************************************
 *           MODULE_GetBinaryType
 */
enum binary_type MODULE_GetBinaryType( HANDLE hfile, void **res_start, void **res_end )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char ignored[12];
            unsigned short type;
        } elf;
        struct
        {
            unsigned long magic;
            unsigned long cputype;
            unsigned long cpusubtype;
            unsigned long filetype;
        } macho;
        IMAGE_DOS_HEADER mz;
    } header;

    DWORD len;

    /* Seek to the start of the file and read the header information. */
    if (SetFilePointer( hfile, 0, NULL, SEEK_SET ) == -1)
        return BINARY_UNKNOWN;
    if (!ReadFile( hfile, &header, sizeof(header), &len, NULL ) || len != sizeof(header))
        return BINARY_UNKNOWN;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
        /* FIXME: we don't bother to check byte order, architecture, etc. */
        switch(header.elf.type)
        {
        case 2: return BINARY_UNIX_EXE;
        case 3: return BINARY_UNIX_LIB;
        }
        return BINARY_UNKNOWN;
    }

    /* Mach-o File with Endian set to Big Endian or Little Endian */
    if (header.macho.magic == 0xfeedface || header.macho.magic == 0xcefaedfe)
    {
        switch(header.macho.filetype)
        {
            case 0x8: /* MH_BUNDLE */ return BINARY_UNIX_LIB;
        }
        return BINARY_UNKNOWN;
    }

    /* Not ELF, try DOS */

    if (header.mz.e_magic == IMAGE_DOS_SIGNATURE)
    {
        union
        {
            IMAGE_OS2_HEADER os2;
            IMAGE_NT_HEADERS nt;
        } ext_header;

        /* We do have a DOS image so we will now try to seek into
         * the file by the amount indicated by the field
         * "Offset to extended header" and read in the
         * "magic" field information at that location.
         * This will tell us if there is more header information
         * to read or not.
         */
        if (SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) == -1)
            return BINARY_DOS;
        if (!ReadFile( hfile, &ext_header, sizeof(ext_header), &len, NULL ) || len < 4)
            return BINARY_DOS;

        /* Reading the magic field succeeded so
         * we will try to determine what type it is.
         */
        if (!memcmp( &ext_header.nt.Signature, "PE\0\0", 4 ))
        {
            if (len >= sizeof(ext_header.nt.FileHeader))
            {
                if (len < sizeof(ext_header.nt))  /* clear remaining part of header if missing */
                    memset( (char *)&ext_header.nt + len, 0, sizeof(ext_header.nt) - len );
                if (res_start) *res_start = (void *)ext_header.nt.OptionalHeader.ImageBase;
                if (res_end) *res_end = (void *)(ext_header.nt.OptionalHeader.ImageBase +
                                                 ext_header.nt.OptionalHeader.SizeOfImage);
                if (ext_header.nt.FileHeader.Characteristics & IMAGE_FILE_DLL) return BINARY_PE_DLL;
                return BINARY_PE_EXE;
            }
            return BINARY_DOS;
        }

        if (!memcmp( &ext_header.os2.ne_magic, "NE", 2 ))
        {
            /* This is a Windows executable (NE) header.  This can
             * mean either a 16-bit OS/2 or a 16-bit Windows or even a
             * DOS program (running under a DOS extender).  To decide
             * which, we'll have to read the NE header.
             */
            if (len >= sizeof(ext_header.os2))
            {
                switch ( ext_header.os2.ne_exetyp )
                {
                case 1:  return BINARY_OS216; /* OS/2 */
                case 2:  return BINARY_WIN16; /* Windows */
                case 3:  return BINARY_DOS; /* European MS-DOS 4.x */
                case 4:  return BINARY_WIN16; /* Windows 386; FIXME: is this 32bit??? */
                case 5:  return BINARY_DOS; /* BOSS, Borland Operating System Services */
                /* other types, e.g. 0 is: "unknown" */
                default: return MODULE_Decide_OS2_OldWin(hfile, &header.mz, &ext_header.os2);
                }
            }
            /* Couldn't read header, so abort. */
            return BINARY_DOS;
        }

        /* Unknown extended header, but this file is nonetheless DOS-executable. */
        return BINARY_DOS;
    }

    return BINARY_UNKNOWN;
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
 *  The type of executable is a property that determines which subsytem an
 *  executable file runs under. lpBinaryType can be set to one of the following
 *  values:
 *   SCS_32BIT_BINARY: A Win32 based application
 *   SCS_DOS_BINARY: An MS-Dos based application
 *   SCS_WOW_BINARY: A Win16 based application
 *   SCS_PIF_BINARY: A PIF file that executes an MS-Dos based app
 *   SCS_POSIX_BINARY: A POSIX based application ( Not implemented )
 *   SCS_OS216_BINARY: A 16bit OS/2 based application
 *
 *  To find the binary type, this function reads in the files header information.
 *  If extended header information is not present it will assume that the file
 *  is a DOS executable. If extended header information is present it will
 *  determine if the file is a 16 or 32 bit Windows executable by checking the
 *  flags in the header.
 *
 *  ".com" and ".pif" files are only recognized by their file name extension,
 *  as per native Windows.
 */
BOOL WINAPI GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    HANDLE hfile;

    TRACE("%s\n", debugstr_w(lpApplicationName) );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Open the file indicated by lpApplicationName for reading.
     */
    hfile = CreateFileW( lpApplicationName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return FALSE;

    /* Check binary type
     */
    switch(MODULE_GetBinaryType( hfile, NULL, NULL ))
    {
    case BINARY_UNKNOWN:
    {
        static const WCHAR comW[] = { '.','C','O','M',0 };
        static const WCHAR pifW[] = { '.','P','I','F',0 };
        const WCHAR *ptr;

        /* try to determine from file name */
        ptr = strrchrW( lpApplicationName, '.' );
        if (!ptr) break;
        if (!strcmpiW( ptr, comW ))
        {
            *lpBinaryType = SCS_DOS_BINARY;
            ret = TRUE;
        }
        else if (!strcmpiW( ptr, pifW ))
        {
            *lpBinaryType = SCS_PIF_BINARY;
            ret = TRUE;
        }
        break;
    }
    case BINARY_PE_EXE:
    case BINARY_PE_DLL:
        *lpBinaryType = SCS_32BIT_BINARY;
        ret = TRUE;
        break;
    case BINARY_WIN16:
        *lpBinaryType = SCS_WOW_BINARY;
        ret = TRUE;
        break;
    case BINARY_OS216:
        *lpBinaryType = SCS_OS216_BINARY;
        ret = TRUE;
        break;
    case BINARY_DOS:
        *lpBinaryType = SCS_DOS_BINARY;
        ret = TRUE;
        break;
    case BINARY_UNIX_EXE:
    case BINARY_UNIX_LIB:
        ret = FALSE;
        break;
    }

    CloseHandle( hfile );
    return ret;
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
    NTSTATUS status;

    TRACE("%s\n", debugstr_a(lpApplicationName));

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    RtlInitAnsiString(&app_nameA, lpApplicationName);
    status = RtlAnsiStringToUnicodeString(&NtCurrentTeb()->StaticUnicodeString,
                                          &app_nameA, FALSE);
    if (!status)
        return GetBinaryTypeW(NtCurrentTeb()->StaticUnicodeString.Buffer, lpBinaryType);

    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
}

/***********************************************************************
 *              GetModuleHandleExA         (KERNEL32.@)
 */
BOOL WINAPI GetModuleHandleExA( DWORD flags, LPCSTR name, HMODULE *module )
{
    WCHAR *nameW;

    if (!name || (flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
        return GetModuleHandleExW( flags, (LPCWSTR)name, module );

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return FALSE;
    return GetModuleHandleExW( flags, nameW, module );
}

/***********************************************************************
 *              GetModuleHandleExW         (KERNEL32.@)
 */
BOOL WINAPI GetModuleHandleExW( DWORD flags, LPCWSTR name, HMODULE *module )
{
    NTSTATUS status = STATUS_SUCCESS;
    HMODULE ret;
    ULONG magic;

    /* if we are messing with the refcount, grab the loader lock */
    if ((flags & GET_MODULE_HANDLE_EX_FLAG_PIN) ||
        !(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
        LdrLockLoaderLock( 0, NULL, &magic );

    if (!name)
    {
        ret = NtCurrentTeb()->Peb->ImageBaseAddress;
    }
    else if (flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
    {
        void *dummy;
        if (!(ret = RtlPcToFileHeader( (void *)name, &dummy ))) status = STATUS_DLL_NOT_FOUND;
    }
    else
    {
        UNICODE_STRING wstr;
        RtlInitUnicodeString( &wstr, name );
        status = LdrGetDllHandle( NULL, 0, &wstr, &ret );
    }

    if (status == STATUS_SUCCESS)
    {
        if (flags & GET_MODULE_HANDLE_EX_FLAG_PIN)
            FIXME( "should pin refcount for %p\n", ret );
        else if (!(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
            LdrAddRefDll( 0, ret );
    }
    else SetLastError( RtlNtStatusToDosError( status ) );

    if ((flags & GET_MODULE_HANDLE_EX_FLAG_PIN) ||
        !(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
        LdrUnlockLoaderLock( 0, magic );

    if (module) *module = ret;
    return (status == STATUS_SUCCESS);
}

/***********************************************************************
 *              GetModuleHandleA         (KERNEL32.@)
 *              GetModuleHandle32        (KERNEL.488)
 *
 * Get the handle of a dll loaded into the process address space.
 *
 * PARAMS
 *  module [I] Name of the dll
 *
 * RETURNS
 *  Success: A handle to the loaded dll.
 *  Failure: A NULL handle. Use GetLastError() to determine the cause.
 */
HMODULE WINAPI GetModuleHandleA(LPCSTR module)
{
    HMODULE ret;

    if (!GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret )) ret = 0;
    return ret;
}

/***********************************************************************
 *		GetModuleHandleW (KERNEL32.@)
 *
 * Unicode version of GetModuleHandleA.
 */
HMODULE WINAPI GetModuleHandleW(LPCWSTR module)
{
    HMODULE ret;

    if (!GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret )) ret = 0;
    return ret;
}


/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.@)
 *              GetModuleFileName32     (KERNEL.487)
 *
 * Get the file name of a loaded module from its handle.
 *
 * RETURNS
 *  Success: The length of the file name, excluding the terminating NUL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  This function always returns the long path of hModule (as opposed to
 *  GetModuleFileName16() which returns short paths when the modules version
 *  field is < 4.0).
 *  The function doesn't write a terminating '\0' if the buffer is too 
 *  small.
 */
DWORD WINAPI GetModuleFileNameA(
	HMODULE hModule,	/* [in] Module handle (32 bit) */
	LPSTR lpFileName,	/* [out] Destination for file name */
        DWORD size )		/* [in] Size of lpFileName in characters */
{
    LPWSTR filenameW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
    DWORD len;

    if (!filenameW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((len = GetModuleFileNameW( hModule, filenameW, size )))
    {
    	len = FILE_name_WtoA( filenameW, len, lpFileName, size );
        if (len < size) lpFileName[len] = '\0';
    }
    HeapFree( GetProcessHeap(), 0, filenameW );
    return len;
}

/***********************************************************************
 *              GetModuleFileNameW      (KERNEL32.@)
 *
 * Unicode version of GetModuleFileNameA.
 */
DWORD WINAPI GetModuleFileNameW( HMODULE hModule, LPWSTR lpFileName, DWORD size )
{
    ULONG magic, len = 0;
    LDR_MODULE *pldr;
    NTSTATUS nts;
    WIN16_SUBSYSTEM_TIB *win16_tib;

    if (!hModule && ((win16_tib = NtCurrentTeb()->Tib.SubSystemTib)) && win16_tib->exe_name)
    {
        len = min(size, win16_tib->exe_name->Length / sizeof(WCHAR));
        memcpy( lpFileName, win16_tib->exe_name->Buffer, len * sizeof(WCHAR) );
        if (len < size) lpFileName[len] = '\0';
        goto done;
    }

    LdrLockLoaderLock( 0, NULL, &magic );

    if (!hModule) hModule = NtCurrentTeb()->Peb->ImageBaseAddress;
    nts = LdrFindEntryForAddress( hModule, &pldr );
    if (nts == STATUS_SUCCESS)
    {
        len = min(size, pldr->FullDllName.Length / sizeof(WCHAR));
        memcpy(lpFileName, pldr->FullDllName.Buffer, len * sizeof(WCHAR));
        if (len < size) lpFileName[len] = '\0';
    }
    else SetLastError( RtlNtStatusToDosError( nts ) );

    LdrUnlockLoaderLock( 0, magic );
done:
    TRACE( "%s\n", debugstr_wn(lpFileName, len) );
    return len;
}


/***********************************************************************
 *           get_dll_system_path
 */
static const WCHAR *get_dll_system_path(void)
{
    static WCHAR *cached_path;

    if (!cached_path)
    {
        WCHAR *p, *path;
        int len = 3;

        len += 2 * GetSystemDirectoryW( NULL, 0 );
        len += GetWindowsDirectoryW( NULL, 0 );
        p = path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        *p++ = '.';
        *p++ = ';';
        GetSystemDirectoryW( p, path + len - p);
        p += strlenW(p);
        /* if system directory ends in "32" add 16-bit version too */
        if (p[-2] == '3' && p[-1] == '2')
        {
            *p++ = ';';
            GetSystemDirectoryW( p, path + len - p);
            p += strlenW(p) - 2;
        }
        *p++ = ';';
        GetWindowsDirectoryW( p, path + len - p);
        cached_path = path;
    }
    return cached_path;
}

/******************************************************************
 *		get_module_path_end
 *
 * Returns the end of the directory component of the module path.
 */
static inline const WCHAR *get_module_path_end(const WCHAR *module)
{
    const WCHAR *p;
    const WCHAR *mod_end = module;
    if (!module) return mod_end;

    if ((p = strrchrW( mod_end, '\\' ))) mod_end = p;
    if ((p = strrchrW( mod_end, '/' ))) mod_end = p;
    if (mod_end == module + 2 && module[1] == ':') mod_end++;
    if (mod_end == module && module[0] && module[1] == ':') mod_end += 2;

    return mod_end;
}

/******************************************************************
 *		MODULE_get_dll_load_path
 *
 * Compute the load path to use for a given dll.
 * Returned pointer must be freed by caller.
 */
WCHAR *MODULE_get_dll_load_path( LPCWSTR module )
{
    static const WCHAR pathW[] = {'P','A','T','H',0};

    const WCHAR *system_path = get_dll_system_path();
    const WCHAR *mod_end = NULL;
    UNICODE_STRING name, value;
    WCHAR *p, *ret;
    int len = 0, path_len = 0;

    /* adjust length for module name */

    if (module)
        mod_end = get_module_path_end( module );
    /* if module is NULL or doesn't contain a path, fall back to directory
     * process was loaded from */
    if (module == mod_end)
    {
        module = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
        mod_end = get_module_path_end( module );
    }
    len += (mod_end - module) + 1;

    len += strlenW( system_path ) + 2;

    /* get the PATH variable */

    RtlInitUnicodeString( &name, pathW );
    value.Length = 0;
    value.MaximumLength = 0;
    value.Buffer = NULL;
    if (RtlQueryEnvironmentVariable_U( NULL, &name, &value ) == STATUS_BUFFER_TOO_SMALL)
        path_len = value.Length;

    RtlEnterCriticalSection( &dlldir_section );
    if (dll_directory) len += strlenW(dll_directory) + 1;
    if ((p = ret = HeapAlloc( GetProcessHeap(), 0, path_len + len * sizeof(WCHAR) )))
    {
        if (module)
        {
            memcpy( ret, module, (mod_end - module) * sizeof(WCHAR) );
            p += (mod_end - module);
            *p++ = ';';
        }
        if (dll_directory)
        {
            strcpyW( p, dll_directory );
            p += strlenW(p);
            *p++ = ';';
        }
    }
    RtlLeaveCriticalSection( &dlldir_section );
    if (!ret) return NULL;

    strcpyW( p, system_path );
    p += strlenW(p);
    *p++ = ';';
    value.Buffer = p;
    value.MaximumLength = path_len;

    while (RtlQueryEnvironmentVariable_U( NULL, &name, &value ) == STATUS_BUFFER_TOO_SMALL)
    {
        WCHAR *new_ptr;

        /* grow the buffer and retry */
        path_len = value.Length;
        if (!(new_ptr = HeapReAlloc( GetProcessHeap(), 0, ret, path_len + len * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }
        value.Buffer = new_ptr + (value.Buffer - ret);
        value.MaximumLength = path_len;
        ret = new_ptr;
    }
    value.Buffer[value.Length / sizeof(WCHAR)] = 0;
    return ret;
}


/******************************************************************
 *		load_library_as_datafile
 */
static BOOL load_library_as_datafile( LPCWSTR name, HMODULE* hmod)
{
    static const WCHAR dotDLL[] = {'.','d','l','l',0};

    WCHAR filenameW[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE mapping;
    HMODULE module;

    *hmod = 0;

    if (SearchPathW( NULL, (LPCWSTR)name, dotDLL, sizeof(filenameW) / sizeof(filenameW[0]),
                     filenameW, NULL ))
    {
        hFile = CreateFileW( filenameW, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, 0 );
    }
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    mapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    CloseHandle( hFile );
    if (!mapping) return FALSE;

    module = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!module) return FALSE;

    /* make sure it's a valid PE file */
    if (!RtlImageNtHeader(module))
    {
        UnmapViewOfFile( module );
        return FALSE;
    }
    *hmod = (HMODULE)((char *)module + 1);  /* set low bit of handle to indicate datafile module */
    return TRUE;
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
    WCHAR *load_path;

    if (flags & LOAD_LIBRARY_AS_DATAFILE)
    {
        /* The method in load_library_as_datafile allows searching for the
         * 'native' libraries only
         */
        if (load_library_as_datafile( libname->Buffer, &hModule )) return hModule;
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    load_path = MODULE_get_dll_load_path( flags & LOAD_WITH_ALTERED_SEARCH_PATH ? libname->Buffer : NULL );
    nts = LdrLoadDll( load_path, flags, libname, &hModule );
    HeapFree( GetProcessHeap(), 0, load_path );
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
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
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
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
HMODULE WINAPI LoadLibraryExW(LPCWSTR libnameW, HANDLE hfile, DWORD flags)
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
HMODULE WINAPI LoadLibraryA(LPCSTR libname)
{
    return LoadLibraryExA(libname, 0, 0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32.@)
 *
 * Unicode version of LoadLibraryA.
 */
HMODULE WINAPI LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW, 0, 0);
}

/***********************************************************************
 *           FreeLibrary   (KERNEL32.@)
 *           FreeLibrary32 (KERNEL.486)
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
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    BOOL                retv = FALSE;
    NTSTATUS            nts;

    if (!hLibModule)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if ((ULONG_PTR)hLibModule & 1)
    {
        /* this is a LOAD_LIBRARY_AS_DATAFILE module */
        char *ptr = (char *)hLibModule - 1;
        UnmapViewOfFile( ptr );
        return TRUE;
    }

    if ((nts = LdrUnloadDll( hLibModule )) == STATUS_SUCCESS) retv = TRUE;
    else SetLastError( RtlNtStatusToDosError( nts ) );

    return retv;
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
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    NTSTATUS    nts;
    FARPROC     fp;

    if (!hModule) hModule = NtCurrentTeb()->Peb->ImageBaseAddress;

    if (HIWORD(function))
    {
        ANSI_STRING     str;

        RtlInitAnsiString( &str, function );
        nts = LdrGetProcedureAddress( hModule, &str, 0, (void**)&fp );
    }
    else
        nts = LdrGetProcedureAddress( hModule, NULL, LOWORD(function), (void**)&fp );
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        fp = NULL;
    }
    return fp;
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
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
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    /* FIXME: we used to disable snoop when returning proc for Win16 subsystem */
    return GetProcAddress( hModule, function );
}


/***********************************************************************
 *           DelayLoadFailureHook  (KERNEL32.@)
 */
FARPROC WINAPI DelayLoadFailureHook( LPCSTR name, LPCSTR function )
{
    ULONG_PTR args[2];

    if ((ULONG_PTR)function >> 16)
        ERR( "failed to delay load %s.%s\n", name, function );
    else
        ERR( "failed to delay load %s.%u\n", name, LOWORD(function) );
    args[0] = (ULONG_PTR)name;
    args[1] = (ULONG_PTR)function;
    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args );
    return NULL;
}
