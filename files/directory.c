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
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

static DOS_FULL_NAME DIR_Windows;
static DOS_FULL_NAME DIR_System;

static const WCHAR wineW[] = {'w','i','n','e',0};

/***********************************************************************
 *           FILE_contains_pathW
 */
inline static int FILE_contains_pathW (LPCWSTR name)
{
    return ((*name && (name[1] == ':')) ||
            strchrW (name, '/') || strchrW (name, '\\'));
}

/***********************************************************************
 *           DIR_GetPath
 *
 * Get a path name from the wine.ini file and make sure it is valid.
 */
static int DIR_GetPath( HKEY hkey, LPCWSTR keyname, LPCWSTR defval, DOS_FULL_NAME *full_name,
                        LPWSTR longname, INT longname_len, BOOL warn )
{
    UNICODE_STRING nameW;
    DWORD dummy;
    WCHAR tmp[MAX_PATHNAME_LEN];
    BY_HANDLE_FILE_INFORMATION info;
    const WCHAR *path = defval;
    const char *mess = "does not exist";

    RtlInitUnicodeString( &nameW, keyname );
    if (hkey && !NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        path = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;

    if (!DOSFS_GetFullName( path, TRUE, full_name ) ||
        (!FILE_Stat( full_name->long_name, &info, NULL ) && (mess=strerror(errno)))||
        (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (mess="not a directory")) ||
        (!(GetLongPathNameW(full_name->short_name, longname, longname_len))) )
    {
        if (warn)
        {
            MESSAGE("Invalid path %s for %s directory: %s.\n",
                    debugstr_w(path), debugstr_w(keyname), mess);
            MESSAGE("Perhaps you have not properly edited your Wine configuration file (%s/config)\n",
                    wine_get_config_dir());
        }
        return 0;
    }
    return 1;
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
    DOS_FULL_NAME tmp_dir, profile_dir;
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
        WCHAR szdrive[3]={drive+'A',':',0};
        MultiByteToWideChar(DRIVE_GetCodepage(drive), 0, cwd, -1, longpath, MAX_PATHNAME_LEN);
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, longpath );
	if(GetDriveTypeW(szdrive)==DRIVE_CDROM)
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

    if (!(DIR_GetPath( hkey, windowsW, windows_dirW, &DIR_Windows, longpath, MAX_PATHNAME_LEN, TRUE )) ||
        !(DIR_GetPath( hkey, systemW, system_dirW, &DIR_System, longpath, MAX_PATHNAME_LEN, TRUE )) ||
        !(DIR_GetPath( hkey, tempW, windows_dirW, &tmp_dir, longpath, MAX_PATHNAME_LEN, TRUE )))
    {
        if (hkey) NtClose( hkey );
        return 0;
    }
    if (-1 == access( tmp_dir.long_name, W_OK ))
    {
    	if (errno==EACCES)
	{
		MESSAGE("Warning: the temporary directory '%s' specified in your\n"
                        "configuration file (%s) is not writeable.\n",
                        tmp_dir.long_name, wine_get_config_dir() );
	}
	else
		MESSAGE("Warning: access to temporary directory '%s' failed (%s).\n",
		    tmp_dir.long_name, strerror(errno));
    }

    if (drive == -1)
    {
        drive = DIR_Windows.drive;
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, DIR_Windows.short_name + 2 );
    }

    /* Set the environment variables */

    /* set COMSPEC only if it doesn't exist already */
    if (!GetEnvironmentVariableW( comspecW, NULL, 0 ))
    {
        strcpyW( longpath, DIR_System.short_name );
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

    SetEnvironmentVariableW( temp_capsW, tmp_dir.short_name );
    SetEnvironmentVariableW( tmp_capsW, tmp_dir.short_name );
    SetEnvironmentVariableW( windirW, DIR_Windows.short_name );
    SetEnvironmentVariableW( winsysdirW, DIR_System.short_name );

    TRACE("WindowsDir = %s (%s)\n",
          debugstr_w(DIR_Windows.short_name), DIR_Windows.long_name );
    TRACE("SystemDir  = %s (%s)\n",
          debugstr_w(DIR_System.short_name), DIR_System.long_name );
    TRACE("TempDir    = %s (%s)\n",
          debugstr_w(tmp_dir.short_name), tmp_dir.long_name );
    TRACE("Cwd        = %c:\\%s\n",
          'A' + drive, debugstr_w(DRIVE_GetDosCwd(drive)) );

    if (DIR_GetPath( hkey, profileW, empty_strW, &profile_dir, longpath, MAX_PATHNAME_LEN, FALSE ))
    {
        TRACE("USERPROFILE= %s\n", debugstr_w(longpath) );
        SetEnvironmentVariableW( userprofileW, longpath );
    }

    TRACE("SYSTEMROOT = %s\n", debugstr_w(DIR_Windows.short_name) );
    SetEnvironmentVariableW( systemrootW, DIR_Windows.short_name );
    if (hkey) NtClose( hkey );

    return 1;
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
 *           DIR_GetWindowsUnixDir
 */
UINT DIR_GetWindowsUnixDir( LPSTR path, UINT count )
{
    if (path) lstrcpynA( path, DIR_Windows.long_name, count );
    return strlen( DIR_Windows.long_name );
}


/***********************************************************************
 *           DIR_GetSystemUnixDir
 */
UINT DIR_GetSystemUnixDir( LPSTR path, UINT count )
{
    if (path) lstrcpynA( path, DIR_System.long_name, count );
    return strlen( DIR_System.long_name );
}


/***********************************************************************
 *           GetTempDrive   (KERNEL.92)
 * A closer look at krnl386.exe shows what the SDK doesn't mention:
 *
 * returns:
 *   AL: driveletter
 *   AH: ':'		- yes, some kernel code even does stosw with
 *                            the returned AX.
 *   DX: 1 for success
 */
UINT WINAPI GetTempDrive( BYTE ignored )
{
    char *buffer;
    BYTE ret;
    UINT len = GetTempPathA( 0, NULL );

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len + 1 )) )
        ret = DRIVE_GetCurrentDrive() + 'A';
    else
    {
        /* FIXME: apparently Windows does something with the ignored byte */
        if (!GetTempPathA( len, buffer )) buffer[0] = 'C';
        ret = toupper(buffer[0]);
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    return MAKELONG( ret | (':' << 8), 1 );
}


