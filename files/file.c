/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

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
#include "xmalloc.h"

#define MAX_OPEN_FILES 64  /* Max. open files for all tasks; must be <255 */

typedef struct tagDOS_FILE
{
    struct tagDOS_FILE *next;
    int                 count;        /* Usage count (0 if free) */
    int                 unix_handle;
    int                 mode;
    char               *unix_name;
    WORD                filedate;
    WORD                filetime;
} DOS_FILE;

/* Global files array */
static DOS_FILE DOSFiles[MAX_OPEN_FILES];

static DOS_FILE *FILE_First = DOSFiles;
static DOS_FILE *FILE_LastUsed = DOSFiles;

/* Small file handles array for boot-up, before the first PDB is created */
#define MAX_BOOT_HANDLES  4
static BYTE bootFileHandles[MAX_BOOT_HANDLES] = { 0xff, 0xff, 0xff, 0xff };

/***********************************************************************
 *           FILE_Alloc
 *
 * Allocate a DOS file.
 */
static DOS_FILE *FILE_Alloc(void)
{
    DOS_FILE *file = FILE_First;
    if (file) FILE_First = file->next;
    else if (FILE_LastUsed >= &DOSFiles[MAX_OPEN_FILES-1])
    {
        DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError, SA_Abort, EL_Disk );
        return NULL;
    }
    else file = ++FILE_LastUsed;
    file->count = 1;
    file->unix_handle = -1;
    file->unix_name = NULL;
    return file;
}


/***********************************************************************
 *           FILE_Close
 *
 * Close a DOS file.
 */
static BOOL FILE_Close( DOS_FILE *file )
{
    if (!file->count) return FALSE;
    if (--file->count > 0) return TRUE;
    /* Now really close the file */
    if (file->unix_handle != -1) close( file->unix_handle );
    if (file->unix_name) free( file->unix_name );
    file->next = FILE_First;
    FILE_First = file;
    return TRUE;
}


/***********************************************************************
 *           FILE_GetPDBFiles
 *
 * Return a pointer to the current PDB files array.
 */
static void FILE_GetPDBFiles( BYTE **files, WORD *nbFiles )
{
    PDB *pdb;

    if ((pdb = (PDB *)GlobalLock16( GetCurrentPDB() )) != NULL)
    {
        *files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
        *nbFiles = pdb->nbFiles;
    }
    else
    {
        *files = bootFileHandles;
        *nbFiles = MAX_BOOT_HANDLES;
    }
}


/***********************************************************************
 *           FILE_AllocTaskHandle
 *
 * Allocate a task file handle for a DOS file.
 */
static HFILE FILE_AllocTaskHandle( DOS_FILE *dos_file )
{
    BYTE *files, *fp;
    WORD i, nbFiles;

    FILE_GetPDBFiles( &files, &nbFiles );
    fp = files + 1;  /* Don't use handle 0, as some programs don't like it */
    for (i = nbFiles - 1; (i > 0) && (*fp != 0xff); i--, fp++);
    if (!i)
    {  /* No more handles or files */
        DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError, SA_Abort, EL_Disk );
        return -1;
    }
    *fp = dos_file ? (BYTE)(dos_file - DOSFiles) : 0;
    dprintf_file(stddeb, 
       "FILE_AllocTaskHandle: returning task handle %d, dos_file %d, file %d of %d \n", 
             (fp - files), *fp, nbFiles - i, nbFiles  );
    return (HFILE)(fp - files);
}


/***********************************************************************
 *           FILE_FreeTaskHandle
 *
 * Free a per-task file handle.
 */
static void FILE_FreeTaskHandle( HFILE handle )
{
    BYTE *files;
    WORD nbFiles;

    FILE_GetPDBFiles( &files, &nbFiles );
    dprintf_file( stddeb,"FILE_FreeTaskHandle: dos=%d file=%d\n",
                  handle, files[handle] );
    if ((handle < 0) || (handle >= (INT)nbFiles))
    {
        fprintf( stderr, "FILE_FreeTaskHandle: invalid file handle %d\n",
                 handle );
        return;
    }
    files[handle] = 0xff;
}


/***********************************************************************
 *           FILE_SetTaskHandle
 *
 * Set the value of a task handle (no error checking).
 */
static void FILE_SetTaskHandle( HFILE handle, DOS_FILE *file )
{
    BYTE *files;
    WORD nbFiles;

    FILE_GetPDBFiles( &files, &nbFiles );
    files[handle] = (BYTE)(file - DOSFiles);
}


