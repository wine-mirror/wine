/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "windows.h"
#include "directory.h"
#include "dos_fs.h"
#include "drive.h"
#include "global.h"
#include "msdos.h"
#include "options.h"
#include "ldt.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"

#define MAX_OPEN_FILES 64  /* Max. open files for all tasks; must be <255 */

typedef struct
{
    int unix_handle;
    int mode;
} DOS_FILE;

/* Global files array */
static DOS_FILE DOSFiles[MAX_OPEN_FILES];


/***********************************************************************
 *           FILE_AllocDOSFile
 *
 * Allocate a file from the DOS files array.
 */
static BYTE FILE_AllocDOSFile( int unix_handle )
{
    BYTE i;
    for (i = 0; i < MAX_OPEN_FILES; i++) if (!DOSFiles[i].mode)
    {
        DOSFiles[i].unix_handle = unix_handle;
        DOSFiles[i].mode = 1;
        return i;
    }
    return 0xff;
}


/***********************************************************************
 *           FILE_FreeDOSFile
 *
 * Free a file from the DOS files array.
 */
static BOOL FILE_FreeDOSFile( BYTE handle )
{
    if (handle >= MAX_OPEN_FILES) return FALSE;
    if (!DOSFiles[handle].mode) return FALSE;
    DOSFiles[handle].mode = 0;
    return TRUE;
}


/***********************************************************************
 *           FILE_GetUnixHandle
 *
 * Return the Unix handle for a global DOS file handle.
 */
static int FILE_GetUnixHandle( BYTE handle )
{
    if (handle >= MAX_OPEN_FILES) return -1;
    if (!DOSFiles[handle].mode) return -1;
    return DOSFiles[handle].unix_handle;
}


/***********************************************************************
 *           FILE_AllocTaskHandle
 *
 * Allocate a per-task file handle.
 */
static HFILE FILE_AllocTaskHandle( int unix_handle )
{
    PDB *pdb = (PDB *)GlobalLock( GetCurrentPDB() );
    BYTE *files, *fp;
    WORD i;

    if (!pdb)
    {
        fprintf(stderr,"FILE_MakeTaskHandle: internal error, no current PDB.\n");
        exit(1);
    }
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    fp = files + 1;  /* Don't use handle 0, as some programs don't like it */
    for (i = pdb->nbFiles - 1; (i > 0) && (*fp != 0xff); i--, fp++);
    if (!i || (*fp = FILE_AllocDOSFile( unix_handle )) == 0xff)
    {  /* No more handles or files */
        DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError, SA_Abort, EL_Disk );
        return -1;
    }
    dprintf_file(stddeb, 
       "FILE_AllocTaskHandle: returning task handle %d, file %d for unix handle %d file %d of %d \n", 
             (fp - files), *fp, unix_handle, pdb->nbFiles - i, pdb->nbFiles  );
    return (HFILE)(fp - files);
}


/***********************************************************************
 *           FILE_FreeTaskHandle
 *
 * Free a per-task file handle.
 */
static void FILE_FreeTaskHandle( HFILE handle )
{
    PDB *pdb = (PDB *)GlobalLock( GetCurrentPDB() );
    BYTE *files;
    
    if (!pdb)
    {
        fprintf(stderr,"FILE_FreeTaskHandle: internal error, no current PDB.\n");
        exit(1);
    }
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    dprintf_file( stddeb,"FILE_FreeTaskHandle: dos=%d file=%d\n",
                  handle, files[handle] );
    if ((handle < 0) || (handle >= (INT)pdb->nbFiles) ||
        !FILE_FreeDOSFile( files[handle] ))
    {
        fprintf( stderr, "FILE_FreeTaskHandle: invalid file handle %d\n",
                 handle );
        return;
    }
    files[handle] = 0xff;
}


/***********************************************************************
 *           FILE_GetUnixTaskHandle
 *
 * Return the Unix file handle associated to a task file handle.
 */
