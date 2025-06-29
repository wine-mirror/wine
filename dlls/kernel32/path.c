/*
 * File handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996, 2004 Alexandre Julliard
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
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"

#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_PATHNAME_LEN        1024

static const WCHAR system_dir[] = L"C:\\windows\\system32";

/***********************************************************************
 *           copy_filename_WtoA
 *
 * copy a file name back to OEM/Ansi, but only if the buffer is large enough
 */
static DWORD copy_filename_WtoA( LPCWSTR nameW, LPSTR buffer, DWORD len )
{
    UNICODE_STRING strW;
    DWORD ret;
    BOOL is_ansi = AreFileApisANSI();

    RtlInitUnicodeString( &strW, nameW );

    ret = is_ansi ? RtlUnicodeStringToAnsiSize(&strW) : RtlUnicodeStringToOemSize(&strW);
    if (buffer && ret <= len)
    {
        ANSI_STRING str;

        str.Buffer = buffer;
        str.MaximumLength = min( len, UNICODE_STRING_MAX_CHARS );
        if (is_ansi)
            RtlUnicodeStringToAnsiString( &str, &strW, FALSE );
        else
            RtlUnicodeStringToOemString( &str, &strW, FALSE );
        ret = str.Length;  /* length without terminating 0 */
    }
    return ret;
}

/***********************************************************************
 *           GetShortPathNameA   (KERNEL32.@)
 */
DWORD WINAPI GetShortPathNameA( LPCSTR longpath, LPSTR shortpath, DWORD shortlen )
{
    WCHAR *longpathW;
    WCHAR shortpathW[MAX_PATH];
    DWORD ret;

    TRACE("%s\n", debugstr_a(longpath));

    if (!(longpathW = FILE_name_AtoW( longpath, FALSE ))) return 0;

    ret = GetShortPathNameW(longpathW, shortpathW, MAX_PATH);

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    return copy_filename_WtoA( shortpathW, shortpath, shortlen );
}


/**************************************************************************
 *           CopyFileA   (KERNEL32.@)
 */
BOOL WINAPI CopyFileA( LPCSTR source, LPCSTR dest, BOOL fail_if_exists)
{
    WCHAR *sourceW, *destW;
    BOOL ret;

    if (!(sourceW = FILE_name_AtoW( source, FALSE ))) return FALSE;
    if (!(destW = FILE_name_AtoW( dest, TRUE ))) return FALSE;

    ret = CopyFileW( sourceW, destW, fail_if_exists );

    HeapFree( GetProcessHeap(), 0, destW );
    return ret;
}


/**************************************************************************
 *           CopyFileExA   (KERNEL32.@)
 */
BOOL WINAPI CopyFileExA(LPCSTR sourceFilename, LPCSTR destFilename,
                        LPPROGRESS_ROUTINE progressRoutine, LPVOID appData,
                        LPBOOL cancelFlagPointer, DWORD copyFlags)
{
    WCHAR *sourceW, *destW;
    BOOL ret;

    /* can't use the TEB buffer since we may have a callback routine */
    if (!(sourceW = FILE_name_AtoW( sourceFilename, TRUE ))) return FALSE;
    if (!(destW = FILE_name_AtoW( destFilename, TRUE )))
    {
        HeapFree( GetProcessHeap(), 0, sourceW );
        return FALSE;
    }
    ret = CopyFileExW(sourceW, destW, progressRoutine, appData,
                      cancelFlagPointer, copyFlags);
    HeapFree( GetProcessHeap(), 0, sourceW );
    HeapFree( GetProcessHeap(), 0, destW );
    return ret;
}

