/*
 * DOS directories functions
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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "file.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

static WCHAR *DIR_Windows;
static WCHAR *DIR_System;

/***********************************************************************
 *           DIR_GetPath
 *
 * Get a path name from the wine.ini file and make sure it is valid.
 */
static WCHAR *DIR_GetPath( HKEY hkey, LPCWSTR keyname, LPCWSTR defval, BOOL warn )
{
    UNICODE_STRING nameW;
    DWORD dummy;
    WCHAR tmp[MAX_PATHNAME_LEN];
    const WCHAR *path;
    const char *mess;
    WCHAR *ret;
    DWORD attr;

    RtlInitUnicodeString( &nameW, keyname );
    if (hkey && !NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        path = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
    else
        path = defval;

    attr = GetFileAttributesW( path );
    if (attr == INVALID_FILE_ATTRIBUTES) mess = "does not exist";
    else if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) mess = "not a directory";
    else
    {
        DWORD len = GetFullPathNameW( path, 0, NULL, NULL );
        ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (ret) GetFullPathNameW( path, len, ret, NULL );
        return ret;
    }

    if (warn)
    {
        MESSAGE("Invalid path %s for %s directory: %s.\n",
                debugstr_w(path), debugstr_w(keyname), mess);
        MESSAGE("Perhaps you have not properly edited your Wine configuration file (%s/config)\n",
                wine_get_config_dir());
    }
    return NULL;
}


/***********************************************************************
 *           DIR_Init
 */
