/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 *
 * TODO:
 *    Fix the CopyFileEx methods to implement the "extented" functionality.
 *    Right now, they simply call the CopyFile method.
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
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "device.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "ldt.h"
#include "process.h"
#include "task.h"
#include "async.h"
#include "wincon.h"
#include "debug.h"

#include "server/request.h"
#include "server.h"

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

/* The file object */
typedef struct
{
    K32OBJ    header;
} FILE_OBJECT;


/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256


/***********************************************************************
 *              FILE_ConvertOFMode
 *
 * Convert OF_* mode into flags for CreateFile.
 */
static void FILE_ConvertOFMode( INT32 mode, DWORD *access, DWORD *sharing )
{
    switch(mode & 0x03)
    {
    case OF_READ:      *access = GENERIC_READ; break;
    case OF_WRITE:     *access = GENERIC_WRITE; break;
    case OF_READWRITE: *access = GENERIC_READ | GENERIC_WRITE; break;
    default:           *access = 0; break;
    }
    switch(mode & 0x70)
    {
    case OF_SHARE_EXCLUSIVE:  *sharing = 0; break;
    case OF_SHARE_DENY_WRITE: *sharing = FILE_SHARE_READ; break;
    case OF_SHARE_DENY_READ:  *sharing = FILE_SHARE_WRITE; break;
    case OF_SHARE_DENY_NONE:
    case OF_SHARE_COMPAT:
    default:                  *sharing = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
    }
}


#if 0
/***********************************************************************
 *              FILE_ShareDeny
 *
 * PARAMS
 *       oldmode[I] mode how file was first opened
 *       mode[I] mode how the file should get opened
 * RETURNS
 *      TRUE: deny open
 *      FALSE: allow open
 *
 * Look what we have to do with the given SHARE modes
 *
 * Ralph Brown's interrupt list gives following explication, I guess
 * the same holds for Windows, DENY ALL should be OF_SHARE_COMPAT
 *
 * FIXME: Validate this function
========from Ralph Brown's list =========
(Table 0750)
Values of DOS file sharing behavior:
          |     Second and subsequent Opens
 First    |Compat  Deny   Deny   Deny   Deny
 Open     |        All    Write  Read   None
          |R W RW R W RW R W RW R W RW R W RW
 - - - - -| - - - - - - - - - - - - - - - - -
 Compat R |Y Y Y  N N N  1 N N  N N N  1 N N
        W |Y Y Y  N N N  N N N  N N N  N N N
        RW|Y Y Y  N N N  N N N  N N N  N N N
 - - - - -|
 Deny   R |C C C  N N N  N N N  N N N  N N N
 All    W |C C C  N N N  N N N  N N N  N N N
        RW|C C C  N N N  N N N  N N N  N N N
 - - - - -|
 Deny   R |2 C C  N N N  Y N N  N N N  Y N N
 Write  W |C C C  N N N  N N N  Y N N  Y N N
        RW|C C C  N N N  N N N  N N N  Y N N
 - - - - -|
 Deny   R |C C C  N N N  N Y N  N N N  N Y N
 Read   W |C C C  N N N  N N N  N Y N  N Y N
        RW|C C C  N N N  N N N  N N N  N Y N
 - - - - -|
 Deny   R |2 C C  N N N  Y Y Y  N N N  Y Y Y
 None   W |C C C  N N N  N N N  Y Y Y  Y Y Y
        RW|C C C  N N N  N N N  N N N  Y Y Y
Legend: Y = open succeeds, N = open fails with error code 05h
        C = open fails, INT 24 generated
        1 = open succeeds if file read-only, else fails with error code
        2 = open succeeds if file read-only, else fails with INT 24      
========end of description from Ralph Brown's List =====
	For every "Y" in the table we return FALSE
	For every "N" we set the DOS_ERROR and return TRUE 
	For all	other cases we barf,set the DOS_ERROR and return TRUE

 */
