/*
 * DOS directories functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "windows.h"
#include "winerror.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_PATH_ELEMENTS 20

static char *DIR_WindowsDosDir;
static char *DIR_WindowsUnixDir;
static char *DIR_SystemDosDir;
static char *DIR_SystemUnixDir;
static char *DIR_TempDosDir;
static char *DIR_TempUnixDir;

static char *DIR_DosPath[MAX_PATH_ELEMENTS];  /* Path in DOS format */
static char *DIR_UnixPath[MAX_PATH_ELEMENTS]; /* Path in Unix format */
static int DIR_PathElements = 0;

/***********************************************************************
 *           DIR_GetPath
 *
 * Get a path name from the wine.ini file and make sure it is valid.
 */
static int DIR_GetPath( const char *keyname, const char *defval,
                        char **dos_path, char **unix_path )
{
    char path[MAX_PATHNAME_LEN];
    const char *dos_name ,*unix_name;
    BY_HANDLE_FILE_INFORMATION info;

    PROFILE_GetWineIniString( "wine", keyname, defval, path, sizeof(path) );
    if (!(unix_name = DOSFS_GetUnixFileName( path, TRUE )) ||
        !FILE_Stat( unix_name, &info ) ||
        !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        fprintf(stderr, "Invalid path '%s' for %s directory\n", path, keyname);
        return 0;
    }
    if (!(dos_name = DOSFS_GetDosTrueName( unix_name, TRUE )))
    {
        fprintf( stderr, "Could not get DOS name for %s directory '%s'\n",
                 keyname, unix_name );
        return 0;
    }
    *unix_path = HEAP_strdupA( SystemHeap, 0, unix_name );
    *dos_path  = HEAP_strdupA( SystemHeap, 0, dos_name );
    return 1;
}


/***********************************************************************
 *           DIR_ParseWindowsPath
 */
void DIR_ParseWindowsPath( char *path )
{
    char *p;
    const char *dos_name ,*unix_name;
    BY_HANDLE_FILE_INFORMATION info;
    int i;

    for ( ; path && *path; path = p)
    {
        p = strchr( path, ';' );
        if (p) while (*p == ';') *p++ = '\0';

        if (DIR_PathElements >= MAX_PATH_ELEMENTS)
        {
            fprintf( stderr, "Warning: path has more than %d elements.\n",
                     MAX_PATH_ELEMENTS );
            break;
        }
        if (!(unix_name = DOSFS_GetUnixFileName( path, TRUE )) ||
            !FILE_Stat( unix_name, &info ) ||
            !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            fprintf(stderr,"Warning: invalid dir '%s' in path, deleting it.\n",
                    path );
            continue;
        }
        if (!(dos_name = DOSFS_GetDosTrueName( unix_name, TRUE )))
        {
            fprintf( stderr, "Warning: could not get DOS name for '%s' in path, deleting it.\n",
                     unix_name );
            continue;
        }
        DIR_UnixPath[DIR_PathElements] = HEAP_strdupA(SystemHeap,0,unix_name);
        DIR_DosPath[DIR_PathElements]  = HEAP_strdupA(SystemHeap,0,dos_name);
        DIR_PathElements++;
    }

    if (debugging_dosfs)
        for (i = 0; i < DIR_PathElements; i++)
        {
            dprintf_dosfs( stddeb, "Path[%d]: %s = %s\n",
                           i, DIR_DosPath[i], DIR_UnixPath[i] );
        }
}


/***********************************************************************
 *           DIR_Init
 */