int FILE_GetUnixTaskHandle( HFILE handle )
{
    PDB *pdb = (PDB *)GlobalLock( GetCurrentPDB() );
    BYTE *files;
    int unix_handle;

    if (!pdb)
    {
        fprintf(stderr,"FILE_GetUnixHandle: internal error, no current PDB.\n");
        exit(1);
    }
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    if ((handle < 0) || (handle >= (INT)pdb->nbFiles) ||
        ((unix_handle = FILE_GetUnixHandle( files[handle] )) == -1))
    {
        DOS_ERROR( ER_InvalidHandle, EC_ProgramError, SA_Abort, EL_Disk );
        return -1;
    }
    return unix_handle;
}


/***********************************************************************
 *           FILE_CloseAllFiles
 *
 * Close all open files of a given PDB. Used on task termination.
 */
void FILE_CloseAllFiles( HANDLE hPDB )
{
    BYTE *files;
    WORD count;
    PDB *pdb = (PDB *)GlobalLock( hPDB );
    int unix_handle;

    if (!pdb) return;
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    dprintf_file(stddeb,"FILE_CloseAllFiles: closing %d files\n",pdb->nbFiles);
    for (count = pdb->nbFiles; count > 0; count--, files++)
    {
        if (*files != 0xff)
        {
            if ((unix_handle = FILE_GetUnixHandle( *files )) != -1)
            {
                close( unix_handle );
                FILE_FreeDOSFile( *files );
            }
            *files = 0xff;
        }
    }
}


/***********************************************************************
 *           FILE_SetDosError
 *
 * Set the DOS error code from errno.
 */
void FILE_SetDosError(void)
{
    dprintf_file(stddeb, "FILE_SetDosError: errno = %d\n", errno );
    switch (errno)
    {
    case EAGAIN:
        DOS_ERROR( ER_ShareViolation, EC_Temporary, SA_Retry, EL_Disk );
        break;
    case EBADF:
        DOS_ERROR( ER_InvalidHandle, EC_ProgramError, SA_Abort, EL_Disk );
        break;
    case ENOSPC:
        DOS_ERROR( ER_DiskFull, EC_MediaError, SA_Abort, EL_Disk );
        break;
    case EACCES:
    case EPERM:
    case EROFS:
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        break;
    case EBUSY:
        DOS_ERROR( ER_LockViolation, EC_AccessDenied, SA_Abort, EL_Disk );
        break;
    case ENOENT:
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        break;
    case EISDIR:
        DOS_ERROR( ER_CanNotMakeDir, EC_AccessDenied, SA_Abort, EL_Unknown );
        break;
    case ENFILE:
    case EMFILE:
        DOS_ERROR( ER_NoMoreFiles, EC_MediaError, SA_Abort, EL_Unknown );
        break;
    case EEXIST:
        DOS_ERROR( ER_FileExists, EC_Exists, SA_Abort, EL_Disk );
        break;
    default:
        perror( "int21: unknown errno" );
        DOS_ERROR( ER_GeneralFailure, EC_SystemFailure, SA_Abort, EL_Unknown );
        break;
    }
}


/***********************************************************************
 *           FILE_OpenUnixFile
 */
static int FILE_OpenUnixFile( const char *name, int mode )
{
    int handle;
    struct stat st;

    if ((handle = open( name, mode )) == -1)
    {
        if (Options.allowReadOnly && (mode == O_RDWR))
        {
            if ((handle = open( name, O_RDONLY )) != -1)
                fprintf( stderr, "Warning: could not open %s for writing, opening read-only.\n", name );
        }
    }
    if (handle != -1)  /* Make sure it's not a directory */
    {
        if ((fstat( handle, &st ) == -1))
        {
            FILE_SetDosError();
            close( handle );
            handle = -1;
        }
        else if (S_ISDIR(st.st_mode))
        {
            DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
            close( handle );
            handle = -1;
        }
    }
    return handle;
}


/***********************************************************************
 *           FILE_Open
 */
int FILE_Open( LPCSTR path, int mode )
{
    const char *unixName;

    dprintf_file(stddeb, "FILE_Open: '%s' %04x\n", path, mode );
    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file( stddeb, "FILE_Open: opening device '%s'\n", unixName );
        if (!unixName[0])  /* Non-existing device */
        {
            dprintf_file(stddeb, "FILE_Open: Non-existing device\n");
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            return -1;
        }
    }
    else if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return -1;
    return FILE_OpenUnixFile( unixName, mode );
}


