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

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "wine/server.h"

#include "msdos.h"
#include "kernel_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/***********************************************************************
 *           _hread16   (KERNEL.349)
 */
LONG WINAPI _hread16( HFILE16 hFile, LPVOID buffer, LONG count)
{
    return _lread( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, count );
}


/***********************************************************************
 *           _hwrite   (KERNEL.350)
 */
LONG WINAPI _hwrite16( HFILE16 hFile, LPCSTR buffer, LONG count )
{
    return _hwrite( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, count );
}


/***********************************************************************
 *           _lcreat   (KERNEL.83)
 */
HFILE16 WINAPI _lcreat16( LPCSTR path, INT16 attr )
{
    return Win32HandleToDosFileHandle( (HANDLE)_lcreat( path, attr ) );
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
 *           _lopen   (KERNEL.85)
 */
HFILE16 WINAPI _lopen16( LPCSTR path, INT16 mode )
{
    return Win32HandleToDosFileHandle( (HANDLE)_lopen( path, mode ) );
}


/***********************************************************************
 *           _lread16   (KERNEL.82)
 */
UINT16 WINAPI _lread16( HFILE16 hFile, LPVOID buffer, UINT16 count )
{
    return (UINT16)_lread((HFILE)DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
}


/***********************************************************************
 *           _lwrite   (KERNEL.86)
 */
UINT16 WINAPI _lwrite16( HFILE16 hFile, LPCSTR buffer, UINT16 count )
{
    return (UINT16)_hwrite( (HFILE)DosFileHandleToWin32Handle(hFile), buffer, (LONG)count );
}

/***********************************************************************
 *           _hread (KERNEL.349)
 */
LONG WINAPI WIN16_hread( HFILE16 hFile, SEGPTR buffer, LONG count )
{
    LONG maxlen;

    TRACE("%d %08lx %ld\n", hFile, (DWORD)buffer, count );

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
 *           DeleteFile   (KERNEL.146)
 */
BOOL16 WINAPI DeleteFile16( LPCSTR path )
{
    return DeleteFileA( path );
}

/**************************************************************************
 *           GetFileAttributes   (KERNEL.420)
 */
DWORD WINAPI GetFileAttributes16( LPCSTR name )
{
    return GetFileAttributesA( name );
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
    {
        GetCurrentDirectoryA(sizeof(temppath), temppath); 
        drive |= temppath[0];
    }

    if (drive & TF_FORCEDRIVE)
    {
        char    d[3];

        d[0] = drive & ~TF_FORCEDRIVE;
        d[1] = ':';
        d[2] = '\0';
        if (GetDriveTypeA(d) == DRIVE_NO_ROOT_DIR)
        {
            drive &= ~TF_FORCEDRIVE;
            WARN("invalid drive %d specified\n", drive );
        }
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

/**************************************************************************
 *              SetFileAttributes	(KERNEL.421)
 */
BOOL16 WINAPI SetFileAttributes16( LPCSTR lpFileName, DWORD attributes )
{
    return SetFileAttributesA( lpFileName, attributes );
}


/***********************************************************************
 *           SetHandleCount   (KERNEL.199)
 */
UINT16 WINAPI SetHandleCount16( UINT16 count )
{
    return SetHandleCount( count );
}


/*************************************************************************
 *           FindFirstFile   (KERNEL.413)
 */
HANDLE16 WINAPI FindFirstFile16( LPCSTR path, WIN32_FIND_DATAA *data )
{
    HGLOBAL16 h16;
    HANDLE handle, *ptr;

    if (!(h16 = GlobalAlloc16( GMEM_MOVEABLE, sizeof(handle) ))) return INVALID_HANDLE_VALUE16;
    ptr = GlobalLock16( h16 );
    *ptr = handle = FindFirstFileA( path, data );
    GlobalUnlock16( h16 );

    if (handle == INVALID_HANDLE_VALUE)
    {
        GlobalFree16( h16 );
        h16 = INVALID_HANDLE_VALUE16;
    }
    return h16;
}


/*************************************************************************
 *           FindNextFile   (KERNEL.414)
 */
BOOL16 WINAPI FindNextFile16( HANDLE16 handle, WIN32_FIND_DATAA *data )
{
    HANDLE *ptr;
    BOOL ret = FALSE;

    if ((handle == INVALID_HANDLE_VALUE16) || !(ptr = GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    ret = FindNextFileA( *ptr, data );
    GlobalUnlock16( handle );
    return ret;
}


/*************************************************************************
 *           FindClose   (KERNEL.415)
 */
BOOL16 WINAPI FindClose16( HANDLE16 handle )
{
    HANDLE *ptr;

    if ((handle == INVALID_HANDLE_VALUE16) || !(ptr = GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    FindClose( *ptr );
    GlobalUnlock16( handle );
    GlobalFree16( handle );
    return TRUE;
}
