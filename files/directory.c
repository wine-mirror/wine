/*
 * DOS directories functions
 *
 * Copyright 1995 Alexandre Julliard
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
#include "wine/winestring.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "process.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dosfs);
DECLARE_DEBUG_CHANNEL(file);

static DOS_FULL_NAME DIR_Windows;
static DOS_FULL_NAME DIR_System;


/***********************************************************************
 *           DIR_GetPath
 *
 * Get a path name from the wine.ini file and make sure it is valid.
 */
static int DIR_GetPath( const char *keyname, const char *defval,
                        DOS_FULL_NAME *full_name )
{
    char path[MAX_PATHNAME_LEN];
    BY_HANDLE_FILE_INFORMATION info;

    PROFILE_GetWineIniString( "wine", keyname, defval, path, sizeof(path) );
    if (!DOSFS_GetFullName( path, TRUE, full_name ) ||
        !FILE_Stat( full_name->long_name, &info ) ||
        !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        MESSAGE("Invalid path '%s' for %s directory\n", path, keyname);
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
        MESSAGE("Warning: could not find wine.conf [Drive x] entry "
            "for current working directory %s; "
	    "starting in windows directory.\n", cwd );
    }
    else
    {
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, cwd );
    }

    if (!(DIR_GetPath( "windows", "c:\\windows", &DIR_Windows )) ||
	!(DIR_GetPath( "system", "c:\\windows\\system", &DIR_System )) ||
	!(DIR_GetPath( "temp", "c:\\windows", &tmp_dir )))
    {
	PROFILE_UsageWineIni();
        return 0;
    }
    if (-1 == access( tmp_dir.long_name, W_OK ))
    {
    	if (errno==EACCES)
	{
		MESSAGE("Warning: The Temporary Directory (as specified in your configuration file) is NOT writeable.\n");
		PROFILE_UsageWineIni();
	}
	else
		MESSAGE("Warning: Access to Temporary Directory failed (%s).\n",
		    strerror(errno));
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
	MESSAGE("No '/' allowed in [wine] 'Path=' statement of wine.conf !\n");
	PROFILE_UsageWineIni();
	ExitProcess(1);
    }

    /* Set the environment variables */

    SetEnvironmentVariableA( "PATH", path );
    SetEnvironmentVariableA( "COMSPEC", "c:\\command.com" );
    SetEnvironmentVariableA( "TEMP", tmp_dir.short_name );
    SetEnvironmentVariableA( "windir", DIR_Windows.short_name );
    SetEnvironmentVariableA( "winsysdir", DIR_System.short_name );

    TRACE("WindowsDir = %s (%s)\n",
          DIR_Windows.short_name, DIR_Windows.long_name );
    TRACE("SystemDir  = %s (%s)\n",
          DIR_System.short_name, DIR_System.long_name );
    TRACE("TempDir    = %s (%s)\n",
          tmp_dir.short_name, tmp_dir.long_name );
    TRACE("Path       = %s\n", path );
    TRACE("Cwd        = %c:\\%s\n",
          'A' + drive, DRIVE_GetDosCwd( drive ) );

    if (DIR_GetPath( "profile", "", &profile_dir ))
    {
        TRACE("USERPROFILE= %s\n", profile_dir.short_name );
        SetEnvironmentVariableA( "USERPROFILE", profile_dir.short_name );
    }	

    TRACE("SYSTEMROOT = %s\n", DIR_Windows.short_name );
    SetEnvironmentVariableA( "SYSTEMROOT", DIR_Windows.short_name );

    return 1;
}


/***********************************************************************
 *           GetTempPathA   (KERNEL32.292)
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
 *           GetTempPathW   (KERNEL32.293)
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
 */
BYTE WINAPI GetTempDrive( BYTE ignored )
{
    char *buffer;
    BYTE ret;
    UINT len = GetTempPathA( 0, NULL );

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len + 1 )) )
      return DRIVE_GetCurrentDrive() + 'A';

    /* FIXME: apparently Windows does something with the ignored byte */
    if (!GetTempPathA( len, buffer )) buffer[0] = 'C';
    ret = buffer[0];
    HeapFree( GetProcessHeap(), 0, buffer );
    return toupper(ret);
}


