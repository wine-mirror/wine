/*
 * DOS directories functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "windows.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "options.h"
#include "xmalloc.h"
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
    BYTE attr;

    PROFILE_GetWineIniString( "wine", keyname, defval, path, sizeof(path) );
    if (!(unix_name = DOSFS_GetUnixFileName( path, TRUE )) ||
        !FILE_Stat( unix_name, &attr, NULL, NULL, NULL ) ||
        !(attr & FA_DIRECTORY))
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
    *unix_path = xstrdup( unix_name );
    *dos_path  = xstrdup( dos_name );
    return 1;
}


/***********************************************************************
 *           DIR_ParseWindowsPath
 */
void DIR_ParseWindowsPath( char *path )
{
    char *p;
    const char *dos_name ,*unix_name;
    BYTE attr;
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
            !FILE_Stat( unix_name, &attr, NULL, NULL, NULL ) ||
            !(attr & FA_DIRECTORY))
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
        DIR_UnixPath[DIR_PathElements] = xstrdup( unix_name );
        DIR_DosPath[DIR_PathElements]  = xstrdup( dos_name );
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

    env_p = (char *)xmalloc( strlen(DIR_TempDosDir) + 6 );
    strcpy( env_p, "TEMP=" );
    strcpy( env_p + 5, DIR_TempDosDir );
    putenv( env_p );
    env_p = (char *)xmalloc( strlen(DIR_WindowsDosDir) + 8 );
    strcpy( env_p, "windir=" );
    strcpy( env_p + 7, DIR_WindowsDosDir );
    putenv( env_p );

    return 1;
}


/***********************************************************************
 *           DIR_GetTempDosDir
 */
UINT DIR_GetTempDosDir( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_TempDosDir, count );
    return strlen( DIR_TempDosDir );
}


/***********************************************************************
 *           DIR_GetTempUnixDir
 */
UINT DIR_GetTempUnixDir( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_TempUnixDir, count );
    return strlen( DIR_TempUnixDir );
}


/***********************************************************************
 *           DIR_GetWindowsUnixDir
 */
UINT DIR_GetWindowsUnixDir( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_WindowsUnixDir, count );
    return strlen( DIR_WindowsUnixDir );
}


/***********************************************************************
 *           DIR_GetSystemUnixDir
 */
UINT DIR_GetSystemUnixDir( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_SystemUnixDir, count );
    return strlen( DIR_SystemUnixDir );
}


/***********************************************************************
 *           DIR_GetDosPath
 */
UINT DIR_GetDosPath( int element, LPSTR path, UINT count )
{
    if ((element < 0) || (element >= DIR_PathElements)) return 0;
    if (path) lstrcpyn( path, DIR_DosPath[element], count );
    return strlen( DIR_DosPath[element] );
}


/***********************************************************************
 *           GetTempDrive   (KERNEL.92)
 */
BYTE GetTempDrive( BYTE ignored )
{
    return DIR_TempDosDir[0];
}


/***********************************************************************
 *           GetWindowsDirectory   (KERNEL.134)
 */
UINT GetWindowsDirectory( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_WindowsDosDir, count );
    return strlen( DIR_WindowsDosDir );
}


/***********************************************************************
 *           GetSystemDirectory   (KERNEL.135)
 */
UINT GetSystemDirectory( LPSTR path, UINT count )
{
    if (path) lstrcpyn( path, DIR_SystemDosDir, count );
    return strlen( DIR_SystemDosDir );
}