/***********************************************************************
 *           FILE_Create
 */
int FILE_Create( LPCSTR path, int mode, int unique )
{
    const char *unixName;
    int handle;

    dprintf_file(stddeb, "FILE_Create: '%s' %04x %d\n", path, mode, unique );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_Create: creating device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_NotFound, SA_Abort, EL_Disk );
        return -1;
    }

    if (!(unixName = DOSFS_GetUnixFileName( path, FALSE ))) return -1;
    if ((handle = open( unixName,
                        O_CREAT | O_TRUNC | O_RDWR | (unique ? O_EXCL : 0),
                        mode )) == -1)
        FILE_SetDosError();
    return handle;
}


/***********************************************************************
 *           FILE_Unlink
 */
int FILE_Unlink( LPCSTR path )
{
    const char *unixName;

    dprintf_file(stddeb, "FILE_Unlink: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_Unlink: removing device '%s'!\n", unixName);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }

    if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return 0;
    if (unlink( unixName ) == -1)
    {
        FILE_SetDosError();
        return 0;
    }
    return 1;
}


/***********************************************************************
 *           FILE_Stat
 *
 * Stat a Unix path name. Return 1 if OK.
 */
int FILE_Stat( LPCSTR unixName, BYTE *pattr, DWORD *psize,
               WORD *pdate, WORD *ptime )
{
    struct stat st;

    if (stat( unixName, &st ) == -1)
    {
        FILE_SetDosError();
        return 0;
    }
    if (pattr) *pattr = FA_ARCHIVE | (S_ISDIR(st.st_mode) ? FA_DIRECTORY : 0);
    if (psize) *psize = S_ISDIR(st.st_mode) ? 0 : st.st_size;
    DOSFS_ToDosDateTime( &st.st_mtime, pdate, ptime );
    return 1;
}


/***********************************************************************
 *           FILE_Fstat
 *
 * Stat a DOS handle. Return 1 if OK.
 */
int FILE_Fstat( HFILE hFile, BYTE *pattr, DWORD *psize,
                WORD *pdate, WORD *ptime )
{
    struct stat st;
    int handle;

    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return 0;
    if (fstat( handle, &st ) == -1)
    {
        FILE_SetDosError();
        return 0;
    }
    if (pattr) *pattr = FA_ARCHIVE | (S_ISDIR(st.st_mode) ? FA_DIRECTORY : 0);
    if (psize) *psize = S_ISDIR(st.st_mode) ? 0 : st.st_size;
    DOSFS_ToDosDateTime( &st.st_mtime, pdate, ptime );
    return 1;
}


/***********************************************************************
 *           FILE_MakeDir
 */
int FILE_MakeDir( LPCSTR path )
{
    const char *unixName;

    dprintf_file(stddeb, "FILE_MakeDir: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_MakeDir: device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        return 0;
    }
    if (!(unixName = DOSFS_GetUnixFileName( path, FALSE ))) return 0;
    if ((mkdir( unixName, 0777 ) == -1) && (errno != EEXIST))
    {
        FILE_SetDosError();
        return 0;
    }
    return 1;
}


/***********************************************************************
 *           FILE_RemoveDir
 */
int FILE_RemoveDir( LPCSTR path )
{
    const char *unixName;

    dprintf_file(stddeb, "FILE_RemoveDir: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_RemoveDir: device '%s'!\n", unixName);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }
    if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return 0;
    if (rmdir( unixName ) == -1)
    {
        FILE_SetDosError();
        return 0;
    }
    return 1;
}


/***********************************************************************
 *           FILE_Dup
 *
 * dup() function for DOS handles.
 */
