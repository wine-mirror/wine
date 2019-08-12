/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2008 Jeff Zaroyko
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "wincon.h"
#include "ddk/ntddk.h"
#include "kernel_private.h"
#include "fileapi.h"

#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/* info structure for FindFirstFile handle */
typedef struct
{
    DWORD             magic;       /* magic number */
    HANDLE            handle;      /* handle to directory */
    CRITICAL_SECTION  cs;          /* crit section protecting this structure */
    FINDEX_SEARCH_OPS search_op;   /* Flags passed to FindFirst.  */
    FINDEX_INFO_LEVELS level;      /* Level passed to FindFirst */
    UNICODE_STRING    path;        /* NT path used to open the directory */
    BOOL              is_root;     /* is directory the root of the drive? */
    BOOL              wildcard;    /* did the mask contain wildcard characters? */
    UINT              data_pos;    /* current position in dir data */
    UINT              data_len;    /* length of dir data */
    UINT              data_size;   /* size of data buffer, or 0 when everything has been read */
    BYTE              data[1];     /* directory data */
} FIND_FIRST_INFO;

#define FIND_FIRST_MAGIC  0xc0ffee11

static const UINT max_entry_size = offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[256] );

static const WCHAR wildcardsW[] = { '*','?',0 };
static const WCHAR krnl386W[] = {'k','r','n','l','3','8','6','.','e','x','e','1','6',0};

/***********************************************************************
 *              create_file_OF
 *
 * Wrapper for CreateFile that takes OF_* mode flags.
 */
static HANDLE create_file_OF( LPCSTR path, INT mode )
{
    DWORD access, sharing, creation;

    if (mode & OF_CREATE)
    {
        creation = CREATE_ALWAYS;
        access = GENERIC_READ | GENERIC_WRITE;
    }
    else
    {
        creation = OPEN_EXISTING;
        switch(mode & 0x03)
        {
        case OF_READ:      access = GENERIC_READ; break;
        case OF_WRITE:     access = GENERIC_WRITE; break;
        case OF_READWRITE: access = GENERIC_READ | GENERIC_WRITE; break;
        default:           access = 0; break;
        }
    }

    switch(mode & 0x70)
    {
    case OF_SHARE_EXCLUSIVE:  sharing = 0; break;
    case OF_SHARE_DENY_WRITE: sharing = FILE_SHARE_READ; break;
    case OF_SHARE_DENY_READ:  sharing = FILE_SHARE_WRITE; break;
    case OF_SHARE_DENY_NONE:
    case OF_SHARE_COMPAT:
    default:                  sharing = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
    }
    return CreateFileA( path, access, sharing, NULL, creation, FILE_ATTRIBUTE_NORMAL, 0 );
}


/***********************************************************************
 *              check_dir_symlink
 *
 * Check if a dir symlink should be returned by FindNextFile.
 */
