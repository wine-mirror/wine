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

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winestring.h"
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
#include "wincon.h"
#include "debugtools.h"

#include "server.h"

DEFAULT_DEBUG_CHANNEL(file);

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256


/***********************************************************************
 *              FILE_ConvertOFMode
 *
 * Convert OF_* mode into flags for CreateFile.
 */
static void FILE_ConvertOFMode( INT mode, DWORD *access, DWORD *sharing )
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
static BOOL FILE_ShareDeny( int mode, int oldmode)
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
      ERR("unknown mode\n");
    }
  ERR("shouldn't happen\n");
  ERR("Please report to bon@elektron.ikp.physik.tu-darmstadt.de\n");
  return TRUE;
  
test_ro_int24:
  if (oldmode == OF_READ)
    return FALSE;
  /* Fall through */
fail_int24:
  FIXME("generate INT24 missing\n");
  /* Is this the right error? */
  SetLastError( ERROR_ACCESS_DENIED );
  return TRUE;
  
test_ro_err05:
  if (oldmode == OF_READ)
    return FALSE;
  /* fall through */
fail_error05:
  TRACE("Access Denied, oldmode 0x%02x mode 0x%02x\n",oldmode,mode);
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

    TRACE("errno = %d %s\n", errno, strerror(errno));
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
HFILE FILE_DupUnixHandle( int fd, DWORD access )
{
    int unix_handle;
    struct alloc_file_handle_request *req = get_req_buffer();

    if ((unix_handle = dup(fd)) == -1)
    {
        FILE_SetDosError();
        return INVALID_HANDLE_VALUE;
    }
    req->access  = access;
    server_call_fd( REQ_ALLOC_FILE_HANDLE, unix_handle, NULL );
    return req->handle;
}


/***********************************************************************
 *           FILE_CreateFile
 *
 * Implementation of CreateFile. Takes a Unix path name.
 */
HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                         LPSECURITY_ATTRIBUTES sa, DWORD creation,
                         DWORD attributes, HANDLE template )
{
    struct create_file_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    req->sharing = sharing;
    req->create  = creation;
    req->attrs   = attributes;
    lstrcpynA( req->name, filename, server_remaining(req->name) );
    SetLastError(0);
    server_call( REQ_CREATE_FILE );

    /* If write access failed, retry without GENERIC_WRITE */

    if ((req->handle == -1) && !Options.failReadOnly &&
        (access & GENERIC_WRITE)) 
    {
    	DWORD lasterror = GetLastError();
	if ((lasterror == ERROR_ACCESS_DENIED) || (lasterror == ERROR_WRITE_PROTECT))
            return FILE_CreateFile( filename, access & ~GENERIC_WRITE, sharing,
                                    sa, creation, attributes, template );
    }
    return req->handle;
}


/***********************************************************************
 *           FILE_CreateDevice
 *
 * Same as FILE_CreateFile but for a device
 */
HFILE FILE_CreateDevice( int client_id, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    struct create_device_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    req->id      = client_id;
    SetLastError(0);
    server_call( REQ_CREATE_DEVICE );
    return req->handle;
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
HANDLE WINAPI CreateFileA( LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    DOS_FULL_NAME full_name;

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return HFILE_ERROR;
    }
    TRACE("%s %s%s%s%s%s%s%s\n",filename,
	  ((access & GENERIC_READ)==GENERIC_READ)?"GENERIC_READ ":"",
	  ((access & GENERIC_WRITE)==GENERIC_WRITE)?"GENERIC_WRITE ":"",
	  (!access)?"QUERY_ACCESS ":"",
	  ((sharing & FILE_SHARE_READ)==FILE_SHARE_READ)?"FILE_SHARE_READ ":"",
	  ((sharing & FILE_SHARE_WRITE)==FILE_SHARE_WRITE)?"FILE_SHARE_WRITE ":"",
	  ((sharing & FILE_SHARE_DELETE)==FILE_SHARE_DELETE)?"FILE_SHARE_DELETE ":"",
	  (creation ==CREATE_NEW)?"CREATE_NEW":
	  (creation ==CREATE_ALWAYS)?"CREATE_ALWAYS ":
	  (creation ==OPEN_EXISTING)?"OPEN_EXISTING ":
	  (creation ==OPEN_ALWAYS)?"OPEN_ALWAYS ":
	  (creation ==TRUNCATE_EXISTING)?"TRUNCATE_EXISTING ":"");

    /* If the name starts with '\\?\', ignore the first 4 chars. */
    if (!strncmp(filename, "\\\\?\\", 4))
    {
        filename += 4;
	if (!strncmp(filename, "UNC\\", 4))
	{
            FIXME("UNC name (%s) not supported.\n", filename );
            SetLastError( ERROR_PATH_NOT_FOUND );
            return HFILE_ERROR;
	}
    }

    if (!strncmp(filename, "\\\\.\\", 4)) {
        if (!DOSFS_GetDevice( filename ))
        	return DEVICE_Open( filename+4, access, sa );
	else
        	filename+=4; /* fall into DOSFS_Device case below */
    }

    /* If the name still starts with '\\', it's a UNC name. */
    if (!strncmp(filename, "\\\\", 2))
    {
        FIXME("UNC name (%s) not supported.\n", filename );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return HFILE_ERROR;
    }

    /* If the name contains a DOS wild card (* or ?), do no create a file */
    if(strchr(filename,'*') || strchr(filename,'?'))
        return HFILE_ERROR;

    /* Open a console for CONIN$ or CONOUT$ */
    if (!lstrcmpiA(filename, "CONIN$")) return CONSOLE_OpenHandle( FALSE, access, sa );
    if (!lstrcmpiA(filename, "CONOUT$")) return CONSOLE_OpenHandle( TRUE, access, sa );

    if (DOSFS_GetDevice( filename ))
    {
    	HFILE	ret;

        TRACE("opening device '%s'\n", filename );

	if (HFILE_ERROR!=(ret=DOSFS_OpenDevice( filename, access )))
		return ret;

	/* Do not silence this please. It is a critical error. -MM */
        ERR("Couldn't open device '%s'!\n",filename);
        SetLastError( ERROR_FILE_NOT_FOUND );
        return HFILE_ERROR;
    }

    /* check for filename, don't check for last entry if creating */
    if (!DOSFS_GetFullName( filename,
               (creation == OPEN_EXISTING) || (creation == TRUNCATE_EXISTING), &full_name ))
        return HFILE_ERROR;

    return FILE_CreateFile( full_name.long_name, access, sharing,
                            sa, creation, attributes, template );
}



