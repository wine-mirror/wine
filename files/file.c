/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 */

#include <assert.h>
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
#include "winerror.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "ldt.h"
#include "process.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"


typedef struct
{
    K32OBJ    header;
    int       unix_handle;
    int       mode;
    char     *unix_name;
    DWORD     type;         /* Type for win32 apps */
} DOS_FILE;


/***********************************************************************
 *           FILE_Alloc
 *
 * Allocate a file.
 */
static HFILE32 FILE_Alloc( DOS_FILE **file )
{
    HFILE32 handle;
    *file = HeapAlloc( SystemHeap, 0, sizeof(DOS_FILE) );
    if (!*file)
    {
        DOS_ERROR( ER_TooManyOpenFiles, EC_ProgramError, SA_Abort, EL_Disk );
        return NULL;
    }
    (*file)->header.type = K32OBJ_FILE;
    (*file)->header.refcount = 0;
    (*file)->unix_handle = -1;
    (*file)->unix_name = NULL;
    (*file)->type = FILE_TYPE_DISK;

    handle = PROCESS_AllocHandle( &(*file)->header, 0 );
    if (handle == INVALID_HANDLE_VALUE32) *file = NULL;
    return handle;
}


/***********************************************************************
 *           FILE_Destroy
 *
 * Destroy a DOS file.
 */
void FILE_Destroy( K32OBJ *ptr )
{
    DOS_FILE *file = (DOS_FILE *)ptr;
    assert( ptr->type == K32OBJ_FILE );

    if (file->unix_handle != -1) close( file->unix_handle );
    if (file->unix_name) HeapFree( SystemHeap, 0, file->unix_name );
    ptr->type = K32OBJ_UNKNOWN;
    HeapFree( SystemHeap, 0, file );
}


/***********************************************************************
 *           FILE_GetFile
 *
 * Return the DOS file associated to a task file handle. FILE_ReleaseFile must
 * be called to release the file.
 */
static DOS_FILE *FILE_GetFile( HFILE32 handle )
{
    return (DOS_FILE *)PROCESS_GetObjPtr( handle, K32OBJ_FILE );
}


/***********************************************************************
 *           FILE_ReleaseFile
 *
 * Release a DOS file obtained with FILE_GetFile.
 */
static void FILE_ReleaseFile( DOS_FILE *file )
{
    K32OBJ_DecCount( &file->header );
}


/***********************************************************************
 *           FILE_GetUnixHandle
 *
 * Return the Unix handle associated to a file handle.
 */
int FILE_GetUnixHandle( HFILE32 hFile )
{
    DOS_FILE *file;
    int ret;

    if (!(file = FILE_GetFile( hFile ))) return -1;
    ret = file->unix_handle;
    FILE_ReleaseFile( file );
    return ret;
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
 *           FILE_DupUnixHandle
 *
 * Duplicate a Unix handle into a task handle.
 */
HFILE32 FILE_DupUnixHandle( int fd )
{
    HFILE32 handle;
    DOS_FILE *file;

    if ((handle = FILE_Alloc( &file )) != INVALID_HANDLE_VALUE32)
    {
        if ((file->unix_handle = dup(fd)) == -1)
        {
            FILE_SetDosError();
            CloseHandle( handle );
            return INVALID_HANDLE_VALUE32;
        }
    }
    return handle;
}


/***********************************************************************
 *           FILE_OpenUnixFile
 */
static HFILE32 FILE_OpenUnixFile( const char *name, int mode )
{
    HFILE32 handle;
    DOS_FILE *file;
    struct stat st;

    if ((handle = FILE_Alloc( &file )) == INVALID_HANDLE_VALUE32)
        return INVALID_HANDLE_VALUE32;

    if ((file->unix_handle = open( name, mode, 0666 )) == -1)
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
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    }
    if (S_ISDIR(st.st_mode))
    {
        DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    }

    /* File opened OK, now fill the DOS_FILE */

    file->unix_name = HEAP_strdupA( SystemHeap, 0, name );
    return handle;
}


/***********************************************************************
 *           FILE_Open
 */