static BOOL check_dir_symlink( FIND_FIRST_INFO *info, const FILE_BOTH_DIR_INFORMATION *file_info )
{
    UNICODE_STRING str;
    ANSI_STRING unix_name;
    struct stat st, parent_st;
    BOOL ret = TRUE;
    DWORD len;

    str.MaximumLength = info->path.Length + sizeof(WCHAR) + file_info->FileNameLength;
    if (!(str.Buffer = HeapAlloc( GetProcessHeap(), 0, str.MaximumLength ))) return TRUE;
    memcpy( str.Buffer, info->path.Buffer, info->path.Length );
    len = info->path.Length / sizeof(WCHAR);
    if (!len || str.Buffer[len-1] != '\\') str.Buffer[len++] = '\\';
    memcpy( str.Buffer + len, file_info->FileName, file_info->FileNameLength );
    str.Length = len * sizeof(WCHAR) + file_info->FileNameLength;

    unix_name.Buffer = NULL;
    if (!wine_nt_to_unix_file_name( &str, &unix_name, OPEN_EXISTING, FALSE ) &&
        !stat( unix_name.Buffer, &st ))
    {
        char *p = unix_name.Buffer + unix_name.Length - 1;

        /* skip trailing slashes */
        while (p > unix_name.Buffer && *p == '/') p--;

        while (ret && p > unix_name.Buffer)
        {
            while (p > unix_name.Buffer && *p != '/') p--;
            while (p > unix_name.Buffer && *p == '/') p--;
            p[1] = 0;
            if (!stat( unix_name.Buffer, &parent_st ) &&
                parent_st.st_dev == st.st_dev &&
                parent_st.st_ino == st.st_ino)
            {
                WARN( "suppressing dir symlink %s pointing to parent %s\n",
                      debugstr_wn( str.Buffer, str.Length/sizeof(WCHAR) ),
                      debugstr_a( unix_name.Buffer ));
                ret = FALSE;
            }
        }
    }
    RtlFreeAnsiString( &unix_name );
    RtlFreeUnicodeString( &str );
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
 *           FILE_name_AtoW
 *
 * Convert a file name to Unicode, taking into account the OEM/Ansi API mode.
 *
 * If alloc is FALSE uses the TEB static buffer, so it can only be used when
 * there is no possibility for the function to do that twice, taking into
 * account any called function.
 */
WCHAR *FILE_name_AtoW( LPCSTR name, BOOL alloc )
{
    ANSI_STRING str;
    UNICODE_STRING strW, *pstrW;
    NTSTATUS status;

    RtlInitAnsiString( &str, name );
    pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;
    if (!AreFileApisANSI())
        status = RtlOemStringToUnicodeString( pstrW, &str, alloc );
    else
        status = RtlAnsiStringToUnicodeString( pstrW, &str, alloc );
    if (status == STATUS_SUCCESS) return pstrW->Buffer;

    if (status == STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return NULL;
}


/***********************************************************************
 *           FILE_name_WtoA
 *
 * Convert a file name back to OEM/Ansi. Returns number of bytes copied.
 */
DWORD FILE_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen )
{
    DWORD ret;

    if (srclen < 0) srclen = strlenW( src ) + 1;
    if (!AreFileApisANSI())
        RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    else
        RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    return ret;
}


/***********************************************************************
 *           _hread   (KERNEL32.@)
 */
LONG WINAPI _hread( HFILE hFile, LPVOID buffer, LONG count)
{
    return _lread( hFile, buffer, count );
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

    TRACE("%d %p %d\n", handle, buffer, count );

    if (!count)
    {
        /* Expand or truncate at current position */
        if (!SetEndOfFile( LongToHandle(handle) )) return HFILE_ERROR;
        return 0;
    }
    if (!WriteFile( LongToHandle(handle), buffer, count, &result, NULL ))
        return HFILE_ERROR;
    return result;
}


/***********************************************************************
 *           _lclose   (KERNEL32.@)
 */
HFILE WINAPI _lclose( HFILE hFile )
{
    TRACE("handle %d\n", hFile );
    return CloseHandle( LongToHandle(hFile) ) ? 0 : HFILE_ERROR;
}


/***********************************************************************
 *           _lcreat   (KERNEL32.@)
 */
HFILE WINAPI _lcreat( LPCSTR path, INT attr )
{
    HANDLE hfile;

    /* Mask off all flags not explicitly allowed by the doc */
    attr &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    TRACE("%s %02x\n", path, attr );
    hfile = CreateFileA( path, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, attr, 0 );
    return HandleToLong(hfile);
}


/***********************************************************************
 *           _lopen   (KERNEL32.@)
 */
HFILE WINAPI _lopen( LPCSTR path, INT mode )
{
    HANDLE hfile;

    TRACE("(%s,%04x)\n", debugstr_a(path), mode );
    hfile = create_file_OF( path, mode & ~OF_CREATE );
    return HandleToLong(hfile);
}

/***********************************************************************
 *           _lread   (KERNEL32.@)
 */
UINT WINAPI _lread( HFILE handle, LPVOID buffer, UINT count )
{
    DWORD result;
    if (!ReadFile( LongToHandle(handle), buffer, count, &result, NULL ))
        return HFILE_ERROR;
    return result;
}


/***********************************************************************
 *           _llseek   (KERNEL32.@)
 */
LONG WINAPI _llseek( HFILE hFile, LONG lOffset, INT nOrigin )
{
    return SetFilePointer( LongToHandle(hFile), lOffset, NULL, nOrigin );
}


/***********************************************************************
 *           _lwrite   (KERNEL32.@)
 */
UINT WINAPI _lwrite( HFILE hFile, LPCSTR buffer, UINT count )
{
    return (UINT)_hwrite( hFile, buffer, (LONG)count );
}


/**************************************************************************
 *           SetFileCompletionNotificationModes   (KERNEL32.@)
 */
BOOL WINAPI SetFileCompletionNotificationModes( HANDLE file, UCHAR flags )
{
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    info.Flags = flags;
    status = NtSetInformationFile( file, &io, &info, sizeof(info), FileIoCompletionNotificationInformation );
    if (status == STATUS_SUCCESS) return TRUE;
    SetLastError( RtlNtStatusToDosError(status) );
    return FALSE;
}


/*************************************************************************
 *           SetHandleCount   (KERNEL32.@)
 */
UINT WINAPI SetHandleCount( UINT count )
{
    return count;
}


/**************************************************************************
 *                      Operations on file names                          *
 **************************************************************************/


/**************************************************************************
 *           ReplaceFileW   (KERNEL32.@)
 *           ReplaceFile    (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileW(LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName,
                         LPCWSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    UNICODE_STRING nt_replaced_name, nt_replacement_name;
    ANSI_STRING unix_replaced_name, unix_replacement_name, unix_backup_name;
    HANDLE hReplaced = NULL, hReplacement = NULL, hBackup = NULL;
    DWORD error = ERROR_SUCCESS;
    UINT replaced_flags;
    BOOL ret = FALSE;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    FILE_BASIC_INFORMATION info;

    TRACE("%s %s %s 0x%08x %p %p\n", debugstr_w(lpReplacedFileName),
          debugstr_w(lpReplacementFileName), debugstr_w(lpBackupFileName),
          dwReplaceFlags, lpExclude, lpReserved);

    if (dwReplaceFlags)
        FIXME("Ignoring flags %x\n", dwReplaceFlags);

    /* First two arguments are mandatory */
    if (!lpReplacedFileName || !lpReplacementFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    unix_replaced_name.Buffer = NULL;
    unix_replacement_name.Buffer = NULL;
    unix_backup_name.Buffer = NULL;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* Open the "replaced" file for reading */
    if (!(RtlDosPathNameToNtPathName_U(lpReplacedFileName, &nt_replaced_name, NULL, NULL)))
    {
        error = ERROR_PATH_NOT_FOUND;
        goto fail;
    }
    replaced_flags = lpBackupFileName ? FILE_OPEN : FILE_OPEN_IF;
    attr.ObjectName = &nt_replaced_name;
    status = NtOpenFile(&hReplaced, GENERIC_READ|DELETE|SYNCHRONIZE,
                        &attr, &io,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);
    if (status == STATUS_SUCCESS)
        status = wine_nt_to_unix_file_name(&nt_replaced_name, &unix_replaced_name, replaced_flags, FALSE);
    RtlFreeUnicodeString(&nt_replaced_name);
    if (status != STATUS_SUCCESS)
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            error = ERROR_FILE_NOT_FOUND;
        else
            error = ERROR_UNABLE_TO_REMOVE_REPLACED;
        goto fail;
    }

    /* Replacement should fail if replaced is READ_ONLY */
    status = NtQueryInformationFile(hReplaced, &io, &info, sizeof(info), FileBasicInformation);
    if (status != STATUS_SUCCESS)
    {
        error = RtlNtStatusToDosError(status);
        goto fail;
    }

    if (info.FileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        error = ERROR_ACCESS_DENIED;
        goto fail;
    }

    /*
     * Open the replacement file for reading, writing, and deleting
     * (writing and deleting are needed when finished)
     */
    if (!(RtlDosPathNameToNtPathName_U(lpReplacementFileName, &nt_replacement_name, NULL, NULL)))
    {
        error = ERROR_PATH_NOT_FOUND;
        goto fail;
    }
    attr.ObjectName = &nt_replacement_name;
    status = NtOpenFile(&hReplacement,
                        GENERIC_READ|GENERIC_WRITE|DELETE|WRITE_DAC|SYNCHRONIZE,
                        &attr, &io, 0,
                        FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);
    if (status == STATUS_SUCCESS)
        status = wine_nt_to_unix_file_name(&nt_replacement_name, &unix_replacement_name, FILE_OPEN, FALSE);
    RtlFreeUnicodeString(&nt_replacement_name);
    if (status != STATUS_SUCCESS)
    {
        error = RtlNtStatusToDosError(status);
        goto fail;
    }

    /* If the user wants a backup then that needs to be performed first */
    if (lpBackupFileName)
    {
        UNICODE_STRING nt_backup_name;
        FILE_BASIC_INFORMATION replaced_info;

        /* Obtain the file attributes from the "replaced" file */
        status = NtQueryInformationFile(hReplaced, &io, &replaced_info,
                                        sizeof(replaced_info),
                                        FileBasicInformation);
        if (status != STATUS_SUCCESS)
        {
            error = RtlNtStatusToDosError(status);
            goto fail;
        }

        if (!(RtlDosPathNameToNtPathName_U(lpBackupFileName, &nt_backup_name, NULL, NULL)))
        {
            error = ERROR_PATH_NOT_FOUND;
            goto fail;
        }
        attr.ObjectName = &nt_backup_name;
        /* Open the backup with permissions to write over it */
        status = NtCreateFile(&hBackup, GENERIC_WRITE | SYNCHRONIZE,
                              &attr, &io, NULL, replaced_info.FileAttributes,
                              FILE_SHARE_WRITE, FILE_OPEN_IF,
                              FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                              NULL, 0);
        if (status == STATUS_SUCCESS)
            status = wine_nt_to_unix_file_name(&nt_backup_name, &unix_backup_name, FILE_OPEN_IF, FALSE);
        RtlFreeUnicodeString(&nt_backup_name);
        if (status != STATUS_SUCCESS)
        {
            error = RtlNtStatusToDosError(status);
            goto fail;
        }

        /* If an existing backup exists then copy over it */
        if (rename(unix_replaced_name.Buffer, unix_backup_name.Buffer) == -1)
        {
            error = ERROR_UNABLE_TO_REMOVE_REPLACED; /* is this correct? */
            goto fail;
        }
    }

    /*
     * Now that the backup has been performed (if requested), copy the replacement
     * into place
     */
    if (rename(unix_replacement_name.Buffer, unix_replaced_name.Buffer) == -1)
    {
        if (errno == EACCES)
        {
            /* Inappropriate permissions on "replaced", rename will fail */
            error = ERROR_UNABLE_TO_REMOVE_REPLACED;
            goto fail;
        }
        /* on failure we need to indicate whether a backup was made */
        if (!lpBackupFileName)
            error = ERROR_UNABLE_TO_MOVE_REPLACEMENT;
        else
            error = ERROR_UNABLE_TO_MOVE_REPLACEMENT_2;
        goto fail;
    }
    /* Success! */
    ret = TRUE;

    /* Perform resource cleanup */
fail:
    if (hBackup) CloseHandle(hBackup);
    if (hReplaced) CloseHandle(hReplaced);
    if (hReplacement) CloseHandle(hReplacement);
    RtlFreeAnsiString(&unix_backup_name);
    RtlFreeAnsiString(&unix_replacement_name);
    RtlFreeAnsiString(&unix_replaced_name);

    /* If there was an error, set the error code */
    if(!ret)
        SetLastError(error);
    return ret;
}