HFILE FILE_Dup( HFILE hFile )
{
    int handle, newhandle;
    HFILE dosHandle;

    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return HFILE_ERROR;
    dprintf_file( stddeb, "FILE_Dup for handle %d\n",handle);
    if ((newhandle = dup(handle)) == -1)
    {
        FILE_SetDosError();
        return HFILE_ERROR;
    }
    if ((dosHandle = FILE_AllocTaskHandle( newhandle )) == HFILE_ERROR)
        close( newhandle );
    dprintf_file( stddeb, "FILE_Dup return handle %d\n",dosHandle);
    return dosHandle;
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE FILE_Dup2( HFILE hFile1, HFILE hFile2 )
{
    PDB *pdb = (PDB *)GlobalLock( GetCurrentPDB() );
    BYTE *files;
    int handle, newhandle;

    if ((handle = FILE_GetUnixTaskHandle( hFile1 )) == -1) return HFILE_ERROR;
    dprintf_file( stddeb, "FILE_Dup2 for handle %d\n",handle);
    if ((hFile2 < 0) || (hFile2 >= (INT)pdb->nbFiles))
    {
        DOS_ERROR( ER_InvalidHandle, EC_ProgramError, SA_Abort, EL_Disk );
        return HFILE_ERROR;
    }

    if ((newhandle = dup(handle)) == -1)
    {
        FILE_SetDosError();
        return HFILE_ERROR;
    }
    if (newhandle >= 0xff)
    {
        DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError, SA_Abort, EL_Disk );
        close( newhandle );
        return HFILE_ERROR;
    }
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    if (files[hFile2] != 0xff) 
    {
        dprintf_file( stddeb, "FILE_Dup2 closing old  handle2 %d\n",
                      files[hFile2]);
        close( files[hFile2] );
    }
    files[hFile2] = (BYTE)newhandle;
    dprintf_file( stddeb, "FILE_Dup2 return handle2 %d\n",newhandle);
    return hFile2;
}


/***********************************************************************
 *           FILE_OpenFile
 *
 * Implementation of API function OpenFile(). Returns a Unix file handle.
 */
int FILE_OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    const char *unixName, *dosName;
    char *p;
    int handle, len, i, unixMode;
    struct stat st;

    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;
    dprintf_file( stddeb, "FILE_Openfile: %s %04x\n", name, mode );

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        if (!(dosName = DOSFS_GetDosTrueName( name, FALSE )))
        {
            ofs->nErrCode = DOS_ExtendedError;
            dprintf_file( stddeb, "FILE_Openfile: %s  return = -1\n", name);
            return -1;
        }
        lstrcpyn( ofs->szPathName, dosName, sizeof(ofs->szPathName) );
        ofs->fFixedDisk = (GetDriveType( dosName[0]-'A' ) != DRIVE_REMOVABLE);
        dprintf_file( stddeb, "FILE_Openfile: %s  return = 0\n", name);
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if ((unixName = DOSFS_GetUnixFileName( name, FALSE )) == NULL)
        {
            ofs->nErrCode = DOS_ExtendedError;
            dprintf_file( stddeb, "FILE_Openfile: %s  return = -1\n", name);
            return -1;
        }
        dprintf_file( stddeb, "FILE_OpenFile: creating '%s'\n", unixName );
        handle = open( unixName, O_TRUNC | O_RDWR | O_CREAT, 0666 );
        if (handle == -1)
        {
            FILE_SetDosError();
            ofs->nErrCode = DOS_ExtendedError;
            dprintf_file( stddeb, "FILE_Openfile: %s  return = -1\n", name);
            return -1;
        }   
        lstrcpyn( ofs->szPathName, DOSFS_GetDosTrueName( name, FALSE ),
                  sizeof(ofs->szPathName) );
        dprintf_file( stddeb, "FILE_Openfile: %s  return = %d \n", name, handle);
        return handle;
    }

    /* Now look for the file */

    /* First try the current directory */

    lstrcpyn( ofs->szPathName, name, sizeof(ofs->szPathName) );
    if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE )) != NULL)
        goto found;

    /* Now try some different paths if none was specified */

    if ((mode & OF_SEARCH) && !(mode & OF_REOPEN))
    {
        if (name[1] == ':') name += 2;
        if ((p = strrchr( name, '\\' ))) name = p + 1;
        if ((p = strrchr( name, '/' ))) name = p + 1;
        if (!name[0]) goto not_found;
    }
    else
    {
        if ((name[1] == ':') || strchr( name, '/' ) || strchr( name, '\\' ))
            goto not_found;
    }

    if ((len = sizeof(ofs->szPathName) - strlen(name) - 1) < 0) goto not_found;

    /* Try the Windows directory */

    GetWindowsDirectory( ofs->szPathName, len );
    strcat( ofs->szPathName, "\\" );
    strcat( ofs->szPathName, name );
    if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE )) != NULL)
        goto found;

    /* Try the Windows system directory */

    GetSystemDirectory( ofs->szPathName, len );
    strcat( ofs->szPathName, "\\" );
    strcat( ofs->szPathName, name );
    if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE )) != NULL)
        goto found;

    /* Try the path of the current executable */

    if (GetCurrentTask())
    {
        GetModuleFileName( GetCurrentTask(), ofs->szPathName, len );
        if ((p = strrchr( ofs->szPathName, '\\' )))
        {
            strcpy( p + 1, name );
            if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE )))
                goto found;
        }
    }

    /* Try all directories in path */

    for (i = 0; ; i++)
    {
        if (!DIR_GetDosPath( i, ofs->szPathName, len )) break;
        strcat( ofs->szPathName, "\\" );
        strcat( ofs->szPathName, name );
        if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE)) != NULL)
            goto found;
    }