int DIR_Init(void)
{
    char path[MAX_PATHNAME_LEN], *env_p;
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
        fprintf( stderr, "Warning: could not find DOS drive for cwd %s; starting in windows directory.\n",
                 cwd );
    }
    else
    {
        cwd = DOSFS_GetDosTrueName( path, TRUE );
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, cwd + 2 );
    }

    if (!(DIR_GetPath( "windows", "c:\\windows",
                       &DIR_WindowsDosDir, &DIR_WindowsUnixDir ))) return 0;
    if (!(DIR_GetPath( "system", "c:\\windows\\system",
                       &DIR_SystemDosDir, &DIR_SystemUnixDir ))) return 0;
    if (!(DIR_GetPath( "temp", "c:\\windows",
                       &DIR_TempDosDir, &DIR_TempUnixDir ))) return 0;
    if (-1==access(DIR_TempUnixDir,W_OK)) {
    	if (errno==EACCES)
		fprintf(stderr,"Warning: The Temporary Directory (as specified in wine.conf) is NOT writeable. Please check your configuration.\n");
	else
		fprintf(stderr,"Warning: Access to Temporary Directory failed (%s).\n",strerror(errno));
    }
   
    if (drive == -1)
    {
        drive = DIR_WindowsDosDir[0] - 'A';
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, DIR_WindowsDosDir + 2 );
    }

    PROFILE_GetWineIniString("wine", "path", "c:\\windows;c:\\windows\\system",
                             path, sizeof(path) );
    DIR_ParseWindowsPath( path );

    dprintf_dosfs( stddeb, "WindowsDir = %s\nSystemDir  = %s\n",
                   DIR_WindowsDosDir, DIR_SystemDosDir );
    dprintf_dosfs( stddeb, "TempDir    = %s\nCwd        = %c:\\%s\n",
                   DIR_TempDosDir, 'A' + drive, DRIVE_GetDosCwd( drive ) );

    /* Put the temp and Windows directories into the environment */

    env_p = HEAP_xalloc( SystemHeap, 0, strlen(DIR_TempDosDir) + 6 );
    strcpy( env_p, "TEMP=" );
    strcpy( env_p + 5, DIR_TempDosDir );
    putenv( env_p );
    env_p = HEAP_xalloc( SystemHeap, 0, strlen(DIR_WindowsDosDir) + 8 );
    strcpy( env_p, "windir=" );
    strcpy( env_p + 7, DIR_WindowsDosDir );
    putenv( env_p );

    return 1;
}


/***********************************************************************
 *           GetTempPath32A   (KERNEL32.292)
 */
UINT32 GetTempPath32A( UINT32 count, LPSTR path )
{
    if (path) lstrcpyn32A( path, DIR_TempDosDir, count );
    return strlen( DIR_TempDosDir );
}


/***********************************************************************
 *           GetTempPath32W   (KERNEL32.293)
 */
UINT32 GetTempPath32W( UINT32 count, LPWSTR path )
{
    if (path) lstrcpynAtoW( path, DIR_TempDosDir, count );
    return strlen( DIR_TempDosDir );
}


/***********************************************************************
 *           DIR_GetTempUnixDir
 */
UINT32 DIR_GetTempUnixDir( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_TempUnixDir, count );
    return strlen( DIR_TempUnixDir );
}


/***********************************************************************
 *           DIR_GetWindowsUnixDir
 */
UINT32 DIR_GetWindowsUnixDir( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_WindowsUnixDir, count );
    return strlen( DIR_WindowsUnixDir );
}


/***********************************************************************
 *           DIR_GetSystemUnixDir
 */
UINT32 DIR_GetSystemUnixDir( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_SystemUnixDir, count );
    return strlen( DIR_SystemUnixDir );
}


/***********************************************************************
 *           DIR_GetDosPath
 */
UINT32 DIR_GetDosPath( INT32 element, LPSTR path, UINT32 count )
{
    if ((element < 0) || (element >= DIR_PathElements)) return 0;
    if (path) lstrcpyn32A( path, DIR_DosPath[element], count );
    return strlen( DIR_DosPath[element] );
}


/***********************************************************************
 *           GetTempDrive   (KERNEL.92)
 */
BYTE GetTempDrive( BYTE ignored )
{
    /* FIXME: apparently Windows does something with the ignored byte */
    return DIR_TempDosDir[0];
}


UINT32 WIN16_GetTempDrive( BYTE ignored )
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
UINT16 GetWindowsDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetWindowsDirectory32A( path, count );
}


/***********************************************************************
 *           GetWindowsDirectory32A   (KERNEL32.311)
 */
UINT32 GetWindowsDirectory32A( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_WindowsDosDir, count );
    return strlen( DIR_WindowsDosDir );
}


/***********************************************************************
 *           GetWindowsDirectory32W   (KERNEL32.312)
 */
UINT32 GetWindowsDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_WindowsDosDir, count );
    return strlen( DIR_WindowsDosDir );
}