/***********************************************************************
 *           FILE_GetFile
 *
 * Return the DOS file associated to a task file handle.
 */
static DOS_FILE *FILE_GetFile( HFILE handle )
{
    BYTE *files;
    WORD nbFiles;
    DOS_FILE *file;

    FILE_GetPDBFiles( &files, &nbFiles );
    if ((handle < 0) || (handle >= (INT)nbFiles) ||
        (files[handle] >= MAX_OPEN_FILES) ||
        !(file = &DOSFiles[files[handle]])->count)
    {
        DOS_ERROR( ER_InvalidHandle, EC_ProgramError, SA_Abort, EL_Disk );
        return NULL;
    }
    return file;
}


int FILE_GetUnixHandle( HFILE hFile )
{
    DOS_FILE *file;

    if (!(file = FILE_GetFile( hFile ))) return -1;
    return file->unix_handle;
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
    PDB *pdb = (PDB *)GlobalLock16( hPDB );

    if (!pdb) return;
    files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );
    dprintf_file(stddeb,"FILE_CloseAllFiles: closing %d files\n",pdb->nbFiles);
    for (count = pdb->nbFiles; count > 0; count--, files++)
    {
        if (*files < MAX_OPEN_FILES) FILE_Close( &DOSFiles[*files] );
        *files = 0xff;
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
static DOS_FILE *FILE_OpenUnixFile( const char *name, int mode )
{
    DOS_FILE *file;
    struct stat st;

    if (!(file = FILE_Alloc())) return NULL;
    if ((file->unix_handle = open( name, mode )) == -1)
    {
        if (Options.allowReadOnly && (mode == O_RDWR))
        {
            if ((file->unix_handle = open( name, O_RDONLY )) != -1)
                fprintf( stderr, "Warning: could not open %s for writing, opening read-only.\n", name );
        }
    }
    if ((file->unix_handle == -1) || (fstat( file->unix_handle, &st ) == -1))
    {
        FILE_SetDosError();
        FILE_Close( file );
        return NULL;
    }
    if (S_ISDIR(st.st_mode))
    {
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        FILE_Close( file );
        return NULL;
    }

    /* File opened OK, now fill the DOS_FILE */

    file->unix_name = xstrdup( name );
    DOSFS_ToDosDateTime( st.st_mtime, &file->filedate, &file->filetime );
    return file;
}


/***********************************************************************
 *           FILE_Open
 */
static DOS_FILE *FILE_Open( LPCSTR path, int mode )
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
            return NULL;
        }
    }
    else if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return NULL;
    return FILE_OpenUnixFile( unixName, mode );
}


/***********************************************************************
 *           FILE_Create
 */
static DOS_FILE *FILE_Create( LPCSTR path, int mode, int unique )
{
    DOS_FILE *file;
    const char *unixName;

    dprintf_file(stddeb, "FILE_Create: '%s' %04x %d\n", path, mode, unique );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_Create: creating device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_NotFound, SA_Abort, EL_Disk );
        return NULL;
    }

    if (!(file = FILE_Alloc())) return NULL;

    if (!(unixName = DOSFS_GetUnixFileName( path, FALSE )))
    {
        FILE_Close( file );
        return NULL;
    }
    if ((file->unix_handle = open( unixName,
                           O_CREAT | O_TRUNC | O_RDWR | (unique ? O_EXCL : 0),
                           mode )) == -1)
    {
        FILE_SetDosError();
        FILE_Close( file );
        return NULL;
    } 

    /* File created OK, now fill the DOS_FILE */

    file->unix_name = xstrdup( unixName );
    DOSFS_ToDosDateTime( time(NULL), &file->filedate, &file->filetime );
    return file;
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
    DOSFS_ToDosDateTime( st.st_mtime, pdate, ptime );
    return 1;
}


/***********************************************************************
 *           FILE_GetDateTime
 *
 * Get the date and time of a file.
 */
int FILE_GetDateTime( HFILE hFile, WORD *pdate, WORD *ptime, BOOL refresh )
{
    DOS_FILE *file;

    if (!(file = FILE_GetFile( hFile ))) return 0;
    if (refresh)
    {
        struct stat st;
        if (fstat( file->unix_handle, &st ) == -1)
        {
            FILE_SetDosError();
            return 0;
        }
        DOSFS_ToDosDateTime( st.st_mtime, &file->filedate, &file->filetime );
    }
    *pdate = file->filedate;
    *ptime = file->filetime;
    return 1;
}


