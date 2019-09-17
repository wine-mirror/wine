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

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "kernel_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_PATHNAME_LEN        1024

static int path_safe_mode = -1;  /* path mode set by SetSearchPathMode */

/* check if a file name is for an executable file (.exe or .com) */
static inline BOOL is_executable( const WCHAR *name )
{
    static const WCHAR exeW[] = {'.','e','x','e',0};
    static const WCHAR comW[] = {'.','c','o','m',0};
    int len = strlenW(name);

    if (len < 4) return FALSE;
    return (!strcmpiW( name + len - 4, exeW ) || !strcmpiW( name + len - 4, comW ));
}

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
 *           add_boot_rename_entry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registry key is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL add_boot_rename_entry( LPCWSTR source, LPCWSTR dest, DWORD flags )
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE Reboot = 0;
    DWORD len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

    if (!RtlDosPathNameToNtPathName_U( source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (dest && !RtlDosPathNameToNtPathName_U( dest, &dest_name, NULL, NULL ))
    {
        RtlFreeUnicodeString( &source_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, SessionW );

    if (NtCreateKey( &Reboot, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        WARN("Error creating key for reboot management [%s]\n",
             "SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        RtlFreeUnicodeString( &source_name );
        RtlFreeUnicodeString( &dest_name );
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (dest)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                         NULL, 0, &DataSize ) == STATUS_BUFFER_TOO_SMALL)
    {
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
        if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                             Buffer, DataSize, &DataSize )) goto Quit;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)Buffer;
        if (info->Type != REG_MULTI_SZ) goto Quit;
        if (DataSize > sizeof(info)) DataSize -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        DataSize = info_size;
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
    }

    memcpy( Buffer + DataSize, source_name.Buffer, len1 );
    DataSize += len1;
    p = (WCHAR *)(Buffer + DataSize);
    if (dest)
    {
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        memcpy( p, dest_name.Buffer, len2 );
        DataSize += len2;
    }
    else
    {
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    RtlFreeUnicodeString( &source_name );
    RtlFreeUnicodeString( &dest_name );
    if (Reboot) NtClose(Reboot);
    HeapFree( GetProcessHeap(), 0, Buffer );
    return(rc);
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

/***********************************************************************
 *           get_path_safe_mode
 */
static BOOL get_path_safe_mode(void)
{
    static const WCHAR keyW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                 'M','a','c','h','i','n','e','\\',
                                 'S','y','s','t','e','m','\\',
                                 'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                 'C','o','n','t','r','o','l','\\',
                                 'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const WCHAR valueW[] = {'S','a','f','e','P','r','o','c','e','s','s','S','e','a','r','c','h','M','o','d','e',0};

    if (path_safe_mode == -1)
    {
        char buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING nameW;
        HANDLE hkey;
        DWORD size = sizeof(buffer);
        BOOL mode = FALSE;

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;

        RtlInitUnicodeString( &nameW, keyW );
        if (!NtOpenKey( &hkey, KEY_READ, &attr ))
        {
            RtlInitUnicodeString( &nameW, valueW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, buffer, size, &size ) &&
                info->Type == REG_DWORD && info->DataLength == sizeof(DWORD))
                mode = !!*(DWORD *)info->Data;
            NtClose( hkey );
        }
        InterlockedCompareExchange( &path_safe_mode, mode, -1 );
    }
    return path_safe_mode != 0;
}


/***********************************************************************
 *           contains_pathW
 *
 * Check if the file name contains a path; helper for SearchPathW.
 * A relative path is not considered a path unless it starts with ./ or ../
 */
static inline BOOL contains_pathW (LPCWSTR name)
{
    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH) return TRUE;
    if (name[0] != '.') return FALSE;
    if (name[1] == '/' || name[1] == '\\') return TRUE;
    return (name[1] == '.' && (name[2] == '/' || name[2] == '\\'));
}

/***********************************************************************
 *	find_actctx_dllpath
 *
 * Find the path (if any) of the dll from the activation context.
 * Returned path doesn't include a name.
 */
