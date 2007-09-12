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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
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
        str.MaximumLength = len;
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
 * entry of the registrykey is always appended with two zero
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
    static const WCHAR SessionW[] = {'M','a','c','h','i','n','e','\\',
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
        WARN("Error creating key for reboot managment [%s]\n",
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
                         NULL, 0, &DataSize ) == STATUS_BUFFER_OVERFLOW)
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
 *           GetFullPathNameW   (KERNEL32.@)
 * NOTES
 *   if the path closed with '\', *lastpart is 0
 */
DWORD WINAPI GetFullPathNameW( LPCWSTR name, DWORD len, LPWSTR buffer,
                               LPWSTR *lastpart )
{
    return RtlGetFullPathName_U(name, len * sizeof(WCHAR), buffer, lastpart) / sizeof(WCHAR);
}

/***********************************************************************
 *           GetFullPathNameA   (KERNEL32.@)
 * NOTES
 *   if the path closed with '\', *lastpart is 0
 */
DWORD WINAPI GetFullPathNameA( LPCSTR name, DWORD len, LPSTR buffer,
                               LPSTR *lastpart )
{
    WCHAR *nameW;
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return 0;

    ret = GetFullPathNameW( nameW, MAX_PATH, bufferW, NULL);

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, len );
    if (ret < len && lastpart)
    {
        LPSTR p = buffer + strlen(buffer) - 1;

        if (*p != '\\')
        {
            while ((p > buffer + 2) && (*p != '\\')) p--;
            *lastpart = p + 1;
        }
        else *lastpart = NULL;
    }
    return ret;
}


/***********************************************************************
 *           GetLongPathNameW   (KERNEL32.@)
 *
 * NOTES
 *  observed (Win2000):
 *  shortpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  shortpath="":   LastError=ERROR_PATH_NOT_FOUND, ret=0
 */
