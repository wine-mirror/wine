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
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

#define NE_FFLAGS_LIBMODULE 0x8000

struct dll_dir_entry
{
    struct list entry;
    WCHAR       dir[1];
};

static struct list dll_dir_list = LIST_INIT( dll_dir_list );  /* extra dirs from AddDllDirectory */
static WCHAR *dll_directory;  /* extra path for SetDllDirectoryW */
static DWORD default_search_flags;  /* default flags set by SetDefaultDllDirectories */

/* to keep track of LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE file handles */
struct exclusive_datafile
{
    struct list entry;
    HMODULE     module;
    HANDLE      file;
};
static struct list exclusive_datafile_list = LIST_INIT( exclusive_datafile_list );

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
 *              AddDllDirectory   (KERNEL32.@)
 */
DLL_DIRECTORY_COOKIE WINAPI AddDllDirectory( const WCHAR *dir )
{
    WCHAR path[MAX_PATH];
    DWORD len;
    struct dll_dir_entry *ptr;
    DOS_PATHNAME_TYPE type = RtlDetermineDosPathNameType_U( dir );

    if (type != ABSOLUTE_PATH && type != ABSOLUTE_DRIVE_PATH)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(len = GetFullPathNameW( dir, MAX_PATH, path, NULL ))) return NULL;
    if (GetFileAttributesW( path ) == INVALID_FILE_ATTRIBUTES) return NULL;

    if (!(ptr = HeapAlloc( GetProcessHeap(), 0, offsetof(struct dll_dir_entry, dir[++len] )))) return NULL;
    memcpy( ptr->dir, path, len * sizeof(WCHAR) );
    TRACE( "%s\n", debugstr_w( ptr->dir ));

    RtlEnterCriticalSection( &dlldir_section );
    list_add_head( &dll_dir_list, &ptr->entry );
    RtlLeaveCriticalSection( &dlldir_section );
    return ptr;
}


/****************************************************************************
 *              RemoveDllDirectory   (KERNEL32.@)
 */
BOOL WINAPI RemoveDllDirectory( DLL_DIRECTORY_COOKIE cookie )
{
    struct dll_dir_entry *ptr = cookie;

    TRACE( "%s\n", debugstr_w( ptr->dir ));

    RtlEnterCriticalSection( &dlldir_section );
    list_remove( &ptr->entry );
    HeapFree( GetProcessHeap(), 0, ptr );
    RtlLeaveCriticalSection( &dlldir_section );
    return TRUE;
}


/*************************************************************************
 *           SetDefaultDllDirectories   (KERNEL32.@)
 */
BOOL WINAPI SetDefaultDllDirectories( DWORD flags )
{
    /* LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR doesn't make sense in default dirs */
    const DWORD load_library_search_flags = (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    if (!flags || (flags & ~load_library_search_flags))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    default_search_flags = flags;
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
 * Reading the module imports is the only reasonable way of discerning
 * old Windows binaries from OS/2 ones.
 */
static DWORD MODULE_Decide_OS2_OldWin(HANDLE hfile, const IMAGE_DOS_HEADER *mz, const IMAGE_OS2_HEADER *ne)
{
    DWORD currpos = SetFilePointer( hfile, 0, NULL, SEEK_CUR);
    DWORD ret = BINARY_OS216;
    LPWORD modtab = NULL;
    LPSTR nametab = NULL;
    DWORD len;
    int i;

    /* read modref table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_modtab, NULL, SEEK_SET ) == -1)
      || (!(modtab = HeapAlloc( GetProcessHeap(), 0, ne->ne_cmod*sizeof(WORD))))
      || (!(ReadFile(hfile, modtab, ne->ne_cmod*sizeof(WORD), &len, NULL)))
      || (len != ne->ne_cmod*sizeof(WORD)) )
	goto done;

    /* read imported names table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_imptab, NULL, SEEK_SET ) == -1)
      || (!(nametab = HeapAlloc( GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab)))
      || (!(ReadFile(hfile, nametab, ne->ne_enttab - ne->ne_imptab, &len, NULL)))
      || (len != ne->ne_enttab - ne->ne_imptab) )
	goto done;

    for (i=0; i < ne->ne_cmod; i++)
    {
        LPSTR module = &nametab[modtab[i]];
        TRACE("modref: %.*s\n", module[0], &module[1]);
        if (!(strncmp(&module[1], "KERNEL", module[0])))
        { /* very old Windows file */
            MESSAGE("This seems to be a very old (pre-3.0) Windows executable. Expect crashes, especially if this is a real-mode binary !\n");
            ret = BINARY_WIN16;
            break;
        }
    }

done:
    HeapFree( GetProcessHeap(), 0, modtab);
    HeapFree( GetProcessHeap(), 0, nametab);
    SetFilePointer( hfile, currpos, NULL, SEEK_SET); /* restore filepos */
    return ret;
}