UINT WINAPI WIN16_GetTempDrive( BYTE ignored )
{
    /* A closer look at krnl386.exe shows what the SDK doesn't mention:
     *
     * returns:
     *	 AL: driveletter
     *   AH: ':'		- yes, some kernel code even does stosw with
     *                            the returned AX.
     *   DX: 1 for success
     */
    return MAKELONG( GetTempDrive(ignored) | (':' << 8), 1 );
}


/***********************************************************************
 *           GetWindowsDirectory16   (KERNEL.134)
 */
UINT16 WINAPI GetWindowsDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetWindowsDirectoryA( path, count );
}


/***********************************************************************
 *           GetWindowsDirectoryA   (KERNEL32.311)
 */
UINT WINAPI GetWindowsDirectoryA( LPSTR path, UINT count )
{
    if (path) lstrcpynA( path, DIR_Windows.short_name, count );
    return strlen( DIR_Windows.short_name );
}


/***********************************************************************
 *           GetWindowsDirectoryW   (KERNEL32.312)
 */
UINT WINAPI GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    if (path) lstrcpynAtoW( path, DIR_Windows.short_name, count );
    return strlen( DIR_Windows.short_name );
}


/***********************************************************************
 *           GetSystemDirectory16   (KERNEL.135)
 */
UINT16 WINAPI GetSystemDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetSystemDirectoryA( path, count );
}


/***********************************************************************
 *           GetSystemDirectoryA   (KERNEL32.282)
 */
UINT WINAPI GetSystemDirectoryA( LPSTR path, UINT count )
{
    if (path) lstrcpynA( path, DIR_System.short_name, count );
    return strlen( DIR_System.short_name );
}


/***********************************************************************
 *           GetSystemDirectoryW   (KERNEL32.283)
 */
UINT WINAPI GetSystemDirectoryW( LPWSTR path, UINT count )
{
    if (path) lstrcpynAtoW( path, DIR_System.short_name, count );
    return strlen( DIR_System.short_name );
}


/***********************************************************************
 *           CreateDirectory16   (KERNEL.144)
 */
BOOL16 WINAPI CreateDirectory16( LPCSTR path, LPVOID dummy )
{
    TRACE_(file)("(%s,%p)\n", path, dummy );
    return (BOOL16)CreateDirectoryA( path, NULL );
}


/***********************************************************************
 *           CreateDirectoryA   (KERNEL32.39)
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
        WARN_(file)("Errno %i trying to create directory %s.\n", errno, full_name.long_name);
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
 *           CreateDirectoryW   (KERNEL32.42)
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
 *           CreateDirectoryExA   (KERNEL32.40)
 */
BOOL WINAPI CreateDirectoryExA( LPCSTR template, LPCSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectoryA(path,lpsecattribs);
}


/***********************************************************************
 *           CreateDirectoryExW   (KERNEL32.41)
 */
BOOL WINAPI CreateDirectoryExW( LPCWSTR template, LPCWSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectoryW(path,lpsecattribs);
}


/***********************************************************************
 *           RemoveDirectory16   (KERNEL)
 */
BOOL16 WINAPI RemoveDirectory16( LPCSTR path )
{
    return (BOOL16)RemoveDirectoryA( path );
}


