/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "wine/server.h"

#include "drive.h"
#include "file.h"
#include "async.h"
#include "heap.h"
#include "msdos.h"
#include "wincon.h"
#include "../kernel/kernel_private.h"

#include "smb.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

/* Size of per-process table of DOS handles */
#define DOS_TABLE_SIZE 256

/* Macro to derive file offset from OVERLAPPED struct */
#define OVERLAPPED_OFFSET(overlapped) ((off_t) (overlapped)->Offset + ((off_t) (overlapped)->OffsetHigh << 32))

static HANDLE dos_handles[DOS_TABLE_SIZE];

mode_t FILE_umask;

extern HANDLE WINAPI FILE_SmbOpen(LPCSTR name);

/***********************************************************************
 *                  Asynchronous file I/O                              *
 */
static DWORD fileio_get_async_status (const async_private *ovp);
static DWORD fileio_get_async_count (const async_private *ovp);
static void fileio_set_async_status (async_private *ovp, const DWORD status);
static void CALLBACK fileio_call_completion_func (ULONG_PTR data);
static void fileio_async_cleanup (async_private *ovp);

static async_ops fileio_async_ops =
{
    fileio_get_async_status,       /* get_status */
    fileio_set_async_status,       /* set_status */
    fileio_get_async_count,        /* get_count */
    fileio_call_completion_func,   /* call_completion */
    fileio_async_cleanup           /* cleanup */
};

static async_ops fileio_nocomp_async_ops =
{
    fileio_get_async_status,       /* get_status */
    fileio_set_async_status,       /* set_status */
    fileio_get_async_count,        /* get_count */
    NULL,                          /* call_completion */
    fileio_async_cleanup           /* cleanup */
};

typedef struct async_fileio
{
    struct async_private             async;
    LPOVERLAPPED                     lpOverlapped;
    LPOVERLAPPED_COMPLETION_ROUTINE  completion_func;
    char                             *buffer;
    unsigned int                     count;
    enum fd_type                     fd_type;
} async_fileio;

static DWORD fileio_get_async_status (const struct async_private *ovp)
{
    return ((async_fileio*) ovp)->lpOverlapped->Internal;
}

static void fileio_set_async_status (async_private *ovp, const DWORD status)
{
    ((async_fileio*) ovp)->lpOverlapped->Internal = status;
}

static DWORD fileio_get_async_count (const struct async_private *ovp)
{
    async_fileio *fileio = (async_fileio*) ovp;

    if (fileio->count < fileio->lpOverlapped->InternalHigh)
    	return 0;
    return fileio->count - fileio->lpOverlapped->InternalHigh;
}

static void CALLBACK fileio_call_completion_func (ULONG_PTR data)
{
    async_fileio *ovp = (async_fileio*) data;
    TRACE ("data: %p\n", ovp);

    ovp->completion_func( RtlNtStatusToDosError ( ovp->lpOverlapped->Internal ),
                          ovp->lpOverlapped->InternalHigh,
                          ovp->lpOverlapped );

    fileio_async_cleanup ( &ovp->async );
}

static void fileio_async_cleanup ( struct async_private *ovp )
{
    HeapFree ( GetProcessHeap(), 0, ovp );
}

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
    int ret = 0;
    for ( ; ; str1++, str2++)
        if ((ret = FILE_toupper(*str1) - FILE_toupper(*str2)) || !*str1) break;
    return ret;
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
 *           FILE_GetNtStatus(void)
 *
 * Retrieve the Nt Status code from errno.
 * Try to be consistent with FILE_SetDosError().
 */
DWORD FILE_GetNtStatus(void)
{
    int err = errno;
    DWORD nt;
    TRACE ( "errno = %d\n", errno );
    switch ( err )
    {
    case EAGAIN:       nt = STATUS_SHARING_VIOLATION;       break;
    case EBADF:        nt = STATUS_INVALID_HANDLE;          break;
    case ENOSPC:       nt = STATUS_DISK_FULL;               break;
    case EPERM:
    case EROFS:
    case EACCES:       nt = STATUS_ACCESS_DENIED;           break;
    case ENOENT:       nt = STATUS_SHARING_VIOLATION;       break;
    case EISDIR:       nt = STATUS_FILE_IS_A_DIRECTORY;     break;
    case EMFILE:
    case ENFILE:       nt = STATUS_NO_MORE_FILES;           break;
    case EINVAL:
    case ENOTEMPTY:    nt = STATUS_DIRECTORY_NOT_EMPTY;     break;
    case EPIPE:        nt = STATUS_PIPE_BROKEN;             break;
    case ENOEXEC:      /* ?? */
    case ESPIPE:       /* ?? */
    case EEXIST:       /* ?? */
    default:
        FIXME ( "Converting errno %d to STATUS_UNSUCCESSFUL\n", err );
        nt = STATUS_UNSUCCESSFUL;
    }
    return nt;
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
 *           FILE_GetUnixHandleType
 *
 * Retrieve the Unix handle corresponding to a file handle.
 * Returns -1 on failure.
 */
static int FILE_GetUnixHandleType( HANDLE handle, DWORD access, enum fd_type *type, int *flags_ptr )
{
    int ret, flags, fd = -1;

    ret = wine_server_handle_to_fd( handle, access, &fd, type, &flags );
    if (flags_ptr) *flags_ptr = flags;
    if (ret) SetLastError( RtlNtStatusToDosError(ret) );
    else if (((access & GENERIC_READ)  && (flags & FD_FLAG_RECV_SHUTDOWN)) ||
             ((access & GENERIC_WRITE) && (flags & FD_FLAG_SEND_SHUTDOWN)))
    {
        close (fd);
        SetLastError ( ERROR_PIPE_NOT_CONNECTED );
        return -1;
    }
    return fd;
}

/***********************************************************************
 *           FILE_GetUnixHandle
 *
 * Retrieve the Unix handle corresponding to a file handle.
 * Returns -1 on failure.
 */
int FILE_GetUnixHandle( HANDLE handle, DWORD access )
{
    return FILE_GetUnixHandleType( handle, access, NULL, NULL );
}

/******************************************************************
 *		OpenConsoleW            (KERNEL32.@)
 *
 * Undocumented
 *      Open a handle to the current process console.
 *      Returns INVALID_HANDLE_VALUE on failure.
 */
HANDLE WINAPI OpenConsoleW(LPCWSTR name, DWORD access, LPSECURITY_ATTRIBUTES sa,
                           DWORD creation)
{
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};
    BOOL        output;
    HANDLE ret;

    if (strcmpW(coninW, name) == 0) 
        output = FALSE;
    else if (strcmpW(conoutW, name) == 0) 
        output = TRUE;
    else
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }
    if (creation != OPEN_EXISTING)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    SERVER_START_REQ( open_console )
    {
        req->from    = output;
        req->access  = access;
	req->share   = FILE_SHARE_READ | FILE_SHARE_WRITE;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret ? console_handle_map(ret) : INVALID_HANDLE_VALUE;
}

/******************************************************************
 *		VerifyConsoleIoHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI VerifyConsoleIoHandle(HANDLE handle)
{
    BOOL ret;

    if (!is_console_handle(handle)) return FALSE;
    SERVER_START_REQ(get_console_mode)
    {
	req->handle = console_handle_unmap(handle);
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		DuplicateConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI DuplicateConsoleHandle(HANDLE handle, DWORD access, BOOL inherit,
                                     DWORD options)
{
    HANDLE      ret;

    if (!is_console_handle(handle) ||
        !DuplicateHandle(GetCurrentProcess(), console_handle_unmap(handle), 
                         GetCurrentProcess(), &ret, access, inherit, options))
        return INVALID_HANDLE_VALUE;
    return console_handle_map(ret);
}

/******************************************************************
 *		CloseConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI CloseConsoleHandle(HANDLE handle)
{
    if (!is_console_handle(handle)) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return CloseHandle(console_handle_unmap(handle));
}

/******************************************************************
 *		GetConsoleInputWaitHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI GetConsoleInputWaitHandle(void)
{
    static HANDLE console_wait_event;
 
    /* FIXME: this is not thread safe */
    if (!console_wait_event)
    {
        SERVER_START_REQ(get_console_wait_event)
        {
            if (!wine_server_call_err( req )) console_wait_event = reply->handle;
        }
        SERVER_END_REQ;
    }
    return console_wait_event;
}
/* end of FIXME */


/* FIXME: those routines defined as pointers are needed, because this file is
 * currently compiled into NTDLL whereas it belongs to kernel32.
 * this shall go away once all the DLL separation process is done
 */
