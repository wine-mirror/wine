/*
 * File handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"

#include "kernel_private.h"
#include "file.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_PATHNAME_LEN        1024


/* check if a file name is for an executable file (.exe or .com) */
inline static BOOL is_executable( const WCHAR *name )
{
    static const WCHAR exeW[] = {'.','e','x','e',0};
    static const WCHAR comW[] = {'.','c','o','m',0};
    int len = strlenW(name);

    if (len < 4) return FALSE;
    return (!strcmpiW( name + len - 4, exeW ) || !strcmpiW( name + len - 4, comW ));
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
static BOOL add_boot_rename_entry( LPCWSTR fn1, LPCWSTR fn2, DWORD flags )
{
    static const WCHAR PreString[] = {'\\','?','?','\\',0};
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
    UNICODE_STRING nameW;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HKEY Reboot = 0;
    DWORD len0, len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

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
        return FALSE;
    }

    len0 = strlenW(PreString);
    len1 = strlenW(fn1) + len0 + 1;
    if (fn2)
    {
        len2 = strlenW(fn2) + len0 + 1;
        if (flags & MOVEFILE_REPLACE_EXISTING) len2++; /* Plus 1 because of the leading '!' */
    }
    else len2 = 1; /* minimum is the 0 characters for the empty second string */

    /* convert characters to bytes */
    len0 *= sizeof(WCHAR);
    len1 *= sizeof(WCHAR);
    len2 *= sizeof(WCHAR);

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

    p = (WCHAR *)(Buffer + DataSize);
    strcpyW( p, PreString );
    strcatW( p, fn1 );
    DataSize += len1;
    if (fn2)
    {
        p = (WCHAR *)(Buffer + DataSize);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        strcpyW( p, PreString );
        strcatW( p, fn2 );
        DataSize += len2;
    }
    else
    {
        p = (WCHAR *)(Buffer + DataSize);
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    if (Reboot) NtClose(Reboot);
    if (Buffer) HeapFree( GetProcessHeap(), 0, Buffer );
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
    UNICODE_STRING nameW;
    WCHAR bufferW[MAX_PATH];
    DWORD ret, retW;

    if (!name)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&nameW, name))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetFullPathNameW( nameW.Buffer, MAX_PATH, bufferW, NULL);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (ret && ret <= len)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, len, NULL, NULL);
            ret--; /* length without 0 */

            if (lastpart)
            {
                LPSTR p = buffer + strlen(buffer) - 1;

                if (*p != '\\')
                {
                    while ((p > buffer + 2) && (*p != '\\')) p--;
                    *lastpart = p + 1;
                }
                else *lastpart = NULL;
            }
        }
    }

    RtlFreeUnicodeString(&nameW);
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
    BOOL                unixabsolute = (shortpath[0] == '/');
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

    TRACE("%s,%p,%ld\n", debugstr_w(shortpath), longpath, longlen);

    if (shortpath[0] == '\\' && shortpath[1] == '\\')
    {
        ERR("UNC pathname %s\n", debugstr_w(shortpath));
        lstrcpynW( longpath, shortpath, longlen );
        return strlenW(longpath);
    }

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
    UNICODE_STRING shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret, retW;

    if (!shortpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("%s\n", debugstr_a(shortpath));

    if (!RtlCreateUnicodeStringFromAsciiz(&shortpathW, shortpath))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetLongPathNameW(shortpathW.Buffer, longpathW, MAX_PATH);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, longpathW, -1, NULL, 0, NULL, NULL);
        if (ret <= longlen)
        {
            WideCharToMultiByte(CP_ACP, 0, longpathW, -1, longpath, longlen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }

    RtlFreeUnicodeString(&shortpathW);
    return ret;
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
    BOOL                unixabsolute = (longpath[0] == '/');
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
    if (!unixabsolute && longpath[1] == ':' )
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
        if (tmplen <= 8+1+3+1)
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
    UNICODE_STRING longpathW;
    WCHAR shortpathW[MAX_PATH];
    DWORD ret, retW;

    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("%s\n", debugstr_a(longpath));

    if (!RtlCreateUnicodeStringFromAsciiz(&longpathW, longpath))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetShortPathNameW(longpathW.Buffer, shortpathW, MAX_PATH);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, shortpathW, -1, NULL, 0, NULL, NULL);
        if (ret <= shortlen)
        {
            WideCharToMultiByte(CP_ACP, 0, shortpathW, -1, shortpath, shortlen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }

    RtlFreeUnicodeString(&longpathW);
    return ret;
}


/***********************************************************************
 *           GetTempPathA   (KERNEL32.@)
 */
UINT WINAPI GetTempPathA( UINT count, LPSTR path )
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

    ret = WideCharToMultiByte(CP_ACP, 0, pathW, -1, NULL, 0, NULL, NULL);
    if (ret <= count)
    {
        WideCharToMultiByte(CP_ACP, 0, pathW, -1, path, count, NULL, NULL);
        ret--; /* length without 0 */
    }
    return ret;
}


/***********************************************************************
 *           GetTempPathW   (KERNEL32.@)
 */