/***********************************************************************
 *           FILE_SetDateTime
 *
 * Set the date and time of a file.
 */
int FILE_SetDateTime( HFILE hFile, WORD date, WORD time )
{
    DOS_FILE *file;
    struct tm newtm;
    struct utimbuf filetime;

    if (!(file = FILE_GetFile( hFile ))) return 0;
    newtm.tm_sec  = (time & 0x1f) * 2;
    newtm.tm_min  = (time >> 5) & 0x3f;
    newtm.tm_hour = (time >> 11);
    newtm.tm_mday = (date & 0x1f);
    newtm.tm_mon  = ((date >> 5) & 0x0f) - 1;
    newtm.tm_year = (date >> 9) + 80;

    filetime.actime = filetime.modtime = mktime( &newtm );
    if (utime( file->unix_name, &filetime ) != -1) return 1;
    FILE_SetDosError();
    return 0;
}


/***********************************************************************
 *           FILE_Sync
 */
int FILE_Sync( HFILE hFile )
{
    DOS_FILE *file;

    if (!(file = FILE_GetFile( hFile ))) return 0;
    if (fsync( file->unix_handle ) != -1) return 1;
    FILE_SetDosError();
    return 0;
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
    DOS_FILE *file;
    HFILE handle;

    dprintf_file( stddeb, "FILE_Dup for handle %d\n", hFile );
    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( file )) != HFILE_ERROR) file->count++;
    dprintf_file( stddeb, "FILE_Dup return handle %d\n", handle );
    return handle;
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE FILE_Dup2( HFILE hFile1, HFILE hFile2 )
{
    DOS_FILE *file;
    PDB *pdb = (PDB *)GlobalLock16( GetCurrentPDB() );
    BYTE *files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );

    dprintf_file( stddeb, "FILE_Dup2 for handle %d\n", hFile1 );
    if (!(file = FILE_GetFile( hFile1 ))) return HFILE_ERROR;

    if ((hFile2 < 0) || (hFile2 >= (INT)pdb->nbFiles))
    {
        DOS_ERROR( ER_InvalidHandle, EC_ProgramError, SA_Abort, EL_Disk );
        return HFILE_ERROR;
    }
    if (files[hFile2] < MAX_OPEN_FILES)
    {
        dprintf_file( stddeb, "FILE_Dup2 closing old handle2 %d\n",
                      files[hFile2] );
        FILE_Close( &DOSFiles[files[hFile2]] );
    }
    files[hFile2] = (BYTE)(file - DOSFiles);
    file->count++;
    dprintf_file( stddeb, "FILE_Dup2 return handle2 %d\n", hFile2 );
    return hFile2;
}


/***********************************************************************
 *           FILE_Read
 */
INT32 FILE_Read( HFILE hFile, LPVOID buffer, UINT32 count )
{
    DOS_FILE *file;
    INT32 result;

    dprintf_file( stddeb, "FILE_Read: %d %p %d\n", hFile, buffer, count );
    if (!(file = FILE_GetFile( hFile ))) return -1;
    if (!count) return 0;
    if ((result = read( file->unix_handle, buffer, count )) == -1)
        FILE_SetDosError();
    return result;
}


/***********************************************************************
 *           GetTempFileName   (KERNEL.97)
 */