/***********************************************************************
 *           MODULE_GetBinaryType
 */
void MODULE_get_binary_info( HANDLE hfile, struct binary_info *info )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[8];
            unsigned int phoff;
            unsigned char ignored3[12];
            unsigned short phnum;
        } elf;
        struct
        {
            unsigned char magic[4];
            unsigned char class;
            unsigned char data;
            unsigned char ignored1[10];
            unsigned short type;
            unsigned short machine;
            unsigned char ignored2[12];
            unsigned __int64 phoff;
            unsigned char ignored3[16];
            unsigned short phnum;
        } elf64;
        struct
        {
            unsigned int magic;
            unsigned int cputype;
            unsigned int cpusubtype;
            unsigned int filetype;
        } macho;
        IMAGE_DOS_HEADER mz;
    } header;

    DWORD len;

    memset( info, 0, sizeof(*info) );

    /* Seek to the start of the file and read the header information. */
    if (SetFilePointer( hfile, 0, NULL, SEEK_SET ) == -1) return;
    if (!ReadFile( hfile, &header, sizeof(header), &len, NULL ) || len != sizeof(header)) return;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
#ifdef WORDS_BIGENDIAN
        BOOL byteswap = (header.elf.data == 1);
#else
        BOOL byteswap = (header.elf.data == 2);