not_found:
    dprintf_file( stddeb, "FILE_OpenFile: '%s' not found\n", name );
    DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
    ofs->nErrCode = ER_FileNotFound;
    dprintf_file( stddeb, "FILE_Openfile: %s  return =-1\n", name);
    return -1;

found:
    dprintf_file( stddeb, "FILE_OpenFile: found '%s'\n", unixName );
    lstrcpyn( ofs->szPathName, DOSFS_GetDosTrueName( ofs->szPathName, FALSE ),
              sizeof(ofs->szPathName) );

    if (mode & OF_DELETE)
    {
        if (unlink( unixName ) == -1) goto not_found;
        dprintf_file( stddeb, "FILE_Openfile: %s  return = 0\n", name);
        return 0;
    }

    switch(mode & 3)
    {
    case OF_WRITE:
        unixMode = O_WRONLY; break;
    case OF_READWRITE:
        unixMode = O_RDWR; break;
    case OF_READ:
    default:
        unixMode = O_RDONLY; break;
    }

    if ((handle = FILE_OpenUnixFile( unixName, unixMode )) == -1)
        goto not_found;

    if (fstat( handle, &st ) != -1)
    {
        if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
        {
            if (memcmp( ofs->reserved, &st.st_mtime, sizeof(ofs->reserved) ))
            {
                dprintf_file( stddeb, "FILE_Openfile: %s  return = -1\n", name);
                close( handle );
                return -1;
            }
        }
        memcpy( ofs->reserved, &st.st_mtime, sizeof(ofs->reserved) );
    }

    if (mode & OF_EXIST)
    {
        close( handle );
        return 0;
    }
    dprintf_file( stddeb, "FILE_Openfile: %s  return = %d\n", name,handle);

    return handle;
}


/***********************************************************************
 *           FILE_Read
 */
LONG FILE_Read( HFILE hFile, LPSTR buffer, LONG count )
{
    int handle;
    LONG result;

    dprintf_file( stddeb, "FILE_Read: %d %p %ld\n", hFile, buffer, count );
    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return -1;
    if (!count) return 0;
    if ((result = read( handle, buffer, count )) == -1) FILE_SetDosError();
    return result;
}


/***********************************************************************
 *           GetTempFileName   (KERNEL.97)
 */