/***********************************************************************
 *           GetSystemDirectory16   (KERNEL.135)
 */
UINT16 GetSystemDirectory16( LPSTR path, UINT16 count )
{
    return (UINT16)GetSystemDirectory32A( path, count );
}


/***********************************************************************
 *           GetSystemDirectory32A   (KERNEL32.282)
 */
UINT32 GetSystemDirectory32A( LPSTR path, UINT32 count )
{
    if (path) lstrcpyn32A( path, DIR_SystemDosDir, count );
    return strlen( DIR_SystemDosDir );
}


/***********************************************************************
 *           GetSystemDirectory32W   (KERNEL32.283)
 */
UINT32 GetSystemDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_SystemDosDir, count );
    return strlen( DIR_SystemDosDir );
}


/***********************************************************************
 *           CreateDirectory16   (KERNEL.144)
 */
BOOL16 CreateDirectory16( LPCSTR path, LPVOID dummy )
{
    dprintf_file( stddeb,"CreateDirectory16(%s,%p)\n", path, dummy );
    return (BOOL16)CreateDirectory32A( path, NULL );
}


/***********************************************************************
 *           CreateDirectory32A   (KERNEL32.39)
 */
BOOL32 CreateDirectory32A( LPCSTR path, LPSECURITY_ATTRIBUTES lpsecattribs )
{
    const char *unixName;

    dprintf_file( stddeb, "CreateDirectory32A(%s,%p)\n", path, lpsecattribs );
    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "CreateDirectory: device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!(unixName = DOSFS_GetUnixFileName( path, FALSE ))) return 0;
    if ((mkdir( unixName, 0777 ) == -1) && (errno != EEXIST))
    {
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           CreateDirectory32W   (KERNEL32.42)
 */
BOOL32 CreateDirectory32W( LPCWSTR path, LPSECURITY_ATTRIBUTES lpsecattribs )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = CreateDirectory32A( xpath, lpsecattribs );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           CreateDirectoryEx32A   (KERNEL32.40)
 */
BOOL32 CreateDirectoryEx32A( LPCSTR template, LPCSTR path,
                             LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectory32A(path,lpsecattribs);
}


/***********************************************************************
 *           CreateDirectoryEx32W   (KERNEL32.41)
 */
BOOL32 CreateDirectoryEx32W( LPCWSTR template, LPCWSTR path,
                             LPSECURITY_ATTRIBUTES lpsecattribs)
{
    return CreateDirectory32W(path,lpsecattribs);
}


/***********************************************************************
 *           RemoveDirectory16   (KERNEL)
 */
BOOL16 RemoveDirectory16( LPCSTR path )
{
    return (BOOL16)RemoveDirectory32A( path );
}


/***********************************************************************
 *           RemoveDirectory32A   (KERNEL32.437)
 */
BOOL32 RemoveDirectory32A( LPCSTR path )
{
    const char *unixName;

    dprintf_file(stddeb, "RemoveDirectory: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "RemoveDirectory: device '%s'!\n", unixName);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return FALSE;
    if (rmdir( unixName ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           RemoveDirectory32W   (KERNEL32.438)
 */
BOOL32 RemoveDirectory32W( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = RemoveDirectory32A( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
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
                      DWORD buflen, LPSTR buffer, LPSTR *lastpart,
                      BOOL32 win32 )
{
    DWORD len;
    LPSTR tmp;
    int i;

    /* First check the supplied parameters */

    if (strchr( name, '.' )) ext = NULL;  /* Ignore the specified extension */
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
    }
    else tmp = (LPSTR)name;
    
    /* If we have an explicit path, everything's easy */

    if (path || (*tmp && (tmp[1] == ':')) ||
        strchr( tmp, '/' ) || strchr( tmp, '\\' ))
    {
        if (!DOSFS_GetUnixFileName( tmp, TRUE )) goto not_found;
        lstrcpyn32A( buffer, tmp, buflen );
        if (tmp != name) HeapFree( GetProcessHeap(), 0, tmp );
        return len;
    }

    /* Try the path of the current executable (for Win32 search order) */

    if (win32 && GetCurrentTask())
    {
        LPSTR p;
        GetModuleFileName32A( GetCurrentTask(), buffer, buflen );
        if ((p = strrchr( buffer, '\\' )))
        {
            lstrcpyn32A( p + 1, tmp, (INT32)buflen - (p - buffer) );
            if (DOSFS_GetUnixFileName( buffer, TRUE ))
            {
                *p = '\0';
                goto found;
            }
        }
    }

    /* Try the current directory */

    if (DOSFS_GetUnixFileName( tmp, TRUE ))
    {
        GetCurrentDirectory32A( buflen, buffer );
        goto found;
    }

    /* Try the Windows directory */

    if (DOSFS_FindUnixName( DIR_WindowsUnixDir, name, NULL, 0,
                            DRIVE_GetFlags( DIR_WindowsDosDir[0] - 'A' ) ))
    {
        lstrcpyn32A( buffer, DIR_WindowsDosDir, buflen );
        goto found;
    }

    /* Try the Windows system directory */

    if (DOSFS_FindUnixName( DIR_SystemUnixDir, name, NULL, 0,
                            DRIVE_GetFlags( DIR_SystemDosDir[0] - 'A' ) ))
    {
        lstrcpyn32A( buffer, DIR_SystemDosDir, buflen );
        goto found;
    }

    /* Try the path of the current executable (for Win16 search order) */

    if (!win32 && GetCurrentTask())
    {
        LPSTR p;
        GetModuleFileName32A( GetCurrentTask(), buffer, buflen );
        if ((p = strrchr( buffer, '\\' )))
        {
            lstrcpyn32A( p + 1, tmp, (INT32)buflen - (p - buffer) );
            if (DOSFS_GetUnixFileName( buffer, TRUE ))
            {
                *p = '\0';
                goto found;
            }
        }
    }
    /* Try all directories in path */

    for (i = 0; i < DIR_PathElements; i++)
    {
        if (DOSFS_FindUnixName( DIR_UnixPath[i], name, NULL, 0,
                                DRIVE_GetFlags( DIR_DosPath[i][0] - 'A' ) ))
        {
            lstrcpyn32A( buffer, DIR_DosPath[i], buflen );
            goto found;
        }
    }

not_found:
    if (tmp != name) HeapFree( GetProcessHeap(), 0, tmp );
    SetLastError( ERROR_FILE_NOT_FOUND );
    DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
    return 0;

found:
    len = strlen(buffer);
    if (lastpart) *lastpart = buffer + len + 1;
    if (len + 1 < buflen)
    {
        buffer += len;
        *buffer++ = '\\';
        buflen -= len + 1;
        lstrcpyn32A( buffer, tmp, buflen );
    }
    len += strlen(tmp) + 1;
    if (tmp != name) HeapFree( GetProcessHeap(), 0, tmp );
    return len;
}


/***********************************************************************
 *           SearchPath32A   (KERNEL32.447)
 */
DWORD SearchPath32A( LPCSTR path, LPCSTR name, LPCSTR ext, DWORD buflen,
                     LPSTR buffer, LPSTR *lastpart )
{
    return DIR_SearchPath( path, name, ext, buflen, buffer, lastpart, TRUE );
}


/***********************************************************************
 *           SearchPath32W   (KERNEL32.448)
 */
DWORD SearchPath32W( LPCWSTR path, LPCWSTR name, LPCWSTR ext, DWORD buflen,
                     LPWSTR buffer, LPWSTR *lastpart )
{
    LPSTR pathA = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    LPSTR extA  = HEAP_strdupWtoA( GetProcessHeap(), 0, ext );
    LPSTR lastpartA;
    LPSTR bufferA = HeapAlloc( GetProcessHeap(), 0, buflen + 1 );

    DWORD ret = DIR_SearchPath( pathA, nameA, extA, buflen, bufferA,
                                &lastpartA, TRUE );
    lstrcpyAtoW( buffer, bufferA );
    if (lastpart)
    {
        if (lastpartA) *lastpart = buffer + (lastpartA - bufferA);
        else *lastpart = NULL;
    }
    HeapFree( GetProcessHeap(), 0, bufferA );
    HeapFree( GetProcessHeap(), 0, extA );
    HeapFree( GetProcessHeap(), 0, nameA );
    HeapFree( GetProcessHeap(), 0, pathA );
    return ret;
}


