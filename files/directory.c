/*
 * DOS directories functions
 *
 * Copyright 1995 Alexandre Julliard
 */

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
    DOS_FULL_NAME full_name;

    BY_HANDLE_FILE_INFORMATION info;

    PROFILE_GetWineIniString( "wine", keyname, defval, path, sizeof(path) );
    if (!DOSFS_GetFullName( path, TRUE, &full_name ) ||
        !FILE_Stat( full_name.long_name, &info ) ||
        !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        fprintf(stderr, "Invalid path '%s' for %s directory\n", path, keyname);
        return 0;
    }
    *unix_path = HEAP_strdupA( SystemHeap, 0, full_name.long_name );
    *dos_path  = HEAP_strdupA( SystemHeap, 0, full_name.short_name );
    return 1;
}


/***********************************************************************
 *           DIR_ParseWindowsPath
 */
void DIR_ParseWindowsPath( char *path )
{
    char *p;
    DOS_FULL_NAME full_name;
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
        if (!DOSFS_GetFullName( path, TRUE, &full_name ) ||
            !FILE_Stat( full_name.long_name, &info ) ||
            !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            fprintf(stderr,"Warning: invalid dir '%s' in path, deleting it.\n",
                    path );
            continue;
        }
        DIR_UnixPath[DIR_PathElements] = HEAP_strdupA( SystemHeap, 0,
                                                       full_name.long_name );
        DIR_DosPath[DIR_PathElements]  = HEAP_strdupA( SystemHeap, 0,
                                                       full_name.short_name );
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
        DRIVE_SetCurrentDrive( drive );
        DRIVE_Chdir( drive, cwd );
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
UINT32 WINAPI GetTempPath32A( UINT32 count, LPSTR path )
{
    if (path) lstrcpyn32A( path, DIR_TempDosDir, count );
    return strlen( DIR_TempDosDir );
}


/***********************************************************************
 *           GetTempPath32W   (KERNEL32.293)
 */
UINT32 WINAPI GetTempPath32W( UINT32 count, LPWSTR path )
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
BYTE WINAPI GetTempDrive( BYTE ignored )
{
    /* FIXME: apparently Windows does something with the ignored byte */
    return DIR_TempDosDir[0];
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
    if (path) lstrcpyn32A( path, DIR_WindowsDosDir, count );
    return strlen( DIR_WindowsDosDir );
}


/***********************************************************************
 *           GetWindowsDirectory32W   (KERNEL32.312)
 */
UINT32 WINAPI GetWindowsDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_WindowsDosDir, count );
    return strlen( DIR_WindowsDosDir );
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
    if (path) lstrcpyn32A( path, DIR_SystemDosDir, count );
    return strlen( DIR_SystemDosDir );
}


/***********************************************************************
 *           GetSystemDirectory32W   (KERNEL32.283)
 */
UINT32 WINAPI GetSystemDirectory32W( LPWSTR path, UINT32 count )
{
    if (path) lstrcpynAtoW( path, DIR_SystemDosDir, count );
    return strlen( DIR_SystemDosDir );
}


/***********************************************************************
 *           CreateDirectory16   (KERNEL.144)
 */
BOOL16 WINAPI CreateDirectory16( LPCSTR path, LPVOID dummy )
{
    dprintf_file( stddeb,"CreateDirectory16(%s,%p)\n", path, dummy );
    return (BOOL16)CreateDirectory32A( path, NULL );
}


/***********************************************************************
 *           CreateDirectory32A   (KERNEL32.39)
 */
BOOL32 WINAPI CreateDirectory32A( LPCSTR path,
                                  LPSECURITY_ATTRIBUTES lpsecattribs )
{
    DOS_FULL_NAME full_name;
    LPCSTR unixName;

    dprintf_file( stddeb, "CreateDirectory32A(%s,%p)\n", path, lpsecattribs );
    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "CreateDirectory: device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!DOSFS_GetFullName( path, FALSE, &full_name )) return 0;
    if ((mkdir( full_name.long_name, 0777 ) == -1) && (errno != EEXIST))
    {
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
    LPCSTR unixName;

    dprintf_file(stddeb, "RemoveDirectory: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "RemoveDirectory: device '%s'!\n", unixName);
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
static BOOL32 DIR_TryPath( LPCSTR unix_dir, LPCSTR dos_dir, LPCSTR name,
                           DOS_FULL_NAME *full_name )
{
    LPSTR p_l = full_name->long_name + strlen(unix_dir) + 1;
    LPSTR p_s = full_name->short_name + strlen(dos_dir) + 1;

    if ((p_s >= full_name->short_name + sizeof(full_name->short_name) - 14) ||
        (p_l >= full_name->long_name + sizeof(full_name->long_name) - 1))
    {
        DOS_ERROR( ER_PathNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return FALSE;
    }
    if (!DOSFS_FindUnixName( unix_dir, name, p_l,
                   sizeof(full_name->long_name) - (p_l - full_name->long_name),
                   p_s, DRIVE_GetFlags( dos_dir[0] - 'A' ) ))
        return FALSE;
    strcpy( full_name->long_name, unix_dir );
    p_l[-1] = '/';
    strcpy( full_name->short_name, dos_dir );
    p_s[-1] = '\\';
    return TRUE;
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
    int i;
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

    if (DIR_TryPath( DIR_WindowsUnixDir, DIR_WindowsDosDir, name, full_name ))
        goto done;

    /* Try the Windows system directory */

    if (DIR_TryPath( DIR_SystemUnixDir, DIR_SystemDosDir, name, full_name ))
        goto done;

    /* Try the path of the current executable (for Win16 search order) */

    if (!win32 && DIR_TryModulePath( name, full_name )) goto done;

    /* Try all directories in path */

    for (i = 0; i < DIR_PathElements; i++)
    {
        if (DIR_TryPath( DIR_UnixPath[i], DIR_DosPath[i], name, full_name ))
            goto done;
    }

    ret = FALSE;
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
        if (buflen > 3) lstrcpynAtoW( buffer + 3, res + 1, buflen - 3 );
        for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
        if (lastpart)
        {
            for (p = *lastpart = buffer; *p; p++)
                if (*p == '\\') *lastpart = p + 1;
        }
    }
    return *res ? strlen(res) + 2 : 3;
}


