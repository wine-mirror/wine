/*
 * msvcrt.dll errno functions
 *
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "msvcrt.h"
#include "winnls.h"
#include "excpt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* error strings generated with glibc strerror */
static char str_success[]       = "Success";
static char str_EPERM[]         = "Operation not permitted";
static char str_ENOENT[]        = "No such file or directory";
static char str_ESRCH[]         = "No such process";
static char str_EINTR[]         = "Interrupted system call";
static char str_EIO[]           = "Input/output error";
static char str_ENXIO[]         = "No such device or address";
static char str_E2BIG[]         = "Argument list too long";
static char str_ENOEXEC[]       = "Exec format error";
static char str_EBADF[]         = "Bad file descriptor";
static char str_ECHILD[]        = "No child processes";
static char str_EAGAIN[]        = "Resource temporarily unavailable";
static char str_ENOMEM[]        = "Cannot allocate memory";
static char str_EACCES[]        = "Permission denied";
static char str_EFAULT[]        = "Bad address";
static char str_EBUSY[]         = "Device or resource busy";
static char str_EEXIST[]        = "File exists";
static char str_EXDEV[]         = "Invalid cross-device link";
static char str_ENODEV[]        = "No such device";
static char str_ENOTDIR[]       = "Not a directory";
static char str_EISDIR[]        = "Is a directory";
static char str_EINVAL[]        = "Invalid argument";
static char str_ENFILE[]        = "Too many open files in system";
static char str_EMFILE[]        = "Too many open files";
static char str_ENOTTY[]        = "Inappropriate ioctl for device";
static char str_EFBIG[]         = "File too large";
static char str_ENOSPC[]        = "No space left on device";
static char str_ESPIPE[]        = "Illegal seek";
static char str_EROFS[]         = "Read-only file system";
static char str_EMLINK[]        = "Too many links";
static char str_EPIPE[]         = "Broken pipe";
static char str_EDOM[]          = "Numerical argument out of domain";
static char str_ERANGE[]        = "Numerical result out of range";
static char str_EDEADLK[]       = "Resource deadlock avoided";
static char str_ENAMETOOLONG[]  = "File name too long";
static char str_ENOLCK[]        = "No locks available";
static char str_ENOSYS[]        = "Function not implemented";
static char str_ENOTEMPTY[]     = "Directory not empty";
static char str_EILSEQ[]        = "Invalid or incomplete multibyte or wide character";
static char str_generic_error[] = "Unknown error";

char *MSVCRT__sys_errlist[] =
{
    str_success,
    str_EPERM,
    str_ENOENT,
    str_ESRCH,
    str_EINTR,
    str_EIO,
    str_ENXIO,
    str_E2BIG,
    str_ENOEXEC,
    str_EBADF,
    str_ECHILD,
    str_EAGAIN,
    str_ENOMEM,
    str_EACCES,
    str_EFAULT,
    str_generic_error,
    str_EBUSY,
    str_EEXIST,
    str_EXDEV,
    str_ENODEV,
    str_ENOTDIR,
    str_EISDIR,
    str_EINVAL,
    str_ENFILE,
    str_EMFILE,
    str_ENOTTY,
    str_generic_error,
    str_EFBIG,
    str_ENOSPC,
    str_ESPIPE,
    str_EROFS,
    str_EMLINK,
    str_EPIPE,
    str_EDOM,
    str_ERANGE,
    str_generic_error,
    str_EDEADLK,
    str_generic_error,
    str_ENAMETOOLONG,
    str_ENOLCK,
    str_ENOSYS,
    str_ENOTEMPTY,
    str_EILSEQ,
    str_generic_error
};

unsigned int MSVCRT__sys_nerr = sizeof(MSVCRT__sys_errlist)/sizeof(MSVCRT__sys_errlist[0]) - 1;

static MSVCRT_invalid_parameter_handler invalid_parameter_handler = NULL;

