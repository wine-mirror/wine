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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_PATHNAME_LEN        1024

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