/**************************************************************************
 *           MoveFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedA(const char *source, const char *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_a(source), debugstr_a(dest), progress, data, flags, handle);

    return MoveFileWithProgressA(source, dest, progress, data, flags);
}

/**************************************************************************
 *           MoveFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedW(const WCHAR *source, const WCHAR *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_w(source), debugstr_w(dest), progress, data, flags, handle);

    return MoveFileWithProgressW(source, dest, progress, data, flags);
}

/*************************************************************************
 *           CreateFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedA( LPCSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template,
                                                       HANDLE transaction, PUSHORT version,
                                                       PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_a(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileA(name, access, sharing, sa, creation, attributes, template);
}

/*************************************************************************
 *           CreateFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedW( LPCWSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template, HANDLE transaction,
                                                       PUSHORT version, PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_w(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileW(name, access, sharing, sa, creation, attributes, template);
}

/***********************************************************************
 *           CreateDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedA(LPCSTR template, LPCSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_a(template), debugstr_a(path), sa, transaction);
    return CreateDirectoryExA(template, path, sa);
}

/***********************************************************************
 *           CreateDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedW(LPCWSTR template, LPCWSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_w(template), debugstr_w(path), sa, transaction);
    return CreateDirectoryExW(template, path, sa);
}

/***********************************************************************
 *           DeleteFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return DeleteFileA(path);
}

/***********************************************************************
 *           DeleteFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return DeleteFileW(path);
}

/******************************************************************************
 *           FindFirstFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedA( LPCSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_a(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExA(filename, level, data, search_op, filter, flags);
}

/******************************************************************************
 *           FindFirstFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_w(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExW(filename, level, data, search_op, filter, flags);
}


/**************************************************************************
 *           GetFileAttributesTransactedA   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedA(LPCSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(name), transaction);
    return GetFileAttributesExA(name, level, ptr);
}

/**************************************************************************
 *           GetFileAttributesTransactedW   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedW(LPCWSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(name), transaction);
    return GetFileAttributesExW(name, level, ptr);
}

/***********************************************************************
 *           RemoveDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return RemoveDirectoryA(path);
}

/***********************************************************************
 *           RemoveDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return RemoveDirectoryW(path);
}

/**************************************************************************
 *           MoveFileWithProgressA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileWithProgressA( LPCSTR source, LPCSTR dest,
                                   LPPROGRESS_ROUTINE fnProgress,
                                   LPVOID param, DWORD flag )
{
    WCHAR *sourceW, *destW;
    BOOL ret;

    if (!(sourceW = FILE_name_AtoW( source, FALSE ))) return FALSE;
    if (dest)
    {
        if (!(destW = FILE_name_AtoW( dest, TRUE ))) return FALSE;
    }
    else
        destW = NULL;

    ret = MoveFileWithProgressW( sourceW, destW, fnProgress, param, flag );
    HeapFree( GetProcessHeap(), 0, destW );
    return ret;
}

/**************************************************************************
 *           MoveFileExA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExA( LPCSTR source, LPCSTR dest, DWORD flag )
{
    return MoveFileWithProgressA( source, dest, NULL, NULL, flag );
}


/**************************************************************************
 *           MoveFileW   (KERNEL32.@)
 *
 *  Move file or directory
 */
BOOL WINAPI MoveFileW( LPCWSTR source, LPCWSTR dest )
{
    return MoveFileExW( source, dest, MOVEFILE_COPY_ALLOWED );
}


/**************************************************************************
 *           MoveFileA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileA( LPCSTR source, LPCSTR dest )
{
    return MoveFileExA( source, dest, MOVEFILE_COPY_ALLOWED );
}


/***********************************************************************
 *           CreateDirectoryExA   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryExA( LPCSTR template, LPCSTR path, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR *pathW, *templateW = NULL;
    BOOL ret;

    if (!(pathW = FILE_name_AtoW( path, FALSE ))) return FALSE;
    if (template && !(templateW = FILE_name_AtoW( template, TRUE ))) return FALSE;

    ret = CreateDirectoryExW( templateW, pathW, sa );
    HeapFree( GetProcessHeap(), 0, templateW );
    return ret;
}


/***********************************************************************
 *           GetSystemDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetSystemDirectoryW( LPWSTR path, UINT count )
{
    UINT len = ARRAY_SIZE(system_dir);
    if (path && count >= len)
    {
        lstrcpyW( path, system_dir );
        len--;
    }
    return len;
}


/***********************************************************************
 *           GetSystemDirectoryA   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetSystemDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( system_dir, path, count );
}


/***********************************************************************
 *           wine_get_unix_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full Unix file name for a given path.
 * Returned buffer must be freed by caller.
 */
char * CDECL wine_get_unix_file_name( LPCWSTR dosW )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;

    if (!RtlDosPathNameToNtPathName_U( dosW, &nt_name, NULL, NULL )) return NULL;
    InitializeObjectAttributes( &attr, &nt_name, 0, 0, NULL );
    for (;;)
    {
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size )))
        {
            RtlFreeUnicodeString( &nt_name );
            return NULL;
        }
        status = wine_nt_to_unix_file_name( &attr, buffer, &size, FILE_OPEN_IF );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    RtlFreeUnicodeString( &nt_name );
    if (status && status != STATUS_NO_SUCH_FILE)
    {
        HeapFree( GetProcessHeap(), 0, buffer );
        SetLastError( RtlNtStatusToDosError( status ) );
        return NULL;
    }
    return buffer;
}


/***********************************************************************
 *           wine_get_dos_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full DOS file name for a given Unix path.
 * Returned buffer must be freed by caller.
 */
