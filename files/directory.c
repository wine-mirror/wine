/*
 * DOS directories functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "debug.h"

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
        MSG("Invalid path '%s' for %s directory\n", path, keyname);
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
    DOS_FULL_NAME tmp_dir;
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
        MSG("Warning: could not find DOS drive for cwd %s; "
	    "starting in windows directory.\n", cwd );
    }
    else
    {
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, cwd );
    }

    if (!(DIR_GetPath( "windows", "c:\\windows", &DIR_Windows )))
        return 0;
    if (!(DIR_GetPath( "system", "c:\\windows\\system", &DIR_System )))
        return 0;
    if (!(DIR_GetPath( "temp", "c:\\windows", &tmp_dir )))
        return 0;
    if (-1 == access( tmp_dir.long_name, W_OK ))
    {
    	if (errno==EACCES)
		MSG("Warning: The Temporary Directory (as specified in wine.conf) is NOT writeable. Please check your configuration.\n");
	else
		MSG("Warning: Access to Temporary Directory failed (%s).\n",
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

    /* Set the environment variables */

    SetEnvironmentVariable32A( "PATH", path );
    SetEnvironmentVariable32A( "TEMP", tmp_dir.short_name );
    SetEnvironmentVariable32A( "windir", DIR_Windows.short_name );
    SetEnvironmentVariable32A( "winsysdir", DIR_System.short_name );

    TRACE(dosfs, "WindowsDir = %s (%s)\n",
          DIR_Windows.short_name, DIR_Windows.long_name );
    TRACE(dosfs, "SystemDir  = %s (%s)\n",
          DIR_System.short_name, DIR_System.long_name );
    TRACE(dosfs, "TempDir    = %s (%s)\n",
          tmp_dir.short_name, tmp_dir.long_name );
    TRACE(dosfs, "Path       = %s\n", path );
    TRACE(dosfs, "Cwd        = %c:\\%s\n",
          'A' + drive, DRIVE_GetDosCwd( drive ) );

    return 1;
}


/***********************************************************************
 *           GetTempPath32A   (KERNEL32.292)
 */
UINT32 WINAPI GetTempPath32A( UINT32 count, LPSTR path )
{
    UINT32 ret;
    if (!(ret = GetEnvironmentVariable32A( "TMP", path, count )))
        if (!(ret = GetEnvironmentVariable32A( "TEMP", path, count )))
            ret = GetCurrentDirectory32A( count, path );
    return ret;
}


/***********************************************************************
 *           GetTempPath32W   (KERNEL32.293)
 */
UINT32 WINAPI GetTempPath32W( UINT32 count, LPWSTR path )
{
    static const WCHAR tmp[]  = { 'T', 'M', 'P', 0 };
    static const WCHAR temp[] = { 'T', 'E', 'M', 'P', 0 };
    UINT32 ret;
    if (!(ret = GetEnvironmentVariable32W( tmp, path, count )))
        if (!(ret = GetEnvironmentVariable32W( temp, path, count )))
            ret = GetCurrentDirectory32W( count, path );
    return ret;
}


/***********************************************************************
 *           DIR_GetWindowsUnixDir
 */
UINT32 DIR_GetWindowsUnixDir( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_Windows.long_name, count );
    return strlen( DIR_Windows.long_name );
}


/***********************************************************************
 *           DIR_GetSystemUnixDir
 */
UINT32 DIR_GetSystemUnixDir( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_System.long_name, count );
    return strlen( DIR_System.long_name );
}


/***********************************************************************
 *           GetTempDrive   (KERNEL.92)
 */
BYTE WINAPI GetTempDrive( BYTE ignored )
{
    char buffer[2];
    /* FIXME: apparently Windows does something with the ignored byte */
    if (!GetTempPath32A( sizeof(buffer), buffer )) buffer[0] = 'C';
    return toupper(buffer[0]) - 'A';
}


UINT32 WINAPI WIN16_GetTempDrive( BYTE ignored )
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
    return (UINT16)GetWindowsDirectory32A( path, count );
}


/***********************************************************************
 *           GetWindowsDirectory32A   (KERNEL32.311)
 */
UINT32 WINAPI GetWindowsDirectory32A( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_Windows.short_name, count );
    return strlen( DIR_Windows.short_name );
}


/***********************************************************************
 *           GetWindowsDirectory32W   (KERNEL32.312)
 */
UINT32 WINAPI GetWindowsDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_Windows.short_name, count );
    return strlen( DIR_Windows.short_name );
}


/***********************************************************************
 *           GetSystemDirectory16   (KERNEL.135)
 */
UINT16 WINAPI GetSystemDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetSystemDirectory32A( path, count );
}


/***********************************************************************
 *           GetSystemDirectory32A   (KERNEL32.282)
 */
UINT32 WINAPI GetSystemDirectory32A( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_System.short_name, count );
    return strlen( DIR_System.short_name );
}


/***********************************************************************
 *           GetSystemDirectory32W   (KERNEL32.283)
 */
UINT32 WINAPI GetSystemDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_System.short_name, count );
    return strlen( DIR_System.short_name );
}


/***********************************************************************
 *           CreateDirectory16   (KERNEL.144)
 */
BOOL16 WINAPI CreateDirectory16( LPCSTR path, LPVOID dummy )
{
    TRACE(file,"(%s,%p)\n", path, dummy );
    return (BOOL16)CreateDirectory32A( path, NULL );
}


/***********************************************************************
 *           CreateDirectory32A   (KERNEL32.39)
 */