INT GetTempFileName( BYTE drive, LPCSTR prefix, UINT unique, LPSTR buffer )
{
    int i;
    UINT num = unique ? (unique & 0xffff) : time(NULL) & 0xffff;
    char *p;

    if ((drive & TF_FORCEDRIVE) &&
        !DRIVE_IsValid( toupper(drive & ~TF_FORCEDRIVE) - 'A' ))
    {
        drive &= ~TF_FORCEDRIVE;
        fprintf( stderr, "Warning: GetTempFileName: invalid drive %d specified\n",
                 drive );
    }

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
        lstrcpyn32A( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
        dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
        return unique;
    }

    /* Now try to create it */

    do
    {
        DOS_FILE *file;
        if ((file = FILE_Create( buffer, 0666, TRUE )) != NULL)
        {  /* We created it */
            dprintf_file( stddeb, "GetTempFileName: created %s\n", buffer );
            FILE_Close( file );
            break;
        }
        if (DOS_ExtendedError != ER_FileExists) break;  /* No need to go on */
        num++;
        sprintf( p, "%04x.tmp", num );
    } while (num != (unique & 0xffff));

    lstrcpyn32A( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
    dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
    return num;
}


/***********************************************************************
 *           OpenFile   (KERNEL.74)
 */
HFILE OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    DOS_FILE *file;
    HFILE hFileRet;
    WORD filedatetime[2];
    const char *unixName, *dosName;
    char *p;
    int len, i, unixMode;

    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;
    dprintf_file( stddeb, "OpenFile: %s %04x\n", name, mode );

    /* First allocate a task handle */

    if ((hFileRet = FILE_AllocTaskHandle( NULL )) == HFILE_ERROR)
    {
        ofs->nErrCode = DOS_ExtendedError;
        dprintf_file( stddeb, "OpenFile: no more task handles.\n" );
        return HFILE_ERROR;
    }

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        if (!(dosName = DOSFS_GetDosTrueName( name, FALSE ))) goto error;
        lstrcpyn32A( ofs->szPathName, dosName, sizeof(ofs->szPathName) );
        ofs->fFixedDisk = (GetDriveType( dosName[0]-'A' ) != DRIVE_REMOVABLE);
        dprintf_file( stddeb, "OpenFile(%s): OF_PARSE, res = '%s', %d\n",
                      name, ofs->szPathName, hFileRet );
        /* Return the handle, but close it first */
        FILE_FreeTaskHandle( hFileRet );
/*        return hFileRet; */
        return 0;  /* Progman seems to like this better */
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if (!(file = FILE_Create( name, 0666, FALSE ))) goto error;
        lstrcpyn32A( ofs->szPathName, DOSFS_GetDosTrueName( name, FALSE ),
                     sizeof(ofs->szPathName) );
        goto success;
    }

    /* Now look for the file */

    /* First try the current directory */

    lstrcpyn32A( ofs->szPathName, name, sizeof(ofs->szPathName) );
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
        if (!DIR_GetDosPath( i, ofs->szPathName, len )) goto not_found;
        strcat( ofs->szPathName, "\\" );
        strcat( ofs->szPathName, name );
        if ((unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE)) != NULL)
            break;
    }

found:
    dprintf_file( stddeb, "OpenFile: found '%s'\n", unixName );
    lstrcpyn32A(ofs->szPathName, DOSFS_GetDosTrueName( ofs->szPathName, FALSE),
                sizeof(ofs->szPathName) );

    if (mode & OF_DELETE)
    {
        if (unlink( unixName ) == -1) goto not_found;
        dprintf_file( stddeb, "OpenFile(%s): OF_DELETE return = OK\n", name);
        /* Return the handle, but close it first */
        FILE_FreeTaskHandle( hFileRet );
        return hFileRet;
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

    if (!(file = FILE_OpenUnixFile( unixName, unixMode ))) goto not_found;
    filedatetime[0] = file->filedate;
    filedatetime[1] = file->filetime;
    if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
    {
        if (memcmp( ofs->reserved, filedatetime, sizeof(ofs->reserved) ))
        {
            FILE_Close( file );
            dprintf_file( stddeb, "OpenFile(%s): OF_VERIFY failed\n", name );
            /* FIXME: what error here? */
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            goto error;
        }
    }
    memcpy( ofs->reserved, filedatetime, sizeof(ofs->reserved) );

    if (mode & OF_EXIST)
    {
        FILE_Close( file );
        /* Return the handle, but close it first */
        FILE_FreeTaskHandle( hFileRet );
        return hFileRet;
    }

success:  /* We get here if the open was successful */
    dprintf_file( stddeb, "OpenFile(%s): OK, return = %d\n", name, hFileRet );
    FILE_SetTaskHandle( hFileRet, file );
    return hFileRet;

not_found:  /* We get here if the file does not exist */
    dprintf_file( stddeb, "OpenFile: '%s' not found\n", name );
    DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = DOS_ExtendedError;
    dprintf_file( stddeb, "OpenFile(%s): return = HFILE_ERROR\n", name );
    FILE_FreeTaskHandle( hFileRet );
    return HFILE_ERROR;
}


/***********************************************************************
 *           _lclose   (KERNEL.81)
 */
HFILE _lclose( HFILE hFile )
{
    DOS_FILE *file;

    dprintf_file( stddeb, "_lclose: handle %d\n", hFile );
    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR;
    FILE_Close( file );
    FILE_FreeTaskHandle( hFile );
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
HFILE _lcreat( LPCSTR path, INT attr )
{
    DOS_FILE *file;
    HFILE handle;
    int mode;
    
    dprintf_file( stddeb, "_lcreat: %s %02x\n", path, attr );
    mode = (attr & 1) ? 0444 : 0666;
    if (!(file = FILE_Create( path, mode, FALSE ))) return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( file )) == HFILE_ERROR)
        FILE_Close( file );
    return handle;
}