WCHAR * CDECL wine_get_dos_file_name( LPCSTR str )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle = 0;
    WCHAR *buffer = NULL, *nt_str = NULL;
    ULONG res, len = strlen(str) + 1;

    if (str[0] != '/')  /* relative path name */
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
        MultiByteToWideChar( CP_UNIXCP, 0, str, len, buffer, len );
        status = RtlDosPathNameToNtPathName_U_WithStatus( buffer, &nt_name, NULL, NULL );
        if (!set_ntstatus( status )) goto failed;
        buffer = nt_name.Buffer;
    }
    else
    {
        if (!(nt_str = RtlAllocateHeap( GetProcessHeap(), 0, (len + 8) * sizeof(WCHAR) ))) return NULL;
        wcscpy( nt_str, L"\\??\\unix" );
        MultiByteToWideChar( CP_UNIXCP, 0, str, len, nt_str + 8, len );
        for (WCHAR *p = nt_str; *p; p++) if (*p == '/') *p = '\\';
        RtlInitUnicodeString( &nt_name, nt_str );
        InitializeObjectAttributes( &attr, &nt_name, 0, 0, NULL );
        status = NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT );
        while (status)
        {
            ULONG i = nt_name.Length / sizeof(WCHAR);
            while (i && nt_name.Buffer[i - 1] != '\\') i--;
            while (i && nt_name.Buffer[i - 1] == '\\') i--;
            if (i <= 9) break;
            nt_name.Length = i * sizeof(WCHAR);
            status = NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
        }
        if (!set_ntstatus( status )) goto failed;
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, (len + 8) * sizeof(WCHAR) ))) goto failed;
        res = GetFinalPathNameByHandleW( handle, buffer, len + 8, VOLUME_NAME_DOS );
        if (!res || res > len + 8) goto failed;
        wcscat( buffer, nt_name.Buffer + nt_name.Length / sizeof(WCHAR) );
        NtClose( handle );
        RtlFreeHeap( GetProcessHeap(), 0, nt_str );
    }
    if (buffer[5] == ':')
    {
        /* get rid of the \??\ prefix */
        /* FIXME: should implement RtlNtPathNameToDosPathName and use that instead */
        memmove( buffer, buffer + 4, (wcslen(buffer + 4) + 1) * sizeof(WCHAR) );
    }
    else buffer[1] = '\\';
    return buffer;

 failed:
    if (handle) NtClose( handle );
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    RtlFreeHeap( GetProcessHeap(), 0, nt_str );
    return NULL;
}

/*************************************************************************
 *           CreateSymbolicLinkA   (KERNEL32.@)
 */
BOOLEAN WINAPI CreateSymbolicLinkA(LPCSTR link, LPCSTR target, DWORD flags)
{
    FIXME("(%s %s %ld): stub\n", debugstr_a(link), debugstr_a(target), flags);
    return TRUE;
}

/*************************************************************************
 *           CreateHardLinkTransactedA   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedA(LPCSTR link, LPCSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_a(link), debugstr_a(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*************************************************************************
 *           CreateHardLinkTransactedW   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedW(LPCWSTR link, LPCWSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_w(link), debugstr_w(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*************************************************************************
 *           CheckNameLegalDOS8Dot3A   (KERNEL32.@)
 */
BOOL WINAPI CheckNameLegalDOS8Dot3A(const char *name, char *oemname, DWORD oemname_len,
        BOOL *contains_spaces, BOOL *is_legal)
{
    WCHAR *nameW;

    TRACE("(%s %p %lu %p %p)\n", name, oemname,
            oemname_len, contains_spaces, is_legal);

    if (!name || !is_legal)
        return FALSE;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return FALSE;

    return CheckNameLegalDOS8Dot3W( nameW, oemname, oemname_len, contains_spaces, is_legal );
}

/*************************************************************************
 *           CheckNameLegalDOS8Dot3W   (KERNEL32.@)
 */
BOOL WINAPI CheckNameLegalDOS8Dot3W(const WCHAR *name, char *oemname, DWORD oemname_len,
        BOOL *contains_spaces_ret, BOOL *is_legal)
{
    OEM_STRING oem_str;
    UNICODE_STRING nameW;
    BOOLEAN contains_spaces;

    TRACE("(%s %p %lu %p %p)\n", wine_dbgstr_w(name), oemname,
          oemname_len, contains_spaces_ret, is_legal);

    if (!name || !is_legal)
        return FALSE;

    RtlInitUnicodeString( &nameW, name );

    if (oemname) {
        oem_str.Length = oemname_len;
        oem_str.MaximumLength = oemname_len;
        oem_str.Buffer = oemname;
    }

    *is_legal = RtlIsNameLegalDOS8Dot3( &nameW, oemname ? &oem_str : NULL, &contains_spaces );
    if (contains_spaces_ret) *contains_spaces_ret = contains_spaces;

    return TRUE;
}

/*************************************************************************
 *           SetSearchPathMode   (KERNEL32.@)
 */
BOOL WINAPI SetSearchPathMode( DWORD flags )
{
    return set_ntstatus( RtlSetSearchPathMode( flags ));
}
