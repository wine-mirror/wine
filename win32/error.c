/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <errno.h>
#include "windows.h"
#include "winerror.h"
#include "stddebug.h"
#include "debug.h"

/* WIN32_LastError contains the last error that occurred in the
 * Win32 API.  This value should be stored separately for each
 * thread, when we eventually get thread support.
 */
static int WIN32_LastError;

/* The errno_xlat_table contains the errno-to-Win32 error
 * mapping.  Since this is a single table, it can't easily
 * take into account function-specific differences, so there
 * will probably be quite a few points where we don't exactly
 * match what NT would return.  Then again, neither does
 * Windows 95. :-)
 */
typedef struct {
    int         errno;
    DWORD       win32err;
} ERRNO_XLAT_TABLE;

/* The table looks pretty ugly due to the preprocessor stuff,
 * but I honestly have no idea how many of these values are
 * portable.  I'm not even sure how many of them are even
 * used at all. :-)
 */
static ERRNO_XLAT_TABLE errno_xlat_table[] = {
#if defined(EPERM)
    {   EPERM,          ERROR_ACCESS_DENIED             },
#endif
#if defined(ENOENT)
    {   ENOENT,         ERROR_FILE_NOT_FOUND            },
#endif
#if defined(ESRCH)
    {   ESRCH,          ERROR_INVALID_PARAMETER         },
#endif
#if defined(EIO)
    {   EIO,            ERROR_IO_DEVICE                 },
#endif
#if defined(ENOEXEC)
    {   ENOEXEC,        ERROR_BAD_FORMAT                },
#endif
#if defined(EBADF)
    {   EBADF,          ERROR_INVALID_HANDLE            },
#endif
#if defined(ENOMEM)
    {   ENOMEM,         ERROR_OUTOFMEMORY               },
#endif
#if defined(EACCES)
    {   EACCES,         ERROR_ACCESS_DENIED             },
#endif
#if defined(EBUSY)
    {   EBUSY,          ERROR_BUSY                      },
#endif
#if defined(EEXIST)
    {   EEXIST,         ERROR_FILE_EXISTS               },
#endif
#if defined(ENODEV)
    {   ENODEV,         ERROR_BAD_DEVICE                },
#endif
#if defined(EINVAL)
    {   EINVAL,         ERROR_INVALID_PARAMETER         },
#endif
#if defined(EMFILE)
    {   EMFILE,         ERROR_TOO_MANY_OPEN_FILES       },
#endif
#if defined(ETXTBSY)
    {   ETXTBSY,        ERROR_BUSY,                     },
#endif
#if defined(ENOSPC)
    {   ENOSPC,         ERROR_DISK_FULL                 },
#endif
#if defined(ESPIPE)
    {   ESPIPE,         ERROR_SEEK_ON_DEVICE            },
#endif
#if defined(EPIPE)
    {   EPIPE,          ERROR_BROKEN_PIPE               },
#endif
#if defined(EDEADLK)
    {   EDEADLK,        ERROR_POSSIBLE_DEADLOCK         },
#endif
#if defined(ENAMETOOLONG)
    {   ENAMETOOLONG,   ERROR_FILENAME_EXCED_RANGE      },
#endif
#if defined(ENOTEMPTY)
    {   ENOTEMPTY,      ERROR_DIR_NOT_EMPTY             },
#endif
    {   -1,             0                               }
};

/**********************************************************************
 *              GetLastError            (KERNEL32.227)
 */
DWORD GetLastError(void)
{
    return WIN32_LastError;
}

/**********************************************************************
 *              SetLastError            (KERNEL32.497)
 *
 * This is probably not used by apps too much, but it's useful for
 * our own internal use.
 */
void SetLastError(DWORD error)
{
    WIN32_LastError = error;
}

DWORD ErrnoToLastError(int errno_num)
{
    DWORD rc = ERROR_UNKNOWN;
    int i = 0;

    while(errno_xlat_table[i].errno != -1)
    {
        if(errno_xlat_table[i].errno == errno_num)
        {
            rc = errno_xlat_table[i].win32err;
            break;
        }
        i++;
    }

    return rc;
}
