/*
 * msvcrt.dll errno functions
 *
 * Copyright 2000 Jon Griffiths
 */

#include "msvcrt.h"
#include "ms_errno.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);


/* INTERNAL: Set the crt and dos errno's from the OS error given. */
void MSVCRT__set_errno(int err)
{
  int *errno = GET_THREAD_VAR_PTR(errno);
  int *doserrno = GET_THREAD_VAR_PTR(doserrno);

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
int *__cdecl MSVCRT__errno(void)
{
  return GET_THREAD_VAR_PTR(errno);
}

/*********************************************************************
 *		__doserrno (MSVCRT.@)
 */
int *__cdecl MSVCRT___doserrno(void)
{
  return GET_THREAD_VAR_PTR(doserrno);
}

char *strerror(int);

/*********************************************************************
 *		strerror (MSVCRT.@)
 */
char * __cdecl MSVCRT_strerror (int err)
{
  return strerror(err); /* FIXME */
}

/**********************************************************************
 *		_strerror	(MSVCRT.@)
 */
extern int sprintf(char *str, const char *format, ...);

const char *__cdecl MSVCRT__strerror (const char *err)
{
  static char strerrbuff[256]; /* FIXME: Per thread, nprintf */
  sprintf(strerrbuff,"%s: %s\n",err,MSVCRT_strerror(GET_THREAD_VAR(errno)));
  return strerrbuff;
}

int __cdecl MSVCRT__cprintf( const char * format, ... );

/*********************************************************************
 *		perror (MSVCRT.@)
 */
void __cdecl MSVCRT_perror (const char *str)
{
  MSVCRT__cprintf("%s: %s\n",str,MSVCRT_strerror(GET_THREAD_VAR(errno)));
}
