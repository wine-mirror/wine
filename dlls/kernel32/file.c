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

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

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
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

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

    if (srclen < 0) srclen = lstrlenW( src ) + 1;
    if (!destlen)
    {
        if (!AreFileApisANSI())
        {
            UNICODE_STRING strW;
            strW.Buffer = (WCHAR *)src;
            strW.Length = srclen * sizeof(WCHAR);
            ret = RtlUnicodeStringToOemSize( &strW ) - 1;
        }
        else
            RtlUnicodeToMultiByteSize( &ret, src, srclen * sizeof(WCHAR) );
    }
    else
    {
        if (!AreFileApisANSI())
            RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
        else
            RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    }
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

    TRACE("%d %p %ld\n", handle, buffer, count );

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

    info.Flags = flags;
    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info),
                                               FileIoCompletionNotificationInformation ));
}


/*************************************************************************
 *           SetHandleCount   (KERNEL32.@)
 */
UINT WINAPI SetHandleCount( UINT count )
{
    return count;
}


/***********************************************************************
 *           DosDateTimeToFileTime   (KERNEL32.@)
 */
BOOL WINAPI DosDateTimeToFileTime( WORD fatdate, WORD fattime, FILETIME *ft )
{
    TIME_FIELDS fields;
    LARGE_INTEGER time;

    fields.Year         = (fatdate >> 9) + 1980;
    fields.Month        = ((fatdate >> 5) & 0x0f);
    fields.Day          = (fatdate & 0x1f);
    fields.Hour         = (fattime >> 11);
    fields.Minute       = (fattime >> 5) & 0x3f;
    fields.Second       = (fattime & 0x1f) * 2;
    fields.Milliseconds = 0;
    if (!RtlTimeFieldsToTime( &fields, &time )) return FALSE;
    ft->dwLowDateTime  = time.u.LowPart;
    ft->dwHighDateTime = time.u.HighPart;
    return TRUE;
}


/***********************************************************************
 *           FileTimeToDosDateTime   (KERNEL32.@)
 */
BOOL WINAPI FileTimeToDosDateTime( const FILETIME *ft, WORD *fatdate, WORD *fattime )
{
    TIME_FIELDS fields;
    LARGE_INTEGER time;

    if (!fatdate || !fattime)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    time.u.LowPart  = ft->dwLowDateTime;
    time.u.HighPart = ft->dwHighDateTime;
    RtlTimeToTimeFields( &time, &fields );
    if (fields.Year < 1980)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *fattime = (fields.Hour << 11) + (fields.Minute << 5) + (fields.Second / 2);
    *fatdate = ((fields.Year - 1980) << 9) + (fields.Month << 5) + fields.Day;
    return TRUE;
}


/**************************************************************************
 *                      Operations on file names                          *
 **************************************************************************/


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
BOOL WINAPI KERNEL32_DeviceIoControl( HANDLE handle, DWORD code, void *in_buff, DWORD in_count,
                                      void *out_buff, DWORD out_count, DWORD *returned,
                                      OVERLAPPED *overlapped )
{
    TRACE( "(%p,%#lx,%p,%ld,%p,%ld,%p,%p)\n",
           handle, code, in_buff, in_count, out_buff, out_count, returned, overlapped );

    /* Check if this is a user defined control code for a VxD */

    if (HIWORD( code ) == 0 && (GetVersion() & 0x80000000))
    {
        typedef BOOL (WINAPI *DeviceIoProc)(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
        static DeviceIoProc (*vxd_get_proc)(HANDLE);
        DeviceIoProc proc = NULL;

        if (!vxd_get_proc) vxd_get_proc = (void *)GetProcAddress( GetModuleHandleW(L"krnl386.exe16"),
                                                                  "__wine_vxd_get_proc" );
        if (vxd_get_proc) proc = vxd_get_proc( handle );
        if (proc) return proc( code, in_buff, in_count, out_buff, out_count, returned, overlapped );
    }

    return DeviceIoControl( handle, code, in_buff, in_count, out_buff, out_count, returned, overlapped );
}


/***********************************************************************
 *           OpenFile   (KERNEL32.@)
 */
HFILE WINAPI OpenFile( LPCSTR name, OFSTRUCT *ofs, UINT mode )
{
    HANDLE handle;
    FILETIME filetime;
    WORD filedatetime[2];
    DWORD len;

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
    len = GetFullPathNameA( name, sizeof(ofs->szPathName), ofs->szPathName, NULL );
    if (!len) goto error;
    if (len >= sizeof(ofs->szPathName))
    {
        SetLastError(ERROR_INVALID_DATA);
        goto error;
    }

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

        len = SearchPathA( NULL, name, NULL, sizeof(ofs->szPathName), ofs->szPathName, NULL );
        if (!len) goto error;
        if (len >= sizeof(ofs->szPathName))
        {
            SetLastError(ERROR_INVALID_DATA);
            goto error;
        }

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
