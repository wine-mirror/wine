/*
 * msvcrt.dll drive/directory functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
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

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* INTERNAL: Translate WIN32_FIND_DATAA to finddata_t  */
static void msvcrt_fttofd( const WIN32_FIND_DATAA *fd, struct MSVCRT__finddata_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAA to finddata32_t  */
static void msvcrt_fttofd32( const WIN32_FIND_DATAA *fd, struct MSVCRT__finddata32_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAW to wfinddata_t  */
static void msvcrt_wfttofd( const WIN32_FIND_DATAW *fd, struct MSVCRT__wfinddata_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpyW(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAW to wfinddata32_t  */
static void msvcrt_wfttofd32(const WIN32_FIND_DATAW *fd, struct MSVCRT__wfinddata32_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpyW(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAA to finddatai64_t  */
static void msvcrt_fttofdi64( const WIN32_FIND_DATAA *fd, struct MSVCRT__finddatai64_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = ((__int64)fd->nFileSizeHigh) << 32 | fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAA to finddata64_t  */
static void msvcrt_fttofd64( const WIN32_FIND_DATAA *fd, struct MSVCRT__finddata64_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = ((__int64)fd->nFileSizeHigh) << 32 | fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAW to wfinddata64_t  */
static void msvcrt_wfttofd64( const WIN32_FIND_DATAW *fd, struct MSVCRT__wfinddata64_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = ((__int64)fd->nFileSizeHigh) << 32 | fd->nFileSizeLow;
  strcpyW(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAA to finddata64i32_t  */
static void msvcrt_fttofd64i32( const WIN32_FIND_DATAA *fd, struct MSVCRT__finddata64i32_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAW to wfinddatai64_t  */
static void msvcrt_wfttofdi64( const WIN32_FIND_DATAW *fd, struct MSVCRT__wfinddatai64_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = ((__int64)fd->nFileSizeHigh) << 32 | fd->nFileSizeLow;
  strcpyW(ft->name, fd->cFileName);
}

/* INTERNAL: Translate WIN32_FIND_DATAW to wfinddata64i32_t  */
static void msvcrt_wfttofd64i32( const WIN32_FIND_DATAW *fd, struct MSVCRT__wfinddata64i32_t* ft)
{
  DWORD dw;

  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( (const LARGE_INTEGER *)&fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpyW(ft->name, fd->cFileName);
}

/*********************************************************************
 *		_chdir (MSVCRT.@)
 *
 * Change the current working directory.
 *
 * PARAMS
 *  newdir [I] Directory to change to
 *
 * RETURNS
 *  Success: 0. The current working directory is set to newdir.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See SetCurrentDirectoryA.
 */
int CDECL MSVCRT__chdir(const char * newdir)
{
  if (!SetCurrentDirectoryA(newdir))
  {
    msvcrt_set_errno(newdir?GetLastError():0);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_wchdir (MSVCRT.@)
 *
 * Unicode version of _chdir.
 */
int CDECL MSVCRT__wchdir(const MSVCRT_wchar_t * newdir)
{
  if (!SetCurrentDirectoryW(newdir))
  {
    msvcrt_set_errno(newdir?GetLastError():0);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_chdrive (MSVCRT.@)
 *
 * Change the current drive.
 *
 * PARAMS
 *  newdrive [I] Drive number to change to (1 = 'A', 2 = 'B', ...)
 *
 * RETURNS
 *  Success: 0. The current drive is set to newdrive.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See SetCurrentDirectoryA.
 */
int CDECL MSVCRT__chdrive(int newdrive)
{
  WCHAR buffer[] = {'A', ':', 0};

  buffer[0] += newdrive - 1;
  if (!SetCurrentDirectoryW( buffer ))
  {
    msvcrt_set_errno(GetLastError());
    if (newdrive <= 0)
      *MSVCRT__errno() = MSVCRT_EACCES;
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_findclose (MSVCRT.@)
 *
 * Close a handle returned by _findfirst().
 *
 * PARAMS
 *  hand [I] Handle to close
 *
 * RETURNS
 *  Success: 0. All resources associated with hand are freed.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See FindClose.
 */
int CDECL MSVCRT__findclose(MSVCRT_intptr_t hand)
{
  TRACE(":handle %ld\n",hand);
  if (!FindClose((HANDLE)hand))
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_findfirst (MSVCRT.@)
 *
 * Open a handle for iterating through a directory.
 *
 * PARAMS
 *  fspec [I] File specification of files to iterate.
 *  ft    [O] Information for the first file found.
 *
 * RETURNS
 *  Success: A handle suitable for passing to _findnext() and _findclose().
 *           ft is populated with the details of the found file.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See FindFirstFileA.
 */
MSVCRT_intptr_t CDECL MSVCRT__findfirst(const char * fspec, struct MSVCRT__finddata_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_fttofd(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *              _findfirst32 (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL MSVCRT__findfirst32(const char * fspec, struct MSVCRT__finddata32_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_fttofd32(&find_data, ft);
  TRACE(":got handle %p\n", hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_wfindfirst (MSVCRT.@)
 *
 * Unicode version of _findfirst.
 */
MSVCRT_intptr_t CDECL MSVCRT__wfindfirst(const MSVCRT_wchar_t * fspec, struct MSVCRT__wfinddata_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind  = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_wfttofd(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *              _wfindfirst32 (MSVCRT.@)
 *
 * Unicode version of _findfirst32.
 */
MSVCRT_intptr_t CDECL MSVCRT__wfindfirst32(const MSVCRT_wchar_t * fspec, struct MSVCRT__wfinddata32_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_wfttofd32(&find_data, ft);
  TRACE(":got handle %p\n", hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_findfirsti64 (MSVCRT.@)
 *
 * 64-bit version of _findfirst.
 */
MSVCRT_intptr_t CDECL MSVCRT__findfirsti64(const char * fspec, struct MSVCRT__finddatai64_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_fttofdi64(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_findfirst64 (MSVCRT.@)
 *
 * 64-bit version of _findfirst.
 */
MSVCRT_intptr_t CDECL MSVCRT__findfirst64(const char * fspec, struct MSVCRT__finddata64_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_fttofd64(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_wfindfirst64 (MSVCRT.@)
 *
 * Unicode version of _findfirst64.
 */
MSVCRT_intptr_t CDECL MSVCRT__wfindfirst64(const MSVCRT_wchar_t * fspec, struct MSVCRT__wfinddata64_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind  = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_wfttofd64(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_findfirst64i32 (MSVCRT.@)
 *
 * 64-bit/32-bit version of _findfirst.
 */
MSVCRT_intptr_t CDECL MSVCRT__findfirst64i32(const char * fspec, struct MSVCRT__finddata64i32_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_fttofd64i32(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_wfindfirst64i32 (MSVCRT.@)
 *
 * Unicode version of _findfirst64i32.
 */
MSVCRT_intptr_t CDECL MSVCRT__wfindfirst64i32(const MSVCRT_wchar_t * fspec, struct MSVCRT__wfinddata64i32_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind  = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_wfttofd64i32(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_wfindfirsti64 (MSVCRT.@)
 *
 * Unicode version of _findfirsti64.
 */
MSVCRT_intptr_t CDECL MSVCRT__wfindfirsti64(const MSVCRT_wchar_t * fspec, struct MSVCRT__wfinddatai64_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind  = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  msvcrt_wfttofdi64(&find_data,ft);
  TRACE(":got handle %p\n",hfind);
  return (MSVCRT_intptr_t)hfind;
}

/*********************************************************************
 *		_findnext (MSVCRT.@)
 *
 * Find the next file from a file search handle.
 *
 * PARAMS
 *  hand  [I] Handle to the search returned from _findfirst().
 *  ft    [O] Information for the file found.
 *
 * RETURNS
 *  Success: 0. ft is populated with the details of the found file.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See FindNextFileA.
 */
int CDECL MSVCRT__findnext(MSVCRT_intptr_t hand, struct MSVCRT__finddata_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_fttofd(&find_data,ft);
  return 0;
}

/*********************************************************************
 *               _findnext32 (MSVCRT.@)
 */
int CDECL MSVCRT__findnext32(MSVCRT_intptr_t hand, struct MSVCRT__finddata32_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_fttofd32(&find_data, ft);
  return 0;
}

/*********************************************************************
 *		_wfindnext (MSVCRT.@)
 *
 * Unicode version of _findnext.
 */
int CDECL MSVCRT__wfindnext(MSVCRT_intptr_t hand, struct MSVCRT__wfinddata_t * ft)
{
  WIN32_FIND_DATAW find_data;

  if (!FindNextFileW((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_wfttofd(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_findnexti64 (MSVCRT.@)
 *
 * 64-bit version of _findnext.
 */
int CDECL MSVCRT__findnexti64(MSVCRT_intptr_t hand, struct MSVCRT__finddatai64_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_fttofdi64(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_findnext64 (MSVCRT.@)
 *
 * 64-bit version of _findnext.
 */
int CDECL MSVCRT__findnext64(MSVCRT_intptr_t hand, struct MSVCRT__finddata64_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_fttofd64(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_wfindnext64 (MSVCRT.@)
 *
 * Unicode version of _wfindnext64.
 */
int CDECL MSVCRT__wfindnext64(MSVCRT_intptr_t hand, struct MSVCRT__wfinddata64_t * ft)
{
  WIN32_FIND_DATAW find_data;

  if (!FindNextFileW((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_wfttofd64(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_findnext64i32 (MSVCRT.@)
 *
 * 64-bit/32-bit version of _findnext.
 */
int CDECL MSVCRT__findnext64i32(MSVCRT_intptr_t hand, struct MSVCRT__finddata64i32_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_fttofd64i32(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_wfindnexti64 (MSVCRT.@)
 *
 * Unicode version of _findnexti64.
 */
int CDECL MSVCRT__wfindnexti64(MSVCRT_intptr_t hand, struct MSVCRT__wfinddatai64_t * ft)
{
  WIN32_FIND_DATAW find_data;

  if (!FindNextFileW((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_wfttofdi64(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_wfindnext64i32 (MSVCRT.@)
 *
 * Unicode version of _findnext64i32.
 */
int CDECL MSVCRT__wfindnext64i32(MSVCRT_intptr_t hand, struct MSVCRT__wfinddata64i32_t * ft)
{
  WIN32_FIND_DATAW find_data;

  if (!FindNextFileW((HANDLE)hand, &find_data))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  msvcrt_wfttofd64i32(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_getcwd (MSVCRT.@)
 *
 * Get the current working directory.
 *
 * PARAMS
 *  buf  [O] Destination for current working directory.
 *  size [I] Size of buf in characters
 *
 * RETURNS
 * Success: If buf is NULL, returns an allocated string containing the path.
 *          Otherwise populates buf with the path and returns it.
 * Failure: NULL. errno indicates the error.
 */
char* CDECL MSVCRT__getcwd(char * buf, int size)
{
  char dir[MAX_PATH];
  int dir_len = GetCurrentDirectoryA(MAX_PATH,dir);

  if (dir_len < 1)
    return NULL; /* FIXME: Real return value untested */

  if (!buf)
  {
      if (size <= dir_len) size = dir_len + 1;
      if (!(buf = MSVCRT_malloc( size ))) return NULL;
  }
  else if (dir_len >= size)
  {
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return NULL; /* buf too small */
  }
  strcpy(buf,dir);
  return buf;
}

/*********************************************************************
 *		_wgetcwd (MSVCRT.@)
 *
 * Unicode version of _getcwd.
 */
MSVCRT_wchar_t* CDECL MSVCRT__wgetcwd(MSVCRT_wchar_t * buf, int size)
{
  MSVCRT_wchar_t dir[MAX_PATH];
  int dir_len = GetCurrentDirectoryW(MAX_PATH,dir);

  if (dir_len < 1)
    return NULL; /* FIXME: Real return value untested */

  if (!buf)
  {
      if (size <= dir_len) size = dir_len + 1;
      if (!(buf = MSVCRT_malloc( size * sizeof(WCHAR) ))) return NULL;
  }
  if (dir_len >= size)
  {
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return NULL; /* buf too small */
  }
  strcpyW(buf,dir);
  return buf;
}

/*********************************************************************
 *		_getdrive (MSVCRT.@)
 *
 * Get the current drive number.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: The drive letter number from 1 to 26 ("A:" to "Z:").
 *  Failure: 0.
 */
int CDECL MSVCRT__getdrive(void)
{
    WCHAR buffer[MAX_PATH];
    if (GetCurrentDirectoryW( MAX_PATH, buffer ) &&
        buffer[0] >= 'A' && buffer[0] <= 'z' && buffer[1] == ':')
        return toupperW(buffer[0]) - 'A' + 1;
    return 0;
}

/*********************************************************************
 *		_getdcwd (MSVCRT.@)
 *
 * Get the current working directory on a given disk.
 * 
 * PARAMS
 *  drive [I] Drive letter to get the current working directory from.
 *  buf   [O] Destination for the current working directory.
 *  size  [I] Length of drive in characters.
 *
 * RETURNS
 *  Success: If drive is NULL, returns an allocated string containing the path.
 *           Otherwise populates drive with the path and returns it.
 *  Failure: NULL. errno indicates the error.
 */
char* CDECL MSVCRT__getdcwd(int drive, char * buf, int size)
{
  static char* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == MSVCRT__getdrive())
    return MSVCRT__getcwd(buf,size); /* current */
  else
  {
    char dir[MAX_PATH];
    char drivespec[] = {'A', ':', 0};
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveTypeA(drivespec) < DRIVE_REMOVABLE)
    {
      *MSVCRT__errno() = MSVCRT_EACCES;
      return NULL;
    }

    dir_len = GetFullPathNameA(drivespec,MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      *MSVCRT__errno() = MSVCRT_ERANGE;
      return NULL; /* buf too small */
    }

    TRACE(":returning '%s'\n", dir);
    if (!buf)
      return MSVCRT__strdup(dir); /* allocate */

    strcpy(buf,dir);
  }
  return buf;
}

/*********************************************************************
 *		_wgetdcwd (MSVCRT.@)
 *
 * Unicode version of _wgetdcwd.
 */
MSVCRT_wchar_t* CDECL MSVCRT__wgetdcwd(int drive, MSVCRT_wchar_t * buf, int size)
{
  static MSVCRT_wchar_t* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == MSVCRT__getdrive())
    return MSVCRT__wgetcwd(buf,size); /* current */
  else
  {
    MSVCRT_wchar_t dir[MAX_PATH];
    MSVCRT_wchar_t drivespec[4] = {'A', ':', '\\', 0};
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveTypeW(drivespec) < DRIVE_REMOVABLE)
    {
      *MSVCRT__errno() = MSVCRT_EACCES;
      return NULL;
    }

    dir_len = GetFullPathNameW(drivespec,MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      *MSVCRT__errno() = MSVCRT_ERANGE;
      return NULL; /* buf too small */
    }

    TRACE(":returning %s\n", debugstr_w(dir));
    if (!buf)
      return MSVCRT__wcsdup(dir); /* allocate */
    strcpyW(buf,dir);
  }
  return buf;
}

/*********************************************************************
 *		_getdiskfree (MSVCRT.@)
 *
 * Get information about the free space on a drive.
 *
 * PARAMS
 *  disk [I] Drive number to get information about (1 = 'A', 2 = 'B', ...)
 *  info [O] Destination for the resulting information.
 *
 * RETURNS
 *  Success: 0. info is updated with the free space information.
 *  Failure: An error code from GetLastError().
 *
 * NOTES
 *  See GetLastError().
 */
unsigned int CDECL MSVCRT__getdiskfree(unsigned int disk, struct MSVCRT__diskfree_t * d)
{
  WCHAR drivespec[] = {'@', ':', '\\', 0};
  DWORD ret[4];
  unsigned int err;

  if (disk > 26)
    return ERROR_INVALID_PARAMETER; /* MSVCRT doesn't set errno here */

  drivespec[0] += disk; /* make a drive letter */

  if (GetDiskFreeSpaceW(disk==0?NULL:drivespec,ret,ret+1,ret+2,ret+3))
  {
    d->sectors_per_cluster = ret[0];
    d->bytes_per_sector = ret[1];
    d->avail_clusters = ret[2];
    d->total_clusters = ret[3];
    return 0;
  }
  err = GetLastError();
  msvcrt_set_errno(err);
  return err;
}

/*********************************************************************
 *		_mkdir (MSVCRT.@)
 *
 * Create a directory.
 *
 * PARAMS
 *  newdir [I] Name of directory to create.
 *
 * RETURNS
 *  Success: 0. The directory indicated by newdir is created.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See CreateDirectoryA.
 */
int CDECL MSVCRT__mkdir(const char * newdir)
{
  if (CreateDirectoryA(newdir,NULL))
    return 0;
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wmkdir (MSVCRT.@)
 *
 * Unicode version of _mkdir.
 */
int CDECL MSVCRT__wmkdir(const MSVCRT_wchar_t* newdir)
{
  if (CreateDirectoryW(newdir,NULL))
    return 0;
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_rmdir (MSVCRT.@)
 *
 * Delete a directory.
 *
 * PARAMS
 *  dir [I] Name of directory to delete.
 *
 * RETURNS
 *  Success: 0. The directory indicated by newdir is deleted.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See RemoveDirectoryA.
 */
int CDECL MSVCRT__rmdir(const char * dir)
{
  if (RemoveDirectoryA(dir))
    return 0;
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wrmdir (MSVCRT.@)
 *
 * Unicode version of _rmdir.
 */
int CDECL MSVCRT__wrmdir(const MSVCRT_wchar_t * dir)
{
  if (RemoveDirectoryW(dir))
    return 0;
  msvcrt_set_errno(GetLastError());
  return -1;
}

/******************************************************************
 *		_splitpath_s (MSVCRT.@)
 */
int CDECL MSVCRT__splitpath_s(const char* inpath,
        char* drive, MSVCRT_size_t sz_drive,
        char* dir, MSVCRT_size_t sz_dir,
        char* fname, MSVCRT_size_t sz_fname,
        char* ext, MSVCRT_size_t sz_ext)
{
    const char *p, *end;

    if (!inpath || (!drive && sz_drive) ||
            (drive && !sz_drive) ||
            (!dir && sz_dir) ||
            (dir && !sz_dir) ||
            (!fname && sz_fname) ||
            (fname && !sz_fname) ||
            (!ext && sz_ext) ||
            (ext && !sz_ext))
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (inpath[0] && inpath[1] == ':')
    {
        if (drive)
        {
            if (sz_drive <= 2) goto do_error;
            drive[0] = inpath[0];
            drive[1] = inpath[1];
            drive[2] = 0;
        }
        inpath += 2;
    }
    else if (drive) drive[0] = '\0';

    /* look for end of directory part */
    end = NULL;
    for (p = inpath; *p; p++)
    {
        if (_ismbblead((unsigned char)*p))
        {
            p++;
            continue;
        }
        if (*p == '/' || *p == '\\') end = p + 1;
    }

    if (end)  /* got a directory */
    {
        if (dir)
        {
            if (sz_dir <= end - inpath) goto do_error;
            memcpy( dir, inpath, (end - inpath) );
            dir[end - inpath] = 0;
        }
        inpath = end;
    }
    else if (dir) dir[0] = 0;

    /* look for extension: what's after the last dot */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '.') end = p;

    if (!end) end = p; /* there's no extension */

    if (fname)
    {
        if (sz_fname <= end - inpath) goto do_error;
        memcpy( fname, inpath, (end - inpath) );
        fname[end - inpath] = 0;
    }
    if (ext)
    {
        if (sz_ext <= strlen(end)) goto do_error;
        strcpy( ext, end );
    }
    return 0;
do_error:
    if (drive)  drive[0] = '\0';
    if (dir)    dir[0] = '\0';
    if (fname)  fname[0]= '\0';
    if (ext)    ext[0]= '\0';
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *              _splitpath (MSVCRT.@)
 */
void CDECL MSVCRT__splitpath(const char *inpath, char *drv, char *dir,
        char *fname, char *ext)
{
    MSVCRT__splitpath_s(inpath, drv, drv?MSVCRT__MAX_DRIVE:0, dir, dir?MSVCRT__MAX_DIR:0,
            fname, fname?MSVCRT__MAX_FNAME:0, ext, ext?MSVCRT__MAX_EXT:0);
}

/******************************************************************
 *		_wsplitpath_s (MSVCRT.@)
 *
 * Secure version of _wsplitpath
 */
int CDECL MSVCRT__wsplitpath_s(const MSVCRT_wchar_t* inpath,
                  MSVCRT_wchar_t* drive, MSVCRT_size_t sz_drive,
                  MSVCRT_wchar_t* dir, MSVCRT_size_t sz_dir,
                  MSVCRT_wchar_t* fname, MSVCRT_size_t sz_fname,
                  MSVCRT_wchar_t* ext, MSVCRT_size_t sz_ext)
{
    const MSVCRT_wchar_t *p, *end;

    if (!inpath || (!drive && sz_drive) ||
            (drive && !sz_drive) ||
            (!dir && sz_dir) ||
            (dir && !sz_dir) ||
            (!fname && sz_fname) ||
            (fname && !sz_fname) ||
            (!ext && sz_ext) ||
            (ext && !sz_ext))
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (inpath[0] && inpath[1] == ':')
    {
        if (drive)
        {
            if (sz_drive <= 2) goto do_error;
            drive[0] = inpath[0];
            drive[1] = inpath[1];
            drive[2] = 0;
        }
        inpath += 2;
    }
    else if (drive) drive[0] = '\0';

    /* look for end of directory part */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '/' || *p == '\\') end = p + 1;

    if (end)  /* got a directory */
    {
        if (dir)
        {
            if (sz_dir <= end - inpath) goto do_error;
            memcpy( dir, inpath, (end - inpath) * sizeof(MSVCRT_wchar_t) );
            dir[end - inpath] = 0;
        }
        inpath = end;
    }
    else if (dir) dir[0] = 0;

    /* look for extension: what's after the last dot */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '.') end = p;

    if (!end) end = p; /* there's no extension */

    if (fname)
    {
        if (sz_fname <= end - inpath) goto do_error;
        memcpy( fname, inpath, (end - inpath) * sizeof(MSVCRT_wchar_t) );
        fname[end - inpath] = 0;
    }
    if (ext)
    {
        if (sz_ext <= strlenW(end)) goto do_error;
        strcpyW( ext, end );
    }
    return 0;
do_error:
    if (drive)  drive[0] = '\0';
    if (dir)    dir[0] = '\0';
    if (fname)  fname[0]= '\0';
    if (ext)    ext[0]= '\0';
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *		_wsplitpath (MSVCRT.@)
 *
 * Unicode version of _splitpath.
 */
void CDECL MSVCRT__wsplitpath(const MSVCRT_wchar_t *inpath, MSVCRT_wchar_t *drv, MSVCRT_wchar_t *dir,
        MSVCRT_wchar_t *fname, MSVCRT_wchar_t *ext)
{
    MSVCRT__wsplitpath_s(inpath, drv, drv?MSVCRT__MAX_DRIVE:0, dir, dir?MSVCRT__MAX_DIR:0,
            fname, fname?MSVCRT__MAX_FNAME:0, ext, ext?MSVCRT__MAX_EXT:0);
}

/*********************************************************************
 *		_wfullpath (MSVCRT.@)
 *
 * Unicode version of _fullpath.
 */
MSVCRT_wchar_t * CDECL MSVCRT__wfullpath(MSVCRT_wchar_t * absPath, const MSVCRT_wchar_t* relPath, MSVCRT_size_t size)
{
  DWORD rc;
  WCHAR* buffer;
  WCHAR* lastpart;
  BOOL alloced = FALSE;

  if (!relPath || !*relPath)
    return MSVCRT__wgetcwd(absPath, size);

  if (absPath == NULL)
  {
      buffer = MSVCRT_malloc(MAX_PATH * sizeof(WCHAR));
      size = MAX_PATH;
      alloced = TRUE;
  }
  else
      buffer = absPath;

  if (size < 4)
  {
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return NULL;
  }

  TRACE(":resolving relative path %s\n",debugstr_w(relPath));

  rc = GetFullPathNameW(relPath,size,buffer,&lastpart);

  if (rc > 0 && rc <= size )
    return buffer;
  else
  {
      if (alloced)
          MSVCRT_free(buffer);
        return NULL;
  }
}

/*********************************************************************
 *		_fullpath (MSVCRT.@)
 *
 * Create an absolute path from a relative path.
 *
 * PARAMS
 *  absPath [O] Destination for absolute path
 *  relPath [I] Relative path to convert to absolute
 *  size    [I] Length of absPath in characters.
 *
 * RETURNS
 * Success: If absPath is NULL, returns an allocated string containing the path.
 *          Otherwise populates absPath with the path and returns it.
 * Failure: NULL. errno indicates the error.
 */
char * CDECL MSVCRT__fullpath(char * absPath, const char* relPath, unsigned int size)
{
  DWORD rc;
  char* lastpart;
  char* buffer;
  BOOL alloced = FALSE;

  if (!relPath || !*relPath)
    return MSVCRT__getcwd(absPath, size);

  if (absPath == NULL)
  {
      buffer = MSVCRT_malloc(MAX_PATH);
      size = MAX_PATH;
      alloced = TRUE;
  }
  else
      buffer = absPath;

  if (size < 4)
  {
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return NULL;
  }

  TRACE(":resolving relative path '%s'\n",relPath);

  rc = GetFullPathNameA(relPath,size,buffer,&lastpart);

  if (rc > 0 && rc <= size)
    return buffer;
  else
  {
      if (alloced)
          MSVCRT_free(buffer);
        return NULL;
  }
}

/*********************************************************************
 *		_makepath (MSVCRT.@)
 *
 * Create a pathname.
 *
 * PARAMS
 *  path      [O] Destination for created pathname
 *  drive     [I] Drive letter (e.g. "A:")
 *  directory [I] Directory
 *  filename  [I] Name of the file, excluding extension
 *  extension [I] File extension (e.g. ".TXT")
 *
 * RETURNS
 *  Nothing. If path is not large enough to hold the resulting pathname,
 *  random process memory will be overwritten.
 */
VOID CDECL MSVCRT__makepath(char * path, const char * drive,
                     const char *directory, const char * filename,
                     const char * extension)
{
    char *p = path;

    TRACE("(%s %s %s %s)\n", debugstr_a(drive), debugstr_a(directory),
          debugstr_a(filename), debugstr_a(extension) );

    if ( !path )
        return;

    if (drive && drive[0])
    {
        *p++ = drive[0];
        *p++ = ':';
    }
    if (directory && directory[0])
    {
        unsigned int len = strlen(directory);
        memmove(p, directory, len);
        p += len;
        if (p[-1] != '/' && p[-1] != '\\')
            *p++ = '\\';
    }
    if (filename && filename[0])
    {
        unsigned int len = strlen(filename);
        memmove(p, filename, len);
        p += len;
    }
    if (extension && extension[0])
    {
        if (extension[0] != '.')
            *p++ = '.';
        strcpy(p, extension);
    }
    else
        *p = '\0';
    TRACE("returning %s\n",path);
}

/*********************************************************************
 *		_wmakepath (MSVCRT.@)
 *
 * Unicode version of _wmakepath.
 */
VOID CDECL MSVCRT__wmakepath(MSVCRT_wchar_t *path, const MSVCRT_wchar_t *drive, const MSVCRT_wchar_t *directory,
                      const MSVCRT_wchar_t *filename, const MSVCRT_wchar_t *extension)
{
    MSVCRT_wchar_t *p = path;

    TRACE("%s %s %s %s\n", debugstr_w(drive), debugstr_w(directory),
          debugstr_w(filename), debugstr_w(extension));

    if ( !path )
        return;

    if (drive && drive[0])
    {
        *p++ = drive[0];
        *p++ = ':';
    }
    if (directory && directory[0])
    {
        unsigned int len = strlenW(directory);
        memmove(p, directory, len * sizeof(MSVCRT_wchar_t));
        p += len;
        if (p[-1] != '/' && p[-1] != '\\')
            *p++ = '\\';
    }
    if (filename && filename[0])
    {
        unsigned int len = strlenW(filename);
        memmove(p, filename, len * sizeof(MSVCRT_wchar_t));
        p += len;
    }
    if (extension && extension[0])
    {
        if (extension[0] != '.')
            *p++ = '.';
        strcpyW(p, extension);
    }
    else
        *p = '\0';

    TRACE("returning %s\n", debugstr_w(path));
}

/*********************************************************************
 *		_makepath_s (MSVCRT.@)
 *
 * Safe version of _makepath.
 */
int CDECL MSVCRT__makepath_s(char *path, MSVCRT_size_t size, const char *drive,
                      const char *directory, const char *filename,
                      const char *extension)
{
    char *p = path;

    if (!path || !size)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (drive && drive[0])
    {
        if (size <= 2)
            goto range;

        *p++ = drive[0];
        *p++ = ':';
        size -= 2;
    }

    if (directory && directory[0])
    {
        unsigned int len = strlen(directory);
        unsigned int needs_separator = directory[len - 1] != '/' && directory[len - 1] != '\\';
        unsigned int copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, directory, copylen);

        if (size <= len)
            goto range;

        p += copylen;
        size -= copylen;

        if (needs_separator)
        {
            if (size < 2)
                goto range;

            *p++ = '\\';
            size -= 1;
        }
    }

    if (filename && filename[0])
    {
        unsigned int len = strlen(filename);
        unsigned int copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, filename, copylen);

        if (size <= len)
            goto range;

        p += len;
        size -= len;
    }

    if (extension && extension[0])
    {
        unsigned int len = strlen(extension);
        unsigned int needs_period = extension[0] != '.';
        unsigned int copylen;

        if (size < 2)
            goto range;

        if (needs_period)
        {
            *p++ = '.';
            size -= 1;
        }

        copylen = min(size - 1, len);
        memcpy(p, extension, copylen);

        if (size <= len)
            goto range;

        p += copylen;
    }

    *p = '\0';
    return 0;

range:
    path[0] = '\0';
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *		_wmakepath_s (MSVCRT.@)
 *
 * Safe version of _wmakepath.
 */
int CDECL MSVCRT__wmakepath_s(MSVCRT_wchar_t *path, MSVCRT_size_t size, const MSVCRT_wchar_t *drive,
                       const MSVCRT_wchar_t *directory, const MSVCRT_wchar_t *filename,
                       const MSVCRT_wchar_t *extension)
{
    MSVCRT_wchar_t *p = path;

    if (!path || !size)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (drive && drive[0])
    {
        if (size <= 2)
            goto range;

        *p++ = drive[0];
        *p++ = ':';
        size -= 2;
    }

    if (directory && directory[0])
    {
        unsigned int len = strlenW(directory);
        unsigned int needs_separator = directory[len - 1] != '/' && directory[len - 1] != '\\';
        unsigned int copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, directory, copylen * sizeof(MSVCRT_wchar_t));

        if (size <= len)
            goto range;

        p += copylen;
        size -= copylen;

        if (needs_separator)
        {
            if (size < 2)
                goto range;

            *p++ = '\\';
            size -= 1;
        }
    }

    if (filename && filename[0])
    {
        unsigned int len = strlenW(filename);
        unsigned int copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, filename, copylen * sizeof(MSVCRT_wchar_t));

        if (size <= len)
            goto range;

        p += len;
        size -= len;
    }

    if (extension && extension[0])
    {
        unsigned int len = strlenW(extension);
        unsigned int needs_period = extension[0] != '.';
        unsigned int copylen;

        if (size < 2)
            goto range;

        if (needs_period)
        {
            *p++ = '.';
            size -= 1;
        }

        copylen = min(size - 1, len);
        memcpy(p, extension, copylen * sizeof(MSVCRT_wchar_t));

        if (size <= len)
            goto range;

        p += copylen;
    }

    *p = '\0';
    return 0;

range:
    path[0] = '\0';
    *MSVCRT__errno() = MSVCRT_ERANGE;
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *		_searchenv (MSVCRT.@)
 *
 * Search for a file in a list of paths from an environment variable.
 *
 * PARAMS
 *  file   [I] Name of the file to search for.
 *  env    [I] Name of the environment variable containing a list of paths.
 *  buf    [O] Destination for the found file path.
 *
 * RETURNS
 *  Nothing. If the file is not found, buf will contain an empty string
 *  and errno is set.
 */
void CDECL MSVCRT__searchenv(const char* file, const char* env, char *buf)
{
  char*envVal, *penv;
  char curPath[MAX_PATH];

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributesA( file ) != INVALID_FILE_ATTRIBUTES)
  {
    GetFullPathNameA( file, MAX_PATH, buf, NULL );
    /* Sigh. This error is *always* set, regardless of success */
    msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  /* Search given environment variable */
  envVal = MSVCRT_getenv(env);
  if (!envVal)
  {
    msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  penv = envVal;
  TRACE(":searching for %s in paths %s\n", file, envVal);

  do
  {
    char *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return;
    }
    memcpy(curPath, penv, end - penv);
    if (curPath[end - penv] != '/' && curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    strcat(curPath, file);
    TRACE("Checking for file %s\n", curPath);
    if (GetFileAttributesA( curPath ) != INVALID_FILE_ATTRIBUTES)
    {
      strcpy(buf, curPath);
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return; /* Found */
    }
    penv = *end ? end + 1 : end;
  } while(1);
}

/*********************************************************************
 *		_searchenv_s (MSVCRT.@)
 */
int CDECL MSVCRT__searchenv_s(const char* file, const char* env, char *buf, MSVCRT_size_t count)
{
  char*envVal, *penv;
  char curPath[MAX_PATH];

  if (!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EINVAL;
  if (!MSVCRT_CHECK_PMT(buf != NULL)) return MSVCRT_EINVAL;
  if (!MSVCRT_CHECK_PMT(count > 0)) return MSVCRT_EINVAL;

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributesA( file ) != INVALID_FILE_ATTRIBUTES)
  {
    if (GetFullPathNameA( file, count, buf, NULL )) return 0;
    msvcrt_set_errno(GetLastError());
    return 0;
  }

  /* Search given environment variable */
  envVal = MSVCRT_getenv(env);
  if (!envVal)
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return MSVCRT_ENOENT;
  }

  penv = envVal;
  TRACE(":searching for %s in paths %s\n", file, envVal);

  do
  {
    char *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      *MSVCRT__errno() = MSVCRT_ENOENT;
      return MSVCRT_ENOENT;
    }
    memcpy(curPath, penv, end - penv);
    if (curPath[end - penv] != '/' && curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    strcat(curPath, file);
    TRACE("Checking for file %s\n", curPath);
    if (GetFileAttributesA( curPath ) != INVALID_FILE_ATTRIBUTES)
    {
      if (strlen(curPath) + 1 > count)
      {
          MSVCRT_INVALID_PMT("buf[count] is too small", MSVCRT_ERANGE);
          return MSVCRT_ERANGE;
      }
      strcpy(buf, curPath);
      return 0;
    }
    penv = *end ? end + 1 : end;
  } while(1);
}

/*********************************************************************
 *      _wsearchenv (MSVCRT.@)
 *
 * Unicode version of _searchenv
 */
void CDECL MSVCRT__wsearchenv(const MSVCRT_wchar_t* file, const MSVCRT_wchar_t* env, MSVCRT_wchar_t *buf)
{
  MSVCRT_wchar_t *envVal, *penv;
  MSVCRT_wchar_t curPath[MAX_PATH];

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributesW( file ) != INVALID_FILE_ATTRIBUTES)
  {
    GetFullPathNameW( file, MAX_PATH, buf, NULL );
    /* Sigh. This error is *always* set, regardless of success */
    msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  /* Search given environment variable */
  envVal = MSVCRT__wgetenv(env);
  if (!envVal)
  {
    msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  penv = envVal;
  TRACE(":searching for %s in paths %s\n", debugstr_w(file), debugstr_w(envVal));

  do
  {
    MSVCRT_wchar_t *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return;
    }
    memcpy(curPath, penv, (end - penv) * sizeof(MSVCRT_wchar_t));
    if (curPath[end - penv] != '/' && curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    strcatW(curPath, file);
    TRACE("Checking for file %s\n", debugstr_w(curPath));
    if (GetFileAttributesW( curPath ) != INVALID_FILE_ATTRIBUTES)
    {
      strcpyW(buf, curPath);
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return; /* Found */
    }
    penv = *end ? end + 1 : end;
  } while(1);
}

/*********************************************************************
 *		_wsearchenv_s (MSVCRT.@)
 */
int CDECL MSVCRT__wsearchenv_s(const MSVCRT_wchar_t* file, const MSVCRT_wchar_t* env,
                        MSVCRT_wchar_t *buf, MSVCRT_size_t count)
{
  MSVCRT_wchar_t*       envVal, *penv;
  MSVCRT_wchar_t        curPath[MAX_PATH];

  if (!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EINVAL;
  if (!MSVCRT_CHECK_PMT(buf != NULL)) return MSVCRT_EINVAL;
  if (!MSVCRT_CHECK_PMT(count > 0)) return MSVCRT_EINVAL;

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributesW( file ) != INVALID_FILE_ATTRIBUTES)
  {
    if (GetFullPathNameW( file, count, buf, NULL )) return 0;
    msvcrt_set_errno(GetLastError());
    return 0;
  }

  /* Search given environment variable */
  envVal = MSVCRT__wgetenv(env);
  if (!envVal)
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return MSVCRT_ENOENT;
  }

  penv = envVal;
  TRACE(":searching for %s in paths %s\n", debugstr_w(file), debugstr_w(envVal));

  do
  {
    MSVCRT_wchar_t *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      *MSVCRT__errno() = MSVCRT_ENOENT;
      return MSVCRT_ENOENT;
    }
    memcpy(curPath, penv, (end - penv) * sizeof(MSVCRT_wchar_t));
    if (curPath[end - penv] != '/' && curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    strcatW(curPath, file);
    TRACE("Checking for file %s\n", debugstr_w(curPath));
    if (GetFileAttributesW( curPath ) != INVALID_FILE_ATTRIBUTES)
    {
      if (strlenW(curPath) + 1 > count)
      {
          MSVCRT_INVALID_PMT("buf[count] is too small", MSVCRT_ERANGE);
          return MSVCRT_ERANGE;
      }
      strcpyW(buf, curPath);
      return 0;
    }
    penv = *end ? end + 1 : end;
  } while(1);
}