/*************************************************************************
 *              CreateFile32W              (KERNEL32.48)
 */
HANDLE WINAPI CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template)
{
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    HANDLE res = CreateFileA( afn, access, sharing, sa, creation, attributes, template );
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
BOOL FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info )
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
DWORD WINAPI GetFileInformationByHandle( HANDLE hFile,
                                         BY_HANDLE_FILE_INFORMATION *info )
{
    struct get_file_info_request *req = get_req_buffer();

    if (!info) return 0;
    req->handle = hFile;
    if (server_call( REQ_GET_FILE_INFO )) return 0;
    DOSFS_UnixTimeToFileTime( req->write_time, &info->ftCreationTime, 0 );
    DOSFS_UnixTimeToFileTime( req->write_time, &info->ftLastWriteTime, 0 );
    DOSFS_UnixTimeToFileTime( req->access_time, &info->ftLastAccessTime, 0 );
    info->dwFileAttributes     = req->attr;
    info->dwVolumeSerialNumber = req->serial;
    info->nFileSizeHigh        = req->size_high;
    info->nFileSizeLow         = req->size_low;
    info->nNumberOfLinks       = req->links;
    info->nFileIndexHigh       = req->index_high;
    info->nFileIndexLow        = req->index_low;
    return 1;
}


/**************************************************************************
 *           GetFileAttributes16   (KERNEL.420)
 */
DWORD WINAPI GetFileAttributes16( LPCSTR name )
{
    return GetFileAttributesA( name );
}


/**************************************************************************
 *           GetFileAttributes32A   (KERNEL32.217)
 */
DWORD WINAPI GetFileAttributesA( LPCSTR name )
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
DWORD WINAPI GetFileAttributesW( LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    DWORD res = GetFileAttributesA( nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.220)
 */
DWORD WINAPI GetFileSize( HANDLE hFile, LPDWORD filesizehigh )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return 0;
    if (filesizehigh) *filesizehigh = info.nFileSizeHigh;
    return info.nFileSizeLow;
}


/***********************************************************************
 *           GetFileTime   (KERNEL32.221)
 */