BOOL32 WINAPI CreateDirectory32A( LPCSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    DOS_FULL_NAME full_name;

    TRACE(file, "(%s,%p)\n", path, lpsecattribs );
    if (DOSFS_IsDevice( path ))
    {
        TRACE(file, "cannot use device '%s'!\n",path);
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!DOSFS_GetFullName( path, FALSE, &full_name )) return 0;
    if ((mkdir( full_name.long_name, 0777 ) == -1) && (errno != EEXIST))
    {
      WARN (file, "Errno %i trying to create directory %s.\n", errno, full_name.long_name);
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDirectory32W   (KERNEL32.42)
 */
BOOL32 WINAPI CreateDirectory32W( LPCWSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = CreateDirectory32A( xpath, lpsecattribs );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           CreateDirectoryEx32A   (KERNEL32.40)
 */
BOOL32 WINAPI CreateDirectoryEx32A( LPCSTR template, LPCSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectory32A(path,lpsecattribs);
}


/***********************************************************************
 *           CreateDirectoryEx32W   (KERNEL32.41)
 */
BOOL32 WINAPI CreateDirectoryEx32W( LPCWSTR template, LPCWSTR path,
                                    LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectory32W(path,lpsecattribs);
}


/***********************************************************************
 *           RemoveDirectory16   (KERNEL)
 */
BOOL16 WINAPI RemoveDirectory16( LPCSTR path )
{
    return (BOOL16)RemoveDirectory32A( path );
}


/***********************************************************************
 *           RemoveDirectory32A   (KERNEL32.437)
 */
BOOL32 WINAPI RemoveDirectory32A( LPCSTR path )
{
    DOS_FULL_NAME full_name;

    TRACE(file, "'%s'\n", path );

    if (DOSFS_IsDevice( path ))
    {
        TRACE(file, "cannot remove device '%s'!\n", path);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
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
 *           RemoveDirectory32W   (KERNEL32.438)
 */
BOOL32 WINAPI RemoveDirectory32W( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = RemoveDirectory32A( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           DIR_TryPath
 *
 * Helper function for DIR_SearchPath.
 */
static BOOL32 DIR_TryPath( const DOS_FULL_NAME *dir, LPCSTR name,
                           DOS_FULL_NAME *full_name )
{
    LPSTR p_l = full_name->long_name + strlen(dir->long_name) + 1;
    LPSTR p_s = full_name->short_name + strlen(dir->short_name) + 1;

    if ((p_s >= full_name->short_name + sizeof(full_name->short_name) - 14) ||
        (p_l >= full_name->long_name + sizeof(full_name->long_name) - 1))
    {
        DOS_ERROR( ER_PathNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!DOSFS_FindUnixName( dir->long_name, name, p_l,
                   sizeof(full_name->long_name) - (p_l - full_name->long_name),
                   p_s, DRIVE_GetFlags( dir->drive ) ))
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
static BOOL32 DIR_TryEnvironmentPath( LPCSTR name, DOS_FULL_NAME *full_name )
{
    LPSTR path, next, buffer;
    BOOL32 ret = FALSE;
    INT32 len = strlen(name);
    DWORD size = GetEnvironmentVariable32A( "PATH", NULL, 0 );

    if (!size) return FALSE;
    if (!(path = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
    if (!GetEnvironmentVariable32A( "PATH", path, size )) goto done;
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
static BOOL32 DIR_TryModulePath( LPCSTR name, DOS_FULL_NAME *full_name )
{
    /* FIXME: for now, GetModuleFileName32A can't return more */
    /* than OFS_MAXPATHNAME. This may change with Win32. */
    char buffer[OFS_MAXPATHNAME];
    LPSTR p;

    if (!GetCurrentTask()) return FALSE;
    GetModuleFileName32A( GetCurrentTask(), buffer, sizeof(buffer) );
    if (!(p = strrchr( buffer, '\\' ))) return FALSE;
    if (sizeof(buffer) - (++p - buffer) <= strlen(name)) return FALSE;
    strcpy( p, name );
    return DOSFS_GetFullName( buffer, TRUE, full_name );
}


/***********************************************************************
 *           DIR_SearchPath
 *
 * Implementation of SearchPath32A. 'win32' specifies whether the search
 * order is Win16 (module path last) or Win32 (module path first).
 *
 * FIXME: should return long path names.
 */
DWORD DIR_SearchPath( LPCSTR path, LPCSTR name, LPCSTR ext,
                      DOS_FULL_NAME *full_name, BOOL32 win32 )
{
    DWORD len;
    LPCSTR p;
    LPSTR tmp = NULL;
    BOOL32 ret = TRUE;

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

    /* Try the Windows directory */

    if (DIR_TryPath( &DIR_Windows, name, full_name ))
        goto done;

    /* Try the Windows system directory */

    if (DIR_TryPath( &DIR_System, name, full_name ))
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
 *           SearchPath32A   (KERNEL32.447)
 */
DWORD WINAPI SearchPath32A( LPCSTR path, LPCSTR name, LPCSTR ext, DWORD buflen,
                            LPSTR buffer, LPSTR *lastpart )
{
    LPSTR p, res;
    DOS_FULL_NAME full_name;

    if (!DIR_SearchPath( path, name, ext, &full_name, TRUE )) return 0;
    lstrcpyn32A( buffer, full_name.short_name, buflen );
    res = full_name.long_name +
              strlen(DRIVE_GetRoot( full_name.short_name[0] - 'A' ));
    while (*res == '/') res++;
    if (buflen)
    {
        if (buflen > 3) lstrcpyn32A( buffer + 3, res, buflen - 3 );
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart) *lastpart = strrchr( buffer, '\\' ) + 1;
    }
    return *res ? strlen(res) + 2 : 3;
}


/***********************************************************************
 *           SearchPath32W   (KERNEL32.448)
 */
DWORD WINAPI SearchPath32W( LPCWSTR path, LPCWSTR name, LPCWSTR ext,
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
    return *res ? strlen(res) + 2 : 3;
}


