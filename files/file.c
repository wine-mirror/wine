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
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "ldt.h"
#include "process.h"
#include "task.h"
#include "debug.h"

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

static BOOL32 FILE_Read(K32OBJ *ptr, LPVOID lpBuffer, DWORD nNumberOfChars,
			LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped);
static BOOL32 FILE_Write(K32OBJ *ptr, LPCVOID lpBuffer, DWORD nNumberOfChars,
			 LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped);
static void FILE_Destroy( K32OBJ *obj );

const K32OBJ_OPS FILE_Ops =
{
    /* Object cannot be waited upon (FIXME: for now) */
    NULL,              /* signaled */
    NULL,              /* satisfied */
    NULL,              /* add_wait */
    NULL,              /* remove_wait */
    FILE_Read,	        /* read */
    FILE_Write,        /* write */
    FILE_Destroy       /* destroy */
};

struct DOS_FILE_LOCK {
  struct DOS_FILE_LOCK *	next;
  DWORD				base;
  DWORD				len;
  DWORD				processId;
  FILE_OBJECT *			dos_file;
  char *			unix_name;
};

typedef struct DOS_FILE_LOCK DOS_FILE_LOCK;

static DOS_FILE_LOCK *locks = NULL;
static void DOS_RemoveFileLocks(FILE_OBJECT *file);

/***********************************************************************
 *           FILE_Alloc
 *
 * Allocate a file.
 */
static HFILE32 FILE_Alloc( FILE_OBJECT **file )
{
    HFILE32 handle;
    *file = HeapAlloc( SystemHeap, 0, sizeof(FILE_OBJECT) );
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

    handle = HANDLE_Alloc( PROCESS_Current(), &(*file)->header,
                           FILE_ALL_ACCESS | GENERIC_READ |
                           GENERIC_WRITE | GENERIC_EXECUTE /*FIXME*/, TRUE );
    /* If the allocation failed, the object is already destroyed */
    if (handle == INVALID_HANDLE_VALUE32) *file = NULL;
    return handle;
}


/* FIXME: lpOverlapped is ignored */
static BOOL32 FILE_Read(K32OBJ *ptr, LPVOID lpBuffer, DWORD nNumberOfChars,
			LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped)
{
	FILE_OBJECT *file = (FILE_OBJECT *)ptr;
	int result;

	TRACE(file, "%p %p %ld\n", ptr, lpBuffer,
                     nNumberOfChars);

	if (nNumberOfChars == 0) {
		*lpNumberOfChars = 0;  /* FIXME: does this change */
		return TRUE;
	}

	if ((result = read(file->unix_handle, lpBuffer, nNumberOfChars)) == -1)
        {
            FILE_SetDosError();
            return FALSE;
	}
	*lpNumberOfChars = result;
	return TRUE;
}

/**
 *  experimentation yields that WriteFile:
 *	o does not truncate on write of 0
 *	o always changes the *lpNumberOfChars to actual number of
 *	  characters written
 *	o write of 0 nNumberOfChars returns TRUE
 */
static BOOL32 FILE_Write(K32OBJ *ptr, LPCVOID lpBuffer, DWORD nNumberOfChars,
			 LPDWORD lpNumberOfChars, LPOVERLAPPED lpOverlapped)
{
	FILE_OBJECT *file = (FILE_OBJECT *)ptr;
	int result;

	TRACE(file, "%p %p %ld\n", ptr, lpBuffer,
		      nNumberOfChars);

	*lpNumberOfChars = 0; 

	/* 
	 * I assume this loop around EAGAIN is here because
	 * win32 doesn't have interrupted system calls 
	 */

	for (;;)
        {
		result = write(file->unix_handle, lpBuffer, nNumberOfChars);
		if (result != -1) {
			*lpNumberOfChars = result;
                        return TRUE;
		}
		if (errno != EINTR) {
			FILE_SetDosError();
			return FALSE;
		}
        }
}



/***********************************************************************
 *           FILE_Destroy
 *
 * Destroy a DOS file.
 */
static void FILE_Destroy( K32OBJ *ptr )
{
    FILE_OBJECT *file = (FILE_OBJECT *)ptr;
    assert( ptr->type == K32OBJ_FILE );

    DOS_RemoveFileLocks(file);

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
static FILE_OBJECT *FILE_GetFile( HFILE32 handle )
{
    return (FILE_OBJECT *)HANDLE_GetObjPtr( PROCESS_Current(), handle,
                                            K32OBJ_FILE, 0 /*FIXME*/ );
}


/***********************************************************************
 *           FILE_ReleaseFile
 *
 * Release a DOS file obtained with FILE_GetFile.
 */
static void FILE_ReleaseFile( FILE_OBJECT *file )
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
    FILE_OBJECT *file;
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
    int save_errno = errno; /* errno gets overwritten by printf */

    TRACE(file, "errno = %d %s\n", errno, strerror(errno));
    switch (save_errno)
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
    case EINVAL:
    case ESPIPE:
        DOS_ERROR( ER_SeekError, EC_NotFound, SA_Ignore, EL_Disk );
        break;
    case ENOTEMPTY:
        DOS_ERROR( ERROR_DIR_NOT_EMPTY, EC_Exists, SA_Ignore, EL_Disk );
        break;
    default:
        perror( "int21: unknown errno" );
        DOS_ERROR( ER_GeneralFailure, EC_SystemFailure, SA_Abort, EL_Unknown );
        break;
    }
    errno = save_errno;
}


/***********************************************************************
 *           FILE_DupUnixHandle
 *
 * Duplicate a Unix handle into a task handle.
 */