static BOOL32 FILE_ShareDeny( int mode, int oldmode)
{
  int oldsharemode = oldmode & 0x70;
  int sharemode    =    mode & 0x70;
  int oldopenmode  = oldmode & 3;
  int openmode     =    mode & 3;
  
  switch (oldsharemode)
    {
    case OF_SHARE_COMPAT:
      if (sharemode == OF_SHARE_COMPAT) return FALSE;
      if (openmode  == OF_READ) goto test_ro_err05 ;
      goto fail_error05;
    case OF_SHARE_EXCLUSIVE:
      if (sharemode == OF_SHARE_COMPAT) goto fail_int24;
      goto fail_error05;
    case OF_SHARE_DENY_WRITE:
      if (openmode  != OF_READ)
	{
	  if (sharemode == OF_SHARE_COMPAT) goto fail_int24;
	  goto fail_error05;
	}
      switch (sharemode)
	{
	case OF_SHARE_COMPAT:
	  if (oldopenmode == OF_READ) goto test_ro_int24 ;
	  goto fail_int24;
	case OF_SHARE_DENY_NONE : 
	  return FALSE;
	case OF_SHARE_DENY_WRITE :
	  if (oldopenmode == OF_READ) return FALSE;
	case OF_SHARE_DENY_READ :
	  if (oldopenmode == OF_WRITE) return FALSE;
	case OF_SHARE_EXCLUSIVE:
	default:
	  goto fail_error05;
	}
      break;
    case OF_SHARE_DENY_READ:
      if (openmode  != OF_WRITE)
	{
	  if (sharemode == OF_SHARE_COMPAT) goto fail_int24;
	  goto fail_error05;
	}
      switch (sharemode)
	{
	case OF_SHARE_COMPAT:
	  goto fail_int24;
	case OF_SHARE_DENY_NONE : 
	  return FALSE;
	case OF_SHARE_DENY_WRITE :
	  if (oldopenmode == OF_READ) return FALSE;
	case OF_SHARE_DENY_READ :
	  if (oldopenmode == OF_WRITE) return FALSE;
	case OF_SHARE_EXCLUSIVE:
	default:
	  goto fail_error05;
	}
      break;
    case OF_SHARE_DENY_NONE:
      switch (sharemode)
	{
	case OF_SHARE_COMPAT:
	  goto fail_int24;
	case OF_SHARE_DENY_NONE : 
	  return FALSE;
	case OF_SHARE_DENY_WRITE :
	  if (oldopenmode == OF_READ) return FALSE;
	case OF_SHARE_DENY_READ :
	  if (oldopenmode == OF_WRITE) return FALSE;
	case OF_SHARE_EXCLUSIVE:
	default:
	  goto fail_error05;
	}
    default:
      ERR(file,"unknown mode\n");
    }
  ERR(file,"shouldn't happen\n");
  ERR(file,"Please report to bon@elektron.ikp.physik.tu-darmstadt.de\n");
  return TRUE;
  
test_ro_int24:
  if (oldmode == OF_READ)
    return FALSE;
  /* Fall through */
fail_int24:
  FIXME(file,"generate INT24 missing\n");
  /* Is this the right error? */
  SetLastError( ERROR_ACCESS_DENIED );
  return TRUE;
  
test_ro_err05:
  if (oldmode == OF_READ)
    return FALSE;
  /* fall through */
fail_error05:
  TRACE(file,"Access Denied, oldmode 0x%02x mode 0x%02x\n",oldmode,mode);
  SetLastError( ERROR_ACCESS_DENIED );
  return TRUE;
}
#endif


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
        SetLastError( ERROR_SHARING_VIOLATION );
        break;
    case EBADF:
        SetLastError( ERROR_INVALID_HANDLE );
        break;
    case ENOSPC:
        SetLastError( ERROR_HANDLE_DISK_FULL );
        break;
    case EACCES:
    case EPERM:
    case EROFS:
        SetLastError( ERROR_ACCESS_DENIED );
        break;
    case EBUSY:
        SetLastError( ERROR_LOCK_VIOLATION );
        break;
    case ENOENT:
        SetLastError( ERROR_FILE_NOT_FOUND );
        break;
    case EISDIR:
        SetLastError( ERROR_CANNOT_MAKE );
        break;
    case ENFILE:
    case EMFILE:
        SetLastError( ERROR_NO_MORE_FILES );
        break;
    case EEXIST:
        SetLastError( ERROR_FILE_EXISTS );
        break;
    case EINVAL:
    case ESPIPE:
        SetLastError( ERROR_SEEK );
        break;
    case ENOTEMPTY:
        SetLastError( ERROR_DIR_NOT_EMPTY );
        break;
    default:
        perror( "int21: unknown errno" );
        SetLastError( ERROR_GEN_FAILURE );
        break;
    }
    errno = save_errno;
}


/***********************************************************************
 *           FILE_DupUnixHandle
 *
 * Duplicate a Unix handle into a task handle.
 */