#endif
        if (header.elf.class == 2) info->flags |= BINARY_FLAG_64BIT;
        if (byteswap)
        {
            header.elf.type = RtlUshortByteSwap( header.elf.type );
            header.elf.machine = RtlUshortByteSwap( header.elf.machine );
        }
        switch(header.elf.type)
        {
        case 2:
            info->type = BINARY_UNIX_EXE;
            break;
        case 3:
        {
            LARGE_INTEGER phoff;
            unsigned short phnum;
            unsigned int type;
            if (header.elf.class == 2)
            {
                phoff.QuadPart = byteswap ? RtlUlonglongByteSwap( header.elf64.phoff ) : header.elf64.phoff;
                phnum = byteswap ? RtlUshortByteSwap( header.elf64.phnum ) : header.elf64.phnum;
            }
            else
            {
                phoff.QuadPart = byteswap ? RtlUlongByteSwap( header.elf.phoff ) : header.elf.phoff;
                phnum = byteswap ? RtlUshortByteSwap( header.elf.phnum ) : header.elf.phnum;
            }
            while (phnum--)
            {
                if (SetFilePointerEx( hfile, phoff, NULL, FILE_BEGIN ) == -1) return;
                if (!ReadFile( hfile, &type, sizeof(type), &len, NULL ) || len < sizeof(type)) return;
                if (byteswap) type = RtlUlongByteSwap( type );
                if (type == 3)
                {
                    info->type = BINARY_UNIX_EXE;
                    break;
                }
                phoff.QuadPart += (header.elf.class == 2) ? 56 : 32;
            }
            if (!info->type) info->type = BINARY_UNIX_LIB;
            break;
        }
        default:
            return;
        }
        switch(header.elf.machine)
        {
        case 3:   info->arch = IMAGE_FILE_MACHINE_I386; break;
        case 20:  info->arch = IMAGE_FILE_MACHINE_POWERPC; break;
        case 40:  info->arch = IMAGE_FILE_MACHINE_ARMNT; break;
        case 50:  info->arch = IMAGE_FILE_MACHINE_IA64; break;
        case 62:  info->arch = IMAGE_FILE_MACHINE_AMD64; break;
        case 183: info->arch = IMAGE_FILE_MACHINE_ARM64; break;
        }
    }
    /* Mach-o File with Endian set to Big Endian or Little Endian */
    else if (header.macho.magic == 0xfeedface || header.macho.magic == 0xcefaedfe ||
             header.macho.magic == 0xfeedfacf || header.macho.magic == 0xcffaedfe)
    {
        if ((header.macho.cputype >> 24) == 1) info->flags |= BINARY_FLAG_64BIT;
        if (header.macho.magic == 0xcefaedfe || header.macho.magic == 0xcffaedfe)
        {
            header.macho.filetype = RtlUlongByteSwap( header.macho.filetype );
            header.macho.cputype = RtlUlongByteSwap( header.macho.cputype );
        }
        switch(header.macho.filetype)
        {
        case 2: info->type = BINARY_UNIX_EXE; break;
        case 8: info->type = BINARY_UNIX_LIB; break;
        }
        switch(header.macho.cputype)
        {
        case 0x00000007: info->arch = IMAGE_FILE_MACHINE_I386; break;
        case 0x01000007: info->arch = IMAGE_FILE_MACHINE_AMD64; break;
        case 0x0000000c: info->arch = IMAGE_FILE_MACHINE_ARMNT; break;
        case 0x0100000c: info->arch = IMAGE_FILE_MACHINE_ARM64; break;
        case 0x00000012: info->arch = IMAGE_FILE_MACHINE_POWERPC; break;
        }
    }
    /* Not ELF, try DOS */
    else if (header.mz.e_magic == IMAGE_DOS_SIGNATURE)
    {
        union
        {
            IMAGE_OS2_HEADER os2;
            IMAGE_NT_HEADERS32 nt;
            IMAGE_NT_HEADERS64 nt64;
        } ext_header;

        /* We do have a DOS image so we will now try to seek into
         * the file by the amount indicated by the field
         * "Offset to extended header" and read in the
         * "magic" field information at that location.
         * This will tell us if there is more header information
         * to read or not.
         */
        info->type = BINARY_DOS;
        info->arch = IMAGE_FILE_MACHINE_I386;
        if (SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) == -1) return;
        if (!ReadFile( hfile, &ext_header, sizeof(ext_header), &len, NULL ) || len < 4) return;

        /* Reading the magic field succeeded so
         * we will try to determine what type it is.
         */
        if (!memcmp( &ext_header.nt.Signature, "PE\0\0", 4 ))
        {
            if (len >= sizeof(ext_header.nt.FileHeader))
            {
                static const char fakedll_signature[] = "Wine placeholder DLL";
                char buffer[sizeof(fakedll_signature)];

                info->type = BINARY_PE;
                info->arch = ext_header.nt.FileHeader.Machine;
                if (ext_header.nt.FileHeader.Characteristics & IMAGE_FILE_DLL)
                    info->flags |= BINARY_FLAG_DLL;
                if (len < sizeof(ext_header))  /* clear remaining part of header if missing */
                    memset( (char *)&ext_header + len, 0, sizeof(ext_header) - len );
                switch (ext_header.nt.OptionalHeader.Magic)
                {
                case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
                    info->res_start = ext_header.nt.OptionalHeader.ImageBase;
                    info->res_end = info->res_start + ext_header.nt.OptionalHeader.SizeOfImage;
                    break;
                case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
                    info->res_start = ext_header.nt64.OptionalHeader.ImageBase;
                    info->res_end = info->res_start + ext_header.nt64.OptionalHeader.SizeOfImage;
                    info->flags |= BINARY_FLAG_64BIT;
                    break;
                }

                if (header.mz.e_lfanew >= sizeof(header.mz) + sizeof(fakedll_signature) &&
                    SetFilePointer( hfile, sizeof(header.mz), NULL, SEEK_SET ) == sizeof(header.mz) &&
                    ReadFile( hfile, buffer, sizeof(fakedll_signature), &len, NULL ) &&
                    len == sizeof(fakedll_signature) &&
                    !memcmp( buffer, fakedll_signature, sizeof(fakedll_signature) ))
                {
                    info->flags |= BINARY_FLAG_FAKEDLL;
                }
            }
        }
        else if (!memcmp( &ext_header.os2.ne_magic, "NE", 2 ))
        {
            /* This is a Windows executable (NE) header.  This can
             * mean either a 16-bit OS/2 or a 16-bit Windows or even a
             * DOS program (running under a DOS extender).  To decide
             * which, we'll have to read the NE header.
             */
            if (len >= sizeof(ext_header.os2))
            {
                if (ext_header.os2.ne_flags & NE_FFLAGS_LIBMODULE) info->flags |= BINARY_FLAG_DLL;
                switch ( ext_header.os2.ne_exetyp )
                {
                case 1:  info->type = BINARY_OS216; break; /* OS/2 */
                case 2:  info->type = BINARY_WIN16; break; /* Windows */
                case 3:  info->type = BINARY_DOS; break; /* European MS-DOS 4.x */
                case 4:  info->type = BINARY_WIN16; break; /* Windows 386; FIXME: is this 32bit??? */
                case 5:  info->type = BINARY_DOS; break; /* BOSS, Borland Operating System Services */
                /* other types, e.g. 0 is: "unknown" */
                default: info->type = MODULE_Decide_OS2_OldWin(hfile, &header.mz, &ext_header.os2); break;
                }
            }
        }
    }
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
BOOL WINAPI GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    HANDLE hfile;
    struct binary_info binary_info;

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
    MODULE_get_binary_info( hfile, &binary_info );
    switch (binary_info.type)
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
    case BINARY_PE:
        *lpBinaryType = (binary_info.flags & BINARY_FLAG_64BIT) ? SCS_64BIT_BINARY : SCS_32BIT_BINARY;
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
    ULONG_PTR magic;
    BOOL lock;

    if (!module)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* if we are messing with the refcount, grab the loader lock */
    lock = (flags & GET_MODULE_HANDLE_EX_FLAG_PIN) || !(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT);
    if (lock)
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
            LdrAddRefDll( LDR_ADDREF_DLL_PIN, ret );
        else if (!(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
            LdrAddRefDll( 0, ret );
    }
    else SetLastError( RtlNtStatusToDosError( status ) );

    if (lock)
        LdrUnlockLoaderLock( 0, magic );

    if (status == STATUS_SUCCESS) *module = ret;
    else *module = NULL;

    return (status == STATUS_SUCCESS);
}

