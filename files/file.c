/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 *
 * TODO:
 *    Fix the CopyFileEx methods to implement the "extended" functionality.
 *    Right now, they simply call the CopyFile method.
 */

#include "config.h"
#include "wine/port.h"

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
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "wincon.h"
#include "debugtools.h"

#include "server.h"

DEFAULT_DEBUG_CHANNEL(file);

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256

static HANDLE dos_handles[DOS_TABLE_SIZE];


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


/***********************************************************************
 *              FILE_strcasecmp
 *
 * locale-independent case conversion for file I/O
 */
int FILE_strcasecmp( const char *str1, const char *str2 )
{
    for (;;)
    {
        int ret = FILE_toupper(*str1) - FILE_toupper(*str2);
        if (ret || !*str1) return ret;
        str1++;
        str2++;
    }
}


/***********************************************************************
 *              FILE_strncasecmp
 *
 * locale-independent case conversion for file I/O
 */
int FILE_strncasecmp( const char *str1, const char *str2, int len )
{
    int ret = 0;
    for ( ; len > 0; len--, str1++, str2++)
        if ((ret = FILE_toupper(*str1) - FILE_toupper(*str2)) || !*str1) break;
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
    case ENOEXEC:
        SetLastError( ERROR_BAD_FORMAT );
        break;
    default:
        WARN("unknown file error: %s\n", strerror(save_errno) );
        SetLastError( ERROR_GEN_FAILURE );
        break;
    }
    errno = save_errno;
}


/***********************************************************************
 *           FILE_DupUnixHandle
 *
 * Duplicate a Unix handle into a task handle.
 * Returns 0 on failure.
 */