/***********************************************************************
 *           GetWindowsDirectory   (KERNEL.134)
 */
UINT16 WINAPI GetWindowsDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetWindowsDirectoryA( path, count );
}


/***********************************************************************
 *           GetWindowsDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    UINT len = strlenW( DIR_Windows.short_name ) + 1;
    if (path && count >= len)
    {
        strcpyW( path, DIR_Windows.short_name );
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
    UINT len = WideCharToMultiByte( CP_ACP, 0, DIR_Windows.short_name, -1, NULL, 0, NULL, NULL );
    if (path && count >= len)
    {
        WideCharToMultiByte( CP_ACP, 0, DIR_Windows.short_name, -1, path, count, NULL, NULL );
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
 *           GetSystemDirectory   (KERNEL.135)
 */
UINT16 WINAPI GetSystemDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetSystemDirectoryA( path, count );
}


/***********************************************************************
 *           GetSystemDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryA.
 */
UINT WINAPI GetSystemDirectoryW( LPWSTR path, UINT count )
{
    UINT len = strlenW( DIR_System.short_name ) + 1;
    if (path && count >= len)
    {
        strcpyW( path, DIR_System.short_name );
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
    UINT len = WideCharToMultiByte( CP_ACP, 0, DIR_System.short_name, -1, NULL, 0, NULL, NULL );
    if (path && count >= len)
    {
        WideCharToMultiByte( CP_ACP, 0, DIR_System.short_name, -1, path, count, NULL, NULL );
        len--;
    }
    return len;
}


/***********************************************************************
 *           CreateDirectory   (KERNEL.144)
 */
BOOL16 WINAPI CreateDirectory16( LPCSTR path, LPVOID dummy )
{
    TRACE_(file)("(%s,%p)\n", path, dummy );
    return (BOOL16)CreateDirectoryA( path, NULL );
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

    if (DOSFS_GetDevice( path ))
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
 *           RemoveDirectory   (KERNEL.145)
 */
BOOL16 WINAPI RemoveDirectory16( LPCSTR path )
{
    return (BOOL16)RemoveDirectoryA( path );
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

    if (DOSFS_GetDevice( path ))
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


/***********************************************************************
 *           DIR_TryPath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryPath( const DOS_FULL_NAME *dir, LPCWSTR name,
                           DOS_FULL_NAME *full_name )
{
    LPSTR p_l = full_name->long_name + strlen(dir->long_name) + 1;
    LPWSTR p_s = full_name->short_name + strlenW(dir->short_name) + 1;

    if ((p_s >= full_name->short_name + sizeof(full_name->short_name)/sizeof(full_name->short_name[0]) - 14) ||
        (p_l >= full_name->long_name + sizeof(full_name->long_name) - 1))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    if (!DOSFS_FindUnixName( dir, name, p_l,
                   sizeof(full_name->long_name) - (p_l - full_name->long_name),
                   p_s, !(DRIVE_GetFlags(dir->drive) & DRIVE_CASE_SENSITIVE) ))
        return FALSE;

    full_name->drive = dir->drive;
    strcpy( full_name->long_name, dir->long_name );
    p_l[-1] = '/';
    strcpyW( full_name->short_name, dir->short_name );
    p_s[-1] = '\\';
    return TRUE;
}

static BOOL DIR_SearchSemicolonedPaths(LPCWSTR name, DOS_FULL_NAME *full_name, LPWSTR pathlist)
{
    LPWSTR next, buffer = NULL;
    INT len = strlenW(name), newlen, currlen = 0;
    BOOL ret = FALSE;

    next = pathlist;
    while (!ret && next)
    {
        static const WCHAR bkslashW[] = {'\\',0};
        LPWSTR cur = next;
        while (*cur == ';') cur++;
        if (!*cur) break;
        next = strchrW( cur, ';' );
        if (next) *next++ = '\0';
        newlen = strlenW(cur) + len + 2;

	if (newlen > currlen)
	{
            if (!(buffer = HeapReAlloc( GetProcessHeap(), 0, buffer, newlen * sizeof(WCHAR))))
                goto done;
	    currlen = newlen;
	}

        strcpyW( buffer, cur );
        strcatW( buffer, bkslashW );
        strcatW( buffer, name );
        ret = DOSFS_GetFullName( buffer, TRUE, full_name );
    }
done:
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}


/***********************************************************************
 *           DIR_TryEnvironmentPath
 *
 * Helper function for DIR_SearchPath.
 * Search in the specified path, or in $PATH if NULL.
 */
static BOOL DIR_TryEnvironmentPath( LPCWSTR name, DOS_FULL_NAME *full_name, LPCWSTR envpath )
{
    LPWSTR path;
    BOOL ret = FALSE;
    DWORD size;
    static const WCHAR pathW[] = {'P','A','T','H',0};

    size = envpath ? strlenW(envpath)+1 : GetEnvironmentVariableW( pathW, NULL, 0 );
    if (!size) return FALSE;
    if (!(path = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
    if (envpath) strcpyW( path, envpath );
    else if (!GetEnvironmentVariableW( pathW, path, size )) goto done;

    ret = DIR_SearchSemicolonedPaths(name, full_name, path);

done:
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}


/***********************************************************************
 *           DIR_TryModulePath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryModulePath( LPCWSTR name, DOS_FULL_NAME *full_name, BOOL win32 )
{
    WCHAR bufferW[MAX_PATH];
    LPWSTR p;

    if (!win32)
    {
        char buffer[OFS_MAXPATHNAME];
	if (!GetCurrentTask()) return FALSE;
	if (!GetModuleFileName16( GetCurrentTask(), buffer, sizeof(buffer) ))
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, buffer, -1, bufferW, MAX_PATH);
    } else {
	if (!GetModuleFileNameW( 0, bufferW, MAX_PATH ) )
            return FALSE;
    }
    if (!(p = strrchrW( bufferW, '\\' ))) return FALSE;
    if (MAX_PATH - (++p - bufferW) <= strlenW(name)) return FALSE;
    strcpyW( p, name );
    return DOSFS_GetFullName( bufferW, TRUE, full_name );
}


/***********************************************************************
 *           DIR_SearchPath
 *
 * Implementation of SearchPathA. 'win32' specifies whether the search
 * order is Win16 (module path last) or Win32 (module path first).
 *
 * FIXME: should return long path names.
 */
DWORD DIR_SearchPath( LPCWSTR path, LPCWSTR name, LPCWSTR ext,
                      DOS_FULL_NAME *full_name, BOOL win32 )
{
    LPCWSTR p;
    LPWSTR tmp = NULL;
    BOOL ret = TRUE;

    /* First check the supplied parameters */

    p = strrchrW( name, '.' );
    if (p && !strchrW( p, '/' ) && !strchrW( p, '\\' ))
        ext = NULL;  /* Ignore the specified extension */
    if (FILE_contains_pathW (name))
        path = NULL;  /* Ignore path if name already contains a path */
    if (path && !*path) path = NULL;  /* Ignore empty path */

    /* Allocate a buffer for the file name and extension */

    if (ext)
    {
        DWORD len = strlenW(name) + strlenW(ext);
        if (!(tmp = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
        strcpyW( tmp, name );
        strcatW( tmp, ext );
        name = tmp;
    }

    /* If the name contains an explicit path, everything's easy */

    if (FILE_contains_pathW(name))
    {
        ret = DOSFS_GetFullName( name, TRUE, full_name );
        goto done;
    }

    /* Search in the specified path */

    if (path)
    {
        ret = DIR_TryEnvironmentPath( name, full_name, path );
        goto done;
    }

    /* Try the path of the current executable (for Win32 search order) */

    if (win32 && DIR_TryModulePath( name, full_name, win32 )) goto done;

    /* Try the current directory */

    if (DOSFS_GetFullName( name, TRUE, full_name )) goto done;

    /* Try the Windows system directory */

    if (DIR_TryPath( &DIR_System, name, full_name ))
        goto done;

    /* Try the Windows directory */

    if (DIR_TryPath( &DIR_Windows, name, full_name ))
        goto done;

    /* Try the path of the current executable (for Win16 search order) */

    if (!win32 && DIR_TryModulePath( name, full_name, win32 )) goto done;

    /* Try all directories in path */

    ret = DIR_TryEnvironmentPath( name, full_name, NULL );

done:
    if (tmp) HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
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
    LPSTR res;
    DOS_FULL_NAME full_name;

    if (!DIR_SearchPath( path, name, ext, &full_name, TRUE ))
    {
	SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
    }

TRACE("found %s %s\n", full_name.long_name, debugstr_w(full_name.short_name));
TRACE("drive %c: root %s\n", 'A' + full_name.drive, DRIVE_GetRoot(full_name.drive));

    lstrcpynW( buffer, full_name.short_name, buflen );
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.drive ));
    while (*res == '/') res++;
    if (buflen)
    {
        LPWSTR p;
        if (buflen > 3)
        {
            MultiByteToWideChar(DRIVE_GetCodepage(full_name.drive), 0,
                                res, -1, buffer + 3, buflen - 3);
            buffer[buflen - 1] = 0;
        }
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart) *lastpart = strrchrW( buffer, '\\' ) + 1;
    }
    TRACE("Returning %s\n", debugstr_w(buffer) );
    return strlenW(buffer);
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