static NTSTATUS find_actctx_dllpath(const WCHAR *libname, WCHAR **path)
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\'};
    static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    RtlInitUnicodeString( &nameW, libname );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    for (;;)
    {
        if (!(info = HeapAlloc( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        HeapFree( GetProcessHeap(), 0, info );
        size = needed;
        /* restart with larger buffer */
    }

    if (!info->lpAssemblyManifestPath || !info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    if ((p = strrchrW( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);

        p++;
        if (strncmpiW( p, info->lpAssemblyDirectoryName, dirlen ) || strcmpiW( p + dirlen, dotManifestW ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR);
            if (!(*path = p = HeapAlloc( GetProcessHeap(), 0, needed )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            *(p + dirlen) = 0;
            goto done;
        }
    }

    needed = (strlenW( DIR_Windows ) * sizeof(WCHAR) +
              sizeof(winsxsW) + info->ulAssemblyDirectoryNameLength + 2*sizeof(WCHAR));

    if (!(*path = p = HeapAlloc( GetProcessHeap(), 0, needed )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    strcpyW( p, DIR_Windows );
    p += strlenW(p);
    memcpy( p, winsxsW, sizeof(winsxsW) );
    p += ARRAY_SIZE( winsxsW );
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = '\\';
    *p = 0;
done:
    HeapFree( GetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    return status;
}

/***********************************************************************
 * SearchPathW [KERNEL32.@]
 *
 * Searches for a specified file in the search path.
 *
 * PARAMS
 *    path	[I] Path to search (NULL means default)
 *    name	[I] Filename to search for.
 *    ext	[I] File extension to append to file name. The first
 *		    character must be a period. This parameter is
 *                  specified only if the filename given does not
 *                  contain an extension.
 *    buflen	[I] size of buffer, in characters
 *    buffer	[O] buffer for found filename
 *    lastpart  [O] address of pointer to last used character in
 *                  buffer (the final '\')
 *
 * RETURNS
 *    Success: length of string copied into buffer, not including
 *             terminating null character. If the filename found is
 *             longer than the length of the buffer, the length of the
 *             filename is returned.
 *    Failure: Zero
 *
 * NOTES
 *    If the file is not found, calls SetLastError(ERROR_FILE_NOT_FOUND)
 *    (tested on NT 4.0)
 */
DWORD WINAPI SearchPathW( LPCWSTR path, LPCWSTR name, LPCWSTR ext, DWORD buflen,
                          LPWSTR buffer, LPWSTR *lastpart )
{
    DWORD ret = 0;

    if (!name || !name[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* If the name contains an explicit path, ignore the path */

    if (contains_pathW(name))
    {
        /* try first without extension */
        if (RtlDoesFileExists_U( name ))
            return GetFullPathNameW( name, buflen, buffer, lastpart );

        if (ext)
        {
            LPCWSTR p = strrchrW( name, '.' );
            if (p && !strchrW( p, '/' ) && !strchrW( p, '\\' ))
                ext = NULL;  /* Ignore the specified extension */
        }

        /* Allocate a buffer for the file name and extension */
        if (ext)
        {
            LPWSTR tmp;
            DWORD len = strlenW(name) + strlenW(ext);

            if (!(tmp = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
            strcpyW( tmp, name );
            strcatW( tmp, ext );
            if (RtlDoesFileExists_U( tmp ))
                ret = GetFullPathNameW( tmp, buflen, buffer, lastpart );
            HeapFree( GetProcessHeap(), 0, tmp );
        }
    }
    else if (path && path[0])  /* search in the specified path */
    {
        ret = RtlDosSearchPath_U( path, name, ext, buflen * sizeof(WCHAR),
                                  buffer, lastpart ) / sizeof(WCHAR);
    }
    else  /* search in active context and default path */
    {
        WCHAR *dll_path = NULL, *search = NULL;
        DWORD req_len, name_len;

        req_len = name_len = strlenW(name);

        if (strchrW( name, '.' )) ext = NULL;
        if (ext)
        {
            DWORD ext_len = strlenW(ext);

            req_len += ext_len;
            name_len += ext_len;

            search = HeapAlloc( GetProcessHeap(), 0, (name_len + ext_len + 1) * sizeof(WCHAR) );
            if (!search)
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
            strcpyW( search, name );
            strcatW( search, ext );
            name = search;

            /* now that we have combined name we don't need extension any more */
        }

        /* When file is found with activation context no attempt is made
          to check if it's really exist, path is returned only basing on context info. */
        if (find_actctx_dllpath( name, &dll_path ) == STATUS_SUCCESS)
        {
            DWORD path_len;

            path_len = strlenW(dll_path);
            req_len += path_len;

            if (lastpart) *lastpart = NULL;

            /* count null termination char too */
            if (req_len + 1 <= buflen)
            {
                memcpy( buffer, dll_path, path_len * sizeof(WCHAR) );
                memcpy( &buffer[path_len], name, name_len * sizeof(WCHAR) );
                buffer[req_len] = 0;
                if (lastpart) *lastpart = buffer + path_len;
                ret = req_len;
            }
            else
                ret = req_len + 1;

            HeapFree( GetProcessHeap(), 0, dll_path );
            HeapFree( GetProcessHeap(), 0, search );
        }
        else
        {
            if ((dll_path = MODULE_get_dll_load_path( NULL, get_path_safe_mode() )))
            {
                ret = RtlDosSearchPath_U( dll_path, name, NULL, buflen * sizeof(WCHAR),
                                          buffer, lastpart ) / sizeof(WCHAR);
                HeapFree( GetProcessHeap(), 0, dll_path );
                HeapFree( GetProcessHeap(), 0, search );
            }
            else
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
        }
    }

    if (!ret) SetLastError( ERROR_FILE_NOT_FOUND );
    else TRACE( "found %s\n", debugstr_w(buffer) );
    return ret;
}


/***********************************************************************
 *           SearchPathA   (KERNEL32.@)
 *
 * See SearchPathW.
 */
DWORD WINAPI SearchPathA( LPCSTR path, LPCSTR name, LPCSTR ext,
                          DWORD buflen, LPSTR buffer, LPSTR *lastpart )
{
    WCHAR *pathW = NULL, *nameW, *extW = NULL;
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (!name)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return 0;
    if (path && !(pathW = FILE_name_AtoW( path, TRUE ))) return 0;
    
    if (ext && !(extW = FILE_name_AtoW( ext, TRUE )))
    {
        HeapFree( GetProcessHeap(), 0, pathW );
        return 0;
    }

    ret = SearchPathW(pathW, nameW, extW, MAX_PATH, bufferW, NULL);

    HeapFree( GetProcessHeap(), 0, pathW );
    HeapFree( GetProcessHeap(), 0, extW );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, buflen );
    if (buflen > ret && lastpart)
        *lastpart = strrchr(buffer, '\\') + 1;
    return ret;
}

static BOOL is_same_file(HANDLE h1, HANDLE h2)
{
    int fd1;
    BOOL ret = FALSE;
    if (wine_server_handle_to_fd(h1, 0, &fd1, NULL) == STATUS_SUCCESS)
    {
        int fd2;
        if (wine_server_handle_to_fd(h2, 0, &fd2, NULL) == STATUS_SUCCESS)
        {
            struct stat stat1, stat2;
            if (fstat(fd1, &stat1) == 0 && fstat(fd2, &stat2) == 0)
                ret = (stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino);
            wine_server_release_fd(h2, fd2);
        }
        wine_server_release_fd(h1, fd1);
    }
    return ret;
}

/**************************************************************************
 *           CopyFileW   (KERNEL32.@)
 */
BOOL WINAPI CopyFileW( LPCWSTR source, LPCWSTR dest, BOOL fail_if_exists )
{
    return CopyFileExW( source, dest, NULL, NULL, NULL,
                        fail_if_exists ? COPY_FILE_FAIL_IF_EXISTS : 0 );
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
 *           CopyFileExW   (KERNEL32.@)
 */
BOOL WINAPI CopyFileExW(LPCWSTR source, LPCWSTR dest,
                        LPPROGRESS_ROUTINE progress, LPVOID param,
                        LPBOOL cancel_ptr, DWORD flags)
{
    static const int buffer_size = 65536;
    HANDLE h1, h2;
    BY_HANDLE_FILE_INFORMATION info;
    DWORD count;
    BOOL ret = FALSE;
    char *buffer;

    if (!source || !dest)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_size )))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    TRACE("%s -> %s, %x\n", debugstr_w(source), debugstr_w(dest), flags);

    if ((h1 = CreateFileW(source, GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open source %s\n", debugstr_w(source));
        HeapFree( GetProcessHeap(), 0, buffer );
        return FALSE;
    }

    if (!GetFileInformationByHandle( h1, &info ))
    {
        WARN("GetFileInformationByHandle returned error for %s\n", debugstr_w(source));
        HeapFree( GetProcessHeap(), 0, buffer );
        CloseHandle( h1 );
        return FALSE;
    }

    if (!(flags & COPY_FILE_FAIL_IF_EXISTS))
    {
        BOOL same_file = FALSE;
        h2 = CreateFileW( dest, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, 0, 0);
        if (h2 != INVALID_HANDLE_VALUE)
        {
            same_file = is_same_file( h1, h2 );
            CloseHandle( h2 );
        }
        if (same_file)
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            CloseHandle( h1 );
            SetLastError( ERROR_SHARING_VIOLATION );
            return FALSE;
        }
    }

    if ((h2 = CreateFileW( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             (flags & COPY_FILE_FAIL_IF_EXISTS) ? CREATE_NEW : CREATE_ALWAYS,
                             info.dwFileAttributes, h1 )) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open dest %s\n", debugstr_w(dest));
        HeapFree( GetProcessHeap(), 0, buffer );
        CloseHandle( h1 );
        return FALSE;
    }

    while (ReadFile( h1, buffer, buffer_size, &count, NULL ) && count)
    {
        char *p = buffer;
        while (count != 0)
        {
            DWORD res;
            if (!WriteFile( h2, p, count, &res, NULL ) || !res) goto done;
            p += res;
            count -= res;
        }
    }
    ret =  TRUE;
done:
    /* Maintain the timestamp of source file to destination file */
    SetFileTime(h2, NULL, NULL, &info.ftLastWriteTime);
    HeapFree( GetProcessHeap(), 0, buffer );
    CloseHandle( h1 );
    CloseHandle( h2 );
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
    FIXME("(%s, %s, %p, %p, %d, %p)\n", debugstr_a(source), debugstr_a(dest), progress, data, flags, handle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**************************************************************************
 *           MoveFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedW(const WCHAR *source, const WCHAR *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %d, %p)\n", debugstr_w(source), debugstr_w(dest), progress, data, flags, handle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**************************************************************************
 *           MoveFileWithProgressW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileWithProgressW( LPCWSTR source, LPCWSTR dest,
                                   LPPROGRESS_ROUTINE fnProgress,
                                   LPVOID param, DWORD flag )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE source_handle = 0, dest_handle = 0;
    ANSI_STRING source_unix, dest_unix;
    DWORD options;

    TRACE("(%s,%s,%p,%p,%04x)\n",
          debugstr_w(source), debugstr_w(dest), fnProgress, param, flag );

    if (flag & MOVEFILE_DELAY_UNTIL_REBOOT)
        return add_boot_rename_entry( source, dest, flag );

    if (!dest)
        return DeleteFileW( source );

    /* check if we are allowed to rename the source */

    if (!RtlDosPathNameToNtPathName_U( source, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    source_unix.Buffer = NULL;
    dest_unix.Buffer = NULL;
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &source_handle, DELETE | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT );
    if (status == STATUS_SUCCESS)
        status = wine_nt_to_unix_file_name( &nt_name, &source_unix, FILE_OPEN, FALSE );
    RtlFreeUnicodeString( &nt_name );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }
    status = NtQueryInformationFile( source_handle, &io, &info, sizeof(info), FileBasicInformation );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }

    /* we must have write access to the destination, and it must */
    /* not exist except if MOVEFILE_REPLACE_EXISTING is set */

    if (!RtlDosPathNameToNtPathName_U( dest, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto error;
    }
    options = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;
    if (flag & MOVEFILE_WRITE_THROUGH)
        options |= FILE_WRITE_THROUGH;
    status = NtOpenFile( &dest_handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, options );
    if (status == STATUS_SUCCESS)  /* destination exists */
    {
        if (!(flag & MOVEFILE_REPLACE_EXISTING))
        {
            if (!is_same_file( source_handle, dest_handle ))
            {
                SetLastError( ERROR_ALREADY_EXISTS );
                RtlFreeUnicodeString( &nt_name );
                goto error;
            }
        }
        else if (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) /* cannot replace directory */
        {
            SetLastError( ERROR_ACCESS_DENIED );
            goto error;
        }

        NtClose( dest_handle );
        dest_handle = NULL;
    }
    else if (status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        RtlFreeUnicodeString( &nt_name );
        goto error;
    }

    status = wine_nt_to_unix_file_name( &nt_name, &dest_unix, FILE_OPEN_IF, FALSE );
    RtlFreeUnicodeString( &nt_name );
    if (status != STATUS_SUCCESS && status != STATUS_NO_SUCH_FILE)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }

    /* now perform the rename */

    if (rename( source_unix.Buffer, dest_unix.Buffer ) == -1)
    {
        if (errno == EXDEV && (flag & MOVEFILE_COPY_ALLOWED))
        {
            NtClose( source_handle );
            RtlFreeAnsiString( &source_unix );
            RtlFreeAnsiString( &dest_unix );
            if (!CopyFileExW( source, dest, fnProgress, param, NULL,
                              flag & MOVEFILE_REPLACE_EXISTING ?
                              0 : COPY_FILE_FAIL_IF_EXISTS ))
                return FALSE;
            return DeleteFileW( source );
        }
        FILE_SetDosError();
        /* if we created the destination, remove it */
        if (io.Information == FILE_CREATED) unlink( dest_unix.Buffer );
        goto error;
    }

    /* fixup executable permissions */

    if (is_executable( source ) != is_executable( dest ))
    {
        struct stat fstat;
        if (stat( dest_unix.Buffer, &fstat ) != -1)
        {
            if (is_executable( dest ))
                /* set executable bit where read bit is set */
                fstat.st_mode |= (fstat.st_mode & 0444) >> 2;
            else
                fstat.st_mode &= ~0111;
            chmod( dest_unix.Buffer, fstat.st_mode );
        }
    }

    NtClose( source_handle );
    RtlFreeAnsiString( &source_unix );
    RtlFreeAnsiString( &dest_unix );
    return TRUE;

error:
    if (source_handle) NtClose( source_handle );
    if (dest_handle) NtClose( dest_handle );
    RtlFreeAnsiString( &source_unix );
    RtlFreeAnsiString( &dest_unix );
    return FALSE;
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
 *           MoveFileExW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExW( LPCWSTR source, LPCWSTR dest, DWORD flag )
{
    return MoveFileWithProgressW( source, dest, NULL, NULL, flag );
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


/*************************************************************************
 *           CreateHardLinkW   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkW(LPCWSTR lpFileName, LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    NTSTATUS status;
    UNICODE_STRING ntDest, ntSource;
    ANSI_STRING unixDest, unixSource;
    BOOL ret = FALSE;

    TRACE("(%s, %s, %p)\n", debugstr_w(lpFileName),
        debugstr_w(lpExistingFileName), lpSecurityAttributes);

    ntDest.Buffer = ntSource.Buffer = NULL;
    if (!RtlDosPathNameToNtPathName_U( lpFileName, &ntDest, NULL, NULL ) ||
        !RtlDosPathNameToNtPathName_U( lpExistingFileName, &ntSource, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto err;
    }

    unixSource.Buffer = unixDest.Buffer = NULL;
    status = wine_nt_to_unix_file_name( &ntSource, &unixSource, FILE_OPEN, FALSE );
    if (!status)
    {
        status = wine_nt_to_unix_file_name( &ntDest, &unixDest, FILE_CREATE, FALSE );
        if (!status) /* destination must not exist */
        {
            status = STATUS_OBJECT_NAME_EXISTS;
        } else if (status == STATUS_NO_SUCH_FILE)
        {
            status = STATUS_SUCCESS;
        }
    }

    if (status)
         SetLastError( RtlNtStatusToDosError(status) );
    else if (!link( unixSource.Buffer, unixDest.Buffer ))
    {
        TRACE("Hardlinked '%s' to '%s'\n", debugstr_a( unixDest.Buffer ),
                debugstr_a( unixSource.Buffer ));
        ret = TRUE;
    }
    else
        FILE_SetDosError();

    RtlFreeAnsiString( &unixSource );
    RtlFreeAnsiString( &unixDest );

err:
    RtlFreeUnicodeString( &ntSource );
    RtlFreeUnicodeString( &ntDest );
    return ret;
}


/*************************************************************************
 *           CreateHardLinkA   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkA(LPCSTR lpFileName, LPCSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    WCHAR *sourceW, *destW;
    BOOL res;

    if (!(sourceW = FILE_name_AtoW( lpExistingFileName, TRUE )))
    {
        return FALSE;
    }
    if (!(destW = FILE_name_AtoW( lpFileName, TRUE )))
    {
        HeapFree( GetProcessHeap(), 0, sourceW );
        return FALSE;
    }

    res = CreateHardLinkW( destW, sourceW, lpSecurityAttributes );

    HeapFree( GetProcessHeap(), 0, sourceW );
    HeapFree( GetProcessHeap(), 0, destW );

    return res;
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
 *           RemoveDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryW( LPCWSTR path )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    ANSI_STRING unix_name;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE( "%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, DELETE | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        RtlFreeUnicodeString( &nt_name );
        return FALSE;
    }

    status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, FALSE );
    RtlFreeUnicodeString( &nt_name );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        NtClose( handle );
        return FALSE;
    }

    if (!(ret = (rmdir( unix_name.Buffer ) != -1))) FILE_SetDosError();
    RtlFreeAnsiString( &unix_name );
    NtClose( handle );
    return ret;
}


/***********************************************************************
 *           RemoveDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryA( LPCSTR path )
{
    WCHAR *pathW;

    if (!(pathW = FILE_name_AtoW( path, FALSE ))) return FALSE;
    return RemoveDirectoryW( pathW );
}


/***********************************************************************
 *           GetSystemDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetSystemDirectoryW( LPWSTR path, UINT count )
{
    UINT len = strlenW( DIR_System ) + 1;
    if (path && count >= len)
    {
        strcpyW( path, DIR_System );
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
    return copy_filename_WtoA( DIR_System, path, count );
}


/***********************************************************************
 *           GetSystemWow64DirectoryW   (KERNEL32.@)
 *
 * As seen on MSDN
 * - On Win32 we should return ERROR_CALL_NOT_IMPLEMENTED
 * - On Win64 we should return the SysWow64 (system64) directory
 */
UINT WINAPI GetSystemWow64DirectoryW( LPWSTR path, UINT count )
{
    UINT len;

    if (!DIR_SysWow64)
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }
    len = strlenW( DIR_SysWow64 ) + 1;
    if (path && count >= len)
    {
        strcpyW( path, DIR_SysWow64 );
        len--;
    }
    return len;
}


/***********************************************************************
 *           GetSystemWow64DirectoryA   (KERNEL32.@)
 *
 * See comment for GetWindowsWow64DirectoryW.
 */
UINT WINAPI GetSystemWow64DirectoryA( LPSTR path, UINT count )
{
    if (!DIR_SysWow64)
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }
    return copy_filename_WtoA( DIR_SysWow64, path, count );
}


/***********************************************************************
 *           Wow64EnableWow64FsRedirection   (KERNEL32.@)
 */
BOOLEAN WINAPI Wow64EnableWow64FsRedirection( BOOLEAN enable )
{
    NTSTATUS status = RtlWow64EnableFsRedirection( enable );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
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
    ANSI_STRING unix_name;
    NTSTATUS status;

    if (!RtlDosPathNameToNtPathName_U( dosW, &nt_name, NULL, NULL )) return NULL;
    status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN_IF, FALSE );
    RtlFreeUnicodeString( &nt_name );
    if (status && status != STATUS_NO_SUCH_FILE)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return NULL;
    }
    return unix_name.Buffer;
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
    ANSI_STRING unix_name;
    NTSTATUS status;
    DWORD len;

    RtlInitAnsiString( &unix_name, str );
    status = wine_unix_to_nt_file_name( &unix_name, &nt_name );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return NULL;
    }
    if (nt_name.Buffer[5] == ':')
    {
        /* get rid of the \??\ prefix */
        /* FIXME: should implement RtlNtPathNameToDosPathName and use that instead */
        len = nt_name.Length - 4 * sizeof(WCHAR);
        memmove( nt_name.Buffer, nt_name.Buffer + 4, len );
        nt_name.Buffer[len / sizeof(WCHAR)] = 0;
    }
    else
        nt_name.Buffer[1] = '\\';
    return nt_name.Buffer;
}

/*************************************************************************
 *           CreateSymbolicLinkW   (KERNEL32.@)
 */
BOOLEAN WINAPI CreateSymbolicLinkW(LPCWSTR link, LPCWSTR target, DWORD flags)
{
    FIXME("(%s %s %d): stub\n", debugstr_w(link), debugstr_w(target), flags);
    return TRUE;
}

/*************************************************************************
 *           CreateSymbolicLinkA   (KERNEL32.@)
 */
BOOLEAN WINAPI CreateSymbolicLinkA(LPCSTR link, LPCSTR target, DWORD flags)
{
    FIXME("(%s %s %d): stub\n", debugstr_a(link), debugstr_a(target), flags);
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

    TRACE("(%s %p %u %p %p)\n", name, oemname,
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

    TRACE("(%s %p %u %p %p)\n", wine_dbgstr_w(name), oemname,
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
    int val;

    switch (flags)
    {
    case BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE:
        val = 1;
        break;
    case BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE:
        val = 0;
        break;
    case BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT:
        InterlockedExchange( &path_safe_mode, 2 );
        return TRUE;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (;;)
    {
        int prev = path_safe_mode;
        if (prev == 2) break;  /* permanently set */
        if (InterlockedCompareExchange( &path_safe_mode, val, prev ) == prev) return TRUE;
    }
    SetLastError( ERROR_ACCESS_DENIED );
    return FALSE;
}