HFILE32 FILE_DupUnixHandle( int fd, DWORD access )
{
    FILE_OBJECT *file;
    int unix_handle;
    struct create_file_request req;
    struct create_file_reply reply;

    if ((unix_handle = dup(fd)) == -1)
    {
        FILE_SetDosError();
        return INVALID_HANDLE_VALUE32;
    }
    req.access  = access;
    req.inherit = 1;
    req.sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    req.create  = 0;
    req.attrs   = 0;
    
    CLIENT_SendRequest( REQ_CREATE_FILE, unix_handle, 1,
                        &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return INVALID_HANDLE_VALUE32;

    if (!(file = HeapAlloc( SystemHeap, 0, sizeof(FILE_OBJECT) )))
    {
        CLIENT_CloseHandle( reply.handle );
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
        return (HFILE32)NULL;
    }
    file->header.type = K32OBJ_FILE;
    file->header.refcount = 0;
    return HANDLE_Alloc( PROCESS_Current(), &file->header, req.access,
                         req.inherit, reply.handle );
}


/***********************************************************************
 *           FILE_CreateFile
 *
 * Implementation of CreateFile. Takes a Unix path name.
 */
HFILE32 FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                         LPSECURITY_ATTRIBUTES sa, DWORD creation,
                         DWORD attributes, HANDLE32 template )
{
    FILE_OBJECT *file;
    struct create_file_request req;
    struct create_file_reply reply;

    req.access  = access;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    req.sharing = sharing;
    req.create  = creation;
    req.attrs   = attributes;
    CLIENT_SendRequest( REQ_CREATE_FILE, -1, 2,
                        &req, sizeof(req),
                        filename, strlen(filename) + 1 );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );

    /* If write access failed, retry without GENERIC_WRITE */

    if ((reply.handle == -1) && !Options.failReadOnly &&
        (access & GENERIC_WRITE)) 
    {
    	DWORD lasterror = GetLastError();
	if ((lasterror == ERROR_ACCESS_DENIED) || 
	    (lasterror == ERROR_WRITE_PROTECT))
        {
	    req.access &= ~GENERIC_WRITE;
	    CLIENT_SendRequest( REQ_CREATE_FILE, -1, 2,
				&req, sizeof(req),
				filename, strlen(filename) + 1 );
	    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
	}
    }
    if (reply.handle == -1) return INVALID_HANDLE_VALUE32;

    /* Now build the FILE_OBJECT */

    if (!(file = HeapAlloc( SystemHeap, 0, sizeof(FILE_OBJECT) )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        CLIENT_CloseHandle( reply.handle );
        return (HFILE32)INVALID_HANDLE_VALUE32;
    }
    file->header.type = K32OBJ_FILE;
    file->header.refcount = 0;
    return HANDLE_Alloc( PROCESS_Current(), &file->header, req.access,
                         req.inherit, reply.handle );
}


/***********************************************************************
 *           FILE_CreateDevice
 *
 * Same as FILE_CreateFile but for a device
 */
HFILE32 FILE_CreateDevice( int client_id, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    FILE_OBJECT *file;
    struct create_device_request req;
    struct create_device_reply reply;

    req.access  = access;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    req.id      = client_id;
    CLIENT_SendRequest( REQ_CREATE_DEVICE, -1, 1, &req, sizeof(req) );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return INVALID_HANDLE_VALUE32;

    /* Now build the FILE_OBJECT */

    if (!(file = HeapAlloc( SystemHeap, 0, sizeof(FILE_OBJECT) )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        CLIENT_CloseHandle( reply.handle );
        return (HFILE32)INVALID_HANDLE_VALUE32;
    }
    file->header.type = K32OBJ_FILE;
    file->header.refcount = 0;
    return HANDLE_Alloc( PROCESS_Current(), &file->header, req.access,
                         req.inherit, reply.handle );
}


/*************************************************************************
 * CreateFile32A [KERNEL32.45]  Creates or opens a file or other object
 *
 * Creates or opens an object, and returns a handle that can be used to
 * access that object.
 *
 * PARAMS
 *
 * filename     [I] pointer to filename to be accessed
 * access       [I] access mode requested
 * sharing      [I] share mode
 * sa           [I] pointer to security attributes
 * creation     [I] how to create the file
 * attributes   [I] attributes for newly created file
 * template     [I] handle to file with extended attributes to copy
 *
 * RETURNS
 *   Success: Open handle to specified file
 *   Failure: INVALID_HANDLE_VALUE
 *
 * NOTES
 *  Should call SetLastError() on failure.
 *
 * BUGS
 *
 * Doesn't support character devices, pipes, template files, or a
 * lot of the 'attributes' flags yet.
 */
HFILE32 WINAPI CreateFile32A( LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE32 template )
{
    DOS_FULL_NAME full_name;

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return HFILE_ERROR32;
    }

    /* If the name starts with '\\?\', ignore the first 4 chars. */
    if (!strncmp(filename, "\\\\?\\", 4))
    {
        filename += 4;
	if (!strncmp(filename, "UNC\\", 4))
	{
            FIXME( file, "UNC name (%s) not supported.\n", filename );
            SetLastError( ERROR_PATH_NOT_FOUND );
            return HFILE_ERROR32;
	}
    }

    if (!strncmp(filename, "\\\\.\\", 4))
        return DEVICE_Open( filename+4, access, sa );

    /* If the name still starts with '\\', it's a UNC name. */
    if (!strncmp(filename, "\\\\", 2))
    {
        FIXME( file, "UNC name (%s) not supported.\n", filename );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return HFILE_ERROR32;
    }

    /* Open a console for CONIN$ or CONOUT$ */
    if (!lstrcmpi32A(filename, "CONIN$")) return CONSOLE_OpenHandle( FALSE, access, sa );
    if (!lstrcmpi32A(filename, "CONOUT$")) return CONSOLE_OpenHandle( TRUE, access, sa );

    if (DOSFS_GetDevice( filename ))
    {
    	HFILE32	ret;

        TRACE(file, "opening device '%s'\n", filename );

	if (HFILE_ERROR32!=(ret=DOSFS_OpenDevice( filename, access )))
		return ret;

	/* Do not silence this please. It is a critical error. -MM */
        ERR(file, "Couldn't open device '%s'!\n",filename);
        SetLastError( ERROR_FILE_NOT_FOUND );
        return HFILE_ERROR32;
    }

    /* check for filename, don't check for last entry if creating */
    if (!DOSFS_GetFullName( filename,
               (creation == OPEN_EXISTING) || (creation == TRUNCATE_EXISTING), &full_name ))
        return HFILE_ERROR32;

    return FILE_CreateFile( full_name.long_name, access, sharing,
                            sa, creation, attributes, template );
}