/**************************************************************************
 *           ReplaceFileA (KERNEL32.@)
 */
BOOL WINAPI ReplaceFileA(LPCSTR lpReplacedFileName,LPCSTR lpReplacementFileName,
                         LPCSTR lpBackupFileName, DWORD dwReplaceFlags,
                         LPVOID lpExclude, LPVOID lpReserved)
{
    WCHAR *replacedW, *replacementW, *backupW = NULL;
    BOOL ret;

    /* This function only makes sense when the first two parameters are defined */
    if (!lpReplacedFileName || !(replacedW = FILE_name_AtoW( lpReplacedFileName, TRUE )))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!lpReplacementFileName || !(replacementW = FILE_name_AtoW( lpReplacementFileName, TRUE )))
    {
        HeapFree( GetProcessHeap(), 0, replacedW );
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* The backup parameter, however, is optional */
    if (lpBackupFileName)
    {
        if (!(backupW = FILE_name_AtoW( lpBackupFileName, TRUE )))
        {
            HeapFree( GetProcessHeap(), 0, replacedW );
            HeapFree( GetProcessHeap(), 0, replacementW );
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    ret = ReplaceFileW( replacedW, replacementW, backupW, dwReplaceFlags, lpExclude, lpReserved );
    HeapFree( GetProcessHeap(), 0, replacedW );
    HeapFree( GetProcessHeap(), 0, replacementW );
    HeapFree( GetProcessHeap(), 0, backupW );
    return ret;
}


/*************************************************************************
 *           FindFirstFileExW  (KERNEL32.@)
 *
 * NOTE: The FindExSearchLimitToDirectories is ignored - it gives the same
 * results as FindExSearchNameMatch
 */
HANDLE WINAPI FindFirstFileExW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                LPVOID data, FINDEX_SEARCH_OPS search_op,
                                LPVOID filter, DWORD flags)
{
    WCHAR *mask;
    BOOL has_wildcard = FALSE;
    FIND_FIRST_INFO *info = NULL;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD size, device = 0;

    TRACE("%s %d %p %d %p %x\n", debugstr_w(filename), level, data, search_op, filter, flags);

    if (flags & ~FIND_FIRST_EX_LARGE_FETCH)
    {
        FIXME("flags not implemented 0x%08x\n", flags );
    }
    if (search_op != FindExSearchNameMatch && search_op != FindExSearchLimitToDirectories)
    {
        FIXME("search_op not implemented 0x%08x\n", search_op);
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }
    if (level != FindExInfoStandard && level != FindExInfoBasic)
    {
        FIXME("info level %d not implemented\n", level );
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nt_name, &mask, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    if (!mask && (device = RtlIsDosDeviceName_U( filename )))
    {
        static const WCHAR dotW[] = {'.',0};
        WCHAR *dir = NULL;

        /* we still need to check that the directory can be opened */

        if (HIWORD(device))
        {
            if (!(dir = HeapAlloc( GetProcessHeap(), 0, HIWORD(device) + sizeof(WCHAR) )))
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                goto error;
            }
            memcpy( dir, filename, HIWORD(device) );
            dir[HIWORD(device)/sizeof(WCHAR)] = 0;
        }
        RtlFreeUnicodeString( &nt_name );
        if (!RtlDosPathNameToNtPathName_U( dir ? dir : dotW, &nt_name, &mask, NULL ))
        {
            HeapFree( GetProcessHeap(), 0, dir );
            SetLastError( ERROR_PATH_NOT_FOUND );
            goto error;
        }
        HeapFree( GetProcessHeap(), 0, dir );
        size = 0;
    }
    else if (!mask || !*mask)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto error;
    }
    else
    {
        nt_name.Length = (mask - nt_name.Buffer) * sizeof(WCHAR);
        has_wildcard = strpbrkW( mask, wildcardsW ) != NULL;
        size = has_wildcard ? 8192 : max_entry_size;
    }

    if (!(info = HeapAlloc( GetProcessHeap(), 0, offsetof( FIND_FIRST_INFO, data[size] ))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error;
    }

    /* check if path is the root of the drive, skipping the \??\ prefix */
    info->is_root = FALSE;
    if (nt_name.Length >= 6 * sizeof(WCHAR) && nt_name.Buffer[5] == ':')
    {
        DWORD pos = 6;
        while (pos * sizeof(WCHAR) < nt_name.Length && nt_name.Buffer[pos] == '\\') pos++;
        info->is_root = (pos * sizeof(WCHAR) >= nt_name.Length);
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &info->handle, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );

    if (status != STATUS_SUCCESS)
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            SetLastError( ERROR_PATH_NOT_FOUND );
        else
            SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }

    RtlInitializeCriticalSection( &info->cs );
    info->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FIND_FIRST_INFO.cs");
    info->path     = nt_name;
    info->magic    = FIND_FIRST_MAGIC;
    info->wildcard = has_wildcard;
    info->data_pos = 0;
    info->data_len = 0;
    info->data_size = size;
    info->search_op = search_op;
    info->level     = level;

    if (device)
    {
        WIN32_FIND_DATAW *wfd = data;

        memset( wfd, 0, sizeof(*wfd) );
        memcpy( wfd->cFileName, filename + HIWORD(device)/sizeof(WCHAR), LOWORD(device) );
        wfd->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        CloseHandle( info->handle );
        info->handle = 0;
    }
    else
    {
        UNICODE_STRING mask_str;

        RtlInitUnicodeString( &mask_str, mask );
        status = NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, info->data_size,
                                       FileBothDirectoryInformation, FALSE, &mask_str, TRUE );
        if (status)
        {
            FindClose( info );
            SetLastError( RtlNtStatusToDosError( status ) );
            return INVALID_HANDLE_VALUE;
        }

        info->data_len = io.Information;
        if (!has_wildcard || info->data_len < info->data_size - max_entry_size)
        {
            if (has_wildcard)  /* release unused buffer space */
                HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY,
                             info, offsetof( FIND_FIRST_INFO, data[info->data_len] ));
            info->data_size = 0;  /* we read everything */
        }

        if (!FindNextFileW( info, data ))
        {
            TRACE( "%s not found\n", debugstr_w(filename) );
            FindClose( info );
            SetLastError( ERROR_FILE_NOT_FOUND );
            return INVALID_HANDLE_VALUE;
        }
        if (!has_wildcard)  /* we can't find two files with the same name */
        {
            CloseHandle( info->handle );
            info->handle = 0;
        }
    }
    return info;

