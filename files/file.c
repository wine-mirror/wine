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
#include <stdarg.h>
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
#ifdef HAVE_IO_H
# include <io.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "wine/server.h"

#include "file.h"
#include "wincon.h"
#include "kernel_private.h"

#include "smb.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#if defined(MAP_ANONYMOUS) && !defined(MAP_ANON)
#define MAP_ANON MAP_ANONYMOUS
#endif

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

#define SECSPERDAY         86400
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)

/***********************************************************************
 *              FILE_ConvertOFMode
 *
 * Convert OF_* mode into flags for CreateFile.
 */
void FILE_ConvertOFMode( INT mode, DWORD *access, DWORD *sharing )
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
        SetLastError( ERROR_TOO_MANY_OPEN_FILES );
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
    case ENOTDIR:
        SetLastError( ERROR_PATH_NOT_FOUND );
        break;
    case EXDEV:
        SetLastError( ERROR_NOT_SAME_DEVICE );
        break;
    default:
        WARN("unknown file error: %s\n", strerror(save_errno) );
        SetLastError( ERROR_GEN_FAILURE );
        break;
    }
    errno = save_errno;
}


/***********************************************************************
 *           FILE_CreateFile
 *
 * Implementation of CreateFile. Takes a Unix path name.
 * Returns 0 on failure.
 */
HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                        LPSECURITY_ATTRIBUTES sa, DWORD creation,
                        DWORD attributes, HANDLE template )
{
    unsigned int err;
    UINT disp, options;
    HANDLE ret;

    switch (creation)
    {
    case CREATE_ALWAYS:     disp = FILE_OVERWRITE_IF; break;
    case CREATE_NEW:        disp = FILE_CREATE; break;
    case OPEN_ALWAYS:       disp = FILE_OPEN_IF; break;
    case OPEN_EXISTING:     disp = FILE_OPEN; break;
    case TRUNCATE_EXISTING: disp = FILE_OVERWRITE; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    options = 0;
    if (attributes & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
        options |= FILE_DELETE_ON_CLOSE;
    if (!(attributes & FILE_FLAG_OVERLAPPED))
        options |= FILE_SYNCHRONOUS_IO_ALERT;
    if (attributes & FILE_FLAG_RANDOM_ACCESS)
        options |= FILE_RANDOM_ACCESS;
    attributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    SERVER_START_REQ( create_file )
    {
        req->access     = access;
        req->inherit    = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        req->sharing    = sharing;
        req->create     = disp;
        req->options    = options;
        req->attrs      = attributes;
        wine_server_add_data( req, filename, strlen(filename) );
        SetLastError(0);
        err = wine_server_call( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;

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
    NTSTATUS status;
    UINT options;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    HANDLE ret;
    DWORD dosdev;
    static const WCHAR bkslashes_with_dotW[] = {'\\','\\','.','\\',0};
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};

    static const char * const creation_name[5] =
        { "CREATE_NEW", "CREATE_ALWAYS", "OPEN_EXISTING", "OPEN_ALWAYS", "TRUNCATE_EXISTING" };

    static const UINT nt_disposition[5] =
    {
        FILE_CREATE,        /* CREATE_NEW */
        FILE_OVERWRITE_IF,  /* CREATE_ALWAYS */
        FILE_OPEN,          /* OPEN_EXISTING */
        FILE_OPEN_IF,       /* OPEN_ALWAYS */
        FILE_OVERWRITE      /* TRUNCATE_EXISTING */
    };


    /* sanity checks */

    if (!filename || !filename[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    if (creation < CREATE_NEW || creation > TRUNCATE_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    TRACE("%s %s%s%s%s%s%s%s attributes 0x%lx\n", debugstr_w(filename),
          (access & GENERIC_READ)?"GENERIC_READ ":"",
          (access & GENERIC_WRITE)?"GENERIC_WRITE ":"",
          (!access)?"QUERY_ACCESS ":"",
          (sharing & FILE_SHARE_READ)?"FILE_SHARE_READ ":"",
          (sharing & FILE_SHARE_WRITE)?"FILE_SHARE_WRITE ":"",
          (sharing & FILE_SHARE_DELETE)?"FILE_SHARE_DELETE ":"",
          creation_name[creation - CREATE_NEW], attributes);

    /* Open a console for CONIN$ or CONOUT$ */

    if (!strcmpiW(filename, coninW) || !strcmpiW(filename, conoutW))
    {
        ret = OpenConsoleW(filename, access, (sa && sa->bInheritHandle), creation);
        goto done;
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
            const char *device = DRIVE_GetDevice( toupperW(filename[4]) - 'A' );
            if (device)
            {
                ret = FILE_CreateFile( device, access, sharing, sa, creation,
                                       attributes, template );
            }
            else
            {
                SetLastError( ERROR_ACCESS_DENIED );
                return INVALID_HANDLE_VALUE;
            }
            goto done;
        }
        else if ((dosdev = RtlIsDosDeviceName_U( filename + 4 )))
        {
            dosdev += MAKELONG( 0, 4*sizeof(WCHAR) );  /* adjust position to start of filename */
        }
        else if (filename[4])
        {
            ret = VXD_Open( filename+4, access, sa );
            goto done;
        }
        else
        {
            SetLastError( ERROR_INVALID_NAME );
            return INVALID_HANDLE_VALUE;
        }
    }
    else dosdev = RtlIsDosDeviceName_U( filename );

    if (dosdev)
    {
        static const WCHAR conW[] = {'C','O','N'};

        if (LOWORD(dosdev) == sizeof(conW) &&
            !memicmpW( filename + HIWORD(dosdev)/sizeof(WCHAR), conW, sizeof(conW)))
        {
            switch (access & (GENERIC_READ|GENERIC_WRITE))
            {
            case GENERIC_READ:
                ret = OpenConsoleW(coninW, access, (sa && sa->bInheritHandle), creation);
                goto done;
            case GENERIC_WRITE:
                ret = OpenConsoleW(conoutW, access, (sa && sa->bInheritHandle), creation);
                goto done;
            default:
                SetLastError( ERROR_FILE_NOT_FOUND );
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    /* now call NtCreateFile */

    options = 0;
    if (attributes & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
        options |= FILE_DELETE_ON_CLOSE;
    if (!(attributes & FILE_FLAG_OVERLAPPED))
        options |= FILE_SYNCHRONOUS_IO_ALERT;
    if (attributes & FILE_FLAG_RANDOM_ACCESS)
        options |= FILE_RANDOM_ACCESS;
    attributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    status = NtCreateFile( &ret, access, &attr, &io, NULL, attributes,
                           sharing, nt_disposition[creation - CREATE_NEW],
                           options, NULL, 0 );
    if (status)
    {
        WARN("Unable to create file %s (status %lx)\n", debugstr_w(filename), status);
        ret = INVALID_HANDLE_VALUE;

        /* In the case file creation was rejected due to CREATE_NEW flag
         * was specified and file with that name already exists, correct
         * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
         * Note: RtlNtStatusToDosError is not the subject to blame here.
         */
        if (status == STATUS_OBJECT_NAME_COLLISION)
            SetLastError( ERROR_FILE_EXISTS );
        else
            SetLastError( RtlNtStatusToDosError(status) );
    }
    else SetLastError(0);
    RtlFreeUnicodeString( &nameW );

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
    if (S_ISDIR(st->st_mode))
    {
        info->nFileSizeHigh  = 0;
        info->nFileSizeLow   = 0;
        info->nNumberOfLinks = 1;
    }
    else
    {
        info->nFileSizeHigh  = st->st_size >> 32;
        info->nFileSizeLow   = (DWORD)st->st_size;
        info->nNumberOfLinks = st->st_nlink;
    }
    info->nFileIndexHigh = st->st_ino >> 32;
    info->nFileIndexLow  = (DWORD)st->st_ino;
}


/***********************************************************************
 *           get_show_dot_files_option
 */
static BOOL get_show_dot_files_option(void)
{
    static const WCHAR WineW[] = {'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','W','i','n','e','\\',
                                  'C','o','n','f','i','g','\\','W','i','n','e',0};
    static const WCHAR ShowDotFilesW[] = {'S','h','o','w','D','o','t','F','i','l','e','s',0};

    char tmp[80];
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    BOOL ret = FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, WineW );

    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        RtlInitUnicodeString( &nameW, ShowDotFilesW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            ret = IS_OPTION_TRUE( str[0] );
        }
        NtClose( hkey );
    }
    return ret;
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
	static int show_dot_files = -1;
	if (show_dot_files == -1)
	    show_dot_files = get_show_dot_files_option();
	if (!show_dot_files)
	    info->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    }
    if (is_symlink_ptr) *is_symlink_ptr = is_symlink;
    return TRUE;
}


/***********************************************************************
 *             GetFileInformationByHandle   (KERNEL32.@)
 */
BOOL WINAPI GetFileInformationByHandle( HANDLE hFile, BY_HANDLE_FILE_INFORMATION *info )
{
    NTSTATUS status;
    int fd;
    BOOL ret = FALSE;

    TRACE("%p,%p\n", hFile, info);

    if (!info) return 0;

    if (!(status = wine_server_handle_to_fd( hFile, 0, &fd, NULL, NULL )))
    {
        struct stat st;

        if (fstat( fd, &st ) == -1)
            FILE_SetDosError();
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            SetLastError( ERROR_INVALID_FUNCTION );
        else
        {
            FILE_FillInfo( &st, info );
            ret = TRUE;
        }
        wine_server_release_fd( hFile, fd );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return ret;
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
    static const WCHAR formatW[] = {'%','x','.','t','m','p',0};

    int i;
    LPWSTR p;

    if ( !path || !prefix || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    strcpyW( buffer, path );
    p = buffer + strlenW(buffer);

    /* add a \, if there isn't one  */
    if ((p == buffer) || (p[-1] != '\\')) *p++ = '\\';

    for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    unique &= 0xffff;

    if (unique) sprintfW( p, formatW, unique );
    else
    {
        /* get a "random" unique number and try to create the file */
        HANDLE handle;
        UINT num = GetTickCount() & 0xffff;

        if (!num) num = 1;
        unique = num;
        do
        {
            sprintfW( p, formatW, unique );
            handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL,
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
            if (!(++unique & 0xffff)) unique = 1;
        } while (unique != num);
    }

    TRACE("returning %s\n", debugstr_w(buffer) );
    return unique;
}


/******************************************************************
 *		FILE_ReadWriteApc (internal)
 *
 *
 */
static void WINAPI      FILE_ReadWriteApc(void* apc_user, PIO_STATUS_BLOCK io_status, ULONG len)
{
    LPOVERLAPPED_COMPLETION_ROUTINE  cr = (LPOVERLAPPED_COMPLETION_ROUTINE)apc_user;

    cr(RtlNtStatusToDosError(io_status->u.Status), len, (LPOVERLAPPED)io_status);
}

/***********************************************************************
 *              ReadFileEx                (KERNEL32.@)
 */
BOOL WINAPI ReadFileEx(HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                       LPOVERLAPPED overlapped,
                       LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    PIO_STATUS_BLOCK    io_status;

    if (!overlapped)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;
    io_status = (PIO_STATUS_BLOCK)overlapped;
    io_status->u.Status = STATUS_PENDING;

    status = NtReadFile(hFile, NULL, FILE_ReadWriteApc, lpCompletionRoutine,
                        io_status, buffer, bytesToRead, &offset, NULL);

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.@)
 */
BOOL WINAPI ReadFile( HANDLE hFile, LPVOID buffer, DWORD bytesToRead,
                      LPDWORD bytesRead, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER       offset;
    PLARGE_INTEGER      poffset = NULL;
    IO_STATUS_BLOCK     iosb;
    PIO_STATUS_BLOCK    io_status = &iosb;
    HANDLE              hEvent = 0;
    NTSTATUS            status;
        
    TRACE("%p %p %ld %p %p\n", hFile, buffer, bytesToRead,
          bytesRead, overlapped );

    if (bytesRead) *bytesRead = 0;  /* Do this before anything else */
    if (!bytesToRead) return TRUE;

    if (IsBadReadPtr(buffer, bytesToRead))
    {
        SetLastError(ERROR_WRITE_FAULT); /* FIXME */
        return FALSE;
    }
    if (is_console_handle(hFile))
        return ReadConsoleA(hFile, buffer, bytesToRead, bytesRead, NULL);

    if (overlapped != NULL)
    {
        offset.u.LowPart = overlapped->Offset;
        offset.u.HighPart = overlapped->OffsetHigh;
        poffset = &offset;
        hEvent = overlapped->hEvent;
        io_status = (PIO_STATUS_BLOCK)overlapped;
    }
    io_status->u.Status = STATUS_PENDING;
    io_status->Information = 0;

    status = NtReadFile(hFile, hEvent, NULL, NULL, io_status, buffer, bytesToRead, poffset, NULL);

    if (status != STATUS_PENDING && bytesRead)
        *bytesRead = io_status->Information;

    if (status && status != STATUS_END_OF_FILE)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *              WriteFileEx                (KERNEL32.@)
 */
BOOL WINAPI WriteFileEx(HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                        LPOVERLAPPED overlapped,
                        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    LARGE_INTEGER       offset;
    NTSTATUS            status;
    PIO_STATUS_BLOCK    io_status;

    TRACE("%p %p %ld %p %p\n", 
          hFile, buffer, bytesToWrite, overlapped, lpCompletionRoutine);

    if (overlapped == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;

    io_status = (PIO_STATUS_BLOCK)overlapped;
    io_status->u.Status = STATUS_PENDING;

    status = NtWriteFile(hFile, NULL, FILE_ReadWriteApc, lpCompletionRoutine,
                         io_status, buffer, bytesToWrite, &offset, NULL);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.@)
 */
BOOL WINAPI WriteFile( HANDLE hFile, LPCVOID buffer, DWORD bytesToWrite,
                       LPDWORD bytesWritten, LPOVERLAPPED overlapped )
{
    HANDLE hEvent = NULL;
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK piosb = &iosb;

    TRACE("%p %p %ld %p %p\n", 
          hFile, buffer, bytesToWrite, bytesWritten, overlapped );

    if (is_console_handle(hFile))
        return WriteConsoleA(hFile, buffer, bytesToWrite, bytesWritten, NULL);

    if (IsBadReadPtr(buffer, bytesToWrite))
    {
        SetLastError(ERROR_READ_FAULT); /* FIXME */
        return FALSE;
    }

    if (overlapped)
    {
        offset.u.LowPart = overlapped->Offset;
        offset.u.HighPart = overlapped->OffsetHigh;
        poffset = &offset;
        hEvent = overlapped->hEvent;
        piosb = (PIO_STATUS_BLOCK)overlapped;
    }
    piosb->u.Status = STATUS_PENDING;
    piosb->Information = 0;

    status = NtWriteFile(hFile, hEvent, NULL, NULL, piosb,
                         buffer, bytesToWrite, poffset, NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    if (bytesWritten) *bytesWritten = piosb->Information;

    return TRUE;
}


/***********************************************************************
 *           SetFilePointer   (KERNEL32.@)
 */
DWORD WINAPI SetFilePointer( HANDLE hFile, LONG distance, LONG *highword,
                             DWORD method )
{
    static const int whence[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
    DWORD ret = INVALID_SET_FILE_POINTER;
    NTSTATUS status;
    int fd;

    TRACE("handle %p offset %ld high %ld origin %ld\n",
          hFile, distance, highword?*highword:0, method );

    if (method > FILE_END)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ret;
    }

    if (!(status = wine_server_handle_to_fd( hFile, 0, &fd, NULL, NULL )))
    {
        off_t pos, res;

        if (highword) pos = ((off_t)*highword << 32) | (ULONG)distance;
        else pos = (off_t)distance;
        if ((res = lseek( fd, pos, whence[method] )) == (off_t)-1)
        {
            /* also check EPERM due to SuSE7 2.2.16 lseek() EPERM kernel bug */
            if (((errno == EINVAL) || (errno == EPERM)) && (method != FILE_BEGIN) && (pos < 0))
                SetLastError( ERROR_NEGATIVE_SEEK );
            else
                FILE_SetDosError();
        }
        else
        {
            ret = (DWORD)res;
            if (highword) *highword = (res >> 32);
            if (ret == INVALID_SET_FILE_POINTER) SetLastError( 0 );
        }
        wine_server_release_fd( hFile, fd );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return ret;
}


/*************************************************************************
 *           SetHandleCount   (KERNEL32.@)
 */
UINT WINAPI SetHandleCount( UINT count )
{
    return min( 256, count );
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
 *           GetFileType   (KERNEL32.@)
 */
DWORD WINAPI GetFileType( HANDLE hFile )
{
    NTSTATUS status;
    int fd;
    DWORD ret = FILE_TYPE_UNKNOWN;

    if (is_console_handle( hFile ))
        return FILE_TYPE_CHAR;

    if (!(status = wine_server_handle_to_fd( hFile, 0, &fd, NULL, NULL )))
    {
        struct stat st;

        if (fstat( fd, &st ) == -1)
            FILE_SetDosError();
        else if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
            ret = FILE_TYPE_PIPE;
        else if (S_ISCHR(st.st_mode))
            ret = FILE_TYPE_CHAR;
        else
            ret = FILE_TYPE_DISK;
        wine_server_release_fd( hFile, fd );
    }
    else SetLastError( RtlNtStatusToDosError(status) );

    return ret;
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