INT GetTempFileName( BYTE drive, LPCSTR prefix, UINT unique, LPSTR buffer )
{
    int i, handle;
    UINT num = unique ? (unique & 0xffff) : time(NULL) & 0xffff;
    char *p;

    if (drive & TF_FORCEDRIVE)
    {
        sprintf( buffer, "%c:", drive & ~TF_FORCEDRIVE );
    }
    else
    {
        DIR_GetTempDosDir( buffer, 132 );  /* buffer must be at least 144 */
        strcat( buffer, "\\" );
    }

    p = buffer + strlen(buffer);
    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;
    sprintf( p, "%04x.tmp", num );

    if (unique)
    {
        lstrcpyn( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
        dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
        return unique;
    }

    /* Now try to create it */

    do
    {
        if ((handle = FILE_Create( buffer, 0666, TRUE )) != -1)
        {  /* We created it */
            dprintf_file( stddeb, "GetTempFileName: created %s\n", buffer );
            close( handle );
            break;
        }
        if (DOS_ExtendedError != ER_FileExists) break;  /* No need to go on */
        num++;
        sprintf( p, "%04x.tmp", num );
    } while (num != (unique & 0xffff));

    lstrcpyn( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
    dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
    return num;
}


/***********************************************************************
 *           OpenFile   (KERNEL.74)
 */
HFILE OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    int unixHandle;
    HFILE handle;

    dprintf_file( stddeb, "OpenFile %s \n",name);
    if ((unixHandle = FILE_OpenFile( name, ofs, mode )) == -1)
        return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( unixHandle )) == HFILE_ERROR)
    {
        ofs->nErrCode = DOS_ExtendedError;
        if (unixHandle) close( unixHandle );
    }
    if (!unixHandle) FILE_FreeTaskHandle( handle );
    return handle;
}


/***********************************************************************
 *           _lclose   (KERNEL.81)
 */
HFILE _lclose( HFILE hFile )
{
    int handle;

    
    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return HFILE_ERROR;
    dprintf_file( stddeb, "_lclose: doshandle %d unixhandle %d\n", hFile,handle );
    if (handle <= 2)
    {
        fprintf( stderr, "_lclose: internal error: closing handle %d\n", handle );
        exit(1);
    }
    FILE_FreeTaskHandle( hFile );
    close( handle );
    return 0;
}


/***********************************************************************
 *           _lread   (KERNEL.82)
 */
INT _lread( HFILE hFile, SEGPTR buffer, WORD count )
{
    return (INT)_hread( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat   (KERNEL.83)
 */
INT _lcreat( LPCSTR path, INT attr )
{
    int unixHandle, mode;
    HFILE handle;
    
    dprintf_file( stddeb, "_lcreat: %s %02x\n", path, attr );
    mode = (attr & 1) ? 0444 : 0666;
    if ((unixHandle = FILE_Create( path, mode, FALSE )) == -1)
        return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( unixHandle )) == HFILE_ERROR)
        close( unixHandle );
    return handle;
}


/***********************************************************************
 *           _lcreat_uniq   (Not a Windows API)
 */
INT _lcreat_uniq( LPCSTR path, INT attr )
{
    int unixHandle, mode;
    HFILE handle;
    
    dprintf_file( stddeb, "_lcreat: %s %02x\n", path, attr );
    mode = (attr & 1) ? 0444 : 0666;
    if ((unixHandle = FILE_Create( path, mode, TRUE )) == -1)
        return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( unixHandle )) == HFILE_ERROR)
        close( unixHandle );
    return handle;
}


/***********************************************************************
 *           _llseek   (KERNEL.84)
 */
LONG _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    int handle, origin, result;

    dprintf_file( stddeb, "_llseek: handle %d, offset %ld, origin %d\n", 
                  hFile, lOffset, nOrigin);

    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return HFILE_ERROR;
    switch(nOrigin)
    {
        case 1:  origin = SEEK_CUR; break;
        case 2:  origin = SEEK_END; break;
        default: origin = SEEK_SET; break;
    }

    if ((result = lseek( handle, lOffset, origin )) == -1) FILE_SetDosError();
    return (result == -1) ? HFILE_ERROR : result;
}


/***********************************************************************
 *           _lopen   (KERNEL.85)
 */
HFILE _lopen( LPCSTR path, INT mode )
{
    int unixHandle;
    int unixMode;
    HFILE handle;

    dprintf_file(stddeb, "_lopen('%s',%04x)\n", path, mode );

    switch(mode & 3)
    {
    case OF_WRITE:
        unixMode = O_WRONLY | O_TRUNC;
        break;
    case OF_READWRITE:
        unixMode = O_RDWR;
        break;
    case OF_READ:
    default:
        unixMode = O_RDONLY;
        break;
    }
    if ((unixHandle = FILE_Open( path, unixMode )) == -1) return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( unixHandle )) == HFILE_ERROR)
        close( unixHandle );
    return handle;
}