typedef BOOL    (WINAPI* pRW)(HANDLE, const void*, DWORD, DWORD*, void*);

static  BOOL FILE_ReadConsole(HANDLE hCon, void* buf, DWORD nb, DWORD* nr, void* p)
{
    static      HANDLE  hKernel /* = 0 */;
    static      pRW     pReadConsole  /* = 0 */;

    if ((!hKernel && !(hKernel = LoadLibraryA("kernel32"))) ||
        (!pReadConsole &&
         !(pReadConsole = GetProcAddress(hKernel, "ReadConsoleA"))))
    {
        *nr = 0;
        return 0;
    }
    return (pReadConsole)(hCon, buf, nb, nr, p);
}

static  BOOL FILE_WriteConsole(HANDLE hCon, const void* buf, DWORD nb, DWORD* nr, void* p)
{
    static      HANDLE  hKernel /* = 0 */;
    static      pRW     pWriteConsole  /* = 0 */;

    if ((!hKernel && !(hKernel = LoadLibraryA("kernel32"))) ||
        (!pWriteConsole &&
         !(pWriteConsole = GetProcAddress(hKernel, "WriteConsoleA"))))
    {
        *nr = 0;
        return 0;
    }
    return (pWriteConsole)(hCon, buf, nb, nr, p);
}
/* end of FIXME */

/***********************************************************************
 *           FILE_CreateFile
 *
 * Implementation of CreateFile. Takes a Unix path name.
 * Returns 0 on failure.
 */
HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                        LPSECURITY_ATTRIBUTES sa, DWORD creation,
                        DWORD attributes, HANDLE template, BOOL fail_read_only,
                        UINT drive_type )
{
    unsigned int err;
    HANDLE ret;

    for (;;)
    {
        SERVER_START_REQ( create_file )
        {
            req->access     = access;
            req->inherit    = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
            req->sharing    = sharing;
            req->create     = creation;
            req->attrs      = attributes;
            req->drive_type = drive_type;
            wine_server_add_data( req, filename, strlen(filename) );
            SetLastError(0);
            err = wine_server_call( req );
            ret = reply->handle;
        }
        SERVER_END_REQ;

        /* If write access failed, retry without GENERIC_WRITE */

        if (!ret && !fail_read_only && (access & GENERIC_WRITE))
        {
            if ((err == STATUS_MEDIA_WRITE_PROTECTED) || (err == STATUS_ACCESS_DENIED))
            {
                TRACE("Write access failed for file '%s', trying without "
                      "write access\n", filename);
                access &= ~GENERIC_WRITE;
                continue;
            }
        }

        if (err)
        {
            /* In the case file creation was rejected due to CREATE_NEW flag
             * was specified and file with that name already exists, correct
             * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
             * Note: RtlNtStatusToDosError is not the subject to blame here.
             */
            if (err == STATUS_OBJECT_NAME_COLLISION)
                SetLastError( ERROR_FILE_EXISTS );
            else
                SetLastError( RtlNtStatusToDosError(err) );
        }

        if (!ret) WARN("Unable to create file '%s' (GLE %ld)\n", filename, GetLastError());
        return ret;
    }
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
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

static HANDLE FILE_OpenPipe(LPCWSTR name, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE ret;
    DWORD len = 0;

    if (name && (len = strlenW(name)) > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( open_named_pipe )
    {
        req->access = access;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        SetLastError(0);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    TRACE("Returned %p\n",ret);
    return ret;
}

/*************************************************************************
 * CreateFileW [KERNEL32.@]  Creates or opens a file or other object
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
HANDLE WINAPI CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    DOS_FULL_NAME full_name;
    HANDLE ret;
    static const WCHAR bkslashes_with_question_markW[] = {'\\','\\','?','\\',0};
    static const WCHAR bkslashes_with_dotW[] = {'\\','\\','.','\\',0};
    static const WCHAR bkslashesW[] = {'\\','\\',0};
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }
    TRACE("%s %s%s%s%s%s%s%s attributes 0x%lx\n", debugstr_w(filename),
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
	  (creation ==TRUNCATE_EXISTING)?"TRUNCATE_EXISTING ":"", attributes);

    /* If the name starts with '\\?\', ignore the first 4 chars. */
    if (!strncmpW(filename, bkslashes_with_question_markW, 4))
    {
        static const WCHAR uncW[] = {'U','N','C','\\',0};
        filename += 4;
	if (!strncmpiW(filename, uncW, 4))
	{
            FIXME("UNC name (%s) not supported.\n", debugstr_w(filename) );
            SetLastError( ERROR_PATH_NOT_FOUND );
            return INVALID_HANDLE_VALUE;
	}
    }

    if (!strncmpW(filename, bkslashes_with_dotW, 4))
    {
        static const WCHAR pipeW[] = {'P','I','P','E','\\',0};
        if(!strncmpiW(filename + 4, pipeW, 5))
        {
            TRACE("Opening a pipe: %s\n", debugstr_w(filename));
            ret = FILE_OpenPipe( filename, access, sa );
            goto done;
        }
        else if (isalphaW(filename[4]) && filename[5] == ':' && filename[6] == '\0')
        {
            ret = FILE_CreateDevice( (toupperW(filename[4]) - 'A') | 0x20000, access, sa );
            goto done;
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
    if (!strncmpW(filename, bkslashesW, 2))
    {
        ret = SMB_CreateFileW(filename, access, sharing, sa, creation, attributes, template );
        goto done;
    }

    /* If the name contains a DOS wild card (* or ?), do no create a file */
    if(strchrW(filename, '*') || strchrW(filename, '?'))
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return INVALID_HANDLE_VALUE;
    }

    /* Open a console for CONIN$ or CONOUT$ */
    if (!strcmpiW(filename, coninW) || !strcmpiW(filename, conoutW))
    {
        ret = OpenConsoleW(filename, access, sa, creation);
        goto done;
    }

    if (DOSFS_GetDevice( filename ))
    {
        TRACE("opening device %s\n", debugstr_w(filename) );

        if (!(ret = DOSFS_OpenDevice( filename, access, attributes, sa )))
        {
            /* Do not silence this please. It is a critical error. -MM */
            ERR("Couldn't open device %s!\n", debugstr_w(filename));
            SetLastError( ERROR_FILE_NOT_FOUND );
        }
        goto done;
    }

    /* check for filename, don't check for last entry if creating */
    if (!DOSFS_GetFullName( filename,
			    (creation == OPEN_EXISTING) ||
			    (creation == TRUNCATE_EXISTING),
			    &full_name )) {
	WARN("Unable to get full filename from %s (GLE %ld)\n",
	     debugstr_w(filename), GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    ret = FILE_CreateFile( full_name.long_name, access, sharing,
                           sa, creation, attributes, template,
                           DRIVE_GetFlags(full_name.drive) & DRIVE_FAIL_READ_ONLY,
                           GetDriveTypeW( full_name.short_name ) );
 done:
    if (!ret) ret = INVALID_HANDLE_VALUE;
    TRACE("returning %p\n", ret);
    return ret;
}



/*************************************************************************
 *              CreateFileA              (KERNEL32.@)
 */
HANDLE WINAPI CreateFileA( LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template)
{
    UNICODE_STRING filenameW;
    HANDLE ret = INVALID_HANDLE_VALUE;

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&filenameW, filename))
    {
        ret = CreateFileW(filenameW.Buffer, access, sharing, sa, creation,
                          attributes, template);
        RtlFreeUnicodeString(&filenameW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
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

    RtlSecondsSince1970ToTime( st->st_mtime, (LARGE_INTEGER *)&info->ftCreationTime );
    RtlSecondsSince1970ToTime( st->st_mtime, (LARGE_INTEGER *)&info->ftLastWriteTime );
    RtlSecondsSince1970ToTime( st->st_atime, (LARGE_INTEGER *)&info->ftLastAccessTime );

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
BOOL FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info, BOOL *is_symlink_ptr )
{
    struct stat st;
    int is_symlink;
    LPCSTR p;

    if (lstat( unixName, &st ) == -1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    is_symlink = S_ISLNK(st.st_mode);
    if (is_symlink)
    {
        /* do a "real" stat to find out
	   about the type of the symlink destination */
        if (stat( unixName, &st ) == -1)
        {
            FILE_SetDosError();
            return FALSE;
        }
    }

    /* fill in the information we gathered so far */
    FILE_FillInfo( &st, info );

    /* and now see if this is a hidden file, based on the name */
    p = strrchr( unixName, '/');
    p = p ? p + 1 : unixName;
    if (*p == '.' && *(p+1)  && (*(p+1) != '.' || *(p+2)))
    {
	static const WCHAR wineW[] = {'w','i','n','e',0};
	static const WCHAR ShowDotFilesW[] = {'S','h','o','w','D','o','t','F','i','l','e','s',0};
	static int show_dot_files = -1;
	if (show_dot_files == -1)
	    show_dot_files = PROFILE_GetWineIniBool(wineW, ShowDotFilesW, 0);
	if (!show_dot_files)
	    info->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    }
    if (is_symlink_ptr) *is_symlink_ptr = is_symlink;
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

    TRACE("%p\n", hFile);

    SERVER_START_REQ( get_file_info )
    {
        req->handle = hFile;
        if ((ret = !wine_server_call_err( req )))
        {
            /* FIXME: which file types are supported ?
             * Serial ports (FILE_TYPE_CHAR) are not,
             * and MSDN also says that pipes are not supported.
             * FILE_TYPE_REMOTE seems to be supported according to
             * MSDN q234741.txt */
            if ((reply->type == FILE_TYPE_DISK) ||  (reply->type == FILE_TYPE_REMOTE))
            {
                RtlSecondsSince1970ToTime( reply->write_time, (LARGE_INTEGER *)&info->ftCreationTime );
                RtlSecondsSince1970ToTime( reply->write_time, (LARGE_INTEGER *)&info->ftLastWriteTime );
                RtlSecondsSince1970ToTime( reply->access_time, (LARGE_INTEGER *)&info->ftLastAccessTime );
                info->dwFileAttributes     = reply->attr;
                info->dwVolumeSerialNumber = reply->serial;
                info->nFileSizeHigh        = reply->size_high;
                info->nFileSizeLow         = reply->size_low;
                info->nNumberOfLinks       = reply->links;
                info->nFileIndexHigh       = reply->index_high;
                info->nFileIndexLow        = reply->index_low;
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
 *           GetFileAttributesW   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesW( LPCWSTR name )
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
    if (!FILE_Stat( full_name.long_name, &info, NULL )) return -1;
    return info.dwFileAttributes;
}


/**************************************************************************
 *           GetFileAttributesA   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesA( LPCSTR name )
{
    UNICODE_STRING nameW;
    DWORD ret = (DWORD)-1;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD)-1;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&nameW, name))
    {
        ret = GetFileAttributesW(nameW.Buffer);
        RtlFreeUnicodeString(&nameW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}


/**************************************************************************
 *              SetFileAttributes	(KERNEL.421)
 */
BOOL16 WINAPI SetFileAttributes16( LPCSTR lpFileName, DWORD attributes )
{
    return SetFileAttributesA( lpFileName, attributes );
}


/**************************************************************************
 *              SetFileAttributesW	(KERNEL32.@)
 */
BOOL WINAPI SetFileAttributesW(LPCWSTR lpFileName, DWORD attributes)
{
    struct stat buf;
    DOS_FULL_NAME full_name;

    if (!lpFileName)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE("(%s,%lx)\n", debugstr_w(lpFileName), attributes);

    if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name ))
        return FALSE;

    if(stat(full_name.long_name,&buf)==-1)
    {
        FILE_SetDosError();
        return FALSE;
    }
    if (attributes & FILE_ATTRIBUTE_READONLY)
    {
        if(S_ISDIR(buf.st_mode))
            /* FIXME */
            WARN("FILE_ATTRIBUTE_READONLY ignored for directory.\n");
        else
            buf.st_mode &= ~0222; /* octal!, clear write permission bits */
        attributes &= ~FILE_ATTRIBUTE_READONLY;
    }
    else
    {
        /* add write permission */
        buf.st_mode |= (0600 | ((buf.st_mode & 044) >> 1)) & (~FILE_umask);
    }
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (!S_ISDIR(buf.st_mode))
            FIXME("SetFileAttributes expected the file %s to be a directory\n",
                  debugstr_w(lpFileName));
        attributes &= ~FILE_ATTRIBUTE_DIRECTORY;
    }
    attributes &= ~(FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    if (attributes)
        FIXME("(%s):%lx attribute(s) not implemented.\n", debugstr_w(lpFileName), attributes);
    if (-1==chmod(full_name.long_name,buf.st_mode))
    {
        if (GetDriveTypeW(lpFileName) == DRIVE_CDROM)
        {
           SetLastError( ERROR_ACCESS_DENIED );
           return FALSE;
        }

        /*
        * FIXME: We don't return FALSE here because of differences between
        *        Linux and Windows privileges. Under Linux only the owner of
        *        the file is allowed to change file attributes. Under Windows,
        *        applications expect that if you can write to a file, you can also
        *        change its attributes (see GENERIC_WRITE). We could try to be
        *        clever here but that would break multi-user installations where
        *        users share read-only DLLs. This is because some installers like
        *        to change attributes of already installed DLLs.
        */
        FIXME("Couldn't set file attributes for existing file \"%s\".\n"
              "Check permissions or set VFAT \"quiet\" mount flag\n", full_name.long_name);
    }
    return TRUE;
}


/**************************************************************************
 *              SetFileAttributesA	(KERNEL32.@)
 */
BOOL WINAPI SetFileAttributesA(LPCSTR lpFileName, DWORD attributes)
{
    UNICODE_STRING filenameW;
    BOOL ret = FALSE;

    if (!lpFileName)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&filenameW, lpFileName))
    {
        ret = SetFileAttributesW(filenameW.Buffer, attributes);
        RtlFreeUnicodeString(&filenameW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
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
 *           GetFileSizeEx   (KERNEL32.@)
 */
BOOL WINAPI GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize )
{
    BY_HANDLE_FILE_INFORMATION info;

    if (!lpFileSize)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!GetFileInformationByHandle( hFile, &info ))
    {
        return FALSE;
    }

    lpFileSize->s.LowPart = info.nFileSizeLow;
    lpFileSize->s.HighPart = info.nFileSizeHigh;

    return TRUE;
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
 *           FILE_GetTempFileName : utility for GetTempFileName
 */
static UINT FILE_GetTempFileName( LPCWSTR path, LPCWSTR prefix, UINT unique,
                                  LPWSTR buffer )
{
    static UINT unique_temp;
    DOS_FULL_NAME full_name;
    int i;
    LPWSTR p;
    UINT num;
    char buf[20];

    if ( !path || !prefix || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!unique_temp) unique_temp = time(NULL) & 0xffff;
    num = unique ? (unique & 0xffff) : (unique_temp++ & 0xffff);

    strcpyW( buffer, path );
    p = buffer + strlenW(buffer);

    /* add a \, if there isn't one and path is more than just the drive letter ... */
    if ( !((strlenW(buffer) == 2) && (buffer[1] == ':'))
	&& ((p == buffer) || (p[-1] != '\\'))) *p++ = '\\';

    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    sprintf( buf, "%04x.tmp", num );
    MultiByteToWideChar(CP_ACP, 0, buf, -1, p, 20);

    /* Now try to create it */

    if (!unique)
    {
        do
        {
            HANDLE handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL,
                                         CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                TRACE("created %s\n", debugstr_w(buffer) );
                CloseHandle( handle );
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS &&
                GetLastError() != ERROR_SHARING_VIOLATION)
                break;  /* No need to go on */
            num++;
            sprintf( buf, "%04x.tmp", num );
            MultiByteToWideChar(CP_ACP, 0, buf, -1, p, 20);
        } while (num != (unique & 0xffff));
    }

    /* Get the full path name */

    if (DOSFS_GetFullName( buffer, FALSE, &full_name ))
    {
        char *slash;
        /* Check if we have write access in the directory */
        if ((slash = strrchr( full_name.long_name, '/' ))) *slash = '\0';
        if (access( full_name.long_name, W_OK ) == -1)
            WARN("returns %s, which doesn't seem to be writeable.\n",
                  debugstr_w(buffer) );
    }
    TRACE("returning %s\n", debugstr_w(buffer) );
    return unique ? unique : num;
}


/***********************************************************************
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique,
                                  LPSTR buffer)
{
    UNICODE_STRING pathW, prefixW;
    WCHAR bufferW[MAX_PATH];
    UINT ret;

    if ( !path || !prefix || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    RtlCreateUnicodeStringFromAsciiz(&pathW, path);
    RtlCreateUnicodeStringFromAsciiz(&prefixW, prefix);

    ret = GetTempFileNameW(pathW.Buffer, prefixW.Buffer, unique, bufferW);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);

    RtlFreeUnicodeString(&pathW);
    RtlFreeUnicodeString(&prefixW);
    return ret;
}

/***********************************************************************
 *           GetTempFileNameW   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique,
                                  LPWSTR buffer )
{
    return FILE_GetTempFileName( path, prefix, unique, buffer );
}


/***********************************************************************
 *           GetTempFileName   (KERNEL.97)
 */
UINT16 WINAPI GetTempFileName16( BYTE drive, LPCSTR prefix, UINT16 unique,
                                 LPSTR buffer )
{
    char temppath[MAX_PATH];
    char *prefix16 = NULL;
    UINT16 ret;

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
        GetTempPathA( MAX_PATH, temppath );

    if (prefix)
    {
        prefix16 = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + 2);
        *prefix16 = '~';
        strcpy(prefix16 + 1, prefix);
    }

    ret = GetTempFileNameA( temppath, prefix16, unique, buffer );

    if (prefix16) HeapFree(GetProcessHeap(), 0, prefix16);
    return ret;
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
    HANDLE handle;
    FILETIME filetime;
    WORD filedatetime[2];
    DOS_FULL_NAME full_name;
    DWORD access, sharing;
    WCHAR *p;
    WCHAR buffer[MAX_PATH];
    LPWSTR nameW;

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
        if ((handle = CreateFileA( name, GENERIC_READ | GENERIC_WRITE,
                                   sharing, NULL, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, 0 ))== INVALID_HANDLE_VALUE)
            goto error;
        goto success;
    }

    MultiByteToWideChar(CP_ACP, 0, name, -1, buffer, MAX_PATH);
    nameW = buffer;

    /* If OF_SEARCH is set, ignore the given path */

    if ((mode & OF_SEARCH) && !(mode & OF_REOPEN))
    {
        /* First try the file name as is */
        if (DOSFS_GetFullName( nameW, TRUE, &full_name )) goto found;
        /* Now remove the path */
        if (nameW[0] && (nameW[1] == ':')) nameW += 2;
        if ((p = strrchrW( nameW, '\\' ))) nameW = p + 1;
        if ((p = strrchrW( nameW, '/' ))) nameW = p + 1;
        if (!nameW[0]) goto not_found;
    }

    /* Now look for the file */

    if (!DIR_SearchPath( NULL, nameW, NULL, &full_name, win32 )) goto not_found;

found:
    TRACE("found %s = %s\n",
          full_name.long_name, debugstr_w(full_name.short_name) );
    WideCharToMultiByte(CP_ACP, 0, full_name.short_name, -1,
                        ofs->szPathName, sizeof(ofs->szPathName), NULL, NULL);

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

    handle = FILE_CreateFile( full_name.long_name, access, sharing,
                                NULL, OPEN_EXISTING, 0, 0,
                                DRIVE_GetFlags(full_name.drive) & DRIVE_FAIL_READ_ONLY,
                                GetDriveTypeW( full_name.short_name ) );
    if (!handle) goto not_found;

    GetFileTime( handle, NULL, NULL, &filetime );
    FileTimeToDosDateTime( &filetime, &filedatetime[0], &filedatetime[1] );
    if ((mode & OF_VERIFY) && (mode & OF_REOPEN))
    {
        if (ofs->Reserved1 != filedatetime[0] || ofs->Reserved2 != filedatetime[1] )
        {
            CloseHandle( handle );
            WARN("(%s): OF_VERIFY failed\n", name );
            /* FIXME: what error here? */
            SetLastError( ERROR_FILE_NOT_FOUND );
            goto error;
        }
    }
    ofs->Reserved1 = filedatetime[0];
    ofs->Reserved2 = filedatetime[1];

success:  /* We get here if the open was successful */
    TRACE("(%s): OK, return = %p\n", name, handle );
    if (win32)
    {
        hFileRet = (HFILE)handle;
        if (mode & OF_EXIST) /* Return the handle, but close it first */
            CloseHandle( handle );
    }
    else
    {
        hFileRet = Win32HandleToDosFileHandle( handle );
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
    HANDLE cp = GetCurrentProcess();
    DuplicateHandle(cp, GetStdHandle(STD_INPUT_HANDLE), cp, &dos_handles[0],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_OUTPUT_HANDLE), cp, &dos_handles[1],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[2],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[3],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(cp, GetStdHandle(STD_ERROR_HANDLE), cp, &dos_handles[4],
                    0, TRUE, DUPLICATE_SAME_ACCESS);
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
            TRACE("Got %d for h32 %p\n", i, handle );
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
    if ((hFile >= DOS_TABLE_SIZE) || !dos_handles[hFile])
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return HFILE_ERROR16;
    }
    TRACE("%d (handle32=%p)\n", hFile, dos_handles[hFile] );
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
    return CloseHandle( (HANDLE)hFile ) ? 0 : HFILE_ERROR;
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
 *  If successful (and relevant) lpTransferred will hold the number of
 *   bytes transferred during the async operation.
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
    LPDWORD lpTransferred,     /* [in/out] number of bytes transferred  */
    BOOL bWait                 /* [in] wait for the transfer to complete ? */
) {
    DWORD r;

    TRACE("(%p %p %p %x)\n", hFile, lpOverlapped, lpTransferred, bWait);

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

    if ( bWait )
    {
        do {
            TRACE("waiting on %p\n",lpOverlapped);
            r = WaitForSingleObjectEx(lpOverlapped->hEvent, INFINITE, TRUE);
            TRACE("wait on %p returned %ld\n",lpOverlapped,r);
        } while (r==STATUS_USER_APC);
    }
    else if ( lpOverlapped->Internal == STATUS_PENDING )
    {
        /* Wait in order to give APCs a chance to run. */
        /* This is cheating, so we must set the event again in case of success -
           it may be a non-manual reset event. */
        do {
            TRACE("waiting on %p\n",lpOverlapped);
            r = WaitForSingleObjectEx(lpOverlapped->hEvent, 0, TRUE);
            TRACE("wait on %p returned %ld\n",lpOverlapped,r);
        } while (r==STATUS_USER_APC);
        if ( r == WAIT_OBJECT_0 )
            NtSetEvent ( lpOverlapped->hEvent, NULL );
    }

    if(lpTransferred)
        *lpTransferred = lpOverlapped->InternalHigh;

    switch ( lpOverlapped->Internal )
    {
    case STATUS_SUCCESS:
        return TRUE;
    case STATUS_PENDING:
        SetLastError ( ERROR_IO_INCOMPLETE );
        if ( bWait ) ERR ("PENDING status after waiting!\n");
        return FALSE;
    default:
        SetLastError ( RtlNtStatusToDosError ( lpOverlapped->Internal ) );
        return FALSE;
    }
}

/***********************************************************************
 *             CancelIo                   (KERNEL32.@)
 */
BOOL WINAPI CancelIo(HANDLE handle)
{
    async_private *ovp,*t;

    TRACE("handle = %p\n",handle);

    for (ovp = NtCurrentTeb()->pending_list; ovp; ovp = t)
    {
        t = ovp->next;
        if ( ovp->handle == handle )
             cancel_async ( ovp );
    }
    WaitForMultipleObjectsEx(0,NULL,FALSE,1,TRUE);
    return TRUE;
}

/***********************************************************************
 *             FILE_AsyncReadService      (INTERNAL)
 *
 *  This function is called while the client is waiting on the
 *  server, so we can't make any server calls here.
 */
static void FILE_AsyncReadService(async_private *ovp)
{
    async_fileio *fileio = (async_fileio*) ovp;
    LPOVERLAPPED lpOverlapped = fileio->lpOverlapped;
    int result, r;
    int already = lpOverlapped->InternalHigh;

    TRACE("%p %p\n", lpOverlapped, fileio->buffer );

    /* check to see if the data is ready (non-blocking) */

    if ( fileio->fd_type == FD_TYPE_SOCKET )
        result = read (ovp->fd, &fileio->buffer[already], fileio->count - already);
    else
    {
        result = pread (ovp->fd, &fileio->buffer[already], fileio->count - already,
                        OVERLAPPED_OFFSET (lpOverlapped) + already);
        if ((result < 0) && (errno == ESPIPE))
            result = read (ovp->fd, &fileio->buffer[already], fileio->count - already);
    }

    if ( (result<0) && ((errno == EAGAIN) || (errno == EINTR)))
    {
        TRACE("Deferred read %d\n",errno);
        r = STATUS_PENDING;
        goto async_end;
    }

    /* check to see if the transfer is complete */
    if(result<0)
    {
        r = FILE_GetNtStatus ();
        goto async_end;
    }
    else if ( result == 0 )
    {
        r = ( lpOverlapped->InternalHigh ? STATUS_SUCCESS : STATUS_END_OF_FILE );
        goto async_end;
    }

    lpOverlapped->InternalHigh += result;
    TRACE("read %d more bytes %ld/%d so far\n",result,lpOverlapped->InternalHigh,fileio->count);

    if(lpOverlapped->InternalHigh >= fileio->count || fileio->fd_type == FD_TYPE_SOCKET )
        r = STATUS_SUCCESS;
    else
        r = STATUS_PENDING;

async_end:
    lpOverlapped->Internal = r;
}

/***********************************************************************
 *              FILE_ReadFileEx                (INTERNAL)
 */
static BOOL FILE_ReadFileEx(HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
			 LPOVERLAPPED overlapped,
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
                         HANDLE hEvent)
{
    async_fileio *ovp;
    int fd;
    int flags;
    enum fd_type type;

    TRACE("file %p to buf %p num %ld %p func %p\n",
	  hFile, buffer, bytesToRead, overlapped, lpCompletionRoutine);

    /* check that there is an overlapped struct */
    if (overlapped==NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fd = FILE_GetUnixHandleType ( hFile, GENERIC_READ, &type, &flags);
    if ( fd < 0 )
    {
        WARN ( "Couldn't get FD\n" );
        return FALSE;
    }

    ovp = (async_fileio*) HeapAlloc(GetProcessHeap(), 0, sizeof (async_fileio));
    if(!ovp)
    {
        TRACE("HeapAlloc Failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }

    ovp->async.ops = ( lpCompletionRoutine ? &fileio_async_ops : &fileio_nocomp_async_ops );
    ovp->async.handle = hFile;
    ovp->async.fd = fd;
    ovp->async.type = ASYNC_TYPE_READ;
    ovp->async.func = FILE_AsyncReadService;
    ovp->async.event = hEvent;
    ovp->lpOverlapped = overlapped;
    ovp->count = bytesToRead;
    ovp->completion_func = lpCompletionRoutine;
    ovp->buffer = buffer;
    ovp->fd_type = type;

    return !register_new_async (&ovp->async);

error:
    close (fd);
    return FALSE;

}

/***********************************************************************
 *              ReadFileEx                (KERNEL32.@)
 */
BOOL WINAPI ReadFileEx(HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
			 LPOVERLAPPED overlapped,
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    overlapped->InternalHigh = 0;
    return FILE_ReadFileEx(hFile,buffer,bytesToRead,overlapped,lpCompletionRoutine, INVALID_HANDLE_VALUE);
}

static BOOL FILE_TimeoutRead(HANDLE hFile, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead)
{
    OVERLAPPED ov;
    BOOL r = FALSE;

    TRACE("%p %p %ld %p\n", hFile, buffer, bytesToRead, bytesRead );

    ZeroMemory(&ov, sizeof (OVERLAPPED));
    if(STATUS_SUCCESS==NtCreateEvent(&ov.hEvent, SYNCHRONIZE, NULL, 0, 0))
    {
        if(FILE_ReadFileEx(hFile, buffer, bytesToRead, &ov, NULL, ov.hEvent))
        {
            r = GetOverlappedResult(hFile, &ov, bytesRead, TRUE);
        }
    }
    CloseHandle(ov.hEvent);
    return r;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.@)
 */
BOOL WINAPI ReadFile( HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                        LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    int unix_handle, result, flags;
    enum fd_type type;

    TRACE("%p %p %ld %p %p\n", hFile, buffer, bytesToRead,
          bytesRead, overlapped );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    if (is_console_handle(hFile))
	return FILE_ReadConsole(hFile, buffer, bytesToRead, bytesRead, NULL);

    unix_handle = FILE_GetUnixHandleType( hFile, GENERIC_READ, &type, &flags );

    if (flags & FD_FLAG_OVERLAPPED)
    {
	if (unix_handle == -1) return FALSE;
        if ( (overlapped==NULL) || NtResetEvent( overlapped->hEvent, NULL ) )
        {
            TRACE("Overlapped not specified or invalid event flag\n");
	    close(unix_handle);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        close(unix_handle);
        overlapped->InternalHigh = 0;

        if(!FILE_ReadFileEx(hFile, buffer, bytesToRead, overlapped, NULL, overlapped->hEvent))
            return FALSE;

        if ( !GetOverlappedResult (hFile, overlapped, bytesRead, FALSE) )
        {
            if ( GetLastError() == ERROR_IO_INCOMPLETE )
                SetLastError ( ERROR_IO_PENDING );
            return FALSE;
        }

        return TRUE;
    }
    if (flags & FD_FLAG_TIMEOUT)
    {
        close(unix_handle);
        return FILE_TimeoutRead(hFile, buffer, bytesToRead, bytesRead);
    }
    switch(type)
    {
    case FD_TYPE_SMB:
        return SMB_ReadFile(hFile, buffer, bytesToRead, bytesRead, NULL);

    case FD_TYPE_DEFAULT:
        /* normal unix files */
        if (unix_handle == -1) return FALSE;
        if (overlapped)
        {
            DWORD highOffset = overlapped->OffsetHigh;
            if ( (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, overlapped->Offset,
                                                             &highOffset, FILE_BEGIN)) &&
                 (GetLastError() != NO_ERROR) )
            {
              close(unix_handle);
              return FALSE;
            }
        }
        break;

    default:
	if (unix_handle == -1)
	    return FALSE;
    }

    if(overlapped)
    {
	off_t offset = OVERLAPPED_OFFSET(overlapped);
	if(lseek(unix_handle, offset, SEEK_SET) == -1)
	{
	    close(unix_handle);
	    SetLastError(ERROR_INVALID_PARAMETER);
	    return FALSE;
	}
    }

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
static void FILE_AsyncWriteService(struct async_private *ovp)
{
    async_fileio *fileio = (async_fileio *) ovp;
    LPOVERLAPPED lpOverlapped = fileio->lpOverlapped;
    int result, r;
    int already = lpOverlapped->InternalHigh;

    TRACE("(%p %p)\n",lpOverlapped,fileio->buffer);

    /* write some data (non-blocking) */

    if ( fileio->fd_type == FD_TYPE_SOCKET )
        result = write(ovp->fd, &fileio->buffer[already], fileio->count - already);
    else
    {
        result = pwrite(ovp->fd, &fileio->buffer[already], fileio->count - already,
                    OVERLAPPED_OFFSET (lpOverlapped) + already);
        if ((result < 0) && (errno == ESPIPE))
            result = write(ovp->fd, &fileio->buffer[already], fileio->count - already);
    }

    if ( (result<0) && ((errno == EAGAIN) || (errno == EINTR)))
    {
        r = STATUS_PENDING;
        goto async_end;
    }

    /* check to see if the transfer is complete */
    if(result<0)
    {
        r = FILE_GetNtStatus ();
        goto async_end;
    }

    lpOverlapped->InternalHigh += result;

    TRACE("wrote %d more bytes %ld/%d so far\n",result,lpOverlapped->InternalHigh,fileio->count);

    if(lpOverlapped->InternalHigh < fileio->count)
        r = STATUS_PENDING;
    else
        r = STATUS_SUCCESS;

async_end:
    lpOverlapped->Internal = r;
}

/***********************************************************************
 *              FILE_WriteFileEx
 */
static BOOL FILE_WriteFileEx(HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                             LPOVERLAPPED overlapped,
                             LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
                             HANDLE hEvent)
{
    async_fileio *ovp;
    int fd;
    int flags;
    enum fd_type type;

    TRACE("file %p to buf %p num %ld %p func %p handle %p\n",
	  hFile, buffer, bytesToWrite, overlapped, lpCompletionRoutine, hEvent);

    if (overlapped == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fd = FILE_GetUnixHandleType ( hFile, GENERIC_WRITE, &type, &flags );
    if ( fd < 0 )
    {
        TRACE( "Couldn't get FD\n" );
        return FALSE;
    }

    ovp = (async_fileio*) HeapAlloc(GetProcessHeap(), 0, sizeof (async_fileio));
    if(!ovp)
    {
        TRACE("HeapAlloc Failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }

    ovp->async.ops = ( lpCompletionRoutine ? &fileio_async_ops : &fileio_nocomp_async_ops );
    ovp->async.handle = hFile;
    ovp->async.fd = fd;
    ovp->async.type = ASYNC_TYPE_WRITE;
    ovp->async.func = FILE_AsyncWriteService;
    ovp->lpOverlapped = overlapped;
    ovp->async.event = hEvent;
    ovp->buffer = (LPVOID) buffer;
    ovp->count = bytesToWrite;
    ovp->completion_func = lpCompletionRoutine;
    ovp->fd_type = type;

    return !register_new_async (&ovp->async);

error:
    close (fd);
    return FALSE;
}

/***********************************************************************
 *              WriteFileEx                (KERNEL32.@)
 */
BOOL WINAPI WriteFileEx(HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
			 LPOVERLAPPED overlapped,
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    overlapped->InternalHigh = 0;

    return FILE_WriteFileEx(hFile, buffer, bytesToWrite, overlapped, lpCompletionRoutine, INVALID_HANDLE_VALUE);
}

/***********************************************************************
 *             WriteFile               (KERNEL32.@)
 */
BOOL WINAPI WriteFile( HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                         LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    int unix_handle, result, flags;
    enum fd_type type;

    TRACE("%p %p %ld %p %p\n", hFile, buffer, bytesToWrite,
          bytesWritten, overlapped );

    if (bytesWritten) *bytesWritten = 0;  /* Do this before anything else */
    if (!bytesToWrite) return TRUE;

    if (is_console_handle(hFile))
	return FILE_WriteConsole(hFile, buffer, bytesToWrite, bytesWritten, NULL);

    unix_handle = FILE_GetUnixHandleType( hFile, GENERIC_WRITE, &type, &flags );

    if (flags & FD_FLAG_OVERLAPPED)
    {
	if (unix_handle == -1) return FALSE;
        if ( (overlapped==NULL) || NtResetEvent( overlapped->hEvent, NULL ) )
        {
            TRACE("Overlapped not specified or invalid event flag\n");
	    close(unix_handle);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        close(unix_handle);
        overlapped->InternalHigh = 0;

        if(!FILE_WriteFileEx(hFile, buffer, bytesToWrite, overlapped, NULL, overlapped->hEvent))
            return FALSE;

        if ( !GetOverlappedResult (hFile, overlapped, bytesWritten, FALSE) )
        {
            if ( GetLastError() == ERROR_IO_INCOMPLETE )
                SetLastError ( ERROR_IO_PENDING );
            return FALSE;
        }

        return TRUE;
    }

    switch(type)
    {
    case FD_TYPE_DEFAULT:
        if (unix_handle == -1) return FALSE;

        if(overlapped)
        {
            DWORD highOffset = overlapped->OffsetHigh;
            if ( (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, overlapped->Offset,
                                                             &highOffset, FILE_BEGIN)) &&
                 (GetLastError() != NO_ERROR) )
            {
              close(unix_handle);
              return FALSE;
            }
        }
        break;

    default:
        if (unix_handle == -1)
            return FALSE;
        if (overlapped)
        {
            close(unix_handle);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;
    }

    if(overlapped)
    {
	off_t offset = OVERLAPPED_OFFSET(overlapped);
	if(lseek(unix_handle, offset, SEEK_SET) == -1)
	{
	    close(unix_handle);
	    SetLastError(ERROR_INVALID_PARAMETER);
	    return FALSE;
	}
    }

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
    return _lread((HFILE)DosFileHandleToWin32Handle(hFile), MapSL(buffer), count );
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
    if (!ReadFile( (HANDLE)handle, buffer, count, &result, NULL )) return -1;
    return result;
}


/***********************************************************************
 *           _lread16   (KERNEL.82)
 */
UINT16 WINAPI _lread16( HFILE16 hFile, LPVOID buffer, UINT16 count )
{
    return (UINT16)_lread((HFILE)DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
}


/***********************************************************************
 *           _lcreat   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    return Win32HandleToDosFileHandle( (HANDLE)_lcreat( path, attr ) );
}


/***********************************************************************
 *           _lcreat   (KERNEL32.@)
 */
HFILE WINAPI _lcreat( LPCSTR path, INT attr )
{
    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    TRACE("%s %02x\n", path, attr );
    return (HFILE)CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, attr, 0 );
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.@)
 */
DWORD WINAPI SetFilePointer( HANDLE hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    DWORD ret = INVALID_SET_FILE_POINTER;

    TRACE("handle %p offset %ld high %ld origin %ld\n",
          hFile, distance, highword?*highword:0, method );

    SERVER_START_REQ( set_file_pointer )
    {
        req->handle = hFile;
        req->low = distance;
        req->high = highword ? *highword : (distance >= 0) ? 0 : -1;
        /* FIXME: assumes 1:1 mapping between Windows and Unix seek constants */
        req->whence = method;
        SetLastError( 0 );
        if (!wine_server_call_err( req ))
        {
            ret = reply->new_low;
            if (highword) *highword = reply->new_high;
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
    return SetFilePointer( (HANDLE)hFile, lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lopen   (KERNEL.85)
 */
HFILE16 WINAPI _lopen16( LPCSTR path, INT16 mode )
{
    return Win32HandleToDosFileHandle( (HANDLE)_lopen( path, mode ) );
}


/***********************************************************************
 *           _lopen   (KERNEL32.@)
 */
HFILE WINAPI _lopen( LPCSTR path, INT mode )
{
    DWORD access, sharing;

    TRACE("('%s',%04x)\n", path, mode );
    FILE_ConvertOFMode( mode, &access, &sharing );
    return (HFILE)CreateFileA( path, access, sharing, NULL, OPEN_EXISTING, 0, 0 );
}


/***********************************************************************
 *           _lwrite   (KERNEL.86)
 */
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
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
    return _lread( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, count );
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
    return _hwrite( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, count );
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
        if (!SetEndOfFile( (HANDLE)handle )) return HFILE_ERROR;
        return 0;
    }
    if (!WriteFile( (HANDLE)handle, buffer, count, &result, NULL ))
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
    NTSTATUS            nts;
    IO_STATUS_BLOCK     ioblk;

    if (is_console_handle( hFile ))
    {
        /* this will fail (as expected) for an output handle */
        /* FIXME: wait until FlushFileBuffers is moved to dll/kernel */
        /* return FlushConsoleInputBuffer( hFile ); */
        return TRUE;
    }
    nts = NtFlushBuffersFile( hFile, &ioblk );
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        return FALSE;
    }

    return TRUE;
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
        ret = !wine_server_call_err( req );
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
 *           DeleteFileW   (KERNEL32.@)
 */
BOOL WINAPI DeleteFileW( LPCWSTR path )
{
    DOS_FULL_NAME full_name;
    HANDLE hFile;

    TRACE("%s\n", debugstr_w(path) );
    if (!path || !*path)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    if (DOSFS_GetDevice( path ))
    {
        WARN("cannot remove DOS device %s!\n", debugstr_w(path));
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }

    if (!DOSFS_GetFullName( path, TRUE, &full_name )) return FALSE;

    /* check if we are allowed to delete the source */
    hFile = FILE_CreateFile( full_name.long_name, GENERIC_READ|GENERIC_WRITE, 0,
                             NULL, OPEN_EXISTING, 0, 0, TRUE,
                             GetDriveTypeW( full_name.short_name ) );
    if (!hFile) return FALSE;

    if (unlink( full_name.long_name ) == -1)
    {
        FILE_SetDosError();
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);
    return TRUE;
}


/***********************************************************************
 *           DeleteFileA   (KERNEL32.@)
 */
BOOL WINAPI DeleteFileA( LPCSTR path )
{
    UNICODE_STRING pathW;
    BOOL ret = FALSE;

    if (!path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&pathW, path))
    {
        ret = DeleteFileW(pathW.Buffer);
        RtlFreeUnicodeString(&pathW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}


/***********************************************************************
 *           GetFileType   (KERNEL32.@)
 */
DWORD WINAPI GetFileType( HANDLE hFile )
{
    DWORD ret = FILE_TYPE_UNKNOWN;

    if (is_console_handle( hFile ))
        return FILE_TYPE_CHAR;

    SERVER_START_REQ( get_file_info )
    {
        req->handle = hFile;
        if (!wine_server_call_err( req )) ret = reply->type;
    }
    SERVER_END_REQ;
    return ret;
}


/* check if a file name is for an executable file (.exe or .com) */
inline static BOOL is_executable( const char *name )
{
    int len = strlen(name);

    if (len < 4) return FALSE;
    return (!strcasecmp( name + len - 4, ".exe" ) ||
            !strcasecmp( name + len - 4, ".com" ));
}


/***********************************************************************
 *           FILE_AddBootRenameEntry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registrykey is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL FILE_AddBootRenameEntry( LPCWSTR fn1, LPCWSTR fn2, DWORD flags )
{
    static const WCHAR PreString[] = {'\\','?','?','\\',0};
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HKEY Reboot = 0;
    DWORD len0, len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, SessionW );

    if (NtCreateKey( &Reboot, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        WARN("Error creating key for reboot managment [%s]\n",
             "SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        return FALSE;
    }

    len0 = strlenW(PreString);
    len1 = strlenW(fn1) + len0 + 1;
    if (fn2)
    {
        len2 = strlenW(fn2) + len0 + 1;
        if (flags & MOVEFILE_REPLACE_EXISTING) len2++; /* Plus 1 because of the leading '!' */
    }
    else len2 = 1; /* minimum is the 0 characters for the empty second string */

    /* convert characters to bytes */
    len0 *= sizeof(WCHAR);
    len1 *= sizeof(WCHAR);
    len2 *= sizeof(WCHAR);

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                         NULL, 0, &DataSize ) == STATUS_BUFFER_OVERFLOW)
    {
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
        if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                             Buffer, DataSize, &DataSize )) goto Quit;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)Buffer;
        if (info->Type != REG_MULTI_SZ) goto Quit;
        if (DataSize > sizeof(info)) DataSize -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        DataSize = info_size;
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
    }

    p = (WCHAR *)(Buffer + DataSize);
    strcpyW( p, PreString );
    strcatW( p, fn1 );
    DataSize += len1;
    if (fn2)
    {
        p = (WCHAR *)(Buffer + DataSize);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        strcpyW( p, PreString );
        strcatW( p, fn2 );
        DataSize += len2;
    }
    else
    {
        p = (WCHAR *)(Buffer + DataSize);
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    if (Reboot) NtClose(Reboot);
    if (Buffer) HeapFree( GetProcessHeap(), 0, Buffer );
    return(rc);
}


/**************************************************************************
 *           MoveFileExW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExW( LPCWSTR fn1, LPCWSTR fn2, DWORD flag )
{
    DOS_FULL_NAME full_name1, full_name2;
    HANDLE hFile;
    DWORD attr = INVALID_FILE_ATTRIBUTES;

    TRACE("(%s,%s,%04lx)\n", debugstr_w(fn1), debugstr_w(fn2), flag);

    /* FIXME: <Gerhard W. Gruber>sparhawk@gmx.at
       In case of W9x and lesser this function should return 120 (ERROR_CALL_NOT_IMPLEMENTED)
       to be really compatible. Most programs wont have any problems though. In case
       you encounter one, this is what you should return here. I don't know what's up
       with NT 3.5. Is this function available there or not?
       Does anybody really care about 3.5? :)
    */

    /* Filename1 has to be always set to a valid path. Filename2 may be NULL
       if the source file has to be deleted.
    */
    if (!fn1) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* This function has to be run through in order to process the name properly.
       If the BOOTDELAY flag is set, the file doesn't need to exist though. At least
       that is the behaviour on NT 4.0. The operation accepts the filenames as
       they are given but it can't reply with a reasonable returncode. Success
       means in that case success for entering the values into the registry.
    */
    if(!DOSFS_GetFullName( fn1, TRUE, &full_name1 ))
    {
        if(!(flag & MOVEFILE_DELAY_UNTIL_REBOOT))
            return FALSE;
    }

    if (fn2)  /* !fn2 means delete fn1 */
    {
        if (DOSFS_GetFullName( fn2, TRUE, &full_name2 ))
        {
            if(!(flag & MOVEFILE_DELAY_UNTIL_REBOOT))
            {
                /* target exists, check if we may overwrite */
                if (!(flag & MOVEFILE_REPLACE_EXISTING))
                {
                    SetLastError( ERROR_FILE_EXISTS );
                    return FALSE;
                }
            }
        }
        else
        {
            if (!DOSFS_GetFullName( fn2, FALSE, &full_name2 ))
            {
                if(!(flag & MOVEFILE_DELAY_UNTIL_REBOOT))
                    return FALSE;
            }
        }

        /* Source name and target path are valid */

        if (flag & MOVEFILE_DELAY_UNTIL_REBOOT)
        {
            return FILE_AddBootRenameEntry( fn1, fn2, flag );
        }

        attr = GetFileAttributesW( fn1 );
        if ( attr == INVALID_FILE_ATTRIBUTES ) return FALSE;

        /* check if we are allowed to rename the source */
        hFile = FILE_CreateFile( full_name1.long_name, 0, 0,
                                 NULL, OPEN_EXISTING, 0, 0, TRUE,
                                 GetDriveTypeW( full_name1.short_name ) );
        if (!hFile)
        {
            if (GetLastError() != ERROR_ACCESS_DENIED) return FALSE;
            if ( !(attr & FILE_ATTRIBUTE_DIRECTORY) ) return FALSE;
            /* if it's a directory we can continue */
        }
        else CloseHandle(hFile);

        /* check, if we are allowed to delete the destination,
        **     (but the file not being there is fine) */
        hFile = FILE_CreateFile( full_name2.long_name, GENERIC_READ|GENERIC_WRITE, 0,
                                 NULL, OPEN_EXISTING, 0, 0, TRUE,
                                 GetDriveTypeW( full_name2.short_name ) );
        if(!hFile && GetLastError() != ERROR_FILE_NOT_FOUND) return FALSE;
        CloseHandle(hFile);

        if (full_name1.drive != full_name2.drive)
        {
            if (!(flag & MOVEFILE_COPY_ALLOWED))
            {
                SetLastError( ERROR_NOT_SAME_DEVICE );
                return FALSE;
            }
            else if ( attr & FILE_ATTRIBUTE_DIRECTORY )
            {
                /* Strange, but that's what Windows returns */
                SetLastError ( ERROR_ACCESS_DENIED );
                return FALSE;
            }
        }
        if (rename( full_name1.long_name, full_name2.long_name ) == -1)
            /* Try copy/delete unless it's a directory. */
            /* FIXME: This does not handle the (unlikely) case that the two locations
               are on the same Wine drive, but on different Unix file systems. */
        {
            if ( attr & FILE_ATTRIBUTE_DIRECTORY )
            {
                FILE_SetDosError();
                return FALSE;
            }
            else
            {
                if ( ! CopyFileW( fn1, fn2, !(flag & MOVEFILE_REPLACE_EXISTING) ))
                    return FALSE;
                if ( ! DeleteFileW ( fn1 ) )
                    return FALSE;
            }
        }
        if (is_executable( full_name1.long_name ) != is_executable( full_name2.long_name ))
        {
            struct stat fstat;
            if (stat( full_name2.long_name, &fstat ) != -1)
            {
                if (is_executable( full_name2.long_name ))
                    /* set executable bit where read bit is set */
                    fstat.st_mode |= (fstat.st_mode & 0444) >> 2;
                else
                    fstat.st_mode &= ~0111;
                chmod( full_name2.long_name, fstat.st_mode );
            }
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

            return FILE_AddBootRenameEntry( fn1, NULL, flag );
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
 *           MoveFileExA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileExA( LPCSTR fn1, LPCSTR fn2, DWORD flag )
{
    UNICODE_STRING fn1W, fn2W;
    BOOL ret;

    if (!fn1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz(&fn1W, fn1);
    if (fn2) RtlCreateUnicodeStringFromAsciiz(&fn2W, fn2);
    else fn2W.Buffer = NULL;

    ret = MoveFileExW( fn1W.Buffer, fn2W.Buffer, flag );

    RtlFreeUnicodeString(&fn1W);
    RtlFreeUnicodeString(&fn2W);
    return ret;
}


/**************************************************************************
 *           MoveFileW   (KERNEL32.@)
 *
 *  Move file or directory
 */
BOOL WINAPI MoveFileW( LPCWSTR fn1, LPCWSTR fn2 )
{
    return MoveFileExW( fn1, fn2, MOVEFILE_COPY_ALLOWED );
}


/**************************************************************************
 *           MoveFileA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileA( LPCSTR fn1, LPCSTR fn2 )
{
    return MoveFileExA( fn1, fn2, MOVEFILE_COPY_ALLOWED );
}


/**************************************************************************
 *           CopyFileW   (KERNEL32.@)
 */
BOOL WINAPI CopyFileW( LPCWSTR source, LPCWSTR dest, BOOL fail_if_exists )
{
    HANDLE h1, h2;
    BY_HANDLE_FILE_INFORMATION info;
    DWORD count;
    BOOL ret = FALSE;
    char buffer[2048];

    if (!source || !dest)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("%s -> %s\n", debugstr_w(source), debugstr_w(dest));

    if ((h1 = CreateFileW(source, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open source %s\n", debugstr_w(source));
        return FALSE;
    }

    if (!GetFileInformationByHandle( h1, &info ))
    {
        WARN("GetFileInformationByHandle returned error for %s\n", debugstr_w(source));
        CloseHandle( h1 );
        return FALSE;
    }

    if ((h2 = CreateFileW( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             fail_if_exists ? CREATE_NEW : CREATE_ALWAYS,
                             info.dwFileAttributes, h1 )) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open dest %s\n", debugstr_w(dest));
        CloseHandle( h1 );
        return FALSE;
    }

    while (ReadFile( h1, buffer, sizeof(buffer), &count, NULL ) && count)
    {
        char *p = buffer;
        while (count != 0)
        {
            DWORD res;
            if (!WriteFile( h2, p, count, &res, NULL ) || !res) goto done;
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
 *           CopyFileA   (KERNEL32.@)
 */
BOOL WINAPI CopyFileA( LPCSTR source, LPCSTR dest, BOOL fail_if_exists)
{
    UNICODE_STRING sourceW, destW;
    BOOL ret;

    if (!source || !dest)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz(&sourceW, source);
    RtlCreateUnicodeStringFromAsciiz(&destW, dest);

    ret = CopyFileW(sourceW.Buffer, destW.Buffer, fail_if_exists);

    RtlFreeUnicodeString(&sourceW);
    RtlFreeUnicodeString(&destW);
    return ret;
}


/**************************************************************************
 *           CopyFileExW   (KERNEL32.@)
 *
 * This implementation ignores most of the extra parameters passed-in into
 * the "ex" version of the method and calls the CopyFile method.
 * It will have to be fixed eventually.
 */
BOOL WINAPI CopyFileExW(LPCWSTR sourceFilename, LPCWSTR destFilename,
                        LPPROGRESS_ROUTINE progressRoutine, LPVOID appData,
                        LPBOOL cancelFlagPointer, DWORD copyFlags)
{
    /*
     * Interpret the only flag that CopyFile can interpret.
     */
    return CopyFileW(sourceFilename, destFilename, (copyFlags & COPY_FILE_FAIL_IF_EXISTS) != 0);
}

/**************************************************************************
 *           CopyFileExA   (KERNEL32.@)
 */
BOOL WINAPI CopyFileExA(LPCSTR sourceFilename, LPCSTR destFilename,
                        LPPROGRESS_ROUTINE progressRoutine, LPVOID appData,
                        LPBOOL cancelFlagPointer, DWORD copyFlags)
{
    UNICODE_STRING sourceW, destW;
    BOOL ret;

    if (!sourceFilename || !destFilename)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz(&sourceW, sourceFilename);
    RtlCreateUnicodeStringFromAsciiz(&destW, destFilename);

    ret = CopyFileExW(sourceW.Buffer, destW.Buffer, progressRoutine, appData,
                      cancelFlagPointer, copyFlags);

    RtlFreeUnicodeString(&sourceW);
    RtlFreeUnicodeString(&destW);
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
            RtlTimeToSecondsSince1970( (PLARGE_INTEGER) lpLastAccessTime, (DWORD *)&req->access_time );
        else
            req->access_time = 0; /* FIXME */
        if (lpLastWriteTime)
            RtlTimeToSecondsSince1970( (PLARGE_INTEGER) lpLastWriteTime, (DWORD *)&req->write_time );
        else
            req->write_time = 0; /* FIXME */
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           LockFile   (KERNEL32.@)
 */
BOOL WINAPI LockFile( HANDLE hFile, DWORD offset_low, DWORD offset_high,
                      DWORD count_low, DWORD count_high )
{
    BOOL ret;

    TRACE( "%p %lx%08lx %lx%08lx\n", hFile, offset_high, offset_low, count_high, count_low );

    SERVER_START_REQ( lock_file )
    {
        req->handle      = hFile;
        req->offset_low  = offset_low;
        req->offset_high = offset_high;
        req->count_low   = count_low;
        req->count_high  = count_high;
        req->shared      = FALSE;
        req->wait        = FALSE;
        ret = !wine_server_call_err( req );
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
                        DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    NTSTATUS err;
    BOOL async;
    HANDLE handle;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "%p %lx%08lx %lx%08lx flags %lx\n",
           hFile, overlapped->OffsetHigh, overlapped->Offset, count_high, count_low, flags );

    for (;;)
    {
        SERVER_START_REQ( lock_file )
        {
            req->handle      = hFile;
            req->offset_low  = overlapped->Offset;
            req->offset_high = overlapped->OffsetHigh;
            req->count_low   = count_low;
            req->count_high  = count_high;
            req->shared      = !(flags & LOCKFILE_EXCLUSIVE_LOCK);
            req->wait        = !(flags & LOCKFILE_FAIL_IMMEDIATELY);
            err = wine_server_call( req );
            handle = reply->handle;
            async  = reply->overlapped;
        }
        SERVER_END_REQ;
        if (err != STATUS_PENDING)
        {
            if (err) SetLastError( RtlNtStatusToDosError(err) );
            return !err;
        }
        if (async)
        {
            FIXME( "Async I/O lock wait not implemented, might deadlock\n" );
            if (handle) CloseHandle( handle );
            SetLastError( ERROR_IO_PENDING );
            return FALSE;
        }
        if (handle)
        {
            WaitForSingleObject( handle, INFINITE );
            CloseHandle( handle );
        }
        else Sleep(100);  /* Unix lock conflict, sleep a bit and retry */
    }
}


/**************************************************************************
 *           UnlockFile   (KERNEL32.@)
 */
BOOL WINAPI UnlockFile( HANDLE hFile, DWORD offset_low, DWORD offset_high,
                        DWORD count_low, DWORD count_high )
{
    BOOL ret;

    TRACE( "%p %lx%08lx %lx%08lx\n", hFile, offset_high, offset_low, count_high, count_low );

    SERVER_START_REQ( unlock_file )
    {
        req->handle      = hFile;
        req->offset_low  = offset_low;
        req->offset_high = offset_high;
        req->count_low   = count_low;
        req->count_high  = count_high;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *           UnlockFileEx   (KERNEL32.@)
 */
BOOL WINAPI UnlockFileEx( HANDLE hFile, DWORD reserved, DWORD count_low, DWORD count_high,
                          LPOVERLAPPED overlapped )
{
    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return UnlockFile( hFile, overlapped->Offset, overlapped->OffsetHigh, count_low, count_high );
}


/**************************************************************************
 *           GetFileAttributesExW   (KERNEL32.@)
 */
BOOL WINAPI GetFileAttributesExW(
	LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
    DOS_FULL_NAME full_name;
    BY_HANDLE_FILE_INFORMATION info;

    if (!lpFileName || !lpFileInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (fInfoLevelId == GetFileExInfoStandard) {
	LPWIN32_FILE_ATTRIBUTE_DATA lpFad =
	    (LPWIN32_FILE_ATTRIBUTE_DATA) lpFileInformation;
	if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name )) return FALSE;
	if (!FILE_Stat( full_name.long_name, &info, NULL )) return FALSE;

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
 *           GetFileAttributesExA   (KERNEL32.@)
 */
BOOL WINAPI GetFileAttributesExA(
	LPCSTR filename, GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation)
{
    UNICODE_STRING filenameW;
    BOOL ret = FALSE;

    if (!filename || !lpFileInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&filenameW, filename))
    {
        ret = GetFileAttributesExW(filenameW.Buffer, fInfoLevelId, lpFileInformation);
        RtlFreeUnicodeString(&filenameW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}

/**************************************************************************
 *           ReplaceFileW   (KERNEL32.@)
 *           ReplaceFile    (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileW(LPCWSTR lpReplacedFileName,LPCWSTR lpReplacementFileName,
                         LPCWSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    FIXME("(%s,%s,%s,%08lx,%p,%p) stub\n",debugstr_w(lpReplacedFileName),debugstr_w(lpReplacementFileName),
                                          debugstr_w(lpBackupFileName),dwReplaceFlags,lpExclude,lpReserved);
    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
    return FALSE;
}

/**************************************************************************
 *           ReplaceFileA (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileA(LPCSTR lpReplacedFileName,LPCSTR lpReplacementFileName,
                         LPCSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    FIXME("(%s,%s,%s,%08lx,%p,%p) stub\n",lpReplacedFileName,lpReplacementFileName,
                                          lpBackupFileName,dwReplaceFlags,lpExclude,lpReserved);
    SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT);
    return FALSE;
}