error:
    HeapFree( GetProcessHeap(), 0, info );
    RtlFreeUnicodeString( &nt_name );
    return INVALID_HANDLE_VALUE;
}


/*************************************************************************
 *           FindNextFileW   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileW( HANDLE handle, WIN32_FIND_DATAW *data )
{
    FIND_FIRST_INFO *info;
    FILE_BOTH_DIR_INFORMATION *dir_info;
    BOOL ret = FALSE;
    NTSTATUS status;

    TRACE("%p %p\n", handle, data);

    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    info = handle;
    if (info->magic != FIND_FIRST_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }

    RtlEnterCriticalSection( &info->cs );

    if (!info->handle) SetLastError( ERROR_NO_MORE_FILES );
    else for (;;)
    {
        if (info->data_pos >= info->data_len)  /* need to read some more data */
        {
            IO_STATUS_BLOCK io;

            if (info->data_size)
                status = NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, info->data_size,
                                               FileBothDirectoryInformation, FALSE, NULL, FALSE );
            else
                status = STATUS_NO_MORE_FILES;

            if (status)
            {
                SetLastError( RtlNtStatusToDosError( status ) );
                if (status == STATUS_NO_MORE_FILES)
                {
                    CloseHandle( info->handle );
                    info->handle = 0;
                }
                break;
            }
            info->data_len = io.Information;
            info->data_pos = 0;
        }

        dir_info = (FILE_BOTH_DIR_INFORMATION *)(info->data + info->data_pos);

        if (dir_info->NextEntryOffset) info->data_pos += dir_info->NextEntryOffset;
        else info->data_pos = info->data_len;

        /* don't return '.' and '..' in the root of the drive */
        if (info->is_root)
        {
            if (dir_info->FileNameLength == sizeof(WCHAR) && dir_info->FileName[0] == '.') continue;
            if (dir_info->FileNameLength == 2 * sizeof(WCHAR) &&
                dir_info->FileName[0] == '.' && dir_info->FileName[1] == '.') continue;
        }

        /* check for dir symlink */
        if ((dir_info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (dir_info->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
            info->wildcard)
        {
            if (!check_dir_symlink( info, dir_info )) continue;
        }

        data->dwFileAttributes = dir_info->FileAttributes;
        data->ftCreationTime   = *(FILETIME *)&dir_info->CreationTime;
        data->ftLastAccessTime = *(FILETIME *)&dir_info->LastAccessTime;
        data->ftLastWriteTime  = *(FILETIME *)&dir_info->LastWriteTime;
        data->nFileSizeHigh    = dir_info->EndOfFile.QuadPart >> 32;
        data->nFileSizeLow     = (DWORD)dir_info->EndOfFile.QuadPart;
        data->dwReserved0      = 0;
        data->dwReserved1      = 0;

        memcpy( data->cFileName, dir_info->FileName, dir_info->FileNameLength );
        data->cFileName[dir_info->FileNameLength/sizeof(WCHAR)] = 0;

        if (info->level != FindExInfoBasic)
        {
            memcpy( data->cAlternateFileName, dir_info->ShortName, dir_info->ShortNameLength );
            data->cAlternateFileName[dir_info->ShortNameLength/sizeof(WCHAR)] = 0;
        }
        else
            data->cAlternateFileName[0] = 0;

        TRACE("returning %s (%s)\n",
              debugstr_w(data->cFileName), debugstr_w(data->cAlternateFileName) );

        ret = TRUE;
        break;
    }

    RtlLeaveCriticalSection( &info->cs );
    return ret;
}