BOOL WINAPI GetFileTime( HANDLE hFile, FILETIME *lpCreationTime,
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
INT WINAPI CompareFileTime( LPFILETIME x, LPFILETIME y )
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
 *           FILE_GetTempFileName : utility for GetTempFileName
 */
static UINT FILE_GetTempFileName( LPCSTR path, LPCSTR prefix, UINT unique,
                                  LPSTR buffer, BOOL isWin16 )
{
    static UINT unique_temp;
    DOS_FULL_NAME full_name;
    int i;
    LPSTR p;
    UINT num;

    if ( !path || !prefix || !buffer ) return 0;

    if (!unique_temp) unique_temp = time(NULL) & 0xffff;
    num = unique ? (unique & 0xffff) : (unique_temp++ & 0xffff);

    strcpy( buffer, path );
    p = buffer + strlen(buffer);

    /* add a \, if there isn't one and path is more than just the drive letter ... */
    if ( !((strlen(buffer) == 2) && (buffer[1] == ':')) 
	&& ((p == buffer) || (p[-1] != '\\'))) *p++ = '\\';

    if (isWin16) *p++ = '~';
    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;
    sprintf( p, "%04x.tmp", num );

    /* Now try to create it */

    if (!unique)
    {
        do
        {
            HFILE handle = CreateFileA( buffer, GENERIC_WRITE, 0, NULL,
                                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, -1 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                TRACE("created %s\n",
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
            WARN("returns '%s', which doesn't seem to be writeable.\n",
		 buffer);
    }
    TRACE("returning %s\n", buffer );
    return unique ? unique : num;
}


/***********************************************************************
 *           GetTempFileName32A   (KERNEL32.290)
 */
UINT WINAPI GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique,
                                  LPSTR buffer)
{
    return FILE_GetTempFileName(path, prefix, unique, buffer, FALSE);
}

/***********************************************************************
 *           GetTempFileName32W   (KERNEL32.291)
 */
UINT WINAPI GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique,
                                  LPWSTR buffer )
{
    LPSTR   patha,prefixa;
    char    buffera[144];
    UINT  ret;

    if (!path) return 0;
    patha   = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    prefixa = HEAP_strdupWtoA( GetProcessHeap(), 0, prefix );
    ret     = FILE_GetTempFileName( patha, prefixa, unique, buffera, FALSE );
    lstrcpyAtoW( buffer, buffera );
    HeapFree( GetProcessHeap(), 0, patha );
    HeapFree( GetProcessHeap(), 0, prefixa );
    return ret;
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
        WARN("invalid drive %d specified\n", drive );
    }

    if (drive & TF_FORCEDRIVE)
        sprintf(temppath,"%c:", drive & ~TF_FORCEDRIVE );
    else
        GetTempPathA( 132, temppath );
    return (UINT16)FILE_GetTempFileName( temppath, prefix, unique, buffer, TRUE );
}

/***********************************************************************
 *           FILE_DoOpenFile
 *
 * Implementation of OpenFile16() and OpenFile32().
 */
static HFILE FILE_DoOpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode,
                                BOOL win32 )
{
    HFILE hFileRet;
    FILETIME filetime;
    WORD filedatetime[2];
    DOS_FULL_NAME full_name;
    DWORD access, sharing;
    char *p;

    if (!ofs) return HFILE_ERROR;
    
    TRACE("%s %s %s %s%s%s%s%s%s%s%s%s\n",name,
	  ((mode & 0x3 )==OF_READ)?"OF_READ":
	  ((mode & 0x3 )==OF_WRITE)?"OF_WRITE":
	  ((mode & 0x3 )==OF_READWRITE)?"OF_READWRITE":"unknown",
	  ((mode & 0x70 )==OF_SHARE_COMPAT)?"OF_SHARE_COMPAT":
	  ((mode & 0x70 )==OF_SHARE_DENY_NONE)?"OF_SHARE_DENY_NONE":
	  ((mode & 0x70 )==OF_SHARE_DENY_READ)?"OF_SHARE_DENY_READ":
	  ((mode & 0x70 )==OF_SHARE_DENY_WRITE)?"OF_SHARE_DENY_WRITE":
	  ((mode & 0x70 )==OF_SHARE_EXCLUSIVE)?"OF_SHARE_EXCLUSIVE":"unknown",
	  ((mode & OF_PARSE )==OF_PARSE)?"OF_PARSE ":"",
	  ((mode & OF_DELETE )==OF_DELETE)?"OF_DELETE ":"",
	  ((mode & OF_VERIFY )==OF_VERIFY)?"OF_VERIFY ":"",
	  ((mode & OF_SEARCH )==OF_SEARCH)?"OF_SEARCH ":"",
	  ((mode & OF_CANCEL )==OF_CANCEL)?"OF_CANCEL ":"",
	  ((mode & OF_CREATE )==OF_CREATE)?"OF_CREATE ":"",
	  ((mode & OF_PROMPT )==OF_PROMPT)?"OF_PROMPT ":"",
	  ((mode & OF_EXIST )==OF_EXIST)?"OF_EXIST ":"",
	  ((mode & OF_REOPEN )==OF_REOPEN)?"OF_REOPEN ":""
	  );
      

    ofs->cBytes = sizeof(OFSTRUCT);
    ofs->nErrCode = 0;
    if (mode & OF_REOPEN) name = ofs->szPathName;

    if (!name) {
	ERR("called with `name' set to NULL ! Please debug.\n");
	return HFILE_ERROR;
    }

    TRACE("%s %04x\n", name, mode );

    /* the watcom 10.6 IDE relies on a valid path returned in ofs->szPathName
       Are there any cases where getting the path here is wrong? 
       Uwe Bonnes 1997 Apr 2 */
    if (!GetFullPathNameA( name, sizeof(ofs->szPathName),
			     ofs->szPathName, NULL )) goto error;
    FILE_ConvertOFMode( mode, &access, &sharing );

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        ofs->fFixedDisk = (GetDriveType16( ofs->szPathName[0]-'A' )
                           != DRIVE_REMOVABLE);
        TRACE("(%s): OF_PARSE, res = '%s'\n",
                      name, ofs->szPathName );
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if ((hFileRet = CreateFileA( name, GENERIC_READ | GENERIC_WRITE,
                                       sharing, NULL, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, -1 ))== INVALID_HANDLE_VALUE)
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
    TRACE("found %s = %s\n",
                  full_name.long_name, full_name.short_name );
    lstrcpynA( ofs->szPathName, full_name.short_name,
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
	    TRACE("Denying shared open for %s\n",full_name.long_name);
	    return HFILE_ERROR;
	  }
      }

    if (mode & OF_DELETE)
    {
        if (unlink( full_name.long_name ) == -1) goto not_found;
        TRACE("(%s): OF_DELETE return = OK\n", name);
        return 1;
    }

    hFileRet = FILE_CreateFile( full_name.long_name, access, sharing,
                                NULL, OPEN_EXISTING, 0, -1 );
    if (hFileRet == HFILE_ERROR) goto not_found;

    GetFileTime( hFileRet, NULL, NULL, &filetime );
    FileTimeToDosDateTime( &filetime, &filedatetime[0], &filedatetime[1] );
    if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
    {
        if (memcmp( ofs->reserved, filedatetime, sizeof(ofs->reserved) ))
        {
            CloseHandle( hFileRet );
            WARN("(%s): OF_VERIFY failed\n", name );
            /* FIXME: what error here? */
            SetLastError( ERROR_FILE_NOT_FOUND );
            goto error;
        }
    }
    memcpy( ofs->reserved, filedatetime, sizeof(ofs->reserved) );

