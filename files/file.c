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

#include "drive.h"
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

HANDLE dos_handles[DOS_TABLE_SIZE];
mode_t FILE_umask;

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
            req->removable  = (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM);
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
 *           GetFileAttributesW   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesW( LPCWSTR name )
{
    DOS_FULL_NAME full_name;
    BY_HANDLE_FILE_INFORMATION info;

    if (name == NULL)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_FILE_ATTRIBUTES;
    }
    if (!DOSFS_GetFullName( name, TRUE, &full_name) )
        return INVALID_FILE_ATTRIBUTES;
    if (!FILE_Stat( full_name.long_name, &info, NULL ))
        return INVALID_FILE_ATTRIBUTES;
    return info.dwFileAttributes;
}


/**************************************************************************
 *           GetFileAttributesA   (KERNEL32.@)
 */
DWORD WINAPI GetFileAttributesA( LPCSTR name )
{
    UNICODE_STRING nameW;
    DWORD ret = INVALID_FILE_ATTRIBUTES;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_FILE_ATTRIBUTES;
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


/******************************************************************************
 * GetCompressedFileSizeA [KERNEL32.@]
 */
DWORD WINAPI GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh)
{
    UNICODE_STRING filenameW;
    DWORD ret;

    if (RtlCreateUnicodeStringFromAsciiz(&filenameW, lpFileName))
    {
        ret = GetCompressedFileSizeW(filenameW.Buffer, lpFileSizeHigh);
        RtlFreeUnicodeString(&filenameW);
    }
    else
    {
        ret = INVALID_FILE_SIZE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return ret;
}


/******************************************************************************
 * GetCompressedFileSizeW [KERNEL32.@]
 *
 * RETURNS
 *    Success: Low-order doubleword of number of bytes
 *    Failure: INVALID_FILE_SIZE
 */
DWORD WINAPI GetCompressedFileSizeW(
    LPCWSTR lpFileName,     /* [in]  Pointer to name of file */
    LPDWORD lpFileSizeHigh) /* [out] Receives high-order doubleword of size */
{
    DOS_FULL_NAME full_name;
    struct stat st;
    DWORD low;

    TRACE("(%s,%p)\n",debugstr_w(lpFileName),lpFileSizeHigh);

    if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name )) return INVALID_FILE_SIZE;
    if (stat(full_name.long_name, &st) != 0)
    {
        FILE_SetDosError();
        return INVALID_FILE_SIZE;
    }
#if HAVE_STRUCT_STAT_ST_BLOCKS
    /* blocks are 512 bytes long */
    if (lpFileSizeHigh) *lpFileSizeHigh = (st.st_blocks >> 23);
    low = (DWORD)(st.st_blocks << 9);
#else
    if (lpFileSizeHigh) *lpFileSizeHigh = (st.st_size >> 32);
    low = (DWORD)st.st_size;
#endif
    return low;
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
    static const WCHAR formatW[] = {'%','0','4','x','.','t','m','p',0};

    DOS_FULL_NAME full_name;
    int i;
    LPWSTR p;

    if ( !path || !prefix || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    strcpyW( buffer, path );
    p = buffer + strlenW(buffer);

    /* add a \, if there isn't one and path is more than just the drive letter ... */
    if ( !((strlenW(buffer) == 2) && (buffer[1] == ':'))
       && ((p == buffer) || (p[-1] != '\\'))) *p++ = '\\';

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
    return unique;
}


/***********************************************************************
 *           FILE_DoOpenFile
 *
 * Implementation of OpenFile16() and OpenFile32().
 */
static HFILE FILE_DoOpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode, BOOL win32 )
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

    if (mode & OF_DELETE)
    {
        handle = FILE_CreateFile( full_name.long_name, GENERIC_READ|GENERIC_WRITE, 0,
                                 NULL, OPEN_EXISTING, 0, 0, TRUE,
                                 GetDriveTypeW( full_name.short_name ) );
        if (!handle) goto error;
        CloseHandle( handle );
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

    offset.s.LowPart = overlapped->Offset;
    offset.s.HighPart = overlapped->OffsetHigh;
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
        offset.s.LowPart = overlapped->Offset;
        offset.s.HighPart = overlapped->OffsetHigh;
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
    offset.s.LowPart = overlapped->Offset;
    offset.s.HighPart = overlapped->OffsetHigh;

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
        offset.s.LowPart = overlapped->Offset;
        offset.s.HighPart = overlapped->OffsetHigh;
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
       to be really compatible. Most programs won't have any problems though. In case
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
                    SetLastError( ERROR_ALREADY_EXISTS );
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