/*************************************************************************
 *           FindClose   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindClose( HANDLE handle )
{
    FIND_FIRST_INFO *info = handle;

    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    __TRY
    {
        if (info->magic == FIND_FIRST_MAGIC)
        {
            RtlEnterCriticalSection( &info->cs );
            if (info->magic == FIND_FIRST_MAGIC)  /* in case someone else freed it in the meantime */
            {
                info->magic = 0;
                if (info->handle) CloseHandle( info->handle );
                info->handle = 0;
                RtlFreeUnicodeString( &info->path );
                info->data_pos = 0;
                info->data_len = 0;
                RtlLeaveCriticalSection( &info->cs );
                info->cs.DebugInfo->Spare[0] = 0;
                RtlDeleteCriticalSection( &info->cs );
                HeapFree( GetProcessHeap(), 0, info );
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN("Illegal handle %p\n", handle);
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    __ENDTRY

    return TRUE;
}


/*************************************************************************
 *           FindFirstFileA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileA( LPCSTR lpFileName, WIN32_FIND_DATAA *lpFindData )
{
    return FindFirstFileExA(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}

/*************************************************************************
 *           FindFirstFileExA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileExA( LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId,
                                LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp,
                                LPVOID lpSearchFilter, DWORD dwAdditionalFlags)
{
    HANDLE handle;
    WIN32_FIND_DATAA *dataA;
    WIN32_FIND_DATAW dataW;
    WCHAR *nameW;

    if (!(nameW = FILE_name_AtoW( lpFileName, FALSE ))) return INVALID_HANDLE_VALUE;

    handle = FindFirstFileExW(nameW, fInfoLevelId, &dataW, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    if (handle == INVALID_HANDLE_VALUE) return handle;

    dataA = lpFindFileData;
    dataA->dwFileAttributes = dataW.dwFileAttributes;
    dataA->ftCreationTime   = dataW.ftCreationTime;
    dataA->ftLastAccessTime = dataW.ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW.ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW.nFileSizeHigh;
    dataA->nFileSizeLow     = dataW.nFileSizeLow;
    FILE_name_WtoA( dataW.cFileName, -1, dataA->cFileName, sizeof(dataA->cFileName) );
    FILE_name_WtoA( dataW.cAlternateFileName, -1, dataA->cAlternateFileName,
                    sizeof(dataA->cAlternateFileName) );
    return handle;
}


/*************************************************************************
 *           FindFirstFileW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileW( LPCWSTR lpFileName, WIN32_FIND_DATAW *lpFindData )
{
    return FindFirstFileExW(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}


/*************************************************************************
 *           FindNextFileA   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileA( HANDLE handle, WIN32_FIND_DATAA *data )
{
    WIN32_FIND_DATAW dataW;

    if (!FindNextFileW( handle, &dataW )) return FALSE;
    data->dwFileAttributes = dataW.dwFileAttributes;
    data->ftCreationTime   = dataW.ftCreationTime;
    data->ftLastAccessTime = dataW.ftLastAccessTime;
    data->ftLastWriteTime  = dataW.ftLastWriteTime;
    data->nFileSizeHigh    = dataW.nFileSizeHigh;
    data->nFileSizeLow     = dataW.nFileSizeLow;
    FILE_name_WtoA( dataW.cFileName, -1, data->cFileName, sizeof(data->cFileName) );
    FILE_name_WtoA( dataW.cAlternateFileName, -1, data->cAlternateFileName,
                    sizeof(data->cAlternateFileName) );
    return TRUE;
}

/**************************************************************************
 *           FindFirstStreamW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstStreamW(LPCWSTR filename, STREAM_INFO_LEVELS infolevel, void *data, DWORD flags)
{
    FIXME("(%s, %d, %p, %x): stub!\n", debugstr_w(filename), infolevel, data, flags);

    SetLastError(ERROR_HANDLE_EOF);
    return INVALID_HANDLE_VALUE;
}

/**************************************************************************
 *           FindNextStreamW   (KERNEL32.@)
 */