/*************************************************************************
 *              CreateFile32W              (KERNEL32.48)
 */
HFILE32 WINAPI CreateFile32W( LPCWSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE32 template)
{
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    HFILE32 res = CreateFile32A( afn, access, sharing, sa, creation, attributes, template );
    HeapFree( GetProcessHeap(), 0, afn );
    return res;
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
    struct get_file_info_request req;
    struct get_file_info_reply reply;

    if (!info) return 0;
    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return 0;
    CLIENT_SendRequest( REQ_GET_FILE_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ))
        return 0;
    DOSFS_UnixTimeToFileTime( reply.write_time, &info->ftCreationTime, 0 );
    DOSFS_UnixTimeToFileTime( reply.write_time, &info->ftLastWriteTime, 0 );
    DOSFS_UnixTimeToFileTime( reply.access_time, &info->ftLastAccessTime, 0 );
    info->dwFileAttributes     = reply.attr;
    info->dwVolumeSerialNumber = reply.serial;
    info->nFileSizeHigh        = reply.size_high;
    info->nFileSizeLow         = reply.size_low;
    info->nNumberOfLinks       = reply.links;
    info->nFileIndexHigh       = reply.index_high;
    info->nFileIndexLow        = reply.index_low;
    return 1;
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

    if (name == NULL || *name=='\0') return -1;

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
    static UINT32 unique_temp;
    DOS_FULL_NAME full_name;
    int i;
    LPSTR p;
    UINT32 num;

    if ( !path || !prefix || !buffer ) return 0;

    if (!unique_temp) unique_temp = time(NULL) & 0xffff;
    num = unique ? (unique & 0xffff) : (unique_temp++ & 0xffff);

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
            HFILE32 handle = CreateFile32A( buffer, GENERIC_WRITE, 0, NULL,
                                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, -1 );
            if (handle != INVALID_HANDLE_VALUE32)
            {  /* We created it */
                TRACE(file, "created %s\n",
			      buffer);
                CloseHandle( handle );
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS)
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
    DWORD access, sharing;
    char *p;

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
    FILE_ConvertOFMode( mode, &access, &sharing );

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
        if ((hFileRet = CreateFile32A( name, GENERIC_READ | GENERIC_WRITE,
                                       sharing, NULL, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, -1 ))== INVALID_HANDLE_VALUE32)
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

    if (mode & OF_SHARE_EXCLUSIVE)
      /* Some InstallShield version uses OF_SHARE_EXCLUSIVE 
	 on the file <tempdir>/_ins0432._mp to determine how
	 far installation has proceeded.
	 _ins0432._mp is an executable and while running the
	 application expects the open with OF_SHARE_ to fail*/
      /* Probable FIXME:
	 As our loader closes the files after loading the executable,
	 we can't find the running executable with FILE_InUse.
	 Perhaps the loader should keep the file open.
	 Recheck against how Win handles that case */
      {
	char *last = strrchr(full_name.long_name,'/');
	if (!last)
	  last = full_name.long_name - 1;
	if (GetModuleHandle16(last+1))
	  {
	    TRACE(file,"Denying shared open for %s\n",full_name.long_name);
	    return HFILE_ERROR32;
	  }
      }

    if (mode & OF_DELETE)
    {
        if (unlink( full_name.long_name ) == -1) goto not_found;
        TRACE(file, "(%s): OF_DELETE return = OK\n", name);
        return 1;
    }

    hFileRet = FILE_CreateFile( full_name.long_name, access, sharing,
                                NULL, OPEN_EXISTING, 0, -1 );
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
            SetLastError( ERROR_FILE_NOT_FOUND );
            goto error;
        }
    }
    memcpy( ofs->reserved, filedatetime, sizeof(ofs->reserved) );