HFILE32 FILE_DupUnixHandle( int fd )
{
    HFILE32 handle;
    FILE_OBJECT *file;

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
HFILE32 FILE_OpenUnixFile( const char *name, int mode )
{
    HFILE32 handle;
    FILE_OBJECT *file;
    struct stat st;

    if ((handle = FILE_Alloc( &file )) == INVALID_HANDLE_VALUE32)
        return INVALID_HANDLE_VALUE32;

    if ((file->unix_handle = open( name, mode, 0666 )) == -1)
    {
        if (!Options.failReadOnly && (mode == O_RDWR))
            file->unix_handle = open( name, O_RDONLY );
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

    /* File opened OK, now fill the FILE_OBJECT */

    file->unix_name = HEAP_strdupA( SystemHeap, 0, name );
    return handle;
}


/***********************************************************************
 *           FILE_Open
 */
HFILE32 FILE_Open( LPCSTR path, INT32 mode )
{
    DOS_FULL_NAME full_name;
    const char *unixName;

    TRACE(file, "'%s' %04x\n", path, mode );

    if (!path) return HFILE_ERROR32;

    if (DOSFS_IsDevice( path ))
    {
    	HFILE32	ret;

        TRACE(file, "opening device '%s'\n", path );

	if (HFILE_ERROR32!=(ret=DOSFS_OpenDevice( path, mode )))
		return ret;

	/* Do not silence this please. It is a critical error. -MM */
        ERR(file, "Couldn't open device '%s'!\n",path);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return HFILE_ERROR32;
	
    }
    else /* check for filename, don't check for last entry if creating */
    {
        if (!DOSFS_GetFullName( path, !(mode & O_CREAT), &full_name ))
            return HFILE_ERROR32;
        unixName = full_name.long_name;
    }
    return FILE_OpenUnixFile( unixName, mode );
}


/***********************************************************************
 *           FILE_Create
 */
static HFILE32 FILE_Create( LPCSTR path, int mode, int unique )
{
    HFILE32 handle;
    FILE_OBJECT *file;
    DOS_FULL_NAME full_name;

    TRACE(file, "'%s' %04x %d\n", path, mode, unique );

    if (!path) return INVALID_HANDLE_VALUE32;

    if (DOSFS_IsDevice( path ))
    {
        WARN(file, "cannot create DOS device '%s'!\n", path);
        DOS_ERROR( ER_AccessDenied, EC_NotFound, SA_Abort, EL_Disk );
        return INVALID_HANDLE_VALUE32;
    }

    if ((handle = FILE_Alloc( &file )) == INVALID_HANDLE_VALUE32)
        return INVALID_HANDLE_VALUE32;

    if (!DOSFS_GetFullName( path, FALSE, &full_name ))
    {
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    }
    if ((file->unix_handle = open( full_name.long_name,
                           O_CREAT | O_TRUNC | O_RDWR | (unique ? O_EXCL : 0),
                           mode )) == -1)
    {
        FILE_SetDosError();
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE32;
    } 

    /* File created OK, now fill the FILE_OBJECT */

    file->unix_name = HEAP_strdupA( SystemHeap, 0, full_name.long_name );
    return handle;
}


/***********************************************************************
 *           FILE_FillInfo
 *
 * Fill a file information from a struct stat.
 */
static void FILE_FillInfo( struct stat *st, BY_HANDLE_FILE_INFORMATION *info )
{
    if (S_ISDIR(st->st_mode))
        info->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    else
        info->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    if (!(st->st_mode & S_IWUSR))
        info->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

    DOSFS_UnixTimeToFileTime( st->st_mtime, &info->ftCreationTime, 0 );
    DOSFS_UnixTimeToFileTime( st->st_mtime, &info->ftLastWriteTime, 0 );
    DOSFS_UnixTimeToFileTime( st->st_atime, &info->ftLastAccessTime, 0 );

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

    if (!unixName || !info) return FALSE;

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
DWORD WINAPI GetFileInformationByHandle( HFILE32 hFile,
                                         BY_HANDLE_FILE_INFORMATION *info )
{
    FILE_OBJECT *file;
    DWORD ret = 0;
    struct stat st;

    if (!info) return 0;

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
DWORD WINAPI GetFileAttributes16( LPCSTR name )
{
    return GetFileAttributes32A( name );
}


/**************************************************************************
 *           GetFileAttributes32A   (KERNEL32.217)
 */
DWORD WINAPI GetFileAttributes32A( LPCSTR name )
{
    DOS_FULL_NAME full_name;
    BY_HANDLE_FILE_INFORMATION info;

    if (name == NULL) return -1;

    if (!DOSFS_GetFullName( name, TRUE, &full_name )) return -1;
    if (!FILE_Stat( full_name.long_name, &info )) return -1;
    return info.dwFileAttributes;
}


/**************************************************************************
 *           GetFileAttributes32W   (KERNEL32.218)
 */
DWORD WINAPI GetFileAttributes32W( LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    DWORD res = GetFileAttributes32A( nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.220)
 */
DWORD WINAPI GetFileSize( HFILE32 hFile, LPDWORD filesizehigh )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return 0;
    if (filesizehigh) *filesizehigh = info.nFileSizeHigh;
    return info.nFileSizeLow;
}


/***********************************************************************
 *           GetFileTime   (KERNEL32.221)
 */
BOOL32 WINAPI GetFileTime( HFILE32 hFile, FILETIME *lpCreationTime,
                           FILETIME *lpLastAccessTime,
                           FILETIME *lpLastWriteTime )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return FALSE;
    if (lpCreationTime)   *lpCreationTime   = info.ftCreationTime;
    if (lpLastAccessTime) *lpLastAccessTime = info.ftLastAccessTime;
    if (lpLastWriteTime)  *lpLastWriteTime  = info.ftLastWriteTime;
    return TRUE;
}

/***********************************************************************
 *           CompareFileTime   (KERNEL32.28)
 */
INT32 WINAPI CompareFileTime( LPFILETIME x, LPFILETIME y )
{
        if (!x || !y) return -1;

	if (x->dwHighDateTime > y->dwHighDateTime)
		return 1;
	if (x->dwHighDateTime < y->dwHighDateTime)
		return -1;
	if (x->dwLowDateTime > y->dwLowDateTime)
		return 1;
	if (x->dwLowDateTime < y->dwLowDateTime)
		return -1;
	return 0;
}

/***********************************************************************
 *           FILE_Dup
 *
 * dup() function for DOS handles.
 */
HFILE32 FILE_Dup( HFILE32 hFile )
{
    HFILE32 handle;

    TRACE(file, "FILE_Dup for handle %d\n", hFile );
    if (!DuplicateHandle( GetCurrentProcess(), hFile, GetCurrentProcess(),
                          &handle, FILE_ALL_ACCESS /* FIXME */, FALSE, 0 ))
        handle = HFILE_ERROR32;
    TRACE(file, "FILE_Dup return handle %d\n", handle );
    return handle;
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE32 FILE_Dup2( HFILE32 hFile1, HFILE32 hFile2 )
{
    FILE_OBJECT *file;

    TRACE(file, "FILE_Dup2 for handle %d\n", hFile1 );
    /* FIXME: should use DuplicateHandle */
    if (!(file = FILE_GetFile( hFile1 ))) return HFILE_ERROR32;
    if (!HANDLE_SetObjPtr( PROCESS_Current(), hFile2, &file->header, 0 ))
        hFile2 = HFILE_ERROR32;
    FILE_ReleaseFile( file );
    return hFile2;
}


/***********************************************************************
 *           GetTempFileName16   (KERNEL.97)
 */
UINT16 WINAPI GetTempFileName16( BYTE drive, LPCSTR prefix, UINT16 unique,
                                 LPSTR buffer )
{
    char temppath[144];

    if (!(drive & ~TF_FORCEDRIVE)) /* drive 0 means current default drive */
        drive |= DRIVE_GetCurrentDrive() + 'A';

    if ((drive & TF_FORCEDRIVE) &&
        !DRIVE_IsValid( toupper(drive & ~TF_FORCEDRIVE) - 'A' ))
    {
        drive &= ~TF_FORCEDRIVE;
        WARN(file, "invalid drive %d specified\n", drive );
    }

    if (drive & TF_FORCEDRIVE)
        sprintf(temppath,"%c:", drive & ~TF_FORCEDRIVE );
    else
        GetTempPath32A( 132, temppath );
    return (UINT16)GetTempFileName32A( temppath, prefix, unique, buffer );
}


/***********************************************************************
 *           GetTempFileName32A   (KERNEL32.290)
 */
UINT32 WINAPI GetTempFileName32A( LPCSTR path, LPCSTR prefix, UINT32 unique,
                                  LPSTR buffer)
{
    DOS_FULL_NAME full_name;
    int i;
    LPSTR p;
    UINT32 num = unique ? (unique & 0xffff) : time(NULL) & 0xffff;

    if ( !path || !prefix || !buffer ) return 0;

    strcpy( buffer, path );
    p = buffer + strlen(buffer);

    /* add a \, if there isn't one and path is more than just the drive letter ... */
    if ( !((strlen(buffer) == 2) && (buffer[1] == ':')) 
	&& ((p == buffer) || (p[-1] != '\\'))) *p++ = '\\';

    *p++ = '~';
    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;
    sprintf( p, "%04x.tmp", num );

    /* Now try to create it */

    if (!unique)
    {
        do
        {
            HFILE32 handle = FILE_Create( buffer, 0666, TRUE );
            if (handle != INVALID_HANDLE_VALUE32)
            {  /* We created it */
                TRACE(file, "created %s\n",
			      buffer);
                CloseHandle( handle );
                break;
            }
            if (DOS_ExtendedError != ER_FileExists)
                break;  /* No need to go on */
            num++;
            sprintf( p, "%04x.tmp", num );
        } while (num != (unique & 0xffff));
    }

    /* Get the full path name */

    if (DOSFS_GetFullName( buffer, FALSE, &full_name ))
    {
        /* Check if we have write access in the directory */
        if ((p = strrchr( full_name.long_name, '/' ))) *p = '\0';
        if (access( full_name.long_name, W_OK ) == -1)
            WARN(file, "returns '%s', which doesn't seem to be writeable.\n",
		 buffer);
    }
    TRACE(file, "returning %s\n", buffer );
    return unique ? unique : num;
}


/***********************************************************************
 *           GetTempFileName32W   (KERNEL32.291)
 */
UINT32 WINAPI GetTempFileName32W( LPCWSTR path, LPCWSTR prefix, UINT32 unique,
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
    DOS_FULL_NAME full_name;
    char *p;
    int unixMode;

    if (!ofs) return HFILE_ERROR32;


    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;

    if (!name) {
	ERR(file, "called with `name' set to NULL ! Please debug.\n");
	return HFILE_ERROR32;
    }

    TRACE(file, "%s %04x\n", name, mode );

    /* the watcom 10.6 IDE relies on a valid path returned in ofs->szPathName
       Are there any cases where getting the path here is wrong? 
       Uwe Bonnes 1997 Apr 2 */
    if (!GetFullPathName32A( name, sizeof(ofs->szPathName),
			     ofs->szPathName, NULL )) goto error;

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        ofs->fFixedDisk = (GetDriveType16( ofs->szPathName[0]-'A' )
                           != DRIVE_REMOVABLE);
        TRACE(file, "(%s): OF_PARSE, res = '%s'\n",
                      name, ofs->szPathName );
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if ((hFileRet = FILE_Create(name,0666,FALSE))== INVALID_HANDLE_VALUE32)
            goto error;
        goto success;
    }

    /* If OF_SEARCH is set, ignore the given path */

    if ((mode & OF_SEARCH) && !(mode & OF_REOPEN))
    {
        /* First try the file name as is */
        if (DOSFS_GetFullName( name, TRUE, &full_name )) goto found;
        /* Now remove the path */
        if (name[0] && (name[1] == ':')) name += 2;
        if ((p = strrchr( name, '\\' ))) name = p + 1;
        if ((p = strrchr( name, '/' ))) name = p + 1;
        if (!name[0]) goto not_found;
    }

    /* Now look for the file */

    if (!DIR_SearchPath( NULL, name, NULL, &full_name, win32 )) goto not_found;

found:
    TRACE(file, "found %s = %s\n",
                  full_name.long_name, full_name.short_name );
    lstrcpyn32A( ofs->szPathName, full_name.short_name,
                 sizeof(ofs->szPathName) );

    if (mode & OF_DELETE)
    {
        if (unlink( full_name.long_name ) == -1) goto not_found;
        TRACE(file, "(%s): OF_DELETE return = OK\n", name);
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

    hFileRet = FILE_OpenUnixFile( full_name.long_name, unixMode );
    if (hFileRet == HFILE_ERROR32) goto not_found;
    GetFileTime( hFileRet, NULL, NULL, &filetime );
    FileTimeToDosDateTime( &filetime, &filedatetime[0], &filedatetime[1] );
    if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
    {
        if (memcmp( ofs->reserved, filedatetime, sizeof(ofs->reserved) ))
        {
            CloseHandle( hFileRet );
            WARN(file, "(%s): OF_VERIFY failed\n", name );
            /* FIXME: what error here? */
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            goto error;
        }
    }
    memcpy( ofs->reserved, filedatetime, sizeof(ofs->reserved) );

success:  /* We get here if the open was successful */
    TRACE(file, "(%s): OK, return = %d\n", name, hFileRet );
    if (mode & OF_EXIST) /* Return the handle, but close it first */
        CloseHandle( hFileRet );
    return hFileRet;

not_found:  /* We get here if the file does not exist */
    WARN(file, "'%s' not found\n", name );
    DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = DOS_ExtendedError;
    WARN(file, "(%s): return = HFILE_ERROR error= %d\n", 
		  name,ofs->nErrCode );
    return HFILE_ERROR32;
}


/***********************************************************************
 *           OpenFile16   (KERNEL.74)
 */
HFILE16 WINAPI OpenFile16( LPCSTR name, OFSTRUCT *ofs, UINT16 mode )
{
    return FILE_DoOpenFile( name, ofs, mode, FALSE );
}


/***********************************************************************
 *           OpenFile32   (KERNEL32.396)
 */
HFILE32 WINAPI OpenFile32( LPCSTR name, OFSTRUCT *ofs, UINT32 mode )
{
    return FILE_DoOpenFile( name, ofs, mode, TRUE );
}


/***********************************************************************
 *           _lclose16   (KERNEL.81)
 */
HFILE16 WINAPI _lclose16( HFILE16 hFile )
{
    TRACE(file, "handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR16;
}


/***********************************************************************
 *           _lclose32   (KERNEL32.592)
 */
HFILE32 WINAPI _lclose32( HFILE32 hFile )
{
    TRACE(file, "handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR32;
}


/***********************************************************************
 *           WIN16_hread
 */
LONG WINAPI WIN16_hread( HFILE16 hFile, SEGPTR buffer, LONG count )
{
    LONG maxlen;

    TRACE(file, "%d %08lx %ld\n",
                  hFile, (DWORD)buffer, count );

    /* Some programs pass a count larger than the allocated buffer */
    maxlen = GetSelectorLimit( SELECTOROF(buffer) ) - OFFSETOF(buffer) + 1;
    if (count > maxlen) count = maxlen;
    return _lread32( hFile, PTR_SEG_TO_LIN(buffer), count );
}


/***********************************************************************
 *           WIN16_lread
 */
UINT16 WINAPI WIN16_lread( HFILE16 hFile, SEGPTR buffer, UINT16 count )
{
    return (UINT16)WIN16_hread( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lread32   (KERNEL32.596)
 */
UINT32 WINAPI _lread32( HFILE32 handle, LPVOID buffer, UINT32 count )
{
    K32OBJ *ptr;
    DWORD numWritten;
    BOOL32 result = FALSE;

    TRACE( file, "%d %p %d\n", handle, buffer, count);
    if (!(ptr = HANDLE_GetObjPtr( PROCESS_Current(), handle,
                                  K32OBJ_UNKNOWN, 0))) return -1;
    if (K32OBJ_OPS(ptr)->read)
        result = K32OBJ_OPS(ptr)->read(ptr, buffer, count, &numWritten, NULL);
    K32OBJ_DecCount( ptr );
    if (!result) return -1;
    return (UINT32)numWritten;
}


/***********************************************************************
 *           _lread16   (KERNEL.82)
 */
UINT16 WINAPI _lread16( HFILE16 hFile, LPVOID buffer, UINT16 count )
{
    return (UINT16)_lread32( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat16   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    TRACE(file, "%s %02x\n", path, attr );
    return (HFILE16)FILE_Create( path, mode, FALSE );
}


/***********************************************************************
 *           _lcreat32   (KERNEL32.593)
 */
HFILE32 WINAPI _lcreat32( LPCSTR path, INT32 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    TRACE(file, "%s %02x\n", path, attr );
    return FILE_Create( path, mode, FALSE );
}


/***********************************************************************
 *           _lcreat_uniq   (Not a Windows API)
 */
HFILE32 _lcreat_uniq( LPCSTR path, INT32 attr )
{
    int mode = (attr & 1) ? 0444 : 0666;
    TRACE(file, "%s %02x\n", path, attr );
    return FILE_Create( path, mode, TRUE );
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.492)
 */
DWORD WINAPI SetFilePointer( HFILE32 hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    FILE_OBJECT *file;
    int origin, result;

    if (highword && *highword)
    {
        FIXME(file, "64-bit offsets not supported yet\n");
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0xffffffff;
    }
    TRACE(file, "handle %d offset %ld origin %ld\n",
	  hFile, distance, method );

    if (!(file = FILE_GetFile( hFile ))) return 0xffffffff;
    switch(method)
    {
        case FILE_CURRENT: origin = SEEK_CUR; break;
        case FILE_END:     origin = SEEK_END; break;
        default:           origin = SEEK_SET; break;
    }

    if ((result = lseek( file->unix_handle, distance, origin )) == -1)
      {
	/* care for this pathological case:
	   SetFilePointer(00000006,ffffffff,00000000,00000002)
	   ret=0062ab4a fs=01c7 */
	if ((distance != -1))
        FILE_SetDosError();
	else
	  result = 0;
      }
    FILE_ReleaseFile( file );
    return (DWORD)result;
}


/***********************************************************************
 *           _llseek16   (KERNEL.84)
 */
LONG WINAPI _llseek16( HFILE16 hFile, LONG lOffset, INT16 nOrigin )
{
    return SetFilePointer( hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _llseek32   (KERNEL32.594)
 */
LONG WINAPI _llseek32( HFILE32 hFile, LONG lOffset, INT32 nOrigin )
{
    return SetFilePointer( hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lopen16   (KERNEL.85)
 */
HFILE16 WINAPI _lopen16( LPCSTR path, INT16 mode )
{
    return _lopen32( path, mode );
}


/***********************************************************************
 *           _lopen32   (KERNEL32.595)
 */
HFILE32 WINAPI _lopen32( LPCSTR path, INT32 mode )
{
    INT32 unixMode;

    TRACE(file, "('%s',%04x)\n", path, mode );

    switch(mode & 3)
    {
    case OF_WRITE:
        unixMode = O_WRONLY;
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
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite32( hFile, buffer, (LONG)count );
}

/***********************************************************************
 *           _lwrite32   (KERNEL.86)
 */
UINT32 WINAPI _lwrite32( HFILE32 hFile, LPCSTR buffer, UINT32 count )
{
    return (UINT32)_hwrite32( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _hread16   (KERNEL.349)
 */
LONG WINAPI _hread16( HFILE16 hFile, LPVOID buffer, LONG count)
{
    return _lread32( hFile, buffer, count );
}


/***********************************************************************
 *           _hread32   (KERNEL32.590)
 */
LONG WINAPI _hread32( HFILE32 hFile, LPVOID buffer, LONG count)
{
    return _lread32( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite16   (KERNEL.350)
 */
LONG WINAPI _hwrite16( HFILE16 hFile, LPCSTR buffer, LONG count )
{
    return _hwrite32( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite32   (KERNEL32.591)
 *
 *	experimenation yields that _lwrite:
 *		o truncates the file at the current position with 
 *		  a 0 len write
 *		o returns 0 on a 0 length write
 *		o works with console handles
 *		
 */
LONG WINAPI _hwrite32( HFILE32 handle, LPCSTR buffer, LONG count )
{
	K32OBJ *ioptr;
	DWORD result;
	BOOL32 status = FALSE;
	
	TRACE(file, "%d %p %ld\n", handle, buffer, count );

	if (count == 0) {       /* Expand or truncate at current position */
		FILE_OBJECT *file = FILE_GetFile(handle);

		if ( ftruncate(file->unix_handle,
			       lseek( file->unix_handle, 0, SEEK_CUR)) == 0 ) {
			FILE_ReleaseFile(file);
			return 0;
		} else {
			FILE_SetDosError();
			FILE_ReleaseFile(file);
			return HFILE_ERROR32;
		}
	}
	
	if (!(ioptr = HANDLE_GetObjPtr( PROCESS_Current(), handle,
                                        K32OBJ_UNKNOWN, 0 )))
            return HFILE_ERROR32;
        if (K32OBJ_OPS(ioptr)->write)
            status = K32OBJ_OPS(ioptr)->write(ioptr, buffer, count, &result, NULL);
	K32OBJ_DecCount( ioptr );
	if (!status) result = HFILE_ERROR32;
	return result;
}


/***********************************************************************
 *           SetHandleCount16   (KERNEL.199)
 */
UINT16 WINAPI SetHandleCount16( UINT16 count )
{
    HGLOBAL16 hPDB = GetCurrentPDB();
    PDB *pdb = (PDB *)GlobalLock16( hPDB );
    BYTE *files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );

    TRACE(file, "(%d)\n", count );

    if (count < 20) count = 20;  /* No point in going below 20 */
    else if (count > 254) count = 254;

    if (count == 20)
    {
        if (pdb->nbFiles > 20)
        {
            memcpy( pdb->fileHandles, files, 20 );
            GlobalFree16( pdb->hFileHandles );
            pdb->fileHandlesPtr = (SEGPTR)MAKELONG( 0x18,
                                                   GlobalHandleToSel( hPDB ) );
            pdb->hFileHandles = 0;
            pdb->nbFiles = 20;
        }
    }
    else  /* More than 20, need a new file handles table */
    {
        BYTE *newfiles;
        HGLOBAL16 newhandle = GlobalAlloc16( GMEM_MOVEABLE, count );
        if (!newhandle)
        {
            DOS_ERROR( ER_OutOfMemory, EC_OutOfResource, SA_Abort, EL_Memory );
            return pdb->nbFiles;
        }
        newfiles = (BYTE *)GlobalLock16( newhandle );

        if (count > pdb->nbFiles)
        {
            memcpy( newfiles, files, pdb->nbFiles );
            memset( newfiles + pdb->nbFiles, 0xff, count - pdb->nbFiles );
        }
        else memcpy( newfiles, files, count );
        if (pdb->nbFiles > 20) GlobalFree16( pdb->hFileHandles );
        pdb->fileHandlesPtr = WIN16_GlobalLock16( newhandle );
        pdb->hFileHandles   = newhandle;
        pdb->nbFiles = count;
    }
    return pdb->nbFiles;
}


/*************************************************************************
 *           SetHandleCount32   (KERNEL32.494)
 */
UINT32 WINAPI SetHandleCount32( UINT32 count )
{
    return MIN( 256, count );
}


/***********************************************************************
 *           FlushFileBuffers   (KERNEL32.133)
 */
BOOL32 WINAPI FlushFileBuffers( HFILE32 hFile )
{
    FILE_OBJECT *file;
    BOOL32 ret;

    TRACE(file, "(%d)\n", hFile );
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
BOOL32 WINAPI SetEndOfFile( HFILE32 hFile )
{
    FILE_OBJECT *file;
    BOOL32 ret = TRUE;

    TRACE(file, "(%d)\n", hFile );
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
BOOL16 WINAPI DeleteFile16( LPCSTR path )
{
    return DeleteFile32A( path );
}


/***********************************************************************
 *           DeleteFile32A   (KERNEL32.71)
 */
BOOL32 WINAPI DeleteFile32A( LPCSTR path )
{
    DOS_FULL_NAME full_name;

    TRACE(file, "'%s'\n", path );

    if (DOSFS_IsDevice( path ))
    {
        WARN(file, "cannot remove DOS device '%s'!\n", path);
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return FALSE;
    }

    if (!DOSFS_GetFullName( path, TRUE, &full_name )) return FALSE;
    if (unlink( full_name.long_name ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           DeleteFile32W   (KERNEL32.72)
 */
BOOL32 WINAPI DeleteFile32W( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL32 ret = DeleteFile32A( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           FILE_SetFileType
 */
BOOL32 FILE_SetFileType( HFILE32 hFile, DWORD type )
{
    FILE_OBJECT *file = FILE_GetFile( hFile );
    if (!file) return FALSE;
    file->type = type;
    FILE_ReleaseFile( file );
    return TRUE;
}


/***********************************************************************
 *           FILE_mmap
 */
LPVOID FILE_mmap( HFILE32 hFile, LPVOID start,
                  DWORD size_high, DWORD size_low,
                  DWORD offset_high, DWORD offset_low,
                  int prot, int flags )
{
    LPVOID ret;
    FILE_OBJECT *file = FILE_GetFile( hFile );
    if (!file) return (LPVOID)-1;
    ret = FILE_dommap( file, start, size_high, size_low,
                       offset_high, offset_low, prot, flags );
    FILE_ReleaseFile( file );
    return ret;
}


/***********************************************************************
 *           FILE_dommap
 */
LPVOID FILE_dommap( FILE_OBJECT *file, LPVOID start,
                    DWORD size_high, DWORD size_low,
                    DWORD offset_high, DWORD offset_low,
                    int prot, int flags )
{
    int fd = -1;
    int pos;
    LPVOID ret;

    if (size_high || offset_high)
        FIXME(file, "offsets larger than 4Gb not supported\n");

    if (!file)
    {
#ifdef MAP_ANON
        flags |= MAP_ANON;
#else
        static int fdzero = -1;

        if (fdzero == -1)
        {
            if ((fdzero = open( "/dev/zero", O_RDONLY )) == -1)
            {
                perror( "/dev/zero: open" );
                exit(1);
            }
        }
        fd = fdzero;
#endif  /* MAP_ANON */
	/* Linux EINVAL's on us if we don't pass MAP_PRIVATE to an anon mmap */
#ifdef MAP_SHARED
	flags &= ~MAP_SHARED;
#endif
#ifdef MAP_PRIVATE
	flags |= MAP_PRIVATE;
#endif
    }
    else fd = file->unix_handle;

    if ((ret = mmap( start, size_low, prot,
                     flags, fd, offset_low )) != (LPVOID)-1)
        return ret;

    /* mmap() failed; if this is because the file offset is not    */
    /* page-aligned (EINVAL), or because the underlying filesystem */
    /* does not support mmap() (ENOEXEC), we do it by hand.        */

    if (!file) return ret;
    if ((errno != ENOEXEC) && (errno != EINVAL)) return ret;
    if (prot & PROT_WRITE)
    {
        /* We cannot fake shared write mappings */
#ifdef MAP_SHARED
	if (flags & MAP_SHARED) return ret;
#endif
#ifdef MAP_PRIVATE
	if (!(flags & MAP_PRIVATE)) return ret;
#endif
    }
/*    printf( "FILE_mmap: mmap failed (%d), faking it\n", errno );*/
    /* Reserve the memory with an anonymous mmap */
    ret = FILE_dommap( NULL, start, size_high, size_low, 0, 0,
                       PROT_READ | PROT_WRITE, flags );
    if (ret == (LPVOID)-1) return ret;
    /* Now read in the file */
    if ((pos = lseek( fd, offset_low, SEEK_SET )) == -1)
    {
        FILE_munmap( ret, size_high, size_low );
        return (LPVOID)-1;
    }
    read( fd, ret, size_low );
    lseek( fd, pos, SEEK_SET );  /* Restore the file pointer */
    mprotect( ret, size_low, prot );  /* Set the right protection */
    return ret;
}


/***********************************************************************
 *           FILE_munmap
 */
int FILE_munmap( LPVOID start, DWORD size_high, DWORD size_low )
{
    if (size_high)
      FIXME(file, "offsets larger than 4Gb not supported\n");
    return munmap( start, size_low );
}


/***********************************************************************
 *           GetFileType   (KERNEL32.222)
 */
DWORD WINAPI GetFileType( HFILE32 hFile )
{
    FILE_OBJECT *file = FILE_GetFile(hFile);
    if (!file) return FILE_TYPE_UNKNOWN; /* FIXME: correct? */
    FILE_ReleaseFile( file );
    return file->type;
}


/**************************************************************************
 *           MoveFileEx32A   (KERNEL32.???)
 */
BOOL32 WINAPI MoveFileEx32A( LPCSTR fn1, LPCSTR fn2, DWORD flag )
{
    DOS_FULL_NAME full_name1, full_name2;
    int mode=0; /* mode == 1: use copy */

    TRACE(file, "(%s,%s,%04lx)\n", fn1, fn2, flag);

    if (!DOSFS_GetFullName( fn1, TRUE, &full_name1 )) return FALSE;
    if (fn2) { /* !fn2 means delete fn1 */
      if (!DOSFS_GetFullName( fn2, FALSE, &full_name2 )) return FALSE;
      /* Source name and target path are valid */
      if ( full_name1.drive != full_name2.drive)
	/* use copy, if allowed */
	if (!(flag & MOVEFILE_COPY_ALLOWED)) {
	  /* FIXME: Use right error code */
	  DOS_ERROR( ER_FileExists, EC_Exists, SA_Abort, EL_Disk );
	  return FALSE;
	}
	else mode =1;
      if (DOSFS_GetFullName( fn2, TRUE, &full_name2 )) 
	/* target exists, check if we may overwrite */
	if (!(flag & MOVEFILE_REPLACE_EXISTING)) {
	  /* FIXME: Use right error code */
	  DOS_ERROR( ER_AccessDenied, EC_AccessDenied, SA_Abort, EL_Disk );
	  return FALSE;
	}
    }
    else /* fn2 == NULL means delete source */
      if (flag & MOVEFILE_DELAY_UNTIL_REBOOT) {
	if (flag & MOVEFILE_COPY_ALLOWED) {  
	  WARN(file, "Illegal flag\n");
	  DOS_ERROR( ER_GeneralFailure, EC_SystemFailure, SA_Abort,
		     EL_Unknown );
	  return FALSE;
	}
	/* FIXME: (bon@elektron.ikp.physik.th-darmstadt.de 970706)
	   Perhaps we should queue these command and execute it 
	   when exiting... What about using on_exit(2)
	   */
	FIXME(file, "Please delete file '%s' when Wine has finished\n",
	      full_name1.long_name);
	return TRUE;
      }
      else if (unlink( full_name1.long_name ) == -1)
      {
        FILE_SetDosError();
        return FALSE;
      }
      else  return TRUE; /* successfully deleted */

    if (flag & MOVEFILE_DELAY_UNTIL_REBOOT) {
	/* FIXME: (bon@elektron.ikp.physik.th-darmstadt.de 970706)
	   Perhaps we should queue these command and execute it 
	   when exiting... What about using on_exit(2)
	   */
	FIXME(file,"Please move existing file '%s' to file '%s'"
	      "when Wine has finished\n", 
	      full_name1.long_name, full_name2.long_name);
	return TRUE;
    }

    if (!mode) /* move the file */
      if (rename( full_name1.long_name, full_name2.long_name ) == -1)
	{
	  FILE_SetDosError();
	  return FALSE;
	}
      else return TRUE;
    else /* copy File */
      return CopyFile32A(fn1, fn2, (!(flag & MOVEFILE_REPLACE_EXISTING))); 
    
}

/**************************************************************************
 *           MoveFileEx32W   (KERNEL32.???)
 */
BOOL32 WINAPI MoveFileEx32W( LPCWSTR fn1, LPCWSTR fn2, DWORD flag )
{
    LPSTR afn1 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn1 );
    LPSTR afn2 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn2 );
    BOOL32 res = MoveFileEx32A( afn1, afn2, flag );
    HeapFree( GetProcessHeap(), 0, afn1 );
    HeapFree( GetProcessHeap(), 0, afn2 );
    return res;
}


/**************************************************************************
 *           MoveFile32A   (KERNEL32.387)
 *
 *  Move file or directory
 */
BOOL32 WINAPI MoveFile32A( LPCSTR fn1, LPCSTR fn2 )
{
    DOS_FULL_NAME full_name1, full_name2;
    struct stat fstat;

    TRACE(file, "(%s,%s)\n", fn1, fn2 );

    if (!DOSFS_GetFullName( fn1, TRUE, &full_name1 )) return FALSE;
    if (DOSFS_GetFullName( fn2, TRUE, &full_name2 )) 
      /* The new name must not already exist */ 
      return FALSE;
    if (!DOSFS_GetFullName( fn2, FALSE, &full_name2 )) return FALSE;

    if (full_name1.drive == full_name2.drive) /* move */
    if (rename( full_name1.long_name, full_name2.long_name ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
      else return TRUE;
    else /*copy */ {
      if (stat(  full_name1.long_name, &fstat ))
	{
	  WARN(file, "Invalid source file %s\n",
			full_name1.long_name);
	  FILE_SetDosError();
	  return FALSE;
	}
      if (S_ISDIR(fstat.st_mode)) {
	/* No Move for directories across file systems */
	/* FIXME: Use right error code */
	DOS_ERROR( ER_GeneralFailure, EC_SystemFailure, SA_Abort,
		   EL_Unknown );
	return FALSE;
      }
      else
	return CopyFile32A(fn1, fn2, TRUE); /*fail, if exist */ 
    }
}


/**************************************************************************
 *           MoveFile32W   (KERNEL32.390)
 */
BOOL32 WINAPI MoveFile32W( LPCWSTR fn1, LPCWSTR fn2 )
{
    LPSTR afn1 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn1 );
    LPSTR afn2 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn2 );
    BOOL32 res = MoveFile32A( afn1, afn2 );
    HeapFree( GetProcessHeap(), 0, afn1 );
    HeapFree( GetProcessHeap(), 0, afn2 );
    return res;
}


/**************************************************************************
 *           CopyFile32A   (KERNEL32.36)
 */
BOOL32 WINAPI CopyFile32A( LPCSTR source, LPCSTR dest, BOOL32 fail_if_exists )
{
    HFILE32 h1, h2;
    BY_HANDLE_FILE_INFORMATION info;
    UINT32 count;
    BOOL32 ret = FALSE;
    int mode;
    char buffer[2048];

    if ((h1 = _lopen32( source, OF_READ )) == HFILE_ERROR32) return FALSE;
    if (!GetFileInformationByHandle( h1, &info ))
    {
        CloseHandle( h1 );
        return FALSE;
    }
    mode = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666;
    if ((h2 = FILE_Create( dest, mode, fail_if_exists )) == HFILE_ERROR32)
    {
        CloseHandle( h1 );
        return FALSE;
    }
    while ((count = _lread32( h1, buffer, sizeof(buffer) )) > 0)
    {
        char *p = buffer;
        while (count > 0)
        {
            INT32 res = _lwrite32( h2, p, count );
            if (res <= 0) goto done;
            p += res;
            count -= res;
        }
    }
    ret =  TRUE;
done:
    CloseHandle( h1 );
    CloseHandle( h2 );
    return ret;
}


/**************************************************************************
 *           CopyFile32W   (KERNEL32.37)
 */
BOOL32 WINAPI CopyFile32W( LPCWSTR source, LPCWSTR dest, BOOL32 fail_if_exists)
{
    LPSTR sourceA = HEAP_strdupWtoA( GetProcessHeap(), 0, source );
    LPSTR destA   = HEAP_strdupWtoA( GetProcessHeap(), 0, dest );
    BOOL32 ret = CopyFile32A( sourceA, destA, fail_if_exists );
    HeapFree( GetProcessHeap(), 0, sourceA );
    HeapFree( GetProcessHeap(), 0, destA );
    return ret;
}


/***********************************************************************
 *              SetFileTime   (KERNEL32.493)
 */
BOOL32 WINAPI SetFileTime( HFILE32 hFile,
                           const FILETIME *lpCreationTime,
                           const FILETIME *lpLastAccessTime,
                           const FILETIME *lpLastWriteTime )
{
    FILE_OBJECT *file = FILE_GetFile(hFile);
    struct utimbuf utimbuf;
    
    if (!file) return FILE_TYPE_UNKNOWN; /* FIXME: correct? */
    TRACE(file,"(%s,%p,%p,%p)\n",
	file->unix_name,
	lpCreationTime,
	lpLastAccessTime,
	lpLastWriteTime
    );
    if (lpLastAccessTime)
	utimbuf.actime	= DOSFS_FileTimeToUnixTime(lpLastAccessTime, NULL);
    else
	utimbuf.actime	= 0; /* FIXME */
    if (lpLastWriteTime)
	utimbuf.modtime	= DOSFS_FileTimeToUnixTime(lpLastWriteTime, NULL);
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

/* Locks need to be mirrored because unix file locking is based
 * on the pid. Inside of wine there can be multiple WINE processes
 * that share the same unix pid.
 * Read's and writes should check these locks also - not sure
 * how critical that is at this point (FIXME).
 */

static BOOL32 DOS_AddLock(FILE_OBJECT *file, struct flock *f)
{
  DOS_FILE_LOCK *curr;
  DWORD		processId;

  processId = GetCurrentProcessId();

  /* check if lock overlaps a current lock for the same file */
  for (curr = locks; curr; curr = curr->next) {
    if (strcmp(curr->unix_name, file->unix_name) == 0) {
      if ((f->l_start == curr->base) && (f->l_len == curr->len))
	return TRUE;/* region is identic */
      if ((f->l_start < (curr->base + curr->len)) &&
	  ((f->l_start + f->l_len) > curr->base)) {
	/* region overlaps */
	return FALSE;
      }
    }
  }

  curr = HeapAlloc( SystemHeap, 0, sizeof(DOS_FILE_LOCK) );
  curr->processId = GetCurrentProcessId();
  curr->base = f->l_start;
  curr->len = f->l_len;
  curr->unix_name = HEAP_strdupA( SystemHeap, 0, file->unix_name);
  curr->next = locks;
  curr->dos_file = file;
  locks = curr;
  return TRUE;
}

static void DOS_RemoveFileLocks(FILE_OBJECT *file)
{
  DWORD		processId;
  DOS_FILE_LOCK **curr;
  DOS_FILE_LOCK *rem;

  processId = GetCurrentProcessId();
  curr = &locks;
  while (*curr) {
    if ((*curr)->dos_file == file) {
      rem = *curr;
      *curr = (*curr)->next;
      HeapFree( SystemHeap, 0, rem->unix_name );
      HeapFree( SystemHeap, 0, rem );
    }
    else
      curr = &(*curr)->next;
  }
}

static BOOL32 DOS_RemoveLock(FILE_OBJECT *file, struct flock *f)
{
  DWORD		processId;
  DOS_FILE_LOCK **curr;
  DOS_FILE_LOCK *rem;

  processId = GetCurrentProcessId();
  for (curr = &locks; *curr; curr = &(*curr)->next) {
    if ((*curr)->processId == processId &&
	(*curr)->dos_file == file &&
	(*curr)->base == f->l_start &&
	(*curr)->len == f->l_len) {
      /* this is the same lock */
      rem = *curr;
      *curr = (*curr)->next;
      HeapFree( SystemHeap, 0, rem->unix_name );
      HeapFree( SystemHeap, 0, rem );
      return TRUE;
    }
  }
  /* no matching lock found */
  return FALSE;
}


/**************************************************************************
 *           LockFile   (KERNEL32.511)
 */
BOOL32 WINAPI LockFile(
	HFILE32 hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToLockLow,DWORD nNumberOfBytesToLockHigh )
{
  struct flock f;
  FILE_OBJECT *file;

  TRACE(file, "handle %d offsetlow=%ld offsethigh=%ld nbyteslow=%ld nbyteshigh=%ld\n",
	       hFile, dwFileOffsetLow, dwFileOffsetHigh,
	       nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);

  if (dwFileOffsetHigh || nNumberOfBytesToLockHigh) {
    FIXME(file, "Unimplemented bytes > 32bits\n");
    return FALSE;
  }

  f.l_start = dwFileOffsetLow;
  f.l_len = nNumberOfBytesToLockLow;
  f.l_whence = SEEK_SET;
  f.l_pid = 0;
  f.l_type = F_WRLCK;

  if (!(file = FILE_GetFile(hFile))) return FALSE;

  /* shadow locks internally */
  if (!DOS_AddLock(file, &f)) {
    DOS_ERROR( ER_LockViolation, EC_AccessDenied, SA_Ignore, EL_Disk );
    return FALSE;
  }

  /* FIXME: Unix locking commented out for now, doesn't work with Excel */
#ifdef USE_UNIX_LOCKS
  if (fcntl(file->unix_handle, F_SETLK, &f) == -1) {
    if (errno == EACCES || errno == EAGAIN) {
      DOS_ERROR( ER_LockViolation, EC_AccessDenied, SA_Ignore, EL_Disk );
    }
    else {
      FILE_SetDosError();
    }
    /* remove our internal copy of the lock */
    DOS_RemoveLock(file, &f);
    return FALSE;
  }
#endif
  return TRUE;
}


/**************************************************************************
 *           UnlockFile   (KERNEL32.703)
 */
BOOL32 WINAPI UnlockFile(
	HFILE32 hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToUnlockLow,DWORD nNumberOfBytesToUnlockHigh )
{
  FILE_OBJECT *file;
  struct flock f;

  TRACE(file, "handle %d offsetlow=%ld offsethigh=%ld nbyteslow=%ld nbyteshigh=%ld\n",
	       hFile, dwFileOffsetLow, dwFileOffsetHigh,
	       nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);

  if (dwFileOffsetHigh || nNumberOfBytesToUnlockHigh) {
    WARN(file, "Unimplemented bytes > 32bits\n");
    return FALSE;
  }

  f.l_start = dwFileOffsetLow;
  f.l_len = nNumberOfBytesToUnlockLow;
  f.l_whence = SEEK_SET;
  f.l_pid = 0;
  f.l_type = F_UNLCK;

  if (!(file = FILE_GetFile(hFile))) return FALSE;

  DOS_RemoveLock(file, &f);	/* ok if fails - may be another wine */

  /* FIXME: Unix locking commented out for now, doesn't work with Excel */
#ifdef USE_UNIX_LOCKS
  if (fcntl(file->unix_handle, F_SETLK, &f) == -1) {
    FILE_SetDosError();
    return FALSE;
  }
#endif
  return TRUE;
}

/**************************************************************************
 * GetFileAttributesEx32A [KERNEL32.874]
 */
BOOL32 WINAPI GetFileAttributesEx32A(
	LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
    DOS_FULL_NAME full_name;
    BY_HANDLE_FILE_INFORMATION info;
    
    if (lpFileName == NULL) return FALSE;
    if (lpFileInformation == NULL) return FALSE;

    if (fInfoLevelId == GetFileExInfoStandard) {
	LPWIN32_FILE_ATTRIBUTE_DATA lpFad = 
	    (LPWIN32_FILE_ATTRIBUTE_DATA) lpFileInformation;
	if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name )) return FALSE;
	if (!FILE_Stat( full_name.long_name, &info )) return FALSE;

	lpFad->dwFileAttributes = info.dwFileAttributes;
	lpFad->ftCreationTime   = info.ftCreationTime;
	lpFad->ftLastAccessTime = info.ftLastAccessTime;
	lpFad->ftLastWriteTime  = info.ftLastWriteTime;
	lpFad->nFileSizeHigh    = info.nFileSizeHigh;
	lpFad->nFileSizeLow     = info.nFileSizeLow;
    }
    else {
	FIXME (file, "invalid info level %d!\n", fInfoLevelId);
	return FALSE;
    }

    return TRUE;
}


/**************************************************************************
 * GetFileAttributesEx32W [KERNEL32.875]
 */
BOOL32 WINAPI GetFileAttributesEx32W(
	LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFileName );
    BOOL32 res = 
	GetFileAttributesEx32A( nameA, fInfoLevelId, lpFileInformation);
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