/***********************************************************************
 *              GetModuleHandleA         (KERNEL32.@)
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
HMODULE WINAPI DECLSPEC_HOTPATCH GetModuleHandleA(LPCSTR module)
{
    HMODULE ret;

    GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
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

    GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
    return ret;
}


/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.@)
 *
 * Get the file name of a loaded module from its handle.
 *
 * RETURNS
 *  Success: The length of the file name, excluding the terminating NUL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  This function always returns the long path of hModule
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
        if (len < size)
            lpFileName[len] = '\0';
        else
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
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
    ULONG len = 0;
    ULONG_PTR magic;
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
        if (len < size)
        {
            lpFileName[len] = '\0';
            SetLastError( 0 );
        }
        else
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
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
        int len = 1;

        len += 2 * GetSystemDirectoryW( NULL, 0 );
        len += GetWindowsDirectoryW( NULL, 0 );
        p = path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
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

/***********************************************************************
 *           get_dll_safe_mode
 */
static BOOL get_dll_safe_mode(void)
{
    static const WCHAR keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                 'M','a','c','h','i','n','e','\\',
                                 'S','y','s','t','e','m','\\',
                                 'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                 'C','o','n','t','r','o','l','\\',
                                 'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const WCHAR valueW[] = {'S','a','f','e','D','l','l','S','e','a','r','c','h','M','o','d','e',0};

    static int safe_mode = -1;

    if (safe_mode == -1)
    {
        char buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING nameW;
        HANDLE hkey;
        DWORD size = sizeof(buffer);

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;

        safe_mode = 1;
        RtlInitUnicodeString( &nameW, keyW );
        if (!NtOpenKey( &hkey, KEY_READ, &attr ))
        {
            RtlInitUnicodeString( &nameW, valueW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, buffer, size, &size ) &&
                info->Type == REG_DWORD && info->DataLength == sizeof(DWORD))
                safe_mode = !!*(DWORD *)info->Data;
            NtClose( hkey );
        }
        if (!safe_mode) TRACE( "SafeDllSearchMode disabled through the registry\n" );
    }
    return safe_mode;
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
 *		append_path_len
 *
 * Append a counted string to the load path. Helper for MODULE_get_dll_load_path.
 */