UINT WINAPI GetTempPathW( UINT count, LPWSTR path )
{
    static const WCHAR tmp[]  = { 'T', 'M', 'P', 0 };
    static const WCHAR temp[] = { 'T', 'E', 'M', 'P', 0 };
    WCHAR tmp_path[MAX_PATH];
    UINT ret;

    TRACE("%u,%p\n", count, path);

    if (!(ret = GetEnvironmentVariableW( tmp, tmp_path, MAX_PATH )))
        if (!(ret = GetEnvironmentVariableW( temp, tmp_path, MAX_PATH )))
            if (!(ret = GetCurrentDirectoryW( MAX_PATH, tmp_path )))
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
 *           contains_pathW
 *
 * Check if the file name contains a path; helper for SearchPathW.
 * A relative path is not considered a path unless it starts with ./ or ../
 */
inline static BOOL contains_pathW (LPCWSTR name)
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
 *    path	[I] Path to search
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
 */
DWORD WINAPI SearchPathA( LPCSTR path, LPCSTR name, LPCSTR ext,
                          DWORD buflen, LPSTR buffer, LPSTR *lastpart )
{
    UNICODE_STRING pathW, nameW, extW;
    WCHAR bufferW[MAX_PATH];
    DWORD ret, retW;

    if (path) RtlCreateUnicodeStringFromAsciiz(&pathW, path);
    else pathW.Buffer = NULL;
    if (name) RtlCreateUnicodeStringFromAsciiz(&nameW, name);
    else nameW.Buffer = NULL;
    if (ext) RtlCreateUnicodeStringFromAsciiz(&extW, ext);
    else extW.Buffer = NULL;

    retW = SearchPathW(pathW.Buffer, nameW.Buffer, extW.Buffer, MAX_PATH, bufferW, NULL);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (buflen >= ret)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, buflen, NULL, NULL);
            ret--; /* length without 0 */
            if (lastpart) *lastpart = strrchr(buffer, '\\') + 1;
        }
    }

    RtlFreeUnicodeString(&pathW);
    RtlFreeUnicodeString(&nameW);
    RtlFreeUnicodeString(&extW);
    return ret;
}


/**************************************************************************
 *           MoveFileExW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExW( LPCWSTR source, LPCWSTR dest, DWORD flag )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE source_handle = 0, dest_handle;
    char *source_unix = NULL, *dest_unix = NULL;

    TRACE("(%s,%s,%04lx)\n", debugstr_w(source), debugstr_w(dest), flag);

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
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &source_handle, 0, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
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

    if (!(source_unix = wine_get_unix_file_name( source )))  /* should not happen */
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
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
    RtlFreeUnicodeString( &nt_name );
    if (status == STATUS_SUCCESS)
    {
        NtClose( dest_handle );
        if (!(flag & MOVEFILE_REPLACE_EXISTING))
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            goto error;
        }
    }
    else if (status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }
    if (!(dest_unix = wine_get_unix_file_name( dest )))  /* should not happen */
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto error;
    }

    /* now perform the rename */

    if (rename( source_unix, dest_unix ) == -1)
    {
        if (errno == EXDEV && (flag & MOVEFILE_COPY_ALLOWED))
        {
            NtClose( source_handle );
            HeapFree( GetProcessHeap(), 0, source_unix );
            HeapFree( GetProcessHeap(), 0, dest_unix );
            return (CopyFileW( source, dest, TRUE ) && DeleteFileW( source ));
        }
        FILE_SetDosError();
        /* if we created the destination, remove it */
        if (io.Information == FILE_CREATED) unlink( dest_unix );
        goto error;
    }

    /* fixup executable permissions */

    if (is_executable( source ) != is_executable( dest ))
    {
        struct stat fstat;
        if (stat( dest_unix, &fstat ) != -1)
        {
            if (is_executable( dest ))
                /* set executable bit where read bit is set */
                fstat.st_mode |= (fstat.st_mode & 0444) >> 2;
            else
                fstat.st_mode &= ~0111;
            chmod( dest_unix, fstat.st_mode );
        }
    }

    NtClose( source_handle );
    HeapFree( GetProcessHeap(), 0, source_unix );
    HeapFree( GetProcessHeap(), 0, dest_unix );
    return TRUE;

error:
    if (source_handle) NtClose( source_handle );
    if (source_unix) HeapFree( GetProcessHeap(), 0, source_unix );
    if (dest_unix) HeapFree( GetProcessHeap(), 0, dest_unix );
    return FALSE;
}

/**************************************************************************
 *           MoveFileExA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExA( LPCSTR source, LPCSTR dest, DWORD flag )
{
    UNICODE_STRING sourceW, destW;
    BOOL ret;

    if (!source)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz(&sourceW, source);
    if (dest) RtlCreateUnicodeStringFromAsciiz(&destW, dest);
    else destW.Buffer = NULL;

    ret = MoveFileExW( sourceW.Buffer, destW.Buffer, flag );

    RtlFreeUnicodeString(&sourceW);
    RtlFreeUnicodeString(&destW);
    return ret;
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
