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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#include "winbase.h"
#include "wine/winbase16.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "winreg.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

static DOS_FULL_NAME DIR_Windows;
static DOS_FULL_NAME DIR_System;


/***********************************************************************
 *           DIR_GetPath
 *
 * Get a path name from the wine.ini file and make sure it is valid.
 */
static int DIR_GetPath( const char *keyname, const char *defval,
                        DOS_FULL_NAME *full_name, BOOL warn )
{
    char path[MAX_PATHNAME_LEN];
    BY_HANDLE_FILE_INFORMATION info;
    const char *mess = "does not exist";

    PROFILE_GetWineIniString( "wine", keyname, defval, path, sizeof(path) );
    if (!DOSFS_GetFullName( path, TRUE, full_name ) ||
        (!FILE_Stat( full_name->long_name, &info ) && (mess=strerror(errno)))||
        (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (mess="not a directory")))
    {
        if (warn)
           MESSAGE("Invalid path '%s' for %s directory: %s\n", path, keyname, mess);
        return 0;
    }
    return 1;
}


/***********************************************************************
 *           DIR_Init
 */
int DIR_Init(void)
{
    char path[MAX_PATHNAME_LEN];
    DOS_FULL_NAME tmp_dir, profile_dir;
    int drive;
    const char *cwd;

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
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, cwd );
    }

    if (!(DIR_GetPath( "windows", "c:\\windows", &DIR_Windows, TRUE )) ||
	!(DIR_GetPath( "system", "c:\\windows\\system", &DIR_System, TRUE )) ||
	!(DIR_GetPath( "temp", "c:\\windows", &tmp_dir, TRUE )))
    {
	PROFILE_UsageWineIni();
        return 0;
    }
    if (-1 == access( tmp_dir.long_name, W_OK ))
    {
    	if (errno==EACCES)
	{
		MESSAGE("Warning: the temporary directory '%s' (specified in wine configuration file) is not writeable.\n", tmp_dir.long_name);
		PROFILE_UsageWineIni();
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

    PROFILE_GetWineIniString("wine", "path", "c:\\windows;c:\\windows\\system",
                             path, sizeof(path) );
    if (strchr(path, '/'))
    {
	MESSAGE("Fix your wine config to use DOS drive syntax in [wine] 'Path=' statement! (no '/' allowed)\n");
	PROFILE_UsageWineIni();
	ExitProcess(1);
    }

    /* Set the environment variables */

    SetEnvironmentVariableA( "PATH", path );
    SetEnvironmentVariableA( "TEMP", tmp_dir.short_name );
    SetEnvironmentVariableA( "TMP", tmp_dir.short_name );
    SetEnvironmentVariableA( "windir", DIR_Windows.short_name );
    SetEnvironmentVariableA( "winsysdir", DIR_System.short_name );

    /* set COMSPEC only if it doesn't exist already */
    if (!GetEnvironmentVariableA( "COMSPEC", NULL, 0 ))
        SetEnvironmentVariableA( "COMSPEC", "c:\\command.com" );

    TRACE("WindowsDir = %s (%s)\n",
          DIR_Windows.short_name, DIR_Windows.long_name );
    TRACE("SystemDir  = %s (%s)\n",
          DIR_System.short_name, DIR_System.long_name );
    TRACE("TempDir    = %s (%s)\n",
          tmp_dir.short_name, tmp_dir.long_name );
    TRACE("Path       = %s\n", path );
    TRACE("Cwd        = %c:\\%s\n",
          'A' + drive, DRIVE_GetDosCwd( drive ) );

    if (DIR_GetPath( "profile", "", &profile_dir, FALSE ))
    {
        TRACE("USERPROFILE= %s\n", profile_dir.short_name );
        SetEnvironmentVariableA( "USERPROFILE", profile_dir.short_name );
    }	

    TRACE("SYSTEMROOT = %s\n", DIR_Windows.short_name );
    SetEnvironmentVariableA( "SYSTEMROOT", DIR_Windows.short_name );

    return 1;
}


/***********************************************************************
 *           GetTempPathA   (KERNEL32.@)
 */
UINT WINAPI GetTempPathA( UINT count, LPSTR path )
{
    UINT ret;
    if (!(ret = GetEnvironmentVariableA( "TMP", path, count )))
        if (!(ret = GetEnvironmentVariableA( "TEMP", path, count )))
            if (!(ret = GetCurrentDirectoryA( count, path )))
                return 0;
    if (count && (ret < count - 1) && (path[ret-1] != '\\'))
    {
        path[ret++] = '\\';
        path[ret]   = '\0';
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
    UINT ret;
    if (!(ret = GetEnvironmentVariableW( tmp, path, count )))
        if (!(ret = GetEnvironmentVariableW( temp, path, count )))
            if (!(ret = GetCurrentDirectoryW( count, path )))
                return 0;
    if (count && (ret < count - 1) && (path[ret-1] != '\\'))
    {
        path[ret++] = '\\';
        path[ret]   = '\0';
    }
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
 *           GetWindowsDirectoryA   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryW.
 */
UINT WINAPI GetWindowsDirectoryA( LPSTR path, UINT count )
{
    UINT len = strlen( DIR_Windows.short_name ) + 1;
    if (path && count >= len)
    {
        strcpy( path, DIR_Windows.short_name );
        len--;
    }
    return len;
}


/***********************************************************************
 *           GetWindowsDirectoryW   (KERNEL32.@)
 *
 * Return value:
 * If buffer is large enough to hold full path and terminating '\0' character
 * function copies path to buffer and returns length of the path without '\0'.
 * Otherwise function returns required size including '\0' character and
 * does not touch the buffer.
 */
UINT WINAPI GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    UINT len = MultiByteToWideChar( CP_ACP, 0, DIR_Windows.short_name, -1, NULL, 0 );
    if (path && count >= len)
    {
        MultiByteToWideChar( CP_ACP, 0, DIR_Windows.short_name, -1, path, count );
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
 *           GetSystemDirectoryA   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryW.
 */
UINT WINAPI GetSystemDirectoryA( LPSTR path, UINT count )
{
    UINT len = strlen( DIR_System.short_name ) + 1;
    if (path && count >= len)
    {
        strcpy( path, DIR_System.short_name );
        len--;
    }
    return len;
}


/***********************************************************************
 *           GetSystemDirectoryW   (KERNEL32.@)
 *
 * See comment for GetWindowsDirectoryW.
 */
UINT WINAPI GetSystemDirectoryW( LPWSTR path, UINT count )
{
    UINT len = MultiByteToWideChar( CP_ACP, 0, DIR_System.short_name, -1, NULL, 0 );
    if (path && count >= len)
    {
        MultiByteToWideChar( CP_ACP, 0, DIR_System.short_name, -1, path, count );
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
 *           CreateDirectoryA   (KERNEL32.@)
 * RETURNS:
 *	TRUE : success
 *	FALSE : failure
 *		ERROR_DISK_FULL:	on full disk
 *		ERROR_ALREADY_EXISTS:	if directory name exists (even as file)
 *		ERROR_ACCESS_DENIED:	on permission problems
 *		ERROR_FILENAME_EXCED_RANGE: too long filename(s)
 */
BOOL WINAPI CreateDirectoryA( LPCSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    DOS_FULL_NAME full_name;

    TRACE_(file)("(%s,%p)\n", path, lpsecattribs );
    if (DOSFS_GetDevice( path ))
    {
        TRACE_(file)("cannot use device '%s'!\n",path);
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    if (!DOSFS_GetFullName( path, FALSE, &full_name )) return 0;
    if (mkdir( full_name.long_name, 0777 ) == -1) {
        WARN_(file)("Error '%s' trying to create directory '%s'\n", strerror(errno), full_name.long_name);
	/* the FILE_SetDosError() generated error codes don't match the 
	 * CreateDirectory ones for some errnos */
	switch (errno) {
	case EEXIST: SetLastError(ERROR_ALREADY_EXISTS); break;
	case ENOSPC: SetLastError(ERROR_DISK_FULL); break;
	default: FILE_SetDosError();break;
	}
	return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI CreateDirectoryW( LPCWSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL ret = CreateDirectoryA( xpath, lpsecattribs );
    HeapFree( GetProcessHeap(), 0, xpath );
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
 *           RemoveDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryA( LPCSTR path )
{
    DOS_FULL_NAME full_name;

    TRACE_(file)("'%s'\n", path );

    if (DOSFS_GetDevice( path ))
    {
        TRACE_(file)("cannot remove device '%s'!\n", path);
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
 *           RemoveDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI RemoveDirectoryW( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL ret = RemoveDirectoryA( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           DIR_TryPath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryPath( const DOS_FULL_NAME *dir, LPCSTR name,
                           DOS_FULL_NAME *full_name )
{
    LPSTR p_l = full_name->long_name + strlen(dir->long_name) + 1;
    LPSTR p_s = full_name->short_name + strlen(dir->short_name) + 1;

    if ((p_s >= full_name->short_name + sizeof(full_name->short_name) - 14) ||
        (p_l >= full_name->long_name + sizeof(full_name->long_name) - 1))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    if (!DOSFS_FindUnixName( dir->long_name, name, p_l,
                   sizeof(full_name->long_name) - (p_l - full_name->long_name),
                   p_s, !(DRIVE_GetFlags(dir->drive) & DRIVE_CASE_SENSITIVE) ))
        return FALSE;
    strcpy( full_name->long_name, dir->long_name );
    p_l[-1] = '/';
    strcpy( full_name->short_name, dir->short_name );
    p_s[-1] = '\\';
    return TRUE;
}

static BOOL DIR_SearchSemicolonedPaths(LPCSTR name, DOS_FULL_NAME *full_name, LPSTR pathlist)
{
    LPSTR next, buffer = NULL;
    INT len = strlen(name), newlen, currlen = 0;
    BOOL ret = FALSE;
   
    next = pathlist;
    while (!ret && next)
    {
        LPSTR cur = next;
        while (*cur == ';') cur++;
        if (!*cur) break;
        next = strchr( cur, ';' );
        if (next) *next++ = '\0';
	newlen = strlen(cur) + len + 2;
	if (newlen > currlen)
	{
            if (!(buffer = HeapReAlloc( GetProcessHeap(), 0, buffer, newlen)))
                goto done;
	    currlen = newlen;
	}
        strcpy( buffer, cur );
        strcat( buffer, "\\" );
        strcat( buffer, name );
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
static BOOL DIR_TryEnvironmentPath( LPCSTR name, DOS_FULL_NAME *full_name, LPCSTR envpath )
{
    LPSTR path;
    BOOL ret = FALSE;
    DWORD size;

    size = envpath ? strlen(envpath)+1 : GetEnvironmentVariableA( "PATH", NULL, 0 );
    if (!size) return FALSE;
    if (!(path = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    if (envpath) strcpy( path, envpath );
    else if (!GetEnvironmentVariableA( "PATH", path, size )) goto done;

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
static BOOL DIR_TryModulePath( LPCSTR name, DOS_FULL_NAME *full_name, BOOL win32 )
{
    /* FIXME: for now, GetModuleFileNameA can't return more */
    /* than OFS_MAXPATHNAME. This may change with Win32. */

    char buffer[OFS_MAXPATHNAME];
    LPSTR p;

    if (!win32)
    {
	if (!GetCurrentTask()) return FALSE;
	if (!GetModuleFileName16( GetCurrentTask(), buffer, sizeof(buffer) ))
		buffer[0]='\0';
    } else {
	if (!GetModuleFileNameA( 0, buffer, sizeof(buffer) ))
		buffer[0]='\0';
    }
    if (!(p = strrchr( buffer, '\\' ))) return FALSE;
    if (sizeof(buffer) - (++p - buffer) <= strlen(name)) return FALSE;
    strcpy( p, name );
    return DOSFS_GetFullName( buffer, TRUE, full_name );
}


/***********************************************************************
 *           DIR_TryAppPath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryAppPath( LPCSTR name, DOS_FULL_NAME *full_name )
{
    HKEY hkAppPaths, hkApp;
    char lpAppName[MAX_PATHNAME_LEN], lpAppPaths[MAX_PATHNAME_LEN];
    LPSTR lpFileName;
    BOOL res = FALSE;
    DWORD type, count;

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths", &hkAppPaths) != ERROR_SUCCESS)
	return FALSE;

    if (GetModuleFileNameA(0, lpAppName, sizeof(lpAppName)) == 0)
    {
	WARN("huh, module not found ??\n");
	goto end;
    }
    lpFileName = strrchr(lpAppName, '\\');
    if (!lpFileName)
	goto end;
    else lpFileName++; /* skip '\\' */
    if (RegOpenKeyA(hkAppPaths, lpFileName, &hkApp) != ERROR_SUCCESS)
	goto end;
    count = sizeof(lpAppPaths);
    if (RegQueryValueExA(hkApp, "Path", 0, &type, (LPBYTE)lpAppPaths, &count) != ERROR_SUCCESS)
        goto end;
    TRACE("successfully opened App Paths for '%s'\n", lpFileName);

    res = DIR_SearchSemicolonedPaths(name, full_name, lpAppPaths);
end:
    if (hkApp)
	RegCloseKey(hkApp);
    if (hkAppPaths)
	RegCloseKey(hkAppPaths);
    return res;
}

/***********************************************************************
 *           DIR_SearchPath
 *
 * Implementation of SearchPathA. 'win32' specifies whether the search
 * order is Win16 (module path last) or Win32 (module path first).
 *
 * FIXME: should return long path names.
 */
DWORD DIR_SearchPath( LPCSTR path, LPCSTR name, LPCSTR ext,
                      DOS_FULL_NAME *full_name, BOOL win32 )
{
    LPCSTR p;
    LPSTR tmp = NULL;
    BOOL ret = TRUE;

    /* First check the supplied parameters */

    p = strrchr( name, '.' );
    if (p && !strchr( p, '/' ) && !strchr( p, '\\' ))
        ext = NULL;  /* Ignore the specified extension */
    if (FILE_contains_path (name))
        path = NULL;  /* Ignore path if name already contains a path */
    if (path && !*path) path = NULL;  /* Ignore empty path */

    /* Allocate a buffer for the file name and extension */

    if (ext)
    {
        DWORD len = strlen(name) + strlen(ext);
        if (!(tmp = HeapAlloc( GetProcessHeap(), 0, len + 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
        strcpy( tmp, name );
        strcat( tmp, ext );
        name = tmp;
    }

    /* If the name contains an explicit path, everything's easy */

    if (FILE_contains_path(name))
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

    /* Try the "App Paths" entry if existing (undocumented ??) */
    if (DIR_TryAppPath(name, full_name))
	goto done;
    
    /* Try all directories in path */

    ret = DIR_TryEnvironmentPath( name, full_name, NULL );

done:
    if (tmp) HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
}


/***********************************************************************
 * SearchPathA [KERNEL32.@]
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
DWORD WINAPI SearchPathA( LPCSTR path, LPCSTR name, LPCSTR ext, DWORD buflen,
                            LPSTR buffer, LPSTR *lastpart )
{
    LPSTR p, res;
    DOS_FULL_NAME full_name;

    if (!DIR_SearchPath( path, name, ext, &full_name, TRUE ))
    {
	SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
    }
    lstrcpynA( buffer, full_name.short_name, buflen );
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.short_name[0] - 'A' ));
    while (*res == '/') res++;
    if (buflen)
    {
        if (buflen > 3) lstrcpynA( buffer + 3, res, buflen - 3 );
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart) *lastpart = strrchr( buffer, '\\' ) + 1;
    }
    TRACE("Returning %d\n", strlen(res) + 3 );
    return strlen(res) + 3;
}


/***********************************************************************
 *           SearchPathW   (KERNEL32.@)
 */
DWORD WINAPI SearchPathW( LPCWSTR path, LPCWSTR name, LPCWSTR ext,
                            DWORD buflen, LPWSTR buffer, LPWSTR *lastpart )
{
    LPWSTR p;
    LPSTR res;
    DOS_FULL_NAME full_name;

    LPSTR pathA = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    LPSTR extA  = HEAP_strdupWtoA( GetProcessHeap(), 0, ext );
    DWORD ret = DIR_SearchPath( pathA, nameA, extA, &full_name, TRUE );
    HeapFree( GetProcessHeap(), 0, extA );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, pathA );
    if (!ret) return 0;

    if (buflen > 0 && !MultiByteToWideChar( CP_ACP, 0, full_name.short_name, -1, buffer, buflen ))
        buffer[buflen-1] = 0;
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.short_name[0] - 'A' ));
    while (*res == '/') res++;
    if (buflen)
    {
        if (buflen > 3)
        {
            if (!MultiByteToWideChar( CP_ACP, 0, res, -1, buffer+3, buflen-3 ))
                buffer[buflen-1] = 0;
        }
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart)
        {
            for (p = *lastpart = buffer; *p; p++)
                if (*p == '\\') *lastpart = p + 1;
        }
    }
    return strlen(res) + 3;
}


/***********************************************************************
 *           search_alternate_path
 *
 *
 * FIXME: should return long path names.?
 */
static BOOL search_alternate_path(LPCSTR dll_path, LPCSTR name, LPCSTR ext,
                                  DOS_FULL_NAME *full_name)
{
    LPCSTR p;
    LPSTR tmp = NULL;
    BOOL ret = TRUE;

    /* First check the supplied parameters */

    p = strrchr( name, '.' );
    if (p && !strchr( p, '/' ) && !strchr( p, '\\' ))
        ext = NULL;  /* Ignore the specified extension */

    /* Allocate a buffer for the file name and extension */

    if (ext)
    {
        DWORD len = strlen(name) + strlen(ext);
        if (!(tmp = HeapAlloc( GetProcessHeap(), 0, len + 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
        strcpy( tmp, name );
        strcat( tmp, ext );
        name = tmp;
    }

    if (DIR_TryEnvironmentPath (name, full_name, dll_path))
        ;
    else if (DOSFS_GetFullName (name, TRUE, full_name)) /* current dir */
        ;
    else if (DIR_TryPath (&DIR_System, name, full_name)) /* System dir */
        ;
    else if (DIR_TryPath (&DIR_Windows, name, full_name)) /* Windows dir */
        ;
    else
        ret = DIR_TryEnvironmentPath( name, full_name, NULL );

    if (tmp) HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
}


/***********************************************************************
 * DIR_SearchAlternatePath
 *
 * Searches for a specified file in the search path.
 *
 * PARAMS
 *    dll_path	[I] Path to search
 *    name	[I] Filename to search for.
 *    ext	[I] File extension to append to file name. The first
 *		    character must be a period. This parameter is
 *                  specified only if the filename given does not
 *                  contain an extension.
 *    buflen	[I] size of buffer, in characters
 *    buffer	[O] buffer for found filename
 *    lastpart  [O] address of pointer to last used character in
 *                  buffer (the final '\') (May be NULL)
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
 */
DWORD DIR_SearchAlternatePath( LPCSTR dll_path, LPCSTR name, LPCSTR ext,
                               DWORD buflen, LPSTR buffer, LPSTR *lastpart )
{
    LPSTR p, res;
    DOS_FULL_NAME full_name;

    if (!search_alternate_path( dll_path, name, ext, &full_name))
    {
	SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
    }
    lstrcpynA( buffer, full_name.short_name, buflen );
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.short_name[0] - 'A' ));
    while (*res == '/') res++;
    if (buflen)
    {
        if (buflen > 3) lstrcpynA( buffer + 3, res, buflen - 3 );
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart) *lastpart = strrchr( buffer, '\\' ) + 1;
    }
    TRACE("Returning %d\n", strlen(res) + 3 );
    return strlen(res) + 3;
}