/***********************************************************************
 *           RemoveDirectoryA   (KERNEL32.437)
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
 *           RemoveDirectoryW   (KERNEL32.438)
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


/***********************************************************************
 *           DIR_TryEnvironmentPath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryEnvironmentPath( LPCSTR name, DOS_FULL_NAME *full_name )
{
    LPSTR path, next, buffer;
    BOOL ret = FALSE;
    INT len = strlen(name);
    DWORD size = GetEnvironmentVariableA( "PATH", NULL, 0 );

    if (!size) return FALSE;
    if (!(path = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    if (!GetEnvironmentVariableA( "PATH", path, size )) goto done;
    next = path;
    while (!ret && next)
    {
        LPSTR cur = next;
        while (*cur == ';') cur++;
        if (!*cur) break;
        next = strchr( cur, ';' );
        if (next) *next++ = '\0';
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, strlen(cur) + len + 2)))
            goto done;
        strcpy( buffer, cur );
        strcat( buffer, "\\" );
        strcat( buffer, name );
        ret = DOSFS_GetFullName( buffer, TRUE, full_name );
        HeapFree( GetProcessHeap(), 0, buffer );
    }

done:
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}


/***********************************************************************
 *           DIR_TryModulePath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL DIR_TryModulePath( LPCSTR name, DOS_FULL_NAME *full_name )
{
    PDB	*pdb = PROCESS_Current();

    /* FIXME: for now, GetModuleFileNameA can't return more */
    /* than OFS_MAXPATHNAME. This may change with Win32. */

    char buffer[OFS_MAXPATHNAME];
    LPSTR p;

    if (pdb->flags & PDB32_WIN16_PROC) {
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
    DWORD len;
    LPCSTR p;
    LPSTR tmp = NULL;
    BOOL ret = TRUE;

    /* First check the supplied parameters */

    p = strrchr( name, '.' );
    if (p && !strchr( p, '/' ) && !strchr( p, '\\' ))
        ext = NULL;  /* Ignore the specified extension */
    if ((*name && (name[1] == ':')) ||
        strchr( name, '/' ) || strchr( name, '\\' ))
        path = NULL;  /* Ignore path if name already contains a path */
    if (path && !*path) path = NULL;  /* Ignore empty path */

    len = strlen(name);
    if (ext) len += strlen(ext);
    if (path) len += strlen(path) + 1;

    /* Allocate a buffer for the file name and extension */

    if (path || ext)
    {
        if (!(tmp = HeapAlloc( GetProcessHeap(), 0, len + 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
        if (path)
        {
            strcpy( tmp, path );
            strcat( tmp, "\\" );
            strcat( tmp, name );
        }
        else strcpy( tmp, name );
        if (ext) strcat( tmp, ext );
        name = tmp;
    }
    
    /* If we have an explicit path, everything's easy */

    if (path || (*name && (name[1] == ':')) ||
        strchr( name, '/' ) || strchr( name, '\\' ))
    {
        ret = DOSFS_GetFullName( name, TRUE, full_name );
        goto done;
    }

    /* Try the path of the current executable (for Win32 search order) */

    if (win32 && DIR_TryModulePath( name, full_name )) goto done;

    /* Try the current directory */

    if (DOSFS_GetFullName( name, TRUE, full_name )) goto done;

    /* Try the Windows system directory */

    if (DIR_TryPath( &DIR_System, name, full_name ))
        goto done;

    /* Try the Windows directory */

    if (DIR_TryPath( &DIR_Windows, name, full_name ))
        goto done;

    /* Try the path of the current executable (for Win16 search order) */

    if (!win32 && DIR_TryModulePath( name, full_name )) goto done;

    /* Try all directories in path */

    ret = DIR_TryEnvironmentPath( name, full_name );

done:
    if (tmp) HeapFree( GetProcessHeap(), 0, tmp );
    return ret;
}


/***********************************************************************
 * SearchPathA [KERNEL32.447]
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
 *           SearchPathW   (KERNEL32.448)
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

    lstrcpynAtoW( buffer, full_name.short_name, buflen );
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.short_name[0] - 'A' ));
    while (*res == '/') res++;
    if (buflen)
    {
        if (buflen > 3) lstrcpynAtoW( buffer + 3, res, buflen - 3 ); 
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart)
        {
            for (p = *lastpart = buffer; *p; p++)
                if (*p == '\\') *lastpart = p + 1;
        }
    }
    return strlen(res) + 3;
}