/* INTERNAL: Set the crt and dos errno's from the OS error given. */
void msvcrt_set_errno(int err)
{
  int *errno_ptr = MSVCRT__errno();
  MSVCRT_ulong *doserrno = MSVCRT___doserrno();

  *doserrno = err;

  switch(err)
  {
#define ERR_CASE(oserr) case oserr:
#define ERR_MAPS(oserr, crterr) case oserr: *errno_ptr = crterr; break
    ERR_CASE(ERROR_ACCESS_DENIED)
    ERR_CASE(ERROR_NETWORK_ACCESS_DENIED)
    ERR_CASE(ERROR_CANNOT_MAKE)
    ERR_CASE(ERROR_SEEK_ON_DEVICE)
    ERR_CASE(ERROR_LOCK_FAILED)
    ERR_CASE(ERROR_FAIL_I24)
    ERR_CASE(ERROR_CURRENT_DIRECTORY)
    ERR_CASE(ERROR_DRIVE_LOCKED)
    ERR_CASE(ERROR_NOT_LOCKED)
    ERR_CASE(ERROR_INVALID_ACCESS)
    ERR_CASE(ERROR_SHARING_VIOLATION)
    ERR_MAPS(ERROR_LOCK_VIOLATION,       MSVCRT_EACCES);
    ERR_CASE(ERROR_FILE_NOT_FOUND)
    ERR_CASE(ERROR_NO_MORE_FILES)
    ERR_CASE(ERROR_BAD_PATHNAME)
    ERR_CASE(ERROR_BAD_NETPATH)
    ERR_CASE(ERROR_INVALID_DRIVE)
    ERR_CASE(ERROR_BAD_NET_NAME)
    ERR_CASE(ERROR_FILENAME_EXCED_RANGE)
    ERR_MAPS(ERROR_PATH_NOT_FOUND,       MSVCRT_ENOENT);
    ERR_MAPS(ERROR_IO_DEVICE,            MSVCRT_EIO);
    ERR_MAPS(ERROR_BAD_FORMAT,           MSVCRT_ENOEXEC);
    ERR_MAPS(ERROR_INVALID_HANDLE,       MSVCRT_EBADF);
    ERR_CASE(ERROR_OUTOFMEMORY)
    ERR_CASE(ERROR_INVALID_BLOCK)
    ERR_CASE(ERROR_NOT_ENOUGH_QUOTA)
    ERR_MAPS(ERROR_ARENA_TRASHED,        MSVCRT_ENOMEM);
    ERR_MAPS(ERROR_BUSY,                 MSVCRT_EBUSY);
    ERR_CASE(ERROR_ALREADY_EXISTS)
    ERR_MAPS(ERROR_FILE_EXISTS,          MSVCRT_EEXIST);
    ERR_MAPS(ERROR_BAD_DEVICE,           MSVCRT_ENODEV);
    ERR_MAPS(ERROR_TOO_MANY_OPEN_FILES,  MSVCRT_EMFILE);
    ERR_MAPS(ERROR_DISK_FULL,            MSVCRT_ENOSPC);
    ERR_MAPS(ERROR_BROKEN_PIPE,          MSVCRT_EPIPE);
    ERR_MAPS(ERROR_POSSIBLE_DEADLOCK,    MSVCRT_EDEADLK);
    ERR_MAPS(ERROR_DIR_NOT_EMPTY,        MSVCRT_ENOTEMPTY);
    ERR_MAPS(ERROR_BAD_ENVIRONMENT,      MSVCRT_E2BIG);
    ERR_CASE(ERROR_WAIT_NO_CHILDREN)
    ERR_MAPS(ERROR_CHILD_NOT_COMPLETE,   MSVCRT_ECHILD);
    ERR_CASE(ERROR_NO_PROC_SLOTS)
    ERR_CASE(ERROR_MAX_THRDS_REACHED)
    ERR_MAPS(ERROR_NESTING_NOT_ALLOWED,  MSVCRT_EAGAIN);
  default:
    /*  Remaining cases map to EINVAL */
    /* FIXME: may be missing some errors above */
    *errno_ptr = MSVCRT_EINVAL;
  }
}

/*********************************************************************
 * __sys_nerr (MSVCR100.@)
 */
int* CDECL __sys_nerr(void)
{
    return (int*)&MSVCRT__sys_nerr;
}

/*********************************************************************
 *  __sys_errlist (MSVCR100.@)
 */
char** CDECL __sys_errlist(void)
{
    return MSVCRT__sys_errlist;
}

/*********************************************************************
 *		_errno (MSVCRT.@)
 */
