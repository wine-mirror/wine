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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>

#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* error strings generated with glibc strerror */
char str_success[]       = "Success";
char str_EPERM[]         = "Operation not permitted";
char str_ENOENT[]        = "No such file or directory";
char str_ESRCH[]         = "No such process";
char str_EINTR[]         = "Interrupted system call";
char str_EIO[]           = "Input/output error";
char str_ENXIO[]         = "No such device or address";
char str_E2BIG[]         = "Argument list too long";
char str_ENOEXEC[]       = "Exec format error";
char str_EBADF[]         = "Bad file descriptor";
char str_ECHILD[]        = "No child processes";
char str_EAGAIN[]        = "Resource temporarily unavailable";
char str_ENOMEM[]        = "Cannot allocate memory";
char str_EACCES[]        = "Permission denied";
char str_EFAULT[]        = "Bad address";
char str_EBUSY[]         = "Device or resource busy";
char str_EEXIST[]        = "File exists";
char str_EXDEV[]         = "Invalid cross-device link";
char str_ENODEV[]        = "No such device";
char str_ENOTDIR[]       = "Not a directory";
char str_EISDIR[]        = "Is a directory";
char str_EINVAL[]        = "Invalid argument";
char str_ENFILE[]        = "Too many open files in system";
char str_EMFILE[]        = "Too many open files";
char str_ENOTTY[]        = "Inappropriate ioctl for device";
char str_EFBIG[]         = "File too large";
char str_ENOSPC[]        = "No space left on device";
char str_ESPIPE[]        = "Illegal seek";
char str_EROFS[]         = "Read-only file system";
char str_EMLINK[]        = "Too many links";
char str_EPIPE[]         = "Broken pipe";
char str_EDOM[]          = "Numerical argument out of domain";
char str_ERANGE[]        = "Numerical result out of range";
char str_EDEADLK[]       = "Resource deadlock avoided";
char str_ENAMETOOLONG[]  = "File name too long";
char str_ENOLCK[]        = "No locks available";
char str_ENOSYS[]        = "Function not implemented";
char str_ENOTEMPTY[]     = "Directory not empty";
char str_EILSEQ[]        = "Invalid or incomplete multibyte or wide character";
char str_generic_error[] = "Unknown error";

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

/* INTERNAL: Set the crt and dos errno's from the OS error given. */
void msvcrt_set_errno(int err)
{
  int *errno = MSVCRT__errno();
  unsigned long *doserrno = MSVCRT___doserrno();

  *doserrno = err;

  switch(err)
  {
#define ERR_CASE(oserr) case oserr:
#define ERR_MAPS(oserr,crterr) case oserr:*errno = crterr;break;
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
    ERR_CASE(ERROR_NOT_ENOUGH_QUOTA);
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
    *errno = MSVCRT_EINVAL;
  }
}

/*********************************************************************
 *		_errno (MSVCRT.@)
 */
int* MSVCRT__errno(void)
{
    return &msvcrt_get_thread_data()->thread_errno;
}

/*********************************************************************
 *		__doserrno (MSVCRT.@)
 */
unsigned long* MSVCRT___doserrno(void)
{
    return &msvcrt_get_thread_data()->thread_doserrno;
}

/*********************************************************************
 *		strerror (MSVCRT.@)
 */
char* MSVCRT_strerror(int err)
{
  return strerror(err); /* FIXME */
}

/**********************************************************************
 *		_strerror	(MSVCRT.@)
 */
char* _strerror(const char* err)
{
  static char strerrbuff[256]; /* FIXME: Per thread, nprintf */
  sprintf(strerrbuff,"%s: %s\n",err,MSVCRT_strerror(msvcrt_get_thread_data()->thread_errno));
  return strerrbuff;
}

/*********************************************************************
 *		perror (MSVCRT.@)
 */
void MSVCRT_perror(const char* str)
{
  _cprintf("%s: %s\n",str,MSVCRT_strerror(msvcrt_get_thread_data()->thread_errno));
}

/******************************************************************************
 *		_set_error_mode (MSVCRT.@)
 *
 * Set the error mode, which describes where the C run-time writes error
 * messages.
 *
 * PARAMS
 *   mode - the new error mode
 *
 * RETURNS
 *   The old error mode.
 *
 * TODO
 *  This function does not have a proper implementation; the error mode is
 *  never used.
 */
int _set_error_mode(int mode)
{
  static int current_mode = MSVCRT__OUT_TO_DEFAULT;

  const int old = current_mode;
  if ( MSVCRT__REPORT_ERRMODE != mode ) {
    current_mode = mode;
    FIXME("dummy implementation (old mode: %d, new mode: %d)\n",
          old, mode);
  }
  return old;
}

/******************************************************************************
 *		_seterrormode (MSVCRT.@)
 */
void _seterrormode(int mode)
{
    SetErrorMode( mode );
}
