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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/winbase16.h"
#include "winerror.h"
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "heap.h"
#include "thread.h"
#include "file.h"
#include "module.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(win32);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);


/****************************************************************************
 *              DisableThreadLibraryCalls (KERNEL32.@)
 *
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
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
    ERR("Hmm, an error occurred. Is this binary file broken ?\n");

good:
    HeapFree( GetProcessHeap(), 0, modtab);
    HeapFree( GetProcessHeap(), 0, nametab);
    SetFilePointer( hfile, currpos, NULL, SEEK_SET); /* restore filepos */
    return ret;
}

/***********************************************************************
 *           MODULE_GetBinaryType
 */
enum binary_type MODULE_GetBinaryType( HANDLE hfile )
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

    char magic[4];
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

    /* Mach-o File with Endian set to Big Endian  or Little Endian*/
    if (header.macho.magic == 0xfeedface || header.macho.magic == 0xecafdeef)
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
        /* We do have a DOS image so we will now try to seek into
         * the file by the amount indicated by the field
         * "Offset to extended header" and read in the
         * "magic" field information at that location.
         * This will tell us if there is more header information
         * to read or not.
         */
        /* But before we do we will make sure that header
         * structure encompasses the "Offset to extended header"
         * field.
         */
        if (header.mz.e_lfanew < sizeof(IMAGE_DOS_HEADER))
            return BINARY_DOS;
        if (SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) == -1)
            return BINARY_DOS;
        if (!ReadFile( hfile, magic, sizeof(magic), &len, NULL ) || len != sizeof(magic))
            return BINARY_DOS;

        /* Reading the magic field succeeded so
         * we will try to determine what type it is.
         */
        if (!memcmp( magic, "PE\0\0", 4 ))
        {
            IMAGE_FILE_HEADER FileHeader;

            if (ReadFile( hfile, &FileHeader, sizeof(FileHeader), &len, NULL ) && len == sizeof(FileHeader))
            {
                if (FileHeader.Characteristics & IMAGE_FILE_DLL) return BINARY_PE_DLL;
                return BINARY_PE_EXE;
            }
            return BINARY_DOS;
        }

        if (!memcmp( magic, "NE", 2 ))
        {
            /* This is a Windows executable (NE) header.  This can
             * mean either a 16-bit OS/2 or a 16-bit Windows or even a
             * DOS program (running under a DOS extender).  To decide
             * which, we'll have to read the NE header.
             */
            IMAGE_OS2_HEADER ne;
            if (    SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) != -1
                    && ReadFile( hfile, &ne, sizeof(ne), &len, NULL )
                    && len == sizeof(ne) )
            {
                switch ( ne.ne_exetyp )
                {
                case 2:  return BINARY_WIN16;
                case 5:  return BINARY_DOS;
                default: return MODULE_Decide_OS2_OldWin(hfile, &header.mz, &ne);
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
 *             GetBinaryTypeA                     [KERNEL32.@]
 *             GetBinaryType                      [KERNEL32.@]
 *
 * The GetBinaryType function determines whether a file is executable
 * or not and if it is it returns what type of executable it is.
 * The type of executable is a property that determines in which
 * subsystem an executable file runs under.
 *
 * Binary types returned:
 * SCS_32BIT_BINARY: A Win32 based application
 * SCS_DOS_BINARY: An MS-Dos based application
 * SCS_WOW_BINARY: A Win16 based application
 * SCS_PIF_BINARY: A PIF file that executes an MS-Dos based app
 * SCS_POSIX_BINARY: A POSIX based application ( Not implemented )
 * SCS_OS216_BINARY: A 16bit OS/2 based application
 *
 * Returns TRUE if the file is an executable in which case
 * the value pointed by lpBinaryType is set.
 * Returns FALSE if the file is not an executable or if the function fails.
 *
 * To do so it opens the file and reads in the header information
 * if the extended header information is not present it will
 * assume that the file is a DOS executable.
 * If the extended header information is present it will
 * determine if the file is a 16 or 32 bit Windows executable
 * by check the flags in the header.
 *
 * Note that .COM and .PIF files are only recognized by their
 * file name extension; but Windows does it the same way ...
 */
BOOL WINAPI GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    HANDLE hfile;
    char *ptr;

    TRACE_(win32)("%s\n", lpApplicationName );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Open the file indicated by lpApplicationName for reading.
     */
    hfile = CreateFileA( lpApplicationName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return FALSE;

    /* Check binary type
     */
    switch(MODULE_GetBinaryType( hfile ))
    {
    case BINARY_UNKNOWN:
        /* try to determine from file name */
        ptr = strrchr( lpApplicationName, '.' );
        if (!ptr) break;
        if (!FILE_strcasecmp( ptr, ".COM" ))
        {
            *lpBinaryType = SCS_DOS_BINARY;
            ret = TRUE;
        }
        else if (!FILE_strcasecmp( ptr, ".PIF" ))
        {
            *lpBinaryType = SCS_PIF_BINARY;
            ret = TRUE;
        }
        break;
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
 *             GetBinaryTypeW                      [KERNEL32.@]
 */
BOOL WINAPI GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    LPSTR strNew = NULL;

    TRACE_(win32)("%s\n", debugstr_w(lpApplicationName) );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Convert the wide string to a ascii string.
     */
    strNew = HEAP_strdupWtoA( GetProcessHeap(), 0, lpApplicationName );

    if ( strNew != NULL )
    {
        ret = GetBinaryTypeA( strNew, lpBinaryType );

        /* Free the allocated string.
         */
        HeapFree( GetProcessHeap(), 0, strNew );
    }

    return ret;
}


/***********************************************************************
 *              GetModuleHandleA         (KERNEL32.@)
 *              GetModuleHandle32        (KERNEL.488)
 */
HMODULE WINAPI GetModuleHandleA(LPCSTR module)
{
    NTSTATUS            nts;
    HMODULE             ret;
    UNICODE_STRING      wstr;

    if (!module) return NtCurrentTeb()->Peb->ImageBaseAddress;

    RtlCreateUnicodeStringFromAsciiz(&wstr, module);
    nts = LdrGetDllHandle(0, 0, &wstr, &ret);
    RtlFreeUnicodeString( &wstr );
    if (nts != STATUS_SUCCESS)
    {
        ret = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
    return ret;
}

/***********************************************************************
 *		GetModuleHandleW (KERNEL32.@)
 */
HMODULE WINAPI GetModuleHandleW(LPCWSTR module)
{
    NTSTATUS            nts;
    HMODULE             ret;
    UNICODE_STRING      wstr;

    if (!module) return NtCurrentTeb()->Peb->ImageBaseAddress;

    RtlInitUnicodeString( &wstr, module );
    nts = LdrGetDllHandle( 0, 0, &wstr, &ret);
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        ret = 0;
    }
    return ret;
}


/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.@)
 *              GetModuleFileName32     (KERNEL.487)
 *
 * GetModuleFileNameA seems to *always* return the long path;
 * it's only GetModuleFileName16 that decides between short/long path
 * by checking if exe version >= 4.0.
 * (SDK docu doesn't mention this)
 */
DWORD WINAPI GetModuleFileNameA(
	HMODULE hModule,	/* [in] module handle (32bit) */
	LPSTR lpFileName,	/* [out] filenamebuffer */
        DWORD size )		/* [in] size of filenamebuffer */
{
    LPWSTR filenameW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );

    if (!filenameW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    GetModuleFileNameW( hModule, filenameW, size );
    WideCharToMultiByte( CP_ACP, 0, filenameW, -1, lpFileName, size, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, filenameW );
    return strlen( lpFileName );
}

/***********************************************************************
 *              GetModuleFileNameW      (KERNEL32.@)
 */
DWORD WINAPI GetModuleFileNameW( HMODULE hModule, LPWSTR lpFileName, DWORD size )
{
    ULONG magic;

    lpFileName[0] = 0;

    LdrLockLoaderLock( 0, NULL, &magic );
    if (!hModule && !(NtCurrentTeb()->tibflags & TEBF_WIN32))
    {
        /* 16-bit task - get current NE module name */
        NE_MODULE *pModule = NE_GetPtr( GetCurrentTask() );
        if (pModule)
        {
            WCHAR    path[MAX_PATH];

            MultiByteToWideChar( CP_ACP, 0, NE_MODULE_NAME(pModule), -1, path, MAX_PATH );
            GetLongPathNameW(path, lpFileName, size);
        }
    }
    else
    {
        LDR_MODULE* pldr;
        NTSTATUS    nts;

        if (!hModule) hModule = NtCurrentTeb()->Peb->ImageBaseAddress;
        nts = LdrFindEntryForAddress( hModule, &pldr );
        if (nts == STATUS_SUCCESS) lstrcpynW(lpFileName, pldr->FullDllName.Buffer, size);
        else SetLastError( RtlNtStatusToDosError( nts ) );

    }
    LdrUnlockLoaderLock( 0, magic );

    TRACE( "%s\n", debugstr_w(lpFileName) );
    return strlenW(lpFileName);
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
 *		LoadLibraryExA          (KERNEL32.@)
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module representations.
 *
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
    UNICODE_STRING      wstr;
    NTSTATUS            nts;
    HMODULE             hModule;

    if (!libname)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    RtlCreateUnicodeStringFromAsciiz( &wstr, libname );

    if (flags & LOAD_LIBRARY_AS_DATAFILE)
    {
        /* The method in load_library_as_datafile allows searching for the
         * 'native' libraries only
         */
        if (load_library_as_datafile( wstr.Buffer, &hModule))
        {
            RtlFreeUnicodeString( &wstr );
            return hModule;
        }
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    nts = LdrLoadDll(NULL, flags, &wstr, &hModule);
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
    RtlFreeUnicodeString( &wstr );

    return hModule;
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryExW(LPCWSTR libnameW, HANDLE hfile, DWORD flags)
{
    UNICODE_STRING      wstr;
    NTSTATUS            nts;
    HMODULE             hModule;

    if (!libnameW)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (flags & LOAD_LIBRARY_AS_DATAFILE)
    {
        /* The method in load_library_as_datafile allows searching for the
         * 'native' libraries only
         */
        if (load_library_as_datafile(libnameW, &hModule)) return hModule;
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    RtlInitUnicodeString( &wstr, libnameW );
    nts = LdrLoadDll(NULL, flags, &wstr, &hModule);
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
    return hModule;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryA(LPCSTR libname)
{
    return LoadLibraryExA(libname, 0, 0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW, 0, 0);
}

/***********************************************************************
 *           FreeLibrary   (KERNEL32.@)
 *           FreeLibrary32 (KERNEL.486)
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
 */
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    NTSTATUS    nts;
    FARPROC     fp;

    if (HIWORD(function))
    {
        ANSI_STRING     str;

        RtlInitAnsiString( &str, function );
        nts = LdrGetProcedureAddress( hModule, &str, 0, (void**)&fp );
    }
    else
        nts = LdrGetProcedureAddress( hModule, NULL, (DWORD)function, (void**)&fp );
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        fp = NULL;
    }
    return fp;
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    /* FIXME: we used to disable snoop when returning proc for Win16 subsystem */
    return GetProcAddress( hModule, function );
}
