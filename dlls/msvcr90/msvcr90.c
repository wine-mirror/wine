/*
 * msvcr90 specific functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include "stdlib.h"
#include "errno.h"
#include "malloc.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "sys/stat.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcr90);

/*********************************************************************
 *  DllMain (MSVCR90.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
    }
    return TRUE;
}

/*********************************************************************
 *  _decode_pointer (MSVCR90.@)
 *
 * cdecl version of DecodePointer
 *
 */
void * CDECL MSVCR90_decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

/*********************************************************************
 *  _encode_pointer (MSVCR90.@)
 *
 * cdecl version of EncodePointer
 *
 */
void * CDECL MSVCR90_encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

/*********************************************************************
 *  _encoded_null (MSVCR90.@)
 */
void * CDECL _encoded_null(void)
{
    TRACE("\n");

    return MSVCR90_encode_pointer(NULL);
}

/*********************************************************************
 * _invalid_parameter_noinfo (MSVCR90.@)
 */
void CDECL _invalid_parameter_noinfo(void)
{
    _invalid_parameter( NULL, NULL, NULL, 0, 0 );
}

/*********************************************************************
 * __sys_nerr (MSVCR90.@)
 */
int* CDECL __sys_nerr(void)
{
        return (int*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_nerr");
}

/*********************************************************************
 *  __sys_errlist (MSVCR90.@)
 */
char** CDECL __sys_errlist(void)
{
    return (char**)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_errlist");
}

/*********************************************************************
 * __clean_type_info_names_internal (MSVCR90.@)
 */
void CDECL __clean_type_info_names_internal(void *p)
{
    FIXME("(%p) stub\n", p);
}

/*********************************************************************
 * _recalloc (MSVCR90.@)
 */
void* CDECL _recalloc(void* mem, size_t num, size_t size)
{
    size_t old_size;
    void *ret;

    if(!mem)
        return calloc(num, size);

    size = num*size;
    old_size = _msize(mem);

    ret = realloc(mem, size);
    if(!ret) {
        *_errno() = ENOMEM;
        return NULL;
    }

    if(size>old_size)
        memset((BYTE*)mem+old_size, 0, size-old_size);
    return ret;
}

/*********************************************************************
 *		_fstat64i32 (MSVCRT.@)
 */

static void msvcrt_stat64_to_stat64i32(const struct _stat64 *buf64, struct _stat64i32 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

int CDECL _fstat64i32(int fd, struct _stat64i32* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat64i32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_stat64i32 (MSVCRT.@)
 */
int CDECL _stat64i32(const char* path, struct _stat64i32 * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _stat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stat64i32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *              _wstat64i32 (MSVCRT.@)
 */
int CDECL _wstat64i32(const wchar_t *path, struct _stat64i32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}