int* CDECL MSVCRT__errno(void)
{
    return &msvcrt_get_thread_data()->thread_errno;
}

/*********************************************************************
 *		__doserrno (MSVCRT.@)
 */
MSVCRT_ulong* CDECL MSVCRT___doserrno(void)
{
    return &msvcrt_get_thread_data()->thread_doserrno;
}

/*********************************************************************
 *		_get_errno (MSVCRT.@)
 */
int CDECL _get_errno(int *pValue)
{
    if (!pValue)
        return MSVCRT_EINVAL;

    *pValue = *MSVCRT__errno();
    return 0;
}

/*********************************************************************
 *		_get_doserrno (MSVCRT.@)
 */
int CDECL _get_doserrno(int *pValue)
{
    if (!pValue)
        return MSVCRT_EINVAL;

    *pValue = *MSVCRT___doserrno();
    return 0;
}

/*********************************************************************
 *		_set_errno (MSVCRT.@)
 */
int CDECL _set_errno(int value)
{
    *MSVCRT__errno() = value;
    return 0;
}

/*********************************************************************
 *		_set_doserrno (MSVCRT.@)
 */
int CDECL _set_doserrno(int value)
{
    *MSVCRT___doserrno() = value;
    return 0;
}

/*********************************************************************
 *		strerror (MSVCRT.@)
 */
char* CDECL MSVCRT_strerror(int err)
{
    thread_data_t *data = msvcrt_get_thread_data();

    if (!data->strerror_buffer)
        if (!(data->strerror_buffer = MSVCRT_malloc(256))) return NULL;

    if (err < 0 || err > MSVCRT__sys_nerr) err = MSVCRT__sys_nerr;
    strcpy( data->strerror_buffer, MSVCRT__sys_errlist[err] );
    return data->strerror_buffer;
}

/**********************************************************************
 *		strerror_s	(MSVCRT.@)
 */
int CDECL MSVCRT_strerror_s(char *buffer, MSVCRT_size_t numberOfElements, int errnum)
{
    char *ptr;

    if (!buffer || !numberOfElements)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if (errnum < 0 || errnum > MSVCRT__sys_nerr)
        errnum = MSVCRT__sys_nerr;

    ptr = MSVCRT__sys_errlist[errnum];
    while (*ptr && numberOfElements > 1)
    {
        *buffer++ = *ptr++;
        numberOfElements--;
    }

    *buffer = '\0';
    return 0;
}

/**********************************************************************
 *		_strerror	(MSVCRT.@)
 */
char* CDECL MSVCRT__strerror(const char* str)
{
    thread_data_t *data = msvcrt_get_thread_data();
    int err;

    if (!data->strerror_buffer)
        if (!(data->strerror_buffer = MSVCRT_malloc(256))) return NULL;

    err = data->thread_errno;
    if (err < 0 || err > MSVCRT__sys_nerr) err = MSVCRT__sys_nerr;

    if (str && *str)
        sprintf( data->strerror_buffer, "%s: %s\n", str, MSVCRT__sys_errlist[err] );
    else
        sprintf( data->strerror_buffer, "%s\n", MSVCRT__sys_errlist[err] );

    return data->strerror_buffer;
}

/*********************************************************************
 *		perror (MSVCRT.@)
 */
void CDECL MSVCRT_perror(const char* str)
{
    int err = *MSVCRT__errno();
    if (err < 0 || err > MSVCRT__sys_nerr) err = MSVCRT__sys_nerr;

    if (str && *str)
    {
        MSVCRT__write( 2, str, strlen(str) );
        MSVCRT__write( 2, ": ", 2 );
    }
    MSVCRT__write( 2, MSVCRT__sys_errlist[err], strlen(MSVCRT__sys_errlist[err]) );
    MSVCRT__write( 2, "\n", 1 );
}