HFILE32 FILE_Open( LPCSTR path, INT32 mode )
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
            return HFILE_ERROR32;
        }
    }
    else /* check for filename, don't check for last entry if creating */
        if (!(unixName = DOSFS_GetUnixFileName( path, !(mode & O_CREAT) )))
            return HFILE_ERROR32;

    return FILE_OpenUnixFile( unixName, mode );
}


/***********************************************************************
 *           FILE_Create
 */
static HFILE32 FILE_Create( LPCSTR path, int mode, int unique )
{
    HFILE32 handle;
    DOS_FILE *file;
    const char *unixName;

    dprintf_file(stddeb, "FILE_Create: '%s' %04x %d\n", path, mode, unique );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "FILE_Create: creating device '%s'!\n", unixName);
        DOS_ERROR( ER_AccessDenied, EC_NotFound, SA_Abort, EL_Disk );
        return INVALID_HANDLE_VALUE32;
    }

    if ((handle = FILE_Alloc( &file )) == INVALID_HANDLE_VALUE32)
        return INVALID_HANDLE_VALUE32;

    if (!(unixName = DOSFS_GetUnixFileName( path, FALSE )))
    {
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    }
    if ((file->unix_handle = open( unixName,
                           O_CREAT | O_TRUNC | O_RDWR | (unique ? O_EXCL : 0),
                           mode )) == -1)
    {
        FILE_SetDosError();
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    } 

    /* File created OK, now fill the DOS_FILE */

    file->unix_name = HEAP_strdupA( SystemHeap, 0, unixName );
    return handle;
}


/***********************************************************************
 *           FILE_FillInfo
 *
 * Fill a file information from a struct stat.
 */
static void FILE_FillInfo( struct stat *st, BY_HANDLE_FILE_INFORMATION *info )
{
    info->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    if (S_ISDIR(st->st_mode))
        info->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

    DOSFS_UnixTimeToFileTime( st->st_mtime, &info->ftCreationTime );
    DOSFS_UnixTimeToFileTime( st->st_mtime, &info->ftLastWriteTime );
    DOSFS_UnixTimeToFileTime( st->st_atime, &info->ftLastAccessTime );

    info->dwVolumeSerialNumber = 0;  /* FIXME */
    info->nFileSizeHigh = 0;
    info->nFileSizeLow  = S_ISDIR(st->st_mode) ? 0 : st->st_size;
    info->nNumberOfLinks = st->st_nlink;
    info->nFileIndexHigh = 0;
    info->nFileIndexLow  = st->st_ino;
}


/***********************************************************************
 *           FILE_Stat
 *
 * Stat a Unix path name. Return TRUE if OK.
 */