static inline WCHAR *append_path_len( WCHAR *p, const WCHAR *str, DWORD len )
{
    if (!len) return p;
    memcpy( p, str, len * sizeof(WCHAR) );
    p[len] = ';';
    return p + len + 1;
}


/******************************************************************
 *		append_path
 *
 * Append a string to the load path. Helper for MODULE_get_dll_load_path.
 */
static inline WCHAR *append_path( WCHAR *p, const WCHAR *str )
{
    return append_path_len( p, str, strlenW(str) );
}


/******************************************************************
 *		MODULE_get_dll_load_path
 *
 * Compute the load path to use for a given dll.
 * Returned pointer must be freed by caller.
 */
WCHAR *MODULE_get_dll_load_path( LPCWSTR module, int safe_mode )
{
    static const WCHAR pathW[] = {'P','A','T','H',0};
    static const WCHAR dotW[] = {'.',0};

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
    if (safe_mode == -1) safe_mode = get_dll_safe_mode();
    if (dll_directory) len += strlenW(dll_directory) + 1;
    else len += 2;  /* current directory */
    if ((p = ret = HeapAlloc( GetProcessHeap(), 0, path_len + len * sizeof(WCHAR) )))
    {
        if (module) p = append_path_len( p, module, mod_end - module );

        if (dll_directory) p = append_path( p, dll_directory );
        else if (!safe_mode) p = append_path( p, dotW );

        p = append_path( p, system_path );

        if (!dll_directory && safe_mode) p = append_path( p, dotW );
    }
    RtlLeaveCriticalSection( &dlldir_section );
    if (!ret) return NULL;

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
 *		get_dll_load_path_search_flags
 */
static WCHAR *get_dll_load_path_search_flags( LPCWSTR module, DWORD flags )
{
    const WCHAR *image = NULL, *mod_end, *image_end;
    struct dll_dir_entry *dir;
    WCHAR *p, *ret;
    int len = 1;

    if (flags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
        flags |= (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                  LOAD_LIBRARY_SEARCH_USER_DIRS |
                  LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (flags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)
    {
        DWORD type = RtlDetermineDosPathNameType_U( module );
        if (type != ABSOLUTE_DRIVE_PATH && type != ABSOLUTE_PATH)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return NULL;
        }
        mod_end = get_module_path_end( module );
        len += (mod_end - module) + 1;
    }
    else module = NULL;

    RtlEnterCriticalSection( &dlldir_section );

    if (flags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
    {
        image = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
        image_end = get_module_path_end( image );
        len += (image_end - image) + 1;
    }

    if (flags & LOAD_LIBRARY_SEARCH_USER_DIRS)
    {
        LIST_FOR_EACH_ENTRY( dir, &dll_dir_list, struct dll_dir_entry, entry )
            len += strlenW( dir->dir ) + 1;
        if (dll_directory) len += strlenW(dll_directory) + 1;
    }

    if (flags & LOAD_LIBRARY_SEARCH_SYSTEM32) len += GetSystemDirectoryW( NULL, 0 );

    if ((p = ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        if (module) p = append_path_len( p, module, mod_end - module );
        if (image) p = append_path_len( p, image, image_end - image );
        if (flags & LOAD_LIBRARY_SEARCH_USER_DIRS)
        {
            LIST_FOR_EACH_ENTRY( dir, &dll_dir_list, struct dll_dir_entry, entry )
                p = append_path( p, dir->dir );
            if (dll_directory) p = append_path( p, dll_directory );
        }
        if (flags & LOAD_LIBRARY_SEARCH_SYSTEM32) GetSystemDirectoryW( p, ret + len - p );
        else
        {
            if (p > ret) p--;
            *p = 0;
        }
    }

    RtlLeaveCriticalSection( &dlldir_section );
    return ret;
}


/******************************************************************
 *		load_library_as_datafile
 */
static BOOL load_library_as_datafile( LPCWSTR name, HMODULE *hmod, DWORD flags )
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

    if (SearchPathW( NULL, name, dotDLL, sizeof(filenameW) / sizeof(filenameW[0]),
                     filenameW, NULL ))
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
    WCHAR *load_path;
    const DWORD load_library_search_flags = (LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                             LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    const DWORD unsupported_flags = (LOAD_IGNORE_CODE_AUTHZ_LEVEL |
                                     LOAD_LIBRARY_REQUIRE_SIGNED_TARGET);

    if (!(flags & load_library_search_flags)) flags |= default_search_flags;

    if( flags & unsupported_flags)
        FIXME("unsupported flag(s) used (flags: 0x%08x)\n", flags);

    if (flags & load_library_search_flags)
        load_path = get_dll_load_path_search_flags( libname->Buffer, flags );
    else
        load_path = MODULE_get_dll_load_path( flags & LOAD_WITH_ALTERED_SEARCH_PATH ? libname->Buffer : NULL, -1 );
    if (!load_path) return 0;

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
        if (load_library_as_datafile( libname->Buffer, &hModule, flags ))
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
    HeapFree( GetProcessHeap(), 0, load_path );
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
    BOOL                retv = FALSE;
    NTSTATUS            nts;

    if (!hLibModule)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if ((ULONG_PTR)hLibModule & 3) /* this is a datafile module */
    {
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
        return UnmapViewOfFile( (void *)((ULONG_PTR)hLibModule & ~3) );
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
FARPROC get_proc_address( HMODULE hModule, LPCSTR function )
{
    NTSTATUS    nts;
    FARPROC     fp;

    if (!hModule) hModule = NtCurrentTeb()->Peb->ImageBaseAddress;

    if ((ULONG_PTR)function >> 16)
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
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "subq $0x40,%rsp\n\t"
                   "movaps %xmm0,-0x10(%rbp)\n\t"
                   "movaps %xmm1,-0x20(%rbp)\n\t"
                   "movaps %xmm2,-0x30(%rbp)\n\t"
                   "movaps %xmm3,-0x40(%rbp)\n\t"
                   "call " __ASM_NAME("get_proc_address") "\n\t"
                   "movaps -0x40(%rbp), %xmm3\n\t"
                   "movaps -0x30(%rbp), %xmm2\n\t"
                   "movaps -0x20(%rbp), %xmm1\n\t"
                   "movaps -0x10(%rbp), %xmm0\n\t"
                   "movq %rbp,%rsp\n\t"
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
        if (size) file_name[size - 1] = '\0';
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