success:  /* We get here if the open was successful */
    TRACE(file, "(%s): OK, return = %d\n", name, hFileRet );
    if (win32)
    {
        if (mode & OF_EXIST) /* Return the handle, but close it first */
            CloseHandle( hFileRet );
    }
    else
    {
        hFileRet = FILE_AllocDosHandle( hFileRet );
        if (hFileRet == HFILE_ERROR16) goto error;
        if (mode & OF_EXIST) /* Return the handle, but close it first */
            _lclose16( hFileRet );
    }
    return hFileRet;

not_found:  /* We get here if the file does not exist */
    WARN(file, "'%s' not found\n", name );
    SetLastError( ERROR_FILE_NOT_FOUND );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = GetLastError();
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
 *           FILE_InitProcessDosHandles
 *
 * Allocates the default DOS handles for a process. Called either by
 * AllocDosHandle below or by the DOSVM stuff.
 */
BOOL32 FILE_InitProcessDosHandles( void ) {
	HANDLE32 *ptr;

        if (!(ptr = HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY,
                               sizeof(*ptr) * DOS_TABLE_SIZE )))
            return FALSE;
        PROCESS_Current()->dos_handles = ptr;
        ptr[0] = GetStdHandle(STD_INPUT_HANDLE);
        ptr[1] = GetStdHandle(STD_OUTPUT_HANDLE);
        ptr[2] = GetStdHandle(STD_ERROR_HANDLE);
        ptr[3] = GetStdHandle(STD_ERROR_HANDLE);
        ptr[4] = GetStdHandle(STD_ERROR_HANDLE);
	return TRUE;
}

/***********************************************************************
 *           FILE_AllocDosHandle
 *
 * Allocate a DOS handle for a Win32 handle. The Win32 handle is no
 * longer valid after this function (even on failure).
 */
HFILE16 FILE_AllocDosHandle( HANDLE32 handle )
{
    int i;
    HANDLE32 *ptr = PROCESS_Current()->dos_handles;

    if (!handle || (handle == INVALID_HANDLE_VALUE32))
        return INVALID_HANDLE_VALUE16;

    if (!ptr) {
    	if (!FILE_InitProcessDosHandles())
	    goto error;
	ptr = PROCESS_Current()->dos_handles;
    }

    for (i = 0; i < DOS_TABLE_SIZE; i++, ptr++)
        if (!*ptr)
        {
            *ptr = handle;
            TRACE( file, "Got %d for h32 %d\n", i, handle );
            return i;
        }
error:
    CloseHandle( handle );
    SetLastError( ERROR_TOO_MANY_OPEN_FILES );
    return INVALID_HANDLE_VALUE16;
}


/***********************************************************************
 *           FILE_GetHandle32
 *
 * Return the Win32 handle for a DOS handle.
 */