int DIR_Init(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HKEY hkey;
    char path[MAX_PATHNAME_LEN];
    WCHAR longpath[MAX_PATHNAME_LEN];
    WCHAR *tmp_dir, *profile_dir;
    int drive;
    const char *cwd;
    static const WCHAR wineW[] = {'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','W','i','n','e','\\',
                                  'C','o','n','f','i','g','\\','W','i','n','e',0};
    static const WCHAR windowsW[] = {'w','i','n','d','o','w','s',0};
    static const WCHAR systemW[] = {'s','y','s','t','e','m',0};
    static const WCHAR tempW[] = {'t','e','m','p',0};
    static const WCHAR profileW[] = {'p','r','o','f','i','l','e',0};
    static const WCHAR windows_dirW[] = {'c',':','\\','w','i','n','d','o','w','s',0};
    static const WCHAR system_dirW[] = {'c',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e','m',0};
    static const WCHAR temp_dirW[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','m','p',0};
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR path_dirW[] = {'c',':','\\','w','i','n','d','o','w','s',';',
                                      'c',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e','m',0};
    static const WCHAR path_capsW[] = {'P','A','T','H',0};
    static const WCHAR temp_capsW[] = {'T','E','M','P',0};
    static const WCHAR tmp_capsW[] = {'T','M','P',0};
    static const WCHAR windirW[] = {'w','i','n','d','i','r',0};
    static const WCHAR winsysdirW[] = {'w','i','n','s','y','s','d','i','r',0};
    static const WCHAR userprofileW[] = {'U','S','E','R','P','R','O','F','I','L','E',0};
    static const WCHAR systemrootW[] = {'S','Y','S','T','E','M','R','O','O','T',0};
    static const WCHAR wcmdW[] = {'\\','w','c','m','d','.','e','x','e',0};
    static const WCHAR comspecW[] = {'C','O','M','S','P','E','C',0};
    static const WCHAR empty_strW[] = { 0 };

    if (!getcwd( path, MAX_PATHNAME_LEN ))
    {
        perror( "Could not get current directory" );
        return 0;
    }
    cwd = path;
    if ((drive = DRIVE_FindDriveRoot( &cwd )) == -1)
    {
        MESSAGE("Warning: could not find wine config [Drive x] entry "
            "for current working directory %s; "
	    "starting in windows directory.\n", cwd );
    }
    else
    {
        longpath[0] = 'a' + drive;
        longpath[1] = ':';
        MultiByteToWideChar(CP_UNIXCP, 0, cwd, -1, longpath + 2, MAX_PATHNAME_LEN);
        SetCurrentDirectoryW( longpath );
        if(GetDriveTypeW(longpath)==DRIVE_CDROM)
            chdir("/"); /* change to root directory so as not to lock cdroms */
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitUnicodeString( &nameW, wineW );
    if (NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) hkey = 0;

    if (!(DIR_Windows = DIR_GetPath( hkey, windowsW, windows_dirW, TRUE )) ||
        !(DIR_System = DIR_GetPath( hkey, systemW, system_dirW, TRUE )) ||
        !(tmp_dir = DIR_GetPath( hkey, tempW, temp_dirW, TRUE )))
    {
        if (hkey) NtClose( hkey );
        return 0;
    }

    if (drive == -1) SetCurrentDirectoryW( DIR_Windows );

    /* Set the environment variables */

    /* set COMSPEC only if it doesn't exist already */
    if (!GetEnvironmentVariableW( comspecW, NULL, 0 ))
    {
        strcpyW( longpath, DIR_System );
        strcatW( longpath, wcmdW );
        SetEnvironmentVariableW( comspecW, longpath );
    }

    /* set PATH only if not set already */
    if (!GetEnvironmentVariableW( path_capsW, NULL, 0 ))
    {
        WCHAR tmp[MAX_PATHNAME_LEN];
        DWORD dummy;
        const WCHAR *path = path_dirW;

        RtlInitUnicodeString( &nameW, pathW );
        if (hkey && !NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      tmp, sizeof(tmp), &dummy ))
        {
            path = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
        }

        if (strchrW(path, '/'))
        {
            MESSAGE("Fix your wine config (%s/config) to use DOS drive syntax in [wine] 'Path=' statement! (no '/' allowed)\n", wine_get_config_dir() );
            ExitProcess(1);
        }
        SetEnvironmentVariableW( path_capsW, path );
        TRACE("Path       = %s\n", debugstr_w(path) );
    }

    if (!GetEnvironmentVariableW( temp_capsW, NULL, 0 ))
        SetEnvironmentVariableW( temp_capsW, tmp_dir );
    if (!GetEnvironmentVariableW( tmp_capsW, NULL, 0 ))
        SetEnvironmentVariableW( tmp_capsW, tmp_dir );

    SetEnvironmentVariableW( windirW, DIR_Windows );
    SetEnvironmentVariableW( systemrootW, DIR_Windows );
    SetEnvironmentVariableW( winsysdirW, DIR_System );

    TRACE("WindowsDir = %s\n", debugstr_w(DIR_Windows) );
    TRACE("SystemDir  = %s\n", debugstr_w(DIR_System) );
    TRACE("TempDir    = %s\n", debugstr_w(tmp_dir) );
    TRACE("SYSTEMROOT = %s\n", debugstr_w(DIR_Windows) );
    TRACE("Cwd        = %c:\\%s\n",
          'A' + drive, debugstr_w(DRIVE_GetDosCwd(drive)) );

    HeapFree( GetProcessHeap(), 0, tmp_dir );

    if ((profile_dir = DIR_GetPath( hkey, profileW, empty_strW, FALSE )))
    {
        TRACE("USERPROFILE= %s\n", debugstr_w(profile_dir) );
        SetEnvironmentVariableW( userprofileW, profile_dir );
        HeapFree( GetProcessHeap(), 0, profile_dir );
    }

    if (hkey) NtClose( hkey );

    return 1;
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
    UINT len = WideCharToMultiByte( CP_ACP, 0, DIR_Windows, -1, NULL, 0, NULL, NULL );
    if (path && count >= len)
    {
        WideCharToMultiByte( CP_ACP, 0, DIR_Windows, -1, path, count, NULL, NULL );
        len--;
    }
    return len;
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
    UINT len = WideCharToMultiByte( CP_ACP, 0, DIR_System, -1, NULL, 0, NULL, NULL );
    if (path && count >= len)
    {
        WideCharToMultiByte( CP_ACP, 0, DIR_System, -1, path, count, NULL, NULL );
        len--;
    }
    return len;
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
BOOL WINAPI CreateDirectoryW( LPCWSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    DOS_FULL_NAME full_name;

    if (!path || !*path)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    TRACE_(file)("(%s,%p)\n", debugstr_w(path), lpsecattribs );

    if (RtlIsDosDeviceName_U( path ))
    {
        TRACE_(file)("cannot use device %s!\n", debugstr_w(path));
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    if (!DOSFS_GetFullName( path, FALSE, &full_name )) return 0;
    if (mkdir( full_name.long_name, 0777 ) == -1) {
        WARN_(file)("Error '%s' trying to create directory '%s'\n", strerror(errno), full_name.long_name);
	/* the FILE_SetDosError() generated error codes don't match the
	 * CreateDirectory ones for some errnos */
	switch (errno) {
	case EEXIST:
        {
            if (!strcmp(DRIVE_GetRoot(full_name.drive), full_name.long_name))
                SetLastError(ERROR_ACCESS_DENIED);
            else
                SetLastError(ERROR_ALREADY_EXISTS);
            break;
        }
	case ENOSPC: SetLastError(ERROR_DISK_FULL); break;
	default: FILE_SetDosError();break;
	}
	return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryA( LPCSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    UNICODE_STRING pathW;
    BOOL ret = FALSE;

    if (!path || !*path)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&pathW, path))
    {
        ret = CreateDirectoryW(pathW.Buffer, lpsecattribs);
        RtlFreeUnicodeString(&pathW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}


/***********************************************************************
 *           CreateDirectoryExA   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryExA( LPCSTR template, LPCSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectoryA(path,lpsecattribs);
}


/***********************************************************************
 *           CreateDirectoryExW   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryExW( LPCWSTR template, LPCWSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectoryW(path,lpsecattribs);
}


/***********************************************************************
 *           RemoveDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryW( LPCWSTR path )
{
    DOS_FULL_NAME full_name;

    if (!path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE_(file)("%s\n", debugstr_w(path));

    if (RtlIsDosDeviceName_U( path ))
    {
        TRACE_(file)("cannot remove device %s!\n", debugstr_w(path));
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }
    if (!DOSFS_GetFullName( path, TRUE, &full_name )) return FALSE;
    if (rmdir( full_name.long_name ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           RemoveDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryA( LPCSTR path )
{
    UNICODE_STRING pathW;
    BOOL ret = FALSE;

    if (!path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&pathW, path))
    {
        ret = RemoveDirectoryW(pathW.Buffer);
        RtlFreeUnicodeString(&pathW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}