BOOL32 FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info )
{
    struct stat st;

    if (stat( unixName, &st ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    FILE_FillInfo( &st, info );
    return TRUE;
}


/***********************************************************************
 *             GetFileInformationByHandle   (KERNEL32.219)
 */
DWORD GetFileInformationByHandle( HFILE32 hFile,
                                  BY_HANDLE_FILE_INFORMATION *info )
{
    DOS_FILE *file;
    DWORD ret = 0;
    struct stat st;

    if (!(file = FILE_GetFile( hFile ))) return 0;
    if (fstat( file->unix_handle, &st ) == -1) FILE_SetDosError();
    else
    {
        FILE_FillInfo( &st, info );
        ret = 1;
    }
    FILE_ReleaseFile( file );
    return ret;
}


/**************************************************************************
 *           GetFileAttributes16   (KERNEL.420)
 */
DWORD GetFileAttributes16( LPCSTR name )
{
    return GetFileAttributes32A( name );
}


/**************************************************************************
 *           GetFileAttributes32A   (KERNEL32.217)
 */
DWORD GetFileAttributes32A( LPCSTR name )
{
    BY_HANDLE_FILE_INFORMATION info;

    if (!FILE_Stat( name, &info )) return -1;
    return info.dwFileAttributes;
}


/**************************************************************************
 *           GetFileAttributes32W   (KERNEL32.218)
 */
DWORD GetFileAttributes32W( LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    DWORD res = GetFileAttributes32A( nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.220)
 */
DWORD GetFileSize( HFILE32 hFile, LPDWORD filesizehigh )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return 0;
    if (filesizehigh) *filesizehigh = info.nFileSizeHigh;
    return info.nFileSizeLow;
}


/***********************************************************************
 *           GetFileTime   (KERNEL32.221)
 */
BOOL32 GetFileTime( HFILE32 hFile, FILETIME *lpCreationTime,
                    FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return FALSE;
    if (lpCreationTime)   *lpCreationTime   = info.ftCreationTime;
    if (lpLastAccessTime) *lpLastAccessTime = info.ftLastAccessTime;
    if (lpLastWriteTime)  *lpLastWriteTime  = info.ftLastWriteTime;
    return TRUE;
}


/***********************************************************************
 *           FILE_Dup
 *
 * dup() function for DOS handles.
 */
HFILE32 FILE_Dup( HFILE32 hFile )
{
    DOS_FILE *file;
    HFILE32 handle;

    dprintf_file( stddeb, "FILE_Dup for handle %d\n", hFile );
    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR32;
    handle = PROCESS_AllocHandle( &file->header, 0 );
    FILE_ReleaseFile( file );
    dprintf_file( stddeb, "FILE_Dup return handle %d\n", handle );
    return handle;
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE32 FILE_Dup2( HFILE32 hFile1, HFILE32 hFile2 )
{
    DOS_FILE *file;

    dprintf_file( stddeb, "FILE_Dup2 for handle %d\n", hFile1 );
    if (!(file = FILE_GetFile( hFile1 ))) return HFILE_ERROR32;
    if (!PROCESS_SetObjPtr( hFile2, &file->header, 0 )) hFile2 = HFILE_ERROR32;
    FILE_ReleaseFile( file );
    return hFile2;
}


/***********************************************************************
 *           GetTempFileName16   (KERNEL.97)
 */
UINT16 GetTempFileName16( BYTE drive, LPCSTR prefix, UINT16 unique,
                          LPSTR buffer )
{
    char temppath[144];

    if ((drive & TF_FORCEDRIVE) &&
        !DRIVE_IsValid( toupper(drive & ~TF_FORCEDRIVE) - 'A' ))
    {
        drive &= ~TF_FORCEDRIVE;
        fprintf( stderr, "Warning: GetTempFileName: invalid drive %d specified\n",
                 drive );
    }

    if (drive & TF_FORCEDRIVE)
        sprintf(temppath,"%c:", drive & ~TF_FORCEDRIVE );
    else
    {
        GetTempPath32A( 132, temppath );
        strcat( temppath, "\\" );
    }
    return (UINT16)GetTempFileName32A( temppath, prefix, unique, buffer );
}


/***********************************************************************
 *           GetTempFileName32A   (KERNEL32.290)
 */
UINT32 GetTempFileName32A( LPCSTR path, LPCSTR prefix, UINT32 unique,
                           LPSTR buffer)
{
    LPSTR p;
    int i;
    UINT32 num = unique ? (unique & 0xffff) : time(NULL) & 0xffff;

    if (!path) return 0;
    strcpy( buffer, path );
    p = buffer + strlen(buffer);
    *p++ = '~';
    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;
    sprintf( p, "%04x.tmp", num );

    if (unique)
    {
        lstrcpyn32A( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
        dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
	if (-1==access(DOSFS_GetUnixFileName(buffer,TRUE),W_OK))
	    fprintf(stderr,"Warning: GetTempFileName returns '%s', which doesn't seem to be writeable. Please check your configuration file if this generates a failure.\n",buffer);
        return unique;
    }

    /* Now try to create it */

    do
    {
        HFILE32 handle;
        if ((handle = FILE_Create(buffer,0666,TRUE)) != INVALID_HANDLE_VALUE32)
        {  /* We created it */
            dprintf_file( stddeb, "GetTempFileName: created %s\n", buffer );
            CloseHandle( handle );
            break;
        }
        if (DOS_ExtendedError != ER_FileExists) break;  /* No need to go on */
        num++;
        sprintf( p, "%04x.tmp", num );
    } while (num != (unique & 0xffff));

    lstrcpyn32A( buffer, DOSFS_GetDosTrueName( buffer, FALSE ), 144 );
    dprintf_file( stddeb, "GetTempFileName: returning %s\n", buffer );
    if (-1==access(DOSFS_GetUnixFileName(buffer,TRUE),W_OK))
	fprintf(stderr,"Warning: GetTempFileName returns '%s', which doesn't seem to be writeable. Please check your configuration file if this generates a failure.\n",buffer);
    return num;
}


/***********************************************************************
 *           GetTempFileName32W   (KERNEL32.291)
 */
UINT32 GetTempFileName32W( LPCWSTR path, LPCWSTR prefix, UINT32 unique,
                           LPWSTR buffer )
{
    LPSTR   patha,prefixa;
    char    buffera[144];
    UINT32  ret;

    if (!path) return 0;
    patha   = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    prefixa = HEAP_strdupWtoA( GetProcessHeap(), 0, prefix );
    ret     = GetTempFileName32A( patha, prefixa, unique, buffera );
    lstrcpyAtoW( buffer, buffera );
    HeapFree( GetProcessHeap(), 0, patha );
    HeapFree( GetProcessHeap(), 0, prefixa );
    return ret;
}


/***********************************************************************
 *           FILE_DoOpenFile
 *
 * Implementation of OpenFile16() and OpenFile32().
 */
static HFILE32 FILE_DoOpenFile( LPCSTR name, OFSTRUCT *ofs, UINT32 mode,
                                BOOL32 win32 )
{
    HFILE32 hFileRet;
    FILETIME filetime;
    WORD filedatetime[2];
    const char *unixName, *dosName;
    char *p;
    int unixMode;

    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;
    dprintf_file( stddeb, "OpenFile: %s %04x\n", name, mode );

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        if (!(dosName = DOSFS_GetDosTrueName( name, FALSE ))) goto error;
        lstrcpyn32A( ofs->szPathName, dosName, sizeof(ofs->szPathName) );
        ofs->fFixedDisk = (GetDriveType16( dosName[0]-'A' ) != DRIVE_REMOVABLE);
        dprintf_file( stddeb, "OpenFile(%s): OF_PARSE, res = '%s'\n",
                      name, ofs->szPathName );
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if ((hFileRet = FILE_Create(name,0666,FALSE))== INVALID_HANDLE_VALUE32)
            goto error;
        lstrcpyn32A( ofs->szPathName, DOSFS_GetDosTrueName( name, FALSE ),
                     sizeof(ofs->szPathName) );
        goto success;
    }

    /* If OF_SEARCH is set, ignore the given path */

    if ((mode & OF_SEARCH) && !(mode & OF_REOPEN))
    {
        /* First try the file name as is */
        if ((unixName = DOSFS_GetUnixFileName( name, TRUE )) != NULL)
        {
            lstrcpyn32A( ofs->szPathName, name, sizeof(ofs->szPathName) );
            goto found;
        }
        /* Now remove the path */
        if (name[0] && (name[1] == ':')) name += 2;
        if ((p = strrchr( name, '\\' ))) name = p + 1;
        if ((p = strrchr( name, '/' ))) name = p + 1;
        if (!name[0]) goto not_found;
    }

    /* Now look for the file */

    if (!DIR_SearchPath( NULL, name, NULL, sizeof(ofs->szPathName),
                         ofs->szPathName, NULL, win32 ))
        goto not_found;
    if (!(unixName = DOSFS_GetUnixFileName( ofs->szPathName, TRUE )))
        goto not_found;

found:
    dprintf_file( stddeb, "OpenFile: found '%s'\n", unixName );
    lstrcpyn32A(ofs->szPathName, DOSFS_GetDosTrueName( ofs->szPathName, FALSE),
                sizeof(ofs->szPathName) );

    if (mode & OF_DELETE)
    {
        if (unlink( unixName ) == -1) goto not_found;
        dprintf_file( stddeb, "OpenFile(%s): OF_DELETE return = OK\n", name);
        return 1;
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

    if ((hFileRet = FILE_OpenUnixFile( unixName, unixMode )) == HFILE_ERROR32)
        goto not_found;
    GetFileTime( hFileRet, NULL, NULL, &filetime );
    FileTimeToDosDateTime( &filetime, &filedatetime[0], &filedatetime[1] );
    if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
    {
        if (memcmp( ofs->reserved, filedatetime, sizeof(ofs->reserved) ))
        {
            CloseHandle( hFileRet );
            dprintf_file( stddeb, "OpenFile(%s): OF_VERIFY failed\n", name );
            /* FIXME: what error here? */
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            goto error;
        }
    }
    memcpy( ofs->reserved, filedatetime, sizeof(ofs->reserved) );

success:  /* We get here if the open was successful */
    dprintf_file( stddeb, "OpenFile(%s): OK, return = %d\n", name, hFileRet );
    if (mode & OF_EXIST) /* Return the handle, but close it first */
        CloseHandle( hFileRet );
    return hFileRet;

not_found:  /* We get here if the file does not exist */
    dprintf_file( stddeb, "OpenFile: '%s' not found\n", name );
    DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = DOS_ExtendedError;
    dprintf_file( stddeb, "OpenFile(%s): return = HFILE_ERROR\n", name );
    return HFILE_ERROR32;
}


/***********************************************************************
 *           OpenFile16   (KERNEL.74)
 */
HFILE16 OpenFile16( LPCSTR name, OFSTRUCT *ofs, UINT16 mode )
{
    return FILE_DoOpenFile( name, ofs, mode, FALSE );
}


/***********************************************************************
 *           OpenFile32   (KERNEL32.396)
 */
HFILE32 OpenFile32( LPCSTR name, OFSTRUCT *ofs, UINT32 mode )
{
    return FILE_DoOpenFile( name, ofs, mode, TRUE );
}


/***********************************************************************
 *           _lclose16   (KERNEL.81)
 */
HFILE16 _lclose16( HFILE16 hFile )
{
    dprintf_file( stddeb, "_lclose: handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR16;
}


/***********************************************************************
 *           _lclose32   (KERNEL32.592)
 */
HFILE32 _lclose32( HFILE32 hFile )
{
    dprintf_file( stddeb, "_lclose: handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR32;
}


/***********************************************************************
 *           WIN16_hread
 */
LONG WIN16_hread( HFILE16 hFile, SEGPTR buffer, LONG count )
{
    LONG maxlen;

    dprintf_file( stddeb, "_hread16: %d %08lx %ld\n",
                  hFile, (DWORD)buffer, count );

    /* Some programs pass a count larger than the allocated buffer */
    maxlen = GetSelectorLimit( SELECTOROF(buffer) ) - OFFSETOF(buffer) + 1;
    if (count > maxlen) count = maxlen;
    return _lread32( hFile, PTR_SEG_TO_LIN(buffer), count );
}


/***********************************************************************
 *           WIN16_lread
 */
UINT16 WIN16_lread( HFILE16 hFile, SEGPTR buffer, UINT16 count )
{
    return (UINT16)WIN16_hread( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lread32   (KERNEL32.596)
 */
UINT32 _lread32( HFILE32 hFile, LPVOID buffer, UINT32 count )
{
    DOS_FILE *file;
    UINT32 result;

    dprintf_file( stddeb, "_lread32: %d %p %d\n", hFile, buffer, count );
    if (!(file = FILE_GetFile( hFile ))) return -1;
    if (!count) result = 0;
    else if ((result = read( file->unix_handle, buffer, count )) == -1)
        FILE_SetDosError();
    FILE_ReleaseFile( file );
    return result;
}


/***********************************************************************
 *           _lread16   (KERNEL.82)
 */
UINT16 _lread16( HFILE16 hFile, LPVOID buffer, UINT16 count )
{
    return (UINT16)_lread32( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat16   (KERNEL.83)
 */
HFILE16 _lcreat16( LPCSTR path, INT16 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    dprintf_file( stddeb, "_lcreat16: %s %02x\n", path, attr );
    return (HFILE16)FILE_Create( path, mode, FALSE );
}


/***********************************************************************
 *           _lcreat32   (KERNEL32.593)
 */
HFILE32 _lcreat32( LPCSTR path, INT32 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    dprintf_file( stddeb, "_lcreat32: %s %02x\n", path, attr );
    return FILE_Create( path, mode, FALSE );
}


/***********************************************************************
 *           _lcreat_uniq   (Not a Windows API)
 */
HFILE32 _lcreat_uniq( LPCSTR path, INT32 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    dprintf_file( stddeb, "_lcreat: %s %02x\n", path, attr );
    return FILE_Create( path, mode, TRUE );
}


/***********************************************************************
 *           _llseek16   (KERNEL.84)
 */
LONG _llseek16( HFILE16 hFile, LONG lOffset, INT16 nOrigin )
{
    return _llseek32( hFile, lOffset, nOrigin );
}


/***********************************************************************
 *           _llseek32   (KERNEL32.594)
 */
LONG _llseek32( HFILE32 hFile, LONG lOffset, INT32 nOrigin )
{
    DOS_FILE *file;
    int origin, result;

    dprintf_file( stddeb, "_llseek: handle %d, offset %ld, origin %d\n", 
                  hFile, lOffset, nOrigin);

    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR32;
    switch(nOrigin)
    {
        case 1:  origin = SEEK_CUR; break;
        case 2:  origin = SEEK_END; break;
        default: origin = SEEK_SET; break;
    }

    if ((result = lseek( file->unix_handle, lOffset, origin )) == -1)
        FILE_SetDosError();
    FILE_ReleaseFile( file );
    return result;
}


/***********************************************************************
 *           _lopen16   (KERNEL.85)
 */
HFILE16 _lopen16( LPCSTR path, INT16 mode )
{
    return _lopen32( path, mode );
}


/***********************************************************************
 *           _lopen32   (KERNEL32.595)
 */
HFILE32 _lopen32( LPCSTR path, INT32 mode )
{
    INT32 unixMode;

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
    return FILE_Open( path, unixMode );
}


/***********************************************************************
 *           _lwrite16   (KERNEL.86)
 */
UINT16 _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite32( hFile, buffer, (LONG)count );
}

/***********************************************************************
 *           _lwrite32   (KERNEL.86)
 */
UINT32 _lwrite32( HFILE32 hFile, LPCSTR buffer, UINT32 count )
{
    return (UINT32)_hwrite32( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _hread16   (KERNEL.349)
 */
LONG _hread16( HFILE16 hFile, LPVOID buffer, LONG count)
{
    return _lread32( hFile, buffer, count );
}


/***********************************************************************
 *           _hread32   (KERNEL32.590)
 */
LONG _hread32( HFILE32 hFile, LPVOID buffer, LONG count)
{
    return _lread32( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite16   (KERNEL.350)
 */
LONG _hwrite16( HFILE16 hFile, LPCSTR buffer, LONG count )
{
    return _hwrite32( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite32   (KERNEL32.591)
 */
LONG _hwrite32( HFILE32 hFile, LPCSTR buffer, LONG count )
{
    DOS_FILE *file;
    LONG result;

    dprintf_file( stddeb, "_hwrite: %d %p %ld\n", hFile, buffer, count );

    if (!(file = FILE_GetFile( hFile ))) return HFILE_ERROR32;
    if (count == 0)  /* Expand or truncate at current position */
        result = ftruncate( file->unix_handle,
                            lseek( file->unix_handle, 0, SEEK_CUR ) );
    else
        result = write( file->unix_handle, buffer, count );

    if (result == -1) FILE_SetDosError();
    FILE_ReleaseFile( file );
    return result;
}


/***********************************************************************
 *           SetHandleCount16   (KERNEL.199)
 */
UINT16 SetHandleCount16( UINT16 count )
{
    HGLOBAL16 hPDB = GetCurrentPDB();
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
        HGLOBAL16 newhandle = GlobalAlloc16( GMEM_MOVEABLE, count );
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


/*************************************************************************
 *           SetHandleCount32   (KERNEL32.494)
 */
UINT32 SetHandleCount32( UINT32 count )
{
    return MIN( 256, count );
}


/***********************************************************************
 *           FlushFileBuffers   (KERNEL32.133)
 */
BOOL32 FlushFileBuffers( HFILE32 hFile )
{
    DOS_FILE *file;
    BOOL32 ret;

    dprintf_file( stddeb, "FlushFileBuffers(%d)\n", hFile );
    if (!(file = FILE_GetFile( hFile ))) return FALSE;
    if (fsync( file->unix_handle ) != -1) ret = TRUE;
    else
    {
        FILE_SetDosError();
        ret = FALSE;
    }
    FILE_ReleaseFile( file );
    return ret;
}


/**************************************************************************
 *           SetEndOfFile   (KERNEL32.483)
 */
BOOL32 SetEndOfFile( HFILE32 hFile )
{
    DOS_FILE *file;
    BOOL32 ret = TRUE;

    dprintf_file( stddeb, "SetEndOfFile(%d)\n", hFile );
    if (!(file = FILE_GetFile( hFile ))) return FALSE;
    if (ftruncate( file->unix_handle,
                   lseek( file->unix_handle, 0, SEEK_CUR ) ))
    {
        FILE_SetDosError();
        ret = FALSE;
    }
    FILE_ReleaseFile( file );
    return ret;
}


/***********************************************************************
 *           DeleteFile16   (KERNEL.146)
 */
BOOL16 DeleteFile16( LPCSTR path )
{
    return DeleteFile32A( path );
}


/***********************************************************************
 *           DeleteFile32A   (KERNEL32.71)
 */
BOOL32 DeleteFile32A( LPCSTR path )
{
    const char *unixName;

    dprintf_file(stddeb, "DeleteFile: '%s'\n", path );

    if ((unixName = DOSFS_IsDevice( path )) != NULL)
    {
        dprintf_file(stddeb, "DeleteFile: removing device '%s'!\n", unixName);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return FALSE;
    }

    if (!(unixName = DOSFS_GetUnixFileName( path, TRUE ))) return FALSE;
    if (unlink( unixName ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           DeleteFile32W   (KERNEL32.72)
 */
BOOL32 DeleteFile32W( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = RemoveDirectory32A( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           FILE_SetFileType
 */
BOOL32 FILE_SetFileType( HFILE32 hFile, DWORD type )
{
    DOS_FILE *file = FILE_GetFile( hFile );
    if (!file) return FALSE;
    file->type = type;
    FILE_ReleaseFile( file );
    return TRUE;
}


/***********************************************************************
 *           GetFileType   (KERNEL32.222)
 */
DWORD GetFileType( HFILE32 hFile )
{
    DOS_FILE *file = FILE_GetFile(hFile);
    if (!file) return FILE_TYPE_UNKNOWN; /* FIXME: correct? */
    FILE_ReleaseFile( file );
    return file->type;
}


/***********************************************************************
 *              SetFileTime   (KERNEL32.493)
 */
BOOL32 SetFileTime( HFILE32 hFile,
                    const FILETIME *lpCreationTime,
                    const FILETIME *lpLastAccessTime,
                    const FILETIME *lpLastWriteTime )
{
    DOS_FILE *file = FILE_GetFile(hFile);
    struct utimbuf utimbuf;
    
    if (!file) return FILE_TYPE_UNKNOWN; /* FIXME: correct? */
    dprintf_file(stddeb,"SetFileTime(%s,%p,%p,%p)\n",
	file->unix_name,
	lpCreationTime,
	lpLastAccessTime,
	lpLastWriteTime
    );
    if (lpLastAccessTime)
	utimbuf.actime	= DOSFS_FileTimeToUnixTime(lpLastAccessTime);
    else
	utimbuf.actime	= 0; /* FIXME */
    if (lpLastWriteTime)
	utimbuf.modtime	= DOSFS_FileTimeToUnixTime(lpLastWriteTime);
    else
	utimbuf.modtime	= 0; /* FIXME */
    if (-1==utime(file->unix_name,&utimbuf))
    {
        FILE_ReleaseFile( file );
	FILE_SetDosError();
	return FALSE;
    }
    FILE_ReleaseFile( file );
    return TRUE;
}