HANDLE FILE_DupUnixHandle( int fd, DWORD access )
{
    HANDLE ret;

    wine_server_send_fd( fd );

    SERVER_START_REQ( alloc_file_handle )
    {
        req->access  = access;
        req->fd      = fd;
        SERVER_CALL();
        ret = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           FILE_GetUnixHandle
 *
 * Retrieve the Unix handle corresponding to a file handle.
 * Returns -1 on failure.
 */
int FILE_GetUnixHandle( HANDLE handle, DWORD access )
{
    int ret, fd = -1;

    do
    {
        SERVER_START_REQ( get_handle_fd )
        {
            req->handle = handle;
            req->access = access;
            if (!(ret = SERVER_CALL_ERR())) fd = req->fd;
        }
        SERVER_END_REQ;
        if (ret) return -1;

        if (fd == -1)  /* it wasn't in the cache, get it from the server */
            fd = wine_server_recv_fd( handle );

    } while (fd == -2);  /* -2 means race condition, so restart from scratch */

    if (fd != -1)
    {
        if ((fd = dup(fd)) == -1)
            SetLastError( ERROR_TOO_MANY_OPEN_FILES );
    }
    return fd;
}


/*************************************************************************
 * 		FILE_OpenConsole
 *
 * Open a handle to the current process console.
 * Returns 0 on failure.
 */
static HANDLE FILE_OpenConsole( BOOL output, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE ret;

    SERVER_START_REQ( open_console )
    {
        req->output  = output;
        req->access  = access;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           FILE_CreateFile
 *
 * Implementation of CreateFile. Takes a Unix path name.
 * Returns 0 on failure.
 */
HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                        LPSECURITY_ATTRIBUTES sa, DWORD creation,
                        DWORD attributes, HANDLE template, BOOL fail_read_only )
{
    DWORD err;
    HANDLE ret;
    size_t len = strlen(filename);

    if (len > REQUEST_MAX_VAR_SIZE)
    {
        FIXME("filename '%s' too long\n", filename );
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

 restart:
    SERVER_START_VAR_REQ( create_file, len )
    {
        req->access  = access;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        req->sharing = sharing;
        req->create  = creation;
        req->attrs   = attributes;
        memcpy( server_data_ptr(req), filename, len );
        SetLastError(0);
        err = SERVER_CALL();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;

    /* If write access failed, retry without GENERIC_WRITE */

    if (!ret && !fail_read_only && (access & GENERIC_WRITE))
    {
	if ((err == STATUS_MEDIA_WRITE_PROTECTED) || (err == STATUS_ACCESS_DENIED))
        {
	    TRACE("Write access failed for file '%s', trying without "
		  "write access\n", filename);
            access &= ~GENERIC_WRITE;
            goto restart;
        }
    }

    if (err) SetLastError( RtlNtStatusToDosError(err) );

    if (!ret)
        WARN("Unable to create file '%s' (GLE %ld)\n", filename, GetLastError());

    return ret;
}


/***********************************************************************
 *           FILE_CreateDevice
 *
 * Same as FILE_CreateFile but for a device
 * Returns 0 on failure.
 */
HANDLE FILE_CreateDevice( int client_id, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE ret;
    SERVER_START_REQ( create_device )
    {
        req->access  = access;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        req->id      = client_id;
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}

static HANDLE FILE_OpenPipe(LPCSTR name, DWORD access)
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;

    TRACE("name %s access %lx\n",name,access);

    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_VAR_REQ( open_named_pipe, len * sizeof(WCHAR) )
    {
        req->access = access;

        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    TRACE("Returned %d\n",ret);
    return ret;
}

/*************************************************************************
 * CreateFileA [KERNEL32.@]  Creates or opens a file or other object
 *
 * Creates or opens an object, and returns a handle that can be used to
 * access that object.
 *
 * PARAMS
 *
 * filename     [in] pointer to filename to be accessed
 * access       [in] access mode requested
 * sharing      [in] share mode
 * sa           [in] pointer to security attributes
 * creation     [in] how to create the file
 * attributes   [in] attributes for newly created file
 * template     [in] handle to file with extended attributes to copy
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
 * Doesn't support character devices, template files, or a
 * lot of the 'attributes' flags yet.
 */
HANDLE WINAPI CreateFileA( LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    DOS_FULL_NAME full_name;
    HANDLE ret;

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
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
            return INVALID_HANDLE_VALUE;
	}
    }

    if (!strncmp(filename, "\\\\.\\", 4)) {
        if(!strncasecmp(&filename[4],"pipe\\",5))
        {
            TRACE("Opening a pipe: %s\n",filename);
            return FILE_OpenPipe(filename,access);
        }
        else if (!DOSFS_GetDevice( filename ))
        {
            ret = DEVICE_Open( filename+4, access, sa );
            goto done;
        }
	else
        	filename+=4; /* fall into DOSFS_Device case below */
    }

    /* If the name still starts with '\\', it's a UNC name. */
    if (!strncmp(filename, "\\\\", 2))
    {
        FIXME("UNC name (%s) not supported.\n", filename );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    /* If the name contains a DOS wild card (* or ?), do no create a file */
    if(strchr(filename,'*') || strchr(filename,'?'))
        return INVALID_HANDLE_VALUE;

    /* Open a console for CONIN$ or CONOUT$ */
    if (!strcasecmp(filename, "CONIN$"))
    {
        ret = FILE_OpenConsole( FALSE, access, sa );
        goto done;
    }
    if (!strcasecmp(filename, "CONOUT$"))
    {
        ret = FILE_OpenConsole( TRUE, access, sa );
        goto done;
    }

    if (DOSFS_GetDevice( filename ))
    {
        TRACE("opening device '%s'\n", filename );

        if (!(ret = DOSFS_OpenDevice( filename, access )))
        {
            /* Do not silence this please. It is a critical error. -MM */
            ERR("Couldn't open device '%s'!\n",filename);
            SetLastError( ERROR_FILE_NOT_FOUND );
        }
        goto done;
    }

    /* check for filename, don't check for last entry if creating */
    if (!DOSFS_GetFullName( filename,
			    (creation == OPEN_EXISTING) ||
			    (creation == TRUNCATE_EXISTING),
			    &full_name )) {
	WARN("Unable to get full filename from '%s' (GLE %ld)\n",
	     filename, GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    ret = FILE_CreateFile( full_name.long_name, access, sharing,
                           sa, creation, attributes, template,
                           DRIVE_GetFlags(full_name.drive) & DRIVE_FAIL_READ_ONLY );
 done:
    if (!ret) ret = INVALID_HANDLE_VALUE;
    return ret;
}



/*************************************************************************
 *              CreateFileW              (KERNEL32.@)
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

    RtlSecondsSince1970ToTime( st->st_mtime, &info->ftCreationTime );
    RtlSecondsSince1970ToTime( st->st_mtime, &info->ftLastWriteTime );
    RtlSecondsSince1970ToTime( st->st_atime, &info->ftLastAccessTime );

    info->dwVolumeSerialNumber = 0;  /* FIXME */
    info->nFileSizeHigh = 0;
    info->nFileSizeLow  = 0;
    if (!S_ISDIR(st->st_mode)) {
	info->nFileSizeHigh = st->st_size >> 32;
	info->nFileSizeLow  = st->st_size & 0xffffffff;
    }
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

    if (lstat( unixName, &st ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    if (!S_ISLNK(st.st_mode)) FILE_FillInfo( &st, info );
    else
    {
        /* do a "real" stat to find out
	   about the type of the symlink destination */
        if (stat( unixName, &st ) == -1)
        {
            FILE_SetDosError();
            return FALSE;
        }
        FILE_FillInfo( &st, info );
        info->dwFileAttributes |= FILE_ATTRIBUTE_SYMLINK;
    }
    return TRUE;
}


/***********************************************************************
 *             GetFileInformationByHandle   (KERNEL32.@)
 */
DWORD WINAPI GetFileInformationByHandle( HANDLE hFile,
                                         BY_HANDLE_FILE_INFORMATION *info )
{
    DWORD ret;
    if (!info) return 0;

    SERVER_START_REQ( get_file_info )
    {
        req->handle = hFile;
        if ((ret = !SERVER_CALL_ERR()))
        {
	    /* FIXME: which file types are supported ?
	     * Serial ports (FILE_TYPE_CHAR) are not,
	     * and MSDN also says that pipes are not supported.
	     * FILE_TYPE_REMOTE seems to be supported according to
	     * MSDN q234741.txt */
	    if ((req->type == FILE_TYPE_DISK)
	    ||  (req->type == FILE_TYPE_REMOTE))
	    {
                RtlSecondsSince1970ToTime( req->write_time, &info->ftCreationTime );
                RtlSecondsSince1970ToTime( req->write_time, &info->ftLastWriteTime );
                RtlSecondsSince1970ToTime( req->access_time, &info->ftLastAccessTime );
                info->dwFileAttributes     = req->attr;
                info->dwVolumeSerialNumber = req->serial;
                info->nFileSizeHigh        = req->size_high;
                info->nFileSizeLow         = req->size_low;
                info->nNumberOfLinks       = req->links;
                info->nFileIndexHigh       = req->index_high;
                info->nFileIndexLow        = req->index_low;
	    }
	    else
	    {
		SetLastError(ERROR_NOT_SUPPORTED);
		ret = 0;
	    }
        }
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           GetFileAttributes   (KERNEL.420)
 */
DWORD WINAPI GetFileAttributes16( LPCSTR name )
{
    return GetFileAttributesA( name );
}


/**************************************************************************
 *           GetFileAttributesA   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesA( LPCSTR name )
{
    DOS_FULL_NAME full_name;
    BY_HANDLE_FILE_INFORMATION info;

    if (name == NULL)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }
    if (!DOSFS_GetFullName( name, TRUE, &full_name) )
        return -1;
    if (!FILE_Stat( full_name.long_name, &info )) return -1;
    return info.dwFileAttributes;
}


/**************************************************************************
 *           GetFileAttributesW   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesW( LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    DWORD res = GetFileAttributesA( nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetFileSize   (KERNEL32.@)
 */
DWORD WINAPI GetFileSize( HANDLE hFile, LPDWORD filesizehigh )
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle( hFile, &info )) return -1;
    if (filesizehigh) *filesizehigh = info.nFileSizeHigh;
    return info.nFileSizeLow;
}


/***********************************************************************
 *           GetFileTime   (KERNEL32.@)
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
 *           CompareFileTime   (KERNEL32.@)
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
                                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
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
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique,
                                  LPSTR buffer)
{
    return FILE_GetTempFileName(path, prefix, unique, buffer, FALSE);
}

/***********************************************************************
 *           GetTempFileNameW   (KERNEL32.@)
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
    MultiByteToWideChar( CP_ACP, 0, buffera, -1, buffer, MAX_PATH );
    HeapFree( GetProcessHeap(), 0, patha );
    HeapFree( GetProcessHeap(), 0, prefixa );
    return ret;
}


/***********************************************************************
 *           GetTempFileName   (KERNEL.97)
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
                                       FILE_ATTRIBUTE_NORMAL, 0 ))== INVALID_HANDLE_VALUE)
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
	 The loader should keep the file open, as Windows does that, too.
       */
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
                                NULL, OPEN_EXISTING, 0, 0,
                                DRIVE_GetFlags(full_name.drive) & DRIVE_FAIL_READ_ONLY );
    if (!hFileRet) goto not_found;

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
        hFileRet = Win32HandleToDosFileHandle( hFileRet );
        if (hFileRet == HFILE_ERROR16) goto error;
        if (mode & OF_EXIST) /* Return the handle, but close it first */
            _lclose16( hFileRet );
    }
    return hFileRet;

not_found:  /* We get here if the file does not exist */
    WARN("'%s' not found or sharing violation\n", name );
    SetLastError( ERROR_FILE_NOT_FOUND );
    /* fall through */

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = GetLastError();
    WARN("(%s): return = HFILE_ERROR error= %d\n", 
		  name,ofs->nErrCode );
    return HFILE_ERROR;
}


/***********************************************************************
 *           OpenFile   (KERNEL.74)
 *           OpenFileEx (KERNEL.360)
 */
HFILE16 WINAPI OpenFile16( LPCSTR name, OFSTRUCT *ofs, UINT16 mode )
{
    return FILE_DoOpenFile( name, ofs, mode, FALSE );
}


/***********************************************************************
 *           OpenFile   (KERNEL32.@)
 */
HFILE WINAPI OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    return FILE_DoOpenFile( name, ofs, mode, TRUE );
}


/***********************************************************************
 *           FILE_InitProcessDosHandles
 *
 * Allocates the default DOS handles for a process. Called either by
 * Win32HandleToDosFileHandle below or by the DOSVM stuff.
 */
static void FILE_InitProcessDosHandles( void )
{
    dos_handles[0] = GetStdHandle(STD_INPUT_HANDLE);
    dos_handles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
    dos_handles[2] = GetStdHandle(STD_ERROR_HANDLE);
    dos_handles[3] = GetStdHandle(STD_ERROR_HANDLE);
    dos_handles[4] = GetStdHandle(STD_ERROR_HANDLE);
}

/***********************************************************************
 *           Win32HandleToDosFileHandle   (KERNEL32.21)
 *
 * Allocate a DOS handle for a Win32 handle. The Win32 handle is no
 * longer valid after this function (even on failure).
 *
 * Note: this is not exactly right, since on Win95 the Win32 handles
 *       are on top of DOS handles and we do it the other way
 *       around. Should be good enough though.
 */
HFILE WINAPI Win32HandleToDosFileHandle( HANDLE handle )
{
    int i;

    if (!handle || (handle == INVALID_HANDLE_VALUE))
        return HFILE_ERROR;

    for (i = 5; i < DOS_TABLE_SIZE; i++)
        if (!dos_handles[i])
        {
            dos_handles[i] = handle;
            TRACE("Got %d for h32 %d\n", i, handle );
            return (HFILE)i;
        }
    CloseHandle( handle );
    SetLastError( ERROR_TOO_MANY_OPEN_FILES );
    return HFILE_ERROR;
}


/***********************************************************************
 *           DosFileHandleToWin32Handle   (KERNEL32.20)
 *
 * Return the Win32 handle for a DOS handle.
 *
 * Note: this is not exactly right, since on Win95 the Win32 handles
 *       are on top of DOS handles and we do it the other way
 *       around. Should be good enough though.
 */
HANDLE WINAPI DosFileHandleToWin32Handle( HFILE handle )
{
    HFILE16 hfile = (HFILE16)handle;
    if (hfile < 5 && !dos_handles[hfile]) FILE_InitProcessDosHandles();
    if ((hfile >= DOS_TABLE_SIZE) || !dos_handles[hfile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return INVALID_HANDLE_VALUE;
    }
    return dos_handles[hfile];
}


/***********************************************************************
 *           DisposeLZ32Handle   (KERNEL32.22)
 *
 * Note: this is not entirely correct, we should only close the
 *       32-bit handle and not the 16-bit one, but we cannot do
 *       this because of the way our DOS handles are implemented.
 *       It shouldn't break anything though.
 */
void WINAPI DisposeLZ32Handle( HANDLE handle )
{
    int i;

    if (!handle || (handle == INVALID_HANDLE_VALUE)) return;

    for (i = 5; i < DOS_TABLE_SIZE; i++)
        if (dos_handles[i] == handle)
        {
            dos_handles[i] = 0;
            CloseHandle( handle );
            break;
        }
}


/***********************************************************************
 *           FILE_Dup2
 *
 * dup2() function for DOS handles.
 */
HFILE16 FILE_Dup2( HFILE16 hFile1, HFILE16 hFile2 )
{
    HANDLE new_handle;

    if (hFile1 < 5 && !dos_handles[hFile1]) FILE_InitProcessDosHandles();

    if ((hFile1 >= DOS_TABLE_SIZE) || (hFile2 >= DOS_TABLE_SIZE) || !dos_handles[hFile1])
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
    if (!DuplicateHandle( GetCurrentProcess(), dos_handles[hFile1],
                          GetCurrentProcess(), &new_handle,
                          0, FALSE, DUPLICATE_SAME_ACCESS ))
        return HFILE_ERROR16;
    if (dos_handles[hFile2]) CloseHandle( dos_handles[hFile2] );
    dos_handles[hFile2] = new_handle;
    return hFile2;
}


/***********************************************************************
 *           _lclose   (KERNEL.81)
 */
HFILE16 WINAPI _lclose16( HFILE16 hFile )
{
    if (hFile < 5)
    {
        FIXME("stdio handle closed, need proper conversion\n" );
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    if ((hFile >= DOS_TABLE_SIZE) || !dos_handles[hFile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    TRACE("%d (handle32=%d)\n", hFile, dos_handles[hFile] );
    CloseHandle( dos_handles[hFile] );
    dos_handles[hFile] = 0;
    return 0;
}


/***********************************************************************
 *           _lclose   (KERNEL32.@)
 */
HFILE WINAPI _lclose( HFILE hFile )
{
    TRACE("handle %d\n", hFile );
    return CloseHandle( hFile ) ? 0 : HFILE_ERROR;
}

/***********************************************************************
 *              GetOverlappedResult     (KERNEL32.@)
 *
 * Check the result of an Asynchronous data transfer from a file.
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 *  If successful (and relevant) lpTransfered will hold the number of
 *   bytes transfered during the async operation.
 *
 * BUGS
 *
 * Currently only works for WaitCommEvent, ReadFile, WriteFile 
 *   with communications ports.
 *
 */
BOOL WINAPI GetOverlappedResult(
    HANDLE hFile,              /* [in] handle of file to check on */
    LPOVERLAPPED lpOverlapped, /* [in/out] pointer to overlapped  */
    LPDWORD lpTransferred,     /* [in/out] number of bytes transfered  */
    BOOL bWait                 /* [in] wait for the transfer to complete ? */
) {
    DWORD r;

    TRACE("(%d %p %p %x)\n", hFile, lpOverlapped, lpTransferred, bWait);

    if(lpOverlapped==NULL)
    {
        ERR("lpOverlapped was null\n");
        return FALSE;
    }
    if(!lpOverlapped->hEvent)
    {
        ERR("lpOverlapped->hEvent was null\n");
        return FALSE;
    }

    do {
        TRACE("waiting on %p\n",lpOverlapped);
        r = WaitForSingleObjectEx(lpOverlapped->hEvent, bWait?INFINITE:0, TRUE);
        TRACE("wait on %p returned %ld\n",lpOverlapped,r);
    } while (r==STATUS_USER_APC);

    if(lpTransferred)
        *lpTransferred = lpOverlapped->InternalHigh;

    SetLastError(lpOverlapped->Internal);

    return (r==WAIT_OBJECT_0);
}


/***********************************************************************
 *             FILE_AsyncReadService      (INTERNAL)
 *
 *  This function is called while the client is waiting on the
 *  server, so we can't make any server calls here.
 */
static void FILE_AsyncReadService(async_private *ovp, int events)
{
    LPOVERLAPPED lpOverlapped = ovp->lpOverlapped;
    int result, r;

    TRACE("%p %p %08x\n", lpOverlapped, ovp->buffer, events );

    /* if POLLNVAL, then our fd was closed or we have the wrong fd */
    if(events&POLLNVAL)
    {
        ERR("fd %d invalid for %p\n",ovp->fd,ovp);
        r = STATUS_UNSUCCESSFUL;
        goto async_end;
    }

    /* if there are no events, it must be a timeout */
    if(events==0)
    {
        TRACE("read timed out\n");
        r = STATUS_TIMEOUT;
        goto async_end;
    }

    /* check to see if the data is ready (non-blocking) */
    result = read(ovp->fd, &ovp->buffer[lpOverlapped->InternalHigh],
                  lpOverlapped->OffsetHigh - lpOverlapped->InternalHigh);

    if ( (result<0) && ((errno == EAGAIN) || (errno == EINTR)))
    {
        TRACE("Deferred read %d\n",errno);
        r = STATUS_PENDING;
        goto async_end;
    }

    /* check to see if the transfer is complete */
    if(result<0)
    {
        TRACE("read returned errno %d\n",errno);
        r = STATUS_UNSUCCESSFUL;
        goto async_end;
    }

    lpOverlapped->InternalHigh += result;
    TRACE("read %d more bytes %ld/%ld so far\n",result,lpOverlapped->InternalHigh,lpOverlapped->OffsetHigh);

    if(lpOverlapped->InternalHigh < lpOverlapped->OffsetHigh)
        r = STATUS_PENDING;
    else
        r = STATUS_SUCCESS;

async_end:
    lpOverlapped->Internal = r;
}

/* flogged from wineserver */
/* add a timeout in milliseconds to an absolute time */
static void add_timeout( struct timeval *when, int timeout )
{
    if (timeout)
    {
        long sec = timeout / 1000;
        if ((when->tv_usec += (timeout - 1000*sec) * 1000) >= 1000000)
        {
            when->tv_usec -= 1000000;
            when->tv_sec++;
        }
        when->tv_sec += sec;
    }
}

/***********************************************************************
 *              ReadFileEx                (KERNEL32.@)
 */
BOOL WINAPI ReadFileEx(HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
			 LPOVERLAPPED overlapped, 
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    async_private *ovp;
    int fd, timeout, ret;

    TRACE("file %d to buf %p num %ld %p func %p\n",
	  hFile, buffer, bytesToRead, overlapped, lpCompletionRoutine);

    /* check that there is an overlapped struct with an event flag */
    if ( (overlapped==NULL) || NtResetEvent( overlapped->hEvent, NULL ) )
    {
        TRACE("Overlapped not specified or invalid event flag\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    overlapped->Offset       = 0;
    overlapped->OffsetHigh   = bytesToRead; /* FIXME: wrong */
    overlapped->Internal     = STATUS_PENDING;
    overlapped->InternalHigh = 0;

    /* 
     * Although the overlapped transfer will be done in this thread
     * we still need to register the operation with the server, in
     * case it is cancelled and to get a file handle and the timeout info.
     */
    SERVER_START_REQ(create_async)
    {
        req->count = bytesToRead;
        req->type = ASYNC_TYPE_READ;
        req->file_handle = hFile;
        ret = SERVER_CALL();
        timeout = req->timeout;
    }
    SERVER_END_REQ;
    if (ret)
    {
        TRACE("server call failed\n");
        return FALSE;
    }

    fd = FILE_GetUnixHandle( hFile, GENERIC_WRITE );
    if(fd<0)
    {
        TRACE("Couldn't get FD\n");
        return FALSE;
    }

    ovp = (async_private *) HeapAlloc(GetProcessHeap(), 0, sizeof (async_private));
    if(!ovp)
    {
        TRACE("HeapAlloc Failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        close(fd);
        return FALSE;
    }
    ovp->lpOverlapped = overlapped;
    ovp->timeout = timeout;
    gettimeofday(&ovp->tv,NULL);
    add_timeout(&ovp->tv,timeout);
    ovp->event = POLLIN;
    ovp->func = FILE_AsyncReadService;
    ovp->buffer = buffer;
    ovp->fd = fd;

    /* hook this overlap into the pending async operation list */
    ovp->next = NtCurrentTeb()->pending_list;
    ovp->prev = NULL;
    if(ovp->next)
        ovp->next->prev = ovp;
    NtCurrentTeb()->pending_list = ovp;

    SetLastError(ERROR_IO_PENDING);

    /* always fail on return, either ERROR_IO_PENDING or other error */
    return FALSE;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.@)
 */
BOOL WINAPI ReadFile( HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                        LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    int unix_handle, result;

    TRACE("%d %p %ld %p %p\n", hFile, buffer, bytesToRead, 
          bytesRead, overlapped );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    /* this will only have impact if the overlapped structure is specified */
    if ( overlapped )
        return ReadFileEx(hFile, buffer, bytesToRead, overlapped, NULL);

    unix_handle = FILE_GetUnixHandle( hFile, GENERIC_READ );
    if (unix_handle == -1) return FALSE;

    /* code for synchronous reads */
    while ((result = read( unix_handle, buffer, bytesToRead )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        if ((errno == EFAULT) && !IsBadWritePtr( buffer, bytesToRead )) continue;
        FILE_SetDosError();
        break;
    }
    close( unix_handle );
    if (result == -1) return FALSE;
    if (bytesRead) *bytesRead = result;
    return TRUE;
}


/***********************************************************************
 *             FILE_AsyncWriteService      (INTERNAL)
 *
 *  This function is called while the client is waiting on the
 *  server, so we can't make any server calls here.
 */
static void FILE_AsyncWriteService(struct async_private *ovp, int events)
{
    LPOVERLAPPED lpOverlapped = ovp->lpOverlapped;
    int result, r;

    TRACE("(%p %p %08x)\n",lpOverlapped,ovp->buffer,events);

    /* if POLLNVAL, then our fd was closed or we have the wrong fd */
    if(events&POLLNVAL)
    {
        ERR("fd %d invalid for %p\n",ovp->fd,ovp);
        r = STATUS_UNSUCCESSFUL;
        goto async_end;
    }

    /* if there are no events, it must be a timeout */
    if(events==0)
    {
        TRACE("write timed out\n");
        r = STATUS_TIMEOUT;
        goto async_end;
    }

    /* write some data (non-blocking) */
    result = write(ovp->fd, &ovp->buffer[lpOverlapped->InternalHigh],
                  lpOverlapped->OffsetHigh-lpOverlapped->InternalHigh);

    if ( (result<0) && ((errno == EAGAIN) || (errno == EINTR)))
    {
        r = STATUS_PENDING;
        goto async_end;
    }

    /* check to see if the transfer is complete */
    if(result<0)
    {
        r = STATUS_UNSUCCESSFUL;
        goto async_end;
    }

    lpOverlapped->InternalHigh += result;

    TRACE("wrote %d more bytes %ld/%ld so far\n",result,lpOverlapped->InternalHigh,lpOverlapped->OffsetHigh);

    if(lpOverlapped->InternalHigh < lpOverlapped->OffsetHigh)
        r = STATUS_PENDING;
    else
        r = STATUS_SUCCESS;

async_end:
    lpOverlapped->Internal = r;
}

/***********************************************************************
 *              WriteFileEx                (KERNEL32.@)
 */
BOOL WINAPI WriteFileEx(HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
			 LPOVERLAPPED overlapped, 
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    async_private *ovp;
    int timeout,ret;

    TRACE("file %d to buf %p num %ld %p func %p stub\n",
	  hFile, buffer, bytesToWrite, overlapped, lpCompletionRoutine);

    if ( (overlapped == NULL) || NtResetEvent( overlapped->hEvent, NULL ) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    overlapped->Offset       = 0;
    overlapped->OffsetHigh   = bytesToWrite;
    overlapped->Internal     = STATUS_PENDING;
    overlapped->InternalHigh = 0;

    /* need to check the server to get the timeout info */
    SERVER_START_REQ(create_async)
    {
        req->count = bytesToWrite;
        req->type = ASYNC_TYPE_WRITE;
        req->file_handle = hFile;
        ret = SERVER_CALL();
        timeout = req->timeout;
    }
    SERVER_END_REQ;
    if (ret)
    {
        TRACE("server call failed\n");
        return FALSE;
    }

    ovp = (async_private*) HeapAlloc(GetProcessHeap(), 0, sizeof (async_private));
    if(!ovp)
    {
        TRACE("HeapAlloc Failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ovp->lpOverlapped = overlapped;
    ovp->timeout = timeout;
    gettimeofday(&ovp->tv,NULL);
    add_timeout(&ovp->tv,timeout);
    ovp->event = POLLOUT;
    ovp->func = FILE_AsyncWriteService;
    ovp->buffer = (LPVOID) buffer;
    ovp->fd = FILE_GetUnixHandle( hFile, GENERIC_WRITE );
    if(ovp->fd <0)
    {
        HeapFree(GetProcessHeap(), 0, ovp);
        return FALSE;
    }

    /* hook this overlap into the pending async operation list */
    ovp->next = NtCurrentTeb()->pending_list;
    ovp->prev = NULL;
    if(ovp->next)
        ovp->next->prev = ovp;
    NtCurrentTeb()->pending_list = ovp;

    SetLastError(ERROR_IO_PENDING);

    /* always fail on return, either ERROR_IO_PENDING or other error */
    return FALSE;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.@)
 */
BOOL WINAPI WriteFile( HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                         LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    int unix_handle, result;

    TRACE("%d %p %ld %p %p\n", hFile, buffer, bytesToWrite, 
          bytesWritten, overlapped );

    if (bytesWritten) *bytesWritten = 0;  /* Do this before anything else */
    if (!bytesToWrite) return TRUE;

    /* this will only have impact if the overlappd structure is specified */
    if ( overlapped )
        return WriteFileEx(hFile, buffer, bytesToWrite, overlapped, NULL);

    unix_handle = FILE_GetUnixHandle( hFile, GENERIC_WRITE );
    if (unix_handle == -1) return FALSE;

    /* synchronous file write */
    while ((result = write( unix_handle, buffer, bytesToWrite )) == -1)
    {
        if ((errno == EAGAIN) || (errno == EINTR)) continue;
        if ((errno == EFAULT) && !IsBadReadPtr( buffer, bytesToWrite )) continue;
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
 *           _hread (KERNEL.349)
 */
LONG WINAPI WIN16_hread( HFILE16 hFile, SEGPTR buffer, LONG count )
{
    LONG maxlen;

    TRACE("%d %08lx %ld\n",
                  hFile, (DWORD)buffer, count );

    /* Some programs pass a count larger than the allocated buffer */
    maxlen = GetSelectorLimit16( SELECTOROF(buffer) ) - OFFSETOF(buffer) + 1;
    if (count > maxlen) count = maxlen;
    return _lread(DosFileHandleToWin32Handle(hFile), MapSL(buffer), count );
}


/***********************************************************************
 *           _lread (KERNEL.82)
 */
UINT16 WINAPI WIN16_lread( HFILE16 hFile, SEGPTR buffer, UINT16 count )
{
    return (UINT16)WIN16_hread( hFile, buffer, (LONG)count );
}


/***********************************************************************
 *           _lread   (KERNEL32.@)
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
    return (UINT16)_lread(DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    return Win32HandleToDosFileHandle( _lcreat( path, attr ) );
}


/***********************************************************************
 *           _lcreat   (KERNEL32.@)
 */
HFILE WINAPI _lcreat( LPCSTR path, INT attr )
{
    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    TRACE("%s %02x\n", path, attr );
    return CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, attr, 0 );
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.@)
 */
DWORD WINAPI SetFilePointer( HANDLE hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    DWORD ret = 0xffffffff;

    TRACE("handle %d offset %ld high %ld origin %ld\n",
          hFile, distance, highword?*highword:0, method );

    SERVER_START_REQ( set_file_pointer )
    {
        req->handle = hFile;
        req->low = distance;
        req->high = highword ? *highword : (distance >= 0) ? 0 : -1;
        /* FIXME: assumes 1:1 mapping between Windows and Unix seek constants */
        req->whence = method;
        SetLastError( 0 );
        if (!SERVER_CALL_ERR())
        {
            ret = req->new_low;
            if (highword) *highword = req->new_high;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           _llseek   (KERNEL.84)
 *
 * FIXME:
 *   Seeking before the start of the file should be allowed for _llseek16,
 *   but cause subsequent I/O operations to fail (cf. interrupt list)
 *
 */
LONG WINAPI _llseek16( HFILE16 hFile, LONG lOffset, INT16 nOrigin )
{
    return SetFilePointer( DosFileHandleToWin32Handle(hFile), lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _llseek   (KERNEL32.@)
 */
LONG WINAPI _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    return SetFilePointer( hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lopen   (KERNEL.85)
 */
HFILE16 WINAPI _lopen16( LPCSTR path, INT16 mode )
{
    return Win32HandleToDosFileHandle( _lopen( path, mode ) );
}


/***********************************************************************
 *           _lopen   (KERNEL32.@)
 */
HFILE WINAPI _lopen( LPCSTR path, INT mode )
{
    DWORD access, sharing;

    TRACE("('%s',%04x)\n", path, mode );
    FILE_ConvertOFMode( mode, &access, &sharing );
    return CreateFileA( path, access, sharing, NULL, OPEN_EXISTING, 0, 0 );
}


/***********************************************************************
 *           _lwrite   (KERNEL.86)
 */
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite( DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
}

/***********************************************************************
 *           _lwrite   (KERNEL32.@)
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
    return _lread( DosFileHandleToWin32Handle(hFile), buffer, count );
}


/***********************************************************************
 *           _hread   (KERNEL32.@)
 */
LONG WINAPI _hread( HFILE hFile, LPVOID buffer, LONG count)
{
    return _lread( hFile, buffer, count );
}


/***********************************************************************
 *           _hwrite   (KERNEL.350)
 */
LONG WINAPI _hwrite16( HFILE16 hFile, LPCSTR buffer, LONG count )
{
    return _hwrite( DosFileHandleToWin32Handle(hFile), buffer, count );
}


/***********************************************************************
 *           _hwrite   (KERNEL32.@)
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
 *           SetHandleCount   (KERNEL.199)
 */
UINT16 WINAPI SetHandleCount16( UINT16 count )
{
    return SetHandleCount( count );
}


/*************************************************************************
 *           SetHandleCount   (KERNEL32.@)
 */
UINT WINAPI SetHandleCount( UINT count )
{
    return min( 256, count );
}


/***********************************************************************
 *           FlushFileBuffers   (KERNEL32.@)
 */
BOOL WINAPI FlushFileBuffers( HANDLE hFile )
{
    BOOL ret;
    SERVER_START_REQ( flush_file )
    {
        req->handle = hFile;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           SetEndOfFile   (KERNEL32.@)
 */
BOOL WINAPI SetEndOfFile( HANDLE hFile )
{
    BOOL ret;
    SERVER_START_REQ( truncate_file )
    {
        req->handle = hFile;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           DeleteFile   (KERNEL.146)
 */
BOOL16 WINAPI DeleteFile16( LPCSTR path )
{
    return DeleteFileA( path );
}


/***********************************************************************
 *           DeleteFileA   (KERNEL32.@)
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
 *           DeleteFileW   (KERNEL32.@)
 */
BOOL WINAPI DeleteFileW( LPCWSTR path )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    BOOL ret = DeleteFileA( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           GetFileType   (KERNEL32.@)
 */
DWORD WINAPI GetFileType( HANDLE hFile )
{
    DWORD ret = FILE_TYPE_UNKNOWN;
    SERVER_START_REQ( get_file_info )
    {
        req->handle = hFile;
        if (!SERVER_CALL_ERR()) ret = req->type;
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           MoveFileExA   (KERNEL32.@)
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
 *           MoveFileExW   (KERNEL32.@)
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
 *           MoveFileA   (KERNEL32.@)
 *
 *  Move file or directory
 */
BOOL WINAPI MoveFileA( LPCSTR fn1, LPCSTR fn2 )
{
    DOS_FULL_NAME full_name1, full_name2;
    struct stat fstat;


    TRACE("(%s,%s)\n", fn1, fn2 );

    if (!DOSFS_GetFullName( fn1, TRUE, &full_name1 )) return FALSE;
    if (DOSFS_GetFullName( fn2, TRUE, &full_name2 ))  {
      /* The new name must not already exist */ 
      SetLastError(ERROR_ALREADY_EXISTS);
      return FALSE;
    }
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
 *           MoveFileW   (KERNEL32.@)
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
 *           CopyFileA   (KERNEL32.@)
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
                             info.dwFileAttributes, h1 )) == INVALID_HANDLE_VALUE)
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
 *           CopyFileW   (KERNEL32.@)
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
 *           CopyFileExA   (KERNEL32.@)
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
 *           CopyFileExW   (KERNEL32.@)
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
 *              SetFileTime   (KERNEL32.@)
 */
BOOL WINAPI SetFileTime( HANDLE hFile,
                           const FILETIME *lpCreationTime,
                           const FILETIME *lpLastAccessTime,
                           const FILETIME *lpLastWriteTime )
{
    BOOL ret;
    SERVER_START_REQ( set_file_time )
    {
        req->handle = hFile;
        if (lpLastAccessTime)
            RtlTimeToSecondsSince1970( lpLastAccessTime, (DWORD *)&req->access_time );
        else
            req->access_time = 0; /* FIXME */
        if (lpLastWriteTime)
            RtlTimeToSecondsSince1970( lpLastWriteTime, (DWORD *)&req->write_time );
        else
            req->write_time = 0; /* FIXME */
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           LockFile   (KERNEL32.@)
 */
BOOL WINAPI LockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                        DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh )
{
    BOOL ret;
    SERVER_START_REQ( lock_file )
    {
        req->handle      = hFile;
        req->offset_low  = dwFileOffsetLow;
        req->offset_high = dwFileOffsetHigh;
        req->count_low   = nNumberOfBytesToLockLow;
        req->count_high  = nNumberOfBytesToLockHigh;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}

/**************************************************************************
 * LockFileEx [KERNEL32.@]
 *
 * Locks a byte range within an open file for shared or exclusive access.
 *
 * RETURNS
 *   success: TRUE
 *   failure: FALSE
 *
 * NOTES
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
 *           UnlockFile   (KERNEL32.@)
 */
BOOL WINAPI UnlockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                          DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh )
{
    BOOL ret;
    SERVER_START_REQ( unlock_file )
    {
        req->handle      = hFile;
        req->offset_low  = dwFileOffsetLow;
        req->offset_high = dwFileOffsetHigh;
        req->count_low   = nNumberOfBytesToUnlockLow;
        req->count_high  = nNumberOfBytesToUnlockHigh;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           UnlockFileEx   (KERNEL32.@)
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
 *           LockFile   (KERNEL32.@)
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
 *           UnlockFile   (KERNEL32.@)
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
 * GetFileAttributesExA [KERNEL32.@]
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
 * GetFileAttributesExW [KERNEL32.@]
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