HANDLE32 FILE_GetHandle32( HFILE16 hfile )
{
    HANDLE32 *table = PROCESS_Current()->dos_handles;
    if ((hfile >= DOS_TABLE_SIZE) || !table || !table[hfile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return INVALID_HANDLE_VALUE32;
    }
    return table[hfile];
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE16 FILE_Dup2( HFILE16 hFile1, HFILE16 hFile2 )
{
    HANDLE32 *table = PROCESS_Current()->dos_handles;
    HANDLE32 new_handle;

    if ((hFile1 >= DOS_TABLE_SIZE) || (hFile2 >= DOS_TABLE_SIZE) ||
        !table || !table[hFile1])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if (hFile2 < 5)
    {
        FIXME( file, "stdio handle closed, need proper conversion\n" );
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if (!DuplicateHandle( GetCurrentProcess(), table[hFile1],
                          GetCurrentProcess(), &new_handle,
                          0, FALSE, DUPLICATE_SAME_ACCESS ))
        return HFILE_ERROR16;
    if (table[hFile2]) CloseHandle( table[hFile2] );
    table[hFile2] = new_handle;
    return hFile2;
}


/***********************************************************************
 *           _lclose16   (KERNEL.81)
 */
HFILE16 WINAPI _lclose16( HFILE16 hFile )
{
    HANDLE32 *table = PROCESS_Current()->dos_handles;

    if (hFile < 5)
    {
        FIXME( file, "stdio handle closed, need proper conversion\n" );
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if ((hFile >= DOS_TABLE_SIZE) || !table || !table[hFile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    TRACE( file, "%d (handle32=%d)\n", hFile, table[hFile] );
    CloseHandle( table[hFile] );
    table[hFile] = 0;
    return 0;
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
 *              ReadFile                (KERNEL32.428)
 */
BOOL32 WINAPI ReadFile( HANDLE32 hFile, LPVOID buffer, DWORD bytesToRead,
                        LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    struct get_read_fd_request req;
    int unix_handle, result;

    TRACE(file, "%d %p %ld\n", hFile, buffer, bytesToRead );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_UNKNOWN, GENERIC_READ )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_GET_READ_FD, -1, 1, &req, sizeof(req) );
    CLIENT_WaitReply( NULL, &unix_handle, 0 );
    if (unix_handle == -1) return FALSE;
    while ((result = read( unix_handle, buffer, bytesToRead )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        FILE_SetDosError();
        break;
    }
    close( unix_handle );
    if (result == -1) return FALSE;
    if (bytesRead) *bytesRead = result;
    return TRUE;
}


/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 */
BOOL32 WINAPI WriteFile( HANDLE32 hFile, LPCVOID buffer, DWORD bytesToWrite,
                         LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    struct get_write_fd_request req;
    int unix_handle, result;

    TRACE(file, "%d %p %ld\n", hFile, buffer, bytesToWrite );

    if (bytesWritten) *bytesWritten = 0;  /* Do this before anything else */
    if (!bytesToWrite) return TRUE;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_UNKNOWN, GENERIC_READ )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_GET_WRITE_FD, -1, 1, &req, sizeof(req) );
    CLIENT_WaitReply( NULL, &unix_handle, 0 );
    if (unix_handle == -1) return FALSE;
    while ((result = write( unix_handle, buffer, bytesToWrite )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        FILE_SetDosError();
        break;
    }
    close( unix_handle );
    if (result == -1) return FALSE;
    if (bytesWritten) *bytesWritten = result;
    return TRUE;
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
    return _lread32(FILE_GetHandle32(hFile), PTR_SEG_TO_LIN(buffer), count );
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
    DWORD result;
    if (!ReadFile( handle, buffer, count, &result, NULL )) return -1;
    return result;
}


/***********************************************************************
 *           _lread16   (KERNEL.82)
 */
UINT16 WINAPI _lread16( HFILE16 hFile, LPVOID buffer, UINT16 count )
{
    return (UINT16)_lread32(FILE_GetHandle32(hFile), buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat16   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    TRACE(file, "%s %02x\n", path, attr );
    return FILE_AllocDosHandle( _lcreat32( path, attr ) );
}


/***********************************************************************
 *           _lcreat32   (KERNEL32.593)
 */
HFILE32 WINAPI _lcreat32( LPCSTR path, INT32 attr )
{
    TRACE(file, "%s %02x\n", path, attr );
    return CreateFile32A( path, GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          CREATE_ALWAYS, attr, -1 );
}


/***********************************************************************
 *           _lcreat16_uniq   (Not a Windows API)
 */
HFILE16 _lcreat16_uniq( LPCSTR path, INT32 attr )
{
    TRACE(file, "%s %02x\n", path, attr );
    return FILE_AllocDosHandle( CreateFile32A( path, GENERIC_READ | GENERIC_WRITE,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                               CREATE_NEW, attr, -1 ));
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.492)
 */
DWORD WINAPI SetFilePointer( HFILE32 hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    struct set_file_pointer_request req;
    struct set_file_pointer_reply reply;

    if (highword && *highword)
    {
        FIXME(file, "64-bit offsets not supported yet\n");
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0xffffffff;
    }
    TRACE(file, "handle %d offset %ld origin %ld\n",
          hFile, distance, method );

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return 0xffffffff;
    req.low = distance;
    req.high = highword ? *highword : 0;
    /* FIXME: assumes 1:1 mapping between Windows and Unix seek constants */
    req.whence = method;
    CLIENT_SendRequest( REQ_SET_FILE_POINTER, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return 0xffffffff;
    SetLastError( 0 );
    if (highword) *highword = reply.high;
    return reply.low;
}


/***********************************************************************
 *           _llseek16   (KERNEL.84)
 *
 * FIXME:
 *   Seeking before the start of the file should be allowed for _llseek16,
 *   but cause subsequent I/O operations to fail (cf. interrupt list)
 *
 */
LONG WINAPI _llseek16( HFILE16 hFile, LONG lOffset, INT16 nOrigin )
{
    return SetFilePointer( FILE_GetHandle32(hFile), lOffset, NULL, nOrigin );
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
    return FILE_AllocDosHandle( _lopen32( path, mode ) );
}


/***********************************************************************
 *           _lopen32   (KERNEL32.595)
 */
HFILE32 WINAPI _lopen32( LPCSTR path, INT32 mode )
{
    DWORD access, sharing;

    TRACE(file, "('%s',%04x)\n", path, mode );
    FILE_ConvertOFMode( mode, &access, &sharing );
    return CreateFile32A( path, access, sharing, NULL, OPEN_EXISTING, 0, -1 );
}


/***********************************************************************
 *           _lwrite16   (KERNEL.86)
 */
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite32( FILE_GetHandle32(hFile), buffer, (LONG)count );
}

/***********************************************************************
 *           _lwrite32   (KERNEL32.761)
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
    return _lread32( FILE_GetHandle32(hFile), buffer, count );
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
    return _hwrite32( FILE_GetHandle32(hFile), buffer, count );
}


/***********************************************************************
 *           _hwrite32   (KERNEL32.591)
 *
 *	experimentation yields that _lwrite:
 *		o truncates the file at the current position with 
 *		  a 0 len write
 *		o returns 0 on a 0 length write
 *		o works with console handles
 *		
 */
LONG WINAPI _hwrite32( HFILE32 handle, LPCSTR buffer, LONG count )
{
    DWORD result;

    TRACE(file, "%d %p %ld\n", handle, buffer, count );

    if (!count)
    {
        /* Expand or truncate at current position */
        if (!SetEndOfFile( handle )) return HFILE_ERROR32;
        return 0;
    }
    if (!WriteFile( handle, buffer, count, &result, NULL ))
        return HFILE_ERROR32;
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
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
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
    struct flush_file_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_FLUSH_FILE, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/**************************************************************************
 *           SetEndOfFile   (KERNEL32.483)
 */
BOOL32 WINAPI SetEndOfFile( HFILE32 hFile )
{
    struct truncate_file_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return FALSE;
    CLIENT_SendRequest( REQ_TRUNCATE_FILE, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
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

    if (!*path)
    {
        ERR(file, "Empty path passed\n");
        return FALSE;
    }
    if (DOSFS_GetDevice( path ))
    {
        WARN(file, "cannot remove DOS device '%s'!\n", path);
        SetLastError( ERROR_FILE_NOT_FOUND );
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
 *           FILE_dommap
 */
LPVOID FILE_dommap( int unix_handle, LPVOID start,
                    DWORD size_high, DWORD size_low,
                    DWORD offset_high, DWORD offset_low,
                    int prot, int flags )
{
    int fd = -1;
    int pos;
    LPVOID ret;

    if (size_high || offset_high)
        FIXME(file, "offsets larger than 4Gb not supported\n");

    if (unix_handle == -1)
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
    else fd = unix_handle;

    if ((ret = mmap( start, size_low, prot,
                     flags, fd, offset_low )) != (LPVOID)-1)
        return ret;

    /* mmap() failed; if this is because the file offset is not    */
    /* page-aligned (EINVAL), or because the underlying filesystem */
    /* does not support mmap() (ENOEXEC), we do it by hand.        */

    if (unix_handle == -1) return ret;
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
    ret = FILE_dommap( -1, start, size_high, size_low, 0, 0,
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
    struct get_file_info_request req;
    struct get_file_info_reply reply;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_UNKNOWN, 0 )) == -1)
        return FILE_TYPE_UNKNOWN;
    CLIENT_SendRequest( REQ_GET_FILE_INFO, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ))
        return FILE_TYPE_UNKNOWN;
    return reply.type;
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
      {
	/* use copy, if allowed */
	if (!(flag & MOVEFILE_COPY_ALLOWED)) {
	  /* FIXME: Use right error code */
	  SetLastError( ERROR_FILE_EXISTS );
	  return FALSE;
	}
	else mode =1;
      }
      if (DOSFS_GetFullName( fn2, TRUE, &full_name2 )) 
	/* target exists, check if we may overwrite */
	if (!(flag & MOVEFILE_REPLACE_EXISTING)) {
	  /* FIXME: Use right error code */
	  SetLastError( ERROR_ACCESS_DENIED );
	  return FALSE;
	}
    }
    else /* fn2 == NULL means delete source */
      if (flag & MOVEFILE_DELAY_UNTIL_REBOOT) {
	if (flag & MOVEFILE_COPY_ALLOWED) {  
	  WARN(file, "Illegal flag\n");
	  SetLastError( ERROR_GEN_FAILURE );
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
	SetLastError( ERROR_GEN_FAILURE );
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
    if ((h2 = CreateFile32A( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             fail_if_exists ? CREATE_NEW : CREATE_ALWAYS,
                             info.dwFileAttributes, h1 )) == HFILE_ERROR32)
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


/**************************************************************************
 *           CopyFileEx32A   (KERNEL32.858)
 *
 * This implementation ignores most of the extra parameters passed-in into
 * the "ex" version of the method and calls the CopyFile method.
 * It will have to be fixed eventually.
 */
BOOL32 WINAPI CopyFileEx32A(LPCSTR             sourceFilename,
                           LPCSTR             destFilename,
                           LPPROGRESS_ROUTINE progressRoutine,
                           LPVOID             appData,
                           LPBOOL32           cancelFlagPointer,
                           DWORD              copyFlags)
{
  BOOL32 failIfExists = FALSE;

  /*
   * Interpret the only flag that CopyFile can interpret.
   */
  if ( (copyFlags & COPY_FILE_FAIL_IF_EXISTS) != 0)
  {
    failIfExists = TRUE;
  }

  return CopyFile32A(sourceFilename, destFilename, failIfExists);
}

/**************************************************************************
 *           CopyFileEx32W   (KERNEL32.859)
 */
BOOL32 WINAPI CopyFileEx32W(LPCWSTR            sourceFilename,
                           LPCWSTR            destFilename,
                           LPPROGRESS_ROUTINE progressRoutine,
                           LPVOID             appData,
                           LPBOOL32           cancelFlagPointer,
                           DWORD              copyFlags)
{
    LPSTR sourceA = HEAP_strdupWtoA( GetProcessHeap(), 0, sourceFilename );
    LPSTR destA   = HEAP_strdupWtoA( GetProcessHeap(), 0, destFilename );

    BOOL32 ret = CopyFileEx32A(sourceA,
                              destA,
                              progressRoutine,
                              appData,
                              cancelFlagPointer,
                              copyFlags);

    HeapFree( GetProcessHeap(), 0, sourceA );
    HeapFree( GetProcessHeap(), 0, destA );

    return ret;
}


/***********************************************************************
 *              SetFileTime   (KERNEL32.650)
 */
BOOL32 WINAPI SetFileTime( HFILE32 hFile,
                           const FILETIME *lpCreationTime,
                           const FILETIME *lpLastAccessTime,
                           const FILETIME *lpLastWriteTime )
{
    struct set_file_time_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, GENERIC_WRITE )) == -1)
        return FALSE;
    if (lpLastAccessTime)
	req.access_time = DOSFS_FileTimeToUnixTime(lpLastAccessTime, NULL);
    else
	req.access_time = 0; /* FIXME */
    if (lpLastWriteTime)
	req.write_time = DOSFS_FileTimeToUnixTime(lpLastWriteTime, NULL);
    else
	req.write_time = 0; /* FIXME */

    CLIENT_SendRequest( REQ_SET_FILE_TIME, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/**************************************************************************
 *           LockFile   (KERNEL32.511)
 */
BOOL32 WINAPI LockFile( HFILE32 hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                        DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh )
{
    struct lock_file_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return FALSE;
    req.offset_low  = dwFileOffsetLow;
    req.offset_high = dwFileOffsetHigh;
    req.count_low   = nNumberOfBytesToLockLow;
    req.count_high  = nNumberOfBytesToLockHigh;
    CLIENT_SendRequest( REQ_LOCK_FILE, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/**************************************************************************
 *           UnlockFile   (KERNEL32.703)
 */
BOOL32 WINAPI UnlockFile( HFILE32 hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                          DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh )
{
    struct unlock_file_request req;

    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), hFile,
                                              K32OBJ_FILE, 0 )) == -1)
        return FALSE;
    req.offset_low  = dwFileOffsetLow;
    req.offset_high = dwFileOffsetHigh;
    req.count_low   = nNumberOfBytesToUnlockLow;
    req.count_high  = nNumberOfBytesToUnlockHigh;
    CLIENT_SendRequest( REQ_UNLOCK_FILE, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


#if 0

struct DOS_FILE_LOCK {
  struct DOS_FILE_LOCK *	next;
  DWORD				base;
  DWORD				len;
  DWORD				processId;
  FILE_OBJECT *			dos_file;
/*  char *			unix_name;*/
};

typedef struct DOS_FILE_LOCK DOS_FILE_LOCK;

static DOS_FILE_LOCK *locks = NULL;
static void DOS_RemoveFileLocks(FILE_OBJECT *file);


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
#if 0
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
#endif

  curr = HeapAlloc( SystemHeap, 0, sizeof(DOS_FILE_LOCK) );
  curr->processId = GetCurrentProcessId();
  curr->base = f->l_start;
  curr->len = f->l_len;
/*  curr->unix_name = HEAP_strdupA( SystemHeap, 0, file->unix_name);*/
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
/*      HeapFree( SystemHeap, 0, rem->unix_name );*/
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
/*      HeapFree( SystemHeap, 0, rem->unix_name );*/
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

  if (!(file = FILE_GetFile(hFile,0,NULL))) return FALSE;

  /* shadow locks internally */
  if (!DOS_AddLock(file, &f)) {
    SetLastError( ERROR_LOCK_VIOLATION );
    return FALSE;
  }

  /* FIXME: Unix locking commented out for now, doesn't work with Excel */
#ifdef USE_UNIX_LOCKS
  if (fcntl(file->unix_handle, F_SETLK, &f) == -1) {
    if (errno == EACCES || errno == EAGAIN) {
      SetLastError( ERROR_LOCK_VIOLATION );
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

  if (!(file = FILE_GetFile(hFile,0,NULL))) return FALSE;

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
#endif

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