DWORD WINAPI GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
    WCHAR               tmplongpath[MAX_PATHNAME_LEN];
    LPCWSTR             p;
    DWORD               sp = 0, lp = 0;
    DWORD               tmplen;
    BOOL                unixabsolute;
    WIN32_FIND_DATAW    wfd;
    HANDLE              goit;

    if (!shortpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!shortpath[0])
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return 0;
    }

    TRACE("%s,%p,%d\n", debugstr_w(shortpath), longpath, longlen);

    if (shortpath[0] == '\\' && shortpath[1] == '\\')
    {
        ERR("UNC pathname %s\n", debugstr_w(shortpath));
        lstrcpynW( longpath, shortpath, longlen );
        return strlenW(longpath);
    }

    unixabsolute = (shortpath[0] == '/');

    /* check for drive letter */
    if (!unixabsolute && shortpath[1] == ':' )
    {
        tmplongpath[0] = shortpath[0];
        tmplongpath[1] = ':';
        lp = sp = 2;
    }

    while (shortpath[sp])
    {
        /* check for path delimiters and reproduce them */
        if (shortpath[sp] == '\\' || shortpath[sp] == '/')
        {
            if (!lp || tmplongpath[lp-1] != '\\')
            {
                /* strip double "\\" */
                tmplongpath[lp++] = '\\';
            }
            tmplongpath[lp] = 0; /* terminate string */
            sp++;
            continue;
        }

        p = shortpath + sp;
        if (sp == 0 && p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
        {
            tmplongpath[lp++] = *p++;
            tmplongpath[lp++] = *p++;
        }
        for (; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (shortpath + sp);
        lstrcpynW(tmplongpath + lp, shortpath + sp, tmplen + 1);
        /* Check if the file exists and use the existing file name */
        goit = FindFirstFileW(tmplongpath, &wfd);
        if (goit == INVALID_HANDLE_VALUE)
        {
            TRACE("not found %s!\n", debugstr_w(tmplongpath));
            SetLastError ( ERROR_FILE_NOT_FOUND );
            return 0;
        }
        FindClose(goit);
        strcpyW(tmplongpath + lp, wfd.cFileName);
        lp += strlenW(tmplongpath + lp);
        sp += tmplen;
    }
    tmplen = strlenW(shortpath) - 1;
    if ((shortpath[tmplen] == '/' || shortpath[tmplen] == '\\') &&
        (tmplongpath[lp - 1] != '/' && tmplongpath[lp - 1] != '\\'))
        tmplongpath[lp++] = shortpath[tmplen];
    tmplongpath[lp] = 0;

    tmplen = strlenW(tmplongpath) + 1;
    if (tmplen <= longlen)
    {
        strcpyW(longpath, tmplongpath);
        TRACE("returning %s\n", debugstr_w(longpath));
        tmplen--; /* length without 0 */
    }

    return tmplen;
}

/***********************************************************************
 *           GetLongPathNameA   (KERNEL32.@)
 */
DWORD WINAPI GetLongPathNameA( LPCSTR shortpath, LPSTR longpath, DWORD longlen )
{
    WCHAR *shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret;

    TRACE("%s\n", debugstr_a(shortpath));

    if (!(shortpathW = FILE_name_AtoW( shortpath, FALSE ))) return 0;

    ret = GetLongPathNameW(shortpathW, longpathW, MAX_PATH);

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    return copy_filename_WtoA( longpathW, longpath, longlen );
}


/***********************************************************************
 *           GetShortPathNameW   (KERNEL32.@)
 *
 * NOTES
 *  observed:
 *  longpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  longpath="" or invalid: LastError=ERROR_BAD_PATHNAME, ret=0
 *
 * more observations ( with NT 3.51 (WinDD) ):
 * longpath <= 8.3 -> just copy longpath to shortpath
 * longpath > 8.3  ->
 *             a) file does not exist -> return 0, LastError = ERROR_FILE_NOT_FOUND
 *             b) file does exist     -> set the short filename.
 * - trailing slashes are reproduced in the short name, even if the
 *   file is not a directory
 * - the absolute/relative path of the short name is reproduced like found
 *   in the long name
 * - longpath and shortpath may have the same address
 * Peter Ganten, 1999
 */
DWORD WINAPI GetShortPathNameW( LPCWSTR longpath, LPWSTR shortpath, DWORD shortlen )
{
    WCHAR               tmpshortpath[MAX_PATHNAME_LEN];
    LPCWSTR             p;
    DWORD               sp = 0, lp = 0;
    DWORD               tmplen;
    WIN32_FIND_DATAW    wfd;
    HANDLE              goit;
    UNICODE_STRING      ustr;
    WCHAR               ustr_buf[8+1+3+1];

    TRACE("%s\n", debugstr_w(longpath));

    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!longpath[0])
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }

    /* check for drive letter */
    if (longpath[0] != '/' && longpath[1] == ':' )
    {
        tmpshortpath[0] = longpath[0];
        tmpshortpath[1] = ':';
        sp = lp = 2;
    }

    ustr.Buffer = ustr_buf;
    ustr.Length = 0;
    ustr.MaximumLength = sizeof(ustr_buf);

    while (longpath[lp])
    {
        /* check for path delimiters and reproduce them */
        if (longpath[lp] == '\\' || longpath[lp] == '/')
        {
            if (!sp || tmpshortpath[sp-1] != '\\')
            {
                /* strip double "\\" */
                tmpshortpath[sp] = '\\';
                sp++;
            }
            tmpshortpath[sp] = 0; /* terminate string */
            lp++;
            continue;
        }

        for (p = longpath + lp; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (longpath + lp);
        lstrcpynW(tmpshortpath + sp, longpath + lp, tmplen + 1);
        /* Check, if the current element is a valid dos name */
        if (tmplen <= 8+1+3)
        {
            BOOLEAN spaces;
            memcpy(ustr_buf, longpath + lp, tmplen * sizeof(WCHAR));
            ustr_buf[tmplen] = '\0';
            ustr.Length = tmplen * sizeof(WCHAR);
            if (RtlIsNameLegalDOS8Dot3(&ustr, NULL, &spaces) && !spaces)
            {
                sp += tmplen;
                lp += tmplen;
                continue;
            }
        }

        /* Check if the file exists and use the existing short file name */
        goit = FindFirstFileW(tmpshortpath, &wfd);
        if (goit == INVALID_HANDLE_VALUE) goto notfound;
        FindClose(goit);
        strcpyW(tmpshortpath + sp, wfd.cAlternateFileName);
        sp += strlenW(tmpshortpath + sp);
        lp += tmplen;
    }
    tmpshortpath[sp] = 0;

    tmplen = strlenW(tmpshortpath) + 1;
    if (tmplen <= shortlen)
    {
        strcpyW(shortpath, tmpshortpath);
        TRACE("returning %s\n", debugstr_w(shortpath));
        tmplen--; /* length without 0 */
    }

    return tmplen;

 notfound:
    TRACE("not found!\n" );
    SetLastError ( ERROR_FILE_NOT_FOUND );
    return 0;
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
 *           GetTempPathA   (KERNEL32.@)
 */
DWORD WINAPI GetTempPathA( DWORD count, LPSTR path )
{
    WCHAR pathW[MAX_PATH];
    UINT ret;

    ret = GetTempPathW(MAX_PATH, pathW);

    if (!ret)
        return 0;

    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    return copy_filename_WtoA( pathW, path, count );
}


/***********************************************************************
 *           GetTempPathW   (KERNEL32.@)
 */
DWORD WINAPI GetTempPathW( DWORD count, LPWSTR path )
{
    static const WCHAR tmp[]  = { 'T', 'M', 'P', 0 };
    static const WCHAR temp[] = { 'T', 'E', 'M', 'P', 0 };
    static const WCHAR userprofile[] = { 'U','S','E','R','P','R','O','F','I','L','E',0 };
    WCHAR tmp_path[MAX_PATH];
    UINT ret;

    TRACE("%u,%p\n", count, path);

    if (!(ret = GetEnvironmentVariableW( tmp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( temp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( userprofile, tmp_path, MAX_PATH )) &&
        !(ret = GetWindowsDirectoryW( tmp_path, MAX_PATH )))
        return 0;

    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    ret = GetFullPathNameW(tmp_path, MAX_PATH, tmp_path, NULL);
    if (!ret) return 0;

    if (ret > MAX_PATH - 2)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    if (tmp_path[ret-1] != '\\')
    {
        tmp_path[ret++] = '\\';
        tmp_path[ret]   = '\0';
    }

    ret++; /* add space for terminating 0 */

    if (count)
    {
        lstrcpynW(path, tmp_path, count);
        if (count >= ret)
            ret--; /* return length without 0 */
        else if (count < 4)
            path[0] = 0; /* avoid returning ambiguous "X:" */
    }

    TRACE("returning %u, %s\n", ret, debugstr_w(path));
    return ret;
}


/***********************************************************************
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique, LPSTR buffer)
{
    WCHAR *pathW, *prefixW = NULL;
    WCHAR bufferW[MAX_PATH];
    UINT ret;

    if (!(pathW = FILE_name_AtoW( path, FALSE ))) return 0;
    if (prefix && !(prefixW = FILE_name_AtoW( prefix, TRUE ))) return 0;

    ret = GetTempFileNameW(pathW, prefixW, unique, bufferW);
    if (ret) FILE_name_WtoA( bufferW, -1, buffer, MAX_PATH );

    HeapFree( GetProcessHeap(), 0, prefixW );
    return ret;
}

/***********************************************************************
 *           GetTempFileNameW   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer )
{
    static const WCHAR formatW[] = {'%','x','.','t','m','p',0};

    int i;
    LPWSTR p;

    if ( !path || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    strcpyW( buffer, path );
    p = buffer + strlenW(buffer);

    /* add a \, if there isn't one  */
    if ((p == buffer) || (p[-1] != '\\')) *p++ = '\\';

    if (prefix)
        for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    unique &= 0xffff;

    if (unique) sprintfW( p, formatW, unique );
    else
    {
        /* get a "random" unique number and try to create the file */
        HANDLE handle;
        UINT num = GetTickCount() & 0xffff;

        if (!num) num = 1;
        unique = num;
        do
        {
            sprintfW( p, formatW, unique );
            handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                TRACE("created %s\n", debugstr_w(buffer) );
                CloseHandle( handle );
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS &&
                GetLastError() != ERROR_SHARING_VIOLATION)
                break;  /* No need to go on */
            if (!(++unique & 0xffff)) unique = 1;
        } while (unique != num);
    }

    TRACE("returning %s\n", debugstr_w(buffer) );
    return unique;
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
    else  /* search in the default path */
    {
        WCHAR *dll_path = MODULE_get_dll_load_path( NULL );

        if (dll_path)
        {
            ret = RtlDosSearchPath_U( dll_path, name, ext, buflen * sizeof(WCHAR),
                                      buffer, lastpart ) / sizeof(WCHAR);
            HeapFree( GetProcessHeap(), 0, dll_path );
        }
        else
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
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
    WCHAR *pathW = NULL, *nameW = NULL, *extW = NULL;
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (!name || !(nameW = FILE_name_AtoW( name, FALSE ))) return 0;
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


/**************************************************************************
 *           CopyFileW   (KERNEL32.@)
 */
BOOL WINAPI CopyFileW( LPCWSTR source, LPCWSTR dest, BOOL fail_if_exists )
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

    TRACE("%s -> %s\n", debugstr_w(source), debugstr_w(dest));

    if ((h1 = CreateFileW(source, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
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

    if ((h2 = CreateFileW( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             fail_if_exists ? CREATE_NEW : CREATE_ALWAYS,
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
 *
 * This implementation ignores most of the extra parameters passed-in into
 * the "ex" version of the method and calls the CopyFile method.
 * It will have to be fixed eventually.
 */
BOOL WINAPI CopyFileExW(LPCWSTR sourceFilename, LPCWSTR destFilename,
                        LPPROGRESS_ROUTINE progressRoutine, LPVOID appData,
                        LPBOOL cancelFlagPointer, DWORD copyFlags)
{
    /*
     * Interpret the only flag that CopyFile can interpret.
     */
    return CopyFileW(sourceFilename, destFilename, (copyFlags & COPY_FILE_FAIL_IF_EXISTS) != 0);
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
    HANDLE source_handle = 0, dest_handle;
    ANSI_STRING source_unix, dest_unix;

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

    status = NtOpenFile( &source_handle, 0, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
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

    if (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (flag & MOVEFILE_REPLACE_EXISTING)  /* cannot replace directory */
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            goto error;
        }
    }

    /* we must have write access to the destination, and it must */
    /* not exist except if MOVEFILE_REPLACE_EXISTING is set */

    if (!RtlDosPathNameToNtPathName_U( dest, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto error;
    }
    status = NtOpenFile( &dest_handle, GENERIC_READ | GENERIC_WRITE, &attr, &io, 0,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (status == STATUS_SUCCESS)
    {
        NtClose( dest_handle );
        if (!(flag & MOVEFILE_REPLACE_EXISTING))
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            RtlFreeUnicodeString( &nt_name );
            goto error;
        }
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
            if (!CopyFileExW( source, dest, fnProgress,
                              param, NULL, COPY_FILE_FAIL_IF_EXISTS ))
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


/***********************************************************************
 *           CreateDirectoryW   (KERNEL32.@)
 * RETURNS:
 *	TRUE : success
 *	FALSE : failure
 *		ERROR_DISK_FULL:	on full disk
 *		ERROR_ALREADY_EXISTS:	if directory name exists (even as file)
 *		ERROR_ACCESS_DENIED:	on permission problems
 *		ERROR_FILENAME_EXCED_RANGE: too long filename(s)
 */
BOOL WINAPI CreateDirectoryW( LPCWSTR path, LPSECURITY_ATTRIBUTES sa )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
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
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile( &handle, GENERIC_READ, &attr, &io, NULL,
                           FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );

    if (status == STATUS_SUCCESS)
    {
        NtClose( handle );
        ret = TRUE;
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    RtlFreeUnicodeString( &nt_name );
    return ret;
}


/***********************************************************************
 *           CreateDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryA( LPCSTR path, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR *pathW;

    if (!(pathW = FILE_name_AtoW( path, FALSE ))) return FALSE;
    return CreateDirectoryW( pathW, sa );
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
 *           CreateDirectoryExW   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryExW( LPCWSTR template, LPCWSTR path, LPSECURITY_ATTRIBUTES sa )
{
    return CreateDirectoryW( path, sa );
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

    status = NtOpenFile( &handle, DELETE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (status == STATUS_SUCCESS)
        status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, FALSE );
    RtlFreeUnicodeString( &nt_name );

    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
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
 *           GetCurrentDirectoryW   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    return RtlGetCurrentDirectory_U( buflen * sizeof(WCHAR), buf ) / sizeof(WCHAR);
}


/***********************************************************************
 *           GetCurrentDirectoryA   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (buflen && buf && !HIWORD(buf))
    {
        /* Win9x catches access violations here, returning zero.
         * This behaviour resulted in some people not noticing
         * that they got the argument order wrong. So let's be
         * nice and fail gracefully if buf is invalid and looks
         * more like a buflen (which is probably MAX_PATH). */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ret = GetCurrentDirectoryW(MAX_PATH, bufferW);

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    return copy_filename_WtoA( bufferW, buf, buflen );
}


/***********************************************************************
 *           SetCurrentDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryW( LPCWSTR dir )
{
    UNICODE_STRING dirW;
    NTSTATUS status;

    RtlInitUnicodeString( &dirW, dir );
    status = RtlSetCurrentDirectory_U( &dirW );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           SetCurrentDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryA( LPCSTR dir )
{
    WCHAR *dirW;

    if (!(dirW = FILE_name_AtoW( dir, FALSE ))) return FALSE;
    return SetCurrentDirectoryW( dirW );
}


/***********************************************************************
 *           GetWindowsDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    UINT len = strlenW( DIR_Windows ) + 1;
    if (path && count >= len)
    {
        strcpyW( path, DIR_Windows );
        len--;
    }
    return len;
}


/***********************************************************************
 *           GetWindowsDirectoryA   (KERNEL32.@)
 *
 * Return value:
 * If buffer is large enough to hold full path and terminating '\0' character
 * function copies path to buffer and returns length of the path without '\0'.
 * Otherwise function returns required size including '\0' character and
 * does not touch the buffer.
 */
UINT WINAPI GetWindowsDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( DIR_Windows, path, count );
}


/***********************************************************************
 *           GetSystemWindowsDirectoryA   (KERNEL32.@) W2K, TS4.0SP4
 */
UINT WINAPI GetSystemWindowsDirectoryA( LPSTR path, UINT count )
{
    return GetWindowsDirectoryA( path, count );
}


/***********************************************************************
 *           GetSystemWindowsDirectoryW   (KERNEL32.@) W2K, TS4.0SP4
 */
UINT WINAPI GetSystemWindowsDirectoryW( LPWSTR path, UINT count )
{
    return GetWindowsDirectoryW( path, count );
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
 * - On Win32 we should returns ERROR_CALL_NOT_IMPLEMENTED
 * - On Win64 we should returns the SysWow64 (system64) directory
 */
UINT WINAPI GetSystemWow64DirectoryW( LPWSTR lpBuffer, UINT uSize )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}


/***********************************************************************
 *           GetSystemWow64DirectoryA   (KERNEL32.@)
 *
 * See comment for GetWindowsWow64DirectoryW.
 */
UINT WINAPI GetSystemWow64DirectoryA( LPSTR lpBuffer, UINT uSize )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}


/***********************************************************************
 *           NeedCurrentDirectoryForExePathW   (KERNEL32.@)
 */
BOOL WINAPI NeedCurrentDirectoryForExePathW( LPCWSTR name )
{
    static const WCHAR env_name[] = {'N','o','D','e','f','a','u','l','t',
                                     'C','u','r','r','e','n','t',
                                     'D','i','r','e','c','t','o','r','y',
                                     'I','n','E','x','e','P','a','t','h',0};
    WCHAR env_val;

    /* MSDN mentions some 'registry location'. We do not use registry. */
    FIXME("(%s): partial stub\n", debugstr_w(name));

    if (strchrW(name, '\\'))
        return TRUE;

    /* Check the existence of the variable, not value */
    if (!GetEnvironmentVariableW( env_name, &env_val, 1 ))
        return TRUE;

    return FALSE;
}


/***********************************************************************
 *           NeedCurrentDirectoryForExePathA   (KERNEL32.@)
 */
BOOL WINAPI NeedCurrentDirectoryForExePathA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( name, FALSE ))) return TRUE;
    return NeedCurrentDirectoryForExePathW( nameW );
}


/***********************************************************************
 *           wine_get_unix_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full Unix file name for a given path.
 * Returned buffer must be freed by caller.
 */
char *wine_get_unix_file_name( LPCWSTR dosW )
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
WCHAR *wine_get_dos_file_name( LPCSTR str )
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
    /* get rid of the \??\ prefix */
    /* FIXME: should implement RtlNtPathNameToDosPathName and use that instead */
    len = nt_name.Length - 4 * sizeof(WCHAR);
    memmove( nt_name.Buffer, nt_name.Buffer + 4, len );
    nt_name.Buffer[len / sizeof(WCHAR)] = 0;
    return nt_name.Buffer;
}