/***********************************************************************
 *           _lwrite   (KERNEL.86)
 */
INT _lwrite( HFILE hFile, LPCSTR buffer, WORD count )
{
    return (INT)_hwrite( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _hread   (KERNEL.349)
 */
LONG _hread( HFILE hFile, SEGPTR buffer, LONG count )
{
#ifndef WINELIB
    LONG maxlen;

    dprintf_file( stddeb, "_hread: %d %08lx %ld\n",
                  hFile, (DWORD)buffer, count );

    /* Some programs pass a count larger than the allocated buffer */
    maxlen = GetSelectorLimit( SELECTOROF(buffer) ) - OFFSETOF(buffer) + 1;
    if (count > maxlen) count = maxlen;
#endif
    return FILE_Read( hFile, PTR_SEG_TO_LIN(buffer), count );
}


/***********************************************************************
 *           _hwrite   (KERNEL.350)
 */
LONG _hwrite( HFILE hFile, LPCSTR buffer, LONG count )
{
    int handle;
    LONG result;

    dprintf_file( stddeb, "_hwrite: %d %p %ld\n", hFile, buffer, count );

    if ((handle = FILE_GetUnixTaskHandle( hFile )) == -1) return HFILE_ERROR;
    
    if (count == 0)  /* Expand or truncate at current position */
        result = ftruncate( handle, lseek( handle, 0, SEEK_CUR ) );
    else
        result = write( handle, buffer, count );

    if (result == -1) FILE_SetDosError();
    return (result == -1) ? HFILE_ERROR : result;
}


/***********************************************************************
 *           SetHandleCount   (KERNEL.199)
 */
WORD SetHandleCount( WORD count )
{
    HANDLE hPDB = GetCurrentPDB();
    PDB *pdb = (PDB *)GlobalLock( hPDB );
    BYTE *files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    WORD i;

    dprintf_file( stddeb, "SetHandleCount(%d)\n", count );

    if (count < 20) count = 20;  /* No point in going below 20 */
    else if (count > 254) count = 254;

    /* If shrinking the table, make sure all extra file handles are closed */
    if (count < pdb->nbFiles)
    {
        for (i = count; i < pdb->nbFiles; i++)
            if (files[i] != 0xff)  /* File open */
            {
                DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError,
                           SA_Abort, EL_Disk );
                return pdb->nbFiles;
            }
    }

    if (count == 20)
    {
        if (pdb->nbFiles > 20)
        {
            memcpy( pdb->fileHandles, files, 20 );
#ifdef WINELIB
            GlobalFree( pdb->fileHandlesPtr );
            pdb->fileHandlesPtr = pdb->fileHandles;
#else
            GlobalFree( GlobalHandle( SELECTOROF(pdb->fileHandlesPtr) ));
            pdb->fileHandlesPtr = (SEGPTR)MAKELONG( 0x18,
                                                   GlobalHandleToSel( hPDB ) );
#endif
            pdb->nbFiles = 20;
        }
    }
    else  /* More than 20, need a new file handles table */
    {
        BYTE *newfiles;
        HANDLE newhandle = GlobalAlloc( GMEM_MOVEABLE, count );
        if (!newhandle)
        {
            DOS_ERROR( ER_OutOfMemory, EC_OutOfResource, SA_Abort, EL_Memory );
            return pdb->nbFiles;
        }
        newfiles = (BYTE *)GlobalLock( newhandle );
        if (count > pdb->nbFiles)
        {
            memcpy( newfiles, files, pdb->nbFiles );
            memset( newfiles + pdb->nbFiles, 0xff, count - pdb->nbFiles );
        }
        else memcpy( newfiles, files, count );
#ifdef WINELIB
        if (pdb->nbFiles > 20) GlobalFree( pdb->fileHandlesPtr );
#else
        if (pdb->nbFiles > 20)
            GlobalFree( GlobalHandle( SELECTOROF(pdb->fileHandlesPtr) ));
#endif
        pdb->fileHandlesPtr = WIN16_GlobalLock( newhandle );
        pdb->nbFiles = count;
    }
    return pdb->nbFiles;
}