/***********************************************************************
 *           _lcreat_uniq   (Not a Windows API)
 */
HFILE _lcreat_uniq( LPCSTR path, INT attr )
{
    DOS_FILE *file;
    HFILE handle;
    int mode;
    
    dprintf_file( stddeb, "_lcreat: %s %02x\n", path, attr );
    mode = (attr & 1) ? 0444 : 0666;
    if (!(file = FILE_Create( path, mode, TRUE ))) return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( file )) == HFILE_ERROR)
        FILE_Close( file );
    return handle;
}


/***********************************************************************
 *           _llseek   (KERNEL.84)
 */
LONG _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    DOS_FILE *file;
    int origin, result;

    dprintf_file( stddeb, "_llseek: handle %d, offset %ld, origin %d\n", 
                  hFile, lOffset, nOrigin);

    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR;
    switch(nOrigin)
    {
        case 1:  origin = SEEK_CUR; break;
        case 2:  origin = SEEK_END; break;
        default: origin = SEEK_SET; break;
    }

    if ((result = lseek( file->unix_handle, lOffset, origin )) == -1)
        FILE_SetDosError();
    return result;
}


/***********************************************************************
 *           _lopen   (KERNEL.85)
 */
HFILE _lopen( LPCSTR path, INT mode )
{
    DOS_FILE *file;
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
    if (!(file = FILE_Open( path, unixMode ))) return HFILE_ERROR;
    if ((handle = FILE_AllocTaskHandle( file )) == HFILE_ERROR)
        FILE_Close( file );
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
    DOS_FILE *file;
    LONG result;

    dprintf_file( stddeb, "_hwrite: %d %p %ld\n", hFile, buffer, count );

    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR;
    
    if (count == 0)  /* Expand or truncate at current position */
        result = ftruncate( file->unix_handle,
                            lseek( file->unix_handle, 0, SEEK_CUR ) );
    else
        result = write( file->unix_handle, buffer, count );

    if (result == -1) FILE_SetDosError();
    return result;
}


/***********************************************************************
 *           SetHandleCount   (KERNEL.199)
 */
WORD SetHandleCount( WORD count )
{
    HANDLE hPDB = GetCurrentPDB();
    PDB *pdb = (PDB *)GlobalLock16( hPDB );
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
            GlobalFree32( (HGLOBAL32)pdb->fileHandlesPtr );
            pdb->fileHandlesPtr = (SEGPTR)pdb->fileHandles;
#else
            GlobalFree16( GlobalHandle16( SELECTOROF(pdb->fileHandlesPtr) ));
            pdb->fileHandlesPtr = (SEGPTR)MAKELONG( 0x18,
                                                   GlobalHandleToSel( hPDB ) );
#endif
            pdb->nbFiles = 20;
        }
    }
    else  /* More than 20, need a new file handles table */
    {
        BYTE *newfiles;
#ifdef WINELIB
        newfiles = (BYTE *)GlobalAlloc32( GMEM_FIXED, count );
#else
        HANDLE newhandle = GlobalAlloc16( GMEM_MOVEABLE, count );
        if (!newhandle)
        {
            DOS_ERROR( ER_OutOfMemory, EC_OutOfResource, SA_Abort, EL_Memory );
            return pdb->nbFiles;
        }
        newfiles = (BYTE *)GlobalLock16( newhandle );
#endif  /* WINELIB */
        if (count > pdb->nbFiles)
        {
            memcpy( newfiles, files, pdb->nbFiles );
            memset( newfiles + pdb->nbFiles, 0xff, count - pdb->nbFiles );
        }
        else memcpy( newfiles, files, count );
#ifdef WINELIB
        if (pdb->nbFiles > 20) GlobalFree32( (HGLOBAL32)pdb->fileHandlesPtr );
        pdb->fileHandlesPtr = (SEGPTR)newfiles;
#else
        if (pdb->nbFiles > 20)
            GlobalFree16( GlobalHandle16( SELECTOROF(pdb->fileHandlesPtr) ));
        pdb->fileHandlesPtr = WIN16_GlobalLock16( newhandle );
#endif  /* WINELIB */
        pdb->nbFiles = count;
    }
    return pdb->nbFiles;
}