BOOL WINAPI FindNextStreamW(HANDLE handle, void *data)
{
    FIXME("(%p, %p): stub!\n", handle, data);

    SetLastError(ERROR_HANDLE_EOF);
    return FALSE;
}


/***********************************************************************
 *		OpenVxDHandle (KERNEL32.@)
 *
 *	This function is supposed to return the corresponding Ring 0
 *	("kernel") handle for a Ring 3 handle in Win9x.
 *	Evidently, Wine will have problems with this. But we try anyway,
 *	maybe it helps...
 */
HANDLE WINAPI OpenVxDHandle(HANDLE hHandleRing3)
{
    FIXME( "(%p), stub! (returning Ring 3 handle instead of Ring 0)\n", hHandleRing3);
    return hHandleRing3;
}


/****************************************************************************
 *		DeviceIoControl (KERNEL32.@)
 */
BOOL WINAPI DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode,
                            LPVOID lpvInBuffer, DWORD cbInBuffer,
                            LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                            LPDWORD lpcbBytesReturned,
                            LPOVERLAPPED lpOverlapped)
{
    NTSTATUS status;

    TRACE( "(%p,%x,%p,%d,%p,%d,%p,%p)\n",
           hDevice,dwIoControlCode,lpvInBuffer,cbInBuffer,
           lpvOutBuffer,cbOutBuffer,lpcbBytesReturned,lpOverlapped );

    /* Check if this is a user defined control code for a VxD */

    if (HIWORD( dwIoControlCode ) == 0 && (GetVersion() & 0x80000000))
    {
        typedef BOOL (WINAPI *DeviceIoProc)(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
        static DeviceIoProc (*vxd_get_proc)(HANDLE);
        DeviceIoProc proc = NULL;

        if (!vxd_get_proc) vxd_get_proc = (void *)GetProcAddress( GetModuleHandleW(krnl386W),
                                                                  "__wine_vxd_get_proc" );
        if (vxd_get_proc) proc = vxd_get_proc( hDevice );
        if (proc) return proc( dwIoControlCode, lpvInBuffer, cbInBuffer,
                               lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped );
    }

    /* Not a VxD, let ntdll handle it */

    if (lpOverlapped)
    {
        LPVOID cvalue = ((ULONG_PTR)lpOverlapped->hEvent & 1) ? NULL : lpOverlapped;
        lpOverlapped->Internal = STATUS_PENDING;
        lpOverlapped->InternalHigh = 0;
        if (HIWORD(dwIoControlCode) == FILE_DEVICE_FILE_SYSTEM)
            status = NtFsControlFile(hDevice, lpOverlapped->hEvent,
                                     NULL, cvalue, (PIO_STATUS_BLOCK)lpOverlapped,
                                     dwIoControlCode, lpvInBuffer, cbInBuffer,
                                     lpvOutBuffer, cbOutBuffer);
        else
            status = NtDeviceIoControlFile(hDevice, lpOverlapped->hEvent,
                                           NULL, cvalue, (PIO_STATUS_BLOCK)lpOverlapped,
                                           dwIoControlCode, lpvInBuffer, cbInBuffer,
                                           lpvOutBuffer, cbOutBuffer);
        if (lpcbBytesReturned) *lpcbBytesReturned = lpOverlapped->InternalHigh;
    }
    else
    {
        IO_STATUS_BLOCK iosb;

        if (HIWORD(dwIoControlCode) == FILE_DEVICE_FILE_SYSTEM)
            status = NtFsControlFile(hDevice, NULL, NULL, NULL, &iosb,
                                     dwIoControlCode, lpvInBuffer, cbInBuffer,
                                     lpvOutBuffer, cbOutBuffer);
        else
            status = NtDeviceIoControlFile(hDevice, NULL, NULL, NULL, &iosb,
                                           dwIoControlCode, lpvInBuffer, cbInBuffer,
                                           lpvOutBuffer, cbOutBuffer);
        if (lpcbBytesReturned) *lpcbBytesReturned = iosb.Information;
    }
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           OpenFile   (KERNEL32.@)
 */
HFILE WINAPI OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    HANDLE handle;
    FILETIME filetime;
    WORD filedatetime[2];

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

    if (!name) return HFILE_ERROR;

    TRACE("%s %04x\n", name, mode );

    /* the watcom 10.6 IDE relies on a valid path returned in ofs->szPathName
       Are there any cases where getting the path here is wrong?
       Uwe Bonnes 1997 Apr 2 */
    if (!GetFullPathNameA( name, sizeof(ofs->szPathName), ofs->szPathName, NULL )) goto error;

    /* OF_PARSE simply fills the structure */

    if (mode & OF_PARSE)
    {
        ofs->fFixedDisk = (GetDriveTypeA( ofs->szPathName ) != DRIVE_REMOVABLE);
        TRACE("(%s): OF_PARSE, res = '%s'\n", name, ofs->szPathName );
        return 0;
    }

    /* OF_CREATE is completely different from all other options, so
       handle it first */

    if (mode & OF_CREATE)
    {
        if ((handle = create_file_OF( name, mode )) == INVALID_HANDLE_VALUE)
            goto error;
    }
    else
    {
        /* Now look for the file */

        if (!SearchPathA( NULL, name, NULL, sizeof(ofs->szPathName), ofs->szPathName, NULL ))
            goto error;

        TRACE("found %s\n", debugstr_a(ofs->szPathName) );

        if (mode & OF_DELETE)
        {
            if (!DeleteFileA( ofs->szPathName )) goto error;
            TRACE("(%s): OF_DELETE return = OK\n", name);
            return TRUE;
        }

        handle = LongToHandle(_lopen( ofs->szPathName, mode ));
        if (handle == INVALID_HANDLE_VALUE) goto error;

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
    }
    TRACE("(%s): OK, return = %p\n", name, handle );
    if (mode & OF_EXIST)  /* Return TRUE instead of a handle */
    {
        CloseHandle( handle );
        return TRUE;
    }
    return HandleToLong(handle);

error:  /* We get here if there was an error opening the file */
    ofs->nErrCode = GetLastError();
    WARN("(%s): return = HFILE_ERROR error= %d\n", name,ofs->nErrCode );
    return HFILE_ERROR;
}


/***********************************************************************
 *           K32EnumDeviceDrivers (KERNEL32.@)
 */
BOOL WINAPI K32EnumDeviceDrivers(void **image_base, DWORD cb, DWORD *needed)
{
    FIXME("(%p, %d, %p): stub\n", image_base, cb, needed);

    if (needed)
        *needed = 0;

    return TRUE;
}

/***********************************************************************
 *          K32GetDeviceDriverBaseNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverBaseNameA(void *image_base, LPSTR base_name, DWORD size)
{
    FIXME("(%p, %p, %d): stub\n", image_base, base_name, size);

    if (base_name && size)
        base_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverBaseNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverBaseNameW(void *image_base, LPWSTR base_name, DWORD size)
{
    FIXME("(%p, %p, %d): stub\n", image_base, base_name, size);

    if (base_name && size)
        base_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverFileNameA(void *image_base, LPSTR file_name, DWORD size)
{
    FIXME("(%p, %p, %d): stub\n", image_base, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverFileNameW(void *image_base, LPWSTR file_name, DWORD size)
{
    FIXME("(%p, %p, %d): stub\n", image_base, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetFinalPathNameByHandleW (KERNEL32.@)
 */
DWORD WINAPI GetFinalPathNameByHandleW(HANDLE file, LPWSTR path, DWORD charcount, DWORD flags)
{
    WCHAR buffer[sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH + 1];
    OBJECT_NAME_INFORMATION *info = (OBJECT_NAME_INFORMATION*)&buffer;
    WCHAR drive_part[MAX_PATH];
    DWORD drive_part_len = 0;
    NTSTATUS status;
    DWORD result = 0;
    ULONG dummy;
    WCHAR *ptr;

    TRACE( "(%p,%p,%d,%x)\n", file, path, charcount, flags );

    if (flags & ~(FILE_NAME_OPENED | VOLUME_NAME_GUID | VOLUME_NAME_NONE | VOLUME_NAME_NT))
    {
        WARN("Unknown flags: %x\n", flags);
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* get object name */
    status = NtQueryObject( file, ObjectNameInformation, &buffer, sizeof(buffer) - sizeof(WCHAR), &dummy );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return 0;
    }
    if (!info->Name.Buffer)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }
    if (info->Name.Length < 4 * sizeof(WCHAR) || info->Name.Buffer[0] != '\\' ||
             info->Name.Buffer[1] != '?' || info->Name.Buffer[2] != '?' || info->Name.Buffer[3] != '\\' )
    {
        FIXME("Unexpected object name: %s\n", debugstr_wn(info->Name.Buffer, info->Name.Length / sizeof(WCHAR)));
        SetLastError( ERROR_GEN_FAILURE );
        return 0;
    }

    /* add terminating null character, remove "\\??\\" */
    info->Name.Buffer[info->Name.Length / sizeof(WCHAR)] = 0;
    info->Name.Length -= 4 * sizeof(WCHAR);
    info->Name.Buffer += 4;

    /* FILE_NAME_OPENED is not supported yet, and would require Wineserver changes */
    if (flags & FILE_NAME_OPENED)
    {
        FIXME("FILE_NAME_OPENED not supported\n");
        flags &= ~FILE_NAME_OPENED;
    }

    /* Get information required for VOLUME_NAME_NONE, VOLUME_NAME_GUID and VOLUME_NAME_NT */
    if (flags == VOLUME_NAME_NONE || flags == VOLUME_NAME_GUID || flags == VOLUME_NAME_NT)
    {
        if (!GetVolumePathNameW( info->Name.Buffer, drive_part, MAX_PATH ))
            return 0;

        drive_part_len = strlenW(drive_part);
        if (!drive_part_len || drive_part_len > strlenW(info->Name.Buffer) ||
                drive_part[drive_part_len-1] != '\\' ||
                strncmpiW( info->Name.Buffer, drive_part, drive_part_len ))
        {
            FIXME("Path %s returned by GetVolumePathNameW does not match file path %s\n",
                debugstr_w(drive_part), debugstr_w(info->Name.Buffer));
            SetLastError( ERROR_GEN_FAILURE );
            return 0;
        }
    }

    if (flags == VOLUME_NAME_NONE)
    {
        ptr    = info->Name.Buffer + drive_part_len - 1;
        result = strlenW(ptr);
        if (result < charcount)
            memcpy(path, ptr, (result + 1) * sizeof(WCHAR));
        else result++;
    }
    else if (flags == VOLUME_NAME_GUID)
    {
        WCHAR volume_prefix[51];

        /* GetVolumeNameForVolumeMountPointW sets error code on failure */
        if (!GetVolumeNameForVolumeMountPointW( drive_part, volume_prefix, 50 ))
            return 0;

        ptr    = info->Name.Buffer + drive_part_len;
        result = strlenW(volume_prefix) + strlenW(ptr);
        if (result < charcount)
        {
            path[0] = 0;
            strcatW(path, volume_prefix);
            strcatW(path, ptr);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else if (flags == VOLUME_NAME_NT)
    {
        WCHAR nt_prefix[MAX_PATH];

        /* QueryDosDeviceW sets error code on failure */
        drive_part[drive_part_len - 1] = 0;
        if (!QueryDosDeviceW( drive_part, nt_prefix, MAX_PATH ))
            return 0;

        ptr    = info->Name.Buffer + drive_part_len - 1;
        result = strlenW(nt_prefix) + strlenW(ptr);
        if (result < charcount)
        {
            path[0] = 0;
            strcatW(path, nt_prefix);
            strcatW(path, ptr);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else if (flags == VOLUME_NAME_DOS)
    {
        static const WCHAR dos_prefix[] = {'\\','\\','?','\\', '\0'};

        result = strlenW(dos_prefix) + strlenW(info->Name.Buffer);
        if (result < charcount)
        {
            path[0] = 0;
            strcatW(path, dos_prefix);
            strcatW(path, info->Name.Buffer);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else
    {
        /* Windows crashes here, but we prefer returning ERROR_INVALID_PARAMETER */
        WARN("Invalid combination of flags: %x\n", flags);
        SetLastError( ERROR_INVALID_PARAMETER );
    }

    return result;
}

/***********************************************************************
 *           GetFinalPathNameByHandleA (KERNEL32.@)
 */
DWORD WINAPI GetFinalPathNameByHandleA(HANDLE file, LPSTR path, DWORD charcount, DWORD flags)
{
    WCHAR *str;
    DWORD result, len, cp;

    TRACE( "(%p,%p,%d,%x)\n", file, path, charcount, flags);

    len = GetFinalPathNameByHandleW(file, NULL, 0, flags);
    if (len == 0)
        return 0;

    str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!str)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    result = GetFinalPathNameByHandleW(file, str, len, flags);
    if (result != len - 1)
    {
        HeapFree(GetProcessHeap(), 0, str);
        WARN("GetFinalPathNameByHandleW failed unexpectedly: %u\n", result);
        return 0;
    }

    cp = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    len = WideCharToMultiByte(cp, 0, str, -1, NULL, 0, NULL, NULL);
    if (!len)
    {
        HeapFree(GetProcessHeap(), 0, str);
        WARN("Failed to get multibyte length\n");
        return 0;
    }

    if (charcount < len)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return len - 1;
    }

    len = WideCharToMultiByte(cp, 0, str, -1, path, charcount, NULL, NULL);
    if (!len)
    {
        HeapFree(GetProcessHeap(), 0, str);
        WARN("WideCharToMultiByte failed\n");
        return 0;
    }

    HeapFree(GetProcessHeap(), 0, str);

    return len - 1;
}