success:  /* We get here if the open was successful */
    TRACE("(%s): OK, return = %d\n", name, hFileRet );
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
    WARN("'%s' not found\n", name );
    SetLastError( ERROR_FILE_NOT_FOUND );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = GetLastError();
    WARN("(%s): return = HFILE_ERROR error= %d\n", 
		  name,ofs->nErrCode );
    return HFILE_ERROR;
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
HFILE WINAPI OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    return FILE_DoOpenFile( name, ofs, mode, TRUE );
}


/***********************************************************************
 *           FILE_InitProcessDosHandles
 *
 * Allocates the default DOS handles for a process. Called either by
 * AllocDosHandle below or by the DOSVM stuff.
 */
BOOL FILE_InitProcessDosHandles( void ) {
	HANDLE *ptr;

        if (!(ptr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
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
HFILE16 FILE_AllocDosHandle( HANDLE handle )
{
    int i;
    HANDLE *ptr = PROCESS_Current()->dos_handles;

    if (!handle || (handle == INVALID_HANDLE_VALUE))
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
            TRACE("Got %d for h32 %d\n", i, handle );
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
HANDLE FILE_GetHandle( HFILE16 hfile )
{
    HANDLE *table = PROCESS_Current()->dos_handles;
    if ((hfile >= DOS_TABLE_SIZE) || !table || !table[hfile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return INVALID_HANDLE_VALUE;
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
    HANDLE *table = PROCESS_Current()->dos_handles;
    HANDLE new_handle;

    if ((hFile1 >= DOS_TABLE_SIZE) || (hFile2 >= DOS_TABLE_SIZE) ||
        !table || !table[hFile1])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if (hFile2 < 5)
    {
        FIXME("stdio handle closed, need proper conversion\n" );
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
    HANDLE *table = PROCESS_Current()->dos_handles;

    if (hFile < 5)
    {
        FIXME("stdio handle closed, need proper conversion\n" );
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if ((hFile >= DOS_TABLE_SIZE) || !table || !table[hFile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    TRACE("%d (handle32=%d)\n", hFile, table[hFile] );
    CloseHandle( table[hFile] );
    table[hFile] = 0;
    return 0;
}


/***********************************************************************
 *           _lclose32   (KERNEL32.592)
 */
HFILE WINAPI _lclose( HFILE hFile )
{
    TRACE("handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR;
}

/***********************************************************************
 *              GetOverlappedResult     (KERNEL32.360)
 */
BOOL WINAPI GetOverlappedResult(HANDLE hFile,LPOVERLAPPED lpOverlapped,
                                LPDWORD lpNumberOfBytesTransferred,
                                BOOL bWait)
{
    /* Since all i/o is currently synchronuos,
     * return true, assuming ReadFile/WriteFile
     * have completed the operation */
    FIXME("NO Asynch I/O, assuming Read/Write succeeded\n" );
    return TRUE;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.428)
 */
BOOL WINAPI ReadFile( HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                        LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    struct get_read_fd_request *req = get_req_buffer();
    int unix_handle, result;

    TRACE("%d %p %ld\n", hFile, buffer, bytesToRead );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    if ( overlapped ) {
      SetLastError ( ERROR_INVALID_PARAMETER );
      return FALSE;
    }

    req->handle = hFile;
    server_call_fd( REQ_GET_READ_FD, -1, &unix_handle );
    if (unix_handle == -1) return FALSE;
    while ((result = read( unix_handle, buffer, bytesToRead )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        if ((errno == EFAULT) && VIRTUAL_HandleFault( buffer )) continue;
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
BOOL WINAPI WriteFile( HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                         LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    struct get_write_fd_request *req = get_req_buffer();
    int unix_handle, result;

    TRACE("%d %p %ld\n", hFile, buffer, bytesToWrite );

    if (bytesWritten) *bytesWritten = 0;  /* Do this before anything else */
    if (!bytesToWrite) return TRUE;

    if ( overlapped ) {
      SetLastError ( ERROR_INVALID_PARAMETER );
      return FALSE;
    }

    req->handle = hFile;
    server_call_fd( REQ_GET_WRITE_FD, -1, &unix_handle );
    if (unix_handle == -1) return FALSE;
    while ((result = write( unix_handle, buffer, bytesToWrite )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        if ((errno == EFAULT) && VIRTUAL_HandleFault( buffer )) continue;
        if (errno == ENOSPC)
            SetLastError( ERROR_DISK_FULL );
        else
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

    TRACE("%d %08lx %ld\n",
                  hFile, (DWORD)buffer, count );

    /* Some programs pass a count larger than the allocated buffer */
    maxlen = GetSelectorLimit16( SELECTOROF(buffer) ) - OFFSETOF(buffer) + 1;
    if (count > maxlen) count = maxlen;
    return _lread(FILE_GetHandle(hFile), PTR_SEG_TO_LIN(buffer), count );
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
UINT WINAPI _lread( HFILE handle, LPVOID buffer, UINT count )
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
    return (UINT16)_lread(FILE_GetHandle(hFile), buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat16   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    return FILE_AllocDosHandle( _lcreat( path, attr ) );
}


/***********************************************************************
 *           _lcreat   (KERNEL32.593)
 */
HFILE WINAPI _lcreat( LPCSTR path, INT attr )
{
    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    TRACE("%s %02x\n", path, attr );
    return CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, attr, -1 );
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.492)
 */
DWORD WINAPI SetFilePointer( HANDLE hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    struct set_file_pointer_request *req = get_req_buffer();

    if (highword && *highword)
    {
        FIXME("64-bit offsets not supported yet\n");
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0xffffffff;
    }
    TRACE("handle %d offset %ld origin %ld\n",
          hFile, distance, method );

    req->handle = hFile;
    req->low = distance;
    req->high = highword ? *highword : 0;
    /* FIXME: assumes 1:1 mapping between Windows and Unix seek constants */
    req->whence = method;
    SetLastError( 0 );
    if (server_call( REQ_SET_FILE_POINTER )) return 0xffffffff;
    if (highword) *highword = req->new_high;
    return req->new_low;
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
    return SetFilePointer( FILE_GetHandle(hFile), lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _llseek32   (KERNEL32.594)
 */
LONG WINAPI _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    return SetFilePointer( hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lopen16   (KERNEL.85)
 */
HFILE16 WINAPI _lopen16( LPCSTR path, INT16 mode )
{
    return FILE_AllocDosHandle( _lopen( path, mode ) );
}


/***********************************************************************
 *           _lopen32   (KERNEL32.595)
 */
HFILE WINAPI _lopen( LPCSTR path, INT mode )
{
    DWORD access, sharing;

    TRACE("('%s',%04x)\n", path, mode );
    FILE_ConvertOFMode( mode, &access, &sharing );
    return CreateFileA( path, access, sharing, NULL, OPEN_EXISTING, 0, -1 );
}


/***********************************************************************
 *           _lwrite16   (KERNEL.86)
 */
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite( FILE_GetHandle(hFile), buffer, (LONG)count );
}

/***********************************************************************
 *           _lwrite32   (KERNEL32.761)
 */
UINT WINAPI _lwrite( HFILE hFile, LPCSTR buffer, UINT count )
{
    return (UINT)_hwrite( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _hread16   (KERNEL.349)
 */
LONG WINAPI _hread16( HFILE16 hFile, LPVOID buffer, LONG count)
{
    return _lread( FILE_GetHandle(hFile), buffer, count );
}


/***********************************************************************
 *           _hread32   (KERNEL32.590)
 */
LONG WINAPI _hread( HFILE hFile, LPVOID buffer, LONG count)
{
    return _lread( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite16   (KERNEL.350)
 */
LONG WINAPI _hwrite16( HFILE16 hFile, LPCSTR buffer, LONG count )
{
    return _hwrite( FILE_GetHandle(hFile), buffer, count );
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
LONG WINAPI _hwrite( HFILE handle, LPCSTR buffer, LONG count )
{
    DWORD result;

    TRACE("%d %p %ld\n", handle, buffer, count );

    if (!count)
    {
        /* Expand or truncate at current position */
        if (!SetEndOfFile( handle )) return HFILE_ERROR;
        return 0;
    }
    if (!WriteFile( handle, buffer, count, &result, NULL ))
        return HFILE_ERROR;
    return result;
}


/***********************************************************************
 *           SetHandleCount16   (KERNEL.199)
 */
UINT16 WINAPI SetHandleCount16( UINT16 count )
{
    HGLOBAL16 hPDB = GetCurrentPDB16();
    PDB16 *pdb = (PDB16 *)GlobalLock16( hPDB );
    BYTE *files = PTR_SEG_TO_LIN( pdb->fileHandlesPtr );

    TRACE("(%d)\n", count );

    if (count < 20) count = 20;  /* No point in going below 20 */
    else if (count > 254) count = 254;

    if (count == 20)
    {
        if (pdb->nbFiles > 20)
        {
            memcpy( pdb->fileHandles, files, 20 );
            GlobalFree16( pdb->hFileHandles );
            pdb->fileHandlesPtr = (SEGPTR)MAKELONG( 0x18,
                                                   GlobalHandleToSel16( hPDB ) );
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
UINT WINAPI SetHandleCount( UINT count )
{
    return MIN( 256, count );
}


/***********************************************************************
 *           FlushFileBuffers   (KERNEL32.133)
 */
BOOL WINAPI FlushFileBuffers( HANDLE hFile )
{
    struct flush_file_request *req = get_req_buffer();
    req->handle = hFile;
    return !server_call( REQ_FLUSH_FILE );
}


/**************************************************************************
 *           SetEndOfFile   (KERNEL32.483)
 */
BOOL WINAPI SetEndOfFile( HANDLE hFile )
{
    struct truncate_file_request *req = get_req_buffer();
    req->handle = hFile;
    return !server_call( REQ_TRUNCATE_FILE );
}


/***********************************************************************
 *           DeleteFile16   (KERNEL.146)
 */
BOOL16 WINAPI DeleteFile16( LPCSTR path )
{
    return DeleteFileA( path );
}


/***********************************************************************
 *           DeleteFile32A   (KERNEL32.71)
 */
BOOL WINAPI DeleteFileA( LPCSTR path )
{
    DOS_FULL_NAME full_name;

    TRACE("'%s'\n", path );

    if (!*path)
    {
        ERR("Empty path passed\n");
        return FALSE;
    }
    if (DOSFS_GetDevice( path ))
    {
        WARN("cannot remove DOS device '%s'!\n", path);
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
BOOL WINAPI DeleteFileW( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL ret = DeleteFileA( xpath );
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
        FIXME("offsets larger than 4Gb not supported\n");

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
      FIXME("offsets larger than 4Gb not supported\n");
    return munmap( start, size_low );
}


/***********************************************************************
 *           GetFileType   (KERNEL32.222)
 */
DWORD WINAPI GetFileType( HANDLE hFile )
{
    struct get_file_info_request *req = get_req_buffer();
    req->handle = hFile;
    if (server_call( REQ_GET_FILE_INFO )) return FILE_TYPE_UNKNOWN;
    return req->type;
}


/**************************************************************************
 *           MoveFileExA   (KERNEL32.???)
 */
BOOL WINAPI MoveFileExA( LPCSTR fn1, LPCSTR fn2, DWORD flag )
{
    DOS_FULL_NAME full_name1, full_name2;

    TRACE("(%s,%s,%04lx)\n", fn1, fn2, flag);

    if (!DOSFS_GetFullName( fn1, TRUE, &full_name1 )) return FALSE;

    if (fn2)  /* !fn2 means delete fn1 */
    {
        if (DOSFS_GetFullName( fn2, TRUE, &full_name2 )) 
        {
            /* target exists, check if we may overwrite */
            if (!(flag & MOVEFILE_REPLACE_EXISTING))
            {
                /* FIXME: Use right error code */
                SetLastError( ERROR_ACCESS_DENIED );
                return FALSE;
            }
        }
        else if (!DOSFS_GetFullName( fn2, FALSE, &full_name2 )) return FALSE;

        /* Source name and target path are valid */

        if (flag & MOVEFILE_DELAY_UNTIL_REBOOT)
        {
            /* FIXME: (bon@elektron.ikp.physik.th-darmstadt.de 970706)
               Perhaps we should queue these command and execute it 
               when exiting... What about using on_exit(2)
            */
            FIXME("Please move existing file '%s' to file '%s' when Wine has finished\n",
                  full_name1.long_name, full_name2.long_name);
            return TRUE;
        }

        if (full_name1.drive != full_name2.drive)
        {
            /* use copy, if allowed */
            if (!(flag & MOVEFILE_COPY_ALLOWED))
            {
                /* FIXME: Use right error code */
                SetLastError( ERROR_FILE_EXISTS );
                return FALSE;
            }
            return CopyFileA( fn1, fn2, !(flag & MOVEFILE_REPLACE_EXISTING) );
        }
        if (rename( full_name1.long_name, full_name2.long_name ) == -1)
	{
            FILE_SetDosError();
            return FALSE;
	}
        return TRUE;
    }
    else /* fn2 == NULL means delete source */
    {
        if (flag & MOVEFILE_DELAY_UNTIL_REBOOT)
        {
            if (flag & MOVEFILE_COPY_ALLOWED) {  
                WARN("Illegal flag\n");
                SetLastError( ERROR_GEN_FAILURE );
                return FALSE;
            }
            /* FIXME: (bon@elektron.ikp.physik.th-darmstadt.de 970706)
               Perhaps we should queue these command and execute it 
               when exiting... What about using on_exit(2)
            */
            FIXME("Please delete file '%s' when Wine has finished\n",
                  full_name1.long_name);
            return TRUE;
        }

        if (unlink( full_name1.long_name ) == -1)
        {
            FILE_SetDosError();
            return FALSE;
        }
        return TRUE; /* successfully deleted */
    }
}

/**************************************************************************
 *           MoveFileEx32W   (KERNEL32.???)
 */
BOOL WINAPI MoveFileExW( LPCWSTR fn1, LPCWSTR fn2, DWORD flag )
{
    LPSTR afn1 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn1 );
    LPSTR afn2 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn2 );
    BOOL res = MoveFileExA( afn1, afn2, flag );
    HeapFree( GetProcessHeap(), 0, afn1 );
    HeapFree( GetProcessHeap(), 0, afn2 );
    return res;
}


/**************************************************************************
 *           MoveFile32A   (KERNEL32.387)
 *
 *  Move file or directory
 */
BOOL WINAPI MoveFileA( LPCSTR fn1, LPCSTR fn2 )
{
    DOS_FULL_NAME full_name1, full_name2;
    struct stat fstat;

    TRACE("(%s,%s)\n", fn1, fn2 );

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
	  WARN("Invalid source file %s\n",
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
	return CopyFileA(fn1, fn2, TRUE); /*fail, if exist */ 
    }
}


/**************************************************************************
 *           MoveFile32W   (KERNEL32.390)
 */
BOOL WINAPI MoveFileW( LPCWSTR fn1, LPCWSTR fn2 )
{
    LPSTR afn1 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn1 );
    LPSTR afn2 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn2 );
    BOOL res = MoveFileA( afn1, afn2 );
    HeapFree( GetProcessHeap(), 0, afn1 );
    HeapFree( GetProcessHeap(), 0, afn2 );
    return res;
}


/**************************************************************************
 *           CopyFile32A   (KERNEL32.36)
 */
BOOL WINAPI CopyFileA( LPCSTR source, LPCSTR dest, BOOL fail_if_exists )
{
    HFILE h1, h2;
    BY_HANDLE_FILE_INFORMATION info;
    UINT count;
    BOOL ret = FALSE;
    int mode;
    char buffer[2048];

    if ((h1 = _lopen( source, OF_READ )) == HFILE_ERROR) return FALSE;
    if (!GetFileInformationByHandle( h1, &info ))
    {
        CloseHandle( h1 );
        return FALSE;
    }
    mode = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666;
    if ((h2 = CreateFileA( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             fail_if_exists ? CREATE_NEW : CREATE_ALWAYS,
                             info.dwFileAttributes, h1 )) == HFILE_ERROR)
    {
        CloseHandle( h1 );
        return FALSE;
    }
    while ((count = _lread( h1, buffer, sizeof(buffer) )) > 0)
    {
        char *p = buffer;
        while (count > 0)
        {
            INT res = _lwrite( h2, p, count );
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
BOOL WINAPI CopyFileW( LPCWSTR source, LPCWSTR dest, BOOL fail_if_exists)
{
    LPSTR sourceA = HEAP_strdupWtoA( GetProcessHeap(), 0, source );
    LPSTR destA   = HEAP_strdupWtoA( GetProcessHeap(), 0, dest );
    BOOL ret = CopyFileA( sourceA, destA, fail_if_exists );
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
BOOL WINAPI CopyFileExA(LPCSTR             sourceFilename,
                           LPCSTR             destFilename,
                           LPPROGRESS_ROUTINE progressRoutine,
                           LPVOID             appData,
                           LPBOOL           cancelFlagPointer,
                           DWORD              copyFlags)
{
  BOOL failIfExists = FALSE;

  /*
   * Interpret the only flag that CopyFile can interpret.
   */
  if ( (copyFlags & COPY_FILE_FAIL_IF_EXISTS) != 0)
  {
    failIfExists = TRUE;
  }

  return CopyFileA(sourceFilename, destFilename, failIfExists);
}

/**************************************************************************
 *           CopyFileEx32W   (KERNEL32.859)
 */
BOOL WINAPI CopyFileExW(LPCWSTR            sourceFilename,
                           LPCWSTR            destFilename,
                           LPPROGRESS_ROUTINE progressRoutine,
                           LPVOID             appData,
                           LPBOOL           cancelFlagPointer,
                           DWORD              copyFlags)
{
    LPSTR sourceA = HEAP_strdupWtoA( GetProcessHeap(), 0, sourceFilename );
    LPSTR destA   = HEAP_strdupWtoA( GetProcessHeap(), 0, destFilename );

    BOOL ret = CopyFileExA(sourceA,
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
BOOL WINAPI SetFileTime( HANDLE hFile,
                           const FILETIME *lpCreationTime,
                           const FILETIME *lpLastAccessTime,
                           const FILETIME *lpLastWriteTime )
{
    struct set_file_time_request *req = get_req_buffer();

    req->handle = hFile;
    if (lpLastAccessTime)
	req->access_time = DOSFS_FileTimeToUnixTime(lpLastAccessTime, NULL);
    else
	req->access_time = 0; /* FIXME */
    if (lpLastWriteTime)
	req->write_time = DOSFS_FileTimeToUnixTime(lpLastWriteTime, NULL);
    else
	req->write_time = 0; /* FIXME */
    return !server_call( REQ_SET_FILE_TIME );
}


/**************************************************************************
 *           LockFile   (KERNEL32.511)
 */
BOOL WINAPI LockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                        DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh )
{
    struct lock_file_request *req = get_req_buffer();

    req->handle      = hFile;
    req->offset_low  = dwFileOffsetLow;
    req->offset_high = dwFileOffsetHigh;
    req->count_low   = nNumberOfBytesToLockLow;
    req->count_high  = nNumberOfBytesToLockHigh;
    return !server_call( REQ_LOCK_FILE );
}

/**************************************************************************
 * LockFileEx [KERNEL32.512]
 *
 * Locks a byte range within an open file for shared or exclusive access.
 *
 * RETURNS
 *   success: TRUE
 *   failure: FALSE
 * NOTES
 *
 * Per Microsoft docs, the third parameter (reserved) must be set to 0.
 */
BOOL WINAPI LockFileEx( HANDLE hFile, DWORD flags, DWORD reserved,
		      DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh,
		      LPOVERLAPPED pOverlapped )
{
    FIXME("hFile=%d,flags=%ld,reserved=%ld,lowbytes=%ld,highbytes=%ld,overlapped=%p: stub.\n",
	  hFile, flags, reserved, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh,
	  pOverlapped);
    if (reserved == 0)
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else
    {
	ERR("reserved == %ld: Supposed to be 0??\n", reserved);
	SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}


/**************************************************************************
 *           UnlockFile   (KERNEL32.703)
 */
BOOL WINAPI UnlockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                          DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh )
{
    struct unlock_file_request *req = get_req_buffer();

    req->handle      = hFile;
    req->offset_low  = dwFileOffsetLow;
    req->offset_high = dwFileOffsetHigh;
    req->count_low   = nNumberOfBytesToUnlockLow;
    req->count_high  = nNumberOfBytesToUnlockHigh;
    return !server_call( REQ_UNLOCK_FILE );
}


/**************************************************************************
 *           UnlockFileEx   (KERNEL32.705)
 */
BOOL WINAPI UnlockFileEx(
		HFILE hFile,
		DWORD dwReserved,
		DWORD nNumberOfBytesToUnlockLow,
		DWORD nNumberOfBytesToUnlockHigh,
		LPOVERLAPPED lpOverlapped
)
{
	FIXME("hFile=%d,reserved=%ld,lowbytes=%ld,highbytes=%ld,overlapped=%p: stub.\n",
	  hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh,
	  lpOverlapped);
	if (dwReserved == 0)
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	else
	{
		ERR("reserved == %ld: Supposed to be 0??\n", dwReserved);
		SetLastError(ERROR_INVALID_PARAMETER);
	}

	return FALSE;
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

static BOOL DOS_AddLock(FILE_OBJECT *file, struct flock *f)
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

  curr = HeapAlloc( GetProcessHeap(), 0, sizeof(DOS_FILE_LOCK) );
  curr->processId = GetCurrentProcessId();
  curr->base = f->l_start;
  curr->len = f->l_len;
/*  curr->unix_name = HEAP_strdupA( GetProcessHeap(), 0, file->unix_name);*/
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
/*      HeapFree( GetProcessHeap(), 0, rem->unix_name );*/
      HeapFree( GetProcessHeap(), 0, rem );
    }
    else
      curr = &(*curr)->next;
  }
}

static BOOL DOS_RemoveLock(FILE_OBJECT *file, struct flock *f)
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
/*      HeapFree( GetProcessHeap(), 0, rem->unix_name );*/
      HeapFree( GetProcessHeap(), 0, rem );
      return TRUE;
    }
  }
  /* no matching lock found */
  return FALSE;
}


/**************************************************************************
 *           LockFile   (KERNEL32.511)
 */
BOOL WINAPI LockFile(
	HFILE hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToLockLow,DWORD nNumberOfBytesToLockHigh )
{
  struct flock f;
  FILE_OBJECT *file;

  TRACE("handle %d offsetlow=%ld offsethigh=%ld nbyteslow=%ld nbyteshigh=%ld\n",
	       hFile, dwFileOffsetLow, dwFileOffsetHigh,
	       nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh);

  if (dwFileOffsetHigh || nNumberOfBytesToLockHigh) {
    FIXME("Unimplemented bytes > 32bits\n");
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
BOOL WINAPI UnlockFile(
	HFILE hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToUnlockLow,DWORD nNumberOfBytesToUnlockHigh )
{
  FILE_OBJECT *file;
  struct flock f;

  TRACE("handle %d offsetlow=%ld offsethigh=%ld nbyteslow=%ld nbyteshigh=%ld\n",
	       hFile, dwFileOffsetLow, dwFileOffsetHigh,
	       nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);

  if (dwFileOffsetHigh || nNumberOfBytesToUnlockHigh) {
    WARN("Unimplemented bytes > 32bits\n");
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
BOOL WINAPI GetFileAttributesExA(
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
	FIXME("invalid info level %d!\n", fInfoLevelId);
	return FALSE;
    }

    return TRUE;
}


/**************************************************************************
 * GetFileAttributesEx32W [KERNEL32.875]
 */
BOOL WINAPI GetFileAttributesExW(
	LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFileName );
    BOOL res = 
	GetFileAttributesExA( nameA, fInfoLevelId, lpFileInformation);
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}