/*********************************************************************
 *		_wcserror_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcserror_s(MSVCRT_wchar_t* buffer, MSVCRT_size_t nc, int err)
{
    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(nc > 0)) return MSVCRT_EINVAL;

    if (err < 0 || err > MSVCRT__sys_nerr) err = MSVCRT__sys_nerr;
    MultiByteToWideChar(CP_ACP, 0, MSVCRT__sys_errlist[err], -1, buffer, nc);
    return 0;
}

/*********************************************************************
 *		_wcserror (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcserror(int err)
{
    thread_data_t *data = msvcrt_get_thread_data();

    if (!data->wcserror_buffer)
        if (!(data->wcserror_buffer = MSVCRT_malloc(256 * sizeof(MSVCRT_wchar_t)))) return NULL;
    MSVCRT__wcserror_s(data->wcserror_buffer, 256, err);
    return data->wcserror_buffer;
}

/**********************************************************************
 *		__wcserror_s	(MSVCRT.@)
 */
int CDECL MSVCRT___wcserror_s(MSVCRT_wchar_t* buffer, MSVCRT_size_t nc, const MSVCRT_wchar_t* str)
{
    int err;
    static const WCHAR colonW[] = {':', ' ', '\0'};
    static const WCHAR nlW[] = {'\n', '\0'};
    size_t len;

    err = *MSVCRT__errno();
    if (err < 0 || err > MSVCRT__sys_nerr) err = MSVCRT__sys_nerr;

    len = MultiByteToWideChar(CP_ACP, 0, MSVCRT__sys_errlist[err], -1, NULL, 0) + 1 /* \n */;
    if (str && *str) len += lstrlenW(str) + 2 /* ': ' */;
    if (len > nc)
    {
        MSVCRT_INVALID_PMT("buffer[nc] is too small", MSVCRT_ERANGE);
        return MSVCRT_ERANGE;
    }
    if (str && *str)
    {
        lstrcpyW(buffer, str);
        lstrcatW(buffer, colonW);
    }
    else buffer[0] = '\0';
    len = lstrlenW(buffer);
    MultiByteToWideChar(CP_ACP, 0, MSVCRT__sys_errlist[err], -1, buffer + len, 256 - len);
    lstrcatW(buffer, nlW);

    return 0;
}

/**********************************************************************
 *		__wcserror	(MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT___wcserror(const MSVCRT_wchar_t* str)
{
    thread_data_t *data = msvcrt_get_thread_data();
    int err;

    if (!data->wcserror_buffer)
        if (!(data->wcserror_buffer = MSVCRT_malloc(256 * sizeof(MSVCRT_wchar_t)))) return NULL;

    err = MSVCRT___wcserror_s(data->wcserror_buffer, 256, str);
    if (err) FIXME("bad wcserror call (%d)\n", err);

    return data->wcserror_buffer;
}

/******************************************************************************
 *		_seterrormode (MSVCRT.@)
 */
void CDECL _seterrormode(int mode)
{
    SetErrorMode( mode );
}

/******************************************************************************
 *		_invalid_parameter (MSVCRT.@)
 */
void __cdecl MSVCRT__invalid_parameter(const MSVCRT_wchar_t *expr, const MSVCRT_wchar_t *func,
                                       const MSVCRT_wchar_t *file, unsigned int line, MSVCRT_uintptr_t arg)
{
    if (invalid_parameter_handler) invalid_parameter_handler( expr, func, file, line, arg );
    else
    {
        ERR( "%s:%u %s: %s %lx\n", debugstr_w(file), line, debugstr_w(func), debugstr_w(expr), arg );
#if _MSVCR_VER > 0
        RaiseException( STATUS_INVALID_CRUNTIME_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, NULL );
#endif
    }
}

/*********************************************************************
 * _invalid_parameter_noinfo (MSVCR100.@)
 */
void CDECL _invalid_parameter_noinfo(void)
{
    MSVCRT__invalid_parameter( NULL, NULL, NULL, 0, 0 );
}

/*********************************************************************
 * _get_invalid_parameter_handler (MSVCR80.@)
 */
MSVCRT_invalid_parameter_handler CDECL _get_invalid_parameter_handler(void)
{
    TRACE("\n");
    return invalid_parameter_handler;
}

/*********************************************************************
 * _set_invalid_parameter_handler (MSVCR80.@)
 */
MSVCRT_invalid_parameter_handler CDECL _set_invalid_parameter_handler(
        MSVCRT_invalid_parameter_handler handler)
{
    MSVCRT_invalid_parameter_handler old = invalid_parameter_handler;

    TRACE("(%p)\n", handler);

    invalid_parameter_handler = handler;
    return old;
}
